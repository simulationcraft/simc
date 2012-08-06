// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//
//  TODO:
//
//  BREWMASTER: Make sure tiger palm cost doesn't double refund and costs 0.
//  Add all damaging abilities
//  Ensure values are correct
//  Add mortal wounds to RSK
//  Add all buffs
//  Remove overemphasized commenting once done with abilities
//
//  Fix Xuen Pet scaling : Melee and crackling lightning damage using guestimations, needs to be looked into.
//
//  Implement Build 15882
//  http://www.mmo-champion.com/content/2852-Mists-of-Pandaria-Beta-Build-15882
//
//  Implement Build 15851
//  http://www.mmo-champion.com/content/2846-Mists-of-Pandaria-Beta-Build-15851

#include "simulationcraft.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace { // UNNAMED NAMESPACE

// The purpose of these namespaces is to allow modern IDEs to collapse sections of code.
// Is neither intended nor desired to provide name-uniqueness, hence the global uplift.

namespace attacks {}
namespace spells {}
namespace statues {}

using namespace attacks;
using namespace spells;
using namespace statues;

struct monk_t;

enum monk_stance_e { STANCE_DRUNKEN_OX=1, STANCE_FIERCE_TIGER, STANCE_WISE_SERPENT=4 };

struct monk_td_t : public actor_pair_t
{
  struct buffs_t
  {
    debuff_t* rising_sun_kick;
    buff_t* enveloping_mist;
  } buff;

  monk_td_t( player_t*, monk_t* );
};

struct monk_t : public player_t
{
public:
  // Pets
   pet_t* pet_xuen;
  monk_stance_e active_stance;
  action_t* active_blackout_kick_dot;
  double track_chi_consumption;

  // Buffs
  struct buffs_t
  {
    // TODO: Finish Adding Buffs - will uncomment as implemented
    //  buff_t* buffs_<buffname>;
    buff_t* energizing_brew;
    buff_t* zen_sphere;
    buff_t* tiger_power;
    //  buff_t* fortifying_brew;
    //  buff_t* zen_meditation;
    //  buff_t* path_of_blossoms;
    buff_t* tigereye_brew;
    buff_t* tigereye_brew_use;
    buff_t* tiger_strikes;
    buff_t* combo_breaker_tp;
    buff_t* combo_breaker_bok;
    buff_t* tiger_stance;
    buff_t* serpent_stance;
    buff_t* chi_sphere;

    //Debuffs
  } buff;

  // Gains
  struct gains_t
  {
    gain_t* chi;
    gain_t* combo_breaker_savings;
    gain_t* energizing_brew;
    gain_t* avoided_chi;
    gain_t* chi_brew;
  } gain;
  // Stances

  // Procs
  struct procs_t
  {
    proc_t* combo_breaker_bok;
    proc_t* combo_breaker_tp;
    // proc_t* tiger_strikes;
  } proc;

  // Random Number Generation
  struct rngs_t
  {
  } rng;

  // Talents
  struct talents_t
  {
//  TODO: Implement
    //   const spell_data_t* celerity;
    //   const spell_data_t* tigers_lust;
    //   const spell_data_t* momentum;

    const spell_data_t* chi_wave;
    const spell_data_t* zen_sphere;
    const spell_data_t* chi_burst;

    const spell_data_t* power_strikes;
    const spell_data_t* ascension;
    const spell_data_t* chi_brew;

    //   const spell_data_t* deadly_reach;
    //   const spell_data_t* charging_ox_wave;
    //   const spell_data_t* leg_sweep;

    //   const spell_data_t* healing_elixers;
    //   const spell_data_t* dampen_harm;
    //   const spell_data_t* diffuse_magic;

    const spell_data_t* rushing_jade_wind;
    const spell_data_t* invoke_xuen;
    //   const spell_data_t* chi_torpedo;
  } talent;

  // Specialization
  struct specs_t
  {
    // GENERAL
    const spell_data_t* leather_specialization;
    const spell_data_t* way_of_the_monk;

    // TREE_MONK_TANK
    // spell_id_t* mastery/passive spells

    // TREE_MONK_DAMAGE

    // TREE_MONK_HEAL
  } spec;

  struct mastery_spells_t
  {
    const spell_data_t* combo_breaker; //WINDWALKER

  } mastery;

  // Glyphs
  struct glyphs_t
  {
    // Prime
    //glyph_t* <glyphname>;

    // Major

  } glyph;

  // Cooldowns
   struct cooldowns_t
   {
     cooldown_t* power_strikes;
     cooldown_t* fists_of_fury;
   } cooldowns;

  // Options
  int initial_chi;
private:
  target_specific_t<monk_td_t> target_data;
public:
  monk_t( sim_t* sim, const std::string& name, race_e r = RACE_PANDAREN ) :
    player_t( sim, MONK, name, r ),
    active_stance( STANCE_DRUNKEN_OX ),
    track_chi_consumption( 0 ),
    buff( buffs_t() ),
    gain( gains_t() ),
    proc( procs_t() ),
    rng( rngs_t() ),
    talent( talents_t() ),
    spec( specs_t() ),
    mastery( mastery_spells_t() ),
    glyph( glyphs_t() ),
    cooldowns( cooldowns_t() ),
    initial_chi( 0 )
  {
    target_data.init( "target_data", this );

    cooldowns.power_strikes = get_cooldown( "power_strikes" );
  }

  // Character Definition
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual double    composite_player_multiplier( school_e school, action_t* a );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      regen( timespan_t periodicity );
  virtual void      init_resources( bool force=false );
  virtual void      reset();
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual int       decode_set( item_t& );
  virtual void      create_options();
  virtual resource_e primary_resource();
  virtual role_e    primary_role();
  virtual void      pre_analyze_hook();




  virtual monk_td_t* get_target_data( player_t* target )
  {
    monk_td_t*& td = target_data[ target ];
    if ( ! td ) td = new monk_td_t( target, this );
    return td;
  }

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( specialization() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( specialization() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_glyphs();
  }
};

// ==========================================================================
// Monk Abilities
// ==========================================================================

// Template for common monk action code. See priest_action_t.
template <class Base>
struct monk_action_t : public Base
{
  int stancemask;

  typedef Base ab; // action base, eg. spell_t
  typedef monk_action_t base_t;

  monk_action_t( const std::string& n, monk_t* player,
                 const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_WISE_SERPENT )
  {
    ab::may_crit   = true;
  }

  monk_t* p() const { return static_cast<monk_t*>( ab::player ); }

