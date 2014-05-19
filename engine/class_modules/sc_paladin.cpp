// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO:
  5.4: Remove Unbreakable Spirit code
  Selfless Healer?
  Sacred Shield proxy buff for APL trickery
*/
#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// Forward declarations
struct paladin_t;
struct hand_of_sacrifice_redirect_t;
struct blessing_of_the_guardians_t;
namespace buffs { struct avenging_wrath_buff_t; }

enum seal_e
{
  SEAL_NONE = 0,
  SEAL_OF_JUSTICE,
  SEAL_OF_INSIGHT,
  SEAL_OF_RIGHTEOUSNESS,
  SEAL_OF_TRUTH,
  SEAL_MAX
};

// ==========================================================================
// Paladin Target Data
// ==========================================================================

struct paladin_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* censure;
    dot_t* eternal_flame;
    dot_t* execution_sentence;
    dot_t* stay_of_execution;
    dot_t* holy_radiance;
  } dots;

  struct buffs_t
  {
    buff_t* debuffs_censure;
    buff_t* eternal_flame;
    buff_t* glyph_of_flash_of_light;
    absorb_buff_t* sacred_shield_tick;
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
  seal_e    active_seal;
  heal_t*   active_beacon_of_light;
  action_t* active_censure;  // this is the Censure dot application
  heal_t*   active_enlightened_judgments;
  action_t* active_hand_of_light_proc;
  action_t* active_holy_shield_proc;
  absorb_t* active_illuminated_healing;
  heal_t*   active_protector_of_the_innocent;
  action_t* active_seal_of_insight_proc;
  action_t* active_seal_of_justice_proc;
  action_t* active_seal_of_righteousness_proc;
  action_t* active_seal_of_truth_proc;
  heal_t*   active_shining_protector_proc;
  player_t* last_judgement_target;

  struct active_actions_t
  {
    hand_of_sacrifice_redirect_t* hand_of_sacrifice_redirect;
    blessing_of_the_guardians_t* blessing_of_the_guardians;
  } active;

  // Buffs
  struct buffs_t
  {
    buff_t* alabaster_shield;
    buff_t* ardent_defender;
    buffs::avenging_wrath_buff_t* avenging_wrath;
    buff_t* bastion_of_glory;
    buff_t* bastion_of_power; // t16_4pc_tank
    buff_t* bladed_armor;
    buff_t* blessed_life;
    buff_t* daybreak;
    buff_t* divine_crusader; // t16_4pc_melee
    buff_t* divine_protection;
    buff_t* divine_purpose;
    buff_t* divine_shield;
    buff_t* double_jeopardy;
    buff_t* enhanced_holy_shock;
    buff_t* guardian_of_ancient_kings;
    buff_t* grand_crusader;
    buff_t* glyph_templars_verdict;
    buff_t* glyph_of_word_of_glory;
    buff_t* hand_of_purity;
    buff_t* holy_avenger;
    buff_t* infusion_of_light;
    buff_t* sacred_shield;  // dummy buff for APL simplicity
    buff_t* selfless_healer;
    buff_t* shield_of_glory; // t15_2pc_tank
    buff_t* shield_of_the_righteous;
    buff_t* warrior_of_the_light; // t16_2pc_melee
    buff_t* tier15_2pc_melee;
    buff_t* tier15_4pc_melee;
  } buffs;
  
  // Gains
  struct gains_t
  {
    gain_t* divine_plea;
    gain_t* extra_regen;
    gain_t* seal_of_insight;
    gain_t* glyph_divine_storm;
    gain_t* glyph_divine_shield;

    // Holy Power
    gain_t* hp_blessed_life;
    gain_t* hp_crusader_strike;
    gain_t* hp_exorcism;
    gain_t* hp_grand_crusader;
    gain_t* hp_hammer_of_the_righteous;
    gain_t* hp_hammer_of_wrath;
    gain_t* hp_holy_avenger;
    gain_t* hp_holy_shock;
    gain_t* hp_judgments_of_the_bold;
    gain_t* hp_judgments_of_the_wise;
    gain_t* hp_pursuit_of_justice;
    gain_t* hp_sanctified_wrath;
    gain_t* hp_selfless_healer;
    gain_t* hp_templars_verdict_refund;
    gain_t* hp_tower_of_radiance;
    gain_t* hp_judgment;
    gain_t* hp_t15_4pc_tank;
  } gains;

  // Cooldowns
  struct cooldowns_t
  {
    // these seem to be required to get Art of War and Grand Crusader procs working
    cooldown_t* avengers_shield;
    cooldown_t* exorcism;
  } cooldowns;

  // Passives
  struct passives_t
  {
    const spell_data_t* boundless_conviction;
    const spell_data_t* daybreak;
    const spell_data_t* divine_bulwark;
    const spell_data_t* grand_crusader;
    const spell_data_t* guarded_by_the_light;
    const spell_data_t* hand_of_light;
    const spell_data_t* holy_insight;
    const spell_data_t* illuminated_healing;
    const spell_data_t* infusion_of_light;
    const spell_data_t* judgments_of_the_bold;
    const spell_data_t* judgments_of_the_wise;
    const spell_data_t* plate_specialization;
    const spell_data_t* sanctity_of_battle;
    const spell_data_t* sanctuary;
    const spell_data_t* shining_protector; // TODO: hook up to heals
    const spell_data_t* seal_of_insight;
    const spell_data_t* sword_of_light;
    const spell_data_t* sword_of_light_value;
    const spell_data_t* the_art_of_war;
    const spell_data_t* vengeance;
  } passives;

  // Perks
  struct 
  {
    // Multiple
    const spell_data_t* improved_crusader_strike;     // prot, ret
    const spell_data_t* improved_divine_protection;   // all
    const spell_data_t* improved_flash_of_light;      // holy, ret
    const spell_data_t* improved_forbearance;         // all
    const spell_data_t* improved_judgment;            // prot, ret

    // Holy
    const spell_data_t* empowered_beacon_of_light;
    const spell_data_t* empowered_holy_shock;
    const spell_data_t* enhanced_holy_shock;
    const spell_data_t* improved_daybreak; 
    const spell_data_t* improved_denounce; // Denounce not implemented at all!
    const spell_data_t* improved_holy_light;

    // Protection
    const spell_data_t* empowered_avengers_shield;
    const spell_data_t* improved_block;
    const spell_data_t* improved_consecration;
    const spell_data_t* improved_hammer_of_the_righteous;
    const spell_data_t* improved_word_of_glory;

    // Retribution
    const spell_data_t* empowered_hammer_of_wrath;
    const spell_data_t* empowered_seal_of_truth;
    const spell_data_t* enhanced_hand_of_sacrifice;
    const spell_data_t* improved_exorcism;
  } perk;
  
  // Procs
  struct procs_t
  {
    proc_t* divine_purpose;
    proc_t* divine_crusader;
    proc_t* eternal_glory;
    proc_t* judgments_of_the_bold;
    proc_t* the_art_of_war;
    proc_t* wasted_art_of_war;
  } procs;

  // Spells
  struct spells_t
  {
    const spell_data_t* alabaster_shield;
    const spell_data_t* holy_light;
    const spell_data_t* glyph_of_word_of_glory;
    const spell_data_t* sanctified_wrath; // needed to pull out spec-specific effects
  } spells;

  // Talents
  struct talents_t
  {
    const spell_data_t* hand_of_purity;
    const spell_data_t* unbreakable_spirit;
    const spell_data_t* clemency;
    const spell_data_t* selfless_healer;
    const spell_data_t* eternal_flame;
    const spell_data_t* sacred_shield;
    const spell_data_t* holy_avenger;
    const spell_data_t* sanctified_wrath;
    const spell_data_t* divine_purpose;
    const spell_data_t* holy_prism;
    const spell_data_t* lights_hammer;
    const spell_data_t* execution_sentence;
    const spell_data_t* empowered_seals;
    const spell_data_t* seraphim;
    const spell_data_t* holy_shield;
    const spell_data_t* final_verdict;
    const spell_data_t* beacon_of_faith;
    const spell_data_t* beacon_of_insight;
    const spell_data_t* saved_by_the_light;
  } talents;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* alabaster_shield;
    const spell_data_t* battle_healer;
    const spell_data_t* blessed_life;
    const spell_data_t* devotion_aura;
    const spell_data_t* divine_protection;
    const spell_data_t* divine_shield;
    const spell_data_t* divine_storm;
    const spell_data_t* double_jeopardy;
    const spell_data_t* flash_of_light;
    const spell_data_t* final_wrath;
    const spell_data_t* focused_shield;
    const spell_data_t* focused_wrath;
    const spell_data_t* hand_of_sacrifice;
    const spell_data_t* harsh_words;
    const spell_data_t* immediate_truth;
    const spell_data_t* mass_exorcism;
    const spell_data_t* templars_verdict;
    const spell_data_t* word_of_glory;
  } glyphs;

  player_t* beacon_target;
  int ret_pvp_gloves;

  bool bok_up;
  bool bom_up;
  timespan_t last_extra_regen;
  timespan_t extra_regen_period;
  double extra_regen_percent;

  paladin_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, PALADIN, name, r ),
    last_judgement_target(),
    active( active_actions_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    cooldowns( cooldowns_t() ),
    passives( passives_t() ),
    procs( procs_t() ),
    spells( spells_t() ),
    talents( talents_t() ),
    glyphs( glyphs_t() ),
    beacon_target( nullptr ),
    ret_pvp_gloves( 0 ),
    bok_up( false ),
    bom_up( false ),
    last_extra_regen( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_period( timespan_t::from_seconds( 0.0 ) ),
    extra_regen_percent( 0.0 )
  {
    active_beacon_of_light             = 0;
    active_censure                     = 0;
    active_enlightened_judgments       = 0;
    active_hand_of_light_proc          = 0;
    active_holy_shield_proc            = 0;
    active_illuminated_healing         = 0;
    active_protector_of_the_innocent   = 0;
    active_seal                        = SEAL_NONE;
    active_seal_of_justice_proc        = 0;
    active_seal_of_insight_proc        = 0;
    active_seal_of_righteousness_proc  = 0;
    active_seal_of_truth_proc          = 0;
    active_shining_protector_proc      = 0;
    bok_up                             = false;
    bom_up                             = false;

    cooldowns.avengers_shield = get_cooldown( "avengers_shield" );
    cooldowns.exorcism = get_cooldown( "exorcism" );

    beacon_target = 0;

    base.distance = 3;
  }

  virtual void      init_defense();
  virtual void      init_base_stats();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_spells();
  virtual void      init_action_list();
  virtual void      reset();
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual double    composite_attribute_multiplier( attribute_e attr ) const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_melee_attack_power() const;
  virtual double    composite_melee_crit() const;
  virtual double    composite_melee_expertise( weapon_t* weapon ) const;
  virtual double    composite_melee_haste() const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_spell_haste() const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    composite_player_heal_multiplier( school_e school ) const;
  virtual double    composite_spell_power( school_e school ) const;
  virtual double    composite_spell_power_multiplier() const;
  virtual double    composite_block() const;
  virtual double    composite_crit_avoidance() const;
  virtual double    composite_dodge() const;
  virtual void      assess_damage( school_e, dmg_e, action_state_t* );
  virtual void      assess_heal( school_e, dmg_e, action_state_t* );
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* );
  virtual void      invalidate_cache( cache_e );
  virtual void      create_options();
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual set_e     decode_set( const item_t& ) const;
  virtual resource_e primary_resource() const { return RESOURCE_MANA; }
  virtual role_e    primary_role() const;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual void      regen( timespan_t periodicity );
  virtual void      combat_begin();

  int     holy_power_stacks() const;
  double  get_divine_bulwark() const;
  double  get_hand_of_light();
  double  jotp_haste();
  void    trigger_grand_crusader();
  void    trigger_shining_protector( action_state_t* );
  void    trigger_holy_shield();
  void    generate_action_prio_list_prot();
  void    generate_action_prio_list_ret();
  void    generate_action_prio_list_holy();
  void    validate_action_priority_list();

  target_specific_t<paladin_td_t*> target_data;

  virtual paladin_td_t* get_target_data( player_t* target ) const
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

struct avenging_wrath_buff_t : public buff_t
{
  avenging_wrath_buff_t( player_t* p ) :
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
    }
    else // we're Holy
    {
      damage_modifier = data().effectN( 6 ).percent();
      healing_modifier = data().effectN( 3 ).percent();
      crit_bonus = data().effectN( 2 ).percent();
      haste_bonus = data().effectN( 1 ).percent();

      // not sure which ones we actually need to invalidate, so let's just nuke them all
      add_invalidate( CACHE_CRIT );
      add_invalidate( CACHE_SPELL_CRIT );
      add_invalidate( CACHE_ATTACK_CRIT );
      add_invalidate( CACHE_HASTE );
      add_invalidate( CACHE_ATTACK_HASTE );
      add_invalidate( CACHE_SPELL_HASTE );
    }

    // Lengthen duration if Sanctified Wrath is taken
    const spell_data_t* s = p -> find_talent_spell( "Sanctified Wrath" );
    if ( s -> ok() )
      buff_duration *= 1.0 + s -> effectN( 2 ).percent();

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

// Eternal Flame buff
struct eternal_flame_t : public buff_t
{
  paladin_td_t* pair;
  eternal_flame_t( paladin_td_t* q ) :
    buff_t( buff_creator_t( *q, "eternal_flame", q -> source -> find_spell( 156322 ) ) ),
    pair( q )
  {
    cooldown -> duration = timespan_t::zero();
  }

  virtual void expire_override()
  {
    buff_t::expire_override();

    // cancel existing Eternal Flame HoT
    pair -> dots.eternal_flame -> cancel();    
  }
  
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

  paladin_action_t( const std::string& n, paladin_t* player,
                    const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
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
      // check for divine purpose - if active, return a resource cost of 0
      if ( p() -> buffs.divine_purpose -> check() )
        return 0.0;

      // otherwise return a value equal to our current holy power,
      // lower-bounded by the ability's base cost, upper-bounded by 3
      return std::max( ab::base_costs[ RESOURCE_HOLY_POWER ], std::min( 3.0, p() -> resources.current[ RESOURCE_HOLY_POWER ] ) );
    }

    return ab::cost();
  }

  // hand of light handling
  void trigger_hand_of_light( action_state_t* s )
  {
    if ( p() -> passives.hand_of_light -> ok() )
    {
      p() -> active_hand_of_light_proc -> base_dd_max = p() -> active_hand_of_light_proc-> base_dd_min = s -> result_amount;
      p() -> active_hand_of_light_proc -> execute();
    }
  }

  // Method for consuming "free" holy power effects (Divine Purpose et. al.)
  virtual void consume_free_hp_effects()
  {
    // check for Divine Purpose buff
    if ( p() -> buffs.divine_purpose -> up() )
    {
      // get rid of buff corresponding to the current proc
      p() -> buffs.divine_purpose -> expire();
    }
    // other general effects can go here if added in the future
    // ability-specific effects (e.g. T16 4-pc melee) go in the ability definition, overriding this method
  }

  virtual void trigger_free_hp_effects( double c )
  {
    // now we trigger new buffs.  Need to treat c=0 as c=3 for proc purposes
    double c_effective = ( c == 0.0 ) ? 3 : c;

    // if Divine Purpose is talented, trigger a DP proc
    if ( p() -> talents.divine_purpose -> ok() )
    {
      // chance to proc the buff needs to be scaled by holy power spent
      bool success = p() -> buffs.divine_purpose -> trigger( 1,
        p() -> buffs.divine_purpose -> default_value,
        p() -> buffs.divine_purpose -> default_chance * c_effective / 3 );

      // record success for output
      if ( success )
        p() -> procs.divine_purpose -> occur();
    }

    // If T16 4pc melee set bonus is equipped, trigger a proc
    if ( p() -> sets.has_set_bonus( SET_T16_4PC_MELEE ) )
    {
      // chance to proc the buff needs to be scaled by holy power spent
      bool success = p() -> buffs.divine_crusader -> trigger( 1,
        p() -> buffs.divine_crusader -> default_value,
        p() -> buffs.divine_crusader -> default_chance * c_effective / 3 );

      // record success for output
      if ( success )
        p() -> procs.divine_crusader -> occur();
    }

  }

  virtual void execute()
  {
    double c = ( this -> current_resource() == RESOURCE_HOLY_POWER ) ? this -> cost() : -1.0;

    ab::execute();

    // if the ability uses Holy Power, apply Unbreakable Spirit and handle Divine Purpose and other freebie effects
    if ( c >= 0 )
    {
      // consume divine purpose and other "free hp finisher" buffs (e.g. Divine Purpose)
      if ( c == 0.0 )
        consume_free_hp_effects();

      // trigger new "free hp finisher" buffs (e.g. Divine Purpose)
      trigger_free_hp_effects( c );
    }
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
// To keep consistency throughout the ability damage definitions, I'm keeping to the following conventions.
// The damage formula in action_t::calculate_direct_amount in sc_action.cpp is as follows:
// da_multiplier * ( weapon_multiplier * ( avg_range( base_dd_min, base_dd_max) + base_dd_adder
//                                         + avg_range( weapon->min_dmg, weapon->max_dmg)
//                                         + weapon -> bonus_damage
//                                         + weapon_speed * attack_power / 14 )
//                   + direct_power_mod * ( base_attack_power_multiplier * attack_power
//                                          + base_spell_power_multiplier * spell_power )
//                  )
// The convention we're sticking to is as follows:
// - for spells that scale with ONLY attack_power or spell_power, the scaling coefficient goes
//   in direct_power_mod and base_X_power_multiplier is set to 0 or 1 accordingly
//
// - for spells that scale with both SP and AP differently (Judgment, Avenger's Shield), we set
//   direct_power_mod equal to 1.0 and put the appropriate scaling factors in base_X_power_multiplier
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
  }
};

struct paladin_heal_t : public paladin_spell_base_t<heal_t>
{
  paladin_heal_t( const std::string& n, paladin_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;
    benefits_from_seal_of_insight = true;

    weapon_multiplier = 0.0;
  }

  bool benefits_from_seal_of_insight;

  virtual double action_multiplier() const
  {
    double am = base_t::action_multiplier();

    if ( p() -> active_seal == SEAL_OF_INSIGHT && benefits_from_seal_of_insight )
      am *= 1.0 + p() -> passives.seal_of_insight -> effectN( 2 ).percent();
    
    // Holy Insight - Holy passive
    am *= 1.0 + p() -> passives.holy_insight -> effectN( 6 ).percent();

    return am;
  }

  virtual double action_ta_multiplier() const
  {
    double am = base_t::action_ta_multiplier();

    if ( p() -> active_seal == SEAL_OF_INSIGHT && benefits_from_seal_of_insight )
      am *= 1.0 + p() -> passives.seal_of_insight -> effectN( 2 ).percent();

    return am;
  }
    
  virtual double composite_target_multiplier( player_t* t ) const
  {
    double ctm = base_t::composite_target_multiplier( t );

    if ( td( t ) -> buffs.glyph_of_flash_of_light -> check() )
    {
      ctm *= 1.0 + p() -> glyphs.flash_of_light -> effectN( 1 ).percent();
      if ( sim -> debug )
        sim -> out_debug.printf("=================== %s benefits from GoFoL buff ====================", name() );
    }

    return ctm;

  }
  
