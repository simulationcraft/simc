// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO (Holy):
    - everything, pretty much :(

  TODO (ret):
    - Blinding Light & Repentance
    - Equality & Eye for an Eye
    - bugfixes & cleanup
    - figure out why spells 193984-193987 can't be found
    - A few Artifact Powers
    - Verify speculative implementations of some artifact powers + BoM
    - BoK/BoW

  TODO (prot):
    - everything, pretty much :(
*/
#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// Forward declarations
struct paladin_t;
struct hand_of_sacrifice_redirect_t;
namespace buffs {
                  struct avenging_wrath_buff_t;
                  struct ardent_defender_buff_t;
                  struct wings_of_liberty_driver_t;
                }

// ==========================================================================
// Paladin Target Data
// ==========================================================================

struct paladin_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* execution_sentence;
  } dots;

  struct buffs_t
  {
    buff_t* debuffs_judgment;
  } buffs;

  paladin_td_t( player_t* target, paladin_t* paladin );
};

// ==========================================================================
// Paladin
// ==========================================================================

struct paladin_t : public player_t
{
public:

  // Active
  heal_t*   active_beacon_of_light;
  heal_t*   active_enlightened_judgments;
  action_t* active_blessing_of_might_proc;
  action_t* active_holy_shield_proc;
  heal_t*   active_protector_of_the_innocent;

  const special_effect_t* retribution_trinket;
  const special_effect_t* ashbringer;
  player_t* last_retribution_trinket_target;

  struct active_actions_t
  {
    hand_of_sacrifice_redirect_t* hand_of_sacrifice_redirect;
  } active;

  // Buffs
  struct buffs_t
  {
    // core
    buffs::ardent_defender_buff_t* ardent_defender;
    buffs::avenging_wrath_buff_t* avenging_wrath;
    buff_t* divine_protection;
    buff_t* divine_shield;
    buff_t* guardian_of_ancient_kings;
    buff_t* grand_crusader;
    buff_t* infusion_of_light;
    buff_t* shield_of_the_righteous;

    // talents
    absorb_buff_t* holy_shield_absorb; // Dummy buff to trigger spell damage "blocking" absorb effect

    buff_t* zeal;
    buff_t* seal_of_light;
    buff_t* blessing_of_might;
    buff_t* conviction;

    // Set Bonuses
    buff_t* faith_barricade;      // t17_2pc_tank
    buff_t* defender_of_the_light; // t17_4pc_tank
    buff_t* vindicators_fury;     // WoD Ret PVP 4-piece
    buff_t* wings_of_liberty;     // Most roleplay name. T18 4P Ret bonus
    buffs::wings_of_liberty_driver_t* wings_of_liberty_driver;
    buff_t* retribution_trinket; // 6.2 Spec-Specific Trinket
  } buffs;

  // Gains
  struct gains_t
  {
    // Healing/absorbs
    gain_t* holy_shield;

    // Mana
    gain_t* extra_regen;
    gain_t* mana_beacon_of_light;

    // Holy Power
    gain_t* hp_crusader_strike;
    gain_t* hp_blade_of_justice;
    gain_t* hp_wake_of_ashes;
    gain_t* hp_templars_verdict_refund;
    gain_t* hp_judgment;
  } gains;

  // Cooldowns
  struct cooldowns_t
  {
    // this seems to be required to get Grand Crusader procs working
    cooldown_t* avengers_shield;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* bladed_armor;
    const spell_data_t* boundless_conviction;
    const spell_data_t* conviction;
    const spell_data_t* divine_bulwark;
    const spell_data_t* grand_crusader;
    const spell_data_t* guarded_by_the_light;
    const spell_data_t* hand_of_light;
    const spell_data_t* holy_insight;
    const spell_data_t* infusion_of_light;
    const spell_data_t* lightbringer;
    const spell_data_t* plate_specialization;
    const spell_data_t* riposte;   // hidden
    const spell_data_t* sanctity_of_battle;
    const spell_data_t* sanctuary;
    const spell_data_t* sword_of_light;
    const spell_data_t* sword_of_light_value;
  } passives;

  // Procs
  struct procs_t
  {
    proc_t* eternal_glory;
    proc_t* focus_of_vengeance_reset;
    proc_t* defender_of_the_light;
  } procs;

  real_ppm_t rppm_defender_of_the_light;

  // Spells
  struct spells_t
  {
    const spell_data_t* holy_light;
    const spell_data_t* sanctified_wrath; // needed to pull out spec-specific effects
  } spells;

  // Talents
  struct talents_t
  {
    // Ignore fist of justice/repentance/blinding light

    // Holy
    const spell_data_t* holy_bolt;
    const spell_data_t* lights_hammer;
    const spell_data_t* crusaders_might;
    // TODO: Beacon of Hope seems like a pain
    const spell_data_t* beacon_of_hope;
    const spell_data_t* unbreakable_spirit;
    const spell_data_t* shield_of_vengeance;
    const spell_data_t* devotion_aura;
    const spell_data_t* aura_of_light;
    const spell_data_t* aura_of_mercy;
    const spell_data_t* divine_purpose;
    const spell_data_t* sanctified_wrath;
    const spell_data_t* holy_prism;
    const spell_data_t* stoicism;
    const spell_data_t* daybreak;
    // TODO: test
    const spell_data_t* judgment_of_light;
    const spell_data_t* beacon_of_faith;
    const spell_data_t* beacon_of_the_lightbringer;
    const spell_data_t* beacon_of_the_savior;

    // Protection
    const spell_data_t* first_avenger;
    const spell_data_t* day_of_reckoning;
    const spell_data_t* consecrated_hammer;
    const spell_data_t* holy_shield;
    const spell_data_t* guardians_light;
    const spell_data_t* last_defender;
    const spell_data_t* blessing_of_negation;
    const spell_data_t* blessing_of_salvation;
    const spell_data_t* retribution_aura;
    const spell_data_t* divine_bulwark;
    const spell_data_t* crusaders_judgment;
    const spell_data_t* blessed_hammer;
    const spell_data_t* righteous_protector;
    const spell_data_t* consecrated_ground;
    // TODO: aegis of light has a positional requirement?
    const spell_data_t* aegis_of_light;
    const spell_data_t* knight_templar;
    const spell_data_t* final_stand;

    // Retribution
    const spell_data_t* execution_sentence;
    const spell_data_t* consecration;
    const spell_data_t* fires_of_justice;
    const spell_data_t* crusader_flurry;
    const spell_data_t* zeal;
    const spell_data_t* virtues_blade;
    const spell_data_t* blade_of_wrath;
    const spell_data_t* divine_hammer;
    const spell_data_t* mass_judgment;
    const spell_data_t* blaze_of_light;
    const spell_data_t* divine_steed;
    const spell_data_t* eye_for_an_eye;
    const spell_data_t* final_verdict;
    const spell_data_t* seal_of_light;
    const spell_data_t* holy_wrath;
  } talents;

  struct artifact_spell_data_t
  {
    // Ret
    artifact_power_t wake_of_ashes;
    artifact_power_t deliver_the_justice;
    artifact_power_t highlords_judgment;
    artifact_power_t righteous_blade;
    artifact_power_t might_of_the_templar;
    artifact_power_t endless_resolve;
    artifact_power_t sharpened_edge;
    artifact_power_t deflection;
    artifact_power_t echo_of_the_highlord;
    artifact_power_t unbreakable_will;              // NYI
    artifact_power_t wrath_of_the_ashbringer;
    artifact_power_t embrace_the_light;             // NYI
    artifact_power_t divine_tempest;                // Implemented 20% damage gain but not AoE shift
    artifact_power_t healing_storm;                 // NYI
    artifact_power_t protector_of_the_ashen_blade;  // NYI
    artifact_power_t blades_of_light;
  } artifact;

  player_t* beacon_target;

  timespan_t last_extra_regen;
  timespan_t extra_regen_period;
  double extra_regen_percent;

  paladin_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, PALADIN, name, r ),
    active( active_actions_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    cooldowns( cooldowns_t() ),
    passives( passives_t() ),
    procs( procs_t() ),
    rppm_defender_of_the_light( *this, 0, RPPM_HASTE ),
    spells( spells_t() ),
    talents( talents_t() ),
    beacon_target( nullptr ),
    last_extra_regen( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_period( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_percent( 0.0 )
  {
    last_retribution_trinket_target = nullptr;
    retribution_trinket = nullptr;
    active_beacon_of_light             = nullptr;
    active_enlightened_judgments       = nullptr;
    active_blessing_of_might_proc      = nullptr;
    active_holy_shield_proc            = nullptr;
    active_protector_of_the_innocent   = nullptr;

    cooldowns.avengers_shield = get_cooldown( "avengers_shield" );

    beacon_target = nullptr;

    base.distance = 3;
    regen_type = REGEN_DYNAMIC;
  }

  virtual void      init_base_stats() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_spells() override;
  virtual void      init_action_list() override;
  virtual void      reset() override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;

  // player stat functions
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_mastery() const override;
  virtual double    composite_mastery_rating() const override;
  virtual double    composite_bonus_armor() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_melee_expertise( const weapon_t* weapon ) const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_player_heal_multiplier( const action_state_t* s ) const override;
  virtual double    composite_spell_power( school_e school ) const override;
  virtual double    composite_spell_power_multiplier() const override;
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_parry_rating() const override;
  virtual double    composite_block() const override;
  virtual double    composite_block_reduction() const override;

  // combat outcome functions
  virtual void      assess_damage( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage_imminent( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* ) override;

  virtual void      invalidate_cache( cache_e ) override;
  virtual void      create_options() override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override;
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual void      regen( timespan_t periodicity ) override;
  virtual void      combat_begin() override;

  double  get_divine_judgment() const;
  void    trigger_grand_crusader();
  void    trigger_holy_shield( action_state_t* s );
  virtual bool has_t18_class_trinket() const override;
  void    generate_action_prio_list_prot();
  void    generate_action_prio_list_ret();
  void    generate_action_prio_list_holy();
  void    generate_action_prio_list_holy_dps();

  target_specific_t<paladin_td_t> target_data;

  virtual paladin_td_t* get_target_data( player_t* target ) const override
  {
    paladin_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new paladin_td_t( target, const_cast<paladin_t*>(this) );
    }
    return td;
  }
};


// ==========================================================================
// Paladin Buffs, Part One
// ==========================================================================
// Paladin buffs are split up into two sections. This one is for ones that
// need to be defined before action_t definitions, because those action_t
// definitions call methods of these buffs. Generic buffs that can be defined
// anywhere are also put here. There's a second buffs section near the end
// containing ones that require action_t definitions to function properly.

namespace buffs {
  struct wings_of_liberty_driver_t: public buff_t
  {
    wings_of_liberty_driver_t( player_t* p ):
      buff_t( buff_creator_t( p, "wings_of_liberty_driver", p -> find_spell( 185655 ) )
      .chance( p -> sets.has_set_bonus( PALADIN_RETRIBUTION, T18, B4 ) )
      .quiet( true )
      .tick_callback( [ p ]( buff_t*, int, const timespan_t& ) {
        paladin_t* paladin = debug_cast<paladin_t*>( p );
        paladin -> buffs.wings_of_liberty -> trigger( 1 );
      } ) )
    { }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );

      paladin_t* p = static_cast<paladin_t*>( player );
      p -> buffs.wings_of_liberty -> expire(); // Force the damage buff to fade.
    }
  };

  struct ardent_defender_buff_t: public buff_t
  {
    bool oneup_triggered;

    ardent_defender_buff_t( player_t* p ):
      buff_t( buff_creator_t( p, "ardent_defender", p -> find_specialization_spell( "Ardent Defender" ) ) ),
      oneup_triggered( false )
    {

    }

    void use_oneup()
    {
      oneup_triggered = true;
    }

  };

  struct avenging_wrath_buff_t: public buff_t
  {
    avenging_wrath_buff_t( player_t* p ):
      buff_t( buff_creator_t( p, "avenging_wrath", p -> specialization() == PALADIN_RETRIBUTION ? p -> find_spell( 31884 ) : p -> find_spell( 31842 ) ) ),
      damage_modifier( 0.0 ),
      healing_modifier( 0.0 ),
      crit_bonus( 0.0 ),
      haste_bonus( 0.0 )
    {
      // Map modifiers appropriately based on spec
      if ( p -> specialization() == PALADIN_RETRIBUTION )
      {
        damage_modifier = data().effectN( 1 ).percent();
        healing_modifier = data().effectN( 2 ).percent();

        paladin_t* paladin = static_cast<paladin_t*>( player );
        if ( paladin -> artifact.wrath_of_the_ashbringer.rank() )
        {
          buff_duration += timespan_t::from_millis(paladin -> artifact.wrath_of_the_ashbringer.value());
        }
      }
      else // we're Holy
      {
        damage_modifier = data().effectN( 5 ).percent();
        healing_modifier = data().effectN( 3 ).percent();
        crit_bonus = data().effectN( 2 ).percent();
        haste_bonus = data().effectN( 1 ).percent();

        // invalidate crit and haste
        add_invalidate( CACHE_CRIT );
        add_invalidate( CACHE_HASTE );

        // Lengthen duration if Sanctified Wrath is taken
        const spell_data_t* s = p -> find_talent_spell( "Sanctified Wrath" );
        if ( s -> ok() )
          buff_duration *= 1.0 + s -> effectN( 2 ).percent();
      }

      // let the ability handle the cooldown
      cooldown -> duration = timespan_t::zero();

      // invalidate Damage and Healing for both specs
      add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
    }

    double get_damage_mod() const
    {
      return damage_modifier;
    }

    double get_healing_mod() const
    {
      return healing_modifier;
    }

    double get_crit_bonus() const
    {
      return crit_bonus;
    }

    double get_haste_bonus() const
    {
      return haste_bonus;
    }
    private:
    double damage_modifier;
    double healing_modifier;
    double crit_bonus;
    double haste_bonus;
  };
} // end namespace buffs
// ==========================================================================
// End Paladin Buffs, Part One
// ==========================================================================


// ==========================================================================
// Paladin Ability Templates
// ==========================================================================

// Template for common paladin action code. See priest_action_t.
template <class Base>
struct paladin_action_t : public Base
{
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef paladin_action_t base_t;

  // Sanctity of Battle bools
  bool hasted_cd;
  bool hasted_gcd;
  bool should_trigger_blessing_of_might;

