// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace
{ // UNNAMED NAMESPACE
// ==========================================================================
// Warrior
// Fix/check Arms autoattack rage gain when it is playable
// try to not break protection
// Add Intercept
// Add back second wind
// Check if endless rage affects white swing rage before or after rounding, whenever they fix it in game.
// Check meat cleaver range as beta testing proceeds, it was 10, currently it's 5.
// ==========================================================================

struct warrior_t;

struct warrior_td_t: public actor_target_data_t
{
  dot_t* dots_deep_wounds;
  dot_t* dots_ravager;
  dot_t* dots_rend;
  dot_t* dots_trauma;
  buff_t* debuffs_colossus_smash;
  buff_t* debuffs_demoralizing_shout;
  buff_t* debuffs_taunt;

  warrior_t& warrior;
  warrior_td_t( player_t* target, warrior_t& p );
};

struct warrior_t: public player_t
{
public:
  event_t* heroic_charge;
  event_t* rampage_driver;
  std::vector<attack_t*> rampage_attacks;
  int initial_rage;
  bool non_dps_mechanics, warrior_fixed_time;

  // Tier 18 (WoD 6.2) class specific trinket effects
  const special_effect_t* fury_trinket;
  const special_effect_t* arms_trinket;
  const special_effect_t* prot_trinket;

  // Artifacts
  const special_effect_t* stromkar_the_warbreaker;
  const special_effect_t* warswords_of_the_valarjar;

  // Active
  struct active_t
  {
    action_t* deep_wounds;
    action_t* corrupted_blood_of_zakajz;
    action_t* opportunity_strikes;
    action_t* trauma;
  } active;

  // Buffs
  struct buffs_t
  {
    buff_t* fury_trinket;
    buff_t* bounding_stride;
    buff_t* massacre;
    buff_t* frothing_berserker;
    buff_t* meat_grinder;
    buff_t* berserking; // Artifact Weapon
    buff_t* berserking_driver;
    buff_t* sense_death;
    // All Warriors
    buff_t* berserker_rage;
    buff_t* bladestorm;
    buff_t* charge_movement;
    buff_t* heroic_leap_movement;
    buff_t* intervene_movement;
    // Arms and Fury
    buff_t* avatar;
    buff_t* battle_cry;
    buff_t* commanding_shout;
    buff_t* die_by_the_sword;
    buff_t* dragon_roar;
    buff_t* overpower;
    buff_t* ravager;
    // Fury Only
    buff_t* enrage;
    buff_t* frenzy;
    buff_t* meat_cleaver;
    buff_t* taste_for_blood;
    buff_t* wrecking_ball;
    // Arms only
    buff_t* colossus_smash;
    buff_t* corrupted_blood_of_zakajz;
    buff_t* cleave;
    buff_t* focused_rage;
    buff_t* precise_strikes;
    buff_t* shattered_defenses;
    // Prot only
    buff_t* last_stand;
    buff_t* ravager_protection;
    buff_t* shield_block;
    buff_t* shield_wall;
    buff_t* sword_and_board;
    // Arms and Prot
    buff_t* defensive_stance;

    // Tier bonuses
    buff_t* tier17_2pc_arms;
    buff_t* tier17_4pc_fury;
    buff_t* tier17_4pc_fury_driver;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    // All Warriors
    cooldown_t* charge;
    cooldown_t* heroic_leap;
    cooldown_t* rage_from_charge;
    // Talents
    cooldown_t* shockwave;
    // Fury And Arms
    cooldown_t* battle_cry;
    // Arms only
    cooldown_t* colossus_smash;
    cooldown_t* mortal_strike;
    // Prot Only
    cooldown_t* demoralizing_shout;
    cooldown_t* last_stand;
    cooldown_t* rage_from_crit_block;
    cooldown_t* revenge;
    cooldown_t* revenge_reset;
    cooldown_t* shield_slam;
    cooldown_t* shield_wall;
  } cooldown;

  // Gains
  struct gains_t
  {
    // All Warriors
    gain_t* avoided_attacks;
    gain_t* charge;
    gain_t* melee_main_hand;
    // Fury Only
    gain_t* bloodthirst;
    gain_t* melee_off_hand;
    gain_t* raging_blow;
    gain_t* battle_cry;
    // Arms Only
    gain_t* in_for_the_kill;
    gain_t* melee_crit;
    gain_t* fervor_of_battle;
    gain_t* will_of_the_first_king;
    // Prot Only
    gain_t* critical_block;
    gain_t* revenge;
    gain_t* shield_slam;
    gain_t* sword_and_board;
    // Tier bonuses
    gain_t* tier17_4pc_arms;
  } gain;

  // Spells
  struct spells_t
  {
    const spell_data_t* charge;
    const spell_data_t* colossus_smash_debuff;
    const spell_data_t* defensive_stance;
    const spell_data_t* fervor_of_battle;
    const spell_data_t* indomitable;
    const spell_data_t* intervene;
    const spell_data_t* headlong_rush;
    const spell_data_t* heroic_leap;
    const spell_data_t* overpower_driver;
    const spell_data_t* revenge_trigger;
    const spell_data_t* t17_prot_2p;
  } spell;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* critical_block; //Protection
    const spell_data_t* unshackled_fury; //Fury
    const spell_data_t* colossal_might; //Arms
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* arms_trinket;
    proc_t* delayed_auto_attack;
    proc_t* tactician;

    //Tier bonuses
    proc_t* t17_2pc_arms;
    proc_t* t17_2pc_fury;
  } proc;

  struct realppm_t
  {
    std::unique_ptr<real_ppm_t> overpower;
    std::unique_ptr<real_ppm_t> shadow_slash;
    std::unique_ptr<real_ppm_t> rage_of_the_valarjar;
    std::unique_ptr<real_ppm_t> wrecking_ball;
  } rppm;

  // Spec Passives
  struct spec_t
  {
    //All specs
    const spell_data_t* execute;
    //Arms only
    const spell_data_t* cleave;
    const spell_data_t* colossus_smash;
    const spell_data_t* hamstring;
    const spell_data_t* mortal_strike;
    const spell_data_t* seasoned_soldier;
    const spell_data_t* slam;
    const spell_data_t* tactician;
    //Arms and Fury
    const spell_data_t* die_by_the_sword;
    const spell_data_t* commanding_shout;
    const spell_data_t* battle_cry;
    const spell_data_t* whirlwind;
    //Fury only
    const spell_data_t* bloodthirst;
    const spell_data_t* enrage;
    const spell_data_t* enraged_regeneration;
    const spell_data_t* furious_slash;
    const spell_data_t* meat_cleaver;
    const spell_data_t* piercing_howl;
    const spell_data_t* raging_blow;
    const spell_data_t* rampage;
    const spell_data_t* singleminded_fury;
    const spell_data_t* titans_grip;
    //Prot only
    const spell_data_t* bastion_of_defense;
    const spell_data_t* bladed_armor;
    const spell_data_t* deep_wounds;
    const spell_data_t* demoralizing_shout;
    const spell_data_t* devastate;
    const spell_data_t* focused_rage;
    const spell_data_t* last_stand;
    const spell_data_t* protection; // Weird spec passive that increases damage of bladestorm/execute.
    const spell_data_t* resolve;
    const spell_data_t* revenge;
    const spell_data_t* riposte;
    const spell_data_t* shield_block;
    const spell_data_t* shield_slam;
    const spell_data_t* shield_wall;
    const spell_data_t* sword_and_board;
    const spell_data_t* thunder_clap;
    const spell_data_t* unwavering_sentinel;
  } spec;

  // Talents
  struct talents_t
  {
    const spell_data_t* war_machine;
    const spell_data_t* endless_rage;
    const spell_data_t* fresh_meat;
    const spell_data_t* dauntless;
    const spell_data_t* overpower;
    const spell_data_t* sweeping_strikes;
    const spell_data_t* warbringer;

    const spell_data_t* shockwave;
    const spell_data_t* storm_bolt;
    const spell_data_t* double_time;
    const spell_data_t* inspiring_presence;

    const spell_data_t* wrecking_ball;
    const spell_data_t* outburst;
    const spell_data_t* avatar;
    const spell_data_t* fervor_of_battle;
    const spell_data_t* rend;
    const spell_data_t* renewed_fury; //
    const spell_data_t* ultimatum; //

    const spell_data_t* furious_charge; //
    const spell_data_t* bounding_stride;
    const spell_data_t* warpaint; //
    const spell_data_t* second_wind; //
    const spell_data_t* die_by_the_sword; //
    const spell_data_t* vigilance; //
    const spell_data_t* crackling_thunder; //

    const spell_data_t* massacre;
    const spell_data_t* frothing_berserker;
    const spell_data_t* bladestorm;
    const spell_data_t* in_for_the_kill;
    const spell_data_t* mortal_combo;
    const spell_data_t* best_served_cold; //
    const spell_data_t* never_surrender; //
    const spell_data_t* indomitable; //

    const spell_data_t* meat_grinder;
    const spell_data_t* frenzy;
    const spell_data_t* inner_rage;
    const spell_data_t* focused_rage;
    const spell_data_t* trauma;
    const spell_data_t* titanic_might;
    const spell_data_t* vengeance;
    const spell_data_t* into_the_fray;
    const spell_data_t* booming_voice;

    const spell_data_t* carnage;
    const spell_data_t* reckless_abandon;
    const spell_data_t* dragon_roar;
    const spell_data_t* anger_management;
    const spell_data_t* opportunity_strikes;
    const spell_data_t* ravager;
    const spell_data_t* heavy_repercussions;
  } talents;

  // Artifacts
  struct artifact_spell_data_t
  {
    // Arms - Strom'kar the Warbreaker
    artifact_power_t corrupted_rage;
    artifact_power_t exploit_the_weakness;
    artifact_power_t war_veteran;
    artifact_power_t shattered_defenses;
    artifact_power_t deathblow;
    artifact_power_t one_against_many;
    artifact_power_t crushing_blows;
    artifact_power_t thoradins_might;
    artifact_power_t void_cleave;
    artifact_power_t precise_strikes;
    artifact_power_t focus_in_battle;
    artifact_power_t unending_rage;
    artifact_power_t corrupted_blood_of_zakajz;
    artifact_power_t will_of_the_first_king;

    artifact_power_t odyns_fury;
    artifact_power_t thirst_for_battle;
    artifact_power_t unstoppable;
    artifact_power_t raging_berserker;
    artifact_power_t battle_scars;
    artifact_power_t unrivaled_strength;
    artifact_power_t uncontrolled_rage;
    artifact_power_t rage_of_the_valarjar;
    artifact_power_t bloodcraze;
    artifact_power_t sense_death;
    artifact_power_t focus_in_chaos;
    artifact_power_t helyas_wrath;
    artifact_power_t odyns_champion;
    artifact_power_t wrath_and_fury;
    artifact_power_t deathdealer;
    artifact_power_t juggernaut;

    // NYI
    artifact_power_t touch_of_zakajz;
    artifact_power_t defensive_measures;
    artifact_power_t tactical_advance;
  } artifact;

  warrior_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ):
    player_t( sim, WARRIOR, name, r ),
    stromkar_the_warbreaker(),
    warswords_of_the_valarjar(),
    heroic_charge( nullptr ),
    rampage_driver( nullptr ),
    rampage_attacks( 0 ),
    active( active_t() ),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    spell( spells_t() ),
    mastery( mastery_t() ),
    proc( procs_t() ),
    rppm( realppm_t() ),
    spec( spec_t() ),
    talents( talents_t() )
  {
    initial_rage = 0;
    non_dps_mechanics = false; // When set to false, disables stuff that isn't important, such as second wind, bloodthirst heal, etc.
    warrior_fixed_time = true;
    base.distance = 5.0;

    fury_trinket = nullptr;
    arms_trinket = nullptr;
    prot_trinket = nullptr;
    regen_type = REGEN_DISABLED;
  }

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_position() override;
  virtual void      init_procs() override;
  virtual void      init_resources( bool ) override;
  virtual void      arise() override;
  virtual void      combat_begin() override;
  virtual double    composite_attribute( attribute_e attr ) const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_armor_multiplier() const override;
  virtual double    composite_block() const override;
  virtual double    composite_block_reduction() const override;
  virtual double    composite_parry_rating() const override;
  virtual double    composite_parry() const override;
  virtual double    composite_melee_expertise( const weapon_t* ) const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_mastery() const override;
  virtual double    composite_crit_block() const override;
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_melee_speed() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_player_critical_damage_multiplier() const override;
  virtual double    composite_leech() const override;
  virtual void      teleport( double yards, timespan_t duration ) override;
  virtual void      trigger_movement( double distance, movement_direction_e direction ) override;
  virtual void      interrupt() override;
  virtual void      reset() override;
  virtual void      moving() override;
  virtual void      init_rng() override;
  virtual void      create_options() override;
  virtual action_t* create_proc_action( const std::string& name, const special_effect_t& ) override;
  virtual std::string      create_profile( save_e type ) override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual double    temporary_movement_modifier() const override;
  virtual bool      has_t18_class_trinket() const override;

  void              apl_precombat();
  void              apl_default();
  void              apl_fury();
  void              apl_arms();
  void              apl_prot();
  virtual void      init_action_list() override;

  virtual action_t*  create_action( const std::string& name, const std::string& options ) override;
  virtual resource_e primary_resource() const override { return RESOURCE_RAGE; }
  virtual role_e     primary_role() const override;
  virtual stat_e     convert_hybrid_stat( stat_e s ) const override;
  virtual void       assess_damage( school_e, dmg_e, action_state_t* s ) override;
  virtual void       copy_from( player_t* source ) override;

  target_specific_t<warrior_td_t> target_data;

  virtual warrior_td_t* get_target_data( player_t* target ) const override
  {
    warrior_td_t*& td = target_data[target];

    if ( !td )
    {
      td = new warrior_td_t( target, const_cast<warrior_t&>( *this ) );
    }
    return td;
  }
};

namespace
{ // UNNAMED NAMESPACE
// Template for common warrior action code. See priest_action_t.
template <class Base>
struct warrior_action_t: public Base
{
  bool headlongrush, headlongrushgcd, sweeping_strikes, dauntless, war_veteran;
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef warrior_action_t base_t;
  warrior_action_t( const std::string& n, warrior_t* player,
                    const spell_data_t* s = spell_data_t::nil() ):
    ab( n, player, s ),
    headlongrush( ab::data().affected_by( player -> spell.headlong_rush -> effectN( 1 ) ) ),
    headlongrushgcd( ab::data().affected_by( player -> spell.headlong_rush -> effectN( 2 ) ) ),
    sweeping_strikes( ab::data().affected_by( player -> talents.sweeping_strikes -> effectN( 1 ) ) ),
    dauntless( ab::data().affected_by( player -> talents.dauntless -> effectN( 1 ) ) ),
    war_veteran( ab::data().affected_by( player -> artifact.war_veteran.data().effectN( 1 ).trigger() -> effectN( 1 ) ) )
  {
    ab::may_crit = true;
  }

  void init() override
  {
    ab::init();

    if ( sweeping_strikes )
      ab::aoe = p() -> talents.sweeping_strikes -> effectN( 1 ).base_value() + 1;
  }

  virtual ~warrior_action_t() {}

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
    return p() -> get_target_data( t );
  }

  virtual double cost() const override
  {
    double c = ab::cost();

    if ( dauntless )
      c *= 1.0 + p() -> talents.dauntless -> effectN( 1 ).percent();

    if ( war_veteran && p() -> buff.battle_cry -> check() )
      c *= 1.0 + p() -> artifact.war_veteran.data().effectN( 1 ).trigger() -> effectN( 1 ).percent();

    return c;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double am = ab::composite_target_multiplier( target );

    if ( td( target ) -> debuffs_colossus_smash -> up() )
    {
      am *= 1.0 + ( td( target ) -> debuffs_colossus_smash -> value() + p() -> cache.mastery_value() )
                * ( 1.0 + p() -> talents.titanic_might -> effectN( 2 ).percent() );
    }

    return am;
  }

  virtual void execute() override
  {
    ab::execute();
  }

  virtual timespan_t gcd() const override
  {
    timespan_t t = ab::action_t::gcd();

    if ( t == timespan_t::zero() )
      return t;

    if ( headlongrushgcd )
      t *= ab::player -> cache.attack_haste();
    if ( t < ab::min_gcd )
      t = ab::min_gcd;

    return t;
  }

  virtual double cooldown_reduction() const override
  {
    double cdr = ab::cooldown_reduction();

    if ( headlongrush )
    {
      cdr *= ab::player -> cache.attack_haste();
    }

    return cdr;
  }

  virtual bool ready() override
  {
    if ( !ab::ready() )
      return false;

    if ( p() -> current.distance_to_move > ab::range && ab::range != -1 )
      // -1 melee range implies that the ability can be used at any distance from the target.
      return false;

    return true;
  }

  bool usable_moving() const override
  { // All warrior abilities are usable while moving, the issue is being in range.
    return true;
  }