  virtual void impact( action_state_t* s )
  {
    base_t::impact( s );

    trigger_illuminated_healing( s );

    if ( s -> target != p() -> beacon_target )
      trigger_beacon_of_light( s );

    // expire Glyph of Flash of Light buff if it exists
    if ( td( s -> target ) -> buffs.glyph_of_flash_of_light -> up() )
    {
      td( s -> target ) -> buffs.glyph_of_flash_of_light -> expire();
      if ( sim -> debug )
        sim -> out_debug.printf("=================== %s consumes GoFoL buff ====================", name() );
    }

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
    amount *= 1.0 + p() -> perk.empowered_beacon_of_light -> effectN( 1 ).percent();

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

  void trigger_illuminated_healing( action_state_t* s )
  {
    if ( s -> result_amount <= 0 )
      return;

    if ( proc )
      return;

    if ( ! p() -> passives.illuminated_healing -> ok() )
      return;

    // FIXME: Each player can have their own bubble, so this should probably be a vector as well
    assert( p() -> active_illuminated_healing );

    // FIXME: This should stack when the buff is present already

    double bubble_value = p() -> cache.mastery_value()
                          * s -> result_amount;

    p() -> active_illuminated_healing -> base_dd_min = p() -> active_illuminated_healing -> base_dd_max = bubble_value;
    p() -> active_illuminated_healing -> execute();
  }
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
    parse_options( nullptr, options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();
    if (  p -> sets.has_set_bonus( SET_T14_2PC_TANK ) )
      cooldown -> duration = data().cooldown() + p -> sets.set( SET_T14_2PC_TANK ) -> effectN( 1 ).time_value();
  }

  virtual void execute()
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
    parse_options( NULL, options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equiped in offhand\n", p -> name(), name() );
      background = true;
    }

    // hits 1 target if FS glyphed, 3 otherwise
    aoe = ( p -> glyphs.focused_shield -> ok() ) ? 1 : 3 + p -> perk.empowered_avengers_shield -> effectN( 1 ).base_value();
    may_crit     = true;
    
    // link needed for trigger_grand_crusader
    cooldown = p -> cooldowns.avengers_shield;
    cooldown -> duration = data().cooldown();
  }

  // Multiplicative damage effects
  virtual double action_multiplier() const
  {
    double am = paladin_spell_t::action_multiplier();

    // Holy Avenger buffs damage if Grand Crusader is active
    if ( p() -> buffs.holy_avenger -> check() && p() -> buffs.grand_crusader -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    // Focused Shield gives another multiplicative factor of 1.3 (tested 5/26/2013, multiplicative with HA)
    if ( p() -> glyphs.focused_shield -> ok() )
    {
      am *= 1.0 + p() -> glyphs.focused_shield -> effectN( 2 ).percent();
    }

    return am;
  }

  // Sanctity of Battle reduces cooldown
  virtual void update_ready( timespan_t cd_duration )
  {
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }

    paladin_spell_t::update_ready( cd_duration );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    // Holy Power gains - only if Grand Crusader is active
    if ( p() -> buffs.grand_crusader -> up() )
    {
      // base gain is 1 Holy Power
      int g = 1;
      // apply gain, attribute to Grand Crusader
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_grand_crusader );

      // Holy Avenger adds another 2 Holy Power
      if ( p() -> buffs.holy_avenger -> check() )
      {
        // apply gain, attribute to Holy Avenger
        p() -> resource_gain( RESOURCE_HOLY_POWER,
                              p() -> buffs.holy_avenger -> value() - g,
                              p() -> gains.hp_holy_avenger );
      }

      // Remove Grand Crsuader buff
      p() -> buffs.grand_crusader -> expire();
    }
  }
};

// Avenging Wrath ===========================================================
// AW is two spells now (31884 for Ret, 31842 for Holy) and the effects are all jumbled. 
// Thus, we need to use some ugly hacks to get it to work seamlessly for both specs within the same spell.
// Most of them can be found in buffs::avenging_wrath_buff_t, this spell just triggers the buff

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "avenging_wrath", p, p -> specialization() == PALADIN_RETRIBUTION ? p -> find_spell( 31884 ) : p -> find_spell( 31842 ) )
  {
    parse_options( NULL, options_str );

    cooldown -> duration += p -> passives.sword_of_light -> effectN( 7 ).time_value();

    // disable for protection
    if ( p -> specialization() == PALADIN_PROTECTION )
      background = true;

    harmful = false;
    use_off_gcd = true;    
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.avenging_wrath -> trigger();
  }
};

// Beacon of Light ==========================================================

struct beacon_of_light_t : public paladin_heal_t
{
  beacon_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "beacon_of_light", p, p -> find_class_spell( "Beacon of Light" ) )
  {
    parse_options( NULL, options_str );

    // Target is required for Beacon
    if ( target_str.empty() )
    {
      sim -> errorf( "Warning %s's \"%s\" needs a target", p -> name(), name() );
      sim -> cancel();
    }

    // Remove the 'dot'
    num_ticks = 0;
  }

  virtual void execute()
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

// Blessing of Kings ========================================================

struct blessing_of_kings_t : public paladin_spell_t
{
  blessing_of_kings_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blessing_of_kings", p, p -> find_class_spell( "Blessing of Kings" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    background = ( sim -> overrides.str_agi_int != 0 );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    if ( ! sim -> overrides.str_agi_int )
    {
      sim -> auras.str_agi_int -> trigger();
      p() -> bok_up = true;
    }

    if ( ! sim -> overrides.mastery && p() -> bom_up )
    {
      sim -> auras.mastery -> decrement();
      p() -> bom_up = false;
    }
  }
};

// Blessing of Might ========================================================

struct blessing_of_might_t : public paladin_spell_t
{
  blessing_of_might_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "blessing_of_might", p, p -> find_class_spell( "Blessing of Might" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    background = ( sim -> overrides.mastery != 0 );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    if ( ! sim -> overrides.mastery )
    {
      double mastery_rating = data().effectN( 1 ).average( player );
      if ( ! sim -> auras.mastery -> check() || sim -> auras.mastery -> current_value < mastery_rating )
        sim -> auras.mastery -> trigger( 1, mastery_rating );
      p() -> bom_up = true;
    }

    if ( ! sim -> overrides.str_agi_int && p() -> bok_up )
    {
      sim -> auras.str_agi_int -> decrement();
      p() -> bok_up = false;
    }
  }
};

// Blessing of the Guardians =====================================
// Blessing of the Guardians is the t16_2pc_tank set bonus

struct blessing_of_the_guardians_t : public paladin_heal_t
{
  double accumulated_damage;
  double healing_multiplier;

  blessing_of_the_guardians_t( paladin_t* p ) :
    paladin_heal_t( "blessing_of_the_guardians", p, p -> find_spell( 144581 ) )
  {
    background = true;
    
    // tested 8/7/2013 by Theck. Cannot crit, not affected by SoI, ticks not hasted
    // It _is_ affected by Sanctified Wrath, but that's handled in assess_heal()
    hasted_ticks = false;
    may_crit = tick_may_crit = false;
    benefits_from_seal_of_insight = false;
    
    // spell info not yet returning proper details, should heal for X every 1 sec for 10 sec.
    base_tick_time = timespan_t::from_seconds( 1 );
    num_ticks = 10;

    // initialize accumulator
    accumulated_damage = 0.0;

    // store healing multiplier
    healing_multiplier = p -> find_spell( 144580 ) -> effectN( 1 ).percent();
    
    // unbreakable spirit seems to halve the healing done
    if ( p -> talents.unbreakable_spirit -> ok() )
      healing_multiplier /= 2; 
  }

  virtual void increment_damage( double amount )
  {
    accumulated_damage += amount;
  }

  virtual void execute()
  {
    base_td = healing_multiplier * accumulated_damage / num_ticks;

    paladin_heal_t::execute();

    accumulated_damage = 0;
  }

};

//Censure  ==================================================================

// Censure is the debuff applied by Seal of Truth, and it's an oddball.  The dot ticks are really melee attacks, believe it or not.
// They cannot miss/dodge/parry (though the application of the debuff can do all of those things, and often shows up in WoL reports
// as such) and use melee crit.  However, the tick interval also scales with spell haste like a normal DoT.  What a mess.
//
// The easiest solution here is to treat it like a spell, since that way it gets all of the spell-like properties it ought to.  However,
// we need to tweak it to use melee crit, which we do in impact() by setting the action_state_t -> crit variable accordingly.
//
// The way this works is that censure_t represents a spell attack proc which is made every time we succeed with a qualifying melee attack.
// That spell attack proc does 0 damage and applies the censure dot, which is defined in the dots_t structure.  The dot takes care of the
// tick damage, but since we don't support stackable dots, we have to handle the stacking in the player -> debuff_censure variable and use
// that value as an action multiplier to scale the damage.  Got all that?

struct censure_t : public paladin_spell_t
{
  censure_t( paladin_t* p ) :
    paladin_spell_t( "censure", p, p -> find_spell( p -> find_class_spell( "Seal of Truth" ) -> ok() ? 31803 : 0 ) )
  {
    background       = true;
    proc             = true;
    tick_may_crit    = true;

    //Undocumented: 80% weaker for protection
    if ( p -> specialization() == PALADIN_PROTECTION )
    {
      spell_power_mod.tick /= 5;
    }

    // Glyph of Immediate Truth reduces DoT damage
    if ( p -> glyphs.immediate_truth -> ok() )
    {
      base_multiplier *= 1.0 + p -> glyphs.immediate_truth -> effectN( 2 ).percent();
    }
  }

  virtual void init()
  {
    paladin_spell_t::init();
    // snapshot haste when we apply the debuff, this modifies the tick interval dynamically on every refresh
    snapshot_flags |= STATE_HASTE;
  }

  virtual void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) )
    {
      // if the application hits, apply/refresh the censure debuff; this will also increment stacks
      td( s -> target ) -> buffs.debuffs_censure -> trigger();
      // cheesy hack to make Censure use melee crit rather than spell crit
      s -> crit = p() -> composite_melee_crit();
    }

    paladin_spell_t::impact( s );
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    // since we don't support stacking debuffs, we handle the stack size in paladin_td buffs.debuffs_censure
    // and apply the stack size as an action multiplier
    double am = paladin_spell_t::composite_target_multiplier( t );

    am *= td( t ) -> buffs.debuffs_censure -> check();

    // Empowered Seal of Truth perk (TODO: See if this affects Censure)
    am *= 1.0 + p() -> perk.empowered_seal_of_truth-> effectN( 1 ).percent();

    return am;
  }

  virtual void last_tick( dot_t* d )
  {
    // if this is the last tick, expire the debuff locally
    // (this shouldn't happen ... ever? ... under normal operation)
    td( d -> state -> target ) -> buffs.debuffs_censure -> expire();

    paladin_spell_t::last_tick( d );
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
  }
};

struct consecration_t : public paladin_spell_t
{
  consecration_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "consecration", p, p -> find_class_spell( "Consecration" ) )
  {
    parse_options( NULL, options_str );

    hasted_ticks   = false;
    may_miss       = false;

    tick_action = new consecration_tick_t( p );
  }

  virtual void tick( dot_t* d )
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

  virtual double action_ta_multiplier() const
  {
    double am = paladin_spell_t::action_ta_multiplier();

    am *= 1.0 + p() -> perk.improved_consecration -> effectN( 1 ).percent();

    return am;
  }
};

// Daybreak =================================================================
//This is the aoe healing proc triggered by Holy Shock

struct daybreak_t : public paladin_heal_t
{
  daybreak_t( paladin_t* p )
    : paladin_heal_t( "daybreak", p, p->find_spell( 121129 ) )
  {
    background = true;
    aoe = -1;
    split_aoe_damage = true;
    may_crit = false;
    may_multistrike = false; // guess, TODO: Test
  }

  virtual void init()
  {
    paladin_heal_t::init();
    // add snapshot flags to allow for action_multiplier() to be called
    snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;
  }


  // Daybreak ignores the target of the holy_shock_heal_t that procs it. 
  // This is the standard heal_t::available_targets() with tl.push_back( target ) removed
  // (since we don't want to include the target) and an unnecessary group_only check removed
  virtual size_t available_targets( std::vector< player_t* >& tl ) const
  {
    tl.clear();

    for ( size_t i = 0, actors = sim -> player_non_sleeping_list.size(); i < actors; i++ )
    {
      player_t* t = sim -> player_non_sleeping_list[ i ];

      if ( t != target )
        tl.push_back( t );
    }

    return tl.size();
  }

  virtual double action_multiplier() const
  {
    double am = p() -> passives.daybreak -> effectN( 1 ).percent();

    am *= 1.0 + p() -> perk.improved_daybreak -> effectN( 1 ).percent();

    return am;
  }
  
};

// Devotion Aura Spell ======================================================

struct devotion_aura_t : public paladin_spell_t
{
  devotion_aura_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "devotion_aura", p, p -> find_specialization_spell( "Devotion Aura" ) )
  {
    parse_options( NULL, options_str );

    if ( p -> glyphs.devotion_aura -> ok() )
      cooldown -> duration += timespan_t::from_millis( p -> glyphs.devotion_aura -> effectN( 1 ).base_value() );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    if ( p() -> glyphs.devotion_aura -> ok() )
      player -> buffs.devotion_aura -> trigger();
    else
    {
      for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
      {
        player_t* p = sim -> player_non_sleeping_list[ i ];
        if ( p -> is_pet() || p -> is_enemy() )
          continue;

        p -> buffs.devotion_aura -> trigger(); // Technically these stack; we're abstracting somewhat by assuming they don't
      }
    }
  }
};

// Divine Plea ==============================================================

struct divine_plea_t : public paladin_spell_t
{
  divine_plea_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "divine_plea", p, p -> find_class_spell( "Divine Plea" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void impact( action_state_t* s )
  {
    paladin_spell_t::impact( s );

    // Mana gain
    p() -> resource_gain( RESOURCE_MANA, 
                          data().effectN( 2 ).base_value() / 10000.0 * p() -> resources.max[ RESOURCE_MANA ], 
                          p() -> gains.divine_plea );
  }

  // TODO: may need to override cost if this cannot consume divine purpose!

  
};

// Divine Protection ========================================================

struct divine_protection_t : public paladin_spell_t
{
  divine_protection_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_protection", p, p -> find_class_spell( "Divine Protection" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );

    if ( p -> sets.has_set_bonus( SET_T16_2PC_TANK ) )
      p -> active.blessing_of_the_guardians = new blessing_of_the_guardians_t( p );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();
    
    p() -> buffs.divine_protection -> trigger();
  }
};

// Divine Shield ============================================================

struct divine_shield_t : public paladin_spell_t
{
  divine_shield_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_shield", p, p -> find_class_spell( "Divine Shield" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    
    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );

  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    // Technically this should also drop you from the mob's threat table,
    // but we'll assume the MT isn't using it for now
    p() -> buffs.divine_shield -> trigger();

    // in this sim, the only debuffs we care about are enemy DoTs.
    // Check for them and remove them when cast, and apply Glyph of Divine Shield appropriately
    for ( size_t i = 0, size = p() -> dot_list.size(); i < size; i++ )
    {
      dot_t* d = p() -> dot_list[ i ];
      int num_destroyed = 0;

      if ( d -> source != p() )
      {
        d -> cancel();
        num_destroyed++;
      }

      // glyph of divine shield heals
      if ( p() -> glyphs.divine_shield -> ok() )
      {
        double amount = std::min( num_destroyed * p() -> glyphs.divine_shield -> effectN( 1 ).percent(),
                                  p() -> glyphs.divine_shield -> effectN( 1 ).percent() * p() -> glyphs.divine_shield -> effectN( 2 ).base_value() );

        amount *= p() -> resources.max[ RESOURCE_HEALTH ];

        p() -> resource_gain( RESOURCE_HEALTH, amount, p() -> gains.glyph_divine_shield, this );
      }
    }
    // trigger forbearance
    p() -> debuffs.forbearance -> trigger();
  }

  virtual bool ready()
  {
    if ( player -> debuffs.forbearance -> check() )
      return false;

    return paladin_spell_t::ready();
  }
};

// Eternal Flame  ===========================================================

// HoT portion
struct eternal_flame_hot_t : public paladin_heal_t
{
  double hopo;

  eternal_flame_hot_t( paladin_t* p ) :
    paladin_heal_t( "eternal_flame_tick", p, p -> find_spell( 156322 ) )
  {
    background = true;
    hopo = 0;
  }
  
  virtual void impact( action_state_t* s )
  {
    paladin_heal_t::impact( s );

    td( s -> target ) -> buffs.eternal_flame -> trigger( 1, buff_t::DEFAULT_VALUE(), -1, s -> action -> hasted_num_ticks( s -> haste ) * s -> action -> tick_time( s -> haste ) + timespan_t::from_millis( 1 ) );
  }
  
  virtual void last_tick( dot_t* d )
  {
    td( d -> state -> target ) -> buffs.eternal_flame -> expire();

    paladin_heal_t::last_tick( d );
  }

  virtual double action_ta_multiplier() const
  {
    // this scales just the ticks
    double am = paladin_heal_t::action_ta_multiplier();

    // Scale based on HP used
    am *= hopo;

    if ( target == player )
    {
      // HoT is more effective when used on self
      am *= 1.0 + p() -> talents.eternal_flame -> effectN( 2 ).percent();
    }

    // Holy Insight buffs all healing by 25% & WoG/EF/LoD by 50%.
    // The 25% buff is already in paladin_heal_t, so we need to divide by that first & then apply 50%
    am /= 1.0 + p() -> passives.holy_insight -> effectN( 6 ).percent();
    am *= 1.0 + p() -> passives.holy_insight -> effectN( 9 ).percent();

    return am;
  }

};

// Direct Heal
struct eternal_flame_t : public paladin_heal_t
{
  eternal_flame_hot_t* hot;

  eternal_flame_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "eternal_flame", p, p -> find_talent_spell( "Eternal Flame" ) ),
    hot( new eternal_flame_hot_t( p ) )
  {
    parse_options( NULL, options_str );

    // remove GCD constraints & cast time for prot
    if ( p -> passives.guarded_by_the_light -> ok() )
    {
      trigger_gcd = timespan_t::zero();
      use_off_gcd = true;
      base_execute_time *= 1 + p -> passives.guarded_by_the_light -> effectN( 10 ).percent();
    }
    // remove cast time for ret
    if ( p -> passives.sword_of_light -> ok() )
    {
      base_execute_time *= 1 + p -> passives.sword_of_light -> effectN( 9 ).percent();
    }

    // attach HoT as child object - doesn't seem to work?
    add_child( hot );
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }

    paladin_heal_t::update_ready( cd_duration );
  }

  virtual double cost() const
  {
    // check for T16 4-pc tank effect
    if ( p() -> buffs.bastion_of_power -> check() && target == player )
      return 0.0;

    return paladin_heal_t::cost();
  }

  virtual void consume_free_hp_effects()
  {
    // order of operations: T16 4-pc tank, then Divine Purpose (assumed!)

    // check for T16 4pc tank bonus
    if ( p() -> buffs.bastion_of_power -> check() && target == player )
    {
      // nothing necessary here, BoG & BoPower will be consumed automatically upon cast
      return;
    }

    paladin_heal_t::consume_free_hp_effects();
  }

  virtual double action_multiplier() const
  {
    // this scales both the base heal and the ticks
    double am = paladin_heal_t::action_multiplier();
    double c = cost();

    // scale the am by holy power spent, can't be more than 3 and Divine Purpose counts as 3
    am *= ( ( p() -> holy_power_stacks() <= 3  && c > 0.0 ) ? p() -> holy_power_stacks() : 3 );

    if ( target == player )
    {      
      // grant 10% extra healing per stack of BoG; can't expire() BoG here because it's needed in execute()
      am *= ( 1.0 + p() -> buffs.bastion_of_glory -> stack() * ( p() -> buffs.bastion_of_glory -> data().effectN( 1 ).percent() + p() -> get_divine_bulwark() ) );
    }

    // Improved Word of Glory perk
    am *= 1.0 + p() -> perk.improved_word_of_glory -> effectN( 1 ).percent();
    
    // Holy Insight buffs all healing by 25% & WoG/EF/LoD by 50%.
    // The 25% buff is already in paladin_heal_t, so we need to divide by that first & then apply 50%
    am /= 1.0 + p() -> passives.holy_insight -> effectN( 6 ).percent();
    am *= 1.0 + p() -> passives.holy_insight -> effectN( 9 ).percent();

    return am;
  }

  virtual void execute()
  {
    double hopo = ( ( p() -> holy_power_stacks() <= 3  && cost() > 0.0 ) ? p() -> holy_power_stacks() : 3 );

    paladin_heal_t::execute();

    // trigger HoT
    hot -> target = target;
    hot -> hopo = hopo;
    hot -> schedule_execute();

    // Glyph of Word of Glory handling
    if ( p() -> spells.glyph_of_word_of_glory -> ok() )
    {
      p() -> buffs.glyph_of_word_of_glory -> trigger( 1, p() -> spells.glyph_of_word_of_glory -> effectN( 1 ).percent() * hopo );
    }

    // Shield of Glory (Tier 15 protection 2-piece bonus)
    if ( p() -> sets.has_set_bonus( SET_T15_2PC_TANK ) )
      p() -> buffs.shield_of_glory -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, p() -> buffs.shield_of_glory -> buff_duration * hopo );
    
    // consume BoG stacks and Bastion of Power if used on self
    if ( target == player )
    {
      p() -> buffs.bastion_of_glory -> expire();
      p() -> buffs.bastion_of_power -> expire();
    }
  }
};