  monk_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : ab::target ); }

  virtual bool ready()
  {
    if ( ! ab::ready() )
      return false;

    // Attack available in current stance?
    if ( ( stancemask & p() -> active_stance ) == 0 )
      return false;

    return true;
  }

  virtual resource_e current_resource()
  {
    if ( p() -> buff.tiger_stance -> data().ok() && ab::data().powerN( POWER_MONK_ENERGY ).aura_id() == 103985 )
    {
      if ( p() -> active_stance == STANCE_FIERCE_TIGER )
        return RESOURCE_ENERGY;
    }
    if ( p() -> buff.serpent_stance -> data().ok() && ab::data().powerN( POWER_MANA ).aura_id() == 115070 )
    {
      if ( p() -> active_stance == STANCE_WISE_SERPENT )
        return RESOURCE_MANA;
    }
    return ab::current_resource();
  }

  virtual void consume_resource()
  {
    ab::consume_resource();

    // Track Chi Consumption
    if ( ab::current_resource() == RESOURCE_CHI )
    {
      p() -> track_chi_consumption += ab::resource_consumed;
    }

    // If Accumulated Chi consumption is greater than 4, reduce it by 4 and trigger a Tigereye Brew stack.
    if ( p() -> track_chi_consumption >= 4 )
    {
      p() -> track_chi_consumption -= 4;

      p() -> buff.tigereye_brew -> trigger();
    }
    // Chi Savings on Dodge & Parry
    if ( ab::current_resource() == RESOURCE_CHI && ab::resource_consumed > 0 && ! ab::aoe && ( ab::execute_state -> result == RESULT_DODGE || ab::execute_state -> result == RESULT_PARRY ) )
    {
      double chi_restored = ab::resource_consumed;
      p() -> resource_gain( RESOURCE_CHI, chi_restored, p() -> gain.avoided_chi );
    }
  }
};

namespace attacks {

struct monk_melee_attack_t : public monk_action_t<melee_attack_t>
{
  weapon_t* mh;
  weapon_t* oh;
  double chi_sphere;

  monk_melee_attack_t( const std::string& n, monk_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ),
    mh( NULL ), oh( NULL )
  {
    may_glance = false;
  }

  virtual double swing_haste()
  {
    double haste = base_t::swing_haste();

    if ( !player -> dual_wield() )
      haste *= 1.0 / ( 1.0 + p() -> spec.way_of_the_monk -> effectN( 2 ).percent() );
    if( p() -> buff.tiger_strikes -> up())
      haste *= 1.0 / p() -> find_spell( 120273 ) -> effectN( 1 ).percent();
    return haste;
  }

  // Special Monk Attack Weapon damage collection, if the pointers mh or oh are set, instead of the classical action_t::weapon
  // Damage is divided instead of multiplied by the weapon speed, AP portion is not multiplied by weapon speed.
  // Both MH and OH are directly weaved into one damage number
  virtual double calculate_weapon_damage( double ap )
  {
    double total_dmg = 0;
    // Main Hand
    if ( mh && weapon_multiplier > 0 )
    {
      assert( mh -> slot != SLOT_OFF_HAND );

      double dmg = sim -> averaged_range( mh -> min_dmg, mh -> max_dmg ) + mh -> bonus_dmg;

      timespan_t weapon_speed  = normalize_weapon_speed  ? mh -> normalized_weapon_speed() : mh -> swing_time;

      dmg /= weapon_speed.total_seconds();

      double power_damage = weapon_power_mod * ap;

      total_dmg = dmg + power_damage;

      if ( sim -> debug )
      {
        sim -> output( "%s main hand weapon damage portion for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f pd=%.3f ap=%.3f",
                       player -> name(), name(), total_dmg, dmg, mh -> bonus_dmg, weapon_speed.total_seconds(), power_damage, ap );
      }
    }

    // Off Hand
    if ( oh && weapon_multiplier > 0 )
    {
      assert( oh -> slot == SLOT_OFF_HAND );

      double dmg = sim -> averaged_range( oh -> min_dmg, oh -> max_dmg ) + oh -> bonus_dmg;

      timespan_t weapon_speed  = normalize_weapon_speed  ? oh -> normalized_weapon_speed() : oh -> swing_time;

      dmg /= weapon_speed.total_seconds();

      // OH penalty
      if ( oh -> slot == SLOT_OFF_HAND )
        dmg *= 0.5;

      total_dmg += dmg;

      if ( sim -> debug )
      {
        sim -> output( "%s off-hand weapon damage portion for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f ap=%.3f",
                       player -> name(), name(), total_dmg, dmg, oh -> bonus_dmg, weapon_speed.total_seconds(), ap );
      }
    }

    if ( !mh && !oh )
      total_dmg += base_t::calculate_weapon_damage( ap );

    return total_dmg;
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = base_t::composite_target_multiplier( t );

    if ( td( t ) -> buff.rising_sun_kick -> up() )
    {
      m *=  1.0 + td( t ) -> buff.rising_sun_kick -> data().effectN( 2 ).percent();
    }

    return m;
  }
};

struct chi_sphere_t : public monk_melee_attack_t
{

  chi_sphere_t( monk_t* p, const std::string& options_str  ) :
    monk_melee_attack_t( "chi_sphere", p, spell_data_t::nil() )
  {
    parse_options( NULL, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();
    player -> resource_gain( RESOURCE_CHI, chi_sphere, p() -> gain.chi );
    p() -> buff.chi_sphere -> expire();
  }

};
//=============================
//====Jab======================
//=============================

struct jab_t : public monk_melee_attack_t
{
  jab_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "jab", p, p -> find_class_spell( "Jab" ) )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;

    base_dd_min = base_dd_max = direct_power_mod = 0.0; // deactivate parsed spelleffect1

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    base_multiplier = 1.5; // hardcoded into tooltip
  }

  virtual resource_e current_resource()
  {
    // Apparently energy requirement in Fierce Tiger stance is not in spell data
    if ( p() -> active_stance == STANCE_FIERCE_TIGER )
      return RESOURCE_ENERGY;

    return monk_melee_attack_t::current_resource();
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    // Windwalker Mastery
    // Debuffs are independent of each other
    double mastery_proc_chance = p() -> mastery.combo_breaker -> effectN( 1 ).mastery_value() * player -> composite_mastery();
    p() -> buff.combo_breaker_bok -> trigger( 1, -1, mastery_proc_chance );
    p() -> buff.combo_breaker_tp  -> trigger( 1, -1, mastery_proc_chance );

    // Chi Gain
    double chi_gain = data().effectN( 2 ).base_value();
    if ( p() -> active_stance  == STANCE_FIERCE_TIGER )
    {
      chi_gain += p() -> buff.tiger_stance -> data().effectN( 4 ).base_value();
    }
    if ( p() -> cooldowns.power_strikes -> remains() == timespan_t::zero() && p() -> talent.power_strikes  ){
      p() -> cooldowns.power_strikes -> start( timespan_t::from_seconds( p() -> find_spell( 121817 ) -> effectN( 2 ).base_value() ) );
        if ( p()-> resources.current[ RESOURCE_CHI ] < 2 ){
           chi_gain += 1.0;
        }
        else
        {
          if ( sim -> log )
                sim -> output( "Chi sphere created" );
          p() -> buff.chi_sphere -> trigger();

          chi_sphere += 1.0;
        }
    }
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi );
  }
};

