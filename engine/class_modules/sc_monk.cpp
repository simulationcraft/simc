// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  TODO:

  Add all buffs
  Change expel harm to heal later on.

  WINDWALKER:
        Power Strikes timers not linked to spelldata (fix soon)

  MISTWEAVER:
  No plans just yet.
  Add conditional for SP multipliers on abilities

  BREWMASTER:
  No plans just yet.

*/
#include "simulationcraft.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace { // UNNAMED NAMESPACE

struct monk_t;

enum monk_stance_e { STANCE_DRUNKEN_OX=1, STANCE_FIERCE_TIGER, STANCE_WISE_SERPENT=4 };

struct monk_td_t : public actor_pair_t
{
public:
  struct buffs_t
  {
    debuff_t* rising_sun_kick;
    debuff_t* rushing_jade_wind;
    buff_t* enveloping_mist;
  } buff;

  monk_td_t( player_t*, monk_t* );

};

struct monk_t : public player_t
{
public:
  typedef player_t base_t;

  // Pets
  monk_stance_e active_stance;
  action_t* active_blackout_kick_dot;
  double track_chi_consumption;

  // Buffs
  struct buffs_t
  {
    // TODO: Finish Adding Buffs - will uncomment as implemented
    buff_t* energizing_brew;
    buff_t* zen_sphere;
    buff_t* tiger_power;
    //  buff_t* fortifying_brew;
    //  buff_t* zen_meditation;
    //  buff_t* path_of_blossoms;
    buff_t* tigereye_brew;
    buff_t* tigereye_brew_use;
    haste_buff_t* tiger_strikes;
    buff_t* combo_breaker_tp;
    buff_t* combo_breaker_bok;
    buff_t* tiger_stance;
    buff_t* serpent_stance;
    buff_t* chi_sphere;
    buff_t* power_strikes;

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
    gain_t* soothing_mist;
    gain_t* tier15_2pc;
  } gain;

  // Procs
  struct procs_t
  {
    proc_t* combo_breaker_bok;
    proc_t* combo_breaker_tp;
    proc_t* tier15_2pc;
    proc_t* tier15_4pc;
  } proc;

  // Random Number Generation
  struct rngs_t
  {
    rng_t* tier15_2pc;
    rng_t* tier15_4pc;
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
    const spell_data_t* chi_torpedo;
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
    const spell_data_t* brewing_tigereye_brew;
    const spell_data_t* combo_breaker;

    // TREE_MONK_HEAL
  } spec;

  struct mastery_spells_t
  {
    const spell_data_t* combo_breaker; // WINDWALKER
    const spell_data_t* bottled_fury;
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
  } cooldowns;

  struct spells_t
  {
    const spell_data_t* tier15_2pc;
  } spell;

  // Options
  int initial_chi;
private:
  target_specific_t<monk_td_t> target_data;
public:
  monk_t( sim_t* sim, const std::string& name, race_e r = RACE_PANDAREN ) :
    player_t( sim, MONK, name, r ),
    active_stance( STANCE_DRUNKEN_OX ),
    active_blackout_kick_dot( NULL ),
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

  }

  // Character Definition
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual double    composite_attack_speed();
  virtual double    energy_regen_per_second();
  virtual double    composite_player_multiplier( school_e school, action_t* a );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_rng();
  virtual void      init_procs();
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
  virtual void      combat_begin();

  virtual monk_td_t* get_target_data( player_t* target )
  {
    monk_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new monk_td_t( target, this );
    }
    return td;
  }
};

// =======================================
// Monk Pets & Statues
// =======================================
namespace pets {

struct statue_t : public pet_t
{
  statue_t( sim_t* sim, monk_t* owner, const std::string& n, pet_e pt, bool guardian = false ) :
    pet_t( sim, owner, n, pt, guardian )
  { }

  monk_t* o() const
  { return static_cast<monk_t*>( owner ); }
};

struct jade_serpent_statue_t : public statue_t
{
  typedef statue_t base_t;

  jade_serpent_statue_t ( sim_t* sim, monk_t* owner, const std::string& n ) :
    base_t( sim, owner, n, PET_NONE, true )
  { }
};

struct xuen_pet_t : public pet_t
{
private:
  struct melee_t : public melee_attack_t
  {
    melee_t( const std::string& n, xuen_pet_t* player ) :
      melee_attack_t( n, player, spell_data_t::nil() )
    {
      background = true;
      repeating = true;
      may_crit = true;
      may_glance = true;
      school      = SCHOOL_NATURE;

      // Use damage numbers from the level-scaled weapon
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;


      trigger_gcd = timespan_t::zero();
      special     = false;
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
        attack_t::execute();
      }
    }
  };

  struct crackling_tiger_lightning_t : public melee_attack_t
  {
    crackling_tiger_lightning_t( xuen_pet_t* player, const std::string options_str ) :
      melee_attack_t( "crackling_tiger_lightning", player, player -> find_spell( 123996 ) )
    {
      parse_options( 0, options_str );

      special = true;
      tick_may_crit  = true;
      aoe = 3;
      tick_power_mod = data().extra_coeff();
      cooldown -> duration = timespan_t::from_seconds( 6.0 );
      base_spell_power_multiplier  = 0;

      //base_multiplier = 1.323; //1.58138311; EDITED FOR ACTUAL VALUE. verify in the future.
    }
  };

  struct auto_attack_t : public attack_t
  {
    auto_attack_t( xuen_pet_t* player, const std::string& options_str ) :
      attack_t( "auto_attack", player, spell_data_t::nil() )
    {
      parse_options( 0, options_str );

      player -> main_hand_attack = new melee_t( "melee_main_hand", player );
      player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero();
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();

      if ( player -> off_hand_attack )
        player -> off_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      if ( player -> is_moving() ) return false;

      return ( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

public:
  xuen_pet_t( sim_t* sim, monk_t* owner ) :
    pet_t( sim, owner, "xuen_the_white_tiger" )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level );
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );

