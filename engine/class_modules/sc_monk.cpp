// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//
//  TODO:
//  Add all damaging abilities
//  Ensure values are correct
//  Add mortal wounds to RSK
//  Add all buffs
//
//  Note: RSK 10% buff works, just doesn't parse.
// <@serge> And in the correct place you multiply the damage multiplier by ( 1.0 + buffs.rsk -> data().effectN( 1 ).percent() )

#include "simulationcraft.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace { // ANONYMOUS NAMESPACE

struct monk_t;

enum monk_stance_e { STANCE_DRUNKEN_OX=1, STANCE_FIERCE_TIGER, STANCE_HEAL=4 };

struct monk_td_t : public actor_pair_t
{
  struct debuffs_t
  {
    debuff_t* rising_sun_kick;
    debuff_t* tiger_palm;
  } debuff;

  monk_td_t( player_t*, monk_t* );
};

struct monk_t : public player_t
{
	monk_stance_e active_stance;
  // Buffs
  struct buffs_t
  {
  // TODO: Finish Adding Buffs - will uncomment as implemented
        //  buff_t* buffs_<buffname>;
        //  buff_t* tiger_power;
        //  buff_t* energizing_brew;
        //  buff_t* zen_sphere;
        //  buff_t* fortifying_brew;
        //  buff_t* zen_meditation;
        //  buff_t* path_of_blossoms;
        //  buff_t* tigereye_brew; // Has a stacking component and on-use component using different spell ids.  Can be stacked while use-buff is up.
        //  buff_t* tigereye_brew_use; // will need to check if needed.
        //  buff_t* tiger_strikes;
        //  buff_t* combo_breaker_tp;
        //  buff_t* combo_breaker_bok;
		buff_t* tiger_stance;

		//Debuffs
  } buff;

  // Gains
  struct gains_t
  {
    gain_t* chi;
  } gain;
  // Stances

  // Procs
  struct procs_t
  {
    //proc_t* procs_<procname>;
  } proc;
    //   proc_t* tiger_strikes;
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

        //   const spell_data_t* chi_wave;
        //   const spell_data_t* zen_sphere;
        //   const spell_data_t* chi_burst;

        //   const spell_data_t* power_strikes;
           const spell_data_t* ascension;
        //   const spell_data_t* chi_brew;

        //   const spell_data_t* deadly_reach;
        //   const spell_data_t* charging_ox_wave;
        //   const spell_data_t* leg_sweep;

        //   const spell_data_t* healing_elixers;
        //   const spell_data_t* dampen_harm;
        //   const spell_data_t* diffuse_magic;

        //   const spell_data_t* rushing_jade_wind;
        //   const spell_data_t* invoke_zuen;
        //   const spell_data_t* chi_torpedo;
  } talent;

  // Passives
  struct passives_t
  {

   const spell_data_t* leather_specialization;
   const spell_data_t* way_of_the_monk; //split for DW / 2H

    // TREE_MONK_TANK
    // spell_id_t* mastery/passive spells

    // TREE_MONK_DAMAGE

    // TREE_MONK_HEAL
  } passive;

  // Glyphs
  struct glyphs_t
  {
    // Prime
    //glyph_t* <glyphname>;

    // Major

  } glyph;

  // Options
  int initial_chi;

  target_specific_t<monk_td_t> target_data;

  monk_t( sim_t* sim, const std::string& name, race_e r = RACE_PANDAREN ) :
    player_t( sim, MONK, name, r ),
    buff( buffs_t() ),
    gain( gains_t() ),
    proc( procs_t() ),
    talent( talents_t() ),
    passive( passives_t() ),
    glyph( glyphs_t() ),
    initial_chi( 0 )
  {
    target_data.init( "target_data", this );

    create_options();
  }

  // Character Definition
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      init_resources( bool force=false );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual int       decode_set( item_t& );
  virtual void      create_options();
  virtual resource_e primary_resource();
  virtual role_e primary_role();

  virtual monk_td_t* get_target_data( player_t* target )
  {
    monk_td_t*& td = target_data[ target ];
    if ( ! td ) td = new monk_td_t( target, this );
    return td;
  }

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( primary_tree() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( primary_tree() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_glyphs();
  }

};
namespace {
// ==========================================================================
// Monk Abilities
// ==========================================================================

// Template for common monk action code. See priest_action_t.
template <class Base>
struct monk_action_t : public Base
{
  int stancemask;