//=============================
//====Tiger Palm===============
//=============================

struct tiger_palm_t : public monk_melee_attack_t
{
  tiger_palm_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "tiger_palm", p, p -> find_class_spell( "Tiger Palm" ) )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;
    base_dd_min = base_dd_max = 0.0; direct_power_mod = 0.0;//  deactivate parsed spelleffect1
    mh = &( player -> main_hand_weapon ) ;
    oh = &( player -> off_hand_weapon ) ;
    base_multiplier = 3.0; // hardcoded into tooltip
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );
// stacks, value, chance, duration
//    p() -> buff.tiger_power -> trigger(1, data().effectN(1).base_value(), data().proc_chance(), data().duration() );


//really ghetto, but until i figure out why values aren't working it'll do.
    if ( p() -> buff.tiger_power -> value() == 15)
      p() -> buff.tiger_power -> trigger(1, 15, 101, timespan_t::from_seconds( 20.0 ));
    if ( p() -> buff.tiger_power -> value() == 10)
      p() -> buff.tiger_power -> trigger(1, 15, 101, timespan_t::from_seconds( 20.0 ));
    if ( p() -> buff.tiger_power -> value() == 5)
      p() -> buff.tiger_power -> trigger(1, 10, 101, timespan_t::from_seconds( 20.0 ));
    if ( p() -> buff.tiger_power -> value() == 0)
      p() -> buff.tiger_power -> trigger(1, 5, 101, timespan_t::from_seconds( 20.0 ));
  }

  virtual double cost()
  {
    if ( p() -> buff.combo_breaker_tp -> check() )
      return 0;

    return monk_melee_attack_t::cost();
  }

  virtual void consume_resource()
  {
    monk_melee_attack_t::consume_resource();

    if ( p() -> buff.combo_breaker_tp -> up() )
    {
      p() -> buff.combo_breaker_tp -> expire();
      p() -> gain.combo_breaker_savings -> add( RESOURCE_CHI, cost() );
    }
  }
};


//=============================
//====Blackout Kick============
//=============================

struct  dot_blackout_kick_t : public ignite::pct_based_action_t< monk_melee_attack_t, monk_t >
{
  dot_blackout_kick_t( monk_t* p ) :
    base_t( "blackout_kick_dot", p, p -> find_spell ( 128531 ) )
  {
    tick_may_crit = true;
    may_miss = false;
    school = SCHOOL_PHYSICAL;
  }
};


struct blackout_kick_t : public monk_melee_attack_t
{
  void trigger_blackout_kick_dot( blackout_kick_t* s, player_t* t, double dmg )
  {
    monk_t* p = s -> p();

    ignite::trigger_pct_based(
      p -> active_blackout_kick_dot,
      t,
      dmg );
  }

  blackout_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "blackout_kick", p, p -> find_class_spell( "Blackout Kick" ) )
  {
    parse_options( 0, options_str );
    base_dd_min = base_dd_max = 0.0; direct_power_mod = 0.0; //  deactivate parsed spelleffect1
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    base_multiplier = 8.0; // hardcoded into tooltip
  }

  virtual void assess_damage( dmg_e type, action_state_t* s )
  {
    monk_melee_attack_t::assess_damage( type, s );


    trigger_blackout_kick_dot( this, s -> target, s -> result_amount * data().effectN( 2 ).percent( ) );
  }

  virtual double cost()
  {
    if ( p() -> buff.combo_breaker_bok -> check() )
      return 0;

    return monk_melee_attack_t::cost();
  }

  virtual void consume_resource()
  {
    monk_melee_attack_t::consume_resource();

    if ( p() -> buff.combo_breaker_bok -> up() )
    {
      p() -> buff.combo_breaker_bok -> expire();
      p() -> gain.combo_breaker_savings -> add( RESOURCE_CHI, cost() );
    }
  }
};

//=============================
//====RISING SUN KICK==========
//=============================

struct rising_sun_kick_t : public monk_melee_attack_t
{
  rising_sun_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "rising_sun_kick", p, p -> find_class_spell( "Rising Sun Kick" ) )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_FIERCE_TIGER;
    base_dd_min = base_dd_max = 0.0; direct_power_mod = 0.0;//  deactivate parsed spelleffect1
    mh = &( player -> main_hand_weapon ) ;
    oh = &( player -> off_hand_weapon ) ;
    base_multiplier = 14.4; // hardcoded into tooltip
  }

//TEST: Mortal Wounds - ADD Later

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    td( s -> target ) -> buff.rising_sun_kick -> trigger();
  }
};

//=============================
//====Spinning Crane Kick====== may need to modify this and fists of fury depending on how spell ticks
//=============================

struct spinning_crane_kick_t : public monk_melee_attack_t
{
  struct spinning_crane_kick_tick_t : public monk_melee_attack_t
  {
    spinning_crane_kick_tick_t( monk_t* p, const spell_data_t* s ) :
      monk_melee_attack_t( "spinning_crane_kick_tick", p, s )
    {
      background  = true;
      dual        = true;
      direct_tick = true;
      aoe = -1;
      base_dd_min = base_dd_max = 0.0; direct_power_mod = 0.0;//  deactivate parsed spelleffect1
      mh = &( player -> main_hand_weapon ) ;
      oh = &( player -> off_hand_weapon ) ;
      base_multiplier = 1.59; // hardcoded into tooltip
      school = SCHOOL_PHYSICAL;
    }
  };
  spinning_crane_kick_tick_t* spinning_crane_kick_tick;

  spinning_crane_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "spinning_crane_kick", p, p -> find_class_spell( "Spinning Crane Kick" ) ),
    spinning_crane_kick_tick( 0 )
  {
    parse_options( 0, options_str );

    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;

    may_crit = false;
    tick_zero = true;
    channeled = true;
    hasted_ticks = false;
    school = SCHOOL_PHYSICAL;

    spinning_crane_kick_tick = new spinning_crane_kick_tick_t( p, p -> find_spell( data().effectN( 1 ).trigger_spell_id() ) );
    assert( spinning_crane_kick_tick );
  }

  virtual void init()
  {
    monk_melee_attack_t::init();

    spinning_crane_kick_tick -> stats = stats;
  }

  virtual resource_e current_resource()
  {
    // Apparently energy requirement in Fierce Tiger stance is not in spell data
    if ( p() -> active_stance == STANCE_FIERCE_TIGER )
      return RESOURCE_ENERGY;

    return monk_melee_attack_t::current_resource();
  }

  virtual void tick( dot_t* d )
  {
    spinning_crane_kick_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    double chi_gain = data().effectN( 5 ).base_value();
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi );
  }
};