    owner_coeff.ap_from_ap = 0.5;
  }

  monk_t* o() const { return static_cast<monk_t*>( owner ); }

  virtual void init_actions()
  {
    action_list_str = "auto_attack";
    action_list_str += "/crackling_tiger_lightning";

    pet_t::init_actions();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str )
  {
    if ( name == "crackling_tiger_lightning" ) return new crackling_tiger_lightning_t( this, options_str );
    if ( name == "auto_attack" ) return new auto_attack_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }

};

} // end namespace pets

namespace actions {

// ==========================================================================
// Monk Abilities
// ==========================================================================

// Template for common monk action code. See priest_action_t.
template <class Base>
struct monk_action_t : public Base
{
  int stancemask;

private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef monk_action_t base_t;

  monk_action_t( const std::string& n, monk_t* player,
                 const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_WISE_SERPENT )
  {
    ab::may_crit   = true;
  }
  virtual ~monk_action_t() {}

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
      double chi_to_track = ab::resource_consumed;
      if ( p() -> bugs && p() -> buff.combo_breaker_tp -> check() )
      {
        // added 10/1/2013 see Issue 1526 or
        // http://elitistjerks.com/f99/t131848-ww_5_1_still_fisting_windwalker_thread/#Known_Issues
        chi_to_track = std::min( 1.0, chi_to_track );
      }
      p() -> track_chi_consumption += chi_to_track;
    }

    if ( p() -> spec.brewing_tigereye_brew -> ok() )
    {
      int chi_to_consume = p() -> spec.brewing_tigereye_brew -> effectN( 1 ).base_value();

      if ( p() -> track_chi_consumption >= chi_to_consume )
      {
        p() -> track_chi_consumption -= chi_to_consume;

        p() -> buff.tigereye_brew -> trigger();

        if ( p() -> dbc.ptr && p() -> set_bonus.tier15_4pc_melee() &&
             p() -> rng.tier15_4pc -> roll( p() -> sets -> set( SET_T15_4PC_MELEE ) -> effectN( 1 ).percent() ) )
        {
          p() -> buff.tigereye_brew -> trigger();
          p() -> proc.tier15_4pc -> occur();
        }
      }
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

  monk_melee_attack_t( const std::string& n, monk_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ),
    mh( NULL ), oh( NULL )
  {
    special = true;
    may_glance = false;
  }

  virtual double target_armor( player_t* t )
  {
    double a = base_t::target_armor( t );

    if ( p() -> buff.tiger_power -> up() )
      a *= 1.0 - p() -> buff.tiger_power -> check() * p() -> buff.tiger_power -> data().effectN( 1 ).percent();

    return a;
  }

  // Special Monk Attack Weapon damage collection, if the pointers mh or oh are set, instead of the classical action_t::weapon
  // Damage is divided instead of multiplied by the weapon speed, AP portion is not multiplied by weapon speed.
  // Both MH and OH are directly weaved into one damage number
  virtual double calculate_weapon_damage( double ap )
  {
    double total_dmg = 0;
    // Main Hand
    if ( mh && mh -> type != WEAPON_NONE && weapon_multiplier > 0 )
    {
      assert( mh -> slot != SLOT_OFF_HAND );

      double dmg = sim -> averaged_range( mh -> min_dmg, mh -> max_dmg ) + mh -> bonus_dmg;

      dmg /= mh -> swing_time.total_seconds();

      total_dmg += dmg;

      if ( sim -> debug )
      {
        sim -> output( "%s main hand weapon damage portion for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f ap=%.3f",
                       player -> name(), name(), total_dmg, dmg, mh -> bonus_dmg, mh -> swing_time.total_seconds(), ap );
      }
    }

    // Off Hand
    if ( oh && oh -> type != WEAPON_NONE && weapon_multiplier > 0 )
    {
      assert( oh -> slot == SLOT_OFF_HAND );

      double dmg = sim -> averaged_range( oh -> min_dmg, oh -> max_dmg ) + oh -> bonus_dmg;

      dmg /= oh -> swing_time.total_seconds();

      // OH penalty
      dmg *= 0.5;

      total_dmg += dmg;

      if ( sim -> debug )
      {
        sim -> output( "%s off-hand weapon damage portion for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f ap=%.3f",
                       player -> name(), name(), total_dmg, dmg, oh -> bonus_dmg, oh -> swing_time.total_seconds(), ap );
      }
    }

    if ( player -> dual_wield() )
      total_dmg *= 0.898882275;

    if ( ! mh && ! oh )
      total_dmg += base_t::calculate_weapon_damage( ap );
    else
      total_dmg += weapon_power_mod * ap;

    return total_dmg;
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = base_t::composite_target_multiplier( t );

    if ( td( t ) -> buff.rising_sun_kick -> up() )
    {
      m *=  1.0 + td( t ) -> buff.rising_sun_kick -> data().effectN( 1 ).percent();
    }

    return m;
  }
};

//=============================
//====Jab======================
//=============================

struct jab_t : public monk_melee_attack_t
{

  jab_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "jab", p, p -> find_class_spell( "Jab" ) )//,
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
    if ( p() -> active_stance == STANCE_FIERCE_TIGER )
      return RESOURCE_ENERGY;