  virtual void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::sim -> log )
    {
      ab::sim -> out_debug.printf(
        "Strength: %4.4f, AP: %4.4f, Crit: %4.4f%%, Crit Dmg Mult: %4.4f,  Mastery: %4.4f%%, Haste: %4.4f%%, Versatility: %4.4f%%, Bonus Armor: %4.4f, Tick Multiplier: %4.4f, Direct Multiplier: %4.4f, Action Multiplier: %4.4f",
        p() -> cache.strength(),
        p() -> cache.attack_power() * p() -> composite_attack_power_multiplier(),
        p() -> cache.attack_crit() * 100,
        p() -> composite_player_critical_damage_multiplier(),
        p() -> cache.mastery_value() * 100,
        ( ( 1 / ( p() -> cache.attack_haste() ) ) - 1 ) * 100,
        p() -> cache.damage_versatility() * 100,
        p() -> cache.bonus_armor(),
        s -> composite_ta_multiplier(),
        s -> composite_da_multiplier(),
        s -> action -> action_multiplier() );
    }
  }

  virtual void tick( dot_t* d ) override
  {
    ab::tick( d );

    if ( ab::sim -> log )
    {
      ab::sim -> out_debug.printf(
        "Strength: %4.4f, AP: %4.4f, Crit: %4.4f%%, Crit Dmg Mult: %4.4f,  Mastery: %4.4f%%, Haste: %4.4f%%, Versatility: %4.4f%%, Bonus Armor: %4.4f, Tick Multiplier: %4.4f, Direct Multiplier: %4.4f, Action Multiplier: %4.4f",
        p() -> cache.strength(),
        p() -> cache.attack_power() * p() -> composite_attack_power_multiplier(),
        p() -> cache.attack_crit() * 100,
        p() -> composite_player_critical_damage_multiplier(),
        p() -> cache.mastery_value() * 100,
        ( ( 1 / ( p() -> cache.attack_haste() ) ) - 1 ) * 100,
        p() -> cache.damage_versatility() * 100,
        p() -> cache.bonus_armor(),
        d -> state -> composite_ta_multiplier(),
        d -> state -> composite_da_multiplier(),
        d -> state -> action -> action_ta_multiplier() );
    }
  }

  virtual void consume_resource() override
  {
    ab::consume_resource();

    double rage = ab::resource_consumed;

    if ( p() -> talents.anger_management -> ok() )
    {
      anger_management( rage );
    }

    if ( ab::result_is_miss( ab::execute_state -> result ) && rage > 0 && !ab::aoe )
      rage_resource_gain( RESOURCE_RAGE, rage*0.8, p() -> gain.avoided_attacks );
  }

  void anger_management( double rage )
  {
    if ( rage > 0 )
    {
      //Anger management takes the amount of rage spent and reduces the cooldown of abilities by 1 second per 10 rage.
      rage /= p() -> talents.anger_management -> effectN( 1 ).base_value();
      rage *= -1;

      if ( p() -> specialization() != WARRIOR_PROTECTION )
        p() -> cooldown.battle_cry -> adjust( timespan_t::from_seconds( rage ) );
      else
      {
        p() -> cooldown.last_stand -> adjust( timespan_t::from_seconds( rage ) );
        p() -> cooldown.shield_wall -> adjust( timespan_t::from_seconds( rage ) );
      }
    }
  }

  void enrage()
  {
    p() -> buff.enrage -> trigger();
  }

  void rage_resource_gain( resource_e resource_type, double amount, gain_t* gain )
  {
    p() -> resource_gain( resource_type, amount, gain );

    if ( p() -> talents.frothing_berserker -> ok() && p() -> resources.current[RESOURCE_RAGE] > 99 )
    {
       p() -> buff.frothing_berserker -> trigger(); 
    }
  }
};

struct warrior_heal_t: public warrior_action_t < heal_t >
{ // Main Warrior Heal Class
  warrior_heal_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( n, p, s )
  {
    may_crit = tick_may_crit = hasted_ticks = false;
    target = p;
  }
};

struct warrior_spell_t: public warrior_action_t < spell_t >
{ // Main Warrior Spell Class - Used for spells that deal no damage, usually buffs.
  warrior_spell_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( n, p, s )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }
};

struct warrior_attack_t: public warrior_action_t < melee_attack_t >
{ // Main Warrior Attack Class
  bool procs_overpower;
  bool procs_tactician;

  warrior_attack_t( const std::string& n, warrior_t* p,
                    const spell_data_t* s = spell_data_t::nil() ):
    base_t( n, p, s ),
    procs_overpower( false ), procs_tactician( false )
  {
    special = true;
    if ( p -> talents.overpower -> ok() )
      procs_overpower = true;
  }

  virtual void assess_damage( dmg_e type, action_state_t* s ) override
  {
    base_t::assess_damage( type, s );
  }

  virtual void execute() override
  {
    base_t::execute();

    if ( p() -> talents.wrecking_ball -> ok() && p() -> rppm.wrecking_ball -> trigger() )
    {
      p() -> buff.wrecking_ball -> trigger();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( procs_overpower && p() -> rppm.overpower -> trigger() )
      {
        p() -> buff.overpower -> trigger();
      }

      // TOCHECK: Does Sweeping Strikes grant MS/Execute 2 chances to proc Tactician?
      if ( procs_tactician && rng().roll( p() -> spec.tactician -> effectN( 2 ).percent() ) )
      {
        p() -> cooldown.colossus_smash -> reset( true );
        p() -> proc.tactician -> occur();
      }

      if ( p() -> buff.corrupted_blood_of_zakajz -> up() )
      {
        residual_action::trigger(
          p() -> active.corrupted_blood_of_zakajz, // ignite spell
          target, // target
          s -> result_amount * p() -> buff.corrupted_blood_of_zakajz -> check_value() );
      }
      if ( p() -> talents.opportunity_strikes -> ok() && s -> result_amount > 0 )
      {
        if ( rng().roll( ( 1 - ( s -> target -> health_percentage() / 100 ) ) * p() -> talents.opportunity_strikes -> proc_chance() ) )
        {
          p() -> active.opportunity_strikes -> target = s -> target;
          p() -> active.opportunity_strikes -> execute(); // Blizzard Employee "What can we do to make this talent really awkward?"
        }
      }
    }
  }

  virtual void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( p() -> buff.corrupted_blood_of_zakajz -> up() )
    {
      residual_action::trigger(
        p() -> active.corrupted_blood_of_zakajz, // ignite spell
        target, // target
        d -> state -> result_amount * p() -> buff.corrupted_blood_of_zakajz -> check_value() );
    }
  }

  virtual double calculate_weapon_damage( double attack_power ) const override
  {
    double dmg = base_t::calculate_weapon_damage( attack_power );

    // Catch the case where weapon == 0 so we don't crash/retest below.
    if ( dmg == 0.0 )
      return 0;

    if ( weapon -> slot == SLOT_OFF_HAND )
    {
      if ( p() -> main_hand_weapon.group() == WEAPON_1H &&
           p() -> off_hand_weapon.group() == WEAPON_1H )
        dmg *= 1.0 + p() -> spec.singleminded_fury -> effectN( 2 ).percent();
    }
    return dmg;
  }
};

// Melee Attack =============================================================

struct melee_t: public warrior_attack_t
{
  bool mh_lost_melee_contact, oh_lost_melee_contact;
  double base_rage_generation, arms_rage_multiplier, fury_rage_multiplier, arms_trinket_chance;
  melee_t( const std::string& name, warrior_t* p ):
    warrior_attack_t( name, p, spell_data_t::nil() ),
    mh_lost_melee_contact( true ), oh_lost_melee_contact( true ),
    base_rage_generation( 1.75 ),
    arms_rage_multiplier( 3.40 ),
    fury_rage_multiplier( 1.00 ),
    arms_trinket_chance( 0 )
  {
    school = SCHOOL_PHYSICAL;
    special = false;
    background = repeating = auto_attack = may_glance = true;
    trigger_gcd = timespan_t::zero();
    if ( p -> dual_wield() )
      base_hit -= 0.19;

    if ( p -> arms_trinket )
    {
      const spell_data_t* data = p -> arms_trinket -> driver();
      arms_trinket_chance = data -> effectN( 1 ).average( p -> arms_trinket -> item ) / 100.0;
    }
  }

  void reset() override
  {
    warrior_attack_t::reset();
    mh_lost_melee_contact = true;
    oh_lost_melee_contact = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = warrior_attack_t::execute_time();
    if ( weapon -> slot == SLOT_MAIN_HAND && mh_lost_melee_contact )
      return timespan_t::zero(); // If contact is lost, the attack is instant.
    else if ( weapon -> slot == SLOT_OFF_HAND && oh_lost_melee_contact ) // Also used for the first attack.
      return timespan_t::zero();
    else
      return t;
  }

  void schedule_execute( action_state_t* s ) override
  {
    warrior_attack_t::schedule_execute( s );
    if ( weapon -> slot == SLOT_MAIN_HAND )
      mh_lost_melee_contact = false;
    else if ( weapon -> slot == SLOT_OFF_HAND )
      oh_lost_melee_contact = false;
  }

  double composite_hit() const override
  {
    if ( p() -> artifact.focus_in_chaos.rank() && p() -> buff.enrage -> up() )
      return 1.0;
    return warrior_attack_t::composite_hit();
  }

  void execute() override
  {
    if ( p() -> current.distance_to_move > 5 )
    { // Cancel autoattacks, auto_attack_t will restart them when we're back in range.
      if ( weapon -> slot == SLOT_MAIN_HAND )
      {
        p() -> proc.delayed_auto_attack -> occur();
        mh_lost_melee_contact = true;
        player -> main_hand_attack -> cancel();
      }
      else
      {
        p() -> proc.delayed_auto_attack -> occur();
        oh_lost_melee_contact = true;
        player -> off_hand_attack -> cancel();
      }
    }
    else
    {
      warrior_attack_t::execute();
      if ( p() -> fury_trinket ) // Buff procs from any attack, even if it misses/parries/etc.
      {
        p() -> buff.fury_trinket -> trigger( 1 );
      }
      else if ( rng().roll( arms_trinket_chance ) ) // Same
      {
        p() -> cooldown.colossus_smash -> reset( true );
        p() -> proc.arms_trinket -> occur();
      }
      if ( result_is_hit( execute_state -> result ) )
      {
        trigger_rage_gain( execute_state );
      }
    }
  }

  void trigger_rage_gain( action_state_t* s )
  {
    weapon_t* w = weapon;
    double rage_gain = w -> swing_time.total_seconds() * base_rage_generation;

    if ( p() -> specialization() == WARRIOR_ARMS )
    {
      if ( s -> result == RESULT_CRIT )
        rage_gain *= rng().range( 7.4375, 7.875 ); // Wild random numbers appear! Accurate as of 2015/02/02
      else
        rage_gain *= arms_rage_multiplier;
    }
    else
    {
      rage_gain *= fury_rage_multiplier;
      if ( w -> slot == SLOT_OFF_HAND )
        rage_gain *= 0.5;
    }

    rage_gain *= 1.0 + p() -> talents.endless_rage -> effectN( 1 ).percent();
    rage_gain = util::round( rage_gain, 1 );

    if ( p() -> specialization() == WARRIOR_ARMS && s -> result == RESULT_CRIT )
    {
      rage_resource_gain( RESOURCE_RAGE,
                            rage_gain,
                            p() -> gain.melee_crit );
    }
    else
    {
      rage_resource_gain( RESOURCE_RAGE,
                            rage_gain,
                            w -> slot == SLOT_OFF_HAND ? p() -> gain.melee_off_hand : p() -> gain.melee_main_hand );
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t: public warrior_attack_t
{
  auto_attack_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "auto_attack", p )
  {
    parse_options( options_str );
    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    ignore_false_positive = true;
    range = 5;

    if ( p -> main_hand_weapon.type == WEAPON_2H && p -> has_shield_equipped() && p -> specialization() != WARRIOR_FURY )
    {
      sim -> errorf( "Player %s is using a 2 hander + shield while specced as Protection or Arms. Disabling autoattacks.", name() );
    }
    else
    {
      p -> main_hand_attack = new melee_t( "auto_attack_mh", p );
      p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
      p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE && p -> specialization() == WARRIOR_FURY )
    {
      p -> off_hand_attack = new melee_t( "auto_attack_oh", p );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      p -> off_hand_attack -> id = 1;
    }
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    if ( p() -> main_hand_attack -> execute_event == nullptr )
      p() -> main_hand_attack -> schedule_execute();
    if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event == nullptr )
    {
      p() -> off_hand_attack -> schedule_execute();
    }
  }

  bool ready() override
  {
    bool ready = warrior_attack_t::ready();
    if ( ready ) // Range check
    {
      if ( p() -> main_hand_attack -> execute_event == nullptr )
        return ready;
      if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event == nullptr )
        // Don't check for execute_event if we don't have an offhand.
        return ready;
    }
    return false;
  }
};

// Bladestorm ===============================================================

struct bladestorm_tick_t: public warrior_attack_t
{
  bladestorm_tick_t( warrior_t* p, const std::string& name ):
    warrior_attack_t( name, p, p -> talents.bladestorm -> effectN( 1 ).trigger() )
  {
    dual = true;
    aoe = -1;
    weapon_multiplier *= 1.0 + p -> spec.seasoned_soldier -> effectN( 2 ).percent();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> has_shield_equipped() )
      am *= 1.0 + p() -> spec.protection -> effectN( 1 ).percent();

    return am;
  }
};

struct bladestorm_t: public warrior_attack_t
{
  attack_t* bladestorm_mh;
  attack_t* bladestorm_oh;
  bladestorm_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "bladestorm", p, p -> talents.bladestorm ),
    bladestorm_mh( new bladestorm_tick_t( p, "bladestorm_mh" ) ),
    bladestorm_oh( nullptr )
  {
    parse_options( options_str );
    channeled = tick_zero = true;
    callbacks = interrupt_auto_attack = false;

    bladestorm_mh -> weapon = &( player -> main_hand_weapon );
    add_child( bladestorm_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE && player -> specialization() == WARRIOR_FURY )
    {
      bladestorm_oh = new bladestorm_tick_t( p, "bladestorm_oh" );
      bladestorm_oh -> weapon = &( player -> off_hand_weapon );
      add_child( bladestorm_oh );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p() -> buff.bladestorm -> trigger();
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    bladestorm_mh -> execute();

    if ( bladestorm_mh -> result_is_hit( execute_state -> result ) && bladestorm_oh )
      bladestorm_oh -> execute();
  }

  void last_tick( dot_t*d ) override
  {
    warrior_attack_t::last_tick( d );
    p() -> buff.bladestorm -> expire();
  }
};

// Bloodthirst Heal =========================================================

struct bloodthirst_heal_t: public warrior_heal_t
{
  bloodthirst_heal_t( warrior_t* p ):
    warrior_heal_t( "bloodthirst_heal", p, p -> find_spell( 117313 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background = true;
  }

  resource_e current_resource() const override { return RESOURCE_NONE; }
};

// Bloodthirst ==============================================================

struct bloodthirst_t: public warrior_attack_t
{
  bloodthirst_heal_t* bloodthirst_heal;
  double fresh_meat_crit_chance;
  double rage_gain;
  int aoe_targets;
  bloodthirst_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "bloodthirst", p, p -> spec.bloodthirst ),
    bloodthirst_heal( nullptr ),
    fresh_meat_crit_chance( p -> talents.endless_rage -> effectN( 1 ).percent() ),
    rage_gain( data().effectN( 3 ).resource( RESOURCE_RAGE ) ),
    aoe_targets( p -> buff.meat_cleaver -> data().effectN( 1 ).base_value() )
  {
    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );
    radius = 5;

    weapon_multiplier *= 1.0 + p -> artifact.thirst_for_battle.percent();
    if ( p -> non_dps_mechanics )
    {
      bloodthirst_heal = new bloodthirst_heal_t( p );
    }
  }

  int n_targets() const override
  {
    if ( p() -> buff.meat_cleaver -> up() )
      return aoe_targets + 1;
    return 1;
  }

  double composite_target_crit( player_t* target ) const override
  {
    double tc = warrior_attack_t::composite_target_crit( target );

    if ( fresh_meat_crit_chance > 0 && target -> health_percentage() >= 80.0 )
      tc += fresh_meat_crit_chance;

    return tc;
  }

  double composite_crit() const override
  {
    double c = warrior_attack_t::composite_crit();

    c += p() -> buff.taste_for_blood -> check_value();

    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( bloodthirst_heal )
      {
        bloodthirst_heal -> execute();
      }

      if ( execute_state -> result == RESULT_CRIT )
      {
        enrage();
        p() -> buff.taste_for_blood -> expire();
      }
    }

    rage_resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.bloodthirst );
  }
};

// Furious Slash ==============================================================

struct furious_slash_t: public warrior_attack_t
{
  furious_slash_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "furious_slash", p, p -> spec.furious_slash )
  {
    parse_options( options_str );

    weapon = &( p -> off_hand_weapon );
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p() -> buff.taste_for_blood -> trigger( 1 );
    p() -> buff.frenzy -> trigger( 1 );
  }
};

// Charge ===================================================================