  paladin_action_t( const std::string& n, paladin_t* player,
                    const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    hasted_cd( ab::data().affected_by( player -> passives.sanctity_of_battle -> effectN( 1 ) ) ),
    hasted_gcd( ab::data().affected_by( player -> passives.sanctity_of_battle -> effectN( 2 ) ) ),
    should_trigger_blessing_of_might( true )
  {
  }

  paladin_t* p()
  { return static_cast<paladin_t*>( ab::player ); }
  const paladin_t* p() const
  { return static_cast<paladin_t*>( ab::player ); }

  paladin_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double cost() const
  {
    if ( ab::current_resource() == RESOURCE_HOLY_POWER )
    {
      return ab::base_costs[ RESOURCE_HOLY_POWER ];
    }

    return ab::cost();
  }

  virtual void execute()
  {
    double c = ( this -> current_resource() == RESOURCE_HOLY_POWER ) ? this -> cost() : -1.0;

    ab::execute();

    // if the ability uses Holy Power, handle Divine Purpose and other freebie effects
    if ( c >= 0 )
    {
      // trigger WoD Ret PvP 4-P
      p() -> buffs.vindicators_fury -> trigger( 1, c > 0 ? c : 3 );
    }

    // Handle benefit tracking
    if ( this -> harmful )
    {
      p() -> buffs.avenging_wrath -> up();
      p() -> buffs.wings_of_liberty -> up();
      p() -> buffs.retribution_trinket -> up();
    }
  }

  void trigger_blessing_of_might( action_state_t* s )
  {
    if ( p() -> buffs.blessing_of_might -> up() )
    {
      if ( p() -> rng().roll( p() -> buffs.blessing_of_might -> data().effectN( 1 ).percent() ) )
      {
        double amount = s -> result_amount;
        amount *= p() -> buffs.blessing_of_might -> data().effectN( 2 ).percent();
        p() -> active_blessing_of_might_proc -> base_dd_max = p() -> active_blessing_of_might_proc -> base_dd_min = amount;
        p() -> active_blessing_of_might_proc -> target = s -> target;
        p() -> active_blessing_of_might_proc -> execute();
      }
    }
  }

  virtual void impact( action_state_t* s )
  {
    ab::impact( s );

    trigger_blessing_of_might(s);
  }

  virtual double cooldown_multiplier() { return 1.0; }

  void update_ready( timespan_t cd = timespan_t::min() )
  {
    if ( hasted_cd )
    {
      if ( cd == timespan_t::min() && ab::cooldown && ab::cooldown -> duration > timespan_t::zero() )
      {
        cd = ab::cooldown -> duration;
        cd *= ab::player -> cache.attack_haste();
        cd *= cooldown_multiplier();
      }
    }

    ab::update_ready( cd );
  }
};

// paladin "Spell" Base for paladin_spell_t, paladin_heal_t and paladin_absorb_t

template <class Base>
struct paladin_spell_base_t : public paladin_action_t< Base >
{
private:
  typedef paladin_action_t< Base > ab;
public:
  typedef paladin_spell_base_t base_t;

  paladin_spell_base_t( const std::string& n, paladin_t* player,
                        const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  {
  }

};


// ==========================================================================
// The damage formula in action_t::calculate_direct_amount in sc_action.cpp is documented here:
// https://github.com/simulationcraft/simc/wiki/DevelopersDocumentation#damage-calculations
// ==========================================================================


// ==========================================================================
// Paladin Spells, Heals, and Absorbs
// ==========================================================================

// paladin-specific spell_t, heal_t, and absorb_t classes for inheritance ===

struct paladin_spell_t : public paladin_spell_base_t<spell_t>
{
  paladin_spell_t( const std::string& n, paladin_t* p,
                   const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    // Holy Insight - Holy passive
    base_multiplier *= 1.0 + p -> passives.holy_insight -> effectN( 6 ).percent();
  }
};

struct paladin_heal_t : public paladin_spell_base_t<heal_t>
{
  const spell_data_t* tower_of_radiance;
  paladin_heal_t( const std::string& n, paladin_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s ), tower_of_radiance( p -> find_spell( 88852 ) )
  {
    may_crit          = true;
    tick_may_crit     = true;
    harmful = false;

    weapon_multiplier = 0.0;
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( s -> target != p() -> beacon_target )
      trigger_beacon_of_light( s );
  }

  void trigger_beacon_of_light( action_state_t* s )
  {
    if ( ! p() -> beacon_target )
      return;

    if ( ! p() -> beacon_target -> buffs.beacon_of_light -> up() )
      return;

    if ( proc )
      return;

    assert( p() -> active_beacon_of_light );

    p() -> active_beacon_of_light -> target = p() -> beacon_target;

    double amount = s -> result_amount;
    amount *= p() -> beacon_target -> buffs.beacon_of_light -> data().effectN( 1 ).percent();

    p() -> active_beacon_of_light -> base_dd_min = amount;
    p() -> active_beacon_of_light -> base_dd_max = amount;

    // Holy light heals for 100% instead of 50%
    if ( data().id() == p() -> spells.holy_light -> id() )
    {
      p() -> active_beacon_of_light -> base_dd_min *= 2.0;
      p() -> active_beacon_of_light -> base_dd_max *= 2.0;
    }

    p() -> active_beacon_of_light -> execute();
  };
};

struct paladin_absorb_t : public paladin_spell_base_t< absorb_t >
{
  paladin_absorb_t( const std::string& n, paladin_t* p,
                    const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  { }
};

// Ardent Defender ==========================================================

struct ardent_defender_t : public paladin_spell_t
{
  ardent_defender_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "ardent_defender", p, p -> find_specialization_spell( "Ardent Defender" ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.ardent_defender -> trigger();
  }
};

// Avengers Shield ==========================================================

struct avengers_shield_t : public paladin_spell_t
{
  avengers_shield_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "avengers_shield", p, p -> find_class_spell( "Avenger's Shield" ) )
  {
    parse_options( options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equipped in offhand\n", p -> name(), name() );
      background = true;
    }

    aoe = 3;
    may_crit     = true;

    // link needed for trigger_grand_crusader
    cooldown = p -> cooldowns.avengers_shield;
    cooldown -> duration = data().cooldown();
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    // Protection T17 2-piece grants block buff
    if ( p() -> sets.has_set_bonus( PALADIN_PROTECTION, T17, B2 ) )
      p() -> buffs.faith_barricade -> trigger();
  }
};

// Blessing of Might
struct blessing_of_might_t : public paladin_heal_t
{
  blessing_of_might_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "blessing_of_might", p, p -> find_spell( 203528 ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    paladin_heal_t::execute();
    p() -> buffs.blessing_of_might -> trigger();
  }
};

// Avenging Wrath ===========================================================
// AW is two spells now (31884 for Ret, 31842 for Holy) and the effects are all jumbled.
// Thus, we need to use some ugly hacks to get it to work seamlessly for both specs within the same spell.
// Most of them can be found in buffs::avenging_wrath_buff_t, this spell just triggers the buff

struct avenging_wrath_t : public paladin_heal_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "avenging_wrath", p, p -> specialization() == PALADIN_RETRIBUTION ? p -> find_spell( 31884 ) : p -> find_spell( 31842 ) )
  {
    parse_options( options_str );
    // disable for protection
    if ( p -> specialization() == PALADIN_PROTECTION )
      background = true;
    else if ( p -> specialization() == PALADIN_RETRIBUTION )
    {
      // TODO: hackfix to make it work with T18
      cooldown -> duration = data().cooldown();
      cooldown -> charges = 1;//data().charges();
      cooldown -> charges += p -> sets.set( PALADIN_RETRIBUTION, T18, B2 ) -> effectN( 1 ).base_value();
    }

    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  void tick( dot_t* d ) override
  {
    // override for this just in case Avenging Wrath were to get canceled or removed
    // early, or if there's a duration mismatch (unlikely, but...)
    if ( p() -> buffs.avenging_wrath -> check() )
    {
      // call tick()
      heal_t::tick( d );
    }
  }

  void execute() override
  {
    paladin_heal_t::execute();

    p() -> buffs.avenging_wrath -> trigger();
    if ( p() -> sets.has_set_bonus( PALADIN_RETRIBUTION, T18, B4 ) )
    {
      p() -> buffs.wings_of_liberty -> trigger( 1 ); // We have to trigger a stack here.
      p() -> buffs.wings_of_liberty_driver -> trigger();
    }
  }

  bool ready() override
  {
    if ( p() -> buffs.avenging_wrath -> check() )
      return false;
    else
      return paladin_heal_t::ready();
  }
};

// Beacon of Light ==========================================================

struct beacon_of_light_t : public paladin_heal_t
{
  beacon_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "beacon_of_light", p, p -> find_class_spell( "Beacon of Light" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    // Target is required for Beacon
    if ( target_str.empty() )
    {
      sim -> errorf( "Warning %s's \"%s\" needs a target", p -> name(), name() );
      sim -> cancel();
    }

    // Remove the 'dot'
    dot_duration = timespan_t::zero();
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    p() -> beacon_target = target;
    target -> buffs.beacon_of_light -> trigger();
  }
};

struct beacon_of_light_heal_t : public heal_t
{
  beacon_of_light_heal_t( paladin_t* p ) :
    heal_t( "beacon_of_light_heal", p, p -> find_spell( 53652 ) )
  {
    background = true;
    may_crit = false;
    proc = true;
    trigger_gcd = timespan_t::zero();

    target = p -> beacon_target;
  }
};

// Consecration =============================================================

struct consecration_tick_t : public paladin_spell_t
{
  consecration_tick_t( paladin_t* p )
    : paladin_spell_t( "consecration_tick", p, p -> find_spell( 81297 ) )
  {
    aoe         = -1;
    dual        = true;
    direct_tick = true;
    background  = true;
    may_crit    = true;
    ground_aoe = true;
  }
};

struct consecration_t : public paladin_spell_t
{
  consecration_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "consecration", p, p -> specialization() == PALADIN_RETRIBUTION ? p -> find_spell( 205228 ) : p -> find_class_spell( "Consecration" ) )
  {
    parse_options( options_str );

    if ( p -> specialization() == PALADIN_RETRIBUTION )
    {
      background = ! ( p -> talents.consecration -> ok() );
    }

    hasted_ticks   = true;
    hasted_cd = true;
    may_miss       = false;

    tick_action = new consecration_tick_t( p );
  }

  virtual void tick( dot_t* d ) override
  {
    if ( d -> state -> target -> debuffs.flying -> check() )
    {
      if ( sim -> debug ) sim -> out_debug.printf( "Ground effect %s can not hit flying target %s", name(), d -> state -> target -> name() );
    }
    else
    {
      paladin_spell_t::tick( d );
    }
  }
};

// Divine Protection ========================================================

struct divine_protection_t : public paladin_spell_t
{
  divine_protection_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_protection", p, p -> find_class_spell( "Divine Protection" ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.divine_protection -> trigger();
  }
};

struct divine_shield_t : public paladin_spell_t
{
  divine_shield_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_shield", p, p -> find_class_spell( "Divine Shield" ) )
  {
    parse_options( options_str );

    harmful = false;

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    // Technically this should also drop you from the mob's threat table,
    // but we'll assume the MT isn't using it for now
    p() -> buffs.divine_shield -> trigger();

    // in this sim, the only debuffs we care about are enemy DoTs.
    // Check for them and remove them when cast, and apply Glyph of Divine Shield appropriately
    int num_destroyed = 0;
    for ( size_t i = 0, size = p() -> dot_list.size(); i < size; i++ )
    {
      dot_t* d = p() -> dot_list[ i ];

      if ( d -> source != p() && d -> source -> is_enemy() && d -> is_ticking() )
      {
        d -> cancel();
        num_destroyed++;
      }
    }

    // trigger forbearance
    if ( p() -> artifact.endless_resolve.rank() )
      // But not if we have endless resolve!
      return;
    timespan_t duration = p() -> debuffs.forbearance -> data().duration();
    p() -> debuffs.forbearance -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration  );
  }

  virtual bool ready() override
  {
    if ( player -> debuffs.forbearance -> check() )
      return false;

    return paladin_spell_t::ready();
  }
};

// Denounce =================================================================

struct denounce_t : public paladin_spell_t
{
  denounce_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "denounce", p, p -> find_specialization_spell( "Denounce" ) )
  {
    parse_options( options_str );
    may_crit = true;
  }

  void execute() override
  {
    paladin_spell_t::execute();
  }
};

// Execution Sentence =======================================================

struct execution_sentence_t : public paladin_spell_t
{
  execution_sentence_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "execution_sentence", p, p -> find_talent_spell( "Execution Sentence" ) )
  {
    parse_options( options_str );
    hasted_ticks   = false;
    travel_speed   = 0;
    tick_may_crit  = true;

    // disable if not talented
    if ( ! ( p -> talents.execution_sentence -> ok() ) )
      background = true;
  }

  virtual double cost() const override
  {
    double base_cost = paladin_spell_t::cost();
    if ( p() -> buffs.conviction -> up() )
      return base_cost - 1;
    return base_cost;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    if ( ! ( p() -> bugs ) )
      if ( p() -> buffs.conviction -> up() )
        p() -> buffs.conviction -> expire();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_spell_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
      m *= judgment_multiplier;
    }

    return m;
  }
};

// Flash of Light Spell =====================================================

struct flash_of_light_t : public paladin_heal_t
{
  flash_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "flash_of_light", p, p -> find_class_spell( "Flash of Light" ) )
  {
    parse_options( options_str );

    // Sword of light
    base_multiplier *= 1.0 + p -> passives.sword_of_light -> effectN( 6 ).percent();
  }

  virtual void impact( action_state_t* s ) override
  {
    paladin_heal_t::impact( s );

    // Grant Mana if healing the beacon target
    if ( s -> target == p() -> beacon_target ){
        int g = static_cast<int>( tower_of_radiance -> effectN(1).percent() * cost() );
        p() -> resource_gain( RESOURCE_MANA, g, p() -> gains.mana_beacon_of_light );

    }
  }
};

// Guardian of Ancient Kings ============================================

struct guardian_of_ancient_kings_t : public paladin_spell_t
{
  guardian_of_ancient_kings_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "guardian_of_ancient_kings", p, p -> find_specialization_spell( "Guardian of Ancient Kings" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.guardian_of_ancient_kings -> trigger();

  }
};

// Hand of Sacrifice ========================================================

struct hand_of_sacrifice_redirect_t : public paladin_spell_t
{
  hand_of_sacrifice_redirect_t( paladin_t* p ) :
    paladin_spell_t( "hand_of_sacrifice_redirect", p, p -> find_class_spell( "Hand of Sacrifice" ) )
  {
    background = true;
    trigger_gcd = timespan_t::zero();
    may_crit = false;
    may_miss = false;
    base_multiplier = data().effectN( 1 ).percent();
    target = p;
  }

  void trigger( double redirect_value )
  {
    // set the redirect amount based on the result of the action
    base_dd_min = redirect_value;
    base_dd_max = redirect_value;
  }

  // Hand of Sacrifice's Execute function is defined after Paladin Buffs, Part Deux because it requires
  // the definition of the buffs_t::hand_of_sacrifice_t object.
};

struct hand_of_sacrifice_t : public paladin_spell_t
{
  hand_of_sacrifice_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "hand_of_sacrifice", p, p-> find_class_spell( "Hand of Sacrifice" ) )
  {
    parse_options( options_str );

    harmful = false;
    may_miss = false;
//    p -> active.hand_of_sacrifice_redirect = new hand_of_sacrifice_redirect_t( p );


    // Create redirect action conditionalized on the existence of HoS.
    if ( ! p -> active.hand_of_sacrifice_redirect )
      p -> active.hand_of_sacrifice_redirect = new hand_of_sacrifice_redirect_t( p );
  }

  virtual void execute() override;

};

// Holy Light Spell =========================================================

struct holy_light_t : public paladin_heal_t
{
  holy_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_light", p, p -> find_class_spell( "Holy Light" ) )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    p() -> buffs.infusion_of_light -> expire();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> check() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    paladin_heal_t::schedule_execute( state );

    p() -> buffs.infusion_of_light -> up(); // Buff uptime tracking
  }

  virtual void impact( action_state_t* s ) override
  {
    paladin_heal_t::impact( s );

    // Grant Mana if healing the beacon target
    if ( s -> target == p() -> beacon_target ){
        int g = static_cast<int>( tower_of_radiance -> effectN(1).percent() * cost() );
        p() -> resource_gain( RESOURCE_MANA, g, p() -> gains.mana_beacon_of_light );

    }
  }

};

// Holy Shield damage proc ====================================================

struct holy_shield_proc_t : public paladin_spell_t
{
  holy_shield_proc_t( paladin_t* p )
    : paladin_spell_t( "holy_shield", p, p -> find_spell( 157122 ) ) // damage data stored in 157122
  {
    background = true;
    proc = true;
    may_miss = false;
    may_crit = true;
  }

};