    return monk_melee_attack_t::current_resource();
  }

  double combo_breaker_chance()
  {
    if ( ! p() -> dbc.ptr )
      return p() -> mastery.combo_breaker -> effectN( 1 ).mastery_value() * player -> composite_mastery();
    else
      return p() -> spec.combo_breaker -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    // Windwalker Mastery
    // Debuffs are independent of each other

    double cb_chance = combo_breaker_chance();
    p() -> buff.combo_breaker_bok -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance );
    p() -> buff.combo_breaker_tp  -> trigger( 1, buff_t::DEFAULT_VALUE(), cb_chance );

    // Chi Gain
    double chi_gain = data().effectN( 2 ).base_value();
    if ( p() -> active_stance  == STANCE_FIERCE_TIGER )
    {
      chi_gain += p() -> buff.tiger_stance -> data().effectN( 4 ).base_value();
    }
    if ( p() -> buff.power_strikes -> up() )
    {
      if ( p()-> resources.current[ RESOURCE_CHI ] < 2 )
      {
        chi_gain += p() -> buff.power_strikes -> data().effectN( 1 ).base_value();
      }
      else
      {
        p() -> buff.chi_sphere -> trigger();
      }
      p() -> buff.power_strikes -> expire();
    }
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi );

    if ( p() -> dbc.ptr && p() -> set_bonus.tier15_2pc_melee() &&
         p() -> rng.tier15_2pc -> roll( p() -> sets -> set( SET_T15_2PC_MELEE ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> spell.tier15_2pc -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc );
      p() -> proc.tier15_2pc -> occur();
    }
  }
};

//=============================
//====Expel Harm=============== Change to heal later.
//=============================

struct expel_harm_t : public monk_melee_attack_t
{
  expel_harm_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "expel_harm", p, p -> find_class_spell( "Expel Harm" ) )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;

    base_dd_min = base_dd_max = direct_power_mod = 0.0; // deactivate parsed spelleffect1

    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );

    base_multiplier = 7.0; // hardcoded into tooltip
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

    // Chi Gain
    double chi_gain = data().effectN( 2 ).base_value();
    if ( p() -> active_stance  == STANCE_FIERCE_TIGER )
    {
      chi_gain += p() -> buff.tiger_stance -> data().effectN( 4 ).base_value();
    }
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi );

    if ( p() -> dbc.ptr && p() -> set_bonus.tier15_2pc_melee() &&
         p() -> rng.tier15_2pc -> roll( p() -> sets -> set( SET_T15_2PC_MELEE ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> spell.tier15_2pc -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc );
      p() -> proc.tier15_2pc -> occur();
    }
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

    p() -> buff.tiger_power -> trigger();
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

struct dot_blackout_kick_t : public ignite::pct_based_action_t< monk_melee_attack_t >
{
  dot_blackout_kick_t( monk_t* p ) :
    base_t( "blackout_kick_dot", p, p -> find_spell ( 128531 ) )
  {
    tick_may_crit = true;
    may_miss = false;
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
    base_multiplier = 8.0 * 0.89; // hardcoded into tooltip
  }

  virtual void assess_damage( dmg_e type, action_state_t* s )
  {
    monk_melee_attack_t::assess_damage( type, s );

    trigger_blackout_kick_dot( this, s -> target, s -> result_amount * data().effectN( 2 ).percent( ) );
  }

  virtual double cost()
  {
    if ( p() -> buff.combo_breaker_bok -> check() )
    {
      p() -> track_chi_consumption += 2;
      return 0;
    }
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
struct rsk_debuff_t : public monk_melee_attack_t
{
  rsk_debuff_t( monk_t* p, const spell_data_t* s ) :
    monk_melee_attack_t( "rsk_debuff", p, s )
  {
    background  = true;
    dual        = true;
    aoe = -1;
    weapon_multiplier = 0;
  }

  virtual void impact ( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    td( s -> target ) -> buff.rising_sun_kick -> trigger();
  }
};

struct rising_sun_kick_t : public monk_melee_attack_t
{
  rsk_debuff_t* rsk_debuff;

  rising_sun_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "rising_sun_kick", p, p -> find_class_spell( "Rising Sun Kick" ) ),
    rsk_debuff( 0 )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_FIERCE_TIGER;
    base_dd_min = base_dd_max = 0.0; direct_power_mod = 0.0;//  deactivate parsed spelleffect1
    mh = &( player -> main_hand_weapon ) ;
    oh = &( player -> off_hand_weapon ) ;
    base_multiplier = 14.4 * 0.89; // hardcoded into tooltip

    rsk_debuff = new rsk_debuff_t( p, p -> find_spell( 130320 ) );
    assert( rsk_debuff );
  }

  virtual void impact ( action_state_t* s )
  {
    monk_melee_attack_t::impact( s );

    rsk_debuff -> execute();
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

  spinning_crane_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "spinning_crane_kick", p, p -> find_class_spell( "Spinning Crane Kick" ) )
  {
    parse_options( 0, options_str );

    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;

    may_crit = false;
    tick_zero = true;
    channeled = true;
    hasted_ticks = true;
    school = SCHOOL_PHYSICAL;

    tick_action = new spinning_crane_kick_tick_t( p, p -> find_spell( data().effectN( 1 ).trigger_spell_id() ) );
    dynamic_tick_action = true;
    assert( tick_action );
  }

  virtual resource_e current_resource()
  {
    // Apparently energy requirement in Fierce Tiger stance is not in spell data
    if ( p() -> active_stance == STANCE_FIERCE_TIGER )
      return RESOURCE_ENERGY;

    return monk_melee_attack_t::current_resource();
  }

  virtual double action_multiplier()
  {
    double m = monk_melee_attack_t::action_multiplier();

    if ( td( target ) -> buff.rushing_jade_wind -> up() )
      m *= 1.0 + p() -> talent.rushing_jade_wind -> effectN( 2 ).percent();

    return m;
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    double chi_gain = data().effectN( 4 ).base_value();
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi );

    if ( p() -> dbc.ptr && p() -> set_bonus.tier15_2pc_melee() &&
         p() -> rng.tier15_2pc -> roll( p() -> sets -> set( SET_T15_2PC_MELEE ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_ENERGY, p() -> spell.tier15_2pc -> effectN( 1 ).base_value(), p() -> gain.tier15_2pc );
      p() -> proc.tier15_2pc -> occur();
    }
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
      aoe = -1; // Todo: need to divide damage by targets for aoe.
      direct_tick = true;
      base_dd_min = base_dd_max = direct_power_mod = 0.0;//  deactivate parsed spelleffect1
      mh = &( player -> main_hand_weapon ) ;
      oh = &( player -> off_hand_weapon ) ;

      school = SCHOOL_PHYSICAL;
    }
  };

  fists_of_fury_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "fists_of_fury", p, p -> find_class_spell( "Fists of Fury" ) )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_FIERCE_TIGER;
    channeled = true;
    tick_zero = true;
    base_multiplier = 7.5 * 0.89; // hardcoded into tooltip
    school = SCHOOL_PHYSICAL;
    weapon_multiplier = 0;

    // T14 WW 2PC
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).time_value();

    tick_action = new fists_of_fury_tick_t( p );
    dynamic_tick_action = true;
    assert( tick_action );
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();
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
    special          = false;
    may_glance       = false;
    if ( player -> dual_wield() )
      base_multiplier *= 1.0 + p -> spec.way_of_the_monk -> effectN( 1 ).percent();
  }
};