struct charge_t: public warrior_attack_t
{
  bool first_charge;
  double movement_speed_increase, min_range, rage_gain;
  charge_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "charge", p, p -> spell.charge ),
    first_charge( true ),
    movement_speed_increase( 5.0 ),
    min_range( data().min_range() ),
    rage_gain( data().effectN( 2 ).resource( RESOURCE_RAGE ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    movement_directionality = MOVEMENT_OMNI;
    rage_gain += p -> artifact.uncontrolled_rage.value();
    cooldown -> duration = data().cooldown();
    if ( p -> talents.warbringer -> ok() )
    {
      aoe = -1;
      parse_effect_data( p -> find_spell( 7922 ) -> effectN( 1 ) );
    }
    if ( p -> talents.double_time -> ok() )
    {
      cooldown -> charges += p -> talents.double_time -> effectN( 1 ).base_value();
      cooldown -> duration += p -> talents.double_time -> effectN( 2 ).time_value();
    }
  }

  void execute() override
  {
    if ( p() -> current.distance_to_move > 5 )
    {
      p() -> buff.charge_movement -> trigger( 1, movement_speed_increase, 1, timespan_t::from_seconds(
        p() -> current.distance_to_move / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() + movement_speed_increase ) ) ) );
      p() -> current.moving_away = 0;
    }

    warrior_attack_t::execute();

    if ( first_charge )
      first_charge = !first_charge;

    p() -> cooldown.rage_from_charge -> start();
    rage_resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.charge );
  }

  void reset() override
  {
    action_t::reset();
    first_charge = true;
  }

  bool ready() override
  {
    if ( first_charge ) // Assumes that we charge into combat, instead of setting initial distance to 20 yards.
      return warrior_attack_t::ready();

    if ( p() -> current.distance_to_move < min_range ) // Cannot charge if too close to the target.
      return false;

    if ( p() -> buff.charge_movement -> check() || p() -> buff.heroic_leap_movement -> check()
         || p() -> buff.intervene_movement -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Cleave ===================================================================

struct cleave_t: public warrior_attack_t
{
  struct void_cleave_t: public warrior_attack_t
  {
    void_cleave_t( warrior_t* p ):
      warrior_attack_t( "void_cleave", p, p -> find_spell( 209700 ) )
    {
      background = true;
      aoe = -1;
    }
  };

  int targets_hit;
  void_cleave_t* void_cleave;

  cleave_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "cleave", p, p -> spec.cleave )
  {
    parse_options( options_str );
    weapon = &( player -> main_hand_weapon );
    aoe = -1;

    if ( p -> artifact.void_cleave.rank() )
    {
      void_cleave = new void_cleave_t( p );
      add_child( void_cleave );
    }
  }

  void execute() override
  {
    targets_hit = 0;

    warrior_attack_t::execute();

    if ( targets_hit )
    {
      p() -> buff.cleave -> trigger( 1, p() -> buff.cleave -> default_value * std::min( p() -> buff.cleave -> max_stack(), targets_hit ) );

      if ( targets_hit >= p() -> artifact.void_cleave.data().effectN( 1 ).base_value() )
      {
        void_cleave -> target = target;
        void_cleave -> execute();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      targets_hit++;

      if ( p() -> talents.fervor_of_battle -> ok() )
      {
        rage_resource_gain( RESOURCE_RAGE, p() -> spell.fervor_of_battle -> effectN( 1 ).resource( RESOURCE_RAGE ), p() -> gain.fervor_of_battle );
      }
    }
  }
};

// Colossus Smash ===========================================================

struct colossus_smash_t: public warrior_attack_t
{
  colossus_smash_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "colossus_smash", p, p -> spec.colossus_smash )
  {
    parse_options( options_str );
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier *= 1.0 + p -> artifact.focus_in_battle.percent();
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      td( execute_state -> target ) -> debuffs_colossus_smash -> trigger();

      if ( p() -> buff.colossus_smash -> remains() < p() -> buff.colossus_smash -> buff_duration )
        p() -> buff.colossus_smash -> trigger();

      p() -> buff.shattered_defenses -> trigger();

      p() -> buff.precise_strikes -> trigger();

      if ( p() -> sets.set( WARRIOR_ARMS, T17, B2 ) && p() -> buff.tier17_2pc_arms -> trigger() )
        p() -> proc.t17_2pc_arms -> occur();
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p() -> cache.mastery_value();

    return am;
  }
};

// Corrupted Blood of Zakajz ========================================================

struct corrupted_blood_of_zakajz_t : public residual_action::residual_periodic_action_t<warrior_attack_t>
{
  corrupted_blood_of_zakajz_t( warrior_t* p ) :
    residual_action::residual_periodic_action_t<warrior_attack_t>( "corrupted_blood_of_zakajz", p, p -> find_spell( 209569 ) )
  {
    background = dual = proc = true;
    may_miss = may_dodge = may_parry = false;
  }
};

// Corrupted Rage ===========================================================

struct corrupted_rage_t: public warrior_attack_t
{
  corrupted_rage_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "corrupted_rage", p, p -> find_spell( 209577 ) )
  {
    parse_options( options_str );
    weapon = &( player -> main_hand_weapon );
    aoe = -1;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs_colossus_smash -> trigger();

      if ( p() -> buff.colossus_smash -> remains() < p() -> buff.colossus_smash -> buff_duration )
        p() -> buff.colossus_smash -> trigger();
      
      p() -> buff.shattered_defenses -> trigger();
    }
  }

  bool ready() override
  {
    if ( ! p() -> artifact.corrupted_rage.rank() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Deep Wounds ==============================================================

struct deep_wounds_t: public warrior_attack_t
{
  deep_wounds_t( warrior_t* p ):
    warrior_attack_t( "deep_wounds", p, p -> spec.deep_wounds -> effectN( 2 ).trigger() )
  {
    background = tick_may_crit = true;
    hasted_ticks = false;
  }
};

// Opportunity Strikes ==============================================================

struct opportunity_strikes_t : public warrior_attack_t
{
  opportunity_strikes_t( warrior_t* p ):
    warrior_attack_t( "opportunity_strikes", p, p -> talents.opportunity_strikes -> effectN( 1 ).trigger() )
  {
    background = true;
  }
};

// Demoralizing Shout =======================================================

struct demoralizing_shout: public warrior_attack_t
{
  demoralizing_shout( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "demoralizing_shout", p, p -> spec.demoralizing_shout )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );
    td( s -> target ) -> debuffs_demoralizing_shout -> trigger( 1, data().effectN( 1 ).percent() );
  }
};

// Devastate ================================================================

struct devastate_t: public warrior_attack_t
{
  devastate_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "devastate", p, p -> spec.devastate )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );
    impact_action = p -> active.deep_wounds;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> buff.sword_and_board -> trigger() )
        p() -> cooldown.shield_slam -> reset( true );
    }
  }

  bool ready() override
  {
    if ( !p() -> has_shield_equipped() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Dragon Roar ==============================================================

struct dragon_roar_t: public warrior_attack_t
{
  dragon_roar_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "dragon_roar", p, p -> talents.dragon_roar )
  {
    parse_options( options_str );
    aoe = -1;
    may_dodge = may_parry = may_block = false;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p() -> buff.dragon_roar -> trigger();
  }

  double target_armor( player_t* ) const override { return 0; }

  double composite_crit() const override { return 1.0; }

  double composite_target_crit( player_t* ) const override { return 0.0; }
};

// Execute ==================================================================

struct execute_off_hand_t: public warrior_attack_t
{
  execute_off_hand_t( warrior_t* p, const char* name, const spell_data_t* s ):
    warrior_attack_t( name, p, s )
  {
    dual = true;
    may_miss = may_dodge = may_parry = may_block = false;
    weapon = &( p -> off_hand_weapon );

    if ( p -> main_hand_weapon.group() == WEAPON_1H &&
         p -> off_hand_weapon.group() == WEAPON_1H )
    {
      weapon_multiplier *= 1.0 + p -> spec.singleminded_fury -> effectN( 3 ).percent();
    }
  }
};

struct execute_t: public warrior_attack_t
{
  execute_off_hand_t* oh_attack;
  execute_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "execute", p, p -> spec.execute )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );
    procs_tactician = true;

    if ( p -> specialization() == WARRIOR_FURY )
    {
      oh_attack = new execute_off_hand_t( p, "execute_offhand", p -> find_spell( 163558 ) );
      add_child( oh_attack );
      if ( p -> main_hand_weapon.group() == WEAPON_1H &&
           p -> off_hand_weapon.group() == WEAPON_1H )
        weapon_multiplier *= 1.0 + p -> spec.singleminded_fury -> effectN( 3 ).percent();
    }

    base_crit += p -> artifact.deathblow.percent();
    base_crit += p -> artifact.deathdealer.percent();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> mastery.colossal_might -> ok() )
    {
      am *= 4.0 * ( std::min( 40.0, ( p() -> resources.current[RESOURCE_RAGE] ) ) / 40 );
    }
    else if ( p() -> has_shield_equipped() )
    { am *= 1.0 + p() -> spec.protection -> effectN( 2 ).percent(); }

    am *= 1.0 + p() -> buff.shattered_defenses -> check_value();

    return am;
  }

  double composite_crit() const override
  {
    double cc = warrior_attack_t::composite_crit();

    if ( p() -> buff.shattered_defenses -> check() )
      cc += p() -> buff.shattered_defenses -> data().effectN( 2 ).percent();

    return cc;
  }

  double cost() const override
  {
    double c = warrior_attack_t::cost();

    if ( p() -> mastery.colossal_might -> ok() )
      c = std::min( 40.0, std::max( p() -> resources.current[RESOURCE_RAGE], c ) );

    c *= 1.0 + p() -> buff.precise_strikes -> check_value();

    if ( p() -> buff.sense_death -> check() )
      c *= 1.0 + p() -> buff.sense_death -> data().effectN( 1 ).percent();

    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p() -> rppm.rage_of_the_valarjar && p() -> rppm.rage_of_the_valarjar -> trigger() )
    {
      p() -> buff.berserking_driver -> trigger();
    }

    p() -> buff.sense_death -> expire();
    p() -> buff.sense_death -> trigger();

    if ( p() -> specialization() == WARRIOR_FURY && result_is_hit( execute_state -> result ) &&
         p() -> off_hand_weapon.type != WEAPON_NONE ) // If MH fails to land, or if there is no OH weapon for Fury, oh attack does not execute.
      oh_attack -> execute();

    if ( p() -> buff.shattered_defenses -> up() )
      p() -> buff.shattered_defenses -> expire();

    if ( p() -> buff.precise_strikes -> up() )
      p() -> buff.precise_strikes -> expire();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT )
    {
      if ( td( s -> target ) -> debuffs_colossus_smash -> check() )
      {
        td( s -> target ) -> debuffs_colossus_smash -> extend_duration( p(),
                                                                        timespan_t::from_seconds( p() -> artifact.exploit_the_weakness.value() ) );

        timespan_t remains = std::max( td( s -> target ) -> debuffs_colossus_smash -> remains(), p() -> buff.colossus_smash -> remains() );
        p() -> buff.colossus_smash -> extend_duration( p(), remains - p() -> buff.colossus_smash -> remains() );
      }
      else
      {
        p() -> buff.massacre -> trigger();
      }
    }
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    // Call warrior_attack_t::ready() first for proper targeting support.
    if ( warrior_attack_t::ready() && target -> health_percentage() < 20 )
      return true;
    else
      return false;
  }
};

// Hamstring ==============================================================

struct hamstring_t: public warrior_attack_t
{
  hamstring_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "hamstring", p, p -> spec.hamstring )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );
  }
};

// Focused Rage ============================================================

struct focused_rage_t: public warrior_attack_t
{
  focused_rage_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "focused_rage", p, p -> specialization() == WARRIOR_ARMS ? p -> talents.focused_rage : p -> spec.focused_rage )
  {
    parse_options( options_str );
    use_off_gcd = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p() -> buff.focused_rage -> trigger();
  }
};

// Heroic Throw ==============================================================

struct heroic_throw_t: public warrior_attack_t
{
  heroic_throw_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "heroic_throw", p, p -> find_class_spell( "Heroic Throw" ) )
  {
    parse_options( options_str );

    weapon = &( player -> main_hand_weapon );
    may_dodge = may_parry = may_block = false;
  }

  bool ready() override
  {
    if ( p() -> current.distance_to_move > range ||
         p() -> current.distance_to_move < data().min_range() ) // Cannot heroic throw unless target is in range.
      return false;

    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Heroic Leap ==============================================================

struct heroic_leap_t: public warrior_attack_t
{
  const spell_data_t* heroic_leap_damage;
  heroic_leap_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "heroic_leap", p, p -> spell.heroic_leap ),
    heroic_leap_damage( p -> find_spell( 52174 ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    aoe = -1;
    may_dodge = may_parry = may_miss = may_block = false;
    movement_directionality = MOVEMENT_OMNI;
    base_teleport_distance = data().max_range();
    range = -1;
    attack_power_mod.direct = heroic_leap_damage -> effectN( 1 ).ap_coeff();

    cooldown -> duration = data().charge_cooldown(); // Fixes bug in spelldata for now. 
    cooldown -> duration += p -> talents.bounding_stride -> effectN( 1 ).time_value();
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 0.25 );
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p() -> current.distance_to_move > 0 && !p() -> buff.heroic_leap_movement -> check() )
    {
      double speed;
      speed = std::min( p() -> current.distance_to_move, base_teleport_distance ) / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() ) ) / travel_time().total_seconds();
      p() -> buff.heroic_leap_movement -> trigger( 1, speed, 1, travel_time() );
    }
  }

  void impact( action_state_t* s ) override
  {
    if ( p() -> current.distance_to_move > heroic_leap_damage -> effectN( 1 ).radius() )
      s -> result_amount = 0;

    warrior_attack_t::impact( s );
  }

  bool ready() override
  {
    if ( p() -> buff.intervene_movement -> check() || p() -> buff.charge_movement -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Impending Victory ========================================================

struct impending_victory_heal_t: public warrior_heal_t
{
  impending_victory_heal_t( warrior_t* p ):
    warrior_heal_t( "impending_victory_heal", p, p -> find_spell( 118340 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background = true;
  }

  resource_e current_resource() const override
  {
    return RESOURCE_NONE;
  }
};

struct impending_victory_t: public warrior_attack_t
{
  impending_victory_heal_t* impending_victory_heal;
  impending_victory_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "impending_victory", p, 0 ),
    impending_victory_heal( nullptr )
  {
    parse_options( options_str );
    if ( p -> non_dps_mechanics )
    {
      impending_victory_heal = new impending_victory_heal_t( p );
    }
    parse_effect_data( data().effectN( 2 ) ); //Both spell effects list an ap coefficient, #2 is correct.
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( impending_victory_heal )
    {
      impending_victory_heal -> execute();
    }
  }
};

// Intervene ===============================================================
// Note: Conveniently ignores that you can only intervene a friendly target.
// For the time being, we're just going to assume that there is a friendly near the target
// that we can intervene to. Maybe in the future with a more complete movement system, we will
// fix this to work in a raid simulation that includes multiple melee.

struct intervene_t: public warrior_attack_t
{
  double movement_speed_increase;
  intervene_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "intervene", p, p -> spell.intervene ),
    movement_speed_increase( 5.0 )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    movement_directionality = MOVEMENT_OMNI;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p() -> current.distance_to_move > 0 )
    {
      p() -> buff.intervene_movement -> trigger( 1, movement_speed_increase, 1,
                                                 timespan_t::from_seconds( p() -> current.distance_to_move / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() + movement_speed_increase ) ) ) );
      p() -> current.moving_away = 0;
    }
  }

  bool ready() override
  {
    if ( p() -> buff.heroic_leap_movement -> check() || p() -> buff.charge_movement -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Heroic Charge ============================================================

struct heroic_charge_movement_ticker_t: public event_t
{
  timespan_t duration;
  warrior_t* warrior;
  heroic_charge_movement_ticker_t( sim_t& s, warrior_t*p, timespan_t d = timespan_t::zero() ):
    event_t( s, p ), warrior( p )
  {
    if ( d > timespan_t::zero() )
      duration = d;
    else
      duration = next_execute();

    add_event( duration );
    if ( sim().debug ) sim().out_debug.printf( "New movement event" );
  }

  timespan_t next_execute() const
  {
    timespan_t min_time = timespan_t::max();
    bool any_movement = false;
    timespan_t time_to_finish = warrior -> time_to_move();
    if ( time_to_finish != timespan_t::zero() )
    {
      any_movement = true;

      if ( time_to_finish < min_time )
        min_time = time_to_finish;
    }

    if ( min_time > timespan_t::from_seconds( 0.05 ) ) // Update a little more than usual, since we're moving a lot faster.
      min_time = timespan_t::from_seconds( 0.05 );

    if ( !any_movement )
      return timespan_t::zero();
    else
      return min_time;
  }

  void execute() override
  {
    if ( warrior -> time_to_move() > timespan_t::zero() )
      warrior -> update_movement( duration );

    timespan_t next = next_execute();
    if ( next > timespan_t::zero() )
      warrior -> heroic_charge = new ( sim() ) heroic_charge_movement_ticker_t( sim(), warrior, next );
    else
    {
      warrior -> heroic_charge = nullptr;
      warrior -> buff.heroic_leap_movement -> expire();
    }
  }
};

struct heroic_charge_t: public warrior_attack_t
{
  action_t*leap;
  heroic_charge_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "heroic_charge", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    leap = new heroic_leap_t( p, options_str );
    trigger_gcd = timespan_t::zero();
    ignore_false_positive = true;
    callbacks = may_crit = false;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p() -> cooldown.heroic_leap -> up() )
    {// We are moving 10 yards, and heroic leap always executes in 0.25 seconds.
      // Do some hacky math to ensure it will only take 0.25 seconds, since it will certainly
      // be the highest temporary movement speed buff.
      double speed;
      speed = 10 / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() ) ) / 0.25;
      p() -> buff.heroic_leap_movement -> trigger( 1, speed, 1, timespan_t::from_millis( 250 ) );
      leap -> execute();
      p() -> trigger_movement( 10.0, MOVEMENT_BOOMERANG ); // Leap 10 yards out, because it's impossible to precisely land 8 yards away.
      p() -> heroic_charge = new ( *sim ) heroic_charge_movement_ticker_t( *sim, p() );
    }
    else
    {
      p() -> trigger_movement( 9.0, MOVEMENT_BOOMERANG );
      p() -> heroic_charge = new ( *sim ) heroic_charge_movement_ticker_t( *sim, p() );
    }
  }

  bool ready() override
  {
    if ( p() -> cooldown.rage_from_charge -> up() && p() -> cooldown.charge -> up() && !p() -> buffs.raid_movement -> check() )
      return warrior_attack_t::ready();
    else
      return false;
  }
};

// Mortal Strike ============================================================

struct mortal_strike_t: public warrior_attack_t
{
  struct shadow_slash_t: public warrior_attack_t
  {
    shadow_slash_t( warrior_t* p ):
      warrior_attack_t( "shadow_slash", p, p -> find_spell( 209733 ) )
    {
      background = true;
      aoe = -1;
    }
  };

  shadow_slash_t* shadow_slash;

  mortal_strike_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "mortal_strike", p, p -> spec.mortal_strike )
  {
    parse_options( options_str );
    cooldown = p -> cooldown.mortal_strike;
    cooldown -> duration = data().charge_cooldown();
    weapon = &( p -> main_hand_weapon );
    base_costs[RESOURCE_RAGE] += p -> sets.set( WARRIOR_ARMS, T17, B4 ) -> effectN( 1 ).resource( RESOURCE_RAGE );
    cooldown -> charges += p -> talents.mortal_combo -> effectN( 1 ).base_value();
    base_multiplier *= 1.0 + p -> artifact.thoradins_might.percent();

    if ( p -> stromkar_the_warbreaker )
    {
      shadow_slash = new shadow_slash_t( p );
      add_child( shadow_slash );
    }
  }

  double composite_crit() const override
  {
    double cc = warrior_attack_t::composite_crit();

    if ( p() -> buff.shattered_defenses -> check() )
      cc += p() -> buff.shattered_defenses -> data().effectN( 2 ).percent();

    return cc;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p() -> buff.shattered_defenses -> check_value();
    am *= 1.0 + p() -> buff.focused_rage -> check_value();

    return am;
  }

  double cost() const override
  {
    double c = warrior_attack_t::cost();

    c *= 1.0 + p() -> buff.precise_strikes -> check_value();

    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( sim -> overrides.mortal_wounds )
      {
        execute_state -> target -> debuffs.mortal_wounds -> trigger();
      }

      if ( p() -> talents.in_for_the_kill -> ok() && execute_state -> target -> health_percentage() <= 20 )
      {
        rage_resource_gain( RESOURCE_RAGE, p() -> talents.in_for_the_kill -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE ),
                            p() -> gain.in_for_the_kill );
      }

      p() -> buff.focused_rage -> expire();
    }

    // Does not require hit.
    if ( p() -> stromkar_the_warbreaker && p() -> rppm.shadow_slash -> trigger() )
    {
      shadow_slash -> target = target;
      shadow_slash -> execute();
    }

    p() -> buff.shattered_defenses -> expire();
    p() -> buff.precise_strikes -> expire();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT && td( s -> target ) -> debuffs_colossus_smash -> check() )
    {
      td( s -> target ) -> debuffs_colossus_smash -> extend_duration( p(),
        timespan_t::from_seconds( p() -> artifact.exploit_the_weakness.value() ) );
      
      timespan_t remains = std::max( td( s -> target ) -> debuffs_colossus_smash -> remains(), p() -> buff.colossus_smash -> remains() );
      p() -> buff.colossus_smash -> extend_duration( p(), remains - p() -> buff.colossus_smash -> remains() );
    }
  }

  double cooldown_reduction() const override
  {
    double cdr = warrior_attack_t::cooldown_reduction();

    if ( p() -> buff.tier17_2pc_arms -> up() )
      cdr *= 1.0 + p() -> buff.tier17_2pc_arms -> data().effectN( 1 ).percent();

    return cdr;
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Pummel ===================================================================

struct pummel_t: public warrior_attack_t
{
  pummel_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "pummel", p, p -> find_class_spell( "Pummel" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    may_miss = may_block = may_dodge = may_parry = false;
  }
};

// Raging Blow ==============================================================

struct raging_blow_attack_t: public warrior_attack_t
{
  raging_blow_attack_t( warrior_t* p, const char* name, const spell_data_t* s ):
    warrior_attack_t( name, p, s )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    dual = true;
    weapon_multiplier *= 1.0 + p -> talents.inner_rage -> effectN( 3 ).percent();
    weapon_multiplier *= 1.0 + p -> artifact.wrath_and_fury.percent();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );
    if ( s -> result == RESULT_CRIT )
    { // Can proc off MH/OH individually from each meat cleaver hit.
      if ( rng().roll( p() -> sets.set( WARRIOR_FURY, T17, B2 ) -> proc_chance() ) )
      {
        enrage();
        p() -> proc.t17_2pc_fury -> occur();
      }
    }
  }
};