// Holy Prism ===============================================================

// Holy Prism AOE (damage) - This is the aoe damage proc that triggers when holy_prism_heal is cast.

struct holy_prism_aoe_damage_t : public paladin_spell_t
{
  holy_prism_aoe_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_prism_aoe_damage", p, p->find_spell( 114871 ) )
  {
    background = true;
    may_crit = true;
    may_miss = false;
    aoe = 5;

  }
};

// Holy Prism AOE (heal) -  This is the aoe healing proc that triggers when holy_prism_damage is cast

struct holy_prism_aoe_heal_t : public paladin_heal_t
{
  holy_prism_aoe_heal_t( paladin_t* p )
    : paladin_heal_t( "holy_prism_aoe_heal", p, p->find_spell( 114871 ) )
  {
    background = true;
    aoe = 5;
  }

};

// Holy Prism (damage) - This is the damage version of the spell; for the heal version, see holy_prism_heal_t

struct holy_prism_damage_t : public paladin_spell_t
{
  holy_prism_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_prism_damage", p, p->find_spell( 114852 ) )
  {
    background = true;
    may_crit = true;
    may_miss = false;
    // this updates the spell coefficients appropriately for single-target damage
    parse_effect_data( p -> find_spell( 114852 ) -> effectN( 1 ) );

    // on impact, trigger a holy_prism_aoe_heal cast
    impact_action = new holy_prism_aoe_heal_t( p );
  }
};


// Holy Prism (heal) - This is the healing version of the spell; for the damage version, see holy_prism_damage_t

struct holy_prism_heal_t : public paladin_heal_t
{
  holy_prism_heal_t( paladin_t* p ) :
    paladin_heal_t( "holy_prism_heal", p, p -> find_spell( 114871 ) )
  {
    background = true;

    // update spell coefficients appropriately for single-target healing
    parse_effect_data( p -> find_spell( 114871 ) -> effectN( 1 ) );

    // on impact, trigger a holy_prism_aoe cast
    impact_action = new holy_prism_aoe_damage_t( p );
  }

};

// Holy Prism - This is the base spell, it chooses the effects to trigger based on the target

struct holy_prism_t : public paladin_spell_t
{
  holy_prism_damage_t* damage;
  holy_prism_heal_t* heal;

  holy_prism_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_prism", p, p->find_spell( 114165 ) )
  {
    parse_options( options_str );

    // create the damage and healing spell effects, designate them as children for reporting
    damage = new holy_prism_damage_t( p );
    add_child( damage );
    heal = new holy_prism_heal_t( p );
    add_child( heal );

    // disable if not talented
    if ( ! ( p -> talents.holy_prism -> ok() ) )
      background = true;
  }

  virtual void execute() override
  {
    if ( target -> is_enemy() )
    {
      // damage enemy
      damage -> target = target;
      damage -> schedule_execute();
    }
    else
    {
      // heal friendly
      heal -> target = target;
      heal -> schedule_execute();
    }

    paladin_spell_t::consume_resource();
    paladin_spell_t::update_ready();
  }
};

// Holy Shock ===============================================================

// Holy Shock is another one of those "heals or does damage depending on target" spells.
// The holy_shock_t structure handles global stuff, and calls one of two children
// (holy_shock_damage_t or holy_shock_heal_t) depending on target to handle healing/damage
// find_class_spell returns spell 20473, which has the cooldown/cost/etc stuff, but the actual
// damage and healing information is in spells 25912 and 25914, respectively.

struct holy_shock_damage_t : public paladin_spell_t
{
  double crit_chance_multiplier;
  double crit_increase;

  holy_shock_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_shock_damage", p, p -> find_spell( 25912 ) ),
      crit_increase( 0.0 )
  {
    background = may_crit = true;
    trigger_gcd = timespan_t::zero();

    // this grabs the 100% base crit bonus from 20473
    crit_chance_multiplier = p -> find_class_spell( "Holy Shock" ) -> effectN( 1 ).base_value() / 10.0;
  }

  virtual double composite_crit() const override
  {
    double cc = paladin_spell_t::composite_crit();

    if ( p() -> buffs.avenging_wrath -> check() )
    {
      cc += crit_increase;
    }

    // effect 1 doubles crit chance
    cc *= crit_chance_multiplier;

    return cc;
  }
};

// Holy Shock Heal Spell ====================================================

struct holy_shock_heal_t : public paladin_heal_t
{
  double crit_chance_multiplier;
  double crit_increase;

  holy_shock_heal_t( paladin_t* p ) :
    paladin_heal_t( "holy_shock_heal", p, p -> find_spell( 25914 ) ),
    crit_increase( 0.0 )
  {
    background = true;
    trigger_gcd = timespan_t::zero();

    // this grabs the crit multiplier bonus from 20473
    crit_chance_multiplier = p -> find_class_spell( "Holy Shock" ) -> effectN( 1 ).base_value() / 10.0;
  }

  virtual double composite_crit() const override
  {
    double cc = paladin_heal_t::composite_crit();

    if ( p() -> buffs.avenging_wrath -> check() )
    {
      cc += crit_increase;
    }

    // effect 1 doubles crit chance
    cc *= crit_chance_multiplier;

    return cc;
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    if ( execute_state -> result == RESULT_CRIT )
      p() -> buffs.infusion_of_light -> trigger();

  }
};

struct holy_shock_t : public paladin_heal_t
{
  holy_shock_damage_t* damage;
  holy_shock_heal_t* heal;
  timespan_t cd_duration;
  double cooldown_mult;
  bool dmg;

  holy_shock_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "holy_shock", p, p -> find_specialization_spell( "Holy Shock" ) ),
    cooldown_mult( 1.0 ), dmg( false )
  {
    add_option( opt_bool( "damage", dmg ) );
    check_spec( PALADIN_HOLY );
    parse_options( options_str );

    cd_duration = cooldown -> duration;

    double crit_increase = 0.0;

    // Bonuses from Sanctified Wrath need to be stored for future use
    if ( ( p -> specialization() == PALADIN_HOLY ) && p -> spells.sanctified_wrath -> ok()  )
    {
      // the actual values of these are stored in spell 114232 rather than the spell returned by find_talent_spell
      cooldown_mult = p -> spells.sanctified_wrath -> effectN( 1 ).percent();
      crit_increase = p -> spells.sanctified_wrath -> effectN( 5 ).percent();
    }

    // create the damage and healing spell effects, designate them as children for reporting
    damage = new holy_shock_damage_t( p );
    damage ->crit_increase = crit_increase;
    add_child( damage );
    heal = new holy_shock_heal_t( p );
    heal ->crit_increase = crit_increase;
    add_child( heal );
  }

  virtual void execute() override
  {
    if ( dmg )
    {
      // damage enemy
      damage -> target = target;
      damage -> schedule_execute();
    }
    else
    {
      // heal friendly
      heal -> target = target;
      heal -> schedule_execute();
    }

    cooldown -> duration = cd_duration;

    paladin_heal_t::execute();
  }

  double cooldown_multiplier() override
  {
    double cdm = paladin_heal_t::cooldown_multiplier();

    if ( p() -> buffs.avenging_wrath -> check() )
      cdm += cooldown_mult;

    return cdm;
  }
};

// Lay on Hands Spell =======================================================

struct lay_on_hands_t : public paladin_heal_t
{
  double mana_return_pct;
  lay_on_hands_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "lay_on_hands", p, p -> find_class_spell( "Lay on Hands" ) ), mana_return_pct( 0 )
  {
      parse_options( options_str );

      // unbreakable spirit reduces cooldown
      if ( p -> talents.unbreakable_spirit -> ok() )
          cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );

      may_crit = false;
      use_off_gcd = true;
      trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    base_dd_min = base_dd_max = p() -> resources.max[RESOURCE_HEALTH];

    paladin_heal_t::execute();

    if ( p() -> artifact.endless_resolve.rank() )
      // Don't trigger forbearance with endless resolve
      return;
    target -> debuffs.forbearance -> trigger();
  }

  virtual bool ready() override
  {
    if ( target -> debuffs.forbearance -> check() )
      return false;

    return paladin_heal_t::ready();
  }
};

// Light's Hammer ===========================================================

struct lights_hammer_damage_tick_t : public paladin_spell_t
{
  lights_hammer_damage_tick_t( paladin_t* p )
    : paladin_spell_t( "lights_hammer_damage_tick", p, p -> find_spell( 114919 ) )
  {
    //dual = true;
    background = true;
    aoe = -1;
    may_crit = true;
    ground_aoe = true;
  }
};

struct lights_hammer_heal_tick_t : public paladin_heal_t
{
  lights_hammer_heal_tick_t( paladin_t* p )
    : paladin_heal_t( "lights_hammer_heal_tick", p, p -> find_spell( 114919 ) )
  {
    dual = true;
    background = true;
    aoe = 6;
    may_crit = true;
  }

  std::vector< player_t* >& target_list() const override
  {
    target_cache.list = paladin_heal_t::target_list();
    target_cache.list = find_lowest_players( aoe );
    return target_cache.list;
  }
};

struct lights_hammer_t : public paladin_spell_t
{
  const timespan_t travel_time_;
  lights_hammer_heal_tick_t* lh_heal_tick;
  lights_hammer_damage_tick_t* lh_damage_tick;

  lights_hammer_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "lights_hammer", p, p -> find_talent_spell( "Light's Hammer" ) ),
      travel_time_( timespan_t::from_seconds( 1.5 ) )
  {
    // 114158: Talent spell, cooldown
    // 114918: Periodic 2s dummy, no duration!
    // 114919: Damage/Scale data
    // 122773: 15.5s duration, 1.5s for hammer to land = 14s aoe dot
    parse_options( options_str );
    may_miss = false;

    school = SCHOOL_HOLY; // Setting this allows the tick_action to benefit from Inquistion

    base_tick_time = p -> find_spell( 114918 ) -> effectN( 1 ).period();
    dot_duration      = p -> find_spell( 122773 ) -> duration() - travel_time_;
    cooldown -> duration = p -> find_spell( 114158 ) -> cooldown();
    hasted_ticks   = false;
    tick_zero = true;
    ignore_false_positive = true;

    dynamic_tick_action = true;
    //tick_action = new lights_hammer_tick_t( p, p -> find_spell( 114919 ) );
    lh_heal_tick = new lights_hammer_heal_tick_t( p );
    add_child( lh_heal_tick );
    lh_damage_tick = new lights_hammer_damage_tick_t( p );
    add_child( lh_damage_tick );

    // disable if not talented
    if ( ! ( p -> talents.lights_hammer -> ok() ) )
      background = true;
  }

  virtual timespan_t travel_time() const override
  { return travel_time_; }

  virtual void tick( dot_t* d ) override
  {
    // trigger healing and damage ticks
    lh_heal_tick -> schedule_execute();
    lh_damage_tick -> schedule_execute();

    paladin_spell_t::tick( d );
  }
};

struct wake_of_ashes_impact_t : public paladin_spell_t
{
  wake_of_ashes_impact_t( paladin_t* p )
    : paladin_spell_t( "wake_of_ashes_impact", p, p -> find_spell( 205290 ))
  {
    aoe = -1;
    background = true;

    // TODO: is this correct?
    weapon_power_mod = 1.0 / 3.5;
  }

  void execute() override
  {
    paladin_spell_t::execute();
    p() -> resource_gain( RESOURCE_HOLY_POWER, 5, p() -> gains.hp_wake_of_ashes );
  }
};

struct wake_of_ashes_t : public paladin_spell_t
{
  wake_of_ashes_impact_t* impact_spell;
  wake_of_ashes_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "wake_of_ashes", p, p -> find_spell( 205273 ) ),
      impact_spell( new wake_of_ashes_impact_t( p ) )
  {
    parse_options( options_str );
    add_child( impact_spell );
    impact_action = impact_spell;
    school = SCHOOL_HOLY;
  }

  bool ready()
  {
    if ( ! player -> artifact_enabled() )
    {
      return false;
    }

    if ( p() -> artifact.wake_of_ashes.rank() == 0 )
    {
      return false;
    }

    return paladin_spell_t::ready();
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.5 );
  }
};

// Light of Dawn ============================================================

struct light_of_dawn_t : public paladin_heal_t
{
  light_of_dawn_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "light_of_dawn", p, p -> find_class_spell( "Light of Dawn" ) )
  {
    parse_options( options_str );

    aoe = 6;

    // Holy Insight buffs all healing by 25% & WoG/EF/LoD by 50%.
    // The 25% buff is already in paladin_heal_t, so we need to divide by that first & then apply 50%
    base_multiplier /= 1.0 + p -> passives.holy_insight -> effectN( 6 ).percent();
    base_multiplier *= 1.0 + p -> passives.holy_insight -> effectN( 9 ).percent();
  }
};

// ==========================================================================
// End Spells, Heals, and Absorbs
// ==========================================================================

// ==========================================================================
// Paladin Attacks
// ==========================================================================

// paladin-specific melee_attack_t class for inheritance

struct paladin_melee_attack_t: public paladin_action_t < melee_attack_t >
{
  bool use2hspec;

  paladin_melee_attack_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(),
                          bool u2h = true ):
                          base_t( n, p, s ),
                          use2hspec( u2h )
  {
    may_crit = true;
    special = true;
    weapon = &( p -> main_hand_weapon );

    // Sword of Light boosts action_multiplier
    if ( use2hspec && ( p -> passives.sword_of_light -> ok() ) && ( p -> main_hand_weapon.group() == WEAPON_2H ) )
    {
      base_multiplier *= 1.0 + p -> passives.sword_of_light_value -> effectN( 1 ).percent();
    }
  }

  virtual timespan_t gcd() const override
  {

    if ( hasted_gcd )
    {
      timespan_t t = action_t::gcd();
      if ( t == timespan_t::zero() ) return timespan_t::zero();

      t *= p() -> cache.attack_haste();
      if ( t < min_gcd ) t = min_gcd;

      return t;
    }
    else
      return base_t::gcd();
  }

  void retribution_trinket_trigger()
  {
    if ( ! p() -> retribution_trinket )
      return;

    if ( ! ( p() -> last_retribution_trinket_target // Target check
          && p() -> last_retribution_trinket_target == execute_state -> target ) )
    {
      // Wrong target, expire stacks and set new target.
      if ( p() -> last_retribution_trinket_target )
      {
        p() -> procs.focus_of_vengeance_reset -> occur();
        p() -> buffs.retribution_trinket -> expire();
      }
      p() -> last_retribution_trinket_target = execute_state -> target;
    }

    p() -> buffs.retribution_trinket -> trigger();
  }
};

struct holy_power_generator_t : public paladin_melee_attack_t
{
  holy_power_generator_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(),
                          bool u2h = true):
                          paladin_melee_attack_t( n, p, s, u2h )
  {

  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
      m *= judgment_multiplier;
    }

    return m;
  }

  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && !background )
    {
      // Conviction
      if ( p() -> passives.conviction -> ok() && rng().roll( p() -> passives.conviction -> proc_chance() ) )
      {
        p() -> buffs.conviction -> trigger();
      }
    }
  }
};

struct holy_power_consumer_t : public paladin_melee_attack_t
{
  holy_power_consumer_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(),
                          bool u2h = true):
                          paladin_melee_attack_t( n, p, s, u2h )
  {

  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
      m *= judgment_multiplier;
    }

    return m;
  }
};

// Melee Attack =============================================================