struct melee_t : public monk_melee_attack_t
{
  struct ts_delay_t : public event_t
  {
    melee_t* melee;

    ts_delay_t( monk_t* player, melee_t* m ) :
      event_t( player, "tiger_strikes_delay" ),
      melee( m )
    {
      sim.add_event( this, timespan_t::from_seconds( sim.gauss( 1.0, 0.2 ) ) );
    }

    void execute()
    {
      assert( melee );
      melee -> tsproc -> execute();
    }
  };

  int sync_weapons;
  tiger_strikes_melee_attack_t* tsproc;
  melee_t( const std::string& name, monk_t* player, int sw ) :
    monk_melee_attack_t( name, player, spell_data_t::nil() ),
    sync_weapons( sw ), tsproc( 0 )
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
      base_multiplier *= 1.0 + player -> spec.way_of_the_monk -> effectN( 1 ).percent();
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
      if ( p() -> buff.tiger_strikes -> up() )
        new ( *sim ) ts_delay_t( p(), this );

      p() -> buff.tiger_strikes -> decrement();

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

    // FIX ME: tsproc should have a significant delay after the auto attack
    // tsproc should consume the buff.
    // If you refresh TS, the buff goes to 4 stacks and a tsproc will happen
    // up to 1200ms later and consume the buff (so you'll basically lose one
    // stack)
    /*    if ( p() -> buff.tiger_strikes -> up() )
        {
          tsproc -> execute();
          p() -> buff.tiger_strikes -> decrement();
        }
        else
        {*/
    p() -> buff.tiger_strikes -> trigger( 4 );
//    }
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
      opt_bool( "sync_weapons", sync_weapons ),
      opt_null()
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
      p() -> off_hand_attack -> id = 1;
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
      m *= 1.0 + td( t ) -> buff.rising_sun_kick -> data().effectN( 1 ).percent();
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
      opt_string( "choose", stance_str ),
      opt_null()
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

struct tigereye_brew_t : public monk_spell_t
{
  tigereye_brew_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "tigereye_brew", player, player -> find_spell( 116740 ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  double value()
  {
    double v = p() -> buff.tigereye_brew_use -> data().effectN( 1 ).percent();

    if ( player -> dbc.ptr )
      v += p() -> mastery.bottled_fury -> effectN( 3 ).mastery_value() * p() -> composite_mastery();

    return v;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    int max_stacks_consumable = p() -> dbc.ptr ?
                                p() -> spec.brewing_tigereye_brew -> effectN( 2 ).base_value() :
                                p() -> buff.tigereye_brew -> max_stack();
    double use_value = value() * std::min( p() -> buff.tigereye_brew -> stack(), max_stacks_consumable );
    p() -> buff.tigereye_brew_use -> trigger( 1, use_value );
    p() -> buff.tigereye_brew -> decrement( max_stacks_consumable );
  }
};

struct energizing_brew_t : public monk_spell_t
{
  energizing_brew_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "energizing_brew", player, player -> find_spell( 115288 ) )
  {
    parse_options( NULL, options_str );

    harmful   = false;
    num_ticks = 0;
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

    harmful = false;
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    double chi_gain = data().effectN( 1 ).base_value();
    player -> resource_gain( RESOURCE_CHI, chi_gain, p() -> gain.chi_brew );
  }
};

struct zen_sphere_damage_t : public monk_spell_t
{
  zen_sphere_damage_t( monk_t* player ) :
    monk_spell_t( "zen_sphere_damage", player, player -> dbc.spell( 124098 ) )
  {
    background  = true;
    base_attack_power_multiplier = 1.0;
    direct_power_mod = data().extra_coeff();
    dual = true;
  }
};
//NYI
struct zen_sphere_detonate_t : public monk_spell_t
{
  zen_sphere_detonate_t( monk_t* player, const std::string& options_str ) :
    monk_spell_t( "zen_sphere_detonate", player, player -> find_spell( 125033 ) )
  {
    parse_options( NULL, options_str );
    aoe = -1;
  }

  virtual void execute()
  {
    monk_spell_t::execute();
    p() -> buff.zen_sphere -> expire();
  }
};

struct spinning_fire_blossom_t : public monk_spell_t
{
  spinning_fire_blossom_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "spinning_fire_blossom", player, player -> find_spell( 115073 ) )
  {
    parse_options( NULL, options_str );

    //const spelleffect_data_t& s = player -> find_spell( 115073 ) -> effectN( 1 );
    //base_dd_min = s.min( player );
    //base_dd_max = s.max( player );
    direct_power_mod = data().extra_coeff();
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
  }