struct raging_blow_t: public warrior_attack_t
{
  raging_blow_attack_t* oh_attack;
  double rage_gain;
  raging_blow_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "raging_blow", p, p -> spec.raging_blow ),
    oh_attack( nullptr ),
    rage_gain( data().effectN( 2 ).resource( RESOURCE_RAGE ) )
  {
    parse_options( options_str );

    oh_attack = new raging_blow_attack_t( p, "raging_blow_offhand", data().effectN( 1 ).trigger() );
    oh_attack -> weapon = &( p -> off_hand_weapon );
    add_child( oh_attack );
    cooldown -> duration += p -> talents.inner_rage -> effectN( 1 ).time_value();
    weapon_multiplier *= 1.0 + p -> talents.inner_rage -> effectN( 3 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );
    if ( s -> result == RESULT_CRIT )
    { // Can proc off MH/OH individually from each meat cleaver hit.
      if ( rng().roll( p() -> sets.set( WARRIOR_FURY, T17, B2 ) -> proc_chance() ) )
      {
        enrage();
        p() -> proc.t17_2pc_fury -> occur();
      }
    }
  }

  void execute() override
  {
    // check attack
    warrior_attack_t::execute();
    if ( result_is_hit( execute_state -> result ) )
    {
      oh_attack -> execute();
      rage_resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.raging_blow );
    }
  }

  bool ready() override
  {
        // Needs weapons in both hands
    if ( p() -> main_hand_weapon.type == WEAPON_NONE ||
         p() -> off_hand_weapon.type == WEAPON_NONE )
      return false;

    if ( p() -> talents.inner_rage -> ok() )
      return warrior_attack_t::ready();

    if ( !p() -> buff.enrage -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Overpower ============================================================

struct overpower_t: public warrior_attack_t
{
  double crit_chance;
  overpower_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "overpower", p, p -> talents.overpower ),
    crit_chance( data().effectN( 3 ).percent() )
  {
    parse_options( options_str );
    may_block = may_parry = may_dodge = false;
    weapon = &( p -> main_hand_weapon );
  }

  double composite_crit() const override
  {
    double c = warrior_attack_t::composite_crit();

    c += crit_chance;

    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p() -> buff.overpower -> expire();
  }
  
  bool ready() override
  {
    if ( p() -> buff.overpower -> check() )
      return warrior_attack_t::ready();
    return false;
  }
};

// Rampage ================================================================

struct rampage_attack_t: public warrior_attack_t
{
  rampage_attack_t( warrior_t* p, const spell_data_t* rampage, const std::string& name ):
    warrior_attack_t( name, p, rampage )
  {
    dual = true;
    base_aoe_multiplier = 0.5; // No spelldata for this, only matters when Meat Grinder is talented. 
  }

  int n_targets() const override
  {
    if ( p() -> buff.meat_grinder -> up() )
      return 5;
    return 1;
  }

  double action_multiplier() const override
  {
    double dam = warrior_attack_t::action_multiplier();
    return dam;
  }
};

struct rampage_event_t: public event_t
{
  timespan_t duration;
  warrior_t* warrior;
  size_t attacks;
  rampage_event_t( warrior_t*p, size_t current_attack ):
    event_t( *p -> sim ), warrior( p ), attacks( current_attack )
  {
    duration = next_execute();
    add_event( duration );
    if ( sim().debug ) sim().out_debug.printf( "New rampage event" );
  }

  timespan_t next_execute() const
  {
    return timespan_t::from_millis( warrior -> spec.rampage -> effectN( attacks + 4 ).misc_value1() - ( attacks > 0 ? warrior -> spec.rampage -> effectN( attacks + 3 ).misc_value1() : 0 ) );
  }

  void execute() override
  {
    warrior -> rampage_attacks[attacks] -> execute();
    attacks++;
    if ( attacks < warrior -> rampage_attacks.size() )
    {
      warrior -> rampage_driver = new ( sim() ) rampage_event_t( warrior, attacks );
    }
    else
    {
      warrior -> buff.meat_grinder -> expire();
    }
  }
};

struct rampage_parent_t: public warrior_attack_t
{
  rampage_parent_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "rampage", p, p -> spec.rampage )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier *= 1.0 + p -> artifact.unstoppable.percent();
    for ( size_t i = 0; i < p -> rampage_attacks.size(); i++ )
    {
      add_child( p -> rampage_attacks[i] );
    }
    base_costs[RESOURCE_RAGE] += p -> talents.carnage -> effectN( 1 ).resource(RESOURCE_RAGE);
  }

  double cost() const override
  {
    if ( p() -> buff.massacre -> check() )
      return 0;
    return warrior_attack_t::cost();
  }

  void execute() override
  {
    enrage();

    warrior_attack_t::execute();

    p() -> buff.massacre -> expire();
    if ( p() -> rppm.rage_of_the_valarjar && p() -> rppm.rage_of_the_valarjar -> trigger() )
    {
      p() -> buff.berserking_driver -> trigger();
    }

    if ( result_is_hit( execute_state -> result ) ) // If the first attack fails to land, the rest do too. 
    {
      p() -> rampage_driver = new ( *sim ) rampage_event_t( p(), 0 );
    }
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Ravager ==============================================================

struct ravager_tick_t: public warrior_attack_t
{
  ravager_tick_t( warrior_t* p, const std::string& name ):
    warrior_attack_t( name, p, p -> find_spell( 156287 ) )
  {
    aoe = -1;
    dual = true;
    ground_aoe = true;
  }
};

struct ravager_t: public warrior_attack_t
{
  ravager_tick_t* ravager;
  ravager_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "ravager", p, p -> talents.ravager ),
    ravager( new ravager_tick_t( p, "ravager_tick" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    hasted_ticks = callbacks = false;
    dot_duration = timespan_t::from_seconds( data().effectN( 4 ).base_value() );
    attack_power_mod.direct = attack_power_mod.tick = 0;
    add_child( ravager );
  }

  void execute() override
  {
    if ( p() -> specialization() == WARRIOR_PROTECTION )
      p() -> buff.ravager_protection -> trigger();
    else
      p() -> buff.ravager -> trigger();

    warrior_attack_t::execute();
  }

  void tick( dot_t*d ) override
  {
    ravager -> execute();
    warrior_attack_t::tick( d );
  }
};

// Revenge ==================================================================

struct revenge_t: public warrior_attack_t
{
  stats_t* absorb_stats;
  double rage_gain;
  revenge_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "revenge", p, p -> spec.revenge ),
    absorb_stats( nullptr ), rage_gain( 0 )
  {
    parse_options( options_str );
    aoe = -1;
    rage_gain = data().effectN( 2 ).resource( RESOURCE_RAGE );
    impact_action = p -> active.deep_wounds;
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Enraged Regeneration ===============================================

struct enraged_regeneration_t: public warrior_heal_t
{
  enraged_regeneration_t( warrior_t* p, const std::string& options_str ):
    warrior_heal_t( "enraged_regeneration", p, p -> spec.enraged_regeneration )
  {
    parse_options( options_str );
    range = -1;
    base_pct_heal = data().effectN( 1 ).percent();
  }
};

// Rend ==============================================================

struct rend_t: public warrior_attack_t
{
  double t18_2pc_chance;
  rend_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "rend", p, p -> talents.rend ),
    t18_2pc_chance( 0 )
  {
    parse_options( options_str );
    tick_may_crit = true;
    base_tick_time /= 1.0 + p -> sets.set( WARRIOR_ARMS, T18, B4 ) -> effectN( 1 ).percent();
    if ( p -> sets.has_set_bonus( WARRIOR_ARMS, T18, B2 ) )
      t18_2pc_chance = p -> sets.set( WARRIOR_ARMS, T18, B2 ) -> proc_chance();
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );

    if ( rng().roll( t18_2pc_chance ) )
      p() -> cooldown.mortal_strike -> reset( true );
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Shield Slam ==============================================================

struct shield_block_2pc_t: public warrior_attack_t
{
  shield_block_2pc_t( warrior_t* p ):
    warrior_attack_t( "shield_block_t17_2pc_proc", p, p -> find_class_spell( "Shield Block" ) )
  {
    background = true;
    base_costs[RESOURCE_RAGE] = 0;
    cooldown -> duration = timespan_t::zero();
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p() -> buff.shield_block -> check() )
      p() -> buff.shield_block -> extend_duration( p(), p() -> buff.shield_block -> data().duration() );
    else
      p() -> buff.shield_block -> trigger();
  }
};

struct shield_slam_t: public warrior_attack_t
{
  double rage_gain;
  attack_t* shield_block_2pc;
  shield_slam_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "shield_slam", p, p -> spec.shield_slam ),
    rage_gain( 0.0 ),
    shield_block_2pc( new shield_block_2pc_t( p ) )
  {
    parse_options( options_str );
    rage_gain = data().effectN( 3 ).resource( RESOURCE_RAGE );

    attack_power_mod.direct = 0.561; // Low level value for shield slam.
    if ( p -> level() >= 80 )
      attack_power_mod.direct += 0.426; // Adds 42.6% ap once the character is level 80
    if ( p -> level() >= 85 )
      attack_power_mod.direct += 3.46; // Adds another 246% ap at level 85
    //Shield slam is just the best.
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.shield_block -> up() )
      am *= 1.0 + p() -> talents.heavy_repercussions -> effectN( 1 ).percent();

    am *= 1.0 + p() -> buff.focused_rage -> check_value();

    return am;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( rng().roll( p() -> sets.set( WARRIOR_PROTECTION, T17, B2 ) -> proc_chance() ) )
    {
      shield_block_2pc -> execute();
    }

    double rage_from_snb = 0;

    if ( result_is_hit( execute_state -> result ) )
    {
      rage_resource_gain( RESOURCE_RAGE,
                            rage_gain,
                            p() -> gain.shield_slam );

      if ( p() -> buff.sword_and_board -> up() )
      {
        rage_from_snb = p() -> buff.sword_and_board -> data().effectN( 1 ).resource( RESOURCE_RAGE );
        rage_resource_gain( RESOURCE_RAGE,
                              rage_from_snb,
                              p() -> gain.sword_and_board );
      }
      p() -> buff.sword_and_board -> expire();
    }
  }

  bool ready() override
  {
    if ( !p() -> has_shield_equipped() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Slam =====================================================================

struct slam_t: public warrior_attack_t
{
  slam_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "slam", p, p -> spec.slam )
  {
    parse_options( options_str );
    weapon = &( p -> main_hand_weapon );
    procs_tactician = true;
    base_multiplier *= 1.0 + p -> artifact.crushing_blows.percent();
  }

  void assess_damage( dmg_e type, action_state_t* s ) override
  {
    warrior_attack_t::assess_damage( type, s );
    if ( p() -> talents.trauma -> ok() )
    {
      residual_action::trigger(
        p() -> active.trauma, // ignite spell
        s -> target, // target
        p() -> talents.trauma -> effectN( 1 ).percent() * s -> result_amount );
    }
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Trauma Dot ============================================================

struct trauma_dot_t: public residual_action::residual_periodic_action_t < warrior_attack_t >
{
  trauma_dot_t( warrior_t* p ):
    base_t( "trauma", p, p -> find_spell( 215537 ) )
  {
    dual = true;
  }
};

// Shockwave ================================================================

struct shockwave_t: public warrior_attack_t
{
  shockwave_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "shockwave", p, p -> talents.shockwave )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
    aoe = -1;
  }

  void update_ready( timespan_t cd_duration ) override
  {
    cd_duration = cooldown -> duration;

    if ( execute_state -> n_targets >= 3 )
    {
      if ( cd_duration > timespan_t::from_seconds( 20 ) )
        cd_duration += timespan_t::from_seconds( -20 );
      else
        cd_duration = timespan_t::zero();
    }
    warrior_attack_t::update_ready( cd_duration );
  }
};

// Storm Bolt ===============================================================

struct storm_bolt_t: public warrior_attack_t
{
  storm_bolt_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "storm_bolt", p, p -> talents.storm_bolt )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// Thunder Clap =============================================================

struct thunder_clap_t: public warrior_attack_t
{
  thunder_clap_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "thunder_clap", p, p -> spec.thunder_clap )
  {
    parse_options( options_str );
    aoe = -1;
    may_dodge = may_parry = may_block = false;
  }

  double cost() const override
  {
    double c = base_t::cost();

    if ( p() -> specialization() == WARRIOR_PROTECTION )
      c = 0;

    return c;
  }
};

// Victory Rush =============================================================

struct victory_rush_heal_t: public warrior_heal_t
{
  victory_rush_heal_t( warrior_t* p ):
    warrior_heal_t( "victory_rush_heal", p, p -> find_spell( 118779 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background = true;
  }
  resource_e current_resource() const override { return RESOURCE_NONE; }
};

struct victory_rush_t: public warrior_attack_t
{
  victory_rush_heal_t* victory_rush_heal;

  victory_rush_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "victory_rush", p, p -> find_class_spell( "Victory Rush" ) ),
    victory_rush_heal( new victory_rush_heal_t( p ) )
  {
    parse_options( options_str );
    if ( p -> non_dps_mechanics )
      execute_action = victory_rush_heal;
    cooldown -> duration = timespan_t::from_seconds( 1000.0 );
  }
};

// Whirlwind ================================================================

struct whirlwind_off_hand_t: public warrior_attack_t
{
  whirlwind_off_hand_t( warrior_t* p, const spell_data_t* whirlwind ):
    warrior_attack_t( "whirlwind_oh", p, whirlwind )
  {
    aoe = -1;
  }
};

struct whirlwind_mh_t: public warrior_attack_t
{
  whirlwind_mh_t( warrior_t* p, const spell_data_t* whirlwind ):
    warrior_attack_t( "whirlwind_mh", p, whirlwind )
  {
    aoe = -1;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( p() -> talents.fervor_of_battle -> ok() )
      rage_resource_gain( RESOURCE_RAGE, p() -> spell.fervor_of_battle -> effectN( 1 ).resource( RESOURCE_RAGE ), p() -> gain.fervor_of_battle );

    if ( p() -> artifact.will_of_the_first_king.rank() )
      rage_resource_gain( RESOURCE_RAGE, p() -> artifact.will_of_the_first_king.data().effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE ), p() -> gain.will_of_the_first_king );

    if ( s -> result_amount > 0 )// Only triggers if damage is done.
      p() -> buff.meat_cleaver -> trigger();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p() -> buff.cleave -> check_value();

    return am;
  }
};

struct whirlwind_parent_t: public warrior_attack_t
{
  whirlwind_off_hand_t* oh_attack;
  whirlwind_mh_t* mh_attack;
  timespan_t spin_time;
  whirlwind_parent_t( warrior_t* p, const std::string& options_str ):
    warrior_attack_t( "whirlwind", p, p -> spec.whirlwind ),
    oh_attack( nullptr ),
    mh_attack( nullptr ),
    spin_time( timespan_t::from_millis( p -> specialization() == WARRIOR_ARMS ? p -> spec.whirlwind -> effectN( 2 ).misc_value1() :
                                                                              p -> spec.whirlwind -> effectN( 3 ).misc_value1() ) )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger() -> effectN( 1 ).radius_max();

    if ( p -> main_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> specialization() == WARRIOR_FURY )
      {
        mh_attack = new whirlwind_mh_t( p, data().effectN( 1 ).trigger() );
        mh_attack -> weapon = &( p -> main_hand_weapon );
        add_child( mh_attack );
        if ( p -> off_hand_weapon.type != WEAPON_NONE )
        {
          oh_attack = new whirlwind_off_hand_t( p, data().effectN( 2 ).trigger() );
          oh_attack -> weapon = &( p -> off_hand_weapon );
          add_child( oh_attack );
        }
      }
      else
      {
        mh_attack = new whirlwind_mh_t( p, data().effectN( 1 ).trigger() );
        mh_attack -> weapon = &( p -> main_hand_weapon );
        add_child( mh_attack );
      }
    }
    tick_zero = true;
    callbacks = false;
    base_tick_time = spin_time;
    hasted_ticks = false;
    dot_duration = base_tick_time * 2;
  }

  double cost() const override
  {
    if ( p() -> buff.wrecking_ball -> check() )
      return 0;

    return warrior_attack_t::cost();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.wrecking_ball -> check() )
      am *= 1.0 + p() -> buff.wrecking_ball -> data().effectN( 2 ).percent();

    return am;
  }

  void tick( dot_t* d ) override
  {
    // Only the first whirl procs tactician.
    mh_attack -> procs_tactician = d -> current_tick == 0;

    warrior_attack_t::tick( d );

    if ( mh_attack )
    {
      mh_attack -> execute();
      if ( oh_attack )
        oh_attack -> execute();
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );

    p() -> buff.cleave -> expire();
    p() -> buff.wrecking_ball -> expire();
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p() -> buff.meat_grinder -> trigger();
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_attack_t::ready();
  }
};

// ==========================================================================
// Warrior Spells
// ==========================================================================

// Avatar ===================================================================

struct avatar_t: public warrior_spell_t
{
  avatar_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "avatar", p, p -> talents.avatar )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p() -> buff.avatar -> trigger();
  }
};

// Berserker Rage ===========================================================

struct berserker_rage_t: public warrior_spell_t
{
  berserker_rage_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "berserker_rage", p, p -> find_class_spell( "Berserker Rage" ) )
  {
    parse_options( options_str );
    callbacks = false;
    range = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p() -> buff.berserker_rage -> trigger();
    if ( p() -> talents.outburst -> ok() )
      enrage();
  }
};