//=============================
//====Fists of Fury============
//=============================

struct fists_of_fury_t : public monk_melee_attack_t
{
  struct fists_of_fury_tick_t : public monk_melee_attack_t
  {
    fists_of_fury_tick_t( monk_t* p ) :
      monk_melee_attack_t( "fists_of_fury_tick", p )
    {
      background  = true;
      dual        = true;
      aoe = -1;
      base_tick_time = timespan_t::from_seconds( 1.0 );
      direct_tick = true;
      base_dd_min = base_dd_max = 0.0; direct_power_mod = 0.0;//  deactivate parsed spelleffect1
      mh = &( player -> main_hand_weapon ) ;
      oh = &( player -> off_hand_weapon ) ;
      base_multiplier = 5; // hardcoded into tooltip
      school = SCHOOL_PHYSICAL;

      cooldown = p -> cooldowns.fists_of_fury;
      cooldown -> duration += p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).time_value();
    }
  };
  fists_of_fury_tick_t* fists_of_fury_tick;

  fists_of_fury_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "fists_of_fury", p, p -> find_class_spell( "Fists of Fury" ) ),
    fists_of_fury_tick( 0 )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_FIERCE_TIGER;
    channeled = true;
    may_crit = false;
    hasted_ticks = false;
    tick_zero = true;
    school = SCHOOL_PHYSICAL;

    fists_of_fury_tick = new fists_of_fury_tick_t( p );
    assert( fists_of_fury_tick );
  }

  virtual void init()
  {
    monk_melee_attack_t::init();

    fists_of_fury_tick -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    if ( fists_of_fury_tick )
      fists_of_fury_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }
};

struct tiger_strikes_melee_attack_t : public monk_melee_attack_t
{
  tiger_strikes_melee_attack_t( const std::string& n, monk_t* p, weapon_t* w ) :
    monk_melee_attack_t( n, p, spell_data_t::nil()  )
  {
    weapon           = w;
    school           = SCHOOL_PHYSICAL;
    background       = true;
    may_glance = may_dodge = may_parry = may_miss = false;
    if ( player -> dual_wield() )
      base_multiplier *= 1.0 + p -> spec.way_of_the_monk -> effectN( 2 ).percent();
  }



void execute(){

  }

};



struct melee_t : public monk_melee_attack_t
{
  int sync_weapons;
  tiger_strikes_melee_attack_t* tsproc;
  melee_t( const std::string& name, monk_t* player, int sw ) :
    monk_melee_attack_t( name, player, spell_data_t::nil() ), sync_weapons( sw )

  {
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    special     = false;
    school      = SCHOOL_PHYSICAL;
    may_glance  = true;
    if ( player -> dual_wield() )
    {
      base_hit -= 0.19;
      base_multiplier *= 1.0 + player -> spec.way_of_the_monk -> effectN( 2 ).percent();
    }
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = monk_melee_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, timespan_t::from_seconds( 0.2 ) ) : t/2 ) : timespan_t::from_seconds( 0.01 );
    }
    return t;
  }

  void execute()
  {
    if ( time_to_execute > timespan_t::zero() && player -> executing )
    {
      if ( sim -> debug ) sim -> output( "Executing '%s' during melee (%s).", player -> executing -> name(), util::slot_type_string( weapon -> slot ) );
      schedule_execute();
    }
    else
    {
      monk_melee_attack_t::execute();
    }
  }

  void init()
  {
    monk_melee_attack_t::init();

    tsproc = new tiger_strikes_melee_attack_t( "tiger_strikes_melee", p(), weapon );
    assert( tsproc );
  }

  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

   // if ( result_is_hit( s -> result ) ) removed, because it can proc on misses. Verify this is correct way to handle it.

    if ( p() -> buff.tiger_strikes -> up() )
      tsproc -> execute();
      p() -> buff.tiger_strikes -> decrement(); //testing decrement

    p() -> buff.tiger_strikes -> trigger( 4 );


  }
};

struct auto_attack_t : public monk_melee_attack_t
{
  int sync_weapons;

  auto_attack_t( monk_t* player, const std::string& options_str ) :
    monk_melee_attack_t( "auto_attack", player, spell_data_t::nil() ),
    sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( player -> main_hand_weapon.type != WEAPON_NONE );
    p() -> main_hand_attack = new melee_t( "melee_main_hand", player, sync_weapons );
    p() -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
    p() -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( ! player -> dual_wield() ) return;

      p() -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      p() -> off_hand_attack -> weapon = &( player -> off_hand_weapon );
      p() -> off_hand_attack -> base_execute_time = player -> off_hand_weapon.swing_time;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    p() -> main_hand_attack -> schedule_execute();

    if ( player -> off_hand_attack )
      p() -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( p() -> is_moving() ) return false;

    return ( p() -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

} // END melee_attacks NAMESPACE

namespace spells {
struct monk_spell_t : public monk_action_t<spell_t>
{
  monk_spell_t( const std::string& n, monk_t* player,
                const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = base_t::composite_target_multiplier( t );

    if ( td( t ) -> buff.rising_sun_kick -> up() )
    {
      m *=  1.0 + td( t ) -> buff.rising_sun_kick -> data().effectN( 2 ).percent();
    }

    return m;
  }
};

// Stance ===================================================================

struct stance_t : public monk_spell_t
{
  monk_stance_e switch_to_stance;
  std::string stance_str;

  stance_t( monk_t* p, const std::string& options_str ) :
    monk_spell_t( "stance", p ),
    switch_to_stance( STANCE_FIERCE_TIGER ), stance_str( "" )
  {
    option_t options[] =
    {
      { "choose",  OPT_STRING, &stance_str     },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( ! stance_str.empty() )
    {
      if ( stance_str == "drunken_ox" )
        switch_to_stance = STANCE_DRUNKEN_OX;
      else if ( stance_str == "fierce_tiger" )
        switch_to_stance = STANCE_FIERCE_TIGER;
      else if ( stance_str == "heal" )
        switch_to_stance = STANCE_WISE_SERPENT;
    }

    harmful = false;
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 1.0 );
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> active_stance = switch_to_stance;

    //TODO: Add stances once implemented
    if ( switch_to_stance == STANCE_FIERCE_TIGER )
    {
      p() -> buff.tiger_stance -> trigger();
      // cancel other stances
    }
    else if ( switch_to_stance == STANCE_DRUNKEN_OX )
    {
      p() -> buff.tiger_stance -> expire();
    }
    else if ( switch_to_stance == STANCE_WISE_SERPENT )
    {
      p() -> buff.tiger_stance -> expire();
    }
  }