  virtual double composite_target_da_multiplier( player_t* t )
  {
    double m = monk_spell_t::composite_target_da_multiplier( t );

    if ( player -> current.distance >= 10.0 ) // replace with target-dependant range check
      m *= 1.5; // hardcoded into tooltip

    return m;
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
    num_ticks = 3;
    hasted_ticks   = false;
    base_tick_time = timespan_t::from_seconds( 1.5 );
    tick_power_mod = player -> find_spell( 132467 ) -> extra_coeff();

    direct_power_mod = base_dd_min = base_dd_max = 0;
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;

    special = false;
  }
  virtual void tick( dot_t* d )
  {
    monk_spell_t::tick( d );

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
    special = false; // Disable pausing of auto attack while casting this spell
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
    direct_power_mod = player -> find_spell( 130651 ) -> extra_coeff();
    base_dd_min = player -> find_spell( 130651 ) -> effectN( 1 ).min( player );
    base_dd_max = player -> find_spell( 130651 ) -> effectN( 1 ).max( player );
  }
};

// Rushing Jade Wind
struct rushing_jade_wind_t : public monk_spell_t
{
  rushing_jade_wind_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "rushing_jade_wind", player, player -> talent.rushing_jade_wind )
  {
    parse_options( NULL, options_str );
    aoe = -1;
    direct_power_mod = data().extra_coeff();
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
  }

  virtual void impact( action_state_t* s )
  {
    monk_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      td( s -> target ) -> buff.rushing_jade_wind -> trigger();
  }
};

// Chi Torpedo

struct chi_torpedo_t : public monk_spell_t
{
  chi_torpedo_t( monk_t* player, const std::string& options_str  ) :
    monk_spell_t( "chi_torpedo", player, player -> talent.chi_torpedo -> ok() ? player -> find_spell( 117993 ) : spell_data_t::not_found() )
  {
    parse_options( NULL, options_str );
    aoe = -1;
    direct_power_mod = data().extra_coeff();
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
  }
};

struct summon_pet_t : public monk_spell_t
{
  timespan_t summoning_duration;
  pet_t* pet;

public:
  summon_pet_t( const std::string& n, const std::string& pet_name, monk_t* p, const spell_data_t* sd = spell_data_t::nil() ) :
    monk_spell_t( n, p, sd ), summoning_duration ( timespan_t::zero() ), pet( 0 )
  {
    harmful = false;

    pet = player -> find_pet( pet_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", player -> name(), pet_name.c_str() );
    }
  }

  virtual void execute()
  {
    pet -> summon( summoning_duration );

    monk_spell_t::execute();
  }
};

// Invoke Xuen, The White Tiger

struct xuen_spell_t : public summon_pet_t
{
  xuen_spell_t( monk_t* p, const std::string& options_str ) :
    summon_pet_t( "invoke_xuen", "xuen_the_white_tiger", p, p -> talent.invoke_xuen ) //123904
  {
    parse_options( NULL, options_str );

    harmful = false;
    summoning_duration = data().duration();
  }
};

struct chi_sphere_t : public monk_spell_t
{
  chi_sphere_t( monk_t* p, const std::string& options_str  ) :
    monk_spell_t( "chi_sphere", p, spell_data_t::nil() )
  {
    parse_options( NULL, options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    monk_spell_t::execute();

    if ( p() -> buff.chi_sphere -> up() )
    {
      // Only use 1 Orb per execution
      player -> resource_gain( RESOURCE_CHI, 1, p() -> gain.chi );

      p() -> buff.chi_sphere -> decrement();
    }
  }

};
} // END spells NAMESPACE

namespace heals {

struct monk_heal_t : public monk_action_t<heal_t>
{
  monk_heal_t( const std::string& n, monk_t& p,
               const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, &p, s )
  {
  }
};

//=============================
//====Zen Sphere=============== TODO: Add healing Component
//=============================

struct zen_sphere_t : public monk_heal_t // TODO: find out if direct tick or tick zero applies
{
  spells::monk_spell_t* zen_sphere_damage;

  zen_sphere_t( monk_t& p, const std::string& options_str  ) :
    monk_heal_t( "zen_sphere", p, p.talent.zen_sphere ),
    zen_sphere_damage( new spells::zen_sphere_damage_t( &p ) )
  {
    parse_options( NULL, options_str );

    tick_power_mod = data().extra_coeff();
  }

  virtual void execute()
  {
    monk_heal_t::execute();

    p() -> buff.zen_sphere -> trigger();

    zen_sphere_damage -> stats -> add_execute( time_to_execute );
  }

  virtual void tick( dot_t* d )
  {
    monk_heal_t::tick( d );

    zen_sphere_damage -> execute();
  }

  virtual bool ready()
  {
    if ( p() -> buff.zen_sphere -> check() )
      return false; // temporary to hold off on action

    return monk_heal_t::ready();
  }
};

// Enveloping Mist

struct enveloping_mist_t : public monk_heal_t
{
  enveloping_mist_t( monk_t& p, const std::string& options_str ) :
    monk_heal_t( "zen_sphere_detonate", p, p.find_class_spell( "Enveloping Mist" ) )
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

struct soothing_mist_t : public monk_heal_t
{
  rng_t* chi_gain;

  soothing_mist_t( monk_t& p, const std::string& options_str ) :
    monk_heal_t( "soothing_mist", p, p.find_specialization_spell( "Soothing Mist" ) ),
    chi_gain( p.get_rng( "soothing_mist" ) )
  {
    parse_options( NULL, options_str );

    stancemask = STANCE_WISE_SERPENT;

    channeled = true;
  }

  virtual void tick( dot_t* d )
  {
    monk_heal_t::tick( d );

    if ( chi_gain -> roll( data().proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_CHI, 1, p() -> gain.soothing_mist, this );
    }
  }
};

} // end namespace heals

using namespace attacks;
using namespace spells;
using namespace heals;

} // end namespace actions;
struct power_strikes_event_t : public event_t
{
  power_strikes_event_t( monk_t* player, timespan_t tick_time ) :
    event_t( player, "power_strikes" )
  {
    // Safety clamp
    tick_time = clamp( tick_time, timespan_t::zero(), timespan_t::from_seconds( 20 ) );
    sim.add_event( this, tick_time );
  }

  virtual void execute()
  {
    monk_t* p = static_cast<monk_t*>( player );

    p -> buff.power_strikes -> trigger();

    new ( sim ) power_strikes_event_t( p, timespan_t::from_seconds( 20.0 ) );
  }
};