// Defensive Stance ===============================================================

struct defensive_stance_t: public warrior_spell_t
{
  std::string onoff;
  bool onoffbool;
  defensive_stance_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "defensive_stance", p, p -> spell.defensive_stance ),
    onoff( nullptr ), onoffbool( 0 )
  {
    add_option( opt_string( "toggle", onoff ) );
    parse_options( options_str );

    if ( onoff == "on" )
      onoffbool = true;
    else if ( onoff == "off" )
      onoffbool = false;
    else
    {
      sim -> errorf( "Defensive stance must use the option 'toggle=on' or 'toggle=off'" );
      background = true;
    }

    use_off_gcd = true;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    if ( onoffbool )
      p() -> buff.defensive_stance -> trigger();
    else
      p() -> buff.defensive_stance -> expire();
  }

  bool ready() override
  {
    if ( onoffbool && p() -> buff.defensive_stance -> check() )
      return false;
    else if ( !onoffbool && !p() -> buff.defensive_stance -> check() )
      return false;

    return warrior_spell_t::ready();
  }
};

// Die By the Sword  ==============================================================

struct die_by_the_sword_t: public warrior_spell_t
{
  die_by_the_sword_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "die_by_the_sword", p, p -> spec.die_by_the_sword ? p -> spec.die_by_the_sword : p -> talents.die_by_the_sword )
  {
    parse_options( options_str );
    range = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p() -> buff.die_by_the_sword -> trigger();
  }

  bool ready() override
  {
    if ( p() -> main_hand_weapon.type == WEAPON_NONE )
      return false;

    return warrior_spell_t::ready();
  }
};

// Last Stand ===============================================================

struct last_stand_t: public warrior_spell_t
{
  last_stand_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "last_stand", p, p -> spec.last_stand )
  {
    parse_options( options_str );
    range = -1;
    cooldown -> duration = data().cooldown();
    cooldown -> duration *= 1.0 + p -> sets.set( WARRIOR_PROTECTION, T18, B2 ) -> effectN( 1 ).percent();
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p() -> buff.last_stand -> trigger();
  }
};

// Commanding Shout ===============================================================

struct commanding_shout_t: public warrior_spell_t
{
  commanding_shout_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "commanding_shout", p, p -> spec.commanding_shout )
  {
    parse_options( options_str );
    range = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p() -> buff.commanding_shout -> trigger();
  }
};

// Battle Cry =============================================================

struct battle_cry_t: public warrior_spell_t
{
  double bonus_crit;
  battle_cry_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "battle_cry", p, p -> spec.battle_cry ),
    bonus_crit( 0.0 )
  {
    parse_options( options_str );
    bonus_crit = data().effectN( 1 ).percent();
    callbacks = false;
    cooldown -> duration += p -> artifact.helyas_wrath.time_value();
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p() -> buff.battle_cry -> trigger( 1, bonus_crit );
    if ( p() -> sets.has_set_bonus( WARRIOR_FURY, T17, B4 ) )
      p() -> buff.tier17_4pc_fury_driver -> trigger();

    if ( p() -> talents.reckless_abandon -> ok() )
    {
      rage_resource_gain( RESOURCE_RAGE,
                            p() -> talents.reckless_abandon -> effectN( 2 ).resource( RESOURCE_RAGE ),
                            p() -> gain.battle_cry );
    }

    p() -> buff.corrupted_blood_of_zakajz -> trigger();
  }
};

// Shield Block =============================================================

struct shield_block_t: public warrior_spell_t
{
  shield_block_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "shield_block", p, p -> find_class_spell( "Shield Block" ) )
  {
    parse_options( options_str );
    cooldown -> duration = data().charge_cooldown();
    cooldown -> charges = data().charges();
    use_off_gcd = true;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if ( p() -> buff.shield_block -> check() )
      p() -> buff.shield_block -> extend_duration( p(), p() -> buff.shield_block -> data().duration() );
    else
      p() -> buff.shield_block -> trigger();
  }

  bool ready() override
  {
    if ( !p() -> has_shield_equipped() )
      return false;

    return warrior_spell_t::ready();
  }
};

// Shield Wall ==============================================================

struct shield_wall_t: public warrior_spell_t
{
  shield_wall_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "shield_wall", p, p -> find_class_spell( "Shield Wall" ) )
  {
    parse_options( options_str );
    harmful = false;
    range = -1;
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> spec.bastion_of_defense -> effectN( 2 ).time_value();
  }

  void execute() override
  {
    warrior_spell_t::execute();

    double value = p() -> buff.shield_wall -> data().effectN( 1 ).percent();

    p() -> buff.shield_wall -> trigger( 1, value );
  }
};

// Spell Reflection  ==============================================================

struct spell_reflection_t: public warrior_spell_t
{
  spell_reflection_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "spell_reflection", p, p -> find_class_spell( "Spell Reflection" ) )
  {
    parse_options( options_str );
  }
};

// Taunt =======================================================================

struct taunt_t: public warrior_spell_t
{
  taunt_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "taunt", p, p -> find_class_spell( "Taunt" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
    ignore_false_positive = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    warrior_spell_t::impact( s );
  }
};

// Vigilance =======================================================================

struct vigilance_t: public warrior_spell_t
{
  vigilance_t( warrior_t* p, const std::string& options_str ):
    warrior_spell_t( "vigilance", p, p -> talents.vigilance )
  {
    parse_options( options_str );
  }
};

} // UNNAMED NAMESPACE

// warrior_t::create_action  ================================================

action_t* warrior_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "auto_attack"          ) return new auto_attack_t          ( this, options_str );
  if ( name == "avatar"               ) return new avatar_t               ( this, options_str );
  if ( name == "berserker_rage"       ) return new berserker_rage_t       ( this, options_str );
  if ( name == "bladestorm"           ) return new bladestorm_t           ( this, options_str );
  if ( name == "bloodthirst"          ) return new bloodthirst_t          ( this, options_str );
  if ( name == "charge"               ) return new charge_t               ( this, options_str );
  if ( name == "cleave"               ) return new cleave_t               ( this, options_str );
  if ( name == "colossus_smash"       ) return new colossus_smash_t       ( this, options_str );
  if ( name == "corrupted_rage"       ) return new corrupted_rage_t       ( this, options_str );
  if ( name == "defensive_stance"     ) return new defensive_stance_t     ( this, options_str );
  if ( name == "demoralizing_shout"   ) return new demoralizing_shout     ( this, options_str );
  if ( name == "devastate"            ) return new devastate_t            ( this, options_str );
  if ( name == "die_by_the_sword"     ) return new die_by_the_sword_t     ( this, options_str );
  if ( name == "dragon_roar"          ) return new dragon_roar_t          ( this, options_str );
  if ( name == "enraged_regeneration" ) return new enraged_regeneration_t ( this, options_str );
  if ( name == "execute"              ) return new execute_t              ( this, options_str );
  if ( name == "focused_rage"         ) return new focused_rage_t         ( this, options_str );
  if ( name == "furious_slash"        ) return new furious_slash_t        ( this, options_str );
  if ( name == "hamstring"            ) return new hamstring_t            ( this, options_str );
  if ( name == "heroic_charge"        ) return new heroic_charge_t        ( this, options_str );
  if ( name == "heroic_leap"          ) return new heroic_leap_t          ( this, options_str );
  if ( name == "heroic_throw"         ) return new heroic_throw_t         ( this, options_str );
  if ( name == "impending_victory"    ) return new impending_victory_t    ( this, options_str );
  if ( name == "intervene"            ) return new intervene_t            ( this, options_str );
  if ( name == "last_stand"           ) return new last_stand_t           ( this, options_str );
  if ( name == "mortal_strike"        ) return new mortal_strike_t        ( this, options_str );
  if ( name == "pummel"               ) return new pummel_t               ( this, options_str );
  if ( name == "overpower"            ) return new overpower_t            ( this, options_str );
  if ( name == "rampage"              ) return new rampage_parent_t       ( this, options_str );
  if ( name == "raging_blow"          ) return new raging_blow_t          ( this, options_str );
  if ( name == "commanding_shout"     ) return new commanding_shout_t     ( this, options_str );
  if ( name == "ravager"              ) return new ravager_t              ( this, options_str );
  if ( name == "battle_cry"           ) return new battle_cry_t           ( this, options_str );
  if ( name == "rend"                 ) return new rend_t                 ( this, options_str );
  if ( name == "revenge"              ) return new revenge_t              ( this, options_str );
  if ( name == "shield_block"         ) return new shield_block_t         ( this, options_str );
  if ( name == "shield_slam"          ) return new shield_slam_t          ( this, options_str );
  if ( name == "shield_wall"          ) return new shield_wall_t          ( this, options_str );
  if ( name == "shockwave"            ) return new shockwave_t            ( this, options_str );
  if ( name == "slam"                 ) return new slam_t                 ( this, options_str );
  if ( name == "spell_reflection"     ) return new spell_reflection_t     ( this, options_str );
  if ( name == "storm_bolt"           ) return new storm_bolt_t           ( this, options_str );
  if ( name == "taunt"                ) return new taunt_t                ( this, options_str );
  if ( name == "thunder_clap"         ) return new thunder_clap_t         ( this, options_str );
  if ( name == "victory_rush"         ) return new victory_rush_t         ( this, options_str );
  if ( name == "vigilance"            ) return new vigilance_t            ( this, options_str );
  if ( name == "whirlwind"            ) return new whirlwind_parent_t     ( this, options_str );

  return player_t::create_action( name, options_str );
}

// warrior_t::init_spells ===================================================

void warrior_t::init_spells()
{
  player_t::init_spells();

 // Mastery
  mastery.critical_block        = find_mastery_spell( WARRIOR_PROTECTION );
  mastery.colossal_might        = find_mastery_spell( WARRIOR_ARMS );
  mastery.unshackled_fury       = find_mastery_spell( WARRIOR_FURY );

  // Spec Passives
  spec.bastion_of_defense       = find_specialization_spell( "Bastion of Defense" );
  spec.battle_cry               = find_specialization_spell( "Battle Cry" );
  spec.bladed_armor             = find_specialization_spell( "Bladed Armor" );
  spec.bloodthirst              = find_specialization_spell( "Bloodthirst" );
  spec.cleave                   = find_specialization_spell( "Cleave" );
  spec.colossus_smash           = find_specialization_spell( "Colossus Smash" );
  spec.commanding_shout         = find_specialization_spell( "Commanding Shout" );
  spec.deep_wounds              = find_specialization_spell( "Deep Wounds" );
  spec.demoralizing_shout       = find_specialization_spell( "Demoralizing Shout" );
  spec.devastate                = find_specialization_spell( "Devastate" );
  spec.die_by_the_sword         = find_specialization_spell( "Die By the Sword" );
  spec.enrage                   = find_specialization_spell( "Enrage" );
  spec.enraged_regeneration     = find_specialization_spell( "Enraged Regeneration" );
  spec.execute                  = find_specialization_spell( "Execute" );
  spec.furious_slash            = find_specialization_spell( "Furious Slash" );
  spec.hamstring                = find_specialization_spell( "Hamstring" );
  spec.last_stand               = find_specialization_spell( "Last Stand" );
  spec.meat_cleaver             = find_specialization_spell( "Meat Cleaver" );
  spec.focused_rage             = find_specialization_spell( "Focused Rage" );
  spec.mortal_strike            = find_specialization_spell( "Mortal Strike" );
  spec.piercing_howl            = find_specialization_spell( "Piercing Howl" );
  spec.protection               = find_specialization_spell( "Protection" );
  spec.raging_blow              = find_specialization_spell( "Raging Blow" );
  spec.rampage                  = find_specialization_spell( "Rampage" );
  spec.resolve                  = find_specialization_spell( "Resolve" );
  spec.revenge                  = find_specialization_spell( "Revenge" );
  spec.riposte                  = find_specialization_spell( "Riposte" );
  spec.seasoned_soldier         = find_specialization_spell( "Seasoned Soldier" );
  spec.shield_block             = find_specialization_spell( "Shield Block" );
  spec.shield_slam              = find_specialization_spell( "Shield Slam" );
  spec.shield_wall              = find_specialization_spell( "Shield Wall" );
  spec.singleminded_fury        = find_specialization_spell( "Single-Minded Fury" );
  spec.slam                     = find_specialization_spell( "Slam" );
  spec.sword_and_board          = find_specialization_spell( "Sword and Board" );
  spec.tactician                = find_specialization_spell( "Tactician" );
  spec.thunder_clap             = find_specialization_spell( "Thunder Clap" );
  spec.titans_grip              = find_specialization_spell( "Titan's Grip" );
  spec.unwavering_sentinel      = find_specialization_spell( "Unwavering Sentinel" );
  spec.whirlwind                = find_specialization_spell( "Whirlwind" );

  // Talents
  talents.anger_management      = find_talent_spell( "Anger Management" );
  talents.avatar                = find_talent_spell( "Avatar" );//
  talents.best_served_cold      = find_talent_spell( "Best Served Cold" );
  talents.bladestorm            = find_talent_spell( "Bladestorm" );
  talents.booming_voice         = find_talent_spell( "Booming Voice" );
  talents.bounding_stride       = find_talent_spell( "Bounding Stride" );
  talents.carnage               = find_talent_spell( "Carnage" );
  talents.crackling_thunder     = find_talent_spell( "Crackling Thunder" );
  talents.dauntless             = find_talent_spell( "Dauntless" );
  talents.die_by_the_sword      = find_talent_spell( "Die by the Sword" );
  talents.double_time           = find_talent_spell( "Double Time" );
  talents.dragon_roar           = find_talent_spell( "Dragon Roar" );
  talents.endless_rage          = find_talent_spell( "Endless Rage" );
  talents.fervor_of_battle      = find_talent_spell( "Fervor of Battle" );
  talents.focused_rage          = find_talent_spell( "Focused Rage" );
  talents.frenzy                = find_talent_spell( "Frenzy" );
  talents.fresh_meat            = find_talent_spell( "Fresh Meat" );
  talents.frothing_berserker    = find_talent_spell( "Frothing Berserker" );
  talents.furious_charge        = find_talent_spell( "Furious Charge");
  talents.heavy_repercussions   = find_talent_spell( "Heavy Repercussions" );
  talents.in_for_the_kill       = find_talent_spell( "In For The Kill" );
  talents.inner_rage            = find_talent_spell( "Inner Rage" );
  talents.into_the_fray         = find_talent_spell( "Into The Fray" );
  talents.massacre              = find_talent_spell( "Massacre" );
  talents.meat_grinder          = find_talent_spell( "Meat Grinder" );
  talents.mortal_combo          = find_talent_spell( "Mortal Combo" );
  talents.never_surrender       = find_talent_spell( "Never Surrender" );
  talents.opportunity_strikes   = find_talent_spell( "Opportunity Strikes" );
  talents.outburst              = find_talent_spell( "Outburst" );
  talents.overpower             = find_talent_spell( "Overpower" );
  talents.ravager               = find_talent_spell( "Ravager" );
  talents.reckless_abandon      = find_talent_spell( "Reckless Abandon" );
  talents.rend                  = find_talent_spell( "Rend" );
  talents.renewed_fury          = find_talent_spell( "Renewed Fury" );
  talents.second_wind           = find_talent_spell( "Second Wind" );
  talents.shockwave             = find_talent_spell( "Shockwave" );
  talents.storm_bolt            = find_talent_spell( "Storm Bolt" );
  talents.sweeping_strikes      = find_talent_spell( "Sweeping Strikes" );
  talents.titanic_might         = find_talent_spell( "Titanic Might" );
  talents.trauma                = find_talent_spell( "Trauma" );
  talents.vengeance             = find_talent_spell( "Vengeance" );
  talents.vigilance             = find_talent_spell( "Vigilance" );
  talents.war_machine           = find_talent_spell( "War Machine" );
  talents.warbringer            = find_talent_spell( "Warbringer" );
  talents.warpaint              = find_talent_spell( "Warpaint" );
  talents.wrecking_ball         = find_talent_spell( "Wrecking Ball" );

  // Artifact
  artifact.corrupted_blood_of_zakajz = find_artifact_spell( "Corrupted Blood of Zakajz" );
  artifact.corrupted_rage            = find_artifact_spell( "Corrupted Rage" );
  artifact.crushing_blows            = find_artifact_spell( "Crushing Blows" );
  artifact.deathblow                 = find_artifact_spell( "Deathblow" );
  artifact.defensive_measures        = find_artifact_spell( "Defensive Measures" );
  artifact.exploit_the_weakness      = find_artifact_spell( "Exploit the Weakness" );
  artifact.focus_in_battle           = find_artifact_spell( "Focus in Battle" );
  artifact.one_against_many          = find_artifact_spell( "One Against Many" );
  artifact.precise_strikes           = find_artifact_spell( "Precise Strikes" );
  artifact.shattered_defenses        = find_artifact_spell( "Shattered Defenses" );
  artifact.tactical_advance          = find_artifact_spell( "Tactical Advance" );
  artifact.thoradins_might           = find_artifact_spell( "Thoradin's Might" );
  artifact.touch_of_zakajz           = find_artifact_spell( "Touch of Zakajz" );
  artifact.unending_rage             = find_artifact_spell( "Unending Rage" );
  artifact.void_cleave               = find_artifact_spell( "Void Cleave" );
  artifact.war_veteran               = find_artifact_spell( "War Veteran" );

  artifact.odyns_fury                = find_artifact_spell( "Odyn's Fury" );
  artifact.thirst_for_battle         = find_artifact_spell( "Thirst for Battle" );
  artifact.unstoppable               = find_artifact_spell( "Unstoppable" );
  artifact.raging_berserker          = find_artifact_spell( "Raging Berserker" );
  artifact.battle_scars              = find_artifact_spell( "Battle Scars" );
  artifact.unrivaled_strength        = find_artifact_spell( "Unrivaled Strength" );
  artifact.uncontrolled_rage         = find_artifact_spell( "Uncontrolled Rage" );
  artifact.rage_of_the_valarjar      = find_artifact_spell( "Rage of the Valarjar" );
  artifact.bloodcraze                = find_artifact_spell( "Bloodcraze" );
  artifact.sense_death               = find_artifact_spell( "Sense Death" );
  artifact.focus_in_chaos            = find_artifact_spell( "Focus in Chaos" );
  artifact.helyas_wrath              = find_artifact_spell( "Helya's Wrath" );
  artifact.odyns_champion            = find_artifact_spell( "Odyn's Champion" );
  artifact.wrath_and_fury            = find_artifact_spell( "Wrath and Fury" );
  artifact.deathdealer               = find_artifact_spell( "Deathdealer" );
  artifact.juggernaut                = find_artifact_spell( "Juggernaut" );
  artifact.will_of_the_first_king    = find_artifact_spell( "Will of the First King" );

  // Generic spells
  spell.charge                  = find_class_spell( "Charge" );
  spell.colossus_smash_debuff   = find_spell( 208086 );
  spell.defensive_stance        = find_class_spell( "Defensive Stance" );
  spell.fervor_of_battle        = find_spell( 202317 );
  if ( specialization() == WARRIOR_FURY )
  { spell.indomitable = find_spell( 202095 ); }
  spell.intervene               = find_class_spell( "Intervene" );
  spell.headlong_rush           = find_spell( 137047 ); // Also may be used for other crap in the future.
  spell.heroic_leap             = find_class_spell( "Heroic Leap" );
  spell.overpower_driver        = find_spell( 119938 );
  spell.revenge_trigger         = find_class_spell( "Revenge Trigger" );

  // Active spells
  active.deep_wounds               = nullptr;
  active.corrupted_blood_of_zakajz = nullptr;
  active.trauma                    = nullptr;
  active.opportunity_strikes       = nullptr;

  if ( spec.deep_wounds -> ok() ) active.deep_wounds = new deep_wounds_t( this );
  if ( talents.opportunity_strikes -> ok() ) active.opportunity_strikes = new opportunity_strikes_t( this );
  if ( talents.trauma -> ok() ) active.trauma = new trauma_dot_t( this );
  if ( artifact.corrupted_blood_of_zakajz.rank() ) active.corrupted_blood_of_zakajz = new corrupted_blood_of_zakajz_t( this );
  if ( sets.has_set_bonus( WARRIOR_PROTECTION, T17, B4 ) )  spell.t17_prot_2p = find_spell( 169688 );
  if ( spec.rampage -> ok() )
  {
    rampage_attack_t* first = new rampage_attack_t( this, spec.rampage -> effectN( 4 ).trigger(), "rampage1" );
    rampage_attack_t* second = new rampage_attack_t( this, spec.rampage -> effectN( 5 ).trigger(), "rampage2" );
    rampage_attack_t* third = new rampage_attack_t( this, spec.rampage -> effectN( 6 ).trigger(), "rampage3" );
    rampage_attack_t* fourth = new rampage_attack_t( this, spec.rampage -> effectN( 7 ).trigger(), "rampage4" );
    first -> weapon = &( this -> off_hand_weapon );
    second -> weapon = &( this -> main_hand_weapon );
    third -> weapon = &( this -> off_hand_weapon );
    fourth -> weapon = &( this -> main_hand_weapon );
    first -> weapon_multiplier *= 1.0 + artifact.unstoppable.percent();
    second -> weapon_multiplier *= 1.0 + artifact.unstoppable.percent();
    third -> weapon_multiplier *= 1.0 + artifact.unstoppable.percent();
    fourth -> weapon_multiplier *= 1.0 + artifact.unstoppable.percent();
    this -> rampage_attacks.push_back( first );
    this -> rampage_attacks.push_back( second );
    this -> rampage_attacks.push_back( third );
    this -> rampage_attacks.push_back( fourth );
  }

  // Cooldowns
  cooldown.battle_cry               = get_cooldown( "battle_cry" );
  cooldown.charge                   = get_cooldown( "charge" );
  cooldown.colossus_smash           = get_cooldown( "colossus_smash" );
  cooldown.demoralizing_shout       = get_cooldown( "demoralizing_shout" );
  cooldown.heroic_leap              = get_cooldown( "heroic_leap" );
  cooldown.last_stand               = get_cooldown( "last_stand" );
  cooldown.mortal_strike            = get_cooldown( "mortal_strike" );
  cooldown.rage_from_charge         = get_cooldown( "rage_from_charge" );
  cooldown.rage_from_charge -> duration = timespan_t::from_seconds( 12.0 );
  cooldown.rage_from_crit_block     = get_cooldown( "rage_from_crit_block" );
  cooldown.rage_from_crit_block -> duration = timespan_t::from_seconds( 3.0 );
  cooldown.revenge                  = get_cooldown( "revenge" );
  cooldown.revenge_reset            = get_cooldown( "revenge_reset" );
  cooldown.revenge_reset -> duration = spell.revenge_trigger -> internal_cooldown();
  cooldown.shield_slam              = get_cooldown( "shield_slam" );
  cooldown.shield_wall              = get_cooldown( "shield_wall" );
  cooldown.shockwave                = get_cooldown( "shockwave" );
}