// Execution Sentence =======================================================

// this is really two components: Execution Sentence is the damage portion, Stay of Execution the heal
// it operates based on smart targeting - if the target is an enemy, Execution Sentence does its thing
// if the target is friendly, Execution Sentence calls Stay of Execution
struct stay_of_execution_t : public paladin_heal_t
{
  std::array<double, 11> soe_tick_multiplier;

  stay_of_execution_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "stay_of_execution", p, p -> find_talent_spell( "Execution Sentence" ) ),
      soe_tick_multiplier()
  {
    parse_options( NULL, options_str );
    hasted_ticks   = false;
    travel_speed   = 0;
    tick_may_crit  = 1;
    benefits_from_seal_of_insight = false;
    background = true;

    // Where the 0.0374151195 comes from
    // The whole dots scales with data().effectN( 2 ).base_value()/1000 * SP
    // Tick 1-9 grow exponentionally by 10% each time, 10th deals 5x the
    // damage of the 9th.
    // 1.1 + 1.1^2 + ... + 1.1^9 + 1.1^9 * 5 = 26.727163056
    // 1 / 26,727163056 = 0.0374151195, which is the factor to get from the
    // whole spells SP scaling to the base scaling of the 0th tick
    // 1st tick is already 0th * 1.1!
    if ( data().ok() )
    {
      parse_effect_data( ( p -> find_spell( 114916 ) -> effectN( 1 ) ) );
      spell_power_mod.tick = p -> find_spell( 114916 ) -> effectN( 2 ).base_value() / 1000.0 * 0.0374151195;
    }

    assert( as<unsigned>( num_ticks ) < sizeof_array( soe_tick_multiplier ) );
    // this is reversed from Execution Sentence's tick order
    soe_tick_multiplier[ 10 ] = 1.0;
    for ( int i = 9; i > 0 ; --i )
      soe_tick_multiplier[ i ] = soe_tick_multiplier[ i + 1 ] * 1.1;
    soe_tick_multiplier[ 0 ] = soe_tick_multiplier[ 1 ] * 5;

  }

  double composite_target_multiplier( player_t* target ) const
  {
    double m = paladin_heal_t::composite_target_multiplier( target );

    m *= soe_tick_multiplier[ td( target ) -> dots.stay_of_execution -> current_tick ];

    return m;
  }
};

struct execution_sentence_t : public paladin_spell_t
{
  std::array<double, 11> tick_multiplier;
  stay_of_execution_t* stay_of_execution;

  execution_sentence_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "execution_sentence", p, p -> find_talent_spell( "Execution Sentence" ) ),
      tick_multiplier()
  {
    parse_options( NULL, options_str );
    hasted_ticks   = false;
    travel_speed   = 0;
    tick_may_crit  = 1;

    // Where the 0.0374151195 comes from
    // The whole dots scales with data().effectN( 2 ).base_value()/1000 * SP
    // Tick 1-9 grow exponentionally by 10% each time, 10th deals 5x the
    // damage of the 9th.
    // 1.1 + 1.1^2 + ... + 1.1^9 + 1.1^9 * 5 = 26.727163056
    // 1 / 26,727163056 = 0.0374151195, which is the factor to get from the
    // whole spells SP scaling to the base scaling of the 0th tick
    // 1st tick is already 0th * 1.1!
    if ( data().ok() )
    {
      parse_effect_data( ( p -> find_spell( 114916 ) -> effectN( 1 ) ) );
      spell_power_mod.tick = p -> find_spell( 114916 ) -> effectN( 2 ).base_value() / 1000.0 * 0.0374151195;
    }

    assert( as<unsigned>( num_ticks ) < sizeof_array( tick_multiplier ) );
    tick_multiplier[ 0 ] = 1.0;
    for ( int i = 1; i < num_ticks; ++i )
      tick_multiplier[ i ] = tick_multiplier[ i - 1 ] * 1.1;
    tick_multiplier[ 10 ] = tick_multiplier[ 9 ] * 5;

    stay_of_execution = new stay_of_execution_t( p, options_str );
    add_child( stay_of_execution );

    // disable if not talented
    if ( ! ( p -> talents.execution_sentence -> ok() ) )
      background = true;
  }

  double composite_target_multiplier( player_t* target ) const
  {
    double m = paladin_spell_t::composite_target_multiplier( target );

    m *= tick_multiplier[ td( target ) -> dots.execution_sentence -> current_tick ];

    return m;
  }

  virtual void execute()
  {
    if ( target -> is_enemy() )
    {
      paladin_spell_t::execute();
    }
    else
    {
      stay_of_execution -> schedule_execute();
    }
  }
};

// Exorcism =================================================================

struct exorcism_t : public paladin_spell_t
{
  exorcism_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "exorcism", p, p -> find_class_spell( "Exorcism" ) )
  {
    parse_options( NULL, options_str );

    may_crit                     = true;

    if ( p -> glyphs.mass_exorcism -> ok() )
    {
      aoe = -1;
      base_aoe_multiplier = 0.25;
    }

    cooldown = p -> cooldowns.exorcism;
    cooldown -> duration = data().cooldown();
  }

  virtual double action_multiplier() const
  {
    double am = paladin_spell_t::action_multiplier();

    // Improved Exorcism perk
    am *= 1.0 + p() -> perk.improved_exorcism -> effectN( 1 ).percent(); 

    // Holy Avenger
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }
    paladin_spell_t::update_ready( cd_duration );
  }

  virtual void execute()
  {
    paladin_spell_t::execute();
    if ( result_is_hit( execute_state -> result ) )
    {
      int g = 1;
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_exorcism );
      if ( p() -> buffs.holy_avenger -> check() )
      {
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }
    }
  }

  virtual void impact( action_state_t* s )
  {
    if ( result_is_hit( s -> result ) && p() -> sets.has_set_bonus( SET_T15_2PC_MELEE ) )
    {
      p() -> buffs.tier15_2pc_melee -> trigger();
    }

    paladin_spell_t::impact( s );
  }
};

// Flash of Light Spell =====================================================

struct flash_of_light_t : public paladin_heal_t
{
  flash_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "flash_of_light", p, p -> find_class_spell( "Flash of Light" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual double cost() const
  {
    // selfless healer reduces mana cost by 35% per stack
    double cost_multiplier = std::max( 1.0 + p() -> buffs.selfless_healer -> stack() * p() -> buffs.selfless_healer -> data().effectN( 3 ).percent(), 0.0 );

    return ( paladin_heal_t::cost() * cost_multiplier );
  }

  virtual timespan_t execute_time() const
  {
    // Selfless Healer reduces cast time by 35% per stack
    double cast_multiplier = std::max( 1.0 + p() -> buffs.selfless_healer -> stack() * p() -> buffs.selfless_healer -> data().effectN( 3 ).percent(), 0.0 );

    return ( paladin_heal_t::execute_time() * cast_multiplier );
  }

  virtual double action_multiplier() const
  {
    double am = paladin_heal_t::action_multiplier();

    // Improved Flash of Light perk
    am *= 1.0 + p() -> perk.improved_flash_of_light -> effectN( 1 ).percent();

    // Selfless healer has two effects
    if ( p() -> talents.selfless_healer -> ok() )
    {
      // multiplicative 20% per Selfless Healer stack when FoL is used on others
      if ( target != player )
      {
        am *= 1.0 + p() -> buffs.selfless_healer -> data().effectN( 2 ).percent() * p() -> buffs.selfless_healer -> stack();
      }
      // multiplicative 20% per stack of Bastion of Glory when used on self, only if 1+ stack of SH
      else if ( p() -> buffs.bastion_of_glory -> stack() > 0  && p() -> buffs.selfless_healer -> stack() > 0 )
      {
        am *= 1.0 + p() -> buffs.selfless_healer -> data().effectN( 2 ).percent() * p() -> buffs.bastion_of_glory -> stack();
      }
    }

    return am;
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    // if Selfless Healer is talented, expire SH buff 
    if ( p() -> talents.selfless_healer -> ok() )
    {
      // also expire BoG buff if self-cast
      if ( target == player && p() -> buffs.selfless_healer -> stack() > 0 )
        p() -> buffs.bastion_of_glory -> expire();

      p() -> buffs.selfless_healer -> expire();
    }

    // Enhanced Holy Shock trigger
    p() -> buffs.enhanced_holy_shock -> trigger();

  }

  virtual void impact( action_state_t* s )
  {
    paladin_heal_t::impact( s );

    // apply glyph_of_flash_of_light buff if applicable
    if ( p() -> glyphs.flash_of_light -> ok() )
    {
      td( s -> target ) -> buffs.glyph_of_flash_of_light -> trigger();
      if ( sim -> debug )
        sim -> out_debug.printf("=================== Flash of Light applies GoFoL buff ====================");      
    }
  }
};

// Guardian of Ancient Kings ============================================

struct guardian_of_ancient_kings_t : public paladin_spell_t
{
  guardian_of_ancient_kings_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "guardian_of_ancient_kings", p, p -> find_specialization_spell( "Guardian of Ancient Kings" ) )
  {
    parse_options( NULL, options_str );
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.guardian_of_ancient_kings -> trigger();

  }
};

// Hand of Purity ===========================================================

struct hand_of_purity_t : public paladin_spell_t
{
  hand_of_purity_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "hand_of_purity", p, p -> find_talent_spell( "Hand of Purity" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    may_miss = false; //probably redundant with harmul=false?

    // disable if not talented
    if ( ! ( p -> talents.hand_of_purity -> ok() ) )
      background = true;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.hand_of_purity -> trigger();
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

    cooldown -> duration += timespan_t::from_millis( p -> perk.enhanced_hand_of_sacrifice -> effectN( 1 ).base_value() );
  }

  void trigger( double redirect_value )
  {
    // set the redirect amount based on the result of the action
    base_dd_min = redirect_value;
    base_dd_max = redirect_value;

    // glyph of HoSac eliminates redirected damage
    if ( ! p() -> glyphs.hand_of_sacrifice -> ok() )
      execute();
  }

  // Hand of Sacrifice's Execute function is defined after Paladin Buffs, Part Deux because it requires 
  // the definition of the buffs_t::hand_of_sacrifice_t object.
};

struct hand_of_sacrifice_t : public paladin_spell_t
{
  hand_of_sacrifice_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "hand_of_sacrifice", p, p-> find_class_spell( "Hand of Sacrifice" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    may_miss = false;
//    p -> active.hand_of_sacrifice_redirect = new hand_of_sacrifice_redirect_t( p );

    if ( p -> talents.clemency -> ok() )
      cooldown -> charges = 2;

    // Create redirect action conditionalized on the existence of HoS.
    if ( ! p -> active.hand_of_sacrifice_redirect )
      p -> active.hand_of_sacrifice_redirect = new hand_of_sacrifice_redirect_t( p );
  }

  virtual void execute();

};

// Holy Avenger =============================================================

struct holy_avenger_t : public paladin_spell_t
{
  holy_avenger_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_avenger", p, p -> find_talent_spell( "Holy Avenger" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    // disable if not talented
    if ( ! p -> talents.holy_avenger -> ok() )
      background = true;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    p() -> buffs.holy_avenger -> trigger( 1, 3 );
  }
};

// Holy Light Spell =========================================================

struct holy_light_t : public paladin_heal_t
{
  holy_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_light", p, p -> find_class_spell( "Holy Light" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    p() -> buffs.infusion_of_light -> expire();
    
    // Enhanced Holy Shock trigger
    p() -> buffs.enhanced_holy_shock -> trigger();
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> check() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    paladin_heal_t::schedule_execute( state );

    p() -> buffs.infusion_of_light -> up(); // Buff uptime tracking
  }

  virtual double action_multiplier() const
  {
    double am = paladin_heal_t::action_multiplier();
    
    am *= 1.0 + p() -> perk.improved_holy_light -> effectN( 1 ).percent();

    return am;
  }
};

// Holy Shield proc ===========================================================

struct holy_shield_proc_t : public paladin_spell_t
{
  holy_shield_proc_t( paladin_t* p )
    : paladin_spell_t( "holy_shield", p, p -> find_spell( 157122 ) ) // damage data stored in 157122
  {
    background = true;
    proc = true;
    may_miss = false;
    // may_crit = true; TODO: test?
    // may_multistrike = false; TODO: test?
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

    // todo: code AoE heal, insert as proc on HPr damage.
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
    parse_options( NULL, options_str );

    // create the damage and healing spell effects, designate them as children for reporting
    damage = new holy_prism_damage_t( p );
    add_child( damage );
    heal = new holy_prism_heal_t( p );
    add_child( heal );

    // disable if not talented
    if ( ! ( p -> talents.holy_prism -> ok() ) )
      background = true;
  }

  virtual void execute()
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

// Holy Radiance ============================================================

struct holy_radiance_hot_t : public paladin_heal_t
{
  holy_radiance_hot_t( paladin_t* p, const spell_data_t* s ) :
    paladin_heal_t( "holy_radiance", p, s )
  {
    background = true;
    dual = true;
    direct_tick = true;
  }
};

struct holy_radiance_t : public paladin_heal_t
{
  holy_radiance_hot_t* hot;

  holy_radiance_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_radiance", p, p -> find_class_spell( "Holy Radiance" ) ),
    hot( new holy_radiance_hot_t( p, data().effectN( 1 ).trigger() ) )
  {
    parse_options( NULL, options_str );

    // FIXME: This is an AoE Hot, which isn't supported currently
    aoe = data().effectN( 2 ).base_value();
  }

  virtual void tick( dot_t* d )
  {
    paladin_heal_t::tick( d );

    hot -> execute();
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    p() -> buffs.infusion_of_light -> expire();
    p() -> buffs.daybreak -> trigger();
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> check() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    paladin_heal_t::schedule_execute( state );

    p() -> buffs.infusion_of_light -> up(); // Buff uptime tracking
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
  double crit_increase;

  holy_shock_damage_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_shock_damage", p, p -> find_spell( 25912 ) ),
      crit_increase( 0.0 )
  {

    background = true;
    trigger_gcd = timespan_t::zero();

    // this grabs the 25% base crit bonus from 20473
    base_crit += p -> find_class_spell( "Holy Shock" ) -> effectN( 1 ).percent();

  }

  virtual double composite_crit() const
  {
    double cc = paladin_spell_t::composite_crit();

    if ( p() -> buffs.avenging_wrath -> check() )
    {
      cc += crit_increase;
    }

    return cc;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      int g = 1;
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_holy_shock );
      if ( p() -> buffs.holy_avenger -> check() )
      {
        p() -> resource_gain( RESOURCE_HOLY_POWER,
                              p() -> buffs.holy_avenger -> value() - g,
                              p() -> gains.hp_holy_avenger );
      }
    }
  }

  virtual double action_multiplier() const
  {
    double am = paladin_spell_t::action_multiplier();

    am *= 1.0 + p() -> perk.empowered_holy_shock -> effectN( 1 ).percent();

    return am;
  }

};

// Holy Shock Heal Spell ====================================================

struct holy_shock_heal_t : public paladin_heal_t
{
  double crit_increase;
  daybreak_t* daybreak;

  holy_shock_heal_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_shock_heal", p, p -> find_spell( 25914 ) ),
    crit_increase( 0.0 )
  {
    background = true;
    trigger_gcd = timespan_t::zero();
        
    // this grabs the 25% base crit bonus from 20473
    base_crit += p -> find_class_spell( "Holy Shock" ) -> effectN( 1 ).percent();

    // Daybreak gives this a 75% splash heal
    daybreak = new daybreak_t( p );
    //add_child( daybreak );
  }

  virtual double composite_crit() const
  {
    double cc = paladin_heal_t::composite_crit();

    if ( p() -> buffs.avenging_wrath -> check() )
    {
      cc += crit_increase;
    }

    return cc;
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    int g = 1;
    p() -> resource_gain( RESOURCE_HOLY_POWER,
                          g,
                          p() -> gains.hp_holy_shock );
    if ( p() -> buffs.holy_avenger -> check() )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER,
                            std::max( ( int ) 0, ( int )( p() -> buffs.holy_avenger -> value() - g ) ),
                            p() -> gains.hp_holy_avenger );
    }

    if ( execute_state -> result == RESULT_CRIT )
      p() -> buffs.infusion_of_light -> trigger();
  }

  virtual void impact ( action_state_t* s )
  {
    paladin_heal_t::impact( s );

    if ( p() -> buffs.daybreak -> up() )
    {
      // trigger the Daybreak heal
      daybreak -> base_dd_min = daybreak -> base_dd_max = s -> result_amount;
      daybreak -> schedule_execute();
      
      // expire the buff
      p() -> buffs.daybreak -> expire();
    }
  }

};

struct holy_shock_t : public paladin_heal_t
{
  holy_shock_damage_t* damage;
  holy_shock_heal_t* heal;
  timespan_t cd_duration;
  double cooldown_mult;

  holy_shock_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "holy_shock", p, p -> find_specialization_spell( "Holy Shock" ) )
  {
    check_spec( PALADIN_HOLY );
    parse_options( NULL, options_str );

    cd_duration = cooldown -> duration;

    double crit_increase = 0.0;

    // Bonuses from Sanctified Wrath need to be stored for future use
    if ( ( p -> specialization() == PALADIN_HOLY ) && p -> find_talent_spell( "Sanctified Wrath" ) -> ok()  )
    {
      // the actual values of these are stored in spell 114232 rather than the spell returned by find_talent_spell
      cooldown_mult = p -> spells.sanctified_wrath -> effectN( 1 ).percent();
      crit_increase = p -> spells.sanctified_wrath -> effectN( 5 ).percent();
    }

    // create the damage and healing spell effects, designate them as children for reporting
    damage = new holy_shock_damage_t( p, options_str );
    damage ->crit_increase = crit_increase;
    add_child( damage );
    heal = new holy_shock_heal_t( p, options_str );
    heal ->crit_increase = crit_increase;
    add_child( heal );

  }

  virtual void execute()
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


    if ( p() -> buffs.enhanced_holy_shock -> check() )
    {
      // this feels like the wrong way to accomplish this, will review later
      cooldown -> duration = timespan_t::zero();
      p() -> buffs.enhanced_holy_shock -> expire();
    }
    else
      cooldown -> duration = cd_duration;

    paladin_heal_t::execute();
  }

  virtual void update_ready( timespan_t cd_override )
  {
    cd_override = cooldown -> duration;
    if ( p() -> buffs.avenging_wrath -> up() )
      cd_override *= cooldown_mult;

    paladin_heal_t::update_ready( cd_override );
  }
};

// Holy Wrath ===============================================================

struct holy_wrath_t : public paladin_spell_t
{
  holy_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_wrath", p, p -> find_class_spell( "Holy Wrath" ) )
  {
    parse_options( NULL, options_str );

    if ( ! p -> glyphs.focused_wrath -> ok() )
      aoe = -1;

    may_crit   = true;
    split_aoe_damage = true;

    //Holy Wrath is an oddball - spell database entry doesn't properly contain damage details
    //Tooltip formula is wrong as well.  We're gong to hardcode it here based on empirical testing
    //see http://maintankadin.failsafedesign.com/forum/viewtopic.php?p=743800#p743800
    attack_power_mod.direct =  0.91;         // 0.91 AP scaling

  }

  //Sanctity of Battle reduces cooldown
  virtual void update_ready( timespan_t cd_duration )
  {
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }
    paladin_spell_t::update_ready( cd_duration );
  }

  virtual double action_multiplier() const
  {
    double am = paladin_spell_t::action_multiplier();

    if ( p() -> glyphs.final_wrath -> ok() && target -> health_percentage() < 20 )
    {
      am *= 1.0 + p() -> glyphs.final_wrath -> effectN( 1 ).percent();
    }

    return am;
  }
};