// ==========================================================================
// Monk Character Definition
// ==========================================================================

monk_td_t::monk_td_t( player_t* target, monk_t* p ) :
  actor_pair_t( target, p ),
  buff( buffs_t() )
{
  buff.rising_sun_kick   = buff_creator_t( *this, "rising_sun_kick"   ).spell( p -> find_spell( 130320 ) );
  buff.enveloping_mist   = buff_creator_t( *this, "enveloping_mist"   ).spell( p -> find_class_spell( "Enveloping Mist" ) );
  buff.rushing_jade_wind = buff_creator_t( *this, "rushing_jade_wind", p ->  talent.rushing_jade_wind -> effectN( 2 ).trigger() );
}

// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  using namespace actions;

  // Melee Attacks
  if ( name == "auto_attack"           ) return new            auto_attack_t( this, options_str );
  if ( name == "jab"                   ) return new                    jab_t( this, options_str );
  if ( name == "expel_harm"            ) return new             expel_harm_t( this, options_str );
  if ( name == "tiger_palm"            ) return new             tiger_palm_t( this, options_str );
  if ( name == "blackout_kick"         ) return new          blackout_kick_t( this, options_str );
  if ( name == "spinning_crane_kick"   ) return new    spinning_crane_kick_t( this, options_str );
  if ( name == "fists_of_fury"         ) return new          fists_of_fury_t( this, options_str );
  if ( name == "rising_sun_kick"       ) return new        rising_sun_kick_t( this, options_str );
  if ( name == "stance"                ) return new                 stance_t( this, options_str );
  if ( name == "tigereye_brew"         ) return new          tigereye_brew_t( this, options_str );
  if ( name == "energizing_brew"       ) return new        energizing_brew_t( this, options_str );
  if ( name == "spinning_fire_blossom" ) return new  spinning_fire_blossom_t( this, options_str );

  // Heals
  if ( name == "enveloping_mist"       ) return new        enveloping_mist_t( *this, options_str );
  if ( name == "soothing_mist"         ) return new          soothing_mist_t( *this, options_str );

  // Talents
  if ( name == "chi_sphere"            ) return new             chi_sphere_t( this, options_str ); // For Power Strikes
  if ( name == "chi_brew"              ) return new               chi_brew_t( this, options_str );

  if ( name == "zen_sphere"            ) return new             zen_sphere_t( *this, options_str );
  if ( name == "zen_sphere_detonate"   ) return new    zen_sphere_detonate_t( this, options_str );
  if ( name == "chi_wave"              ) return new               chi_wave_t( this, options_str );
  if ( name == "chi_burst"             ) return new              chi_burst_t( this, options_str );

  if ( name == "rushing_jade_wind"     ) return new      rushing_jade_wind_t( this, options_str );
  if ( name == "invoke_xuen"           ) return new             xuen_spell_t( this, options_str );
  if ( name == "chi_torpedo"           ) return new            chi_torpedo_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// monk_t::create_pet =====================================================

pet_t* monk_t::create_pet( const std::string& name,
                           const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( name );

  if ( p ) return p;

  using namespace pets;
  if ( name == "xuen_the_white_tiger" ) return new xuen_pet_t( sim, this );

  return 0;
}

// monk_t::create_pets ====================================================

void monk_t::create_pets()
{
  base_t::create_pets();

  create_pet( "xuen_the_white_tiger" );
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
  talent.rushing_jade_wind    = find_talent_spell( "Rushing Jade Wind" );
  talent.chi_torpedo          = find_talent_spell( "Chi Torpedo" );
  talent.power_strikes        = find_talent_spell( "Power Strikes" );

  //PASSIVE/SPECIALIZATION
  spec.way_of_the_monk        = find_spell( 108977 );
  spec.leather_specialization = find_specialization_spell( "Leather Specialization" );
  spec.brewing_tigereye_brew  = find_specialization_spell( "Brewing: Tigereye Brew" );
  spec.combo_breaker          = find_specialization_spell( "Combo Breaker" );

  //SPELLS
  active_blackout_kick_dot = new actions::dot_blackout_kick_t( this );
  if ( dbc.ptr )
    spell.tier15_2pc = find_spell( 138311 );

  //GLYPHS

  //MASTERY
  mastery.combo_breaker = find_mastery_spell( MONK_WINDWALKER );
  mastery.bottled_fury = find_mastery_spell( MONK_WINDWALKER );

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //    C2P      C4P     M2P     M4P     T2P     T4P     H2P     H4P
    {       0,       0,      0,      0,      0,      0,      0,      0 }, // Tier13
    {       0,       0, 123149, 123150, 123157, 123159, 123152, 123153 }, // Tier14
    {       0,       0, 138177, 138315, 138231, 138236, 138289, 138290 }, // Tier15
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

  base_chi_regen_per_second = 0;
  base_energy_regen_per_second = 10.0; // TODO: add increased energy regen for brewmaster.

  base.attack_power = level * 2.0;
  initial.attack_power_per_strength = 1.0;
  initial.attack_power_per_agility  = 2.0;

  // FIXME: Add defensive constants
  //diminished_kfactor    = 0;
  //diminished_dodge_cap = 0;
  //diminished_parry_cap = 0;
}

// monk_t::init_scaling =====================================================

void monk_t::init_scaling()
{
  player_t::init_scaling();

  if ( specialization() != MONK_MISTWEAVER )
  {
    scales_with[ STAT_INTELLECT             ] = false;
    scales_with[ STAT_SPIRIT                ] = false;
    scales_with[ STAT_SPELL_POWER           ] = false;
  }

  if ( off_hand_weapon.type != WEAPON_NONE )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = true;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = sim -> weapon_speed_scale_factors != 0;
    scales_with[ STAT_HIT_RATING2          ] = true;
  }
}

// monk_t::init_buffs =======================================================