// warrior_t::init_base =====================================================

void warrior_t::init_base_stats()
{
  player_t::init_base_stats();

  resources.base[RESOURCE_RAGE] = 100 + artifact.unending_rage.value();

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility = 0.0;

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base.dodge += spec.bastion_of_defense -> effectN( 2 ).percent();

  base_gcd = timespan_t::from_seconds( 1.5 );

  // initialize resolve for prot
  if ( specialization() == WARRIOR_PROTECTION )
    resolve_manager.init();
}

// warrior_t::has_t18_class_trinket ============================================

bool warrior_t::has_t18_class_trinket() const
{
  if ( specialization() == WARRIOR_FURY )
  {
    return fury_trinket != nullptr;
  }
  else if ( specialization() == WARRIOR_ARMS )
  {
    return arms_trinket != nullptr;
  }
  else if ( specialization() == WARRIOR_PROTECTION )
  {
    return prot_trinket != nullptr;
  }
  return false;
}

// Pre-combat Action Priority List============================================

void warrior_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( true_level > 90 )
    {
      if ( primary_role() == ROLE_ATTACK )
        flask_action += "greater_draenic_strength_flask";
      else if ( primary_role() == ROLE_TANK )
        flask_action += "greater_draenic_stamina_flask";
    }
    else
    {
      if ( primary_role() == ROLE_ATTACK )
        flask_action += "winters_bite";
      else if ( primary_role() == ROLE_TANK )
        flask_action += "earth";
    }
    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food )
  {
    std::string food_action = "food,type=";
    if ( specialization() == WARRIOR_FURY )
    {
      if ( level() > 90 )
        food_action += "pickled_eel";
      else
        food_action += "black_pepper_ribs_and_shrimp";
    }
    else if ( specialization() == WARRIOR_ARMS )
    {
      if ( level() > 90 )
        food_action += "sleeper_sushi";
      else
        food_action += "black_pepper_ribs_and_shrimp";
    }
    else
    {
      if ( level() > 90 )
        food_action += "sleeper_sushi";
      else
        food_action += "chun_tian_spring_rolls";
    }
    precombat -> add_action( food_action );
  }
  /*
  if ( specialization() == WARRIOR_ARMS )
  {
    talent_overrides_str +=
  }
  else if ( specialization() == WARRIOR_FURY )
  {
    talent_overrides_str +=
  }
  */
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  //Pre-pot
  if ( sim -> allow_potions )
  {
    if ( true_level > 90 )
    {
      if ( specialization() != WARRIOR_PROTECTION )
        precombat -> add_action( "potion,name=draenic_strength" );
      else
        precombat -> add_action( "potion,name=draenic_armor" );
    }
    else if ( true_level >= 80 )
    {
      if ( primary_role() == ROLE_ATTACK )
        precombat -> add_action( "potion,name=mogu_power" );
      else if ( primary_role() == ROLE_TANK )
        precombat -> add_action( "potion,name=mountains" );
    }
  }
}

// Fury Warrior Action Priority List ========================================

void warrior_t::apl_fury()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* movement = get_action_priority_list( "movement" );
  action_priority_list_t* single_target = get_action_priority_list( "single_target" );
  action_priority_list_t* two_targets = get_action_priority_list( "two_targets" );
  action_priority_list_t* three_targets = get_action_priority_list( "three_targets" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );
  action_priority_list_t* bladestorm = get_action_priority_list( "bladestorm" );
  action_priority_list_t* reck_anger_management = get_action_priority_list( "reck_anger_management" );
  action_priority_list_t* reck_no_anger = get_action_priority_list( "reck_no_anger" );

  default_list -> add_action( "auto_attack" );
  default_list -> add_action( "run_action_list,name=movement,if=movement.distance>5", "This is mostly to prevent cooldowns from being accidentally used during movement." );
  default_list -> add_action( this, "Berserker Rage", "if=buff.enrage.down" );
  default_list -> add_action( this, "Heroic Leap", "if=(raid_event.movement.distance>25&raid_event.movement.in>45)|!raid_event.movement.exists" );

  size_t num_items = items.size();
  for ( size_t i = 0; i < num_items; i++ )
  {
    if ( items[i].name_str == "scabbard_of_kyanos" )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=(spell_targets.whirlwind>1|!raid_event.adds.exists)&((talent.bladestorm.enabled&cooldown.bladestorm.remains=0)|buff.avatar.up|target.time_to_die<25)" );
    else if ( items[i].name_str == "vial_of_convulsive_shadows" )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=(spell_targets.whirlwind>1|!raid_event.adds.exists)&((talent.bladestorm.enabled&cooldown.bladestorm.remains=0)|buff.battle_cry.up|target.time_to_die<25|!talent.anger_management.enabled)" );
    else if ( items[i].name_str == "thorasus_the_stone_heart_of_draenor" )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=(spell_targets.whirlwind>1|!raid_event.adds.exists)&((talent.bladestorm.enabled&cooldown.bladestorm.remains=0)|buff.battle_cry.up|target.time_to_die<25)" );
  }

  if ( sim -> allow_potions )
  {
    if ( true_level > 90 )
      default_list -> add_action( "potion,name=draenic_strength,if=(target.health.pct<20&buff.battle_cry.up)|target.time_to_die<=30" );
    else if ( true_level >= 80 )
      default_list -> add_action( "potion,name=mogu_power,if=(target.health.pct<20&buff.battle_cry.up)|target.time_to_die<=30" );
  }

  default_list -> add_action( "run_action_list,name=single_target,if=(raid_event.adds.cooldown<60&raid_event.adds.count>2&spell_targets.whirlwind=1)|raid_event.movement.cooldown<5", "Skip cooldown usage if we can line them up with bladestorm on a large set of adds, or if movement is coming soon." );
  default_list -> add_action( this, "Battle Cry", "if=target.time_to_die<15&(talent.bladestorm.enabled&(!raid_event.adds.exists|enemies=1))|!talent.bladestorm.enabled" );
  default_list -> add_action( "call_action_list,name=reck_anger_management,if=talent.anger_management.enabled&((talent.bladestorm.enabled&(!raid_event.adds.exists|enemies=1))|!talent.bladestorm.enabled)" );
  default_list -> add_action( "call_action_list,name=reck_no_anger,if=!talent.anger_management.enabled&((talent.bladestorm.enabled&(!raid_event.adds.exists|enemies=1))|!talent.bladestorm.enabled)" );

  reck_anger_management -> add_action( this, "Battle Cry", "if=(target.time_to_die>140|target.health.pct<20)" );
  reck_no_anger -> add_action( this, "Battle Cry", "if=(target.time_to_die>190|target.health.pct<20)" );

  default_list -> add_talent( this, "Avatar", "if=buff.battle_cry.up|cooldown.battle_cry.remains>60|target.time_to_die<30" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      default_list -> add_action( racial_actions[i] + ",if=rage<rage.max-40" );
    else
      default_list -> add_action( racial_actions[i] + ",if=buff.battle_cry.up" );
  }

  default_list -> add_action( "call_action_list,name=two_targets,if=spell_targets.whirlwind=2" );
  default_list -> add_action( "call_action_list,name=three_targets,if=spell_targets.whirlwind=3" );
  default_list -> add_action( "call_action_list,name=aoe,if=spell_targets.whirlwind>3" );
  default_list -> add_action( "call_action_list,name=single_target" );

  movement -> add_action( this, "Heroic Leap" );
  movement -> add_action( this, "Charge" );
  movement -> add_talent( this, "Storm Bolt", "", "May as well throw storm bolt if we can." );
  movement -> add_action( this, "Heroic Throw" );

  single_target -> add_action( this, "Battle Cry", "if=target.health.pct<20&raid_event.adds.exists" );
  single_target -> add_action( this, "Bloodthirst", "if=buff.enrage.down" );
  single_target -> add_talent( this, "Ravager", "if=!raid_event.adds.exists|raid_event.adds.in>60|target.time_to_die<40" );
  single_target -> add_talent( this, "Siegebreaker" );
  single_target -> add_talent( this, "Storm Bolt" );
  single_target -> add_action( this, "Execute", "if=buff.enrage.up|target.time_to_die<12" );
  single_target -> add_talent( this, "Dragon Roar" );
  single_target -> add_action( this, "Raging Blow" );
  single_target -> add_talent( this, "Bladestorm", "if=!raid_event.adds.exists" );
  single_target -> add_talent( this, "Shockwave", "if=!talent.unquenchable_thirst.enabled" );
  single_target -> add_action( this, "Bloodthirst" );

  two_targets -> add_talent( this, "Ravager" );
  two_targets -> add_talent( this, "Dragon Roar" );
  two_targets -> add_action( "call_action_list,name=bladestorm" );
  two_targets -> add_action( this, "Bloodthirst", "if=buff.enrage.down|rage<40" );
  two_targets -> add_talent( this, "Siegebreaker" );
  two_targets -> add_action( this, "Execute", "cycle_targets=1" );
  two_targets -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.up|target.health.pct<20" );
  two_targets -> add_action( this, "Whirlwind", "if=!buff.meat_cleaver.up&target.health.pct>20" );
  two_targets -> add_action( this, "Bloodthirst" );
  two_targets -> add_action( this, "Whirlwind" );

  three_targets -> add_talent( this, "Ravager" );
  three_targets -> add_action( "call_action_list,name=bladestorm" );
  three_targets -> add_action( this, "Bloodthirst", "if=buff.enrage.down|rage<50" );
  three_targets -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.up" );
  three_targets -> add_talent( this, "Siegebreaker" );
  three_targets -> add_action( this, "Execute", "cycle_targets=1" );
  three_targets -> add_talent( this, "Dragon Roar" );
  three_targets -> add_action( this, "Whirlwind", "if=target.health.pct>20" );
  three_targets -> add_action( this, "Bloodthirst" );
  three_targets -> add_action( this, "Raging Blow" );

  aoe -> add_talent( this, "Ravager" );
  aoe -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.up&buff.enrage.up" );
  aoe -> add_action( this, "Bloodthirst", "if=buff.enrage.down|rage<50" );
  aoe -> add_action( "call_action_list,name=bladestorm" );
  aoe -> add_action( this, "Whirlwind" );
  aoe -> add_talent( this, "Siegebreaker" );
  aoe -> add_talent( this, "Dragon Roar" );
  aoe -> add_action( this, "Bloodthirst" );

  bladestorm -> add_action( this, "Battle Cry", "sync=bladestorm,if=buff.enrage.remains>4&(raid_event.adds.in>60|!raid_event.adds.exists|spell_targets.bladestorm_mh>desired_targets)", "oh god why" );
  bladestorm -> add_talent( this, "Bladestorm", "if=buff.enrage.remains>4&(raid_event.adds.in>60|!raid_event.adds.exists|spell_targets.bladestorm_mh>desired_targets)" );
}

// Arms Warrior Action Priority List ========================================

void warrior_t::apl_arms()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* single_target = get_action_priority_list( "single" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );

  default_list -> add_action( this, "Charge" );
  default_list -> add_action( "auto_attack" );
  default_list -> add_action( "run_action_list,name=aoe,if=spell_targets.cleave>1" );
  default_list -> add_action( "run_action_list,name=single" );

  if ( sim -> allow_potions )
  {
    if ( true_level > 90 )
      default_list -> add_action( "potion,name=draenic_strength,if=(target.health.pct<20&buff.battle_cry.up)|target.time_to_die<25" );
    else if ( true_level >= 80 )
      default_list -> add_action( "potion,name=mogu_power,if=(target.health.pct<20&buff.battle_cry.up)|target.time_to_die<25" );
  }

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[i] == "arcane_torrent" )
      default_list -> add_action( racial_actions[i] + ",if=rage<rage.max-40" );
    else
      default_list -> add_action( racial_actions[i] + ",if=debuff.colossus_smash.up|buff.battle_cry.up" );
  }

  single_target -> add_talent( this, "Rend", "if=remains<gcd" );
  single_target -> add_action( this, "Battle Cry", "sync=colossus_smash" );
  single_target -> add_action( this, "Battle Cry", "if=buff.colossus_smash_up.remains>=5|(buff.colossus_smash_up.up&cooldown.colossus_smash.remains=0)" );
  single_target -> add_talent( this, "Avatar", "sync=colossus_smash" );
  single_target -> add_talent( this, "Avatar", "if=buff.colossus_smash_up.remains>=5|(buff.colossus_smash_up.up&cooldown.colossus_smash.remains=0)" );
  if ( artifact.corrupted_rage.rank() )
    single_target -> add_action( "corrupted_rage,if=debuff.colossus_smash.down" );
  single_target -> add_action( this, "Colossus Smash", "if=debuff.colossus_smash.down" );
  single_target -> add_talent( this, "Bladestorm", "if=buff.battle_cry.up" );
  single_target -> add_talent( this, "Overpower" );
  single_target -> add_action( this, "Execute", "if=debuff.colossus_smash.up" );
  single_target -> add_action( this, "Mortal Strike", "if=target.health.pct>20&(!talent.mortal_combo.enabled|(charges=2|debuff.colossus_smash.up))" );
  single_target -> add_action( this, "Execute", "if=rage.deficit<50" );
  single_target -> add_talent( this, "Storm Bolt" );
  single_target -> add_action( this, "Colossus Smash", "if=buff.shattered_defenses.down&buff.precise_strikes.down" );
  single_target -> add_action( this, "Slam", "if=target.health.pct>20&(debuff.colossus_smash.up|rage.deficit<50)" );
  single_target -> add_talent( this, "Rend", "if=remains<=duration*0.3" );
  single_target -> add_action( this, "Execute" );
  single_target -> add_action( this, "Slam", "if=target.health.pct>20" );
  single_target -> add_action( "heroic_charge" );
  single_target -> add_talent( this, "Shockwave" );

  aoe -> add_action( this, "Cleave", "if=buff.cleave.down" );
  aoe -> add_action( this, "Whirlwind" );
  aoe -> add_action( this, "Colossus Smash", "cycle_targets=1,if=debuff.colossus_smash.down" );
  aoe -> add_action( "heroic_charge" );
}

// Protection Warrior Action Priority List ========================================