  virtual bool ready()
  {
    if ( p() -> active_stance == switch_to_stance )
      return false;

    return monk_spell_t::ready();
  }
};
struct tigereye_brew_use_t : public monk_spell_t
{
  tigereye_brew_use_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "tigereye_brew_use", player, player -> find_spell( 116740 ) )
  {
    parse_options( NULL, options_str );

    harmful           = false;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    double use_value = p() -> buff.tigereye_brew_use -> default_value * p() -> buff.tigereye_brew -> stack();
    p() -> buff.tigereye_brew_use -> trigger( 1, use_value );
    p() -> buff.tigereye_brew -> expire();
  }
};

struct energizing_brew_t : public monk_spell_t
{
  energizing_brew_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "energizing_brew", player, player -> find_spell( 115288 ) )
  {
    parse_options( NULL, options_str );

    harmful           = false;
  }


  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> buff.energizing_brew -> trigger();
  }
};

struct chi_brew_t : public monk_spell_t
{
  chi_brew_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "chi_brew", player, player -> talent.chi_brew )
  {
    parse_options( NULL, options_str );

    harmful           = false;
  }

  virtual void execute()
  {
    monk_spell_t::execute();
    double chi_gain = data().effectN( 1 ).base_value();
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi_brew );
  }
};


struct monk_heal_t : public monk_action_t<heal_t>
{
  monk_heal_t( const std::string& n, monk_t* player,
               const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
  }
};

//=============================
//====Zen Sphere=============== TODO: Add healing Component
//=============================

struct zen_sphere_t : public monk_heal_t // TODO: find out if direct tick or tick zero applies
{
  struct zen_sphere_damage_t : public monk_spell_t
  {
    zen_sphere_damage_t( monk_t* player ) :
      monk_spell_t( "zen_sphere_damage", player, player -> dbc.spell( 124098 ) )
    {
      background  = true;
      base_attack_power_multiplier = 1.0;  // .7185; TODO: supposed to be .7185, but 1 gives correct number on live.
      direct_power_mod = data().extra_coeff();
      dual = true;
    }
  };
  monk_spell_t* zen_sphere_damage;

  zen_sphere_t( monk_t* player, const std::string& options_str  ) :
    monk_heal_t( "zen_sphere", player, player -> talent.zen_sphere ),
    zen_sphere_damage( 0 )
  {
    parse_options( NULL, options_str );

    zen_sphere_damage = new zen_sphere_damage_t( player );
  }

  virtual void execute()
  {
    monk_heal_t::execute();

    p() -> buff.zen_sphere -> trigger();

    if ( zen_sphere_damage )
    {
      zen_sphere_damage -> stats -> add_execute( time_to_execute );
    }
  }

  virtual void tick( dot_t* d )
  {
    monk_heal_t::tick( d );

    if ( zen_sphere_damage )
      zen_sphere_damage -> execute();
  }

  virtual bool ready()
  {
    if ( p() -> buff.zen_sphere -> check() )
      return false; // temporary to hold off on action

    return monk_heal_t::ready();
  }

};

//NYI
struct zen_sphere_detonate_t : public monk_spell_t
{
  zen_sphere_detonate_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "zen_sphere_detonate", player, player -> find_spell( 125033 ) )
  {
    parse_options( NULL, options_str );
    aoe     = -1;
    background = true;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

  }
};

//-----
//--Chi wave
//-----
/*
 * TODO: FOR REALISTIC BOUNCING, IT WILL BOUNCE ENEMY -> MONK -> ENEMY -> MONK -> ENEMY but on dummies it hits enemy then monk and stops.
 * So only 3 ticks will occur in a single target simming scenario. Alternate scenarios need to be determined.
 * TODO: Need to add decrementing buff to handle bouncing mechanic. .561 coeff
 * verify damage
*/
struct chi_wave_t : public monk_spell_t
{

  chi_wave_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "chi_wave", player, player -> talent.chi_wave )
  {
    parse_options( NULL, options_str );
    base_dd_min = player -> find_spell( 115108 ) -> effectN( 1 ).min( player );
    base_dd_max = player -> find_spell( 115108 ) -> effectN( 1 ).max( player );
  }

  virtual void execute()
  {
    monk_spell_t::execute();

  }

};

// Chi Burst
struct chi_burst_t : public monk_spell_t
{

  chi_burst_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "chi_burst", player, player -> talent.chi_burst )
  {
    parse_options( NULL, options_str );
    aoe = -1;
    base_attack_power_multiplier = 1.0;
    direct_power_mod = player -> find_spell( 130651 ) -> extra_coeff();
    base_dd_min = player -> find_spell( 130651 ) -> effectN( 1 ).min( player );
    base_dd_max = player -> find_spell( 130651 ) -> effectN( 1 ).max( player );

  }

  virtual void schedule_execute()
  {
    if ( sim -> log )
    {
      sim -> output( "%s schedules execute for %s", player -> name(), name() );
    }

    time_to_execute = execute_time();

    execute_event = start_action_execute_event( time_to_execute );

    if ( ! background )
    {
      player -> executing = this;
      player -> gcd_ready = sim -> current_time + gcd();
      if ( player -> action_queued && sim -> strict_gcd_queue )
      {
        player -> gcd_ready -= sim -> queue_gcd_reduction;
      }
/*
      if ( special && time_to_execute > timespan_t::zero() && ! proc )
      {
        // While an ability is casting, the auto_attack is paused
        // So we simply reschedule the auto_attack by the ability's casttime
        timespan_t time_to_next_hit;
        // Mainhand
        if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
        {
          time_to_next_hit  = player -> main_hand_attack -> execute_event -> occurs();
          time_to_next_hit -= sim -> current_time;
          time_to_next_hit += time_to_execute;
          player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
        }
        // Offhand
        if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
        {
          time_to_next_hit  = player -> off_hand_attack -> execute_event -> occurs();
          time_to_next_hit -= sim -> current_time;
          time_to_next_hit += time_to_execute;
          player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
        }
      }*/
    }
  }
  virtual void execute()
  {
    monk_spell_t::execute();

  }


};

// Rushing Jade Wind
// TODO: Add spinning crane kick debuff - find out if its personal or on target
struct rushing_jade_wind_t : public monk_spell_t
{

  rushing_jade_wind_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "rushing_jade_wind", player, player -> talent.rushing_jade_wind )
  {
    parse_options( NULL, options_str );
    aoe = -1;
    direct_power_mod = data().extra_coeff();
  }
  /* Implement Soon
  virtual void impact( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    td( s -> target ) -> debuff.rushing_jade_wind -> trigger();
    // will this top you from using the proc if you can't afford the ability?
  }
*/
  virtual void execute()
  {
    monk_spell_t::execute();

  }

};