struct melee_t : public paladin_melee_attack_t
{
  bool first;
  melee_t( paladin_t* p ) :
    paladin_melee_attack_t( "melee", p, spell_data_t::nil(), true ),
    first( true )
  {
    school = SCHOOL_PHYSICAL;
    special               = false;
    background            = true;
    repeating             = true;
    trigger_gcd           = timespan_t::zero();
    base_execute_time     = p -> main_hand_weapon.swing_time;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat ) return timespan_t::from_seconds( 0.01 );
    if ( first )
      return timespan_t::zero();
    else
      return paladin_melee_attack_t::execute_time();
  }

  virtual void execute() override
  {
    if ( first )
      first = false;

    paladin_melee_attack_t::execute();
  }
};

// Auto Attack ==============================================================

struct auto_melee_attack_t : public paladin_melee_attack_t
{
  auto_melee_attack_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "auto_attack", p, spell_data_t::nil(), true )
  {
    school = SCHOOL_PHYSICAL;
    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> main_hand_attack = new melee_t( p );

    // does not incur a GCD
    trigger_gcd = timespan_t::zero();

    parse_options( options_str );
  }

  void execute() override
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  bool ready() override
  {
    if ( p() -> is_moving() )
      return false;

    player_t* potential_target = select_target_if_target();
    if ( potential_target && potential_target != p() -> main_hand_attack -> target )
      p() -> main_hand_attack -> target = potential_target;

    return( p() -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Crusader Strike ==========================================================

struct crusader_strike_t : public holy_power_generator_t
{
  const spell_data_t* sword_of_light;
  crusader_strike_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "crusader_strike", p, p -> find_class_spell( "Crusader Strike" ), true ),
      sword_of_light( p -> find_specialization_spell( "Sword of Light" ) )
  {
    parse_options( options_str );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 7 ).percent()
                                       +  p -> passives.sword_of_light -> effectN( 4 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );

    base_multiplier *= 1.0 + p -> artifact.sharpened_edge.percent();

    background = ( p -> talents.crusader_flurry -> ok() ) || ( p -> talents.zeal -> ok() );
  }

  void execute() override
  {
    holy_power_generator_t::execute();
    retribution_trinket_trigger();
  }

  double action_multiplier() const override
  {
    double am = holy_power_generator_t::action_multiplier();

    // Fires of Justice buffs CS damage by 25%
    if ( p() -> talents.fires_of_justice -> ok() )
    {
      am *= 1.0 + p() -> talents.fires_of_justice -> effectN( 1 ).percent();
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    holy_power_generator_t::impact( s );

    // Special things that happen when CS connects
    if ( result_is_hit( s -> result ) )
    {
      // Holy Power gains, only relevant if CS connects
      int g = data().effectN( 3 ).base_value(); // default is a gain of 1 Holy Power
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_crusader_strike ); // apply gain, record as due to CS
    }
  }
};

// Crusader Flurry ==========================================================

struct crusader_flurry_t : public holy_power_generator_t
{
  const spell_data_t* sword_of_light;
  crusader_flurry_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "crusader_flurry", p, p -> find_talent_spell( "Crusader Flurry" ), true ),
      sword_of_light( p -> find_specialization_spell( "Sword of Light" ) )
  {
    parse_options( options_str );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 7 ).percent()
                                       +  p -> passives.sword_of_light -> effectN( 4 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );

    cooldown -> duration = data().charge_cooldown();
    cooldown -> charges = data().charges();

    base_multiplier *= 1.0 + p -> artifact.sharpened_edge.percent();
  }

  void execute() override
  {
    holy_power_generator_t::execute();
    retribution_trinket_trigger();
  }

  void impact( action_state_t* s ) override
  {
    holy_power_generator_t::impact( s );

    // Special things that happen when CF connects
    if ( result_is_hit( s -> result ) )
    {
      // Holy Power gains, only relevant if CF connects
      int g = data().effectN( 3 ).base_value(); // default is a gain of 1 Holy Power
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_crusader_strike ); // apply gain, record as due to CS
    }
  }
};

// Zeal ==========================================================

struct zeal_t : public holy_power_generator_t
{
  const spell_data_t* sword_of_light;
  zeal_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "zeal", p, p -> find_talent_spell( "Zeal" ), true ),
      sword_of_light( p -> find_specialization_spell( "Sword of Light" ) )
  {
    parse_options( options_str );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 7 ).percent()
                                       +  p -> passives.sword_of_light -> effectN( 4 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );

    cooldown -> duration = data().charge_cooldown();
    cooldown -> charges = data().charges();

    base_multiplier *= 1.0 + p -> artifact.sharpened_edge.percent();
  }

  virtual int n_targets() const override
  {
    if ( p() -> buffs.zeal -> stack() )
      return 1 + p() -> buffs.zeal -> stack();
    return holy_power_generator_t::n_targets();
  }

  void execute() override
  {
    holy_power_generator_t::execute();
    retribution_trinket_trigger();
  }

  void impact( action_state_t* s ) override
  {
    holy_power_generator_t::impact( s );

    // Special things that happen when Zeal connects
    if ( result_is_hit( s -> result ) )
    {
      // Apply Zeal stacks
      p() -> buffs.zeal -> trigger();

      // Holy Power gains, only relevant if Zeal connects
      int g = data().effectN( 3 ).base_value(); // default is a gain of 1 Holy Power
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_crusader_strike ); // apply gain, record as due to CS
    }
  }
};

// Blade of Justice =========================================================


struct blade_of_light_t : public paladin_melee_attack_t
{
  const spell_data_t* sword_of_light;
  blade_of_light_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "blade_of_light", p, p -> find_spell( 193115 ), true ),
      sword_of_light( p -> find_specialization_spell( "Sword of Light" ) )
  {
    parse_options( options_str );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 7 ).percent()
                                       +  p -> passives.sword_of_light -> effectN( 4 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );

    background = true;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
      m *= judgment_multiplier;
    }

    return m;
  }
};

struct blade_of_justice_t : public holy_power_generator_t
{
  blade_of_light_t* bol_proc;
  const spell_data_t* sword_of_light;
  blade_of_justice_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "blade_of_justice", p, p -> find_class_spell( "Blade of Justice" ), true ),
      bol_proc( new blade_of_light_t( p, options_str ) ),
      sword_of_light( p -> find_specialization_spell( "Sword of Light" ) )
  {
    parse_options( options_str );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 7 ).percent()
                                       +  p -> passives.sword_of_light -> effectN( 4 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );

    base_multiplier *= 1.0 + p -> artifact.deliver_the_justice.percent();

    background = ( p -> talents.blade_of_wrath -> ok() ) || ( p -> talents.divine_hammer -> ok() );
  }

  double action_multiplier() const override
  {
    double am = holy_power_generator_t::action_multiplier();

    // Virtue's Blade buffs BoJ damage by 25%
    if ( p() -> talents.virtues_blade -> ok() )
    {
      am *= 1.0 + p() -> talents.virtues_blade -> effectN( 1 ).percent();
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    holy_power_generator_t::impact( s );

    // Special things that happen when BoJ connects
    if ( result_is_hit( s -> result ) )
    {
      // Holy Power gains, only relevant if BoJ connects
      int g = data().effectN( 3 ).base_value(); // default is a gain of 2 Holy Power
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_blade_of_justice ); // apply gain, record as due to BoJ

      if ( p() -> artifact.blades_of_light.rank() )
      {
        int remaining_executes = (int)(rng().range(1, 4));
        if (remaining_executes > 3) {
          remaining_executes = 3;
        }
        for (int i = 0; i < remaining_executes; i++) {
          bol_proc -> target = s -> target;
          bol_proc -> schedule_execute();
        }
      }
    }
  }
};

// Blade of Wrath =========================================================

struct blade_of_wrath_t : public holy_power_generator_t
{
  const spell_data_t* sword_of_light;
  blade_of_light_t* bol_proc;

  blade_of_wrath_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "blade_of_wrath", p, p -> find_talent_spell( "Blade of Wrath" ), true ),
      sword_of_light( p -> find_specialization_spell( "Sword of Light" ) ),
      bol_proc( new blade_of_light_t( p, options_str ) )
  {
    parse_options( options_str );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 7 ).percent()
                                       +  p -> passives.sword_of_light -> effectN( 4 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );

    base_multiplier *= 1.0 + p -> artifact.deliver_the_justice.percent();
  }

  void impact( action_state_t* s ) override
  {
    holy_power_generator_t::impact( s );

    // Special things that happen when BoW connects
    if ( result_is_hit( s -> result ) )
    {
      // Holy Power gains, only relevant if BoW connects
      int g = data().effectN( 3 ).base_value(); // default is a gain of 1 Holy Power
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_blade_of_justice ); // apply gain, record as due to BoJ

      if ( p() -> artifact.blades_of_light.rank() )
      {
        int remaining_executes = (int)(rng().range(1, 4));
        if (remaining_executes > 3) {
          remaining_executes = 3;
        }
        for (int i = 0; i < remaining_executes; i++) {
          bol_proc -> target = s -> target;
          bol_proc -> schedule_execute();
        }
      }
    }
  }
};

// Divine Hammer =========================================================

struct divine_hammer_tick_t : public paladin_melee_attack_t
{
  const spell_data_t* sword_of_light;

  divine_hammer_tick_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "divine_hammer_tick", p, p -> find_spell( 198137 ) ),
      sword_of_light( p -> find_specialization_spell( "Sword of Light" ) )
  {
    aoe         = -1;
    dual        = true;
    direct_tick = true;
    background  = true;
    may_crit    = true;
    ground_aoe = true;

    base_multiplier *= 1.0 + p -> artifact.deliver_the_justice.percent();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();;
      m *= judgment_multiplier;
    }

    return m;
  }

  virtual void execute() override
  {
    paladin_melee_attack_t::execute();
    // Conviction
    if ( p() -> passives.conviction -> ok() && rng().roll( p() -> passives.conviction -> proc_chance() ) )
    {
      p() -> buffs.conviction -> trigger();
    }
  }
};

struct divine_hammer_t : public paladin_spell_t
{
  blade_of_light_t* bol_proc;

  divine_hammer_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "divine_hammer", p, p -> find_talent_spell( "Divine Hammer" ) ),
      bol_proc( new blade_of_light_t( p, options_str ) )
  {
    parse_options( options_str );

    hasted_ticks   = false;
    may_miss       = false;
    tick_zero      = true;

    tick_action = new divine_hammer_tick_t( p, options_str );
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();
    // Holy Power gen
    int g = data().effectN( 2 ).base_value(); // default is a gain of 1 Holy Power
    p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_blade_of_justice ); // apply gain, record as due to BoJ
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );
    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> artifact.blades_of_light.rank() )
      {
        bol_proc -> target = s -> target;
        bol_proc -> schedule_execute();
      }
    }
  }
};

// Divine Storm =============================================================

// TODO(mserrano): this is wrong
// TODO(mserrano): supposedly this has a 77.15% attack power coefficient
struct echoed_divine_storm_t: public paladin_melee_attack_t
{
  echoed_divine_storm_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "echoed_divine_storm", p, p -> find_spell( 186876 ), false )
  {
    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );

    base_multiplier *= 1.0 + p -> artifact.righteous_blade.percent();
    base_multiplier *= 1.0 + p -> artifact.divine_tempest.percent( 2 );

    aoe = -1;
    background = true;
  }

  virtual double cost() const override
  {
    return 0;
  }
};

struct divine_storm_t: public holy_power_consumer_t
{
  echoed_divine_storm_t* echoed_spell;
  divine_storm_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "divine_storm", p, p -> find_class_spell( "Divine Storm" ) ),
      echoed_spell( new echoed_divine_storm_t( p, options_str ) )
  {
    parse_options( options_str );

    weapon = &( p -> main_hand_weapon );

    base_multiplier *= 1.0 + p -> artifact.righteous_blade.percent();
    base_multiplier *= 1.0 + p -> artifact.divine_tempest.percent( 2 );

    aoe = -1;
  }

  virtual double cost() const override
  {
    double base_cost = holy_power_consumer_t::cost();
    if ( p() -> buffs.conviction -> up() )
      return base_cost - 1;
    return base_cost;
  }

  virtual void execute() override
  {
    holy_power_consumer_t::execute();

    if ( p() -> buffs.conviction -> up() )
      p() -> buffs.conviction -> expire();

    if ( p() -> artifact.echo_of_the_highlord.rank() )
    {
      echoed_spell -> schedule_execute();
    }
  }
};

// Hammer of Justice, Fist of Justice =======================================

struct hammer_of_justice_t : public paladin_melee_attack_t
{
  hammer_of_justice_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_justice", p, p -> find_class_spell( "Hammer of Justice" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }
};

// Hammer of the Righteous ==================================================

struct hammer_of_the_righteous_aoe_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t( paladin_t* p )
    : paladin_melee_attack_t( "hammer_of_the_righteous_aoe", p, p -> find_spell( 88263 ), false )
  {
    // AoE effect always hits if single-target attack succeeds
    // Doesn't proc Grand Crusader
    may_dodge = false;
    may_parry = false;
    may_miss  = false;
    background = true;
    aoe       = -1;
    trigger_gcd = timespan_t::zero(); // doesn't incur GCD (HotR does that already)
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    paladin_melee_attack_t::available_targets( tl );

    for ( size_t i = 0; i < tl.size(); i++ )
    {
      if ( tl[i] == target ) // Cannot hit the original target.
      {
        tl.erase( tl.begin() + i );
        break;
      }
    }
    return tl.size();
  }
};

struct hammer_of_the_righteous_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t* hotr_aoe;
  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_the_righteous", p, p -> find_class_spell( "Hammer of the Righteous" ), true )
  {
    parse_options( options_str );
    // link with Crusader Strike's cooldown
    cooldown = p -> get_cooldown( "crusader_strike" );
    cooldown -> duration = data().cooldown();
    hotr_aoe = new hammer_of_the_righteous_aoe_t( p );
    // Attach AoE proc as a child
    add_child( hotr_aoe );
  }

  void execute() override
  {
    paladin_melee_attack_t::execute();

    retribution_trinket_trigger();

    // Special things that happen when HotR connects
    if ( result_is_hit( execute_state -> result ) )
    {
      // Grand Crusader
      p() -> trigger_grand_crusader();

      if ( hotr_aoe -> target != execute_state -> target )
        hotr_aoe -> target_cache.is_valid = false;

      hotr_aoe -> target = execute_state -> target;
      hotr_aoe -> execute();
    }
  }
};

// Blessing of Might proc
// TODO: is this a melee attack?
struct blessing_of_might_proc_t : public paladin_melee_attack_t
{
  // TODO: there is an actual spell here: 205729
  blessing_of_might_proc_t( paladin_t* p )
    : paladin_melee_attack_t( "blessing_of_might_proc", p, spell_data_t::nil(), false )
  {
    school = SCHOOL_HOLY;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    may_glance  = false;
    may_crit    = true;
    background  = true;
    trigger_gcd = timespan_t::zero();
    id          = 205729;

    should_trigger_blessing_of_might = false;

    // No weapon multiplier
    weapon_multiplier = 0.0;
    // Note that setting weapon_multiplier=0.0 prevents STATE_MUL_DA from being added
    // to snapshot_flags because both base_dd_min && base_dd_max are zero, so we need
    // to do it specifically in init().
    // Alternate solution is to set weapon = NULL, but we have to use init() to disable
    // other multipliers (Versatility) anyway.
  }

  // Disable multipliers in init() so that it doesn't double-dip on anything
  virtual void init() override
  {
    paladin_melee_attack_t::init();
    // Disable the snapshot_flags for all multipliers
    snapshot_flags &= STATE_NO_MULTIPLIER;
  }
};