// Illuminated Healing ======================================================
struct illuminated_healing_t : public paladin_absorb_t
{
  illuminated_healing_t( paladin_t* p ) :
    paladin_absorb_t( "illuminated_healing", p, p -> find_spell( 86273 ) )
  {
    background = true;
    proc = true;
    trigger_gcd = timespan_t::zero();
  }
};

// Lay on Hands Spell =======================================================

struct lay_on_hands_t : public paladin_heal_t
{
  lay_on_hands_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "lay_on_hands", p, p -> find_class_spell( "Lay on Hands" ) )
  {
    parse_options( NULL, options_str );

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );

    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();
  }

  virtual double calculate_direct_amount( action_state_t* )
  {
    return p() -> resources.max[ RESOURCE_HEALTH ];
  }

  virtual void execute()
  {
    paladin_heal_t::execute();

    target -> debuffs.forbearance -> trigger();
  }

  virtual bool ready()
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
    benefits_from_seal_of_insight = false;
  }
  
  std::vector< player_t* >& target_list() const
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
    parse_options( NULL, options_str );
    may_miss = false;

    school = SCHOOL_HOLY; // Setting this allows the tick_action to benefit from Inquistion

    base_tick_time = p -> find_spell( 114918 ) -> effectN( 1 ).period();
    num_ticks      = ( int ) ( ( p -> find_spell( 122773 ) -> duration() - travel_time_ ) / base_tick_time );
    cooldown -> duration = p -> find_spell( 114158 ) -> cooldown();
    hasted_ticks   = false;

    dynamic_tick_action = true;
    //tick_action = new lights_hammer_tick_t( p, p -> find_spell( 114919 ) );
    lh_heal_tick = new lights_hammer_heal_tick_t( p );
    lh_damage_tick = new lights_hammer_damage_tick_t( p );

    // disable if not talented
    if ( ! ( p -> talents.lights_hammer -> ok() ) )
      background = true;
  }

  virtual timespan_t travel_time() const
  { return travel_time_; }

  virtual void tick( dot_t* d )
  {
    // trigger healing and damage ticks
    lh_heal_tick -> schedule_execute();
    lh_damage_tick -> schedule_execute();

    paladin_spell_t::tick( d );
  }
};

// Light of Dawn ============================================================

struct light_of_dawn_t : public paladin_heal_t
{
  light_of_dawn_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "light_of_dawn", p, p -> find_class_spell( "Light of Dawn" ) )
  {
    parse_options( NULL, options_str );

    aoe = 6;
  }

  virtual double action_multiplier() const
  {
    double am = paladin_heal_t::action_multiplier();

    am *= p() -> holy_power_stacks();

    // Holy Insight buffs all healing by 25% & WoG/EF/LoD by 50%.
    // The 25% buff is already in paladin_heal_t, so we need to divide by that first & then apply 50%
    am /= 1.0 + p() -> passives.holy_insight -> effectN( 6 ).percent();
    am *= 1.0 + p() -> passives.holy_insight -> effectN( 9 ).percent();

    return am;
  }
};

// Sacred Shield ============================================================

struct sacred_shield_t : public paladin_heal_t
{
  sacred_shield_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "sacred_shield", p, p -> find_talent_spell( "Sacred Shield" ) ) // todo: find_talent_spell -> find_specialization_spell
  {
    parse_options( NULL, options_str );
    may_crit = tick_may_crit = false;
    benefits_from_seal_of_insight = false;
    harmful = false;

    // treat this as a HoT that spawns an absorb bubble on each tick() call rather than healing
    // unfortunately, this spell info is split between effects and tooltip 
    base_td = data().effectN( 1 ).average( p ); 
    spell_power_mod.tick = 0.819; // in tooltip, hardcoding

    // redirect HoT to self if not specified
    if ( target -> is_enemy() || target -> type == HEALING_ENEMY )
      target = p;

    // disable if not talented
    if ( ! ( p -> talents.sacred_shield -> ok() ) )
      background = true;

    // Spell data reflects protection values; Ret and Holy are 30% larger
    if ( ( p -> specialization() == PALADIN_RETRIBUTION || p -> specialization() == PALADIN_HOLY ) )
    {
      base_td /= 0.7;
      spell_power_mod.tick /= 0.7;
    }

    // Holy gets other special stuff - no cooldown, no target limit, 3 charges, 10-second recharge time, extra (zero) tick
    if ( p -> specialization() == PALADIN_HOLY )
    {
      // 3 charges, recharge time is 10 seconds (base+4)
      cooldown -> charges = 3;
      cooldown -> duration += timespan_t::from_seconds( 4 );
      // extra tick immediately upon cast
      tick_zero = true;      
    }

  }

  virtual void tick( dot_t* d )
  {
    // Kludge to swap the heal for an absorb
    // calculate the tick amount
    double ss_tick_amount = calculate_tick_amount( d -> state );

    // if an existing absorb bubble is still hanging around, kill it
    td( d -> state -> target ) -> buffs.sacred_shield_tick -> expire();

    // trigger a new 6-second absorb bubble with the absorb amount we stored earlier
    td( d -> state -> target ) -> buffs.sacred_shield_tick -> trigger( 1, ss_tick_amount );

    // note that we don't call paladin_heal_t::tick( d ) so that the heal doesn't happen
  }

  virtual void last_tick( dot_t* d )
  {
    p() -> buffs.sacred_shield -> expire();
    paladin_heal_t::last_tick( d );
  }

  virtual void execute()
  {
    // if cast precombat, we need to kludge some things
    if ( ! p() -> in_combat )
    {
      // in theory, we need to tweak the number of ticks and force a new one precombat
      // in practice, it's ticking immediately when cast precombat for some reason, so we can skip all of that
      // keeping this code here just in case that behavior changes

      //int orig_ticks = num_ticks;
      //num_ticks -= 1;

      paladin_heal_t::execute();

      //num_ticks = orig_ticks;

      p() -> buffs.sacred_shield -> trigger();

      // force tick
      //tick( get_dot( p() ) );

    }
    else
    {
      paladin_heal_t::execute();

      p() -> buffs.sacred_shield -> trigger();
    }
  }
};

// Seal of Insight ==========================================================

struct seal_of_insight_proc_t : public paladin_heal_t
{
  double proc_chance;
  proc_t* proc_tracker;

  seal_of_insight_proc_t( paladin_t* p ) :
    paladin_heal_t( "seal_of_insight_proc", p, p -> find_class_spell( "Seal of Insight" ) ),
    proc_chance( 0.0 ),
    proc_tracker( p -> get_proc( name_str ) )
  {
    background  = true;
    proc = true;
    trigger_gcd = timespan_t::zero();
    may_crit = false; //cannot crit


    // spell database info is in tooltip
    attack_power_mod.direct = 0.15;
    spell_power_mod.direct  = 0.15;

    // Battle Healer glyph
    if ( p -> glyphs.battle_healer -> ok() ) {
      attack_power_mod.direct *= p -> glyphs.battle_healer -> effectN( 1 ).percent();
      spell_power_mod.direct *= p -> glyphs.battle_healer -> effectN( 1 ).percent();
    }

    // needed for weapon speed, I assume
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
    
    // proc chance is now 20 PPM, spell database info still says 15.
    // Best guess is that the 4th effect describes the additional 5 PPM.
    proc_chance = ppm_proc_chance( data().effectN( 1 ).base_value() + data().effectN( 2 ).base_value() );

    target = player;
  }

  virtual void execute()
  {
    if ( rng().roll( proc_chance ) )
    {
      proc_tracker -> occur();

      // Battle Healer glyph makes SoI a smart heal
      if ( p() -> glyphs.battle_healer -> ok() )
      {
        target = find_lowest_player();
        if ( target ) 
          paladin_heal_t::execute();
      }
      else
        paladin_heal_t::execute();
    }
    else
    {
      update_ready();
    }
  }
};

// Shining Protector ========================================================
// This is a protection-specific multistrike effect

struct shining_protector_t : public paladin_heal_t
{
  proc_t* proc_tracker;

  shining_protector_t( paladin_t* p )
    : paladin_heal_t( "shining_protector", p, p -> find_specialization_spell( "Shining Protector" ) ),
    proc_tracker( p -> get_proc( name_str ) )
  {
    background = true;
    proc = true;
    target = player;
    may_multistrike = false; // guess, almost certain
    may_crit = false;        // guess, but pretty likely
  }

  virtual void init()
  {
    paladin_heal_t::init();
    // add snapshot flags to allow for action_multiplier() to be called
    snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;
  }

  virtual double action_da_multiplier() const
  {
    double am = p() -> passives.shining_protector -> effectN( 1 ).percent();

    return am;
  }

  virtual void execute()
  {
    proc_tracker -> occur();

    paladin_heal_t::execute();
  }

};

// Word of Glory  ===========================================================

struct word_of_glory_t : public paladin_heal_t
{
  // find_class_spell("Word of Glory") returns spellid 85673, which is a stub that points to 130551 for heal and 130552 for harsh words.
  // We'll call that to get spell cooldown, resource cost, and other information and then parse_effect_data on 130551 to get healing coefficients
  // Note: if you have Eternal Flame talented, find_class_spell will return a not_found() object!  Will fix in APL parsing.
  word_of_glory_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "word_of_glory", p, p -> find_class_spell( "Word of Glory" ) )
  {
    parse_options( NULL, options_str );

    // healing coefficients stored in 130551
    parse_effect_data( p -> find_spell( 130551 ) -> effectN( 1 ) );

    // remove GCD constraints & cast time for prot
    if ( p -> passives.guarded_by_the_light -> ok() )
    {
      trigger_gcd = timespan_t::zero();
      use_off_gcd = true;
      base_execute_time *= 1 + p -> passives.guarded_by_the_light -> effectN( 10 ).percent();
    }
    // remove cast time for ret
    if ( p -> passives.sword_of_light -> ok() )
    {
      base_execute_time *= 1 + p -> passives.sword_of_light -> effectN( 9 ).percent();
    }
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }

    paladin_heal_t::update_ready( cd_duration );
  }

  virtual double cost() const
  {
    // check for T16 4-pc tank effect
    if ( p() -> buffs.bastion_of_power -> check() && target == player )
      return 0.0;

    return paladin_heal_t::cost();
  }

  virtual void consume_free_hp_effects()
  {
    // order of operations: T16 4-pc tank, then Divine Purpose (assumed!)

    // check for T16 4pc tank bonus
    if ( p() -> buffs.bastion_of_power -> check() && target == player )
    {
      // nothing necessary here, BoG & BoPower will be consumed automatically upon cast
      return;
    }

    paladin_heal_t::consume_free_hp_effects();
  }

  virtual double action_multiplier() const
  {
    double am = paladin_heal_t::action_multiplier();
    double c = cost();

    // scale the am by holy power spent, can't be more than 3 and Divine Purpose counts as 3
    am *= ( ( p() -> holy_power_stacks() <= 3  && c > 0.0 ) ? p() -> holy_power_stacks() : 3 );

    // T14 protection 4-piece bonus
    am *= ( 1.0 + p() -> sets.set( SET_T14_4PC_TANK ) -> effectN( 1 ).percent() );

    if ( p() -> buffs.bastion_of_glory -> up() )
    {
      // grant 10% extra healing per stack of BoG; can't expire() BoG here because it's needed in execute()
      am *= ( 1.0 + p() -> buffs.bastion_of_glory -> stack() * ( p() -> buffs.bastion_of_glory -> data().effectN( 1 ).percent() + p() -> get_divine_bulwark() ) );
    }

    // Improved Word of Glory perk
    am *= 1.0 + p() -> perk.improved_word_of_glory -> effectN( 1 ).percent();

    // Holy Insight buffs all healing by 25% & WoG/EF/LoD by 50%.
    // The 25% buff is already in paladin_heal_t, so we need to divide by that first & then apply 50%
    am /= 1.0 + p() -> passives.holy_insight -> effectN( 6 ).percent();
    am *= 1.0 + p() -> passives.holy_insight -> effectN( 9 ).percent();

    return am;
  }

  virtual void execute()
  {
    double hopo = std::min( 3, p() -> holy_power_stacks() ); //can't consume more than 3 Holy Power

    paladin_heal_t::execute();

    // Glyph of Word of Glory handling
    if ( p() -> spells.glyph_of_word_of_glory -> ok() )
    {
      p() -> buffs.glyph_of_word_of_glory -> trigger( 1, p() -> spells.glyph_of_word_of_glory -> effectN( 1 ).percent() * hopo );
    }

    // Shield of Glory (Tier 15 protection 2-piece bonus)
    if ( p() -> sets.has_set_bonus( SET_T15_2PC_TANK ) )
      p() -> buffs.shield_of_glory -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, p() -> buffs.shield_of_glory -> buff_duration * hopo );
        
    // consume BoG stacks and Bastion of Power if used on self
    if ( target == player )
    {
      p() -> buffs.bastion_of_glory -> expire();
      p() -> buffs.bastion_of_power -> expire();
    }
  }
};

// Harsh Word ( Word of Glory Damage ) =====================================

struct harsh_word_t : public paladin_spell_t
{
  harsh_word_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "harsh_word", p, p -> find_class_spell( "Harsh_Word" ) )
  {
    parse_options( NULL, options_str );
    resource_consumed = RESOURCE_HOLY_POWER;
    resource_current = RESOURCE_HOLY_POWER;

    //disable if not glyphed
    if ( ! p -> glyphs.harsh_words -> ok() )
      background = true;

    base_costs[RESOURCE_HOLY_POWER] = 1;
   
    base_execute_time = timespan_t::from_seconds( 1.5 ); // TODO: test if this still has a cast time as holy with HW glyph; irrelevant for prot/ret

    // remove GCD constraints & cast time for prot
    if ( p -> passives.guarded_by_the_light -> ok() )
    {
      trigger_gcd = timespan_t::zero();
      use_off_gcd = true;
      base_execute_time *= 1 + p -> passives.guarded_by_the_light -> effectN( 10 ).percent();
    }
    // remove cast time for ret
    if ( p -> passives.sword_of_light -> ok() )
    {
      base_execute_time *= 1 + p -> passives.sword_of_light -> effectN( 9 ).percent();
    }

  }

  virtual void update_ready( timespan_t cd_duration )
  {
    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }

    paladin_spell_t::update_ready( cd_duration );
  }

  virtual double action_multiplier() const
  {
    double am = paladin_spell_t::action_multiplier();
    double c = cost();

    // scale the am by holy power spent, can't be more than 3 and Divine Purpose counts as 3
    am *= ( ( p() -> holy_power_stacks() <= 3  && c > 0.0 ) ? p() -> holy_power_stacks() : 3 );

    // Improved Word of Glory perk - sure, why not (TODO: test)
    am *= 1.0 + p() -> perk.improved_word_of_glory -> effectN( 1 ).percent();

    return am;
  }

  virtual void execute()
  {
    double hopo = ( p() -> holy_power_stacks() <= 3 ? p() -> holy_power_stacks() : 3 );

    paladin_spell_t::execute();

    if ( p() -> spells.glyph_of_word_of_glory -> ok() )
    {
      p() -> buffs.glyph_of_word_of_glory -> trigger( 1, p() -> spells.glyph_of_word_of_glory -> effectN( 1 ).percent() * hopo );
    }
  }
};


// ==========================================================================
// End Spells, Heals, and Absorbs
// ==========================================================================

// ==========================================================================
// Paladin Attacks
// ==========================================================================

// paladin-specific melee_attack_t class for inheritance

struct paladin_melee_attack_t : public paladin_action_t< melee_attack_t >
{
  // booleans to allow individual control of seal proc triggers
  bool trigger_seal;
  bool trigger_seal_of_righteousness;
  bool trigger_seal_of_justice;
  bool trigger_seal_of_truth;

  // spell cooldown reduction flags
  bool sanctity_of_battle;  //reduces some spell cooldowns/gcds by melee haste

  bool use2hspec;

  paladin_melee_attack_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil(),
                          bool u2h = true ) :
    base_t( n, p, s ),
    trigger_seal( false ),
    trigger_seal_of_righteousness( false ),
    trigger_seal_of_justice( false ),
    trigger_seal_of_truth( false ),
    sanctity_of_battle( false ),
    use2hspec( u2h )
  {
    may_crit = true;
    special = true;
    weapon = &( p -> main_hand_weapon );
  }

  virtual timespan_t gcd() const
  {

    if ( sanctity_of_battle )
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

  virtual void execute()
  {
    base_t::execute();

    // handle all on-hit effects (seals, Ancient Power, battle healer)
    if ( result_is_hit( execute_state -> result ) )
    {
      if ( trigger_seal || ( trigger_seal_of_righteousness && ( p() -> active_seal == SEAL_OF_RIGHTEOUSNESS ) )
                        || ( trigger_seal_of_justice && ( p() -> active_seal == SEAL_OF_JUSTICE ) )
                        || ( trigger_seal_of_truth && ( p() -> active_seal == SEAL_OF_TRUTH ) ) )
      {
        switch ( p() -> active_seal )
        {
          case SEAL_OF_JUSTICE:
            p() -> active_seal_of_justice_proc       -> execute();
            break;
          case SEAL_OF_INSIGHT:
            p() -> active_seal_of_insight_proc       -> execute();
            break;
          case SEAL_OF_RIGHTEOUSNESS:
            p() -> active_seal_of_righteousness_proc -> execute();
            break;
          case SEAL_OF_TRUTH:
            p() -> active_censure                    -> execute();
            if ( td( target ) -> buffs.debuffs_censure -> stack() >= 1 ) p() -> active_seal_of_truth_proc -> execute();
            break;
          default: break;
        }
      }
    }
  }

  virtual double action_multiplier() const
  {
    double am = base_t::action_multiplier();

    if ( use2hspec && ( p() -> passives.sword_of_light -> ok() ) && ( p() -> main_hand_weapon.group() == WEAPON_2H ) )
    {
      am *= 1.0 + p() -> passives.sword_of_light_value -> effectN( 1 ).percent();
    }

    return am;
  }
};

// Melee Attack =============================================================

struct melee_t : public paladin_melee_attack_t
{
  melee_t( paladin_t* p ) :
    paladin_melee_attack_t( "melee", p, spell_data_t::nil(), true )
  {
    school = SCHOOL_PHYSICAL;
    special               = false;
    trigger_seal          = true;
    background            = true;
    repeating             = true;
    trigger_gcd           = timespan_t::zero();
    base_execute_time     = p -> main_hand_weapon.swing_time;
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat ) return timespan_t::from_seconds( 0.01 );
    return paladin_melee_attack_t::execute_time();
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();
    if ( result_is_hit( execute_state -> result ) )
    {
      // Check for Art of War procs
      if ( p() -> passives.the_art_of_war -> ok() && rng().roll( p() -> passives.the_art_of_war -> proc_chance() ) )
      {
        // if Exorcism was already off-cooldown, count the proc as wasted
        if ( p() -> cooldowns.exorcism -> remains() <= timespan_t::zero() )
        {
          p() -> procs.wasted_art_of_war -> occur();
        }

        // trigger proc, reset Exorcism cooldown
        p() -> procs.the_art_of_war -> occur();
        p() -> cooldowns.exorcism -> reset( true );

        // activate T16 2-piece bonus
        if ( p() -> sets.has_set_bonus( SET_T16_2PC_MELEE ) )
          p() -> buffs.warrior_of_the_light -> trigger();
      }
    }
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

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( p() -> is_moving() ) return false;
    return( p() -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Crusader Strike ==========================================================

struct crusader_strike_t : public paladin_melee_attack_t
{
  const spell_data_t* sword_of_light;

  crusader_strike_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "crusader_strike", p, p -> find_class_spell( "Crusader Strike" ), true ),
      sword_of_light( p -> find_specialization_spell( "Sword of Light" ) )
  {
    parse_options( NULL, options_str );

    // CS triggers all seals
    trigger_seal = true;

    // Sanctity of Battle reduces melee GCD
    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();

    // multiplier modification for T13 Retribution 2-piece bonus
    base_multiplier *= 1.0 + ( p -> sets.set( SET_T13_2PC_MELEE ) -> effectN( 1 ).percent() );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 7 ).percent()
                                       +  p -> passives.sword_of_light -> effectN( 4 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );
  }