  typedef Base action_base_t;
  typedef monk_action_t base_t;

  monk_action_t( const std::string& n, monk_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    action_base_t( n, player, s ),
    stancemask( STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER|STANCE_HEAL )
  {
    action_base_t::may_crit   = true;
    action_base_t::stateless  = true;
  }

  monk_t* p() { return debug_cast<monk_t*>( action_base_t::player ); }

  monk_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : action_base_t::target ); }

  virtual bool ready()
  {
    if ( ! action_base_t::ready() )
      return false;

    // Attack available in current stance?
    if ( ( stancemask & p() -> active_stance ) == 0 )
      return false;

    return true;
  }

};



struct monk_melee_attack_t : public monk_action_t<melee_attack_t>
{
  weapon_t* mh;
  weapon_t* oh;
  monk_melee_attack_t( const std::string& n, monk_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ),
    mh( NULL ), oh( NULL )
  {
    may_glance = false;
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
    // Check for tiger stance and add 20% damage if true
  if ( p() -> active_stance == STANCE_FIERCE_TIGER  )
     {
     		total_dmg *= 1.0 + p() -> buff.tiger_stance -> data().effectN( 3 ).percent();
    		return total_dmg;
      }else{
    	  return total_dmg;
     }

  }

};

struct monk_spell_t : public monk_action_t<spell_t>
{
  monk_spell_t( const std::string& n, monk_t* player,
                const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
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
/* TODO: implement this from druid
struct way_of_the_monk_t : public monk_spell_t
{
	  virtual void execute()
	  {
	    spell_t::execute();

	    weapon_t* w = &( p() -> main_hand_weapon );



	    if ( w -> type =  )
	    {
	      // FIXME: If we really want to model switching between forms, the old values need to be saved somewhere
	      w -> type = WEAPON_BEAST;
	      w -> school = SCHOOL_PHYSICAL;
	      w -> min_dmg /= w -> swing_time.total_seconds();
	      w -> max_dmg /= w -> swing_time.total_seconds();
	      w -> damage = ( w -> min_dmg + w -> max_dmg ) / 2;
	      w -> swing_time = timespan_t::from_seconds( 1.0 );
	    }
};*/

struct jab_t : public monk_melee_attack_t
{
  jab_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "jab", p, p -> find_class_spell( "Jab" ) )
  {
    parse_options( 0, options_str );
    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;

    base_dd_min = base_dd_max = direct_power_mod = 0.0; // deactivate parsed spelleffect1

    mh = &( player -> main_hand_weapon );
    oh = &(player -> off_hand_weapon );

    base_multiplier = 2.52; // hardcoded into tooltip
  }

  virtual void execute()
  {
    monk_melee_attack_t::execute();

    if ( p() -> active_stance  == STANCE_FIERCE_TIGER )
    {
    	// Todo: Add stance buffs and use spell data for the +1 number from fierce_tiger.
    	player -> resource_gain( RESOURCE_CHI,  data().effectN( 2 ).base_value() + 1 , p() -> gain.chi );
    }
    else
    {
    	player -> resource_gain( RESOURCE_CHI,  data().effectN( 2 ).base_value() , p() -> gain.chi );
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
    mh = &(player -> main_hand_weapon) ;
    oh = &(player -> off_hand_weapon) ;
    base_multiplier = 5.0; // hardcoded into tooltip
  }

  virtual void impact_s( action_state_t* s )
  {
    monk_melee_attack_t::impact_s( s );

    td( s -> target ) -> debuff.tiger_palm -> trigger();
  }


};
//=============================
//====Blackout Kick============
//=============================

struct blackout_kick_t : public monk_melee_attack_t
{
  struct  dot_blackout_kick_t : public monk_melee_attack_t
  {
    dot_blackout_kick_t( monk_t* p ) :
      monk_melee_attack_t( "blackout_kick_dot", p, p-> find_spell ( 128531 ) )
    {
      tick_may_crit = true;
      background = true;
      proc = true;
      may_miss = false;
      school = SCHOOL_PHYSICAL;
      dot_behavior = DOT_REFRESH;
    }

    // FIXME: Assuming that the dot damage is not further modified.
    // Needs testing
    virtual double composite_ta_multiplier()
    { return 1.0; }

    //virtual double composite_target_ta_multiplier( player_t* )
    //{ return 1.0; }
  };
	dot_blackout_kick_t* bokdot;

  blackout_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "blackout_kick", p, p -> find_class_spell( "Blackout Kick" ) ),
    bokdot( 0 )
  {
    parse_options( 0, options_str );
    base_dd_min = base_dd_max = 0.0; direct_power_mod = 0.0; //  deactivate parsed spelleffect1
    mh = &( player -> main_hand_weapon );
    oh = &( player -> off_hand_weapon );
    base_multiplier = 12.0; // hardcoded into tooltip

    bokdot = new dot_blackout_kick_t( p );
    add_child( bokdot );
  }

  virtual void assess_damage( player_t* t, double amount, dmg_e type, result_e result )
  {
    monk_melee_attack_t::assess_damage( t, amount, type, result );

    if ( bokdot )
    {
      bokdot -> base_td = ( direct_dmg * data().effectN( 2 ).percent() / bokdot -> num_ticks );
      bokdot -> target = t;
      bokdot -> execute();
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
    mh = &(player -> main_hand_weapon) ;
    oh = &(player -> off_hand_weapon) ;
    base_multiplier = 14.4; // hardcoded into tooltip
  }

//TEST: Mortal Wounds - ADD Later

virtual double action_multiplier()
{
  double m = monk_melee_attack_t::action_multiplier();

  if ( td() -> debuff.rising_sun_kick -> up() )
  {
    m *=  1.0 + td() -> debuff.rising_sun_kick -> data().effectN( 2 ).percent();
  }

  return m;
}
virtual void impact_s( action_state_t* s )
{
  monk_melee_attack_t::impact_s( s );

  td( s -> target ) -> debuff.rising_sun_kick -> trigger();
}

};
//=============================
//====Spinning Crane Kick======
//=============================
struct spinning_crane_kick_tick_t : public monk_melee_attack_t
{
  spinning_crane_kick_tick_t( monk_t* p ) :
    monk_melee_attack_t( "spinning_crane_kick_tick", p )
  {
    background  = true;
    dual        = true;
    direct_tick = true;
    aoe = -1;
  }
};

struct spinning_crane_kick_t : public monk_melee_attack_t
{
  spinning_crane_kick_tick_t* spinning_crane_kick_tick;

  spinning_crane_kick_t( monk_t* p, const std::string& options_str ) :
    monk_melee_attack_t( "spinning_crane_kick", p, p -> find_class_spell( "Spinning Crane Kick" ) ),
    spinning_crane_kick_tick( 0 )
  {
    parse_options( 0, options_str );

    stancemask = STANCE_DRUNKEN_OX|STANCE_FIERCE_TIGER;

    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks = 3;
    tick_zero = true;
    channeled = true;

    spinning_crane_kick_tick = new spinning_crane_kick_tick_t( p );
  }

  virtual void init()
  {
    monk_melee_attack_t::init();

    spinning_crane_kick_tick -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    if ( spinning_crane_kick_tick )
      spinning_crane_kick_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }
};

struct melee_t : public monk_melee_attack_t
{
  int sync_weapons;

  melee_t( const std::string& name, monk_t* player, int sw ) :
    monk_melee_attack_t( name, player, spell_data_t::nil() ), sync_weapons( sw )
  {
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    special     = false;
    school      = SCHOOL_PHYSICAL;
    if ( player -> dual_wield() ) may_glance  = true;
    if ( player -> dual_wield() ) base_hit -= 0.19;
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

};

struct auto_attack_t : public monk_melee_attack_t //used shaman as reference
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

    if ( player -> off_hand_weapon.type != WEAPON_NONE)
    {
      if ( ! player -> dual_wield() ) return;
      player -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      player -> off_hand_attack -> weapon = &( player -> off_hand_weapon );
      player -> off_hand_attack -> base_execute_time = player -> off_hand_weapon.swing_time;
    }

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
        switch_to_stance = STANCE_HEAL;
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
      p() -> buff.tiger_stance -> trigger();

  }

  virtual bool ready()
  {
    if ( p() -> active_stance == switch_to_stance )
      return false;

    return monk_spell_t::ready();
  }
};

} // END ANONYMOUS action NAMESPACE