void warrior_t::apl_prot()
{
  //threshold for defensive abilities
  std::string threshold = "incoming_damage_2500ms>health.max*0.1";

  std::vector<std::string> racial_actions = get_racial_actions();

  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* prot = get_action_priority_list( "prot" );
  action_priority_list_t* prot_aoe = get_action_priority_list( "prot_aoe" );

  default_list -> add_action( this, "charge" );
  default_list -> add_action( "auto_attack" );

  size_t num_items = items.size();
  for ( size_t i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      default_list -> add_action( "use_item,name=" + items[i].name_str + ",if=active_enemies=1|(active_enemies>=2&buff.ravager_protection.up)" );
  }
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] );
  default_list -> add_action( "call_action_list,name=prot" );

  //defensive
  prot -> add_action( this, "Shield Block", "if=!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.shield_block.up)" );
  prot -> add_action( this, "Demoralizing Shout", "if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.shield_block.up|buff.potion.up)" );
  prot -> add_talent( this, "Enraged Regeneration", "if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.shield_block.up|buff.potion.up)" );
  prot -> add_action( this, "Shield Wall", "if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.shield_block.up|buff.potion.up)" );
  prot -> add_action( this, "Last Stand", "if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.shield_block.up|buff.potion.up)" );

  //potion
  if ( sim -> allow_potions )
  {
    if ( true_level > 90 )
      prot -> add_action( "potion,name=draenic_armor,if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.shield_block.up|buff.potion.up)|target.time_to_die<=25" );
    else if ( true_level >= 80 )
      prot -> add_action( "potion,name=mountains,if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.shield_block.up|buff.potion.up)|target.time_to_die<=25" );
  }

  //stoneform
  prot -> add_action( "stoneform,if=" + threshold + "&!(debuff.demoralizing_shout.up|buff.ravager_protection.up|buff.shield_wall.up|buff.last_stand.up|buff.shield_block.up|buff.potion.up)" );

  //dps-single-target
  prot -> add_action( "call_action_list,name=prot_aoe,if=spell_targets.thunder_clap>3" );
  prot -> add_talent( this, "Avatar", "if=talent.avatar.enabled&((cooldown.ravager.remains=0&talent.ravager.enabled)|(cooldown.dragon_roar.remains=0&talent.dragon_roar.enabled)|(talent.storm_bolt.enabled&cooldown.storm_bolt.remains=0)|(!(talent.dragon_roar.enabled|talent.ravager.enabled|talent.storm_bolt.enabled)))" );
  prot -> add_action( this, "Shield Slam" );
  prot -> add_action( this, "Revenge" );
  prot -> add_talent( this, "Ravager" );
  prot -> add_talent( this, "Storm Bolt" );
  prot -> add_talent( this, "Impending Victory", "if=talent.impending_victory.enabled&cooldown.shield_slam.remains<=execute_time" );
  prot -> add_action( this, "Victory Rush", "if=!talent.impending_victory.enabled&cooldown.shield_slam.remains<=execute_time" );
  prot -> add_action( this, "Devastate" );

  //dps-aoe
  prot_aoe -> add_talent( this, "Avatar" );
  prot_aoe -> add_action( this, "Thunder Clap", "if=!dot.deep_wounds.ticking" );
  prot_aoe -> add_action( this, "Heroic Leap", "if=(raid_event.movement.distance>25&raid_event.movement.in>45)|!raid_event.movement.exists" );
  prot_aoe -> add_action( this, "Shield Slam", "if=buff.shield_block.up" );
  prot_aoe -> add_talent( this, "Ravager", "if=(buff.avatar.up|cooldown.avatar.remains>10)|!talent.avatar.enabled" );
  prot_aoe -> add_talent( this, "Shockwave" );
  prot_aoe -> add_action( this, "Revenge" );
  prot_aoe -> add_action( this, "Thunder Clap" );
  prot_aoe -> add_talent( this, "Bladestorm" );
  prot_aoe -> add_action( this, "Shield Slam" );
  prot_aoe -> add_talent( this, "Storm Bolt" );
  prot_aoe -> add_action( this, "Shield Slam" );
  prot_aoe -> add_action( this, "Devastate" );
}

// NO Spec Combat Action Priority List

void warrior_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( this, "Heroic Throw" );
}

namespace rppm
{

template <typename Base>
struct warrior_real_ppm_t: public Base
{
public:
  typedef warrior_real_ppm_t base_t;

  warrior_real_ppm_t( warrior_t& p, const real_ppm_t& params ):
    Base( params ), warrior( p )
  {}

protected:
  warrior_t& warrior;
};

struct overpower_t: public warrior_real_ppm_t < real_ppm_t >
{
  overpower_t( warrior_t& p ):
    base_t( p, real_ppm_t( p, p.spell.overpower_driver -> real_ppm(), 1.0, RPPM_HASTE ) )
  {}
};

struct shadow_slash_t: public warrior_real_ppm_t < real_ppm_t >
{
  shadow_slash_t( warrior_t& p ):
    base_t( p, real_ppm_t( p, p.stromkar_the_warbreaker -> driver() -> real_ppm(), 1.0, RPPM_NONE ) )
  {}
};

struct rage_of_the_valarjar_t: public warrior_real_ppm_t < real_ppm_t >
{
  rage_of_the_valarjar_t( warrior_t& p ):
    base_t( p, real_ppm_t( p, p.warswords_of_the_valarjar -> driver() -> real_ppm(), 1.0, RPPM_NONE ) )
  {}
};

struct wrecking_ball_t : public warrior_real_ppm_t < real_ppm_t >
{
  wrecking_ball_t( warrior_t& p ):
    base_t( p, real_ppm_t( p, p.talents.wrecking_ball -> real_ppm(), 1.0, RPPM_HASTE ) )
  {}
};
};

// ==========================================================================
// Warrior Buffs
// ==========================================================================

namespace buffs
{
template <typename Base>
struct warrior_buff_t: public Base
{
public:
  typedef warrior_buff_t base_t;
  warrior_buff_t( warrior_td_t& p, const buff_creator_basics_t& params ):
    Base( params ), warrior( p.warrior )
  {}

  warrior_buff_t( warrior_t& p, const buff_creator_basics_t& params ):
    Base( params ), warrior( p )
  {}

  warrior_td_t& get_td( player_t* t ) const
  {
    return *( warrior.get_target_data( t ) );
  }

protected:
  warrior_t& warrior;
};

struct commanding_shout_t: public warrior_buff_t < buff_t >
{
  int health_gain;
  commanding_shout_t( warrior_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s ) ),
    health_gain( 0 )
  {}

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    health_gain = static_cast<int>( util::floor( warrior.resources.max[RESOURCE_HEALTH] * warrior.spec.commanding_shout -> effectN( 1 ).percent() ) );
    warrior.stat_gain( STAT_MAX_HEALTH, health_gain, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    return base_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    warrior.stat_loss( STAT_MAX_HEALTH, health_gain, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct last_stand_t: public warrior_buff_t < buff_t >
{
  int health_gain;
  last_stand_t( warrior_t& p, const std::string&n, const spell_data_t*s ):
    base_t( p, buff_creator_t( &p, n, s ).cd( timespan_t::zero() ) ), health_gain( 0 )
  {}

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    health_gain = static_cast<int>( util::floor( warrior.resources.max[RESOURCE_HEALTH] * warrior.spec.last_stand -> effectN( 1 ).percent() ) );
    warrior.stat_gain( STAT_MAX_HEALTH, health_gain, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    return base_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    warrior.stat_loss( STAT_MAX_HEALTH, health_gain, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct debuff_demo_shout_t: public warrior_buff_t < buff_t >
{
  debuff_demo_shout_t( warrior_td_t& p ):
    base_t( p, buff_creator_t( p, "demoralizing_shout", p.source -> find_specialization_spell( "Demoralizing Shout" ) ) )
  {
    default_value = data().effectN( 1 ).percent();
  }
};
} // end namespace buffs

// ==========================================================================
// Warrior Character Definition
// ==========================================================================

warrior_td_t::warrior_td_t( player_t* target, warrior_t& p ):
  actor_target_data_t( target, &p ), warrior( p )
{
  using namespace buffs;

  dots_deep_wounds = target -> get_dot( "deep_wounds", &p );
  dots_ravager = target -> get_dot( "ravager", &p );
  dots_rend = target -> get_dot( "rend", &p );
  dots_trauma = target -> get_dot( "trauma", &p );

  debuffs_colossus_smash = buff_creator_t( *this, "colossus_smash" )
    .default_value( p.spell.colossus_smash_debuff -> effectN( 3 ).percent() )
    .duration( p.spell.colossus_smash_debuff -> duration()
             * ( 1.0 + p.talents.titanic_might -> effectN( 1 ).percent() ) )
    .cd( timespan_t::zero() );

  debuffs_demoralizing_shout = new buffs::debuff_demo_shout_t( *this );
  debuffs_taunt = buff_creator_t( *this, "taunt", p.find_class_spell( "Taunt" ) );
}

// warrior_t::init_buffs ====================================================

void warrior_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  buff.massacre = buff_creator_t( this, "massacre", talents.massacre -> effectN( 1 ).trigger() );

  buff.avatar = buff_creator_t( this, "avatar", talents.avatar )
    .cd( timespan_t::zero() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.berserker_rage = buff_creator_t( this, "berserker_rage", find_class_spell( "Berserker Rage" ) );

  buff.frothing_berserker = buff_creator_t( this, "frothing_berserker", talents.frothing_berserker -> effectN( 1 ).trigger() )
    .default_value( talents.frothing_berserker -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.bounding_stride = buff_creator_t( this, "bounding_stride", find_spell( 202164 ) )
    .chance( talents.bounding_stride -> ok() )
    .default_value( find_spell( 202164 ) -> effectN( 1 ).percent() );

  buff.bladestorm = buff_creator_t( this, "bladestorm", talents.bladestorm )
    .period( timespan_t::zero() )
    .cd( timespan_t::zero() );

  buff.cleave = buff_creator_t( this, "cleave", find_spell( 188923 ) )
    .default_value( find_spell( 188923 ) -> effectN( 1 ).percent()
      + artifact.one_against_many.percent() );

  buff.colossus_smash = buff_creator_t( this, "colossus_smash_up", spell.colossus_smash_debuff )
    .duration( spell.colossus_smash_debuff -> duration()
             * ( 1.0 + talents.titanic_might -> effectN( 1 ).percent() ) )
    .cd( timespan_t::zero() );

  buff.corrupted_blood_of_zakajz = buff_creator_t( this, "corrupted_blood_of_zakajz",
    artifact.corrupted_blood_of_zakajz.data().effectN( 1 ).trigger() )
    .default_value( artifact.corrupted_blood_of_zakajz.percent() )
    .chance( artifact.corrupted_blood_of_zakajz.rank() );

  buff.defensive_stance = buff_creator_t( this, "defensive_stance", find_class_spell( "Defensive Stance" ) )
    .activated( true )
    .add_invalidate( CACHE_EXP )
    .add_invalidate( CACHE_CRIT_AVOIDANCE )
    .add_invalidate( CACHE_CRIT_BLOCK )
    .add_invalidate( CACHE_BLOCK )
    .add_invalidate( CACHE_STAMINA )
    .add_invalidate( CACHE_ARMOR )
    .add_invalidate( CACHE_BONUS_ARMOR );

  buff.die_by_the_sword = buff_creator_t( this, "die_by_the_sword", spec.die_by_the_sword )
    .default_value( spec.die_by_the_sword -> effectN( 2 ).percent() )
    .cd( timespan_t::zero() )
    .add_invalidate( CACHE_PARRY );

  buff.dragon_roar = buff_creator_t( this, "dragon_roar", talents.dragon_roar )
    .default_value( talents.dragon_roar -> effectN( 2 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.enrage = buff_creator_t( this, "enrage", spec.enrage -> effectN( 2 ).trigger() )
    .can_cancel( false )
    .add_invalidate( CACHE_ATTACK_SPEED )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.frenzy = buff_creator_t( this, "frenzy", talents.frenzy -> effectN( 1 ).trigger() )
    .add_invalidate( CACHE_HASTE )
    .default_value( talents.frenzy -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  buff.heroic_leap_movement = buff_creator_t( this, "heroic_leap_movement" );
  buff.charge_movement = buff_creator_t( this, "charge_movement" );
  buff.intervene_movement = buff_creator_t( this, "intervene_movement" );

  buff.meat_grinder = buff_creator_t( this, "meat_grinder", find_spell( 213283 ) )
    .chance( talents.meat_grinder -> ok() );

  buff.focused_rage = buff_creator_t( this, "focused_rage", specialization() == WARRIOR_ARMS ? talents.focused_rage : spec.focused_rage )
    .default_value( specialization() == WARRIOR_ARMS ? talents.focused_rage -> effectN( 1 ).percent() : spec.focused_rage -> effectN( 1 ).percent() );

  buff.last_stand = new buffs::last_stand_t( *this, "last_stand", spec.last_stand );

  buff.meat_cleaver = buff_creator_t( this, "meat_cleaver", spec.meat_cleaver -> effectN( 1 ).trigger() );

  buff.taste_for_blood = buff_creator_t( this, "taste_for_blood", spec.furious_slash -> effectN( 4 ).trigger() )
    .default_value( spec.furious_slash -> effectN( 4 ).trigger() -> effectN( 1 ).percent() );

  buff.commanding_shout = new buffs::commanding_shout_t( *this, "commanding_shout", find_spell( 97463 ) );

  buff.overpower = buff_creator_t( this, "overpower", spell.overpower_driver -> effectN( 1 ).trigger() );

  buff.wrecking_ball = buff_creator_t( this, "wrecking_ball", talents.wrecking_ball -> effectN( 1 ).trigger() );

  buff.precise_strikes = buff_creator_t( this, "precise_strikes", artifact.precise_strikes.data().effectN( 1 ).trigger() )
    .default_value( artifact.precise_strikes.percent() )
    .chance( artifact.precise_strikes.rank() > 0 );

  buff.ravager = buff_creator_t( this, "ravager", talents.ravager );

  buff.ravager_protection = buff_creator_t( this, "ravager_protection", talents.ravager )
    .add_invalidate( CACHE_PARRY );

  buff.sense_death = buff_creator_t( this, "sense_death", artifact.sense_death.data().effectN( 1 ).trigger() )
    .chance( artifact.sense_death.percent() );

  buff.battle_cry = buff_creator_t( this, "battle_cry", spec.battle_cry )
    .add_invalidate( CACHE_CRIT )
    .cd( timespan_t::zero() );

  buff.shattered_defenses = buff_creator_t( this, "shattered_defenses", artifact.shattered_defenses.data().effectN( 1 ).trigger() )
    .default_value( artifact.shattered_defenses.data().effectN( 1 ).trigger() -> effectN( 1 ).percent() )
    .chance( artifact.shattered_defenses.rank() );

  buff.shield_block = buff_creator_t( this, "shield_block", find_spell( 132404 ) )
    .cd( timespan_t::zero() )
    .add_invalidate( CACHE_BLOCK );

  buff.shield_wall = buff_creator_t( this, "shield_wall", spec.shield_wall )
    .default_value( spec.shield_wall -> effectN( 1 ).percent() )
    .cd( timespan_t::zero() );

  buff.sword_and_board = buff_creator_t( this, "sword_and_board", find_spell( 50227 ) )
    .chance( spec.sword_and_board -> effectN( 1 ).percent() );

  buff.tier17_2pc_arms = buff_creator_t( this, "tier17_2pc_arms", sets.set( WARRIOR_ARMS, T17, B2 ) -> effectN( 1 ).trigger() )
    .chance( sets.set( WARRIOR_ARMS, T17, B2 ) -> proc_chance() );

  buff.tier17_4pc_fury = buff_creator_t( this, "rampage", sets.set( WARRIOR_FURY, T17, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
    .add_invalidate( CACHE_ATTACK_SPEED )
    .add_invalidate( CACHE_CRIT );

  buff.tier17_4pc_fury_driver = buff_creator_t( this, "rampage_driver", sets.set( WARRIOR_FURY, T17, B4 ) -> effectN( 1 ).trigger() )
    .tick_callback( [ this ]( buff_t*, int, const timespan_t& ) { buff.tier17_4pc_fury -> trigger( 1 ); } );
}

// warrior_t::init_scaling ==================================================

void warrior_t::init_scaling()
{
  player_t::init_scaling();

  if ( specialization() == WARRIOR_FURY )
    scales_with[STAT_WEAPON_OFFHAND_DPS] = true;

  if ( specialization() == WARRIOR_PROTECTION )
    scales_with[STAT_BONUS_ARMOR] = true;

  scales_with[STAT_AGILITY] = false;
}

// warrior_t::init_gains ====================================================

void warrior_t::init_gains()
{
  player_t::init_gains();

  gain.avoided_attacks = get_gain( "avoided_attacks" );
  gain.bloodthirst = get_gain( "bloodthirst" );
  gain.charge = get_gain( "charge" );
  gain.critical_block = get_gain( "critical_block" );
  gain.in_for_the_kill = get_gain( "in_for_the_kill" );
  gain.melee_crit = get_gain( "melee_crit" );
  gain.fervor_of_battle = get_gain( "fervor_of_battle" );
  gain.melee_main_hand = get_gain( "melee_main_hand" );
  gain.melee_off_hand = get_gain( "melee_off_hand" );
  gain.raging_blow = get_gain( "raging_blow" );
  gain.battle_cry = get_gain( "battle_cry" );
  gain.revenge = get_gain( "revenge" );
  gain.shield_slam = get_gain( "shield_slam" );
  gain.sword_and_board = get_gain( "sword_and_board" );
  gain.will_of_the_first_king = get_gain( "will_of_the_first_king" );

  gain.tier17_4pc_arms = get_gain( "tier17_4pc_arms" );

  if ( !fury_trinket )
  {
    buff.fury_trinket = buff_creator_t( this, "berserkers_fury" )
      .chance( 0 );
  }
  if ( !warswords_of_the_valarjar )
  {
    buff.berserking = buff_creator_t( this, "berserking" )
      .chance( 0 );
    buff.berserking_driver = buff_creator_t( this, "berserking_driver" )
      .chance( 0 );
  }
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

  proc.t17_2pc_fury = get_proc( "t17_2pc_fury" );
  proc.t17_2pc_arms = get_proc( "t17_2pc_arms" );
  proc.arms_trinket = get_proc( "arms_trinket" );
  proc.tactician    = get_proc( "tactician"    );
}

// warrior_t::init_rng ========================================================

void warrior_t::init_rng()
{
  player_t::init_rng();

  if ( talents.overpower )
    rppm.overpower = std::unique_ptr<rppm::overpower_t>( new rppm::overpower_t( *this ) );
  if ( stromkar_the_warbreaker )
    rppm.shadow_slash = std::unique_ptr<rppm::shadow_slash_t>( new rppm::shadow_slash_t( *this ) );
  if ( warswords_of_the_valarjar )
    rppm.rage_of_the_valarjar = std::unique_ptr<rppm::rage_of_the_valarjar_t>( new rppm::rage_of_the_valarjar_t( *this ) );
  if ( talents.wrecking_ball )
    rppm.wrecking_ball = std::unique_ptr<rppm::wrecking_ball_t>( new rppm::wrecking_ball_t( *this ) );
}

// warrior_t::init_resources ================================================

void warrior_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[RESOURCE_RAGE] = 0; // By default, simc sets all resources to full. However, Warriors cannot reliably start combat with more than 0 rage.
                                        // This will also ensure that the 20-35 rage from Charge is not overwritten.
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
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );

    quiet = true;
    return;
  }

  clear_action_priority_lists();

  apl_precombat();

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
  apl_default(); // DEFAULT
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
  if ( !sim -> fixed_time )
  {
    if ( warrior_fixed_time )
    {
      for ( size_t i = 0; i < sim -> player_list.size(); ++i )
      {
        player_t* p = sim -> player_list[i];
        if ( p -> specialization() != WARRIOR_FURY && p -> specialization() != WARRIOR_ARMS )
        {
          warrior_fixed_time = false;
          break;
        }
      }
      if ( warrior_fixed_time )
      {
        sim -> fixed_time = true;
        sim -> errorf( "To fix issues with the target exploding <20% range due to execute, fixed_time=1 has been enabled. This gives similar results" );
        sim -> errorf( "to execute's usage in a raid sim, without taking an eternity to simulate. To disable this option, add warrior_fixed_time=0 to your sim." );
      }
    }
  }

  if ( initial_rage > 0 )
    resources.current[RESOURCE_RAGE] = initial_rage; // User specified rage.

  player_t::combat_begin();

  if ( specialization() == WARRIOR_PROTECTION )
    resolve_manager.start();
}

// warrior_t::reset =========================================================

void warrior_t::reset()
{
  player_t::reset();

  if ( specialization() == WARRIOR_PROTECTION )
    buff.defensive_stance -> trigger();
  else if ( buff.defensive_stance -> check() ) // Arms probably doesn't want defensive stance up every time.
    buff.defensive_stance -> expire();

  heroic_charge = nullptr;
  rampage_driver = nullptr;
  if ( rppm.overpower )
    rppm.overpower -> reset();

  if ( rppm.shadow_slash )
    rppm.shadow_slash -> reset();

  if ( rppm.rage_of_the_valarjar )
    rppm.rage_of_the_valarjar -> reset();

  if ( rppm.wrecking_ball )
    rppm.wrecking_ball -> reset();
}

// Movement related overrides. =============================================

void warrior_t::moving()
{
  return;
}

void warrior_t::interrupt()
{
  buff.charge_movement -> expire();
  buff.heroic_leap_movement -> expire();
  buff.intervene_movement -> expire();
  if ( heroic_charge )
  {
    event_t::cancel( heroic_charge );
  }
  if ( rampage_driver )
  {
    event_t::cancel( rampage_driver );
  }
  player_t::interrupt();
}

void warrior_t::teleport( double, timespan_t )
{
  return; // All movement "teleports" are modeled.
}

void warrior_t::trigger_movement( double distance, movement_direction_e direction )
{
  if ( heroic_charge )
  {
    event_t::cancel( heroic_charge ); // Cancel heroic leap if it's running to make sure nothing weird happens when movement from another source is attempted.
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

  if ( buff.avatar -> check() )
    m *= 1.0 + buff.avatar -> data().effectN( 1 ).percent();

  if ( main_hand_weapon.group() == WEAPON_2H && spec.seasoned_soldier -> ok() )
    m *= 1.0 + spec.seasoned_soldier -> effectN( 1 ).percent();

  // --- Enrages ---
  if ( buff.enrage -> check() && mastery.unshackled_fury -> ok() )
  {
    m *= 1.0 + artifact.raging_berserker.percent();
    m *= 1.0 + cache.mastery_value();
  }

  if ( main_hand_weapon.group() == WEAPON_1H &&
       off_hand_weapon.group() == WEAPON_1H )
    m *= 1.0 + spec.singleminded_fury -> effectN( 1 ).percent();

  if ( buff.dragon_roar -> check() )
    m *= 1.0 + buff.dragon_roar -> default_value;

  if ( buff.frothing_berserker -> check() )
    m *= 1.0 + buff.frothing_berserker -> default_value;

  return m;
}

// warrior_t::composite_attribute =============================================

double warrior_t::composite_attribute( attribute_e attr ) const
{
  double a = player_t::composite_attribute( attr );

  switch ( attr )
  {
  case ATTR_STAMINA:
  if ( buff.defensive_stance -> check() )
    a += spec.unwavering_sentinel -> effectN( 1 ).percent() * player_t::composite_attribute( ATTR_STAMINA );
  if ( specialization() == WARRIOR_FURY )
    a += spell.indomitable -> effectN( 1 ).percent() * player_t::composite_attribute( ATTR_STAMINA );
  break;
  default:
  break;
  }
  return a;
}

// warrior_t::composite_melee_haste ===========================================

double warrior_t::composite_melee_haste() const
{
  double a = player_t::composite_melee_haste();

  if ( fury_trinket )
  {
    a *= 1.0 / ( 1.0 + buff.fury_trinket -> stack_value() );
  }

  a *= 1.0 / ( 1.0 + buff.frenzy -> stack_value() );

  return a;
}

// warrior_t::composite_armor_multiplier ======================================

double warrior_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  return a;
}

// warrior_t::composite_melee_expertise =====================================

double warrior_t::composite_melee_expertise( const weapon_t* ) const
{
  double e = player_t::composite_melee_expertise();

  if ( buff.defensive_stance -> check() )
    e += spec.unwavering_sentinel -> effectN( 5 ).percent();

  return e;
}

// warrior_t::composite_mastery =============================================

double warrior_t::composite_mastery() const
{
  return player_t::composite_mastery();
}

// warrior_t::composite_rating_multiplier =====================================

double warrior_t::composite_rating_multiplier( rating_e rating ) const
{
  return player_t::composite_rating_multiplier( rating );
}

// warrior_t::matching_gear_multiplier ======================================

double warrior_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( ( attr == ATTR_STRENGTH ) && ( specialization() != WARRIOR_PROTECTION ) )
    return 0.05;

  if ( ( attr == ATTR_STAMINA ) && ( specialization() == WARRIOR_PROTECTION ) )
    return 0.05;

  return 0.0;
}

// warrior_t::composite_block ================================================

double warrior_t::composite_block() const
{
  // this handles base block and and all block subject to diminishing returns
  double block_subject_to_dr = cache.mastery() * mastery.critical_block -> effectN( 2 ).mastery_value();
  double b = player_t::composite_block_dr( block_subject_to_dr );

  // add in spec-specific block bonuses not subject to DR
  b += spec.bastion_of_defense -> effectN( 1 ).percent();

  // shield block adds 100% block chance
  if ( buff.shield_block -> check() )
    b += buff.shield_block -> data().effectN( 1 ).percent();

  return b;
}

// warrior_t::composite_block_reduction ======================================

double warrior_t::composite_block_reduction() const
{
  double br = player_t::composite_block_reduction();

  // Prot T17 4-pc increases block value by 5% while shield block is active (additive)
  if ( buff.shield_block -> check() )
  {
    if ( sets.has_set_bonus( WARRIOR_PROTECTION, T17, B4 ) )
      br += spell.t17_prot_2p -> effectN( 1 ).percent();
  }

  return br;
}

// warrior_t::composite_melee_attack_power ==================================

double warrior_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += spec.bladed_armor -> effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// warrior_t::composite_parry_rating() ========================================

double warrior_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  // add Riposte
  if ( spec.riposte -> ok() )
    p += composite_melee_crit_rating();

  return p;
}

// warrior_t::composite_parry =================================================

double warrior_t::composite_parry() const
{
  double parry = player_t::composite_parry();

  if ( buff.ravager_protection -> check() )
    parry += talents.ravager -> effectN( 1 ).percent();

  if ( buff.die_by_the_sword -> check() )
    parry += spec.die_by_the_sword -> effectN( 1 ).percent();

  return parry;
}

// warrior_t::composite_attack_power_multiplier ==============================

double warrior_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.critical_block -> ok() )
    ap *= 1.0 + cache.mastery() * mastery.critical_block -> effectN( 5 ).mastery_value();

  return ap;
}

// warrior_t::composite_crit_block =====================================

double warrior_t::composite_crit_block() const
{
  double b = player_t::composite_crit_block();

  if ( mastery.critical_block -> ok() )
    b += cache.mastery() * mastery.critical_block -> effectN( 1 ).mastery_value();

  return b;
}

// warrior_t::composite_crit_avoidance ===========================================

double warrior_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  if ( buff.defensive_stance -> check() )
    c += spec.unwavering_sentinel -> effectN( 4 ).percent();

  return c;
}

// warrior_t::composite_melee_speed ========================================

double warrior_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  if ( buff.tier17_4pc_fury -> check() )
  {
    s /= 1.0 + buff.tier17_4pc_fury -> current_stack *
      sets.set( WARRIOR_FURY, T17, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() -> effectN( 1 ).percent();
  }

  if ( buff.enrage -> check() )
    s /= 1.0 + buff.enrage -> data().effectN( 1 ).percent();

  return s;
}

// warrior_t::composite_melee_crit =========================================

double warrior_t::composite_melee_crit() const
{
  double c = player_t::composite_melee_crit();

  if ( buff.tier17_4pc_fury -> check() )
  {
    c += buff.tier17_4pc_fury -> current_stack *
      sets.set( WARRIOR_FURY, T17, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() -> effectN( 1 ).percent();
  }

  c += buff.battle_cry -> check_value();

  return c;
}

// warrior_t::composite_player_critical_damage_multiplier ==================

double warrior_t::composite_player_critical_damage_multiplier() const
{
  double cdm = player_t::composite_player_critical_damage_multiplier();
  
  cdm *= 1.0 + artifact.unrivaled_strength.percent();

  return player_t::composite_player_critical_damage_multiplier();
}

// warrior_t::composite_spell_crit =========================================

double warrior_t::composite_spell_crit() const
{
  return composite_melee_crit();
}

// warrior_t::composite_leech ==============================================

double warrior_t::composite_leech() const
{
  return player_t::composite_leech();
}

// warrior_t::temporary_movement_modifier ==================================

double warrior_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  // These are ordered in the highest speed movement increase to the lowest, there's no reason to check the rest as they will just be overridden.
  // Also gives correct benefit numbers.
  if ( buff.heroic_leap_movement -> up() )
    temporary = std::max( buff.heroic_leap_movement -> value(), temporary );
  else if ( buff.charge_movement -> up() )
    temporary = std::max( buff.charge_movement -> value(), temporary );
  else if ( buff.intervene_movement -> up() )
    temporary = std::max( buff.intervene_movement -> value(), temporary );
  else if ( buff.bounding_stride -> up() )
    temporary = std::max( buff.bounding_stride -> value(), temporary );
  else if ( buff.frothing_berserker -> up() )
    temporary = std::max( buff.frothing_berserker -> data().effectN( 2 ).percent(), temporary );

  return temporary;
}

// warrior_t::invalidate_cache ==============================================

void warrior_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_ATTACK_CRIT && mastery.critical_block -> ok() )
    player_t::invalidate_cache( CACHE_PARRY );

  if ( c == CACHE_MASTERY && mastery.critical_block -> ok() )
  {
    player_t::invalidate_cache( CACHE_BLOCK );
    player_t::invalidate_cache( CACHE_CRIT_BLOCK );
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
  }
  if ( c == CACHE_MASTERY && mastery.unshackled_fury -> ok() )
    player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  if ( c == CACHE_BONUS_ARMOR && spec.bladed_armor -> ok() )
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
}

// warrior_t::primary_role() ================================================

role_e warrior_t::primary_role() const
{
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
    return s;
  else
    return STAT_NONE;
  default: return s;
  }
}