// Ashes to Ashes
// TODO: Does this benefit from SoL? Can it trigger BoM? etc
struct ashen_strike_impact_t : public paladin_melee_attack_t
{
  ashen_strike_impact_t( paladin_t* p )
    : paladin_melee_attack_t( "ashen_strike_impact", p, p -> find_spell( 180290 ), true )
  {
    background = true;
    weapon_multiplier = 0.0;

    school = SCHOOL_HOLY;
  }
};


struct ashen_strike_t : public paladin_spell_t
{
  ashen_strike_impact_t* impact_spell;
  ashen_strike_t( paladin_t* p, unsigned spell_id )
    : paladin_spell_t( "ashen_strike", p, p -> find_spell( spell_id ) ),
      impact_spell( new ashen_strike_impact_t( p ) )
  {
    add_child( impact_spell );
    impact_action = impact_spell;
    school = SCHOOL_HOLY;
  }
};


// Judgment =================================================================

struct judgment_t : public paladin_melee_attack_t
{
  ashen_strike_t* ashen_strike_spells[4];

  // TODO: remove this hack once the missile-spawning spells can be found
  ashen_strike_impact_t* ashen_strike_impact_spell;

  judgment_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "judgment", p, p -> find_spell( "Judgment" ), true )
  {
    parse_options( options_str );

    // no weapon multiplier
    weapon_multiplier = 0.0;

    // Special melee attack that can only miss
    may_glance = may_block = may_parry = may_dodge = false;

    // Guarded by the Light reduces mana cost
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 8 ).percent();

    base_multiplier *= 1.0 + p -> artifact.highlords_judgment.percent();

    if ( p -> talents.mass_judgment -> ok() )
    {
      aoe = -1;
      base_aoe_multiplier = 1;
      // TODO: does this need to be multiplied by the Highlord's Judgment percentage too?
    }

    // These spells are on Wowhead, and seem to be the original missile-spawning spells
    // that actually get cast on an Ashes to Ashes proc. However, simc can't seem to find them.
    // So for now, we use the impact spell directly.
    ashen_strike_spells[0] = new ashen_strike_t( p, 193984 );
    ashen_strike_spells[1] = new ashen_strike_t( p, 193985 );
    ashen_strike_spells[2] = new ashen_strike_t( p, 193986 );
    ashen_strike_spells[3] = new ashen_strike_t( p, 193987 );
    ashen_strike_impact_spell = new ashen_strike_impact_t( p );
  }

  // Special things that happen when Judgment damages target
  virtual void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s->result ) )
    {
      td( s -> target ) -> buffs.debuffs_judgment -> trigger();

      if ( p() -> ashbringer )
      {
        // Ashes to Ashes
        // For some reason the proc chance is effect 2, go figure
        if ( rng().roll( p() -> ashbringer -> driver() -> effectN( 2 ).percent() ) )
        {
          for (unsigned i = 0; i < 4; i++)
          {
            // as mentioned above - simc can't find the original missile spells, so we are
            // directly triggering the impact spell. This should be fixed.
            ashen_strike_impact_spell -> target = s -> target;
            ashen_strike_impact_spell -> schedule_execute();
          }
        }
      }
    }

    paladin_melee_attack_t::impact( s );
  }
};

// Rebuke ===================================================================

struct rebuke_t : public paladin_melee_attack_t
{
  rebuke_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "rebuke", p, p -> find_class_spell( "Rebuke" ) )
  {
    parse_options( options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive = true;

    // no weapon multiplier
    weapon_multiplier = 0.0;
  }

  virtual bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return paladin_melee_attack_t::ready();
  }
};

// Reckoning ==================================================================

struct reckoning_t: public paladin_melee_attack_t
{
  reckoning_t( paladin_t* p, const std::string& options_str ):
    paladin_melee_attack_t( "reckoning", p, p -> find_class_spell( "Reckoning" ) )
  {
    parse_options( options_str );
    use_off_gcd = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    paladin_melee_attack_t::impact( s );
  }
};

// Shield of the Righteous ==================================================

struct shield_of_the_righteous_t : public paladin_melee_attack_t
{
  shield_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "shield_of_the_righteous", p, p -> find_class_spell( "Shield of the Righteous" ) )
  {
    parse_options( options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equipped in offhand\n", p -> name(), name() );
      background = true;
    }

    // not on GCD, usable off-GCD
    trigger_gcd = timespan_t::zero();
    use_off_gcd = true;

    // no weapon multiplier
    weapon_multiplier = 0.0;
  }

  virtual void execute() override
  {
    paladin_melee_attack_t::execute();

    //Buff granted regardless of combat roll result
    //Duration is additive on refresh, stats not recalculated
    if ( p() -> buffs.shield_of_the_righteous -> check() )
    {
      p() -> buffs.shield_of_the_righteous -> extend_duration( p(), p() -> buffs.shield_of_the_righteous -> buff_duration );
    }
    else
      p() -> buffs.shield_of_the_righteous -> trigger();
  }
};

// Templar's Verdict ========================================================================

// TODO(mserrano): Are any of these multipliers correct?
// TODO(mserrano): supposedly this has a 85.72% attack power base coefficient
struct echoed_templars_verdict_t : public paladin_melee_attack_t
{
  echoed_templars_verdict_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "echoed_verdict", p, p -> find_spell( 186805 ) , false )
  {
    parse_options( options_str );

    background = true;
    base_multiplier *= 1.0 + p -> artifact.might_of_the_templar.percent();
  }

  virtual double cost() const override
  {
    return 0;
  }

  double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();

    if ( p() -> talents.final_verdict -> ok() )
    {
      am *= 1.0 + p() -> talents.final_verdict -> effectN( 1 ).percent();
    }

    return am;
  }
};

struct templars_verdict_t : public holy_power_consumer_t
{
  echoed_templars_verdict_t* echoed_spell;

  templars_verdict_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "templars_verdict", p, p -> find_class_spell( "Templar's Verdict" ), true ),
      echoed_spell( new echoed_templars_verdict_t( p, options_str ) )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + p -> artifact.might_of_the_templar.percent();
  }

  virtual double cost() const override
  {
    double base_cost = holy_power_consumer_t::cost();
    if ( p() -> buffs.conviction -> up() )
      return base_cost - 1;
    return base_cost;
  }

  virtual void execute() override
  {
    // store cost for potential refunding (see below)
    double c = cost();

    holy_power_consumer_t::execute();

    // TODO: do misses consume conviction?
    if ( p() -> buffs.conviction -> up() )
      p() -> buffs.conviction -> expire();

    // missed/dodged/parried TVs do not consume Holy Power
    // check for a miss, and refund the appropriate amount of HP if we spent any
    if ( result_is_miss( execute_state -> result ) && c > 0 )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, c, p() -> gains.hp_templars_verdict_refund );
    }

    if ( p() -> artifact.echo_of_the_highlord.rank() )
    {
      echoed_spell -> schedule_execute();
    }
  }


  double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();

    // Final Verdict buffs TV damage by 25%
    if ( p() -> talents.final_verdict -> ok() )
    {
      am *= 1.0 + p() -> talents.final_verdict -> effectN( 1 ).percent();
    }

    return am;
  }
};

// Seal of Light ========================================================================
struct seal_of_light_t : public paladin_spell_t
{
  seal_of_light_t( paladin_t* p, const std::string& options_str  )
    : paladin_spell_t( "seal_of_light", p, p -> find_talent_spell( "Seal of Light" ) )
  {
    parse_options( options_str );

    may_miss = false;
  }

  virtual double cost() const override
  {
    double base_cost = paladin_spell_t::cost();
    if ( p() -> buffs.conviction -> up() )
      return base_cost - 1;
    return base_cost;
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    if ( ! ( p() -> bugs ) )
      if ( p() -> buffs.conviction -> up() )
        p() -> buffs.conviction -> expire();

    p() -> buffs.seal_of_light -> trigger();
  }
};

// Holy Wrath ========================================================================
struct holy_wrath_t : public paladin_spell_t
{
  holy_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_wrath", p, p -> find_talent_spell( "Holy Wrath" ) )
  {
    parse_options( options_str );
    channeled = true;
    tick_zero = false;
    direct_tick = true;
    hasted_ticks = false;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_spell_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent();
      m *= judgment_multiplier;
    }

    return m;
  }
};

// ==========================================================================
// End Attacks
// ==========================================================================

// ==========================================================================
// Paladin Buffs, Part Deux
// ==========================================================================
// Certain buffs need to be defined  after actions, because they call methods
// found in action_t definitions.

namespace buffs {

struct hand_of_sacrifice_t : public buff_t
{
  paladin_t* source; // Assumption: Only one paladin can cast HoS per target
  double source_health_pool;

  hand_of_sacrifice_t( player_t* p ) :
    buff_t( buff_creator_t( p, "hand_of_sacrifice", p -> find_spell( 6940 ) ) ),
    source( nullptr ),
    source_health_pool( 0.0 )
  {

  }

  // Trigger function for the paladin applying HoS on the target
  bool trigger_hos( paladin_t& source )
  {
    if ( this -> source )
      return false;

    this -> source = &source;
    source_health_pool = source.resources.max[ RESOURCE_HEALTH ];

    return buff_t::trigger( 1 );
  }

  // Misuse functions as the redirect callback for damage onto the source
  virtual bool trigger( int, double value, double, timespan_t ) override
  {
    assert( source );

    value = std::min( source_health_pool, value );
    source -> active.hand_of_sacrifice_redirect -> trigger( value );
    source_health_pool -= value;

    // If the health pool is fully consumed, expire the buff early
    if ( source_health_pool <= 0 )
    {
      expire();
    }

    return true;
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    source = nullptr;
    source_health_pool = 0.0;
  }

  virtual void reset() override
  {
    buff_t::reset();

    source = nullptr;
    source_health_pool = 0.0;
  }
};

// Divine Protection buff
struct divine_protection_t : public buff_t
{

  divine_protection_t( paladin_t* p ) :
    buff_t( buff_creator_t( p, "divine_protection", p -> find_class_spell( "Divine Protection" ) ) )
  {
    cooldown -> duration = timespan_t::zero();
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }

};

} // end namespace buffs
// ==========================================================================
// End Paladin Buffs, Part Deux
// ==========================================================================

// Hand of Sacrifice execute function

void hand_of_sacrifice_t::execute()
{
  paladin_spell_t::execute();

  buffs::hand_of_sacrifice_t* b = debug_cast<buffs::hand_of_sacrifice_t*>( target -> buffs.hand_of_sacrifice );

  b -> trigger_hos( *p() );
}

// ==========================================================================
// Paladin Character Definition
// ==========================================================================

paladin_td_t::paladin_td_t( player_t* target, paladin_t* paladin ) :
  actor_target_data_t( target, paladin )
{
  dots.execution_sentence = target -> get_dot( "execution_sentence", paladin );
  buffs.debuffs_judgment = buff_creator_t( *this, "judgment", paladin -> find_spell( 197277 ));
}

// paladin_t::create_action =================================================

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
  if ( name == "auto_attack"               ) return new auto_melee_attack_t        ( this, options_str );
  if ( name == "ardent_defender"           ) return new ardent_defender_t          ( this, options_str );
  if ( name == "avengers_shield"           ) return new avengers_shield_t          ( this, options_str );
  if ( name == "avenging_wrath"            ) return new avenging_wrath_t           ( this, options_str );
  if ( name == "blessing_of_might"         ) return new blessing_of_might_t        ( this, options_str );
  if ( name == "beacon_of_light"           ) return new beacon_of_light_t          ( this, options_str );
  if ( name == "consecration"              ) return new consecration_t             ( this, options_str );
  if ( name == "crusader_strike"           ) return new crusader_strike_t          ( this, options_str );
  if ( name == "crusader_flurry"           ) return new crusader_flurry_t          ( this, options_str );
  if ( name == "zeal"                      ) return new zeal_t                     ( this, options_str );
  if ( name == "blade_of_justice"          ) return new blade_of_justice_t         ( this, options_str );
  if ( name == "blade_of_wrath"            ) return new blade_of_wrath_t           ( this, options_str );
  if ( name == "divine_hammer"             ) return new divine_hammer_t            ( this, options_str );
  if ( name == "denounce"                  ) return new denounce_t                 ( this, options_str );
  if ( name == "divine_protection"         ) return new divine_protection_t        ( this, options_str );
  if ( name == "divine_shield"             ) return new divine_shield_t            ( this, options_str );
  if ( name == "divine_storm"              ) return new divine_storm_t             ( this, options_str );
  if ( name == "execution_sentence"        ) return new execution_sentence_t       ( this, options_str );
  if ( name == "hand_of_sacrifice"         ) return new hand_of_sacrifice_t        ( this, options_str );
  if ( name == "hammer_of_justice"         ) return new hammer_of_justice_t        ( this, options_str );
  if ( name == "hammer_of_the_righteous"   ) return new hammer_of_the_righteous_t  ( this, options_str );
  if ( name == "holy_shock"                ) return new holy_shock_t               ( this, options_str );
  if ( name == "guardian_of_ancient_kings" ) return new guardian_of_ancient_kings_t( this, options_str );
  if ( name == "judgment"                  ) return new judgment_t                 ( this, options_str );
  if ( name == "light_of_dawn"             ) return new light_of_dawn_t            ( this, options_str );
  if ( name == "lights_hammer"             ) return new lights_hammer_t            ( this, options_str );
  if ( name == "rebuke"                    ) return new rebuke_t                   ( this, options_str );
  if ( name == "reckoning"                 ) return new reckoning_t                ( this, options_str );
  if ( name == "shield_of_the_righteous"   ) return new shield_of_the_righteous_t  ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );
  if ( name == "holy_wrath"                ) return new holy_wrath_t               ( this, options_str );
  if ( name == "holy_prism"                ) return new holy_prism_t               ( this, options_str );
  if ( name == "wake_of_ashes"             ) return new wake_of_ashes_t            ( this, options_str );
  if ( name == "seal_of_light"             ) return new seal_of_light_t            ( this, options_str );
  if ( name == "holy_light"                ) return new holy_light_t               ( this, options_str );
  if ( name == "flash_of_light"            ) return new flash_of_light_t           ( this, options_str );
  if ( name == "lay_on_hands"              ) return new lay_on_hands_t             ( this, options_str );

  return player_t::create_action( name, options_str );
}

void paladin_t::trigger_grand_crusader()
{
  // escape if we don't have Grand Crusader
  if ( ! passives.grand_crusader -> ok() )
    return;

  // attempt to proc the buff, returns true if successful
  if ( buffs.grand_crusader -> trigger() )
  {
    // reset AS cooldown
    cooldowns.avengers_shield -> reset( true );
  }
}

void paladin_t::trigger_holy_shield( action_state_t* s )
{
  // escape if we don't have Holy Shield
  if ( ! talents.holy_shield -> ok() )
    return;

  // sanity check - no friendly-fire
  if ( ! s -> action -> player -> is_enemy() )
    return;

  // Check for proc
  if ( rng().roll( talents.holy_shield -> proc_chance() ) )
  {
    active_holy_shield_proc -> target = s -> action -> player;
    active_holy_shield_proc -> schedule_execute();
  }
}

// paladin_t::init_base =====================================================