void monk_t::create_buffs()
{
  player_t::create_buffs();

  buff.tiger_stance      = buff_creator_t( this, "tiger_stance"        ).spell( find_spell( 103985 ) );
  buff.serpent_stance    = buff_creator_t( this, "serpent_stance"      ).spell( find_spell( 115070 ) );
  buff.tigereye_brew     = buff_creator_t( this, "tigereye_brew"       ).spell( find_spell( 125195 ) );
  buff.tigereye_brew_use = buff_creator_t( this, "tigereye_brew_use"   ).spell( find_spell( 116740 ) );
  buff.power_strikes     = buff_creator_t( this, "power_strikes"       ).spell( find_spell( 129914 ) );
  buff.tiger_strikes     = haste_buff_creator_t( this, "tiger_strikes" ).spell( find_spell( 120273 ) )
                           .chance( find_spell( 120272 ) -> proc_chance() );
  buff.combo_breaker_bok = buff_creator_t( this, "combo_breaker_bok"   ).spell( find_spell( 116768 ) );
  buff.combo_breaker_tp  = buff_creator_t( this, "combo_breaker_tp"    ).spell( find_spell( 118864 ) );
  buff.energizing_brew   = buff_creator_t( this, "energizing_brew", find_class_spell( "Energizing Brew" ) );
  buff.energizing_brew -> buff_duration += sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value(); //verify working
  buff.zen_sphere        = buff_creator_t( this, "zen_sphere" , talent.zen_sphere       );
  buff.chi_sphere        = buff_creator_t( this, "chi_sphere"          ).max_stack( 5 );
  buff.tiger_power       = buff_creator_t( this, "tiger_power"         , find_class_spell( "Tiger Palm" ) -> effectN( 2 ).trigger() );
}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  player_t::init_gains();

  gain.chi                   = get_gain( "chi" );
  gain.combo_breaker_savings = get_gain( "combo_breaker_savings" );
  gain.energizing_brew       = get_gain( "energizing_brew" );
  gain.avoided_chi           = get_gain( "chi_from_avoided_attacks" );
  gain.chi_brew              = get_gain( "chi_from_chi_brew" );
  gain.soothing_mist         = get_gain( "Soothing Mist" );
  gain.tier15_2pc            = get_gain( "tier15_2pc" );
}

// monk_t::init_rng =======================================================

void monk_t::init_rng()
{
  player_t::init_rng();

  rng.tier15_2pc            = get_rng( "tier15_2pc" );
  rng.tier15_4pc            = get_rng( "tier15_4pc" );
}

// monk_t::init_procs ======================================================

void monk_t::init_procs()
{
  player_t::init_procs();

  proc.tier15_2pc = get_proc( "tier15_2pc" );
  proc.tier15_4pc = get_proc( "tier15_4pc" );
}

// monk_t::init_actions =====================================================