// Enveloping Mist

struct enveloping_mist_t : public monk_heal_t
{
  enveloping_mist_t( monk_t* player, const std::string& options_str ) :
    monk_heal_t( "zen_sphere_detonate", player, player -> find_class_spell( "Enveloping Mist" ) )
  {
    parse_options( NULL, options_str );

    stancemask = STANCE_WISE_SERPENT;
  }

  virtual void impact( action_state_t* s )
  {
    monk_heal_t::impact( s );

    td( s -> target ) -> buff.enveloping_mist -> trigger();
  }
};

// Invoke Xuen, The White Tiger

struct xuen_spell_t : public monk_spell_t
{
  xuen_spell_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "invoke_xuen", player, player -> talent.invoke_xuen ) //123904
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    p() -> pet_xuen -> summon( data().duration() );

  }
};
} // END spells NAMESPACE

namespace statues {

struct statue_t : public pet_t
{
  statue_t( sim_t* sim, monk_t* owner, const std::string& n, pet_e pt, bool guardian = false ) :
    pet_t( sim, owner, n, pt, guardian )
  {

  }

  monk_t* o() const
  { return static_cast<monk_t*>( owner ); }
};

struct jade_serpent_statue_t : public statue_t
{
  typedef statue_t base_t;

  jade_serpent_statue_t ( sim_t* sim, monk_t* owner, const std::string& n ) :
    base_t( sim, owner, n, PET_NONE, true )
  {

  }
};
} // NAMESPACE STATUES

struct xuen_pet_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    melee_t( xuen_pet_t* player ) :
      melee_attack_t( "melee", player, spell_data_t::nil() )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      background = true;
      repeating = true;
      may_crit = true;
      may_glance = false;
      school      = SCHOOL_PHYSICAL;
      weapon_power_mod = .31; // verify

    }

    xuen_pet_t* p() { return static_cast<xuen_pet_t*>( player ); }

    void init()
    {
      melee_attack_t::init();

      pet_t* first_pet = p() -> o() -> find_pet( "xuen_the_white_tiger" );
      if ( first_pet != player )
        stats = first_pet -> find_stats( name() );
    }

    virtual void impact( action_state_t* state )
    {
      melee_attack_t::impact( state );

    }
  };

  struct crackling_tiger_lightning_t : public melee_attack_t
  {
    crackling_tiger_lightning_t( xuen_pet_t* player ) :
      melee_attack_t( "crackling_tiger_lightning", player, player -> find_spell( 123996 ) )
    {
      tick_may_crit  = true;
      //direct_tick = true;
      aoe = 3;
      tick_power_mod = data().extra_coeff();
      base_td = data().effectN( 1 ).max( player );
      cooldown -> duration = timespan_t::from_seconds( 6.0 );
      base_multiplier = 1.323; //1.58138311; EDITED FOR ACTUAL VALUE. verify in the future.

    }

    xuen_pet_t* p() { return static_cast<xuen_pet_t*>( player ); }

// Not sure if this is needed vvv
/*
    void init()
    {
      melee_attack_t::init();
      pet_t* first_pet = p() -> o() -> find_pet( "xuen_the_white_tiger" );
      if ( first_pet != player )
        stats = first_pet -> find_stats( name() );
    }*/

  };

  melee_t* melee;

  xuen_pet_t( sim_t* sim, monk_t* owner ) :
    pet_t( sim, owner, "xuen_the_white_tiger" ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );

    owner_coeff.ap_from_ap = .31; //verify
  }

  monk_t* o() { return static_cast<monk_t*>( owner ); }

  virtual void init_base()
  {
    pet_t::init_base();

    melee = new melee_t( this );
  }

  virtual void init_actions()
  {
    action_list_str = "crackling_tiger_lightning";

    pet_t::init_actions();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str )
  {
    if ( name == "crackling_tiger_lightning" ) return new crackling_tiger_lightning_t( this );

    return pet_t::create_action( name, options_str );
  }

  void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false )
  {
    if ( melee && ! melee -> execute_event )
      melee -> schedule_execute();

    pet_t::schedule_ready( delta_time, waiting );
  }
};

// ==========================================================================
// Monk Character Definition
// ==========================================================================

monk_td_t::monk_td_t( player_t* target, monk_t* p ) :
  actor_pair_t( target, p ),
  buff( buffs_t() )
{
  buff.rising_sun_kick = buff_creator_t( *this, "rising_sun_kick" ).spell( p -> find_class_spell( "Rising Sun Kick" ) );
  buff.enveloping_mist = buff_creator_t( *this, "rising_sun_kick" ).spell( p -> find_class_spell( "Enveloping Mist" ) );

}

// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  // Melee Attacks
  if ( name == "auto_attack"         ) return new         auto_attack_t( this, options_str );
  if ( name == "jab"                 ) return new                 jab_t( this, options_str );
  if ( name == "tiger_palm"          ) return new          tiger_palm_t( this, options_str );
  if ( name == "blackout_kick"       ) return new       blackout_kick_t( this, options_str );
  if ( name == "spinning_crane_kick" ) return new spinning_crane_kick_t( this, options_str );
  if ( name == "fists_of_fury"       ) return new       fists_of_fury_t( this, options_str );
  if ( name == "rising_sun_kick"     ) return new     rising_sun_kick_t( this, options_str );
  if ( name == "stance"              ) return new              stance_t( this, options_str );
  if ( name == "tigereye_brew_use"   ) return new   tigereye_brew_use_t( this, options_str );
  if ( name == "energizing_brew"     ) return new     energizing_brew_t( this, options_str );
  if ( name == "zen_sphere"          ) return new          zen_sphere_t( this, options_str );
  if ( name == "chi_wave"            ) return new            chi_wave_t( this, options_str );
  if ( name == "chi_burst"           ) return new           chi_burst_t( this, options_str );
  if ( name == "rushing_jade_wind"   ) return new   rushing_jade_wind_t( this, options_str );
  if ( name == "chi_sphere"          ) return new          chi_sphere_t( this, options_str );
  if ( name == "chi_brew"            ) return new            chi_brew_t( this, options_str );

  // Heals
  if ( name == "enveloping_mist"     ) return new     enveloping_mist_t( this, options_str );

  // Monk Spell
  if ( name == "invoke_xuen") return new          xuen_spell_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// monk_t::create_pet =====================================================

pet_t* monk_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "xuen_the_white_tiger"             ) return new xuen_pet_t( sim, this );

  return 0;
}

// monk_t::create_pets ====================================================