void paladin_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_agility = 0.0;
  base.attack_power_per_strength = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Boundless Conviction raises max holy power to 5
  resources.base[ RESOURCE_HOLY_POWER ] = 3 + passives.boundless_conviction -> effectN( 1 ).base_value();

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  // add Sanctuary dodge
  base.dodge += passives.sanctuary -> effectN( 3 ).percent();
  // add Sanctuary expertise
  base.expertise += passives.sanctuary -> effectN( 4 ).percent();

  // Holy Insight grants mana regen from spirit during combat
  base.mana_regen_from_spirit_multiplier = passives.holy_insight -> effectN( 3 ).percent();

  // Holy Insight increases max mana for Holy
  resources.base_multiplier[ RESOURCE_MANA ] = 1.0 + passives.holy_insight -> effectN( 1 ).percent();

  // move holy paladins to range
  if ( specialization() == PALADIN_HOLY && primary_role() == ROLE_HEAL )
    base.distance = 30;
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  last_retribution_trinket_target = nullptr;
  last_extra_regen = timespan_t::zero();
  rppm_defender_of_the_light.reset();
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  // Mana
  gains.extra_regen                 = get_gain( ( specialization() == PALADIN_RETRIBUTION ) ? "sword_of_light" : "guarded_by_the_light" );
  gains.mana_beacon_of_light        = get_gain( "beacon_of_light" );

  // Health
  gains.holy_shield                 = get_gain( "holy_shield_absorb" );

  // Holy Power
  gains.hp_crusader_strike          = get_gain( "crusader_strike" );
  gains.hp_blade_of_justice         = get_gain( "blade_of_justice" );
  gains.hp_wake_of_ashes            = get_gain( "wake_of_ashes" );
  gains.hp_judgment                 = get_gain( "judgment" );
  gains.hp_templars_verdict_refund  = get_gain( "templars_verdict_refund" );

  if ( ! retribution_trinket )
  {
    buffs.retribution_trinket = buff_creator_t( this, "focus_of_vengeance" )
      .chance( 0 );
  }
}

// paladin_t::init_procs ====================================================

void paladin_t::init_procs()
{
  player_t::init_procs();

  procs.eternal_glory             = get_proc( "eternal_glory"                  );
  procs.focus_of_vengeance_reset  = get_proc( "focus_of_vengeance_reset"       );
}

// paladin_t::init_scaling ==================================================

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  specialization_e tree = specialization();

  // Only Holy cares about INT/SPI/SP.
  scales_with[ STAT_INTELLECT   ] = ( tree == PALADIN_HOLY );
  scales_with[ STAT_SPELL_POWER ] = ( tree == PALADIN_HOLY );
  scales_with[ STAT_BONUS_ARMOR    ] = ( tree == PALADIN_PROTECTION );

  scales_with[STAT_AGILITY] = false;
}

// paladin_t::init_buffs ====================================================

void paladin_t::create_buffs()
{
  player_t::create_buffs();

  // Talents
  buffs.holy_shield_absorb     = absorb_buff_creator_t( this, "holy_shield", find_spell( 157122 ) )
                                 .school( SCHOOL_MAGIC )
                                 .source( get_stats( "holy_shield_absorb" ) )
                                 .gain( get_gain( "holy_shield_absorb" ) );

  // General
  buffs.avenging_wrath         = new buffs::avenging_wrath_buff_t( this );
  buffs.divine_protection      = new buffs::divine_protection_t( this );
  buffs.divine_shield          = buff_creator_t( this, "divine_shield", find_class_spell( "Divine Shield" ) )
                                 .cd( timespan_t::zero() ) // Let the ability handle the CD
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Holy
  buffs.infusion_of_light      = buff_creator_t( this, "infusion_of_light", find_spell( 54149 ) );

  // Prot
  buffs.guardian_of_ancient_kings      = buff_creator_t( this, "guardian_of_ancient_kings", find_specialization_spell( "Guardian of Ancient Kings" ) )
                                          .cd( timespan_t::zero() ); // let the ability handle the CD
  buffs.grand_crusader                 = buff_creator_t( this, "grand_crusader" ).spell( passives.grand_crusader -> effectN( 1 ).trigger() ).chance( passives.grand_crusader -> proc_chance() );
  buffs.shield_of_the_righteous        = buff_creator_t( this, "shield_of_the_righteous" ).spell( find_spell( 132403 ) );
  buffs.ardent_defender                = new buffs::ardent_defender_buff_t( this );

  // Ret
  buffs.zeal                           = buff_creator_t( this, "zeal" ).spell( find_spell( 203317 ) )
                                          .add_invalidate( CACHE_HASTE );
  buffs.seal_of_light                  = buff_creator_t( this, "seal_of_light" ).spell( find_spell( 202273 ) )
                                          .add_invalidate( CACHE_HASTE );
  buffs.conviction                     = buff_creator_t( this, "conviction" ).spell( find_spell( 209785 ) );
  buffs.blessing_of_might              = buff_creator_t( this, "blessing_of_might" ).spell( find_spell( 203528 ) );

  // Tier Bonuses
  // T17
  buffs.faith_barricade        = buff_creator_t( this, "faith_barricade", sets.set( PALADIN_PROTECTION, T17, B2 ) -> effectN( 1 ).trigger() )
                                 .default_value( sets.set( PALADIN_PROTECTION, T17, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
                                 .add_invalidate( CACHE_BLOCK );
  buffs.defender_of_the_light  = buff_creator_t( this, "defender_of_the_light", sets.set( PALADIN_PROTECTION, T17, B4 ) -> effectN( 1 ).trigger() )
                                 .default_value( sets.set( PALADIN_PROTECTION, T17, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  buffs.vindicators_fury       = buff_creator_t( this, "vindicators_fury", find_spell( 165903 ) )
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                 .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER )
                                 .chance( sets.has_set_bonus( PALADIN_RETRIBUTION, PVP, B4 ) )
                                 .duration( timespan_t::from_seconds( 4 ) );
  buffs.wings_of_liberty       = buff_creator_t( this, "wings_of_liberty", find_spell( 185655 ) -> effectN( 1 ).trigger() )
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                                 .default_value( find_spell( 185655 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
                                 .chance( sets.has_set_bonus( PALADIN_RETRIBUTION, T18, B4 ) );
  buffs.wings_of_liberty_driver = new buffs::wings_of_liberty_driver_t( this );
}

// paladin_t::has_t18_class_trinket ==============================================

bool paladin_t::has_t18_class_trinket() const
{
  if ( specialization() == PALADIN_RETRIBUTION )
  {
    return retribution_trinket != nullptr;
  }
  return false;
}

// ==========================================================================
// Action Priority List Generation - Protection
// ==========================================================================
void paladin_t::generate_action_prio_list_prot()
{
  ///////////////////////
  // Precombat List
  ///////////////////////

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks )
  {
    if ( true_level > 90 )
    {
      precombat -> add_action( "flask,type=greater_draenic_stamina_flask" );
      precombat -> add_action( "flask,type=greater_draenic_strength_flask,if=role.attack|using_apl.max_dps" );
    }
    else if ( true_level > 85 )
      precombat -> add_action( "flask,type=earth" );
    else if ( true_level >= 80 )
      precombat -> add_action( "flask,type=steelskin" );
  }

  // Food
  if ( sim -> allow_food )
  {
    if ( level() > 90 )
    {
      precombat -> add_action( "food,type=whiptail_fillet" );
      precombat -> add_action( "food,type=pickled_eel,if=role.attack|using_apl.max_dps" );
    }
    else if ( level() > 85 )
      precombat -> add_action( "food,type=chun_tian_spring_rolls" );
    else if ( level() >= 80 )
      precombat -> add_action( "food,type=seafood_magnifique_feast" );
  }

  precombat -> add_action( this, "Seal of Insight" );
  precombat -> add_action( this, "Seal of Righteousness", "if=role.attack|using_apl.max_dps" );
  precombat -> add_talent( this, "Sacred Shield" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting
  std::string potion_type = "";
  if ( sim -> allow_potions )
  {
    // no need for off/def pot options - Draenic Armor gives more AP than Draenic STR,
    // and Mountains potion is pathetic at L90
    if ( true_level > 90 )
      potion_type = "draenic_armor";
    else if ( true_level >= 80 )
      potion_type = "mogu_power";

    if ( potion_type.length() > 0 )
      precombat -> add_action( "potion,name=" + potion_type );
  }

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* dps = get_action_priority_list( "max_dps" );
  action_priority_list_t* surv = get_action_priority_list( "max_survival" );

  dps -> action_list_comment_str = "This is a high-DPS (but low-survivability) configuration.\n# Invoke by adding \"actions+=/run_action_list,name=max_dps\" to the beginning of the default APL.";
  surv -> action_list_comment_str = "This is a high-survivability (but low-DPS) configuration.\n# Invoke by adding \"actions+=/run_action_list,name=max_survival\" to the beginning of the default APL.";

  def -> add_action( "auto_attack" );

  // usable items
  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      def -> add_action ( "use_item,name=" + items[ i ].name_str );

  // profession actions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  // racial actions
  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  // clone all of the above to the other two action lists
  dps -> action_list = def -> action_list;
  surv -> action_list = def -> action_list;

  // TODO: Create this.

}

// ==========================================================================
// Action Priority List Generation - Retribution
// ==========================================================================

void paladin_t::generate_action_prio_list_ret()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( true_level > 90 )
      flask_action += "greater_draenic_strength_flask";
    else if ( true_level > 85 )
      flask_action += "winters_bite";
    else
      flask_action += "titanic_strength";

    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";
    if ( level() > 90 )
      food_action += "sleeper_sushi";
    else
      food_action += ( level() > 85 ) ? "black_pepper_ribs_and_shrimp" : "beer_basted_crocolisk";

    precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Blessing of Might" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting
  if ( sim -> allow_potions && true_level >= 80 )
  {
    if ( true_level > 90 )
      precombat -> add_action( "potion,name=draenic_strength" );
    else
      precombat -> add_action( ( true_level > 85 ) ? "potion,name=mogu_power" : "potion,name=golemblood" );
  }

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* single = get_action_priority_list( "single" );
  action_priority_list_t* cleave = get_action_priority_list( "cleave" );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Rebuke" );

  if ( sim -> allow_potions )
  {
    if ( true_level > 90 )
      def -> add_action( "potion,name=draenic_strength,if=(buff.bloodlust.react|buff.avenging_wrath.up|target.time_to_die<=40)" );
    else if ( true_level > 85 )
      def -> add_action( "potion,name=mogu_power,if=(buff.bloodlust.react|buff.avenging_wrath.up|target.time_to_die<=40)" );
    else if ( true_level >= 80 )
      def -> add_action( "potion,name=golemblood,if=buff.bloodlust.react|buff.avenging_wrath.up|target.time_to_die<=40" );
  }

  // Items
  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      std::string item_str;
      item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up";
      def -> add_action( item_str );
    }
  }

  def -> add_action( this, "Avenging Wrath", "if=!buff.avenging_wrath.up" );

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  def -> add_action( "call_action_list,name=cleave,if=spell_targets.divine_storm>=3" );
  def -> add_action( "call_action_list,name=single" );

  // J >
  single -> add_action( this, "Judgment" );

  // TV5 > TV4 >
  single -> add_action( this, "Templar's Verdict", "if=holy_power>=4" );
  single -> add_action( this, "Templar's Verdict", "if=(holy_power>=2)&(buff.conviction.up)");

  // BoJ/DH >
  single -> add_talent( this, "Divine Hammer" );
  single -> add_action( this, "Blade of Justice" );

  // ES >
  single -> add_talent( this, "Execution Sentence" );
  single -> add_talent( this, "Consecration" );
  single -> add_talent( this, "Turalyon's Might" );

  // CF3 >
  single -> add_talent( this, "Crusader Flurry", "if=charges=3" );
  single -> add_talent( this, "Zeal" );
  single -> add_talent( this, "Crusader Strike" );

  // TV3 >
  single -> add_action( this, "Templar's Verdict", "if=holy_power>=3" );

  // CF2 > CF1
  single -> add_talent( this, "Crusader Flurry" );

  // why would you pick this talent
  single -> add_talent( this, "Blade of Wrath" );

  // TODO: Determine where this goes
  // TODO: how should this interact with holy power?
  single -> add_action( "wake_of_ashes" );

  //Executed if three or more targets are present.
  // TODO: this is a total guess
  cleave -> add_action( this, "Judgment" );
  cleave -> add_action( this, "Divine Storm", "if=holy_power>=4" );
  cleave -> add_action( this, "Divine Storm", "if=(holy_power>=2)&(buff.conviction.up)" );

  cleave -> add_talent( this, "Divine Hammer" );
  cleave -> add_action( this, "Blade of Justice" );

  cleave -> add_talent( this, "Consecration" );
  cleave -> add_talent( this, "Turalyon's Might" );
  cleave -> add_talent( this, "Execution Sentence" );

  cleave -> add_talent( this, "Zeal" );
  cleave -> add_talent( this, "Crusader Flurry", "if=charges=3" );
  cleave -> add_action( this, "Crusader Strike" );

  cleave -> add_action( this, "Divine Storm", "if=holy_power>=3" );

  cleave -> add_talent( this, "Crusader Flurry" );

  // TODO: Determine where this goes
  // TODO: how should this interact with holy power?
  cleave -> add_action( "wake_of_ashes" );

  cleave -> add_talent( this, "Blade of Wrath" );
}

// ==========================================================================
// Action Priority List Generation - Holy
// ==========================================================================

void paladin_t::generate_action_prio_list_holy_dps()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( true_level > 90 )
      flask_action += "greater_draenic_intellect_flask";
    else
      flask_action += ( true_level > 85 ) ? "warm_sun" : "draconic_mind";

    precombat -> add_action( flask_action );
  }

  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";
    if ( level() > 90 )
      food_action += "pickled_eel";
    else
      food_action += ( level() > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Seal of Insight" );
  precombat -> add_action( this, "Beacon of Light" , "target=healing_target");

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );
  precombat -> add_action( "potion,name=draenic_intellect" );

  // action priority list
  action_priority_list_t* def = get_action_priority_list( "default" );

  def -> add_action( "potion,name=draenic_intellect,if=buff.bloodlust.react|target.time_to_die<=40" );
  def -> add_action( "auto_attack" );
  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action ( "/use_item,name=" + items[ i ].name_str );
    }
  }

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  def -> add_action( this, "Avenging Wrath" );
  def -> add_talent( this, "Execution Sentence" );
  def -> add_action( "holy_shock,damage=1" );
  def -> add_action( this, "Denounce" );
}

void paladin_t::generate_action_prio_list_holy()
{
  // currently unsupported

  //precombat first
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( true_level > 90 )
      flask_action += "greater_draenic_intellect_flask";
    else
      flask_action += ( true_level > 85 ) ? "warm_sun" : "draconic_mind";

    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";
    if ( level() > 90 )
      food_action += "pickled_eel";
    else
      food_action += ( level() > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Seal of Insight" );
  precombat -> add_action( this, "Beacon of Light" , "target=healing_target");
  // Beacon probably goes somewhere here?
  // Damn right it does, Theckie-poo.

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // action priority list
  action_priority_list_t* def = get_action_priority_list( "default" );

  def -> add_action( "mana_potion,if=mana.pct<=75" );

  def -> add_action( "auto_attack" );

  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action ( "/use_item,name=" + items[ i ].name_str );
    }
  }

  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  // this is just sort of made up to test things - a real Holy dev should probably come up with something useful here eventually
  // Workin on it. Phillipuh to-do
  def -> add_action( this, "Avenging Wrath" );
  def -> add_action( this, "Lay on Hands","if=incoming_damage_5s>health.max*0.7" );
  def -> add_action( "wait,if=target.health.pct>=75&mana.pct<=10" );
  def -> add_action( this, "Flash of Light", "if=target.health.pct<=30" );
  def -> add_action( this, "Lay on Hands", "if=mana.pct<5" );
  def -> add_action( this, "Holy Light" );

}