  virtual double action_multiplier() const
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Holy Avenger buffs CS damage by 30% while active
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    // Improved Crusader Strike perk
    am *= 1.0 + p() -> perk.improved_crusader_strike -> effectN( 1 ).percent();

    return am;
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }

    paladin_melee_attack_t::update_ready( cd_duration );
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    // Special things that happen when CS connects
    if ( result_is_hit( s -> result ) )
    {
      // Holy Power gains, only relevant if CS connects
      int g = 1; // default is a gain of 1 Holy Power
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_crusader_strike ); // apply gain, record as due to CS

      // If Holy Avenger active, grant 2 more
      if ( p() -> buffs.holy_avenger -> check() )
      {
        //apply gain, record as due to Holy Avenger
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }

      // Trigger Hand of Light procs
      trigger_hand_of_light( s );

      // Check for T15 Ret 4-piece bonus proc
      if ( p() -> sets.has_set_bonus( SET_T15_4PC_MELEE ) )
        p() -> buffs.tier15_4pc_melee -> trigger();

    }
  }
};

// Divine Storm =============================================================

struct divine_storm_t : public paladin_melee_attack_t
{
  double heal_percentage;

  divine_storm_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "divine_storm", p, p -> find_class_spell( "Divine Storm" ) ), heal_percentage( 0.0 )
  {
    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );

    aoe                = -1;
    trigger_seal       = false;
    sanctity_of_battle = true;
    trigger_seal_of_righteousness = true;

    if ( p -> glyphs.divine_storm -> ok() )
    {
      heal_percentage = p -> find_spell( 115515 ) -> effectN( 1 ).percent();
    }
  }

  virtual double cost() const
  {
    // check for the T16 4-pc melee buff
    if ( p() -> buffs.divine_crusader -> check() )
      return 0.0;

    return paladin_melee_attack_t::cost();
  }

  virtual void consume_free_hp_effects()
  {
    // order of operations: T16 4-pc melee, then Divine Purpose

    // check for T16 4pc melee set bonus
    if ( p() -> buffs.divine_crusader -> up() )
    {
      p() -> buffs.divine_crusader -> expire();

      return;
    }

    paladin_melee_attack_t::consume_free_hp_effects();
  }

  virtual double action_multiplier() const
  {
    double am = paladin_melee_attack_t::action_multiplier();
    if ( p() -> buffs.divine_crusader -> check() )
    {
      am *= 1.0 + p() -> buffs.divine_crusader -> data().effectN( 2 ).percent();
    }

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_hand_of_light( s );
      if ( p() -> glyphs.divine_storm -> ok() )
      {
        p() -> resource_gain( RESOURCE_HEALTH, heal_percentage * p() -> resources.max[ RESOURCE_HEALTH ], p() -> gains.glyph_divine_storm, this );
      }
    }
  }
};

// Hammer of Justice, Fist of Justice =======================================

struct hammer_of_justice_t : public paladin_melee_attack_t
{
  hammer_of_justice_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_justice", p, p -> find_class_spell( "Hammer of Justice" ) )
  {
    parse_options( NULL, options_str );
  }
};

struct fist_of_justice_t : public paladin_melee_attack_t
{
  fist_of_justice_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "fist_of_justice", p, p -> find_talent_spell( "Fist of Justice" ) )
  {
    parse_options( NULL, options_str );
  }
};


// Hammer of the Righteous ==================================================

struct hammer_of_the_righteous_aoe_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_aoe_t( paladin_t* p )
    : paladin_melee_attack_t( "hammer_of_the_righteous_aoe", p, p -> find_spell( 88263 ), false )
  {
    // AoE effect always hits if single-target attack succeeds
    // Doesn't proc seals or Grand Crusader
    // TODO: need to re-test all of this
    may_dodge = false;
    may_parry = false;
    may_miss  = false;
    background = true;
    aoe       = -1;
    trigger_gcd = timespan_t::zero(); // doesn't incur GCD (HotR does that already)
  }
  virtual double action_multiplier() const
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Improved HotR perk
    am *= 1.0 + p() -> perk.improved_hammer_of_the_righteous -> effectN( 1 ).percent();

    // Holy Avenger buffs HotR damage by 30% while active
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }

  // HotR AoE does not hit the main target
  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    tl.clear();

    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      if ( ! sim -> actor_list[ i ] -> is_sleeping() &&
             sim -> actor_list[ i ] -> is_enemy() &&
             sim -> actor_list[ i ] != target )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    // Trigger Hand of Light procs
    trigger_hand_of_light( s );
  }
};

struct hammer_of_the_righteous_t : public paladin_melee_attack_t
{
  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_the_righteous", p, p -> find_class_spell( "Hammer of the Righteous" ), true ) 
  {
    parse_options( NULL, options_str );

    // link with Crusader Strike's cooldown
    cooldown = p -> get_cooldown( "crusader_strike" );
    cooldown -> duration = data().cooldown();

    // Sanctity of Battle reduces melee GCD
    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();

    // HotR triggers all seals
    trigger_seal = true;

    // Implement AoE proc
    impact_action = new hammer_of_the_righteous_aoe_t( p );
    add_child( impact_action );
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }
    paladin_melee_attack_t::update_ready( cd_duration );
  }

  virtual double action_multiplier() const
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Improved HotR perk
    am *= 1.0 + p() -> perk.improved_hammer_of_the_righteous -> effectN( 1 ).percent();

    // Holy Avenger buffs HotR damage by 30% while active
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    // Special things that happen when HotR connects
    if ( result_is_hit( s -> result ) )
    {
      // Holy Power gains, only relevant if CS connects
      int g = 1; // default is a gain of 1 Holy Power
      p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_hammer_of_the_righteous ); // apply gain, record as due to CS

      // If Holy Avenger active, grant 2 more
      if ( p() -> buffs.holy_avenger -> check() )
      {
        //apply gain, record as due to Holy Avenger
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
      }

      // Trigger Hand of Light procs
      trigger_hand_of_light( s );
    }
  }
};

// Hammer of Wrath ==========================================================

struct hammer_of_wrath_t : public paladin_melee_attack_t
{
  double cooldown_mult;

  hammer_of_wrath_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "hammer_of_wrath", p, p -> find_class_spell( "Hammer of Wrath" ), true ),
      cooldown_mult( 1.0 )
  {
    parse_options( NULL, options_str );

    // Sanctity of Battle reduces melee GCD
    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();

    // Cannot be parried or blocked, but can be dodged
    may_parry    = false;
    may_block    = false;

    // no weapon multiplier
    weapon_multiplier = 0.0;

    // HoW scales with SP, not AP; flip base multipliers (direct_power_mod is correct @ 1.61)
    //base_spell_power_multiplier  = 1.0;
    //base_attack_power_multiplier = data().extra_coeff();

    // define cooldown multiplier for use with Sanctified Wrath talent for retribution only
    if ( ( p -> specialization() == PALADIN_RETRIBUTION ) && p -> find_talent_spell( "Sanctified Wrath" ) -> ok()  )
    {
      cooldown_mult = 1.0 + p -> spells.sanctified_wrath -> effectN( 3 ).percent();
    }
  }

  // Special things that happen with Hammer of Wrath damages target
  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // if Ret spec, grant Holy Power
      if ( p() -> passives.sword_of_light -> ok() )
      {
        int g = 1; // base gain is 1 Holy Power
        //apply gain, attribute to Hammer of Wrath
        p() -> resource_gain( RESOURCE_HOLY_POWER, g, p() -> gains.hp_hammer_of_wrath );

        // Holy Avenger adds 2 more Holy Power if active
        if ( p() -> buffs.holy_avenger -> check() )
        {
          //apply gain, attribute to Holy Avenger
          p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - g, p() -> gains.hp_holy_avenger );
        }
      }

      // Trigger Hand of Light procs
      trigger_hand_of_light( s );
    }
  }

  virtual double action_multiplier() const
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Holy Avenger buffs HoW damage by 30% while active
    if ( p() -> specialization() == PALADIN_RETRIBUTION && p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    return am;
  }

  virtual void update_ready( timespan_t cd_override )
  {
    cd_override = cooldown -> duration;

    // Reduction during AW - does nothing if Sanctified Wrath not talented
    if ( p() -> buffs.avenging_wrath -> up() )
    {
      cd_override *= cooldown_mult;
    }

    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_override *= p() -> cache.attack_haste();
    }

    paladin_melee_attack_t::update_ready( cd_override );
  }

  virtual bool ready()
  {
    // not available if target is above 20% health unless Sword of Light present and Avenging Wrath up
    // Improved HoW perk raises the threshold to 35%
    double threshold = ( p() -> perk.empowered_hammer_of_wrath -> ok() ? 15 : 0 ) + 20;
    if ( target -> health_percentage() > threshold && ! ( p() -> passives.sword_of_light -> ok() && p() -> buffs.avenging_wrath -> check() ) )
      return false;

    return paladin_melee_attack_t::ready();
  }
};

// Hand of Light proc =======================================================

struct hand_of_light_proc_t : public paladin_melee_attack_t
{
  hand_of_light_proc_t( paladin_t* p )
    : paladin_melee_attack_t( "hand_of_light", p, spell_data_t::nil(), false )
  {
    school = SCHOOL_HOLY;
    may_crit    = false;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    may_glance  = false;
    proc        = true;
    background  = true;
    trigger_gcd = timespan_t::zero();
    id          = 96172;

    // no weapon multiplier - needs to be done by setting weapon = NULL
    // setting weapon_multiplier=0.0 prevents STATE_MUL_DA from being added to snapshot_flags because 
    // both base_dd_min && base_dd_max are zero
    weapon = NULL;
  }

  virtual double action_multiplier() const
  {
    //am = melee_attack_t::action_multiplier();
    // not *= since we don't want to double dip, just calling base to initialize variables
    double am = static_cast<paladin_t*>( player ) -> get_hand_of_light();
    
    //was double dipping on avenging wrath - hand of light is not affected by avenging wrath so that it does not double dip
    //easier to remove it here than try to add an exception at the global avenging wrath buff level
    if ( p() -> buffs.avenging_wrath -> check() )
    {
      am /= 1.0 + p() -> buffs.avenging_wrath -> get_damage_mod();
    }
    return am;
  }
};

// Judgment =================================================================

struct judgment_t : public paladin_melee_attack_t
{
  double cooldown_mult; // used to handle Sanctified Wrath cooldown reduction

  judgment_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( "judgment", p, p -> find_spell( "Judgment" ), true ),
      cooldown_mult( 1.0 )
  {
    parse_options( NULL, options_str );

    // Sanctity of Battle reduces melee GCD
    sanctity_of_battle = p -> passives.sanctity_of_battle -> ok();


    // no weapon multiplier
    weapon_multiplier = 0.0;

    // Special melee attack that can only miss
    may_glance                   = false;
    may_block                    = false;
    may_parry                    = false;
    may_dodge                    = false;

    // Only triggers Seal of Truth
    trigger_seal_of_truth        = true;

    // define cooldown multiplier for use with Sanctified Wrath talent for protection only
    if ( ( p -> specialization() == PALADIN_PROTECTION ) && p -> find_talent_spell( "Sanctified Wrath" ) -> ok()  )
    {
      // for some reason, this is stored in spell 114232 instead of 53376 (which is returned by find_talent_spell)
      cooldown_mult = 1.0 + p -> spells.sanctified_wrath -> effectN( 2 ).percent();
    }

    // damage multiplier from T14 Retribution 4-piece bonus
    base_multiplier *= 1.0 + p -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    paladin_melee_attack_t::execute();

    // Special things that happen when Judgment succeeds
    if ( result_is_hit( execute_state -> result ) )
    {
      // +1 Holy Power for Ret
      if ( p() -> passives.judgments_of_the_bold -> ok() )
      {
        // apply gain, attribute gain to Judgments of the Bold
        p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_judgments_of_the_bold );

      }
      // +1 Holy Power for Prot
      else if ( p() -> passives.judgments_of_the_wise -> ok() )
      {
        // apply gain, attribute gain to Judgments of the Wise
        p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_judgments_of_the_wise );

        // if sanctified wrath is talented and Avenging Wrath is active, give another HP
        if ( p() -> talents.sanctified_wrath -> ok() && p() -> buffs.avenging_wrath -> check() )
          p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_sanctified_wrath );
      }
      // +1 Holy Power for Holy with Selfless Healer talent
      else if ( p() -> talents.selfless_healer -> ok() && p() -> specialization() == PALADIN_HOLY )
      {
        // apply gain, attribute gain to selfless healer
        p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_selfless_healer );

      }

      // Holy Avenger adds 2 more Holy Power if active
      if ( p() -> buffs.holy_avenger -> check() )
      {
        //apply gain, attribute to Holy Avenger
        p() -> resource_gain( RESOURCE_HOLY_POWER, p() -> buffs.holy_avenger -> value() - 1, p() -> gains.hp_holy_avenger );
      }
    }
  }

  // Special things that happen when Judgment damages target
  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    // Glyph of Double Jeopardy handling ====================================
    // original implementation was incorrect - would only grant benefit ~ half the time.
    // proper implemenation: buff granted on every J, target set to new value each time.
    // action_multiplier() gets called before impact(), so we can just set the buff parameters here
    // and let action_multiplier() take care of figuring out if we should get a damage boost or not.

    // if the glyph is present, trigger the buff and store the target information
    if ( p() -> glyphs.double_jeopardy -> ok() )
    {
      p() -> buffs.double_jeopardy -> trigger();
      p() -> last_judgement_target = s -> target;
    }
    // end Double Jeopardy ==================================================

    // Physical Vulnerability debuff
    if ( ! sim -> overrides.physical_vulnerability && p() -> passives.judgments_of_the_bold -> ok() )
      s -> target -> debuffs.physical_vulnerability -> trigger();

    // Selfless Healer talent
    if ( p() -> talents.selfless_healer -> ok() )
      p() -> buffs.selfless_healer -> trigger();
  }

  virtual double action_multiplier() const
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Improved Judgment perk
    am *= 1.0 + p() -> perk.improved_judgment -> effectN( 1 ).percent();

    // Holy Avenger buffs J damage by 30% while active
    if ( p() -> buffs.holy_avenger -> check() )
    {
      am *= 1.0 + p() -> buffs.holy_avenger -> data().effectN( 4 ).percent();
    }

    // Double Jeopardy damage boost, only if we hit a different target with it
    // this gets called before impact(), so last_judgment_target won't be updated until after this resolves
    if (  p() -> glyphs.double_jeopardy -> ok()              // glyph equipped
          && target != p() -> last_judgement_target      // current target is different than last_j_target
          && p() -> buffs.double_jeopardy -> check() )   // double_jeopardy buff is still active
    {
      am *= 1.0 + p() -> buffs.double_jeopardy -> value();
    }

    return am;
  }

  // Cooldown reduction effects
  virtual void update_ready( timespan_t cd_duration )
  {
    cd_duration = cooldown -> duration;

    // Sanctity of Battle
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration *= p() -> cache.attack_haste();
    }

    // Reduction during AW - does nothing if Sanctified Wrath not talented
    if ( p() -> buffs.avenging_wrath -> up() )
    {
      cd_duration *= cooldown_mult;
    }

    paladin_melee_attack_t::update_ready( cd_duration );
  }

  virtual bool ready()
  {
    // Only usable if a seal is active
    if ( p() -> active_seal == SEAL_NONE ) return false;

    return paladin_melee_attack_t::ready();
  }
};

// Rebuke ===================================================================

struct rebuke_t : public paladin_melee_attack_t
{
  rebuke_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "rebuke", p, p -> find_class_spell( "Rebuke" ) )
  {
    parse_options( NULL, options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    // no weapon multiplier
    weapon_multiplier = 0.0;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return paladin_melee_attack_t::ready();
  }
};

// Seals ====================================================================

struct paladin_seal_t : public paladin_melee_attack_t
{
  seal_e seal_type;

  paladin_seal_t( paladin_t* p, const std::string& n, seal_e st, const std::string& options_str )
    : paladin_melee_attack_t( n, p ), seal_type( st )
  {
    parse_options( NULL, options_str );

    harmful    = false;
    resource_current = RESOURCE_MANA;
    // all seals cost 16.4% of base mana
    base_costs[ current_resource() ]  = p -> resources.base[ current_resource() ] * 0.164;
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );
    consume_resource();
    //seal_e seal_orig = p() -> active_seal;
    if ( p() -> specialization() == PALADIN_PROTECTION && seal_type == SEAL_OF_JUSTICE )
    {
      sim -> errorf( "Protection attempted to cast Seal of Justice, defaulting to no seal" );
      p() -> active_seal = SEAL_NONE;
    }
    else
      p() -> active_seal = seal_type; // set the new seal

    // if we've swapped to or from Seal of Insight, we'll need to refresh spell haste cache

  }

  virtual bool ready()
  {
    if ( p() -> active_seal == seal_type ) return false;
    return paladin_melee_attack_t::ready();
  }
};

// Seal of Justice ==========================================================

struct seal_of_justice_proc_t : public paladin_melee_attack_t
{
  seal_of_justice_proc_t( paladin_t* p ) :
    paladin_melee_attack_t( "seal_of_justice_proc", p, p -> find_spell( p -> find_class_spell( "Seal of Justice" ) -> ok() ? 20170 : 0 ) )
  {
    background        = true;
    proc              = true;
    trigger_gcd       = timespan_t::zero();

    base_multiplier *= 1.0 + p -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
  }
};

// Seal of Righteousness ====================================================

struct seal_of_righteousness_proc_t : public paladin_melee_attack_t
{
  seal_of_righteousness_proc_t( paladin_t* p ) :
    paladin_melee_attack_t( "seal_of_righteousness_proc", p, p -> find_spell( p -> find_class_spell( "Seal of Righteousness" ) -> ok() ? 101423 : 0 ) )
  {
    background  = true;
    proc        = true;
    trigger_gcd = timespan_t::zero();

    aoe         = -1;

    // T14 Retribution 4-piece increases seal damage
    base_multiplier *= 1.0 + p -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
  }
};

// Seal of Truth ============================================================

struct seal_of_truth_proc_t : public paladin_melee_attack_t
{
  seal_of_truth_proc_t( paladin_t* p )
    : paladin_melee_attack_t( "seal_of_truth_proc", p, p -> find_class_spell( "Seal of Truth" ), true )
  {
    background  = true;
    proc        = true;
    // automatically connects when triggered
    may_block   = false;
    may_glance  = false;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    trigger_gcd = timespan_t::zero();

    if ( data().ok() )
    {
      // proc info stored in spell 42463.
      // Note that effect #2 is a lie, SoT uses unnormalized weapon speed, so we leave that as default (false)
      const spell_data_t* s = p -> find_spell( 42463 );
      if ( s && s -> ok() )
      {
        // Undocumented in spell info: proc is 80% smaller for protection
        weapon_multiplier      = s -> effectN( 1 ).percent();
        if ( p -> specialization() == PALADIN_PROTECTION )
        {
          weapon_multiplier /= 5;
        }
      }
    }
  }

  virtual double action_multiplier() const
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Improved SoT perk
    am *= 1.0 + p() -> perk.empowered_seal_of_truth -> effectN( 1 ).percent();

    // Glyph of Immediate Truth increases direct damage
    if ( p() -> glyphs.immediate_truth -> ok() )
      am *= 1.0 + p() -> glyphs.immediate_truth -> effectN( 1 ).percent();
    
    // Retribution T14 4-piece boosts seal damage
    am *= 1.0 + p() -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();

    return am;
  }
};

// Shield of the Righteous ==================================================