void monk_t::create_pets()
{
  pet_xuen    = create_pet( "xuen_the_white_tiger"             );

}


// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  player_t::init_spells();

  //TALENTS
  talent.ascension            = find_talent_spell( "Ascension" );
  talent.zen_sphere           = find_talent_spell( "Zen Sphere" );
  talent.invoke_xuen          = find_talent_spell( "Invoke Xuen, the White Tiger", "invoke_xuen" ); //find_spell( 123904 );
  talent.chi_wave             = find_talent_spell( "Chi Wave" );
  talent.chi_burst            = find_talent_spell( "Chi Burst" );
  talent.chi_brew             = find_talent_spell( "Chi Brew" );
  talent.rushing_jade_wind    = find_talent_spell( "Rushing Jade Wind", "rushing_jade_wind" );
  talent.chi_brew             = find_talent_spell( "Chi Brew" );
  talent.power_strikes        = find_talent_spell( "Power Strikes" );

  //PASSIVE/SPECIALIZATION
  spec.way_of_the_monk        = find_spell( 108977 );
  spec.leather_specialization = find_specialization_spell( "Leather Specialization" );

  //SPELLS
  active_blackout_kick_dot = new dot_blackout_kick_t( this );

  //GLYPHS

  //MASTERY
  mastery.combo_breaker = find_mastery_spell( MONK_WINDWALKER );


  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //    C2P      C4P     M2P     M4P     T2P     T4P     H2P     H4P
    {       0,       0,      0,      0,      0,      0,      0,      0 }, // Tier13
    {       0,       0, 123149, 123150, 123157, 123159, 123152, 123153 }, // Tier14
    {       0,       0,      0,      0,      0,      0,      0,      0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// monk_t::init_base ========================================================

void monk_t::init_base()
{
  player_t::init_base();

  int tree = specialization();

  initial.distance = ( tree == MONK_MISTWEAVER ) ? 40 : 3;

  base_gcd = timespan_t::from_seconds( 1.0 );

  resources.base[  RESOURCE_CHI  ] = 4 + talent.ascension -> effectN( 1 ).base_value();
  resources.base[ RESOURCE_ENERGY ] = 100;

  base_chi_regen_per_second = 0; //
  base_energy_regen_per_second = 10.0; // TODO: add increased energy regen for brewmaster.

  base.attack_power = level * 2.0;
  initial.attack_power_per_strength = 1.0;
  initial.attack_power_per_agility  = 2.0;


  // FIXME: Add defensive constants
  //diminished_kfactor    = 0;
  //diminished_dodge_capi = 0;
  //diminished_parry_capi = 0;
}

// monk_t::init_scaling =====================================================

void monk_t::init_scaling()
{
  player_t::init_scaling();

}

// monk_t::init_buffs =======================================================

void monk_t::init_buffs()
{
  player_t::init_buffs();

  buff.tiger_stance      = buff_creator_t( this, "tiger_stance"        ).spell( find_spell( 103985 ) );
  buff.serpent_stance    = buff_creator_t( this, "serpent_stance"      ).spell( find_spell( 115070 ) );
  buff.tigereye_brew     = buff_creator_t( this, "tigereye_brew"       ).spell( find_spell( 125195 ) );
  buff.tigereye_brew_use = buff_creator_t( this, "tigereye_brew_use"   ).spell( find_spell( 116740 ) );
  buff.tiger_strikes     = buff_creator_t( this, "tiger_strikes"       ).spell( find_spell( 120273 ) )
                           .chance( find_spell( 120272 ) -> proc_chance() );
  buff.combo_breaker_bok = buff_creator_t( this, "combo_breaker_bok"   ).spell( find_spell( 116768 ) );
  buff.combo_breaker_tp  = buff_creator_t( this, "combo_breaker_tp"    ).spell( find_spell( 118864 ) );
  buff.energizing_brew   = buff_creator_t( this, "energizing_brew"     ).spell( find_spell( 115288 ) );
  buff.energizing_brew -> buff_duration += sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value(); //verify working
  buff.zen_sphere        = buff_creator_t( this, "zen_sphere"          ).spell( find_spell( 124081 ) );
  buff.chi_sphere        = buff_creator_t( this, "chi_sphere"          );
  buff.tiger_power       = buff_creator_t( this, "tiger_power"         ).spell( find_class_spell( "tiger_power" ) );
}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  player_t::init_gains();

  gain.chi                   = get_gain( "chi" );
  gain.combo_breaker_savings = get_gain( "combo_breaker_savings" );
  gain.energizing_brew       = get_gain( "energizing_brew" );
  gain.avoided_chi           = get_gain( "chi_from_avoided_attacks" );
  gain.chi_brew              = get_gain( "chi_from_chi_brew");
}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  player_t::init_procs();

}

// monk_t::init_rng =========================================================

void monk_t::init_rng()
{
  player_t::init_rng();
}

// monk_t::init_actions =====================================================

void monk_t::init_actions()
{
  if ( false )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's class isn't supported yet.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;

    switch ( specialization() )
    {
    case MONK_BREWMASTER:
    case MONK_WINDWALKER:
    case MONK_MISTWEAVER:
    default:
      // Flask
      if ( level > 85 )
        precombat += "/flask,type=warm_sun";
      else if ( level >= 80 && specialization() == MONK_MISTWEAVER )
        precombat += "/flask,type=draconic_mind";
      else
        precombat += "/flask,type=winds";

      // Food
      if ( level > 85 )
      {
        precombat += "/food,type=";
        precombat += ( specialization() == MONK_MISTWEAVER ) ? "mogu_fish_stew" : "sea_mist_rice_noodles";
      }
      else if ( level > 80 )
      {
        precombat += "/food,type=seafood_magnifique_feast";
      }
      precombat += "/stance";
      precombat += "/snapshot_stats";

      action_list_str += "/auto_attack";
   //   action_list_str += "/chi_sphere,if=talent.power_strikes.enabled";
   //   action_list_str += "/chi_brew,if=talent.chi_brew.enabled&energy<=60&chi<2";
      action_list_str += "/energizing_brew,if=energy<=30";
      action_list_str += "/tigereye_brew_use,if=buff.tigereye_brew.react=10";
      action_list_str += "/invoke_xuen,if=talent.invoke_xuen.enabled";
      action_list_str += "/rising_sun_kick";
      action_list_str += "/tiger_palm,if=buff.tiger_power.remains<=3|buff.tiger_power.react<3";
   //   action_list_str += "/chi_burst,if=talent.chi_burst.enabled";
   //   action_list_str += "/chi_wave,if=talent.chi_wave.enabled";
   //   action_list_str += "/rushing_jade_wind,if=talent.rushing_jade_wind.enabled";
   //   if ( talent.zen_sphere -> ok() && level < 85 )
   //     action_list_str += "/zen_sphere,if=!buff.zen_sphere.up";//this can potentionally be used in line with CD's+FoF - Not likely anymore. Will have to sim AOE
   //   action_list_str += "/fists_of_fury";
      action_list_str += "/jab,if=(energy>=80&chi<=2&cooldown.power_strikes.remains)|(energy>=80&chi<=1&!cooldown.power_strikes.remains)";
      action_list_str += "/blackout_kick,if=buff.combo_breaker_bok.react";
      action_list_str += "/tiger_palm,if=buff.combo_breaker_tp.react";
      action_list_str += "/blackout_kick,if=buff.tiger_power.stack=3";
      action_list_str += "/blackout_kick,if=cooldown.rising_sun_kick.remains>=2";
      action_list_str += "/jab,if=(chi<=2&cooldown.power_strikes.remains)|(chi<=1&!cooldown.power_strikes.remains)";

      //   action_list_str += "/spinning_crane_kick,if=cooldown.fists_of_fury.remains";



      break;

    }
  }

  player_t::init_actions();
}