// ==========================================================================
// Monk Character Definition
// ==========================================================================

monk_td_t::monk_td_t( player_t* target, monk_t* p ) :
  actor_pair_t( target, p ),
  debuff( debuffs_t() )
{
  debuff.rising_sun_kick = buff_creator_t( *this, "rising_sun_kick" ).spell( p -> find_class_spell( "Rising Sun Kick" ) );
  debuff.tiger_palm = buff_creator_t( *this, "tiger_power" ).spell( p -> find_spell( 125359 ) );
}
// monk_t::create_action ====================================================

action_t* monk_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  if ( name == "auto_attack"         ) return new         auto_attack_t( this, options_str );
  if ( name == "jab"                 ) return new                 jab_t( this, options_str );
  if ( name == "tiger_palm"          ) return new          tiger_palm_t( this, options_str );
  if ( name == "blackout_kick"       ) return new       blackout_kick_t( this, options_str );
  if ( name == "spinning_crane_kick" ) return new spinning_crane_kick_t( this, options_str );
  if ( name == "rising_sun_kick"     ) return new     rising_sun_kick_t( this, options_str );
  if ( name == "stance"              ) return new              stance_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  player_t::init_spells();

  //TALENTS
  talent.ascension = find_talent_spell( "Ascension" );


  // Add Spells & Glyphs

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    {      0,      0,     0,     0,     0,     0,     0,     0 }, // Tier13
    {      0,      0,     0,     0,     0,     0,     0,     0 }, // Tier14
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// monk_t::init_base ========================================================