struct shield_of_the_righteous_t : public paladin_melee_attack_t
{
  shield_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "shield_of_the_righteous", p, p -> find_class_spell( "Shield of the Righteous" ) )
  {
    parse_options( NULL, options_str );

    if ( ! p -> has_shield_equipped() )
    {
      sim -> errorf( "%s: %s only usable with shield equiped in offhand\n", p -> name(), name() );
      background = true;
    }

    // Triggers all seals
    trigger_seal = true;

    // not on GCD, usable off-GCD
    trigger_gcd = timespan_t::zero();
    use_off_gcd = true;
    

    // no weapon multiplier
    weapon_multiplier = 0.0;
  }

  virtual void execute()
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

    // Add stack of Bastion of Glory
    p() -> buffs.bastion_of_glory -> trigger();

    // if we're using T16_4PC_TANK, apply the bastion_of_power buff if BoG stacks > 3
    if ( p() -> sets.has_set_bonus( SET_T16_4PC_TANK ) && p() -> buffs.bastion_of_glory -> stack() >= 3 )
      p() -> buffs.bastion_of_power -> trigger();

    // clear any Alabaster Shield stacks we may have
    p() -> buffs.alabaster_shield -> expire();
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    // reduce cooldown if Sanctity of Battle present
    if ( p() -> passives.sanctity_of_battle -> ok() )
    {
      cd_duration = cooldown -> duration * p() -> cache.attack_haste();
    }

    paladin_melee_attack_t::update_ready( cd_duration );
  }

  virtual double action_multiplier() const
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Alabaster Shield bonus
    am *= 1.0 + p() -> buffs.alabaster_shield -> stack() * p() -> spells.alabaster_shield -> effectN( 1 ).percent();

    return am;
  }
};

// Templar's Verdict / Final Verdict ========================================================

struct templars_verdict_t : public paladin_melee_attack_t
{
  templars_verdict_t( paladin_t* p, const std::string& options_str )
    : paladin_melee_attack_t( ( p -> talents.final_verdict -> ok() ? "final_verdict" : "templars_verdict" ), 
                              p, 
                              ( p -> talents.final_verdict -> ok() ? p -> find_talent_spell( "Final Verdict" ) : p -> find_class_spell( "Templar's Verdict" ) ), 
                              true )
  {
    parse_options( NULL, options_str );
    sanctity_of_battle = true;
    trigger_seal       = true;
  }

  virtual school_e get_school() const
  {
    // T15 Retribution 4-piece proc turns damage from physical into Holy damage
    if ( p() -> buffs.tier15_4pc_melee -> up() )
      return SCHOOL_HOLY;
    else
      return paladin_melee_attack_t::get_school();
  }

  virtual void execute ()
  {
    // store cost for potential refunding (see below)
    double c = cost();

    paladin_melee_attack_t::execute();

    // missed/dodged/parried TVs do not consume Holy Power, but do consume Divine Purpose
    // check for a miss, and refund the appropriate amount of HP if we spent any
    if ( result_is_miss( execute_state -> result ) && c > 0 )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, c, p() -> gains.hp_templars_verdict_refund );
    }
  }

  virtual void impact( action_state_t* s )
  {
    paladin_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_hand_of_light( s );

      // Remove T15 Retribution 4-piece effect
      p() -> buffs.tier15_4pc_melee -> expire();
    }
  }

  virtual double action_multiplier() const
  {
    double am = paladin_melee_attack_t::action_multiplier();

    // Tier 13 Retribution 4-piece boosts damage
    am *= 1.0 + p() -> sets.set( SET_T13_4PC_MELEE ) -> effectN( 1 ).percent();

    // Tier 14 Retribution 2-piece boosts damage
    am *= 1.0 + p() -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();

    return am;
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
  virtual bool trigger( int, double value, double, timespan_t )
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

  virtual void expire_override()
  {
    buff_t::expire_override();

    source = nullptr;
    source_health_pool = 0.0;
  }

  virtual void reset()
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

  virtual void expire_override()
  {
    buff_t::expire_override();

    paladin_t* p = static_cast<paladin_t*>( player );
    if ( p -> sets.has_set_bonus( SET_T16_2PC_TANK ) )
    {
      // trigger the HoT
      if ( ! p -> active.blessing_of_the_guardians -> target -> is_sleeping() )
        p -> active.blessing_of_the_guardians -> execute();
    }
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
  actor_pair_t( target, paladin )
{
  dots.eternal_flame      = target -> get_dot( "eternal_flame",      paladin );
  dots.holy_radiance      = target -> get_dot( "holy_radiance",      paladin );
  dots.censure            = target -> get_dot( "censure",            paladin );
  dots.execution_sentence = target -> get_dot( "execution_sentence", paladin );
  dots.stay_of_execution  = target -> get_dot( "stay_of_execution",  paladin );

  buffs.debuffs_censure    = buff_creator_t( *this, "censure", paladin -> find_spell( 31803 ) );
  buffs.eternal_flame      = new buffs::eternal_flame_t( this );
  buffs.glyph_of_flash_of_light = buff_creator_t( *this, "glyph_of_flash_of_light", paladin -> find_spell( 54957 ) );
  buffs.sacred_shield_tick = absorb_buff_creator_t( *this, "sacred_shield_tick", paladin -> find_spell( 65148 ) )
                             .source( paladin -> get_stats( "sacred_shield" ) )
                             .cd( timespan_t::zero() )
                             .gain( target -> get_gain( "sacred_shield_tick" ) );
}

// paladin_t::create_action =================================================

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
  if ( name == "auto_attack"               ) return new auto_melee_attack_t        ( this, options_str );
  if ( name == "ardent_defender"           ) return new ardent_defender_t          ( this, options_str );
  if ( name == "avengers_shield"           ) return new avengers_shield_t          ( this, options_str );
  if ( name == "avenging_wrath"            ) return new avenging_wrath_t           ( this, options_str );
  if ( name == "beacon_of_light"           ) return new beacon_of_light_t          ( this, options_str );
  if ( name == "blessing_of_kings"         ) return new blessing_of_kings_t        ( this, options_str );
  if ( name == "blessing_of_might"         ) return new blessing_of_might_t        ( this, options_str );
  if ( name == "consecration"              ) return new consecration_t             ( this, options_str );
  if ( name == "crusader_strike"           ) return new crusader_strike_t          ( this, options_str );
  if ( name == "devotion_aura"             ) return new devotion_aura_t            ( this, options_str );
  if ( name == "divine_plea"               ) return new divine_plea_t              ( this, options_str );
  if ( name == "divine_protection"         ) return new divine_protection_t        ( this, options_str );
  if ( name == "divine_shield"             ) return new divine_shield_t            ( this, options_str );
  if ( name == "divine_storm"              ) return new divine_storm_t             ( this, options_str );
  if ( name == "execution_sentence"        ) return new execution_sentence_t       ( this, options_str );
  if ( name == "exorcism"                  ) return new exorcism_t                 ( this, options_str );
  if ( name == "fist_of_justice"           ) return new fist_of_justice_t          ( this, options_str );
  if ( name == "final_verdict"             ) return new templars_verdict_t         ( this, options_str );
  if ( name == "hand_of_purity"            ) return new hand_of_purity_t           ( this, options_str );
  if ( name == "hand_of_sacrifice"         ) return new hand_of_sacrifice_t        ( this, options_str );
  if ( name == "hammer_of_justice"         ) return new hammer_of_justice_t        ( this, options_str );
  if ( name == "hammer_of_wrath"           ) return new hammer_of_wrath_t          ( this, options_str );
  if ( name == "hammer_of_the_righteous"   ) return new hammer_of_the_righteous_t  ( this, options_str );
  if ( name == "holy_avenger"              ) return new holy_avenger_t             ( this, options_str );
  if ( name == "holy_radiance"             ) return new holy_radiance_t            ( this, options_str );
  if ( name == "holy_shock"                ) return new holy_shock_t               ( this, options_str );
  if ( name == "holy_shock_heal"           ) return new holy_shock_heal_t          ( this, options_str );
  if ( name == "holy_wrath"                ) return new holy_wrath_t               ( this, options_str );
  if ( name == "guardian_of_ancient_kings" ) return new guardian_of_ancient_kings_t( this, options_str );
  if ( name == "judgment"                  ) return new judgment_t                 ( this, options_str );
  if ( name == "light_of_dawn"             ) return new light_of_dawn_t            ( this, options_str );
  if ( name == "lights_hammer"             ) return new lights_hammer_t            ( this, options_str );
  if ( name == "rebuke"                    ) return new rebuke_t                   ( this, options_str );
  if ( name == "shield_of_the_righteous"   ) return new shield_of_the_righteous_t  ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );
  if ( name == "holy_prism"                ) return new holy_prism_t               ( this, options_str );

  action_t* a = 0;
  if ( name == "seal_of_justice"           ) { a = new paladin_seal_t( this, "seal_of_justice",       SEAL_OF_JUSTICE,       options_str );
                                               active_seal_of_justice_proc       = new seal_of_justice_proc_t       ( this ); return a; }
  if ( name == "seal_of_insight"           ) { a = new paladin_seal_t( this, "seal_of_insight",       SEAL_OF_INSIGHT,       options_str );
                                               active_seal_of_insight_proc       = new seal_of_insight_proc_t       ( this ); return a; }
  if ( name == "seal_of_righteousness"     ) { a = new paladin_seal_t( this, "seal_of_righteousness", SEAL_OF_RIGHTEOUSNESS, options_str );
                                               active_seal_of_righteousness_proc = new seal_of_righteousness_proc_t ( this ); return a; }
  if ( name == "seal_of_truth"             ) { a = new paladin_seal_t( this, "seal_of_truth",         SEAL_OF_TRUTH,         options_str );
                                               active_seal_of_truth_proc         = new seal_of_truth_proc_t         ( this );
                                               active_censure                    = new censure_t                    ( this ); return a; }

  if ( name == "eternal_flame"             ) return new eternal_flame_t            ( this, options_str );
  if ( name == "word_of_glory"             ) return new word_of_glory_t            ( this, options_str );
  if ( name == "sacred_shield"             ) return new sacred_shield_t            ( this, options_str );
  if ( name == "harsh_word"                ) return new harsh_word_t               ( this, options_str );
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

void paladin_t::trigger_shining_protector( action_state_t* s )
{
  if ( ! passives.shining_protector -> ok() || s -> action == active_shining_protector_proc )
    return;

  // Attempt to proc the heal
  if ( rng().roll( composite_multistrike() ) )
  {
    active_shining_protector_proc -> base_dd_max = active_shining_protector_proc -> base_dd_min = s -> result_amount;
    active_shining_protector_proc -> schedule_execute();
  }
}

void paladin_t::trigger_holy_shield()
{
  // escape if we don't have Holy Shield
  if ( ! talents.holy_shield -> ok() )
    return;

  // Check for proc
  if ( rng().roll( talents.holy_shield -> proc_chance() ) )
    active_holy_shield_proc -> schedule_execute();
}

// paladin_t::init_defense ==================================================

void paladin_t::init_defense()
{
  gear.armor *= 1.0 + passives.sanctuary -> effectN( 2 ).percent();

  player_t::init_defense();

}

// paladin_t::init_base =====================================================

void paladin_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 1.0;
  base.spell_power_per_intellect = 1.0;

  //base.stats.attack_power = level * 3; Gone in WoD, double check later.

  // Boundless Conviction raises max holy power to 5
  resources.base[ RESOURCE_HOLY_POWER ] = 3 + passives.boundless_conviction -> effectN( 1 ).base_value();

  // Avoidance diminishing Returns constants/conversions
  base.miss    = 0.030;
  base.dodge   = 0.030;  //90
  base.parry   = 0.030;  //90
  base.block   = 0.030;  //90
  base.block_reduction = 0.3;

  // based on http://sacredduty.net/2012/09/14/avoidance-diminishing-returns-in-mop-followup/
  diminished_kfactor    = 0.886;
  diminished_parry_cap = 2.37186;
  diminished_block_cap = 1.5037594692967;
  diminished_dodge_cap = 0.6656744;
  
  // note that these conversions are level-specific; these are L90 values
  base.dodge_per_agility = 1 / 10000.0 / 100.0; // empirically tested
  base.parry_per_strength = 1 / 95115.8596; // exact value given by Blizzard

  // Holy Insight grants mana regen from spirit during combat
  base.mana_regen_from_spirit_multiplier = passives.holy_insight -> effectN( 3 ).percent();
  
  // Holy Insight increases max mana for Holy
  resources.base_multiplier[ RESOURCE_MANA ] = 1.0 + passives.holy_insight -> effectN( 1 ).percent();
  
  switch ( specialization() )
  {
    case PALADIN_HOLY:
      role = ROLE_HEAL;
      base.distance = 30;
      break;
    case PALADIN_PROTECTION:
      role = ROLE_TANK;
      break;
    default:
      break;
  }

  // this fixes the position output in the HTML file
  position_str = util::position_type_string( base.position );
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  last_judgement_target = 0;
  active_seal = SEAL_NONE;
  bok_up      = false;
  bom_up      = false;
  last_extra_regen = timespan_t::zero();
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  // Mana
  gains.divine_plea                 = get_gain( "divine_plea"            );
  gains.extra_regen                 = get_gain( ( specialization() == PALADIN_RETRIBUTION ) ? "sword_of_light" : "guarded_by_the_light" );
  gains.seal_of_insight             = get_gain( "seal_of_insight"        );
  gains.glyph_divine_storm          = get_gain( "glyph_of_divine_storm"  );
  gains.glyph_divine_shield         = get_gain( "glyph_of_divine_shield" );

  // Holy Power
  gains.hp_blessed_life             = get_gain( "blessed_life" );
  gains.hp_crusader_strike          = get_gain( "crusader_strike" );
  gains.hp_exorcism                 = get_gain( "exorcism" );
  gains.hp_grand_crusader           = get_gain( "grand_crusader" );
  gains.hp_hammer_of_the_righteous  = get_gain( "hammer_of_the_righteous" );
  gains.hp_hammer_of_wrath          = get_gain( "hammer_of_wrath" );
  gains.hp_holy_avenger             = get_gain( "holy_avenger" );
  gains.hp_holy_shock               = get_gain( "holy_shock" );
  gains.hp_judgments_of_the_bold    = get_gain( "judgments_of_the_bold" );
  gains.hp_judgments_of_the_wise    = get_gain( "judgments_of_the_wise" );
  gains.hp_pursuit_of_justice       = get_gain( "pursuit_of_justice" );
  gains.hp_sanctified_wrath         = get_gain( "sanctified_wrath" );
  gains.hp_selfless_healer          = get_gain( "selfless_healer" );
  gains.hp_templars_verdict_refund  = get_gain( "templars_verdict_refund" );
  gains.hp_tower_of_radiance        = get_gain( "tower_of_radiance" );
  gains.hp_judgment                 = get_gain( "judgment" );
  gains.hp_t15_4pc_tank             = get_gain( "t15_4pc_tank" );
}

// paladin_t::init_procs ====================================================

void paladin_t::init_procs()
{
  player_t::init_procs();

  procs.divine_purpose           = get_proc( "divine_purpose"                 );
  procs.divine_crusader          = get_proc( "divine_crusader"                );
  procs.eternal_glory            = get_proc( "eternal_glory"                  );
  procs.judgments_of_the_bold    = get_proc( "judgments_of_the_bold"          );
  procs.the_art_of_war           = get_proc( "the_art_of_war"                 );
  procs.wasted_art_of_war        = get_proc( "wasted_art_of_war"              );
}

// paladin_t::init_scaling ==================================================

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  specialization_e tree = specialization();

  // Only Holy cares about INT/SPI/SP.
  scales_with[ STAT_INTELLECT   ] = ( tree == PALADIN_HOLY );
  scales_with[ STAT_SPIRIT      ] = ( tree == PALADIN_HOLY );
  scales_with[ STAT_SPELL_POWER ] = ( tree == PALADIN_HOLY );

  if ( primary_role() == ROLE_TANK )
  {
    scales_with[ STAT_BLOCK_RATING ] = true;
    scales_with[ STAT_STRENGTH     ] = true;
  }
}

// paladin_t::decode_set ====================================================