// warrior_t::assess_damage =================================================

void warrior_t::assess_damage( school_e school,
                               dmg_e    dtype,
                               action_state_t* s )
{
  if ( s -> result == RESULT_HIT ||
       s -> result == RESULT_CRIT ||
       s -> result == RESULT_GLANCE )
  {
    if ( buff.defensive_stance -> up() )
      s -> result_amount *= 1.0 + buff.defensive_stance -> data().effectN( 1 ).percent();

    warrior_td_t* td = get_target_data( s -> action -> player );

    if ( td -> debuffs_demoralizing_shout -> up() )
      s -> result_amount *= 1.0 + td -> debuffs_demoralizing_shout -> value();

    //take care of dmg reduction CDs
    if ( buff.shield_wall -> up() )
      s -> result_amount *= 1.0 + buff.shield_wall -> value();

    if ( buff.die_by_the_sword -> up() )
      s -> result_amount *= 1.0 + buff.die_by_the_sword -> default_value;
  }

  if ( ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY ) && !s -> action -> is_aoe() ) // AoE attacks do not reset revenge.
  {
    if ( cooldown.revenge_reset -> up() )
    { // 3 second internal cooldown on resetting revenge. 
      cooldown.revenge -> reset( true );
      cooldown.revenge_reset -> start();
    }
  }

  if ( prot_trinket )
  {
    if ( school != SCHOOL_PHYSICAL && buff.shield_block -> check() )
      s -> result_amount *= 1.0 + ( prot_trinket -> driver() -> effectN( 1 ).average( prot_trinket -> item ) / 100.0 );
  }

  player_t::assess_damage( school, dtype, s );
}

// warrior_t::create_options ================================================

void warrior_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "initial_rage", initial_rage ) );
  add_option( opt_bool( "non_dps_mechanics", non_dps_mechanics ) );
  add_option( opt_bool( "warrior_fixed_time", warrior_fixed_time ) );
}

// Discordant Chorus Trinket - T18 ================================================

struct fel_cleave_t: public warrior_attack_t
{
  fel_cleave_t( warrior_t* p, const special_effect_t& effect ):
    warrior_attack_t( "fel_cleave", p, p -> find_spell( 184248 ) )
  {
    background = special = may_crit = true;
    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );
    weapon_multiplier = 0;
    aoe = -1;
  }
};

action_t* warrior_t::create_proc_action( const std::string& name, const special_effect_t& effect )
{
  if ( util::str_compare_ci( name, "fel_cleave" ) ) return new fel_cleave_t( this, effect ); // This must be added here so that it takes the damage increase from colossus smash into account.
  return nullptr;
}

// warrior_t::create_profile ================================================

std::string warrior_t::create_profile( save_e type )
{
  if ( specialization() == WARRIOR_PROTECTION && primary_role() == ROLE_TANK )
    position_str = "front";

  return player_t::create_profile( type );
}

// warrior_t::copy_from =====================================================

void warrior_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warrior_t* p = debug_cast<warrior_t*>( source );

  initial_rage = p -> initial_rage;
  non_dps_mechanics = p -> non_dps_mechanics;
  warrior_fixed_time = p -> warrior_fixed_time;
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warrior_report_t: public player_report_extension_t
{
public:
  warrior_report_t( warrior_t& player ):
    p( player )
  {}

  virtual void html_customsection( report::sc_html_stream& /*os*/ ) override
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
  warrior_t& p;
};

static void do_trinket_init( warrior_t*                player,
                             specialization_e         spec,
                             const special_effect_t*& ptr,
                             const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( ! player -> find_spell( effect.spell_id ) -> ok() ||
       player -> specialization() != spec )
  {
    return;
  }
  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

static void fury_trinket( special_effect_t& effect )
{
  warrior_t* s = debug_cast<warrior_t*>( effect.player );
  do_trinket_init( s, WARRIOR_FURY, s -> fury_trinket, effect );

  if ( s -> fury_trinket )
  {
    s -> buff.fury_trinket = buff_creator_t( s, "berserkers_fury", s -> fury_trinket -> driver() -> effectN( 1 ).trigger() )
      .default_value( s -> fury_trinket -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ).average( s -> fury_trinket -> item ) / 100.0 )
      .add_invalidate( CACHE_HASTE );
  }
}

static void arms_trinket( special_effect_t& effect )
{
  warrior_t* s = debug_cast<warrior_t*>( effect.player );
  do_trinket_init( s, WARRIOR_ARMS, s -> arms_trinket, effect );
}

static void prot_trinket( special_effect_t& effect )
{
  warrior_t* s = debug_cast<warrior_t*>( effect.player );
  do_trinket_init( s, WARRIOR_PROTECTION, s -> prot_trinket, effect );
}

// Strom'kar the Warbreaker
static void stromkar_the_warbreaker( special_effect_t& effect )
{
  warrior_t* s = debug_cast<warrior_t*>( effect.player );
  do_trinket_init( s, WARRIOR_ARMS, s -> stromkar_the_warbreaker, effect );
}

static void warswords_of_the_valarjar( special_effect_t& effect )
{
  warrior_t* s = debug_cast<warrior_t*>( effect.player );
  do_trinket_init( s, WARRIOR_FURY, s -> warswords_of_the_valarjar, effect );
  if ( s -> warswords_of_the_valarjar )
  {
    s -> buff.berserking_driver = buff_creator_t( s, "berserking_driver", s -> warswords_of_the_valarjar -> driver() -> effectN( 1 ).trigger() )
      .tick_callback( [ s ]( buff_t*, int, const timespan_t& ) { s -> buff.berserking -> trigger( 1 ); } );
    s -> buff.berserking = buff_creator_t( s, "berserking", s -> warswords_of_the_valarjar -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
      .default_value( s -> warswords_of_the_valarjar -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
      .add_invalidate( CACHE_ATTACK_SPEED )
      .add_invalidate( CACHE_CRIT );
  }
}

// WARRIOR MODULE INTERFACE =================================================

struct warrior_module_t: public module_t
{
  warrior_module_t(): module_t( WARRIOR ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new warrior_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new warrior_report_t( *p ) );
    return p;
  }

  virtual bool valid() const override { return true; }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184926, fury_trinket );
    unique_gear::register_special_effect( 184925, arms_trinket );
    unique_gear::register_special_effect( 184927, prot_trinket );
    unique_gear::register_special_effect( 209579, stromkar_the_warbreaker );
    unique_gear::register_special_effect( 200845, warswords_of_the_valarjar );
  }

  virtual void register_hotfixes() const override
  {}

  virtual void init( player_t* ) const override {}
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};
} // UNNAMED NAMESPACE

const module_t* module_t::warrior()
{
  static warrior_module_t m;
  return &m;
}