void monk_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string use_items_str;
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        // This is never used?
        use_items_str += "/use_item,name=";
        use_items_str += items[ i ].name();
      }
    }

    std::string& precombat = get_action_priority_list( "precombat" ) -> action_list_str;
    std::string& aoe_list_str = get_action_priority_list( "aoe" ) -> action_list_str;
    std::string& st_list_str = get_action_priority_list( "st" ) -> action_list_str;

    switch ( specialization() )
    {

    case MONK_BREWMASTER:
      add_action( "Jab" );
      break;

    case MONK_WINDWALKER:

      if ( sim -> allow_flasks )
      {
        // Flask
        precombat += "flask,type=spring_blossoms";
      }

      if ( sim -> allow_food )
      {
        // Food
        precombat += "/food,type=sea_mist_rice_noodles";
      }

      precombat += "/stance";
      precombat += "/snapshot_stats";

      if ( sim -> allow_potions )
      {
        // Prepotion
        if ( level >= 85 )
          precombat += "/virmens_bite_potion";
        else if ( level > 80 )
          precombat += "/tolvir_potion";
      }

      action_list_str += "/auto_attack";
      action_list_str += "/chi_sphere,if=talent.power_strikes.enabled&buff.chi_sphere.react&chi<4";

      if ( sim -> allow_potions )
      {
        if ( level >= 85 )
          action_list_str += "/virmens_bite_potion,if=buff.bloodlust.react|target.time_to_die<=60";
        else if ( level > 80 )
          action_list_str += "/tolvir_potion,if=buff.bloodlust.react|target.time_to_die<=60";
      }

      // PROFS/RACIALS
      action_list_str += init_use_profession_actions();

      // USE ITEM (engineering etc)
      for ( int i = items.size() - 1; i >= 0; i-- )
      {
        if ( items[ i ].use.active() )
        {
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
        }
      }

      action_list_str += init_use_racial_actions();
      action_list_str += "/chi_brew,if=talent.chi_brew.enabled&chi=0";
      action_list_str += "/rising_sun_kick,if=!target.debuff.rising_sun_kick.remains|target.debuff.rising_sun_kick.remains<=3";
      action_list_str += "/tiger_palm,if=buff.tiger_power.remains<=3";
      action_list_str += "/tigereye_brew,if=!buff.tigereye_brew_use.up&buff.tigereye_brew.react=10";
      action_list_str += "/energizing_brew,if=energy.time_to_max>5";
      action_list_str += "/invoke_xuen,if=talent.invoke_xuen.enabled";
      action_list_str += "/rushing_jade_wind,if=talent.rushing_jade_wind.enabled";
      action_list_str += "/run_action_list,name=aoe,if=active_enemies>=5";
      action_list_str += "/run_action_list,name=st,if=active_enemies<5";
      //aoe
      aoe_list_str += "/rising_sun_kick,if=chi=4";
      aoe_list_str += "/spinning_crane_kick";
      //st
      st_list_str += "/blackout_kick,if=buff.combo_breaker_bok.react&energy.time_to_max<2";
      st_list_str += "/blackout_kick,if=(chi>=3&energy.time_to_max<=2&!talent.ascension.enabled)|(chi>=4&energy.time_to_max<=2&talent.ascension.enabled)";
      st_list_str += "/rising_sun_kick";
      st_list_str += "/tiger_palm,if=buff.combo_breaker_tp.react";
      st_list_str += "/fists_of_fury,if=!buff.energizing_brew.up&energy.time_to_max>(4)&buff.tiger_power.remains>(4)";
      st_list_str += "/blackout_kick,if=buff.combo_breaker_bok.react";
      st_list_str += "/jab,if=talent.ascension.enabled&chi<=3";
      st_list_str += "/jab,if=!talent.ascension.enabled&chi<=2";
      st_list_str += "/blackout_kick,if=((energy+(energy.regen*(cooldown.rising_sun_kick.remains)))>=40)|(chi=4&!talent.ascension.enabled)|(chi=5&talent.ascension.enabled)";
      break;

    case MONK_MISTWEAVER:
      break;

    default:
      add_action( "Jab" );
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
      resource_gain( RESOURCE_ENERGY,
                     buff.energizing_brew -> data().effectN( 1 ).base_value() * periodicity.total_seconds(),
                     gain.energizing_brew );
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

  std::string s = item.name();

  if ( util::str_in_str_ci( s, "red_crane" ) )
  {
    if ( util::str_in_str_ci( s, "helm"      ) ||
         util::str_in_str_ci( s, "mantle"    ) ||
         util::str_in_str_ci( s, "vest"      ) ||
         util::str_in_str_ci( s, "legwraps"  ) ||
         util::str_in_str_ci( s, "handwraps" ) )
    {
      return SET_T14_HEAL;
    }

    if ( util::str_in_str_ci( s, "tunic"     ) ||
         util::str_in_str_ci( s, "headpiece" ) ||
         util::str_in_str_ci( s, "leggings"  ) ||
         util::str_in_str_ci( s, "spaulders" ) ||
         util::str_in_str_ci( s, "grips"     ) )
    {
      return SET_T14_MELEE;
    }

    if ( util::str_in_str_ci( s, "chestguard"     ) ||
         util::str_in_str_ci( s, "crown"          ) ||
         util::str_in_str_ci( s, "legguards"      ) ||
         util::str_in_str_ci( s, "shoulderguards" ) ||
         util::str_in_str_ci( s, "gauntlets"      ) )
    {
      return SET_T14_TANK;
    }
  } // end "red_crane"

  if ( util::str_in_str_ci( s, "fire_charm" ) )
  {
    if ( util::str_in_str_ci( s, "helm"      ) ||
         util::str_in_str_ci( s, "mantle"    ) ||
         util::str_in_str_ci( s, "vest"      ) ||
         util::str_in_str_ci( s, "legwraps"  ) ||
         util::str_in_str_ci( s, "handwraps" ) )
    {
      return SET_T15_HEAL;
    }

    if ( util::str_in_str_ci( s, "tunic"     ) ||
         util::str_in_str_ci( s, "headpiece" ) ||
         util::str_in_str_ci( s, "leggings"  ) ||
         util::str_in_str_ci( s, "spaulders" ) ||
         util::str_in_str_ci( s, "grips"     ) )
    {
      return SET_T15_MELEE;
    }

    if ( util::str_in_str_ci( s, "chestguard"     ) ||
         util::str_in_str_ci( s, "crown"          ) ||
         util::str_in_str_ci( s, "legguards"      ) ||
         util::str_in_str_ci( s, "shoulderguards" ) ||
         util::str_in_str_ci( s, "gauntlets"      ) )
    {
      return SET_T15_TANK;
    }
  } // end "fire_charm"

  if ( util::str_in_str_ci( s, "_gladiators_copperskin_"  ) ) return SET_PVP_HEAL;
  if ( util::str_in_str_ci( s, "_gladiators_ironskin_"    ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// monk_t::composite_attack_speed

double monk_t::composite_attack_speed()
{
  double cas = player_t::composite_attack_speed();

  if ( ! dual_wield() )
    cas *= 1.0 / ( 1.0 + spec.way_of_the_monk -> effectN( 2 ).percent() );

  if ( buff.tiger_strikes -> up() )
    cas *= 1.0 / ( 1.0 + buff.tiger_strikes -> data().effectN( 1 ).percent() );

  return cas;
}

// monk_t::composite_player_multiplier

double monk_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( active_stance == STANCE_FIERCE_TIGER )
  {
    m *= 1.0 + buff.tiger_stance -> data().effectN( 3 ).percent();
  }

  m *= 1.0 + buff.tigereye_brew_use -> value();

  return m;
}

// monk_t::create_options =================================================

void monk_t::create_options()
{
  player_t::create_options();

  option_t monk_options[] =
  {
    opt_int( "initial_chi", initial_chi ),
    opt_null()
  };

  option_t::copy( options, monk_options );
}

// monk_t::primary_role ==================================================

resource_e monk_t::primary_resource()
{
  if ( active_stance == STANCE_WISE_SERPENT )
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

// monk_t::energy_regen_per_second ========================================

double monk_t::energy_regen_per_second()
{
  double r = player_t::energy_regen_per_second();

  r *= 1.0 + talent.ascension -> effectN( 3 ).percent();

  return r;
}

void monk_t::combat_begin()
{
  player_t::combat_begin();

  if ( talent.power_strikes -> ok() )
  {
    // Random start of the first tick.
    timespan_t d = sim -> default_rng() -> real() * timespan_t::from_seconds( 20.0 );
    new ( *sim ) power_strikes_event_t( this, d );
  }
}
// MONK MODULE INTERFACE ================================================

struct monk_module_t : public module_t
{
  monk_module_t() : module_t( MONK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    return new monk_t( sim, name, r );
  }
  virtual bool valid() const { return true; }
  virtual void init        ( sim_t* ) const {}
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end  ( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t& module_t::monk_()
{
  static monk_module_t m = monk_module_t();
  return m;
}