set_e paladin_t::decode_set( const item_t& item ) const
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "_of_radiant_glory" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    bool is_heal = ( strstr( s, "headguard"      ) ||
                     strstr( s, "mantle"         ) ||
                     strstr( s, "breastplate"    ) ||
                     strstr( s, "greaves"        ) ||
                     strstr( s, "gloves"         ) );

    if ( is_melee  ) return SET_T13_MELEE;
    if ( is_tank   ) return SET_T13_TANK;
    if ( is_heal   ) return SET_T13_HEAL;
  }

  if ( strstr( s, "white_tiger_" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    bool is_heal = ( strstr( s, "headguard"      ) ||
                     strstr( s, "mantle"         ) ||
                     strstr( s, "breastplate"    ) ||
                     strstr( s, "greaves"        ) ||
                     strstr( s, "gloves"         ) );

    if ( is_melee  ) return SET_T14_MELEE;
    if ( is_tank   ) return SET_T14_TANK;
    if ( is_heal   ) return SET_T14_HEAL;
  }

  if ( strstr( s, "lightning_emperor" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    bool is_heal = ( strstr( s, "headguard"      ) ||
                     strstr( s, "mantle"         ) ||
                     strstr( s, "breastplate"    ) ||
                     strstr( s, "greaves"        ) ||
                     strstr( s, "gloves"         ) );

    if ( is_melee  ) return SET_T15_MELEE;
    if ( is_tank   ) return SET_T15_TANK;
    if ( is_heal   ) return SET_T15_HEAL;
  }

  if ( strstr( s, "_of_winged_triumph" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    bool is_heal = ( strstr( s, "headguard"      ) ||
                     strstr( s, "mantle"         ) ||
                     strstr( s, "breastplate"    ) ||
                     strstr( s, "greaves"        ) ||
                     strstr( s, "gloves"         ) );

    if ( is_melee  ) return SET_T16_MELEE;
    if ( is_tank   ) return SET_T16_TANK;
    if ( is_heal   ) return SET_T16_HEAL;
  }

  if ( strstr( s, "gladiators_ornamented_"  ) ) return SET_PVP_HEAL;
  if ( strstr( s, "gladiators_scaled_"      ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// paladin_t::init_buffs ====================================================

void paladin_t::create_buffs()
{
  player_t::create_buffs();

  // Glyphs
  buffs.alabaster_shield       = buff_creator_t( this, "alabaster_shield", find_spell( 121467 ) ) // alabaster shield glyph spell contains no useful data
                                 .cd( timespan_t::zero() );
  buffs.bastion_of_glory       = buff_creator_t( this, "bastion_of_glory", find_spell( 114637 ) );
  buffs.bastion_of_power       = buff_creator_t( this, "bastion_of_power", find_spell( 144569 ) );
  buffs.bladed_armor           = buff_creator_t( this, "bladed_armor", find_specialization_spell( "Bladed Armor" ) )
                                 .add_invalidate( CACHE_ATTACK_POWER );
  buffs.blessed_life           = buff_creator_t( this, "glyph_blessed_life", glyphs.blessed_life )
                                 .cd( timespan_t::from_seconds( glyphs.blessed_life -> effectN( 2 ).base_value() ) );
  buffs.double_jeopardy        = buff_creator_t( this, "glyph_double_jeopardy", glyphs.double_jeopardy )
                                 .duration( find_spell( glyphs.double_jeopardy -> effectN( 1 ).trigger_spell_id() ) -> duration() )
                                 .default_value( find_spell( glyphs.double_jeopardy -> effectN( 1 ).trigger_spell_id() ) -> effectN( 1 ).percent() );
  buffs.glyph_templars_verdict       = buff_creator_t( this, "glyph_templars_verdict", glyphs.templars_verdict )
                                       .duration( find_spell( glyphs.templars_verdict -> effectN( 1 ).trigger_spell_id() ) -> duration() )
                                       .default_value( find_spell( glyphs.templars_verdict -> effectN( 1 ).trigger_spell_id() ) -> effectN( 1 ).percent() );

  buffs.glyph_of_word_of_glory = buff_creator_t( this, "glyph_word_of_glory", spells.glyph_of_word_of_glory ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Talents
  buffs.divine_purpose         = buff_creator_t( this, "divine_purpose", find_talent_spell( "Divine Purpose" ) )
                                 .duration( find_spell( find_talent_spell( "Divine Purpose" ) -> effectN( 1 ).trigger_spell_id() ) -> duration() );
  buffs.holy_avenger           = buff_creator_t( this, "holy_avenger", find_talent_spell( "Holy Avenger" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD
  buffs.sacred_shield          = buff_creator_t( this, "sacred_shield", find_talent_spell( "Sacred Shield" ) )
                                 .duration( timespan_t::from_seconds( 60 ) ); // arbitrarily high since this is just a placeholder, we expire() on last_tick()
  buffs.selfless_healer        = buff_creator_t( this, "selfless_healer", find_spell( 114250 ) );

  // General
  buffs.avenging_wrath         = new buffs::avenging_wrath_buff_t( this );
  buffs.divine_protection      = new buffs::divine_protection_t( this );
  buffs.divine_shield          = buff_creator_t( this, "divine_shield", find_class_spell( "Divine Shield" ) )
                                 .cd( timespan_t::zero() ) // Let the ability handle the CD
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.hand_of_purity         = buff_creator_t( this, "hand_of_purity", find_talent_spell( "Hand of Purity" ) ).cd( timespan_t::zero() ); // Let the ability handle the CD

  // Holy
  buffs.daybreak               = buff_creator_t( this, "daybreak", find_spell( 88819 ) );
  buffs.infusion_of_light      = buff_creator_t( this, "infusion_of_light", find_spell( 54149 ) );
  buffs.enhanced_holy_shock    = buff_creator_t( this, "enhanced_holy_shock", find_spell( 160002 ) )
                                 .chance( find_spell( 157478 ) -> proc_chance() );

  // Prot
  buffs.guardian_of_ancient_kings      = buff_creator_t( this, "guardian_of_ancient_kings", find_specialization_spell( "Guardian of Ancient Kings" ) )
                                          .cd( timespan_t::zero() ); // let the ability handle the CD
  buffs.grand_crusader                 = buff_creator_t( this, "grand_crusader" ).spell( passives.grand_crusader -> effectN( 1 ).trigger() ).chance( passives.grand_crusader -> proc_chance() );
  buffs.shield_of_the_righteous        = buff_creator_t( this, "shield_of_the_righteous" ).spell( find_spell( 132403 ) );
  buffs.ardent_defender                = buff_creator_t( this, "ardent_defender" ).spell( find_specialization_spell( "Ardent Defender" ) );
  buffs.shield_of_glory                = buff_creator_t( this, "shield_of_glory" ).spell( find_spell( 138242 ) );

  // Ret
  buffs.tier15_2pc_melee       = buff_creator_t( this, "tier15_2pc_melee", find_spell( 138162 ) )
                                 .default_value( find_spell( 138162 ) -> effectN( 1 ).percent() )
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.tier15_4pc_melee       = buff_creator_t( this, "tier15_4pc_melee", find_spell( 138164 ) )
                                 .chance( find_spell( 138164 ) -> effectN( 1 ).percent() );
  buffs.warrior_of_the_light   = buff_creator_t( this, "warrior_of_the_light", find_spell( 144587 ) )
                                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.divine_crusader        = buff_creator_t( this, "divine_crusader", find_spell( 144595 ) )
                                 .chance( 0.25 ); // spell data errantly defines proc chance as 101%, actual proc chance nowhere to be found; should be 25%
}

// ==========================================================================
// Action Priority List Generation / Validation
// ==========================================================================
void paladin_t::generate_action_prio_list_prot()
{
  ///////////////////////
  // Precombat List
  ///////////////////////

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    flask_action += ( level > 85 ) ? "earth" : "steelskin";
    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
    food_action += ( level > 85 ) ? "chun_tian_spring_rolls" : "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Blessing of Kings", "if=(!aura.str_agi_int.up)&(aura.mastery.up)" );
  precombat -> add_action( this, "Blessing of Might", "if=!aura.mastery.up" );
  precombat -> add_action( this, "Seal of Insight" );
  precombat -> add_talent( this, "Sacred Shield" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting (disabled for now)
  /*
  if (sim -> allow_potions && level >= 80 )
    precombat -> add_action( ( level > 85 ) ? "mogu_power_potion" : "golemblood_potion" );

  */

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list( "default" );

  // potion placeholder; need to think about realistic conditions
  // def -> add_action( "potion_of_the_mountains,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60" );

  def -> add_action( "/auto_attack" );

  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action ( "/use_item,name=" + items[ i ].name_str );
    }
  }

  // profession actions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  // racial actions
  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  //def -> add_action( this, "Avenging Wrath" );
  def -> add_talent( this, "Holy Avenger" );
  def -> add_action( this, "Divine Protection" ); // use on cooldown
  def -> add_action( this, "Guardian of Ancient Kings" ); // use on cooldown
  def -> add_talent( this, "Eternal Flame", "if=buff.eternal_flame.remains<2&buff.bastion_of_glory.react>2&(holy_power>=3|buff.divine_purpose.react|buff.bastion_of_power.react)" );
  def -> add_talent( this, "Eternal Flame", "if=buff.bastion_of_power.react&buff.bastion_of_glory.react>=5" );
  // these two lines are emergency WoG, but seem to do more harm than good
  //def -> add_action( this, "Word of Glory", "if=buff.bastion_of_glory.react>3&incoming_damage_5s>health.max*0.8" );
  //def -> add_talent( this, "Eternal Flame", "if=buff.bastion_of_glory.react>3&incoming_damage_5s>health.max*0.8" );
  def -> add_action( this, "Shield of the Righteous", "if=holy_power>=5|buff.divine_purpose.react|incoming_damage_1500ms>=health.max*0.3" );
  //def -> add_action( this, "Judgment", "if=talent.sanctified_wrath.enabled&buff.avenging_wrath.react" );
  def -> add_action( "wait,sec=cooldown.judgment.remains,if=talent.sanctified_wrath.enabled&cooldown.judgment.remains>0&cooldown.judgment.remains<=0.5" );
  def -> add_action( this, "Crusader Strike" );
  def -> add_action( "wait,sec=cooldown.crusader_strike.remains,if=cooldown.crusader_strike.remains>0&cooldown.crusader_strike.remains<=0.5" );
  def -> add_action( this, "Judgment" );
  def -> add_action( "wait,sec=cooldown.judgment.remains,if=cooldown.judgment.remains>0&cooldown.judgment.remains<=0.5&(cooldown.crusader_strike.remains-cooldown.judgment.remains)>=0.5" );
  def -> add_action( this, "Avenger's Shield" );
  def -> add_talent( this, "Sacred Shield", "if=target.dot.sacred_shield.remains<5" );
  def -> add_action( this, "Holy Wrath" );
  def -> add_talent( this, "Execution Sentence" );
  def -> add_talent( this, "Light's Hammer" );
  def -> add_action( this, "Hammer of Wrath" );
  def -> add_action( this, "Consecration", "if=target.debuff.flying.down&!ticking" );
  def -> add_talent( this, "Holy Prism" );
  def -> add_talent( this, "Sacred Shield" );
}

void paladin_t::generate_action_prio_list_ret()
{
  ///////////////////////
  // Precombat List
  ///////////////////////

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    flask_action += ( level > 85 ) ? "winters_bite" : "titanic_strength";
    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
    food_action += ( level > 85 ) ? "black_pepper_ribs_and_shrimp" : "beer_basted_crocolisk";
    precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Blessing of Kings", "if=!aura.str_agi_int.up" );
  precombat -> add_action( this, "Blessing of Might", "if=!aura.mastery.up" );
  precombat -> add_action( this, "Seal of Truth", "if=active_enemies<4" );
  precombat -> add_action( this, "Seal of Righteousness", "if=active_enemies>=4" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting
  if ( sim -> allow_potions && level >= 80 )
    precombat -> add_action( ( level > 85 ) ? "mogu_power_potion" : "golemblood_potion" );

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list( "default" );

  // Start with Rebuke.  Because why not.
  def -> add_action( this, "Rebuke" );

  if ( sim -> allow_potions )
  {
    if ( level > 85 )
      def -> add_action( "mogu_power_potion,if=(buff.bloodlust.react|buff.avenging_wrath.up|target.time_to_die<=40)" );
    else if ( level >= 80 )
      def -> add_action( "golemblood_potion,if=buff.bloodlust.react|buff.avenging_wrath.up|target.time_to_die<=40" );
  }

  // This should<tm> get Censure up before the auto attack lands
  def -> add_action( "auto_attack" );

  /*if ( find_class_spell( "Judgment" ) -> ok() && find_specialization_spell( "Judgments of the Bold" ) -> ok() )
  {
  def -> add_action ( this, "Judgment", "if=!target.debuff.physical_vulnerability.up|target.debuff.physical_vulnerability.remains<6" );
  }*/

  // Avenging Wrath
  def -> add_action( this, "Avenging Wrath" );

  // Guardian of Ancient Kings
  def -> add_action( this, "Guardian of Ancient Kings" ); 

  // Holy Avenger
  def -> add_talent( this, "Holy Avenger", "if=holy_power<=2" ); 

  // Items (not sure why they're radomly put here? I guess after cooldowns but before rotational abilities)
  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      std::string item_str;
      item_str += "use_item,name=";
      item_str += items[ i ].name();
      def -> add_action( item_str );
    }
  }

  // profession actions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  // racial actions
  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  // Execution Sentence
  def -> add_talent( this, "Execution Sentence" );

  // Light's Hammer
  def -> add_talent( this, "Light's Hammer" );
  
  // Divine Storm
  def -> add_action( this, "Divine Storm", "if=active_enemies>=2&(holy_power=5|buff.divine_purpose.react|(buff.holy_avenger.up&holy_power>=3))" );
  
  // Divine Storm with 4T16
  def -> add_action( this, "Divine Storm", "if=buff.divine_crusader.react&holy_power=5" );

  // Templar's Verdict @ 5 HP, or if DivPurp is about to expire
  def -> add_action( this, "Templar's Verdict", "if=holy_power=5|buff.holy_avenger.up&holy_power>=3" );
  def -> add_action( this, "Templar's Verdict", "if=buff.divine_purpose.react&buff.divine_purpose.remains<4" );
  
  // Hammer of Wrath
  def -> add_action( this, "Hammer of Wrath" );
  def -> add_action( "wait,sec=cooldown.hammer_of_wrath.remains,if=cooldown.hammer_of_wrath.remains>0&cooldown.hammer_of_wrath.remains<=0.2" );
    
  // Divine Storm with 4T16 & AW
  def -> add_action( this, "Divine Storm", "if=buff.divine_crusader.react&buff.avenging_wrath.up" );
  
  // Templar's Verdict if AW is up
  def -> add_action( this, "Templar's Verdict", "if=buff.avenging_wrath.up");

  // HotR/CS > Mexo > J
  def -> add_action( this, "Hammer of the Righteous", "if=active_enemies>=4" );
  def -> add_action( this, "Crusader Strike" );
  def -> add_action( "wait,sec=cooldown.crusader_strike.remains,if=cooldown.crusader_strike.remains>0&cooldown.crusader_strike.remains<=0.2" );
  def -> add_action( this, "Exorcism", "if=active_enemies>=2&active_enemies<=4&set_bonus.tier15_2pc_melee&glyph.mass_exorcism.enabled" );
  def -> add_action( this, "Judgment" );
  def -> add_action( "wait,sec=cooldown.judgment.remains,if=cooldown.judgment.remains>0&cooldown.judgment.remains<=0.2" );

  // Divine Storm with 4T16 
  def -> add_action( this, "Divine Storm", "if=buff.divine_crusader.react" );

  // Templar's Verdict w/ Divine Purpose
  def -> add_action( this, "Templar's Verdict", "if=buff.divine_purpose.react" );

  // Exo > TV4T15 > DS > TV > HPr
  def -> add_action( this, "Exorcism" );
  def -> add_action( "wait,sec=cooldown.exorcism.remains,if=cooldown.exorcism.remains>0&cooldown.exorcism.remains<=0.2" );
  def -> add_action( this, "Templar's Verdict", "if=buff.tier15_4pc_melee.up&active_enemies<4" );
  def -> add_action( this, "Divine Storm", "if=active_enemies>=2" );
  def -> add_action( this, "Templar's Verdict" );
  def -> add_talent( this, "Holy Prism" );
}

void paladin_t::generate_action_prio_list_holy()
{
  // currently unsupported

  //precombat first
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    flask_action += ( level > 85 ) ? "warm_sun" : "draconic_mind";
    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
    food_action += ( level > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Blessing of Kings", "if=(!aura.str_agi_int.up)&(aura.mastery.up)" );
  precombat -> add_action( this, "Blessing of Might", "if=!aura.mastery.up" );
  precombat -> add_action( this, "Seal of Insight" );
  // Beacon probably goes somewhere here?

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting (disabled for now)
  /*
  if (sim -> allow_potions && level >= 80 )
    precombat -> add_action( ( level > 85 ) ? "jade_serpent_potion" : "volcanic_potion" );

  */

  // action priority list
  action_priority_list_t* def = get_action_priority_list( "default" );

  // Potions
  if ( sim -> allow_potions )
    def -> add_action( "mana_potion,if=mana.pct<=75" );

  def -> add_action( "/auto_attack" );

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
  def -> add_action( this, "Avenging Wrath" );
  def -> add_action( this, "Guardian of Ancient Kings" );
  def -> add_action( this, "Word of Glory", "if=holy_power>=3" );
  def -> add_action( this, "Holy Shock", "if=holy_power<=3" );
  def -> add_action( this, "Flash of Light", "if=target.health.pct<=30" );
  def -> add_action( this, "Divine Plea", "if=mana_pct<75" );
  def -> add_action( this, "Holy Light" );

}

void paladin_t::validate_action_priority_list()
{

  for ( size_t alist = 0; alist < action_priority_list.size(); alist++ )
  {
    action_priority_list_t* a = action_priority_list[ alist ];

    // Convert old style action list to new style, all lines are without comments
    // stolen from player_t::init_actions() in sc_player.cpp
    if ( ! action_priority_list[ alist ] -> action_list_str.empty() )
    {
      // convert old string style to vectorized format
      std::vector<std::string> splits = util::string_split( action_priority_list[ alist ] -> action_list_str, "/" );
      for ( size_t i = 0; i < splits.size(); i++ )
        action_priority_list[ alist ] -> action_list.push_back( action_priority_t( splits[ i ], "" ) );

      // set action_list_str to empty to avoid assert() error in player_t::init_actions()
      action_priority_list[ alist ] -> action_list_str.clear();

    }

    // This section performs some validation on hand-written APLs.  The ones created in 
    // generate_action_prio_list_spec() should automatically avoid all of these mistakes.
    // In most cases, this will spit out a warning to inform the user that something was ignored.
    // For WoG w/EF talented, it will try to correct the error.

    for ( size_t i = 0; i < a -> action_list.size(); i++ )
    {
      std::string& action_str = a -> action_list[ i ].action_;
      std::size_t found_position;
      std::vector<std::string> splits = util::string_split( action_str, "," );

      // Check for EF/WoG mistakes
      if ( splits[ 0 ] == "word_of_glory" && talents.eternal_flame -> ok() )
      {
          found_position = action_str.find( splits[ 0 ].c_str() );
          action_str.replace( found_position, splits[ 0 ].length(), "eternal_flame" );
          sim -> errorf( "Action priority list contains Word of Glory with Eternal Flame talent, automatically replacing WoG with EF\n" );
      }
      // Check for usage of talents without talent present
      if ( ( splits[ 0 ] == "holy_prism" && ! talents.holy_prism -> ok() ) || 
           ( splits[ 0 ] == "lights_hammer" && ! talents.lights_hammer -> ok() ) || 
           ( splits[ 0 ] == "execution_sentence" && ! talents.execution_sentence -> ok() ) ||
           ( splits[ 0 ] == "sacred_shield" && ! talents.sacred_shield -> ok() ) ||
           ( splits[ 0 ] == "hand_of_purity" && ! talents.hand_of_purity -> ok() ) ||
           ( splits[ 0 ] == "holy_avenger" && ! talents.holy_avenger -> ok() ) ||
           ( splits[ 0 ] == "eternal_flame" && ! talents.eternal_flame -> ok() ) )
      {
        std::string talent_str = "talent.";
        talent_str += splits[ 0 ].c_str();
        talent_str += ".enabled";
        if ( ! util::str_in_str_ci( action_str, talent_str ) )
          sim -> errorf( "Action priority list contains %s without talent, ignoring.", splits[ 0 ].c_str() );
      }
    }
  }
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

  active_hand_of_light_proc          = new hand_of_light_proc_t         ( this );

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
        generate_action_prio_list_holy(); // HOLY
        break;
      default:
        if ( level > 80 )
        {
          action_list_str = "flask,type=draconic_mind/food,type=severed_sagefish_head";
          action_list_str += "/volcanic_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
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


  validate_action_priority_list(); // this checks for conflicts and cleans up APL accordingly

  player_t::init_action_list();
}

void paladin_t::init_spells()
{
  player_t::init_spells();

  // Talents
  talents.hand_of_purity          = find_talent_spell( "Hand of Purity" );
  talents.unbreakable_spirit      = find_talent_spell( "Unbreakable Spirit" );
  talents.clemency                = find_talent_spell( "Clemency" );
  talents.selfless_healer         = find_talent_spell( "Selfless Healer" );
  talents.eternal_flame           = find_talent_spell( "Eternal Flame" );
  talents.sacred_shield           = find_talent_spell( "Sacred Shield" );
  talents.holy_avenger            = find_talent_spell( "Holy Avenger" );
  talents.sanctified_wrath        = find_talent_spell( "Sanctified Wrath" );
  talents.divine_purpose          = find_talent_spell( "Divine Purpose" );
  talents.holy_prism              = find_talent_spell( "Holy Prism" );
  talents.lights_hammer           = find_talent_spell( "Light's Hammer" );
  talents.execution_sentence      = find_talent_spell( "Execution Sentence" );
  talents.empowered_seals         = find_talent_spell( "Empowered Seals" );
  talents.seraphim                = find_talent_spell( "Seraphim" );
  talents.holy_shield             = find_talent_spell( "Holy Shield" );
  talents.final_verdict           = find_talent_spell( "Final Verdict" );
  talents.beacon_of_faith         = find_talent_spell( "Beacon of Faith" );
  talents.beacon_of_insight       = find_talent_spell( "Beacon of Insight" );
  talents.saved_by_the_light      = find_talent_spell( "Saved by the Light" );

  // Spells
  spells.holy_light                    = find_specialization_spell( "Holy Light" );
  spells.sanctified_wrath              = find_spell( 114232 );  // spec-specific effects for Sanctified Wrath

  // Masteries
  passives.divine_bulwark         = find_mastery_spell( PALADIN_PROTECTION );
  passives.hand_of_light          = find_mastery_spell( PALADIN_RETRIBUTION );
  passives.illuminated_healing    = find_mastery_spell( PALADIN_HOLY );
  
  // Passives

  // Shared Passives
  passives.boundless_conviction   = find_spell( 115675 ); // find_specialization_spell( "Boundless Conviction" ); FIX-ME: (not in our spell lists for some reason)
  passives.plate_specialization   = find_specialization_spell( "Plate Specialization" );
  passives.sanctity_of_battle     = find_spell( 25956 ); // FIX-ME: find_specialization_spell( "Sanctity of Battle" )   (not in spell lists yet)
  passives.seal_of_insight        = find_class_spell( "Seal of Insight" );

  // Holy Passives
  passives.daybreak               = find_specialization_spell( "Daybreak" );
  passives.holy_insight           = find_specialization_spell( "Holy Insight" );
  passives.infusion_of_light      = find_specialization_spell( "Infusion of Light" );

  // Prot Passives
  passives.judgments_of_the_wise  = find_specialization_spell( "Judgments of the Wise" );
  passives.guarded_by_the_light   = find_specialization_spell( "Guarded by the Light" );
  passives.vengeance              = find_specialization_spell( "Vengeance" );
  passives.sanctuary              = find_specialization_spell( "Sanctuary" );
  passives.shining_protector      = find_specialization_spell( "Shining Protector" );
  passives.grand_crusader         = find_specialization_spell( "Grand Crusader" );

  // Ret Passives
  passives.judgments_of_the_bold  = find_specialization_spell( "Judgments of the Bold" );
  passives.sword_of_light         = find_specialization_spell( "Sword of Light" );
  passives.sword_of_light_value   = find_spell( passives.sword_of_light -> ok() ? 20113 : 0 );
  passives.the_art_of_war         = find_specialization_spell( "The Art of War" );

  // Perks
  // Multiple
  perk.improved_crusader_strike   = find_perk_spell( "Improved Crusader Strike" );
  perk.improved_divine_protection = find_perk_spell( "Improved Divine Protection" );
  perk.improved_flash_of_light    = find_perk_spell( "Improved Flash of Light" );
  perk.improved_forbearance       = find_perk_spell( "Improved Forbearance" );
  perk.improved_judgment          = find_perk_spell( "Improved Judgment" );

  // Holy Perks
  perk.empowered_beacon_of_light  = find_perk_spell( "Empowered Beacon of Light" );
  perk.empowered_holy_shock       = find_perk_spell( "Empwowered Holy Shock" );
  perk.enhanced_holy_shock        = find_perk_spell( "Enhanced Holy Shock" );
  perk.improved_daybreak          = find_perk_spell( "Improved Daybreak" );
  perk.improved_denounce          = find_perk_spell( "Improved Denounce" );
  perk.improved_holy_light        = find_perk_spell( "Improved Holy Light" );

  // Prot Perks
  perk.empowered_avengers_shield  = find_perk_spell( "Empowered Avenger's Shield" );
  perk.improved_block             = find_perk_spell( "Improved Block" );
  perk.improved_consecration      = find_perk_spell( "Improved Consecration" );
  perk.improved_hammer_of_the_righteous = find_perk_spell( "Improved Hammer of the Righteous" );
  perk.improved_word_of_glory     = find_perk_spell( "Improved Word of Glory" );

  // Ret Perks
  perk.empowered_hammer_of_wrath  = find_perk_spell( "Empowered Hammer of Wrath" );
  perk.empowered_seal_of_truth    = find_perk_spell( "Empowered Seal of Truth" );
  perk.enhanced_hand_of_sacrifice = find_perk_spell( "Enhanced Hand of Sacrifice" );
  perk.improved_exorcism          = find_perk_spell( "Improved Exorcism" );
  
  // Glyphs
  glyphs.alabaster_shield         = find_glyph_spell( "Glyph of the Alabaster Shield" );
  glyphs.battle_healer            = find_glyph_spell( "Glyph of the Battle Healer" );
  glyphs.blessed_life             = find_glyph_spell( "Glyph of Blessed Life" );
  glyphs.devotion_aura            = find_glyph_spell( "Glyph of Devotion Aura" );
  glyphs.divine_shield            = find_glyph_spell( "Glyph of Divine Shield" );
  glyphs.divine_protection        = find_glyph_spell( "Glyph of Divine Protection" );
  glyphs.divine_storm             = find_glyph_spell( "Glyph of Divine Storm" );
  glyphs.double_jeopardy          = find_glyph_spell( "Glyph of Double Jeopardy" );
  glyphs.flash_of_light           = find_glyph_spell( "Glyph of Flash of Light" );
  glyphs.final_wrath              = find_glyph_spell( "Glyph of Final Wrath" );
  glyphs.focused_wrath            = find_glyph_spell( "Glyph of Focused Wrath" );
  glyphs.focused_shield           = find_glyph_spell( "Glyph of Focused Shield" );
  glyphs.hand_of_sacrifice        = find_glyph_spell( "Glyph of Hand of Sacrifice" );
  glyphs.harsh_words              = find_glyph_spell( "Glyph of Harsh Words" );
  glyphs.immediate_truth          = find_glyph_spell( "Glyph of Immediate Truth" );
  glyphs.mass_exorcism            = find_glyph_spell( "Glyph of Mass Exorcism" );
  glyphs.templars_verdict         = find_glyph_spell( "Glyph of Templar's Verdict" );
  glyphs.word_of_glory            = find_glyph_spell( "Glyph of Word of Glory"   );
  
  // more spells, these need the glyph check to be present before they can be executed
  spells.alabaster_shield              = glyphs.alabaster_shield -> ok() ? find_spell( 121467 ) : spell_data_t::not_found(); // this is the spell containing Alabaster Shield's effects
  spells.glyph_of_word_of_glory        = glyphs.word_of_glory -> ok() ? find_spell( 115522 ) : spell_data_t::not_found();


  if ( specialization() == PALADIN_RETRIBUTION )
  {
    extra_regen_period  = passives.sword_of_light -> effectN( 2 ).period();
    extra_regen_percent = passives.sword_of_light -> effectN( 2 ).percent();
  }
  else if ( specialization() == PALADIN_PROTECTION )
  {
    extra_regen_period  = passives.guarded_by_the_light -> effectN( 3 ).period();
    extra_regen_percent = passives.guarded_by_the_light -> effectN( 3 ).percent();
  }

  if ( find_class_spell( "Beacon of Light" ) -> ok() )
    active_beacon_of_light = new beacon_of_light_heal_t( this );

  if ( passives.illuminated_healing -> ok() )
    active_illuminated_healing = new illuminated_healing_t( this );

  if ( passives.shining_protector -> ok() )
    active_shining_protector_proc = new shining_protector_t( this );

  if ( talents.holy_shield -> ok() )
    active_holy_shield_proc = new holy_shield_proc_t( this );

  // TODO: check if this benefit is only for the paladin (as coded) or for all targets
  debuffs.forbearance -> buff_duration += timespan_t::from_millis( perk.improved_forbearance -> effectN( 1 ).base_value() );

  // Tier Bonuses
  static const set_bonus_description_t set_bonuses =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P     H2P     H4P
    {     0,     0, 105765, 105820, 105800, 105744, 105743, 105798 }, // Tier13
    {     0,     0, 123108,  70762, 123104, 123107, 123102, 123103 }, // Tier14
    {     0,     0, 138159, 138164, 138238, 138244, 138291, 138292 }, // Tier15
    {     0,     0, 144586, 144593, 144566, 144580, 144625, 144613 }, // Tier16
  };

  sets.register_spelldata( set_bonuses );

  // Holy Mastery uses effect#2 by default
  if ( specialization() == PALADIN_HOLY )
  {
    _mastery = &find_mastery_spell( specialization() ) -> effectN( 2 );
  }
}

// paladin_t::primary_role ==================================================

role_e paladin_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_DPS || specialization() == PALADIN_RETRIBUTION )
    return ROLE_HYBRID;

  if ( player_t::primary_role() == ROLE_TANK || specialization() == PALADIN_PROTECTION  )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_HEAL || specialization() == PALADIN_HOLY )
    return ROLE_HEAL; 

  return ROLE_HYBRID;
}

// paladin_t::convert_hybrid_stat ===========================================
stat_e paladin_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  // Guess at how AGI/INT mail or leather will be handled for plate - probably INT?
  case STAT_AGI_INT: 
    return STAT_INTELLECT;
  // This is a guess at how AGI/STR gear will work for Holy, TODO: confirm  
  case STAT_STR_AGI:
    return STAT_STRENGTH;
  case STAT_STR_INT:
    if ( specialization() == PALADIN_HOLY )
      return STAT_INTELLECT;
    else
      return STAT_STRENGTH;
  case STAT_SPIRIT:
    if ( specialization() == PALADIN_HOLY )
      return s;
    else
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
    if ( specialization() == PALADIN_PROTECTION )
      return s;
    else
      return STAT_NONE;
  default:
    return s; 
  }
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

double paladin_t::composite_melee_expertise( weapon_t* w ) const
{
  double expertise = player_t::composite_melee_expertise( w );

  if ( passives.sanctuary -> ok() )
    expertise += passives.sanctuary -> effectN( 4 ).percent();

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
  
  return h;
}

// paladin_t::composite_player_multiplier ===================================

double paladin_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  // These affect all damage done by the paladin
  // Avenging Wrath buffs everything
  if ( buffs.avenging_wrath -> check() )
    m *= 1.0 + buffs.avenging_wrath -> get_damage_mod();

  // T15_2pc_melee buffs holy damage only
  if ( dbc::is_school( school, SCHOOL_HOLY ) )
  {
    m *= 1.0 + buffs.tier15_2pc_melee -> value();
  }

  // Divine Shield reduces everything
  if ( buffs.divine_shield -> up() )
    m *= 1.0 + buffs.divine_shield -> data().effectN( 1 ).percent();

  // Glyph of Word of Glory buffs everything
  if ( buffs.glyph_of_word_of_glory -> check() )
    m *= 1.0 + buffs.glyph_of_word_of_glory -> value();

  // T16_2pc_melee buffs everything
  if ( sets.has_set_bonus( SET_T16_2PC_MELEE ) && buffs.warrior_of_the_light -> up() )
    m *= 1.0 + buffs.warrior_of_the_light -> data().effectN( 1 ).percent();

  return m;
}

// paladin_t::composite_player_heal_multiplier ==============================

double paladin_t::composite_player_heal_multiplier( school_e s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  if ( buffs.avenging_wrath -> check() )
    m *= 1.0 + buffs.avenging_wrath -> get_healing_mod();

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

  ap += buffs.bladed_armor -> data().effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// paladin_t::composite_attack_power_multiplier =============================

double paladin_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_spell_power_multiplier();

  if ( passives.divine_bulwark -> ok () )
    ap += get_divine_bulwark(); // TODO: check if additive or multiplicative

  return ap;
}

// paladin_t::composite_spell_power_multiplier ==============================

double paladin_t::composite_spell_power_multiplier() const
{
  if ( passives.sword_of_light -> ok() || passives.guarded_by_the_light -> ok() )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// paladin_t::composite_tank_block ==========================================

double paladin_t::composite_block() const
{
  // need to reproduce most of player_t::composite_block() because mastery -> block conversion is affected by DR
  // could modify player_t::composite_block() to take one argument if willing to change references in all other files
  double block_by_rating = current.stats.block_rating / current_rating().block;

  // add block from Divine Bulwark
  block_by_rating += get_divine_bulwark();

  double b = current.block;

  // calculate diminishing returns and add to b
  if ( block_by_rating > 0 )
  {
    //the block by rating gets rounded because that's how blizzard rolls...
    b += 1 / ( 1 / diminished_block_cap + diminished_kfactor / ( util::round( 12800 * block_by_rating ) / 12800 ) );
  }

  // Guarded by the Light block not affected by diminishing returns
  b += passives.guarded_by_the_light -> effectN( 6 ).percent();

  // Improved Block perk (assuming for now that it's not affected by DR
  b += perk.improved_block -> effectN( 1 ).percent();

  // Holy Shield (assuming for now that it's not affected by DR)
  b += talents.holy_shield -> effectN( 1 ).percent();

  return b;
}

double paladin_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  // Guarded by the Light grants -6% crit chance
  c += passives.guarded_by_the_light -> effectN( 5 ).percent();

  return c;
}

double paladin_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  // add Sanctuary dodge
  d += passives.sanctuary -> effectN( 3 ).percent();

  return d;
}