// paladin_t::init_actions ==================================================

void paladin_t::init_action_list()
{
  // sanity check - Prot/Ret can't do anything w/o main hand weapon equipped
  if ( main_hand_weapon.type == WEAPON_NONE && ( specialization() == PALADIN_RETRIBUTION || specialization() == PALADIN_PROTECTION ) )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  active_blessing_of_might_proc      = new blessing_of_might_proc_t     ( this );

  // create action priority lists
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    switch ( specialization() )
    {
      case PALADIN_RETRIBUTION:
        generate_action_prio_list_ret(); // RET
        break;
        // for prot, call subroutine
      case PALADIN_PROTECTION:
        generate_action_prio_list_prot(); // PROT
        break;
      case PALADIN_HOLY:
        if ( primary_role() == ROLE_HEAL )
          generate_action_prio_list_holy(); // HOLY
        else
          generate_action_prio_list_holy_dps();
        break;
      default:
        if ( true_level > 80 )
        {
          action_list_str = "flask,type=draconic_mind/food,type=severed_sagefish_head";
          action_list_str += "/potion,name=volcanic_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
        }
        action_list_str += "/snapshot_stats";
        action_list_str += "/auto_attack";
        break;
    }
    use_default_action_list = true;
  }
  else
  {
    // if an apl is provided (i.e. from a simc file), set it as the default so it can be validated
    // precombat APL is automatically stored in the new format elsewhere, no need to fix that
    get_action_priority_list( "default" ) -> action_list_str = action_list_str;
    // clear action_list_str to avoid an assert error in player_t::init_actions()
    action_list_str.clear();
  }

  player_t::init_action_list();
}

void paladin_t::init_spells()
{
  player_t::init_spells();

  // Talents
  talents.holy_bolt                  = find_talent_spell( "Holy Bolt" );
  talents.lights_hammer              = find_talent_spell( "Light's Hammer" );
  talents.crusaders_might            = find_talent_spell( "Crusader's Might" );
  talents.beacon_of_hope             = find_talent_spell( "Beacon of Hope" );
  talents.unbreakable_spirit         = find_talent_spell( "Unbreakable Spirit" );
  talents.shield_of_vengeance        = find_talent_spell( "Shield of Vengeance" );
  talents.devotion_aura              = find_talent_spell( "Devotion Aura" );
  talents.aura_of_light              = find_talent_spell( "Aura of Light" );
  talents.aura_of_mercy              = find_talent_spell( "Aura of Mercy" );
  talents.divine_purpose             = find_talent_spell( "Divine Purpose" );
  talents.sanctified_wrath           = find_talent_spell( "Sanctified Wrath" );
  talents.holy_prism                 = find_talent_spell( "Holy Prism" );
  talents.stoicism                   = find_talent_spell( "Stoicism" );
  talents.daybreak                   = find_talent_spell( "Daybreak" );
  talents.judgment_of_light          = find_talent_spell( "Judgment of Light" );
  talents.beacon_of_faith            = find_talent_spell( "Beacon of Faith" );
  talents.beacon_of_the_lightbringer = find_talent_spell( "Beacon of the Lightbringer" );
  talents.beacon_of_the_savior       = find_talent_spell( "Beacon of the Savior" );

  talents.first_avenger              = find_talent_spell( "First Avenger" );
  talents.day_of_reckoning           = find_talent_spell( "Day of Reckoning" );
  talents.consecrated_hammer         = find_talent_spell( "Consecrated Hammer" );
  talents.holy_shield                = find_talent_spell( "Holy Shield" );
  talents.guardians_light            = find_talent_spell( "Guardian's Light" );
  talents.last_defender              = find_talent_spell( "Last Defender" );
  talents.blessing_of_negation       = find_talent_spell( "Blessing of Negation" );
  talents.blessing_of_salvation      = find_talent_spell( "Blessing of Salvation" );
  talents.retribution_aura           = find_talent_spell( "Retribution Aura" );
  talents.divine_bulwark             = find_talent_spell( "Divine Bulwark" );
  talents.crusaders_judgment         = find_talent_spell( "Crusader's Judgment" );
  talents.blessed_hammer             = find_talent_spell( "Blessed Hammer" );
  talents.righteous_protector        = find_talent_spell( "Righteous Protector" );
  talents.consecrated_ground         = find_talent_spell( "Consecrated Ground" );
  talents.aegis_of_light             = find_talent_spell( "Aegis of Light" );
  talents.knight_templar             = find_talent_spell( "Knight Templar" );
  talents.final_stand                = find_talent_spell( "Final Stand" );

  talents.execution_sentence         = find_talent_spell( "Execution Sentence" );
  talents.consecration               = find_talent_spell( "Consecration" );
  talents.fires_of_justice           = find_talent_spell( "The Fires of Justice" );
  talents.crusader_flurry            = find_talent_spell( "Crusader Flurry" );
  talents.zeal                       = find_talent_spell( "Zeal" );
  talents.virtues_blade              = find_talent_spell( "Virtue's Blade" );
  talents.blade_of_wrath             = find_talent_spell( "Blade of Wrath" );
  talents.divine_hammer              = find_talent_spell( "Divine Hammer" );
  talents.mass_judgment              = find_talent_spell( "Mass Judgment" );
  talents.blaze_of_light             = find_talent_spell( "Blaze of Light" );
  talents.divine_steed               = find_talent_spell( "Divine Steed" );
  talents.eye_for_an_eye             = find_talent_spell( "Eye for an Eye" );
  talents.final_verdict              = find_talent_spell( "Final Verdict" );
  talents.seal_of_light              = find_talent_spell( "Seal of Light" );
  talents.holy_wrath                 = find_talent_spell( "Holy Wrath" );

  artifact.wake_of_ashes           = find_artifact_spell( "Wake of Ashes" );
  artifact.deliver_the_justice     = find_artifact_spell( "Deliver the Justice" );
  artifact.highlords_judgment      = find_artifact_spell( "Highlord's Judgment" );
  artifact.righteous_blade         = find_artifact_spell( "Righteous Blade" );
  artifact.divine_tempest          = find_artifact_spell( "Divine Tempest" );
  artifact.might_of_the_templar    = find_artifact_spell( "Might of the Templar" );
  artifact.sharpened_edge          = find_artifact_spell( "Sharpened Edge" );
  artifact.echo_of_the_highlord    = find_artifact_spell( "Echo of the Highlord" );
  artifact.blades_of_light         = find_artifact_spell( "Blades of Light" );
  artifact.wrath_of_the_ashbringer = find_artifact_spell( "Wrath of the Ashbringer" );
  artifact.endless_resolve         = find_artifact_spell( "Endless Resolve" );
  artifact.deflection              = find_artifact_spell( "Deflection" );

  // Spells
  spells.holy_light                    = find_specialization_spell( "Holy Light" );
  spells.sanctified_wrath              = find_spell( 114232 );  // spec-specific effects for Ret/Holy Sanctified Wrath

  // Masteries
  passives.divine_bulwark         = find_mastery_spell( PALADIN_PROTECTION );
  passives.hand_of_light          = find_mastery_spell( PALADIN_RETRIBUTION );
  passives.lightbringer           = find_mastery_spell( PALADIN_HOLY );

  // Passives

  // Shared Passives
  passives.boundless_conviction   = find_spell( 115675 ); // find_spell fails here
  passives.plate_specialization   = find_specialization_spell( "Plate Specialization" );
  passives.sanctity_of_battle     = find_spell( 25956 );  // find_spell fails here

  // Holy Passives
  passives.holy_insight           = find_specialization_spell( "Holy Insight" );
  passives.infusion_of_light      = find_specialization_spell( "Infusion of Light" );

  // Prot Passives
  passives.bladed_armor           = find_specialization_spell( "Bladed Armor" );
  passives.grand_crusader         = find_specialization_spell( "Grand Crusader" );
  passives.guarded_by_the_light   = find_specialization_spell( "Guarded by the Light" );
  passives.sanctuary              = find_specialization_spell( "Sanctuary" );
  passives.riposte                = find_specialization_spell( "Riposte" );

  // Ret Passives
  passives.sword_of_light         = find_specialization_spell( "Sword of Light" );
  passives.sword_of_light_value   = find_spell( passives.sword_of_light -> ok() ? 20113 : 0 );
  passives.conviction             = find_spell( 185817 );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    extra_regen_period  = passives.sword_of_light -> effectN( 2 ).period();
    extra_regen_percent = passives.sword_of_light -> effectN( 2 ).percent();
  }
  else if ( specialization() == PALADIN_PROTECTION )
  {
    extra_regen_period  = passives.sanctuary -> effectN( 5 ).period();
    extra_regen_percent = passives.sanctuary -> effectN( 5 ).percent();
  }

  if ( find_class_spell( "Beacon of Light" ) -> ok() )
    active_beacon_of_light = new beacon_of_light_heal_t( this );

  if ( talents.holy_shield -> ok() )
    active_holy_shield_proc = new holy_shield_proc_t( this );

  rppm_defender_of_the_light.set_frequency( sets.set( PALADIN_PROTECTION, T17, B4 ) -> real_ppm() );
}

// paladin_t::primary_role ==================================================

role_e paladin_t::primary_role() const
{
  if ( player_t::primary_role() != ROLE_NONE )
    return player_t::primary_role();

  if ( specialization() == PALADIN_RETRIBUTION )
    return ROLE_ATTACK;

  if ( specialization() == PALADIN_PROTECTION  )
    return ROLE_TANK;

  if ( specialization() == PALADIN_HOLY )
    return ROLE_HEAL;

  return ROLE_HYBRID;
}

// paladin_t::convert_hybrid_stat ===========================================

stat_e paladin_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats.
  stat_e converted_stat = s;

  switch ( s )
  {
    case STAT_STR_AGI_INT:
      switch ( specialization() )
      {
        case PALADIN_HOLY:
          return STAT_INTELLECT;
        case PALADIN_RETRIBUTION:
        case PALADIN_PROTECTION:
          return STAT_STRENGTH;
        default:
          return STAT_NONE;
      }
    // Guess at how AGI/INT mail or leather will be handled for plate - probably INT?
    case STAT_AGI_INT:
      converted_stat = STAT_INTELLECT;
      break;
    // This is a guess at how AGI/STR gear will work for Holy, TODO: confirm
    case STAT_STR_AGI:
      converted_stat = STAT_STRENGTH;
      break;
    case STAT_STR_INT:
      if ( specialization() == PALADIN_HOLY )
        converted_stat = STAT_INTELLECT;
      else
        converted_stat = STAT_STRENGTH;
      break;
    default:
      break;
  }

  // Now disable stats that aren't applicable to a given spec.
  switch ( converted_stat )
  {
    case STAT_STRENGTH:
      if ( specialization() == PALADIN_HOLY )
        converted_stat = STAT_NONE;  // STR disabled for Holy
      break;
    case STAT_INTELLECT:
      if ( specialization() != PALADIN_HOLY )
        converted_stat = STAT_NONE; // INT disabled for Ret/Prot
      break;
    case STAT_AGILITY:
      converted_stat = STAT_NONE; // AGI disabled for all paladins
      break;
    case STAT_SPIRIT:
      if ( specialization() != PALADIN_HOLY )
        converted_stat = STAT_NONE;
      break;
    case STAT_BONUS_ARMOR:
      if ( specialization() != PALADIN_PROTECTION )
        converted_stat = STAT_NONE;
      break;
    default:
      break;
  }

  return converted_stat;
}

// paladin_t::composite_attribute_multiplier ================================

double paladin_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  // Guarded by the Light buffs STA
  if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + passives.guarded_by_the_light -> effectN( 2 ).percent();
  }

  return m;
}

// paladin_t::composite_rating_multiplier ==================================

double paladin_t::composite_rating_multiplier( rating_e r ) const
{
  double m = player_t::composite_rating_multiplier( r );

  return m;

};

// paladin_t::composite_melee_crit =========================================

double paladin_t::composite_melee_crit() const
{
  double m = player_t::composite_melee_crit();

  // This should only give a nonzero boost for Holy
  if ( buffs.avenging_wrath -> check() )
    m += buffs.avenging_wrath -> get_crit_bonus();

  return m;
}

// paladin_t::composite_melee_expertise =====================================

double paladin_t::composite_melee_expertise( const weapon_t* w ) const
{
  double expertise = player_t::composite_melee_expertise( w );

  return expertise;
}

// paladin_t::composite_melee_haste =========================================

double paladin_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  // This should only give a nonzero boost for Holy
  if ( buffs.avenging_wrath -> check() )
    h /= 1.0 + buffs.avenging_wrath -> get_haste_bonus();

  // Infusion of Light (Holy) adds 10% haste
  h /= 1.0 + passives.infusion_of_light -> effectN( 2 ).percent();

  if ( buffs.zeal -> up() )
    h /= ( 1.0 + buffs.zeal -> stack() * buffs.zeal -> data().effectN( 4 ).percent() );

  if ( buffs.seal_of_light -> up() )
    h /= ( 1.0 + buffs.seal_of_light -> data().effectN( 1 ).percent() );

  return h;
}

// paladin_t::composite_spell_crit ==========================================

double paladin_t::composite_spell_crit() const
{
  double m = player_t::composite_spell_crit();

  // This should only give a nonzero boost for Holy
  if ( buffs.avenging_wrath -> check() )
    m += buffs.avenging_wrath -> get_crit_bonus();

  return m;
}

// paladin_t::composite_spell_haste ==========================================

double paladin_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  // This should only give a nonzero boost for Holy
  if ( buffs.avenging_wrath -> check() )
    h /= 1.0 + buffs.avenging_wrath -> get_haste_bonus();

  // Infusion of Light (Holy) adds 10% haste
  h /= 1.0 + passives.infusion_of_light -> effectN( 2 ).percent();

  // seal of light adds 25% haste
  if ( buffs.seal_of_light -> up() )
    h /= ( 1.0 + buffs.seal_of_light -> data().effectN( 1 ).percent() );

  // zeal adds 20% haste per stack
  if ( buffs.zeal -> up() )
    h /= ( 1.0 + buffs.zeal -> stack() * buffs.zeal -> data().effectN( 4 ).percent() );

  return h;
}

// paladin_t::composite_mastery =============================================

double paladin_t::composite_mastery() const
{
  double m = player_t::composite_mastery();
  return m;
}

double paladin_t::composite_mastery_rating() const
{
  double m = player_t::composite_mastery_rating();
  return m;
}

// paladin_t::composite_bonus_armor =========================================

double paladin_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  return ba;
}

// paladin_t::composite_player_multiplier ===================================

double paladin_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  // These affect all damage done by the paladin
  // Avenging Wrath buffs everything
  if ( buffs.avenging_wrath -> check() )
    m *= 1.0 + buffs.avenging_wrath -> get_damage_mod();

  m *= 1.0 + buffs.wings_of_liberty -> current_stack * buffs.wings_of_liberty -> current_value;

  if ( retribution_trinket )
    m *= 1.0 + buffs.retribution_trinket -> current_stack * buffs.retribution_trinket -> current_value;

  // WoD Ret PvP 4-piece buffs everything
  if ( buffs.vindicators_fury -> check() )
    m *= 1.0 + buffs.vindicators_fury -> value() * buffs.vindicators_fury -> data().effectN( 1 ).percent();

  return m;
}

// paladin_t::composite_player_heal_multiplier ==============================