// monk_t::reset ==================================================

void monk_t::reset()
{
  player_t::reset();

  track_chi_consumption = 0;
}

// monk_t::regen (brews/teas)=======================================
void monk_t::regen( timespan_t periodicity )
{
  resource_e resource_type = primary_resource();

  if ( resource_type == RESOURCE_MANA )
  {

    //TODO: add mana tea here
  }
  else if ( resource_type == RESOURCE_ENERGY )
  {
    if ( buff.energizing_brew -> up() )
      resource_gain( RESOURCE_ENERGY, buff.energizing_brew -> data().effectN( 1 ).base_value() * periodicity.total_seconds(), gain.energizing_brew );
  }

  player_t::regen( periodicity );
}
// monk_t::init_resources ==================================================

void monk_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_CHI ] = initial_chi;
}

// monk_t::matching_gear_multiplier =========================================

double monk_t::matching_gear_multiplier( attribute_e attr )
{

  switch ( specialization() )
  {
  case MONK_MISTWEAVER:
    if ( attr == ATTR_INTELLECT )
      return spec.leather_specialization -> effectN( 1 ).percent();
    break;
  case MONK_WINDWALKER:
    if ( attr == ATTR_AGILITY )
      return spec.leather_specialization -> effectN( 1 ).percent();
    break;
  case MONK_BREWMASTER:
    if ( attr == ATTR_STAMINA )
      return spec.leather_specialization -> effectN( 1 ).percent();
    break;
  default:
    break;
  }

  return 0.0;
}

// monk_t::decode_set =======================================================

int monk_t::decode_set( item_t& item )
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

  if ( strstr( s, "red_crane" ) )
  {
    bool is_healer = ( strstr( s, "helm"           ) ||
                       strstr( s, "mantle"         ) ||
                       strstr( s, "robes"          ) ||
                       strstr( s, "legwraps"       ) ||
                       strstr( s, "handwraps"      ) );

    if ( is_healer ) return SET_T14_HEAL;

    bool is_tank_or_melee = ( strstr( s, "headpiece"       ) ||
                              strstr( s, "spaulders"       ) ||
                              strstr( s, "raiment"         ) ||
                              strstr( s, "legguards"       ) ||
                              strstr( s, "grips"           ) );

    if ( is_tank_or_melee )
    {
      const char* t = item.encoded_stats_str.c_str();

      bool is_tank = false;

      switch ( item.slot )
      {
        case SLOT_HEAD:      if ( strstr( t, "fixme"     ) ) is_tank = true; break; // Impossible to tell apart without the ID or set name.
        case SLOT_SHOULDERS: if ( strstr( t, "dodge"     ) ) is_tank = true; break;
        case SLOT_CHEST:     if ( strstr( t, "fixme"     ) ) is_tank = true; break; // Impossible to tell apart without the ID or set name.
        case SLOT_HANDS:     if ( strstr( t, "fixme"     ) ) is_tank = true; break; // Impossible to tell apart without the ID or set name.
        case SLOT_LEGS:      if ( strstr( t, "fixme"     ) ) is_tank = true; break; // Impossible to tell apart without the ID or set name.
        default: return SET_NONE;
      }

      if ( is_tank ) return SET_T14_TANK;

      return SET_T14_MELEE;
    }
  }

  if ( strstr( s, "_gladiators_copperskin_"  ) ) return SET_PVP_HEAL;
  if ( strstr( s, "_gladiators_ironskin_"    ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// monk_t::composite_player_multiplier

double monk_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( active_stance == STANCE_FIERCE_TIGER )
  {
    m *= 1.0 + buff.tiger_stance -> data().effectN( 3 ).percent();
  }

  return m;
}

// monk_t::create_options =================================================

void monk_t::create_options()
{
  player_t::create_options();

  option_t monk_options[] =
  {
    { "initial_chi",     OPT_INT,               &( initial_chi      ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, monk_options );
}

// monk_t::primary_role ==================================================

resource_e monk_t::primary_resource()
{
  // FIXME: change to healing stance
  if ( specialization() == MONK_MISTWEAVER )
    return RESOURCE_MANA;

  return RESOURCE_ENERGY;
}
// monk_t::primary_role ==================================================

role_e monk_t::primary_role()
{
  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_HYBRID )
    return ROLE_HYBRID;

  if ( player_t::primary_role() == ROLE_TANK  )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HEAL;

  if ( specialization() == MONK_BREWMASTER )
    return ROLE_TANK;

  if ( specialization() == MONK_MISTWEAVER )
    return ROLE_HEAL;

  if ( specialization() == MONK_WINDWALKER )
    return ROLE_DPS;

  return ROLE_HYBRID;
}

// monk_t::pre_analyze_hook  ==============================================

void monk_t::pre_analyze_hook()
{
  player_t::pre_analyze_hook();

  if ( stats_t* zen_sphere = find_stats( "zen_sphere" ) )
  {
    if ( stats_t* zen_sphere_dmg = find_stats( "zen_sphere_damage" ) )
    {
      zen_sphere_dmg -> total_execute_time = zen_sphere -> total_execute_time;
    }
  }
}

// MONK MODULE INTERFACE ================================================

struct monk_module_t : public module_t
{
  monk_module_t() : module_t( MONK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE )
  {
    return new monk_t( sim, name, r );
  }
  virtual bool valid() { return true; }
  virtual void init        ( sim_t* ) {}
  virtual void combat_begin( sim_t* ) {}
  virtual void combat_end  ( sim_t* ) {}
};

} // UNNAMED NAMESPACE

module_t* module_t::monk()
{
  static module_t* m = 0;
  if ( ! m ) m = new monk_module_t();
  return m;
}