// paladin_t::target_mitigation =============================================

void paladin_t::target_mitigation( school_e school,
                                   dmg_e    dt,
                                   action_state_t* s )
{
  player_t::target_mitigation( school, dt, s );

  // various mitigation effects, Ardent Defender goes last due to absorb/heal mechanics

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

  // Hand of Purity
  if ( buffs.hand_of_purity -> up() )
  {
    if ( s -> result_type == DMG_OVER_TIME )
    {
      s -> result_amount *= 1.0 - buffs.hand_of_purity -> data().effectN( 1 ).percent(); // for some reason, the DoT reduction is stored as +0.7
    }
    else
    {
      s -> result_amount *= 1.0 + buffs.hand_of_purity -> data().effectN( 2 ).percent();
    }
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Hand of Purity is %f", s -> target -> name(), s -> result_amount );
  }

  // Divine Protection
  if ( buffs.divine_protection -> up() )
  {
    if ( util::school_type_component( school, SCHOOL_MAGIC ) )
    {
      s -> result_amount *= 1.0 + buffs.divine_protection -> data().effectN( 1 ).percent() + glyphs.divine_protection -> effectN( 1 ).percent() + perk.improved_divine_protection -> effectN( 1 ).percent();
    }
    else
    {
      s -> result_amount *= 1.0 + buffs.divine_protection -> data().effectN( 2 ).percent() + glyphs.divine_protection -> effectN( 2 ).percent();
    }
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Divine Protection is %f", s -> target -> name(), s -> result_amount );
  }

  // Glyph of Templar's Verdict
  if ( buffs.glyph_templars_verdict -> check() )
  {
    s -> result_amount *= 1.0 + buffs.glyph_templars_verdict -> value();
    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after Glyph of TV is %f", s -> target -> name(), s -> result_amount );
  }

  // Shield of the Righteous
  if ( buffs.shield_of_the_righteous -> check() && school == SCHOOL_PHYSICAL )
  {
    s -> result_amount *= 1.0 + ( buffs.shield_of_the_righteous -> data().effectN( 1 ).percent() - get_divine_bulwark() ) * ( 1.0 + sets.set( SET_T14_4PC_TANK ) -> effectN( 2 ).percent() );

    if ( sim -> debug && s -> action && ! s -> target -> is_enemy() && ! s -> target -> is_add() )
      sim -> out_debug.printf( "Damage to %s after SotR mitigation is %f", s -> target -> name(), s -> result_amount );
  }

  // Ardent Defender
  if ( buffs.ardent_defender -> check() )
  {
    s -> result_amount *= 1.0 - buffs.ardent_defender -> data().effectN( 1 ).percent();

    if ( s -> result_amount > 0 && s -> result_amount >= resources.current[ RESOURCE_HEALTH ] )
    {
      // Ardent defender is a little odd - it doesn't heal you *for* 15%, it heals you *to* 15%.
      // It does this by either absorbing all damage and healing you for the difference between 15% and your current health (if current < 15%)
      // or absorbing any damage that would take you below 15% (if current > 15%).
      // To avoid complications with absorb modeling, we're just going to kludge it by adjusting the amount gained or lost accordingly.
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
        resource_gain( RESOURCE_HEALTH,
                       AD_health_threshold - resources.current[ RESOURCE_HEALTH ],
                       nullptr,
                       s -> action );
      }
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

  if ( ( passives.sword_of_light -> ok() || passives.guarded_by_the_light -> ok() )
       && ( c == CACHE_STRENGTH || c == CACHE_ATTACK_POWER )
     )
  {
    player_t::invalidate_cache( CACHE_SPELL_POWER );
  }

  if ( c == CACHE_MASTERY && passives.divine_bulwark -> ok() )
    player_t::invalidate_cache( CACHE_BLOCK );
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

  // On a block event, trigger Alabaster Shield & Holy Shield
  if ( s -> block_result == BLOCK_RESULT_BLOCKED )
  {
    if ( glyphs.alabaster_shield -> ok() )
      buffs.alabaster_shield -> trigger();
    
    trigger_holy_shield();
  }

  // Also trigger Grand Crusader on an avoidance event
  if ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY )
  {
    trigger_grand_crusader();
  }

  player_t::assess_damage( school, dtype, s );

  // T15 4-piece tank
  if ( sets.has_set_bonus( SET_T15_4PC_TANK ) && buffs.divine_protection -> check() )
  {
    // compare damage to player health to find HP gain
    double hp_gain = std::floor( s -> result_mitigated / resources.max[ RESOURCE_HEALTH ] * 5 );

    // add that much Holy Power
    resource_gain( RESOURCE_HOLY_POWER, hp_gain, gains.hp_t15_4pc_tank );
  }

  // T16 2-piece tank
  if ( sets.has_set_bonus( SET_T16_2PC_TANK ) && buffs.divine_protection -> check() )
  {
    active.blessing_of_the_guardians -> increment_damage( s -> result_mitigated ); // uses post-mitigation, pre-absorb value
  }
}

// paladin_t::assess_heal ===================================================

void paladin_t::assess_heal( school_e school, dmg_e dmg_type, action_state_t* s )
{
  // 20% healing increase due to Sanctified Wrath during Avenging Wrath
  if ( specialization() == PALADIN_PROTECTION && talents.sanctified_wrath -> ok() && buffs.avenging_wrath -> check() )
  {
    s -> result_amount *= 1.0 + spells.sanctified_wrath  -> effectN( 4 ).percent();
  }

  // Shining Protector procs a heal every now and again
  if ( passives.shining_protector -> ok() )
    trigger_shining_protector( s );

  player_t::assess_heal( school, dmg_type, s );
}

// paladin_t::create_options ================================================

void paladin_t::create_options()
{
  player_t::create_options();

  option_t paladin_options[] =
  {
    opt_null()
  };

  option_t::copy( options, paladin_options );
}

// paladin_t::combat_begin ==================================================

void paladin_t::combat_begin()
{
  player_t::combat_begin();

  resources.current[ RESOURCE_HOLY_POWER ] = 0;

  if ( passives.vengeance -> ok() )
    vengeance_start();

  if ( find_specialization_spell( "Bladed Armor" ) )
    buffs.bladed_armor -> trigger();
}

// paladin_t::holy_power_stacks =============================================

int paladin_t::holy_power_stacks() const
{
  if ( buffs.divine_purpose -> check() )
  {
    return std::min( ( int ) 3, ( int ) resources.current[ RESOURCE_HOLY_POWER ] );
  }
  return ( int ) resources.current[ RESOURCE_HOLY_POWER ];
}

// paladin_t::get_divine_bulwark ============================================
double paladin_t::get_divine_bulwark() const
{
  if ( ! passives.divine_bulwark -> ok() )
    return 0.0;

  // block rating, 1% per point of mastery
  return cache.mastery_value();
}

// paladin_t::get_hand_of_light =============================================

double paladin_t::get_hand_of_light()
{
  if ( specialization() != PALADIN_RETRIBUTION ) return 0.0;

  return cache.mastery_value();
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

  struct seal_expr_t : public paladin_expr_t
  {
    seal_e rt;
    seal_expr_t( const std::string& n, paladin_t& p, seal_e r ) :
      paladin_expr_t( n, p ), rt( r ) {}
    virtual double evaluate() { return paladin.active_seal == rt; }
  };

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( ( splits.size() == 2 ) && ( splits[ 0 ] == "seal" ) )
  {
    seal_e s = SEAL_NONE;

    if      ( splits[ 1 ] == "truth"         ) s = SEAL_OF_TRUTH;
    else if ( splits[ 1 ] == "insight"       ) s = SEAL_OF_INSIGHT;
    else if ( splits[ 1 ] == "none"          ) s = SEAL_NONE;
    else if ( splits[ 1 ] == "righteousness" ) s = SEAL_OF_RIGHTEOUSNESS;
    else if ( splits[ 1 ] == "justice"       ) s = SEAL_OF_JUSTICE;
    return new seal_expr_t( name_str, *this, s );
  }

  struct double_jeopardy_expr_t : public paladin_expr_t
  {
    double_jeopardy_expr_t( const std::string& n, paladin_t& p ) :
      paladin_expr_t( n, p ) {}
    virtual double evaluate() { return paladin.last_judgement_target ? (double)paladin.last_judgement_target -> actor_index : -1; }
  };

  if ( splits[ 0 ] == "last_judgment_target" )
  {
    return new double_jeopardy_expr_t( name_str, *this );
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

// PALADIN MODULE INTERFACE =================================================

struct paladin_module_t : public module_t
{
  paladin_module_t() : module_t( PALADIN ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    paladin_t* p = new paladin_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new paladin_report_t( *p ) );
    return p;
  }

  virtual bool valid() const { return true; }

  virtual void init( sim_t* sim ) const
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.beacon_of_light          = buff_creator_t( p, "beacon_of_light", p -> find_spell( 53563 ) );
      p -> buffs.illuminated_healing      = buff_creator_t( p, "illuminated_healing", p -> find_spell( 86273 ) );
      p -> buffs.hand_of_sacrifice        = new buffs::hand_of_sacrifice_t( p );
      p -> buffs.devotion_aura            = buff_creator_t( p, "devotion_aura", p -> find_spell( 31821 ) );
      p -> debuffs.forbearance            = buff_creator_t( p, "forbearance", p -> find_spell( 25771 ) );
    }
  }

  virtual void combat_begin( sim_t* ) const {}

  virtual void combat_end  ( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::paladin()
{
  static paladin_module_t m;
  return &m;
}