double paladin_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  if ( buffs.avenging_wrath -> check() )
    m *= 1.0 + buffs.avenging_wrath -> get_healing_mod();

  // WoD Ret PvP 4-piece buffs everything
  if ( buffs.vindicators_fury -> check() )
    m *= 1.0 + buffs.vindicators_fury -> value() * buffs.vindicators_fury -> data().effectN( 2 ).percent();

  return m;
}

// paladin_t::composite_spell_power =========================================

double paladin_t::composite_spell_power( school_e school ) const
{
  double sp = player_t::composite_spell_power( school );

  // For Protection and Retribution, SP is fixed to AP by passives
  switch ( specialization() )
  {
    case PALADIN_PROTECTION:
      sp = passives.guarded_by_the_light -> effectN( 1 ).percent() * cache.attack_power() * composite_attack_power_multiplier();
      break;
    case PALADIN_RETRIBUTION:
      sp = passives.sword_of_light -> effectN( 1 ).percent() * cache.attack_power() * composite_attack_power_multiplier();
      break;
    default:
      break;
  }
  return sp;
}

// paladin_t::composite_melee_attack_power ==================================

double paladin_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += passives.bladed_armor -> effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// paladin_t::composite_attack_power_multiplier =============================

double paladin_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  // Mastery bonus is multiplicative with other effects
  ap *= 1.0 + cache.mastery() * passives.divine_bulwark -> effectN( 5 ).mastery_value();

  return ap;
}

// paladin_t::composite_spell_power_multiplier ==============================

double paladin_t::composite_spell_power_multiplier() const
{
  if ( passives.sword_of_light -> ok() || passives.guarded_by_the_light -> ok() )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// paladin_t::composite_block ==========================================

double paladin_t::composite_block() const
{
  // this handles base block and and all block subject to diminishing returns
  double block_subject_to_dr = cache.mastery() * passives.divine_bulwark -> effectN( 1 ).mastery_value();
  double b = player_t::composite_block_dr( block_subject_to_dr );

 // Guarded by the Light block not affected by diminishing returns
  b += passives.guarded_by_the_light -> effectN( 6 ).percent();

  // Holy Shield (assuming for now that it's not affected by DR)
  b += talents.holy_shield -> effectN( 1 ).percent();

  // Protection T17 2-piece bonus not affected by DR (confirmed 9/19)
  if ( buffs.faith_barricade -> check() )
    b += buffs.faith_barricade -> value();

  return b;
}

// paladin_t::composite_block_reduction =======================================

double paladin_t::composite_block_reduction() const
{
  double br = player_t::composite_block_reduction();

  if ( buffs.defender_of_the_light -> up() )
    br += buffs.defender_of_the_light -> value();

  return br;
}

// paladin_t::composite_crit_avoidance ========================================

double paladin_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  // Guarded by the Light grants -6% crit chance
  c += passives.guarded_by_the_light -> effectN( 5 ).percent();

  return c;
}

// paladin_t::composite_parry_rating ==========================================

double paladin_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  // add Riposte
  if ( passives.riposte -> ok() )
    p += composite_melee_crit_rating();

  return p;
}

// paladin_t::target_mitigation =============================================

void paladin_t::target_mitigation( school_e school,
                                   dmg_e    dt,
                                   action_state_t* s )
{
  player_t::target_mitigation( school, dt, s );

  // various mitigation effects, Ardent Defender goes last due to absorb/heal mechanics

  // Artifact sources
  if ( artifact.deflection.rank() )
    s -> result_amount *= 1.0 + artifact.deflection.percent();

  // Passive sources (Sanctuary)
  s -> result_amount *= 1.0 + passives.sanctuary -> effectN( 1 ).percent();

  if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
    sim -> out_debug.printf( "Damage to %s after passive mitigation is %f", s -> target -> name(), s -> result_amount );

  // Damage Reduction Cooldowns

  // Guardian of Ancient Kings
  if ( buffs.guardian_of_ancient_kings -> up() && specialization() == PALADIN_PROTECTION )
  {
    s -> result_amount *= 1.0 + buffs.guardian_of_ancient_kings -> data().effectN( 3 ).percent();
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after GAnK is %f", s -> target -> name(), s -> result_amount );
  }

  // Divine Protection
  if ( buffs.divine_protection -> up() )
  {
    if ( util::school_type_component( school, SCHOOL_MAGIC ) )
    {
      s -> result_amount *= 1.0 + buffs.divine_protection -> data().effectN( 1 ).percent();
    }
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Divine Protection is %f", s -> target -> name(), s -> result_amount );
  }

  // Shield of the Righteous
  if ( buffs.shield_of_the_righteous -> check() && school == SCHOOL_PHYSICAL )
  {
    // split his out to make it more readable / easier to debug
    double sotr_mitigation;
    sotr_mitigation = buffs.shield_of_the_righteous -> data().effectN( 1 ).percent() + cache.mastery() * passives.divine_bulwark -> effectN( 4 ).mastery_value();

    // clamp is hardcoded in tooltip, not shown in effects
    sotr_mitigation = std::max( -0.80, sotr_mitigation );
    sotr_mitigation = std::min( -0.20, sotr_mitigation );

    s -> result_amount *= 1.0 + sotr_mitigation;

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after SotR mitigation is %f", s -> target -> name(), s -> result_amount );
  }

  // Ardent Defender
  if ( buffs.ardent_defender -> check() )
  {
    if ( s -> result_amount > 0 && s -> result_amount >= resources.current[ RESOURCE_HEALTH ] )
    {
      // Ardent defender is a little odd - it doesn't heal you *for* 12%, it heals you *to* 12%.
      // It does this by either absorbing all damage and healing you for the difference between 12% and your current health (if current < 12%)
      // or absorbing any damage that would take you below 12% (if current > 12%).
      // To avoid complications with absorb modeling, we're just going to kludge it by adjusting the amount gained or lost accordingly.
      // Also arbitrarily capping at 3x max health because if you're seriously simming bosses that hit for >300% player health I hate you.
      s -> result_amount = 0.0;
      double AD_health_threshold = resources.max[ RESOURCE_HEALTH ] * buffs.ardent_defender -> data().effectN( 2 ).percent();
      if ( resources.current[ RESOURCE_HEALTH ] >= AD_health_threshold )
      {
        resource_loss( RESOURCE_HEALTH,
                       resources.current[ RESOURCE_HEALTH ] - AD_health_threshold,
                       nullptr,
                       s -> action );
      }
      else
      {
        // completely arbitrary heal-capping here at 3x max health, done just so paladin health timelines don't look so insane
        resource_gain( RESOURCE_HEALTH,
                       std::min( AD_health_threshold - resources.current[ RESOURCE_HEALTH ], 3 * resources.max[ RESOURCE_HEALTH ] ),
                       nullptr,
                       s -> action );
      }
      buffs.ardent_defender -> use_oneup();
      buffs.ardent_defender -> expire();
    }

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Ardent Defender is %f", s -> target -> name(), s -> result_amount );
  }
}

// paladin_t::invalidate_cache ==============================================

void paladin_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( ( passives.sword_of_light -> ok() || passives.guarded_by_the_light -> ok() || passives.divine_bulwark -> ok() )
       && ( c == CACHE_STRENGTH || c == CACHE_ATTACK_POWER )
     )
  {
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }

  if ( c == CACHE_ATTACK_CRIT && specialization() == PALADIN_PROTECTION )
    player_t::invalidate_cache( CACHE_PARRY );

  if ( c == CACHE_BONUS_ARMOR && passives.bladed_armor -> ok() )
  {
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }

  if ( c == CACHE_MASTERY && passives.divine_bulwark -> ok() )
  {
    player_t::invalidate_cache( CACHE_BLOCK );
    player_t::invalidate_cache( CACHE_ATTACK_POWER );
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }
}

// paladin_t::matching_gear_multiplier ======================================

double paladin_t::matching_gear_multiplier( attribute_e attr ) const
{
  double mult = 0.01 * passives.plate_specialization -> effectN( 1 ).base_value();

  switch ( specialization() )
  {
    case PALADIN_PROTECTION:
      if ( attr == ATTR_STAMINA )
        return mult;
      break;
    case PALADIN_RETRIBUTION:
      if ( attr == ATTR_STRENGTH )
        return mult;
      break;
    case PALADIN_HOLY:
      if ( attr == ATTR_INTELLECT )
        return mult;
      break;
    default:
      break;
  }
  return 0.0;
}

// paladin_t::regen  ========================================================

void paladin_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  // Guarded by the Light / Sword of Light regen.
  if ( extra_regen_period > timespan_t::from_seconds( 0.0 ) )
  {
    last_extra_regen += periodicity;
    while ( last_extra_regen >= extra_regen_period )
    {
      resource_gain( RESOURCE_MANA, resources.max[ RESOURCE_MANA ] * extra_regen_percent, gains.extra_regen );

      last_extra_regen -= extra_regen_period;
    }
  }
}

// paladin_t::assess_damage =================================================

void paladin_t::assess_damage( school_e school,
                               dmg_e    dtype,
                               action_state_t* s )
{
  if ( buffs.divine_shield -> up() )
  {
    s -> result_amount = 0;

    // Return out, as you don't get to benefit from anything else
    player_t::assess_damage( school, dtype, s );
    return;
  }

  // On a block event, trigger Holy Shield & Defender of the Light
  if ( s -> block_result == BLOCK_RESULT_BLOCKED )
  {
    trigger_holy_shield( s );

    if ( sets.set( PALADIN_PROTECTION, T17, B4 ) -> ok() && rppm_defender_of_the_light.trigger() )
      buffs.defender_of_the_light -> trigger();
  }

  // Also trigger Grand Crusader on an avoidance event (TODO: test if it triggers on misses)
  if ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY || s -> result == RESULT_MISS )
  {
    trigger_grand_crusader();
  }

  player_t::assess_damage( school, dtype, s );
}

// paladin_t::assess_damage_imminent ========================================

void paladin_t::assess_damage_imminent( school_e school, dmg_e, action_state_t* s )
{
  // Holy Shield happens here, after all absorbs are accounted for (see player_t::assess_damage())
  if ( talents.holy_shield -> ok() && s -> result_amount > 0.0 && school != SCHOOL_PHYSICAL )
  {
    // Block code mimics attack_t::block_chance()
    // cache.block() contains our block chance
    double block = cache.block();
    // add or subtract 1.5% per level difference
    block += ( level() - s -> action -> player -> level() ) * 0.015;


    if ( block > 0 )
    {
      // Roll for "block"
      if ( rng().roll( block ) )
      {
        double block_amount = s -> result_amount * composite_block_reduction();

        if ( sim->debug )
          sim -> out_debug.printf( "%s Holy Shield absorbs %f", name(), block_amount );

        // update the relevant counters
        iteration_absorb_taken += block_amount;
        s -> self_absorb_amount += block_amount;
        s -> result_amount -= block_amount;
        s -> result_absorbed = s -> result_amount;

        // hack to register this on the abilities table
        buffs.holy_shield_absorb -> trigger( 1, block_amount );
        buffs.holy_shield_absorb -> consume( block_amount );

        // Trigger the damage event
        trigger_holy_shield( s );
      }
      else
      {
        if ( sim->debug )
          sim -> out_debug.printf( "%s Holy Shield fails to activate", name() );
      }
    }

    if ( sim->debug )
      sim -> out_debug.printf( "Damage to %s after Holy Shield mitigation is %f", name(), s -> result_amount );
  }
}

// paladin_t::assess_heal ===================================================

void paladin_t::assess_heal( school_e school, dmg_e dmg_type, action_state_t* s )
{
  player_t::assess_heal( school, dmg_type, s );
}

// paladin_t::create_options ================================================

void paladin_t::create_options()
{
  player_t::create_options();
}

// paladin_t::combat_begin ==================================================

void paladin_t::combat_begin()
{
  player_t::combat_begin();

  resources.current[ RESOURCE_HOLY_POWER ] = 0;
}

// paladin_t::get_divine_judgment =============================================

double paladin_t::get_divine_judgment() const
{
  if ( specialization() != PALADIN_RETRIBUTION ) return 0.0;

  double handoflight;
  handoflight = cache.mastery_value(); // HoL modifier is in effect #1

  return handoflight;
}

// player_t::create_expression ==============================================

expr_t* paladin_t::create_expression( action_t* a,
                                      const std::string& name_str )
{
  struct paladin_expr_t : public expr_t
  {
    paladin_t& paladin;
    paladin_expr_t( const std::string& n, paladin_t& p ) :
      expr_t( n ), paladin( p ) {}
  };


  std::vector<std::string> splits = util::string_split( name_str, "." );

  struct time_to_hpg_expr_t : public paladin_expr_t
  {
    cooldown_t* cs_cd;
    cooldown_t* boj_cd;
    cooldown_t* j_cd;

    time_to_hpg_expr_t( const std::string& n, paladin_t& p ) :
      paladin_expr_t( n, p ), cs_cd( p.get_cooldown( "crusader_strike" ) ),
      boj_cd ( p.get_cooldown( "blade_of_justice" )),
      j_cd( p.get_cooldown( "judgment" ) )
    { }

    virtual double evaluate() override
    {
      timespan_t gcd_ready = paladin.gcd_ready - paladin.sim -> current_time();
      gcd_ready = std::max( gcd_ready, timespan_t::zero() );

      timespan_t shortest_hpg_time = cs_cd -> remains();

      if ( boj_cd -> remains() < shortest_hpg_time )
        shortest_hpg_time = boj_cd -> remains();

      if ( gcd_ready > shortest_hpg_time )
        return gcd_ready.total_seconds();
      else
        return shortest_hpg_time.total_seconds();
    };
  };

  if ( splits[ 0 ] == "time_to_hpg" )
  {
    return new time_to_hpg_expr_t( name_str, *this );
  }

  return player_t::create_expression( a, name_str );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class paladin_report_t : public player_report_extension_t
{
public:
  paladin_report_t( paladin_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void) p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  paladin_t& p;
};

static void do_trinket_init( paladin_t*                player,
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

static void retribution_trinket( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> retribution_trinket, effect );

  if ( s -> retribution_trinket )
  {
    const spell_data_t* buff_spell = s -> retribution_trinket -> driver() -> effectN( 1 ).trigger();
    s -> buffs.retribution_trinket = buff_creator_t( s, "focus_of_vengeance", buff_spell )
      .default_value( buff_spell -> effectN( 1 ).average( s -> retribution_trinket -> item ) / 100.0 )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
}

// Ashbringer!
static void ashbringer( special_effect_t& effect )
{
  paladin_t* s = debug_cast<paladin_t*>( effect.player );
  do_trinket_init( s, PALADIN_RETRIBUTION, s -> ashbringer, effect );
}

// PALADIN MODULE INTERFACE =================================================

struct paladin_module_t : public module_t
{
  paladin_module_t() : module_t( PALADIN ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new paladin_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new paladin_report_t( *p ) );
    return p;
  }

  virtual bool valid() const override { return true; }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184911, retribution_trinket );
    unique_gear::register_special_effect( 179546, ashbringer );
  }

  virtual void init( player_t* p ) const override
  {
    p -> buffs.beacon_of_light          = buff_creator_t( p, "beacon_of_light", p -> find_spell( 53563 ) );
    p -> buffs.hand_of_sacrifice        = new buffs::hand_of_sacrifice_t( p );
    p -> debuffs.forbearance            = buff_creator_t( p, "forbearance", p -> find_spell( 25771 ) );
  }

  virtual void register_hotfixes() const override
  {
  }

  virtual void combat_begin( sim_t* ) const override {}

  virtual void combat_end  ( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::paladin()
{
  static paladin_module_t m;
  return &m;
}