void monk_t::init_base()
{
  player_t::init_base();

  int tree = primary_tree();

  initial.distance = ( tree == MONK_MISTWEAVER ) ? 40 : 3;

  base_gcd = timespan_t::from_seconds( 1.0 );

  resources.base[  RESOURCE_CHI  ] = 4 + talent.ascension -> effectN( 1 ).base_value();


  base_chi_regen_per_second = 0; //

  if ( tree == MONK_MISTWEAVER )
        active_stance = STANCE_HEAL;
    else if ( tree == MONK_BREWMASTER )
        active_stance = STANCE_DRUNKEN_OX;



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

  buff.tiger_stance = buff_creator_t( this, "tiger_stance" ).spell( find_spell(103985) );
}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  player_t::init_gains();

  gain.chi = get_gain( "chi" );
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

    switch ( primary_tree() )
    {
    case MONK_BREWMASTER:
    case MONK_WINDWALKER:
    case MONK_MISTWEAVER:
    default:
      // Flask
      if ( level > 85 )
        action_list_str += "/flask,type=warm_sun,precombat=1";
      	  else if ( level >= 80 && primary_tree() == MONK_MISTWEAVER)
    		  action_list_str += "/flask,type=draconic_mind,precombat=1";
    	  else
    		  action_list_str += "/flask,type=winds,precombat=1";

      // Food
      if ( level > 85 )
      {
        action_list_str += "/food,type=great_pandaren_banquet,precombat=1";
      }
      else if ( level > 80 )
      {
        action_list_str += "/food,type=seafood_magnifique_feast,precombat=1";
      }
      action_list_str += "/stance,precombat=1";
      action_list_str += "/snapshot_stats,precombat=1";
      action_list_str += "/auto_attack";
      action_list_str += "/rising_sun_kick";
      action_list_str += "/blackout_kick,if=debuff.tiger_power.stack>=3";
      action_list_str += "/tiger_palm";
      action_list_str += "/blackout_kick";
      action_list_str += "/jab";


      break;
    }
  }

  player_t::init_actions();
}

// monk_t::reset ==================================================

void monk_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_CHI ] = initial_chi;
}

// monk_t::matching_gear_multiplier =========================================

double monk_t::matching_gear_multiplier( attribute_e attr )
{
  if ( primary_tree() == MONK_MISTWEAVER )
  {
    if ( attr == ATTR_INTELLECT )
      return 0.05;
  }
  else if ( attr == ATTR_AGILITY )
    return 0.05;

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

  //const char* s = item.name();

  //if ( strstr( s, "<setname>"      ) ) return SET_T14_TANK;
  //if ( strstr( s, "<setname>"      ) ) return SET_T14_MELEE;
  //if ( strstr( s, "<setname>"      ) ) return SET_T14_HEAL;

  return SET_NONE;
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
  if ( primary_tree() == MONK_MISTWEAVER )
    return RESOURCE_MANA;

  return RESOURCE_CHI;
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

  if ( primary_tree() == MONK_BREWMASTER )
    return ROLE_TANK;

  if ( primary_tree() == MONK_MISTWEAVER )
    return ROLE_HEAL;

  return ROLE_HYBRID;
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

} // ANONYMOUS NAMESPACE

module_t* module_t::monk()
{
  static module_t* m = 0;
  if ( ! m ) m = new monk_module_t();
  return m;
}
