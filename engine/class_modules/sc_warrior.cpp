// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
//
// TODO:
//  Later:
//   * Move the bleeds out of being warrior_attack_t to stop them
//     triggering effects or having special cases in the class.
//
// ==========================================================================

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Warrior
// ==========================================================================

struct warrior_t;

enum warrior_stance { STANCE_BATTLE = 1, STANCE_BERSERKER = 2, STANCE_DEFENSE = 4 };

struct warrior_td_t : public actor_pair_t
{
  dot_t* dots_bloodbath;
  dot_t* dots_deep_wounds;

  buff_t* debuffs_colossus_smash;
  buff_t* debuffs_demoralizing_shout;

  warrior_td_t( player_t* target, warrior_t* p );
};

struct warrior_t : public player_t
{
public:
  int initial_rage;

  // Active
  action_t* active_bloodbath_dot;
  action_t* active_deep_wounds;
  action_t* active_opportunity_strike;
  action_t* active_second_wind;
  attack_t* active_sweeping_strikes;

  heal_t* active_t16_2pc;
  warrior_stance active_stance;

  // Buffs
  struct buffs_t
  {
    buff_t* avatar;
    buff_t* battle_stance;
    buff_t* berserker_rage;
    buff_t* berserker_stance;
    buff_t* bloodbath;
    buff_t* bloodsurge;
    buff_t* defensive_stance;
    buff_t* enrage;
    buff_t* enraged_speed;
    buff_t* glyph_hold_the_line;
    buff_t* glyph_incite;
    buff_t* last_stand;
    buff_t* meat_cleaver;
    stat_buff_t* riposte;
    buff_t* raging_blow;
    buff_t* raging_whirlwind;
    buff_t* raging_wind;
    buff_t* recklessness;
    buff_t* rude_interruption;
    absorb_buff_t* shield_barrier;
    buff_t* shield_block;
    buff_t* shield_wall;
    buff_t* sudden_execute;
    buff_t* sweeping_strikes;
    buff_t* sword_and_board;
    buff_t* taste_for_blood;
    buff_t* ultimatum;
    buff_t* death_sentence;
    buff_t* tier16_reckless_defense;
    haste_buff_t* flurry;
    //check
    buff_t* tier15_2pc_tank;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* colossus_smash;
    cooldown_t* revenge;
    cooldown_t* shield_slam;
    cooldown_t* mortal_strike;
    cooldown_t* strikes_of_opportunity;
    cooldown_t* rage_from_crit_block;
    cooldown_t* stance_swap;
  } cooldown;

  // Gains
  struct gains_t
  {
    gain_t* avoided_attacks;
    gain_t* battle_shout;
    gain_t* berserker_stance;
    gain_t* bloodthirst;
    gain_t* bull_rush;
    gain_t* charge;
    gain_t* commanding_shout;
    gain_t* critical_block;
    gain_t* defensive_stance;
    gain_t* enrage;
    gain_t* melee_main_hand;
    gain_t* melee_off_hand;
    gain_t* mortal_strike;
    gain_t* raging_whirlwind;
    gain_t* revenge;
    gain_t* shield_slam;
    gain_t* sweeping_strikes;
    gain_t* sword_and_board;

    gain_t* tier15_4pc_tank;
    gain_t* tier16_2pc_melee;
    
    gain_t* tier16_4pc_tank;
  } gain;

  // Spells
  struct spells_t
  {
    const spell_data_t* colossus_smash;
  } spell;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* bull_rush;
    const spell_data_t* colossus_smash;
    const spell_data_t* death_from_above;
    const spell_data_t* enraged_speed;
    const spell_data_t* furious_sundering;
    const spell_data_t* heavy_repercussions;
    const spell_data_t* hold_the_line;
    const spell_data_t* incite;
    const spell_data_t* raging_whirlwind;
    const spell_data_t* raging_wind;
    const spell_data_t* recklessness;
    const spell_data_t* resonating_power;
    const spell_data_t* rude_interruption;
    const spell_data_t* shield_wall;
    const spell_data_t* sweeping_strikes;
    const spell_data_t* unending_rage;
    const spell_data_t* victory_rush;
  } glyphs;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* critical_block;
    const spell_data_t* strikes_of_opportunity;
    const spell_data_t* unshackled_fury;
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* raging_blow_wasted;
    proc_t* taste_for_blood_wasted;
    proc_t* strikes_of_opportunity;
    proc_t* sudden_death;
    proc_t* t15_2pc_melee;
  } proc;

    real_ppm_t t15_2pc_melee;

  // Spec Passives
  struct spec_t
  {
    const spell_data_t* bastion_of_defense;
    const spell_data_t* blood_and_thunder;
    const spell_data_t* bloodsurge;
    const spell_data_t* crazed_berserker;
    const spell_data_t* flurry;
    const spell_data_t* meat_cleaver;
    const spell_data_t* riposte;
    const spell_data_t* seasoned_soldier;
    const spell_data_t* single_minded_fury;
    const spell_data_t* sudden_death;
    const spell_data_t* sword_and_board;
    const spell_data_t* taste_for_blood;
    const spell_data_t* ultimatum;
    const spell_data_t* unwavering_sentinel;
  } spec;

  // Talents
  struct talents_t
  {
    const spell_data_t* juggernaut;
    const spell_data_t* double_time;
    const spell_data_t* warbringer;

    const spell_data_t* enraged_regeneration;
    const spell_data_t* second_wind;
    const spell_data_t* impending_victory;

    const spell_data_t* staggering_shout;
    const spell_data_t* piercing_howl;
    const spell_data_t* disrupting_shout;

    const spell_data_t* bladestorm;
    const spell_data_t* shockwave;
    const spell_data_t* dragon_roar;

    const spell_data_t* mass_spell_reflection;
    const spell_data_t* safeguard;
    const spell_data_t* vigilance;

    const spell_data_t* avatar;
    const spell_data_t* bloodbath;
    const spell_data_t* storm_bolt;
  } talents;

  warrior_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, WARRIOR, name, r ),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    glyphs( glyphs_t() ),
    mastery( mastery_t() ),
    proc( procs_t() ),
    t15_2pc_melee( *this ),
    spec( spec_t() ),
    talents( talents_t() )
  {
    // Active
    active_bloodbath_dot      = 0;
    active_deep_wounds        = 0;
    active_opportunity_strike = 0;
    active_sweeping_strikes   = 0;
    active_second_wind        = 0;
    active_t16_2pc            = 0;
    active_stance             = STANCE_BATTLE;

    // Cooldowns
    cooldown.colossus_smash           = get_cooldown( "colossus_smash"            );
    cooldown.mortal_strike            = get_cooldown( "mortal_strike"             );
    cooldown.shield_slam              = get_cooldown( "shield_slam"               );
    cooldown.strikes_of_opportunity   = get_cooldown( "strikes_of_opportunity"    );
    cooldown.revenge                  = get_cooldown( "revenge"                   );
    cooldown.rage_from_crit_block     = get_cooldown( "rage_from_crit_block"      );
    cooldown.rage_from_crit_block     -> duration = timespan_t::from_seconds( 3.0 );
    cooldown.stance_swap              = get_cooldown( "stance_swap"               );
    cooldown.stance_swap              -> duration = timespan_t::from_seconds( 1.5 );

    initial_rage = 0;

    base.distance = 3.0;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_defense();
  virtual void      init_base_stats();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      combat_begin();
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual double    composite_block() const;
  virtual double    composite_crit_block() const;
  virtual double    composite_crit_avoidance() const;
  virtual double    composite_dodge() const;
  virtual double    composite_melee_speed() const;
  virtual double    composite_rating_multiplier( rating_e rating ) const;
  virtual void      reset();
  virtual void      regen( timespan_t periodicity );
  virtual void      create_options();
  virtual action_t* create_proc_action( const std::string& name );
  virtual bool      create_profile( std::string& profile_str, save_e type, bool save_html );
  virtual void      invalidate_cache( cache_e );

  void              apl_precombat();
  void              apl_default();
  void              apl_smf_fury();
  void              apl_tg_fury();
  void              apl_arms();
  void              apl_prot();
  virtual void      init_action_list();

  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual set_e       decode_set( const item_t& ) const;
  virtual resource_e primary_resource() const { return RESOURCE_RAGE; }
  virtual role_e    primary_role() const;
  virtual void      assess_damage( school_e, dmg_e, action_state_t* s );
  virtual void      copy_from( player_t* source );

  // Custom Warrior Functions
  void enrage();

  target_specific_t<warrior_td_t*> target_data;

  virtual warrior_td_t* get_target_data( player_t* target ) const
  {
    warrior_td_t*& td = target_data[ target ];

    if ( ! td )
    {
      td = new warrior_td_t( target, const_cast<warrior_t*>(this) );
    }
    return td;
  }
};

namespace { // UNNAMED NAMESPACE

// Template for common warrior action code. See priest_action_t.
template <class Base>
struct warrior_action_t : public Base
{
  int stancemask;

private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef warrior_action_t base_t;

  warrior_action_t( const std::string& n, warrior_t* player,
                    const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    stancemask( STANCE_BATTLE | STANCE_BERSERKER | STANCE_DEFENSE )
  {
    ab::may_crit   = true;
  }

  virtual ~warrior_action_t() {}

  warrior_t* cast()
  { return debug_cast<warrior_t*>( ab::player ); }
  const warrior_t* cast() const
  { return debug_cast<warrior_t*>( ab::player ); }

  warrior_td_t* cast_td( player_t* t ) const
  { return cast() -> get_target_data( t ); }

  virtual bool ready()
  {
    if ( ! ab::ready() )
      return false;

    // Attack available in current stance?
    if ( ( stancemask & cast() -> active_stance ) == 0 )
      return false;

    return true;
  }
};

// ==========================================================================
// Warrior Attack
// ==========================================================================

struct warrior_attack_t : public warrior_action_t< melee_attack_t >
{
  warrior_attack_t( const std::string& n, warrior_t* p,
                    const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    may_crit   = true;
    may_glance = false;
    special    = true;
  }

  virtual double target_armor( player_t* t ) const
  {
    double a = base_t::target_armor( t );

    a *= 1.0 - cast_td( t ) -> debuffs_colossus_smash -> current_value;

    return a;
  }

  virtual void   consume_resource();

  virtual void   execute();

  virtual void   impact( action_state_t* s );

  virtual double calculate_weapon_damage( double attack_power )
  {
    double dmg = base_t::calculate_weapon_damage( attack_power );

    // Catch the case where weapon == 0 so we don't crash/retest below.
    if ( dmg == 0.0 )
      return 0;

    const warrior_t* p = cast();

    if ( weapon -> slot == SLOT_OFF_HAND )
    {
      dmg *= 1.0 + p -> spec.crazed_berserker -> effectN( 2 ).percent();

      if ( p -> dual_wield() )
      {
        if ( p -> main_hand_weapon.group() == WEAPON_1H &&
             p -> off_hand_weapon.group() == WEAPON_1H )
          dmg *= 1.0 + p -> spec.single_minded_fury -> effectN( 2 ).percent();
      }
    }

    return dmg;
  }


  virtual double action_multiplier() const
  {
    double am = base_t::action_multiplier();

    const warrior_t* p = cast();

    if ( ( weapon &&  weapon -> group() == WEAPON_2H && this -> id != 115767 ) || this -> id == 137597 ) //hack to let Deep wounds *not* and the Meta GEM benefit from seasoned soldier
      am *= 1.0 + p -> spec.seasoned_soldier -> effectN( 1 ).percent();

    // --- Enrages ---
    if ( dbc::is_school( school, SCHOOL_PHYSICAL ) )
    {
      if ( p -> buff.enrage -> up() )
      {
        am *= 1.0 + p -> buff.enrage -> data().effectN( 2 ).percent();

        if ( p -> mastery.unshackled_fury -> ok() )
          am *= 1.0 + p -> cache.mastery_value();
      }
    }

    // --- Passive Talents ---
    if ( ( p -> spec.single_minded_fury -> ok() && p -> dual_wield() && this -> id != 115767 )  || this -> id == 137597  ) //hack to let Deep wounds *not* and the Meta GEM benefit from SMF
    {
      if (  p -> main_hand_weapon .group() == WEAPON_1H &&
            p -> off_hand_weapon .group() == WEAPON_1H )
      {
        am *= 1.0 + p -> spec.single_minded_fury -> effectN( 1 ).percent();
      }
    }

    // --- Buffs / Procs ---
    if ( p -> buff.rude_interruption -> up() )
      am *= 1.0 + p -> buff.rude_interruption -> value();

    return am;
  }

  virtual double composite_crit() const
  {
    double cc = base_t::composite_crit();

    const warrior_t* p = cast();

    if ( special && this -> id != 115767 ) // Recklessness crit bonus does not count towards deep wounds.
      cc += p -> buff.recklessness -> value();

    if ( p -> sets.has_set_bonus( SET_T15_4PC_MELEE ) && p -> buffs.skull_banner -> check() && p -> buffs.skull_banner -> source == p )
    {
      cc += p -> sets.set( SET_T15_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return cc;
  }

  // helper functions

  void trigger_bloodbath_dot( player_t* t, double dmg )
  {
    warrior_t& p = *cast();

    if ( ! p.buff.bloodbath -> up() ) return;

    ignite::trigger_pct_based(
      p.active_bloodbath_dot, // ignite spell
      t, // target
      p.buff.bloodbath -> data().effectN( 1 ).percent() * dmg );
  }

  void trigger_rage_gain()
  {
    // MoP: base rage gain is 1.75 * weaponspeed and half that for off-hand
    // Battle stance: +100%
    // Berserker stance: no change
    // Defensive stance: -100%

    if ( proc )
      return;

    warrior_t& p = *cast();
    weapon_t*  w = weapon;

    if ( p.active_stance == STANCE_DEFENSE )
      return;

    double rage_gain = 1.75 * w -> swing_time.total_seconds();

    if ( p.buff.raging_whirlwind -> check() )
      rage_gain *= 1.0 + p.buff.raging_whirlwind -> data().effectN( 2 ).percent();
    else if ( p.active_stance == STANCE_BATTLE )
      rage_gain *= 1.0 + p.buff.battle_stance -> data().effectN( 1 ).percent();
    else if ( p.active_stance == STANCE_DEFENSE )
      rage_gain *= 1.0 + p.buff.defensive_stance -> data().effectN( 3 ).percent();

    if ( w -> slot == SLOT_OFF_HAND )
      rage_gain /= 2.0;

    rage_gain = floor( rage_gain * 10 ) / 10.0;

    p.resource_gain( RESOURCE_RAGE,
                     rage_gain,
                     w -> slot == SLOT_OFF_HAND ? p.gain.melee_off_hand : p.gain.melee_main_hand );
  }

  void trigger_taste_for_blood( int stacks_to_increase )
  {
    warrior_t& p = *cast();

    int stacks_wasted = stacks_to_increase - ( p.buff.taste_for_blood -> max_stack() - p.buff.taste_for_blood -> check() );

    for ( int i = 0; i < stacks_wasted; ++i )
      p.proc.taste_for_blood_wasted -> occur();

    p.buff.taste_for_blood -> trigger( stacks_to_increase );
  }
};

// trigger_bloodbath ========================================================

struct bloodbath_dot_t : public ignite::pct_based_action_t< attack_t >
{
  bloodbath_dot_t( warrior_t* p ) :
    base_t( "bloodbath", p, p -> find_spell( 113344 ) )
  {
    background = true;
    dual = true;
  }
};

// ==========================================================================
// Static Functions
// ==========================================================================

// trigger_sudden_death =====================================================

static void trigger_sudden_death( warrior_attack_t* a, double chance )
{
  warrior_t* p = a -> cast();

  if ( a -> proc )
    return;

  if ( p -> rng().roll ( chance ) )
  {
    p -> cooldown.colossus_smash -> reset( true );
    p -> proc.sudden_death       -> occur();
  }
}

// trigger_strikes_of_opportunity ===========================================

struct opportunity_strike_t : public warrior_attack_t
{
  opportunity_strike_t( warrior_t* p ) :
    warrior_attack_t( "opportunity_strike", p, p -> find_spell( 76858 ) )
  {
    background = true;
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    warrior_t* p = cast();

    if ( result_is_hit( s -> result ) )
    {
      trigger_sudden_death( this, p -> spec.sudden_death -> proc_chance() );
    }
  }
};

static void trigger_strikes_of_opportunity( warrior_attack_t* a )
{
  if ( a -> proc )
    return;

  warrior_t* p = a -> cast();

  if ( ! p -> mastery.strikes_of_opportunity -> ok() )
    return;

  if ( p -> cooldown.strikes_of_opportunity -> down() )
    return;

  double chance = p -> cache.mastery_value();

  if ( ! p -> rng().roll( chance ) )
    return;

  p -> cooldown.strikes_of_opportunity -> start( timespan_t::from_seconds( 0.1 ) ); //Tested 8/29/13, confirmed in game data.

  assert( p -> active_opportunity_strike );

  if ( p -> sim -> debug )
    p -> sim -> out_debug.printf( "Opportunity Strike procced from %s", a -> name() );

  p -> proc.strikes_of_opportunity -> occur();
  p -> active_opportunity_strike -> execute();
}

// trigger_sweeping_strikes =================================================
static  void trigger_sweeping_strikes( action_state_t* s )
{
  struct sweeping_strikes_attack_t : public warrior_attack_t
  {
    sweeping_strikes_attack_t( warrior_t* p ) :
      warrior_attack_t( "sweeping_strikes_attack", p, p -> find_spell( 12328 ) )
    {
      may_miss = may_dodge = may_parry = may_crit = callbacks = false;
      background                 = true;
      range                      = 5;
      aoe                        = 1;
      base_costs[ RESOURCE_RAGE] = 0;     //Resource consumption already accounted for in the buff application.
      base_multiplier            = 0.50;  //Hotfixed 9/10, reduction from 75% damage to 50% damage.
    }

  virtual timespan_t travel_time() const
  {
  // It's possible for sweeping strikes and opportunity strikes to proc off each other into infinity as long as the rng.roll on opportunity strikes returns true. 
  // Sweeping strikes has a 1 second "travel time" in game. 
    return timespan_t::from_seconds( 1 );
  }

  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    tl.clear();

    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      player_t* t = sim -> actor_list[ i ];

      if ( ! t -> is_sleeping() && t -> is_enemy() && ( t != target ) )
        tl.push_back( t );
    }

    return tl.size();
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    warrior_t *p = cast();

    if ( result_is_hit( s -> result ) )
      p -> resource_gain( RESOURCE_RAGE, p -> glyphs.sweeping_strikes -> ok() ? p -> glyphs.sweeping_strikes -> effectN( 1 ).base_value() : 0 ,  p -> gain.sweeping_strikes );
  }
};

  warrior_t* p = debug_cast< warrior_t* >( s -> action -> player );

  if ( ! p -> buff.sweeping_strikes -> check() )
    return;

  if ( ! s -> action -> weapon )
    return;

  if ( ! s -> action -> result_is_hit( s -> result ) )
    return;

  if ( s -> action -> sim -> active_enemies == 1 )
    return;

  if ( ! p -> active_sweeping_strikes )
  {
    p -> active_sweeping_strikes = new sweeping_strikes_attack_t( p );
    p -> active_sweeping_strikes -> init();
  }

  p -> active_sweeping_strikes -> base_dd_min = s -> result_amount;
  p -> active_sweeping_strikes -> base_dd_max = s -> result_amount;
  p -> active_sweeping_strikes -> execute();

  return;
}

// trigger_t15_2pc_melee ====================================================

static bool trigger_t15_2pc_melee( warrior_attack_t* a )
{
  if ( ! a -> player -> sets.has_set_bonus( SET_T15_2PC_MELEE ) )
    return false;

  warrior_t* p = a -> cast();

  bool procced;

  if ( ( procced = p -> t15_2pc_melee.trigger( *a ) ) != false )
  {
    p -> proc.t15_2pc_melee -> occur();
    p -> enrage();
  }

  return procced;
}

// trigger_flurry ===========================================================

static void trigger_flurry( warrior_attack_t* a, int stacks )
{
  warrior_t* p = a -> cast();

  if ( stacks >= 0 )
    p -> buff.flurry -> trigger( stacks );
  else
    p -> buff.flurry -> decrement();
}

// ==========================================================================
// Warrior Attacks
// ==========================================================================

// warrior_attack_t::consume_resource =======================================

void warrior_attack_t::consume_resource()
{
  base_t::consume_resource();
  warrior_t* p = cast();

  if ( proc )
    return;

  double rage = resource_consumed;

   if ( result_is_miss( execute_state -> result ) && rage > 0 && !aoe )
     p -> resource_gain( RESOURCE_RAGE, rage*0.8, p -> gain.avoided_attacks );
}

// warrior_attack_t::execute ================================================

void warrior_attack_t::execute()
{
  warrior_t* p = cast();

  base_t::execute();

  if ( proc ) return;
  // Slam sweeping strikes may actually result in a situation, where there are 
  // no targets to hit. In this case, there will be no execute state.
  if ( ! execute_state ) return;

  if ( execute_state -> result == RESULT_DODGE && p -> specialization() == WARRIOR_ARMS )
  {
    trigger_taste_for_blood( p -> spec.taste_for_blood -> effectN( 1 ).base_value() );
  }
}
// warrior_attack_t::impact =================================================

void warrior_attack_t::impact( action_state_t* s )
{
  base_t::impact( s );
  warrior_t* p     = cast();
  warrior_td_t* td = cast_td( s -> target );

  if ( result_is_hit( s -> result ) && !proc && s -> result_amount > 0 && this -> id != 147891 ) // Flurry of Xuen
  {
    trigger_strikes_of_opportunity( this );
    if ( p -> buff.sweeping_strikes -> up() && !aoe )
      trigger_sweeping_strikes( execute_state );
    if ( special )
    {
      if( p -> buff.bloodbath -> up() )
        trigger_bloodbath_dot( s -> target, s -> result_amount );
      if ( p -> sets.has_set_bonus( SET_T16_2PC_MELEE ) && td ->  debuffs_colossus_smash -> up() && // Melee tier 16 2 piece.
         ( this ->  weapon == &( p -> main_hand_weapon ) || this -> id == 100130 ) &&    // Only procs once per ability used.
           this -> id != 12328 && this -> id != 76858 )                                  // Doesn't proc from opportunity strikes or sweeping strikes.
        p -> resource_gain( RESOURCE_RAGE,
                            p -> sets.set( SET_T16_2PC_MELEE ) -> effectN( 1 ).base_value(), 
                            p -> gain.tier16_2pc_melee );
    }

    trigger_flurry( this, 3 );
  }
}

// Melee Attack =============================================================

struct melee_t : public warrior_attack_t
{
  int sync_weapons;
  bool first;

  melee_t( const std::string& name, warrior_t* p, int sw ) :
    warrior_attack_t( name, p, spell_data_t::nil() ),
    sync_weapons( sw ),  first( true )
  {
    school      = SCHOOL_PHYSICAL;
    may_glance  = true;
    special     = false;
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();

    if ( p -> dual_wield() ) base_hit -= 0.19;
  }

  void reset()
  {
    warrior_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = warrior_attack_t::execute_time();
    if ( first )
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    else
      return t;
  }

  virtual void execute()
  {
    // Be careful changing where this is done.  Flurry that procs from melee
    // must be applied before the (repeating) event schedule, and the decrement
    // here must be done before it.
    trigger_flurry( this, -1 );
    if ( first )
      first = false;

    warrior_attack_t::execute();
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    warrior_t* p = cast();

    if ( result_is_hit( s -> result ) )
    {
      if ( p -> specialization() == WARRIOR_ARMS )
        trigger_sudden_death( this,  p -> spec.sudden_death -> proc_chance() );
      trigger_t15_2pc_melee( this );
      trigger_rage_gain();
    }
  }

  virtual double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    const warrior_t* p = cast();

    if ( p -> specialization() == WARRIOR_FURY )
      am *= 1.0 + p -> spec.crazed_berserker -> effectN( 3 ).percent();

    return am;
  }
};

// Off-hand test attack =====================================================
// This class is used by several attacks which use both weapons (e.g. Raging Blow)

struct off_hand_test_attack_t : public warrior_attack_t
{
  result_e last_result;
  off_hand_test_attack_t( warrior_t* p, const char* name ) :
    warrior_attack_t( name, p ), last_result( RESULT_NONE )
  {
    background        = true;
    weapon            = &( p -> off_hand_weapon );
    trigger_gcd       = timespan_t::zero();
    weapon_multiplier = 0.0;
    direct_power_mod  = 0.0;
    proc              = false; // disable all procs for this attack
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    last_result = execute_state -> result;
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public warrior_attack_t
{
  int sync_weapons;

  auto_attack_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "auto_attack", p ),
    sync_weapons( 0 )
  {
    option_t options[] =
    {
      opt_bool( "sync_weapons", sync_weapons ),
      opt_null()
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      p -> off_hand_attack -> id = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    p -> main_hand_attack -> schedule_execute();

    if ( p -> off_hand_attack )
      p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> is_moving() )
      return false;

    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Bladestorm ===============================================================

struct bladestorm_tick_t : public warrior_attack_t
{
  bladestorm_tick_t( warrior_t* p, const std::string& name ) :
    warrior_attack_t( name, p, p -> find_spell( 50622 ) )
  {
    background = may_miss = may_parry = may_dodge = direct_tick = true;
    aoe         = -1;
    if ( p -> specialization() == WARRIOR_ARMS )       //Bladestorm does 1.5x more damage as Arms with 5.4
      weapon_multiplier *= 1.5;

    if ( p -> specialization() == WARRIOR_PROTECTION ) //Bladestorm does 4/3 more damage as protection with 5.4
      weapon_multiplier *= 4/3;
  }
};

struct bladestorm_t : public warrior_attack_t
{
  attack_t* bladestorm_mh;
  attack_t* bladestorm_oh;

  bladestorm_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "bladestorm", p, p -> talents.bladestorm ),
    bladestorm_mh( new bladestorm_tick_t( p, "bladestorm_mh" ) ),
    bladestorm_oh( 0 )
  {
    parse_options( NULL, options_str );

    harmful   = false;
    channeled = true;
    tick_zero = true;

    bladestorm_mh -> weapon = &( player -> main_hand_weapon );
    add_child( bladestorm_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      bladestorm_oh = new bladestorm_tick_t( p, "bladestorm_oh" );
      bladestorm_oh -> weapon = &( player -> off_hand_weapon );
      add_child( bladestorm_oh );
    }
  }

  virtual void tick( dot_t* d )
  {
    warrior_attack_t::tick( d );

    bladestorm_mh -> execute();

    if ( bladestorm_mh -> result_is_hit( execute_state -> result ) && bladestorm_oh )
    {
      bladestorm_oh -> execute();
    }
  }

  // Bladestorm is not modified by haste effects
  virtual double composite_haste() const { return 1.0; }
};

// Bloodthirst Heal =========================================================

struct bloodthirst_heal_t : public heal_t
{
  double pct_heal;

  bloodthirst_heal_t( warrior_t* p ) :
    heal_t( "bloodthirst_heal", p, p -> find_class_spell( "Bloodthirst" ) )
  {
    // Implemented as an actual heal because of spell callbacks ( for Hurricane, etc. )
    background = true;
    target     = p;
    may_crit   = false;
    pct_heal   = p -> find_spell( 117313 ) -> effectN( 1 ).percent();
  }

  virtual resource_e current_resource() const { return RESOURCE_NONE; }

};

// Bloodthirst ==============================================================

struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_heal_t* bloodthirst_heal;

  bloodthirst_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "bloodthirst", p, p -> find_class_spell( "Bloodthirst" ) ),
    bloodthirst_heal( NULL )
  {
    parse_options( NULL, options_str );

    weapon           = &( p -> main_hand_weapon );
    bloodthirst_heal = new bloodthirst_heal_t( p );

    base_multiplier += p -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 2 ).percent();
  }

  double composite_crit_multiplier() const
  {
    double c = warrior_attack_t::composite_crit_multiplier();

    c *= 2;

    return c;
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( bloodthirst_heal )
        bloodthirst_heal -> execute();

      warrior_t* p = cast();

      p -> active_deep_wounds -> target = s -> target;
      p -> active_deep_wounds -> execute();
      p -> buff.bloodsurge -> trigger( 3 );

      if ( p -> sets.has_set_bonus( SET_T16_4PC_MELEE ) )
        p -> buff.death_sentence -> trigger();

      p -> resource_gain( RESOURCE_RAGE,
                          data().effectN( 3 ).resource( RESOURCE_RAGE ),
                          p -> gain.bloodthirst );

      if ( s -> result == RESULT_CRIT )
        p -> enrage();
    }
  }
};

// Charge ===================================================================

struct charge_t : public warrior_attack_t
{
  int use_in_combat;

  charge_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "charge", p, p -> find_class_spell( "Charge" ) ),
    use_in_combat( 0 ) // For now it's not usable in combat by default because we can't model the distance/movement.
  {
    option_t options[] =
    {
      opt_bool( "use_in_combat", use_in_combat ),
      opt_null()
    };
    parse_options( options, options_str );

    cooldown -> duration += p -> talents.juggernaut -> effectN( 3 ).time_value();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    if ( p -> position() == POSITION_RANGED_FRONT )
      p -> change_position( POSITION_FRONT );
    else if ( ( p -> position() == POSITION_RANGED_BACK ) || ( p -> position() == POSITION_MAX ) )
      p -> change_position( POSITION_BACK );

    p -> resource_gain( RESOURCE_RAGE,
                        data().effectN( 2 ).resource( RESOURCE_RAGE ),
                        p -> gain.charge );
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> in_combat )
    {
      if ( ! use_in_combat )
        return false;

      if ( ( p -> position() == POSITION_BACK ) || ( p -> position() == POSITION_FRONT ) )
      {
        return false;
      }
    }

    return warrior_attack_t::ready();
  }
};

// Cleave ===================================================================

struct cleave_t : public warrior_attack_t
{
  cleave_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "cleave", p, p -> find_class_spell( "Cleave" ) )
  {
    parse_options( NULL, options_str );

    weapon = &( player -> main_hand_weapon );
    // The 140% is hardcoded in the tooltip
    if ( weapon -> group() == WEAPON_1H ||
         weapon -> group() == WEAPON_SMALL )
      base_multiplier *= 1.40;

    aoe = 2;

    normalize_weapon_speed = false;
    use_off_gcd = true;
  }

  virtual double cost() const
  {
    double c = warrior_attack_t::cost();
    const warrior_t* p = cast();

    if ( p -> buff.glyph_incite -> check() )
      c *= 1 + p -> buff.glyph_incite -> data().effectN( 1 ).percent();

    if ( p -> buff.ultimatum -> check() )
      c *= 1 + p->buff.ultimatum -> data().effectN( 1 ).percent();

    return c;
  }

  virtual double crit_chance( double crit, int delta_level ) const
  {
    double cc = warrior_attack_t::crit_chance( crit, delta_level );

    const warrior_t* p = cast();

    if ( p -> buff.ultimatum -> check() )
      cc += p -> buff.ultimatum -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    warrior_attack_t::execute();

    p -> buff.ultimatum -> expire();
    p -> buff.glyph_incite -> decrement();
  }

};

// Colossus Smash ===========================================================

struct colossus_smash_t : public warrior_attack_t
{
  colossus_smash_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "colossus_smash",  p, p -> spell.colossus_smash )
  {
    parse_options( NULL, options_str );

    weapon = &( player -> main_hand_weapon );
  }

  virtual timespan_t travel_time() const
  {
    // Dirty hack to ensure you can fit 3 x 1.5 + 2 x 1s abilities into the colossus smash window, like you can in-game
    return timespan_t::from_millis( 250 );
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      warrior_t* p = cast();
      warrior_td_t* td = cast_td( s -> target );
      td -> debuffs_colossus_smash -> trigger( 1, data().effectN( 2 ).percent() );

      if ( ! sim -> overrides.physical_vulnerability )
        s -> target -> debuffs.physical_vulnerability -> trigger();

      if ( p -> glyphs.colossus_smash -> ok() && ! sim -> overrides.weakened_armor )
        s -> target -> debuffs.weakened_armor -> trigger();

      if ( s -> result == RESULT_CRIT )
        p -> enrage();
    }
  }
};

// Deep Wounds ==============================================================

struct deep_wounds_t : public warrior_attack_t
{
  deep_wounds_t( warrior_t* p ) :
    warrior_attack_t( "deep_wounds", p, p -> find_spell( 115767 ) )
  {
    background    = true;
    proc          = true;
    tick_may_crit = true;
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    tick_power_mod = data().extra_coeff();
    dot_behavior = DOT_REFRESH;

    // tested 7/23/2013, https://code.google.com/p/simulationcraft/issues/detail?id=1811
    // fury gets 218 + 24% AP, matching spell data
    // protection DW ticks are 109 + 12% AP, exactly half of spell data values
    if ( p -> specialization() == WARRIOR_PROTECTION )
    {
      tick_power_mod /= 2;
      base_td /= 2;
    }
    // and arms gets exactly 436 + 48% AP, exactly double
    else if ( p -> specialization() == WARRIOR_ARMS )
    {
      tick_power_mod *= 2;
      base_td *= 2;
    }
  }
};

// Demoralizing Shout =======================================================

struct demoralizing_shout : public warrior_attack_t
{
  demoralizing_shout( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "demoralizing_shout", p, p -> find_class_spell( "Demoralizing Shout" ) )
  {
    parse_options( NULL, options_str );
    harmful     = false;
    use_off_gcd = true;
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );
    if ( result_is_hit( s -> result ) )
    {
      warrior_td_t* td = cast_td( s -> target );

      td -> debuffs_demoralizing_shout -> trigger( 1, data().effectN( 1 ).percent() );
    }
  }
  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();

    p -> buff.glyph_incite -> trigger( 3 );
  }

};

// Devastate ================================================================

struct devastate_t : public warrior_attack_t
{
  devastate_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "devastate", p, p -> find_class_spell( "Devastate" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p -> buff.sword_and_board -> trigger() )
      {
        p -> cooldown.shield_slam -> reset( true );
      }
    }

    p -> active_deep_wounds -> target = execute_state -> target;
    p -> active_deep_wounds -> execute();
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && ! sim -> overrides.weakened_armor )
      s -> target -> debuffs.weakened_armor -> trigger();

    warrior_t* p = cast();

    if ( s -> result == RESULT_CRIT )
      p -> enrage();
  }
};

// Dragon Roar ==============================================================

struct dragon_roar_t : public warrior_attack_t
{
  dragon_roar_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "dragon_roar", p, p -> talents.dragon_roar)
  {
    parse_options( NULL, options_str );
    aoe = -1;
    direct_power_mod = data().extra_coeff();
    may_miss = may_dodge = may_parry = may_block = false;
    // lets us benefit from seasoned_soldier, etc. but do not add weapon damage to it
    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
  }

  double calculate_direct_amount( action_state_t* state )
  {
    warrior_attack_t::calculate_direct_amount( state );

    // Adjust damage based on number of targets
    if ( state -> n_targets > 1 )
    {
      if ( state -> n_targets == 2 )
        state -> result_total *= 0.75;
      else if ( state -> n_targets == 3 )
        state -> result_total *= 0.65;
      else if ( state -> n_targets == 4 )
        state -> result_total *= 0.55;
      else
        state -> result_total *= 0.5;
    }

    return state -> result_total;
  }

  virtual double target_armor( player_t* ) const
  {
    return 0;
  }

  virtual double crit_chance( double, int ) const
  {
    // Dragon Roar always crits
    return 1.0;
  }
};

// Execute ==================================================================

struct execute_t : public warrior_attack_t
{
  execute_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "execute", p, p -> find_class_spell( "Execute" ) )
  {
    parse_options( NULL, options_str );

    // Include the weapon so we benefit from racials
    weapon             = &( player -> main_hand_weapon );
    weapon_multiplier  = 0;
    direct_power_mod   = data().extra_coeff();
  }

  virtual double cost() const
  {
    double c = warrior_attack_t::cost();
    const warrior_t* p = cast();

    if ( p -> buff.death_sentence -> check() && target -> health_percentage() < 20) //Tier 16 4 piece bonus
      c = 0;

    return c;
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    warrior_t* p = cast();

    if ( p -> specialization() == WARRIOR_ARMS )
      p -> buff.sudden_execute -> trigger();

  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( target -> health_percentage() > 20 && ! p -> buff.death_sentence -> check() ) // Tier 16 4 piece bonus
      return false;

    return warrior_attack_t::ready();
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    warrior_attack_t::execute();

    p -> buff.death_sentence -> expire();
  }
};

// Heroic Strike ============================================================

struct heroic_strike_t : public warrior_attack_t
{
  heroic_strike_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "heroic_strike", p, p -> find_class_spell( "Heroic Strike" ) )
  {
    parse_options( NULL, options_str );

    weapon  = &( player -> main_hand_weapon );
    harmful = true;

    // The 140% is hardcoded in the tooltip
    if ( weapon -> group() == WEAPON_1H ||
         weapon -> group() == WEAPON_SMALL )
      base_multiplier *= 1.40;

    use_off_gcd = true;
  }

  virtual double cost() const
  {
    double c = warrior_attack_t::cost();
    const warrior_t* p = cast();

    if ( p -> buff.glyph_incite -> check() )
      c *= 1 + p -> buff.glyph_incite -> data().effectN( 1 ).percent();

    if ( p -> buff.ultimatum -> check() )
      c *= 1 + p -> buff.ultimatum -> data().effectN( 1 ).percent();

    return c;
  }

  virtual double crit_chance( double crit, int delta_level ) const
  {
    double cc = warrior_attack_t::crit_chance( crit, delta_level );

    const warrior_t* p = cast();

    if ( p -> buff.ultimatum -> check() )
      cc += p -> buff.ultimatum -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    warrior_attack_t::execute();

    p -> buff.ultimatum -> expire();
    p -> buff.glyph_incite -> decrement();
  }
};

// Heroic Trow ==============================================================

struct heroic_throw_t : public warrior_attack_t
{
  heroic_throw_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "heroic_throw", p, p -> find_class_spell( "Heroic Throw" ) )
  {
    parse_options( NULL, options_str );
    may_dodge = may_parry = false;
  }
};

// Heroic Leap ==============================================================

struct heroic_leap_t : public warrior_attack_t
{
  heroic_leap_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "heroic_leap", p, p -> find_class_spell( "Heroic Leap" ) )
  {
    parse_options( NULL, options_str );

    aoe       = -1;
    may_dodge = may_parry = may_miss = false;
    harmful   = true; // This should be defaulted to true, but it's not

    // Damage is stored in a trigger spell
    const spell_data_t* dmg_spell = p -> dbc.spell( 52174 );  //data().effectN( 3 ).trigger_spell_id() does not resolve to 52174 anymore
    base_dd_min = p -> dbc.effect_min( dmg_spell -> effectN( 1 ).id(), p -> level );
    base_dd_max = p -> dbc.effect_max( dmg_spell -> effectN( 1 ).id(), p -> level );
    direct_power_mod = dmg_spell -> extra_coeff();

    // Heroic Leap can trigger procs from either weapon
    proc_ignores_slot = true;

    // Allow benefits from seasoned solider, etc.
    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    if ( p -> glyphs.death_from_above -> ok() ) //decreases cd
    {
      // Don't want to lower it multiple times if it's in the action list multiple times
      cooldown -> duration = data().cooldown();
      cooldown -> duration += p -> glyphs.death_from_above -> effectN( 1 ).time_value();
    }
    cooldown -> duration  *= p -> buffs.cooldown_reduction -> default_value;
    use_off_gcd = true;
  }
};

// 2 Piece Tier 16 Tank Set Bonus ===========================================

struct tier16_2pc_tank_heal_t : public heal_t
{
  tier16_2pc_tank_heal_t( warrior_t* p ) :
  heal_t( "tier16_2pc_tank_heal", p )
  {
    // Implemented as an actual heal because of spell callbacks ( for Hurricane, etc. )
    background       = true;
    may_crit         = false;
    target           = p;
    direct_power_mod = 0;
  }
  virtual resource_e current_resource() const { return RESOURCE_NONE; }
};

// Impending Victory ========================================================

struct impending_victory_heal_t : public heal_t
{
  impending_victory_heal_t( warrior_t* p ) :
    heal_t( "impending_victory_heal", p, p -> talents.impending_victory )
  {
    // Implemented as an actual heal because of spell callbacks ( for Hurricane, etc. )
    background = true;
    may_crit   = false;
    target     = p;
  }

  virtual double calculate_direct_amount( action_state_t* state )
  {
    warrior_t* p = static_cast<warrior_t*>( player );
    double pct_heal = 0.20;

    if ( p -> buff.tier15_2pc_tank -> up() )
    {
      pct_heal += p -> buff.tier15_2pc_tank -> value();
      pct_heal *= ( 1 + p -> glyphs.victory_rush -> effectN( 1 ).percent() );
    }

    double amount = state -> target -> resources.max[ RESOURCE_HEALTH ] * pct_heal;

    // Record initial amount to state
    state -> result_raw = state -> result_total = amount;

    return amount;
  }

  virtual resource_e current_resource() const { return RESOURCE_NONE; }

};

struct impending_victory_t : public warrior_attack_t
{
  impending_victory_heal_t* impending_victory_heal;

  impending_victory_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "impending_victory", p, p -> talents.impending_victory ),
    impending_victory_heal( new impending_victory_heal_t( p ) )
  {
    parse_options( NULL, options_str );

    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    direct_power_mod  = data().extra_coeff();
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();

    if ( result_is_hit( execute_state -> result ) )
      impending_victory_heal -> execute();

    p -> buff.tier15_2pc_tank -> decrement();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> talents.impending_victory ->ok() && p -> buff.tier15_2pc_tank -> check() )
      return true;

    return warrior_attack_t::ready();
  }
};

// Mortal Strike ============================================================

struct mortal_strike_t : public warrior_attack_t
{
  mortal_strike_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "mortal_strike", p, p -> find_class_spell( "Mortal Strike" ) )
  {
    parse_options( NULL, options_str );
    base_multiplier += p -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();
    base_multiplier *= 1.228; //Hotfix buff for 5.4
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      warrior_t* p = cast();
      p -> active_deep_wounds -> target = execute_state -> target;
      p -> active_deep_wounds -> execute();
      trigger_taste_for_blood( p -> spec.taste_for_blood -> effectN( 2 ).base_value() );
    }
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      warrior_t* p = cast();

      if ( sim -> overrides.mortal_wounds )
        s -> target -> debuffs.mortal_wounds -> trigger();

      if ( p -> sets.has_set_bonus( SET_T16_4PC_MELEE ) )
        p -> buff.death_sentence -> trigger();

      p -> resource_gain( RESOURCE_RAGE,
                          data().effectN( 4 ).resource( RESOURCE_RAGE ),
                          p -> gain.mortal_strike );

      if ( s -> result == RESULT_CRIT )
        p -> enrage();
    }
  }
};

// Overpower ================================================================

struct overpower_t : public warrior_attack_t
{
  overpower_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "overpower", p, p -> find_class_spell( "Overpower" ) )
  {
    parse_options( NULL, options_str );

    may_dodge = may_parry = may_block = false;
    normalize_weapon_speed = false;
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    p -> buff.sudden_execute -> up();

    warrior_attack_t::execute();

    p -> buff.taste_for_blood -> decrement();
  }

  virtual double crit_chance( double crit, int delta_level ) const
  {
    return warrior_attack_t::crit_chance( crit, delta_level ) + data().effectN( 3 ).percent();
  }

  virtual double cost() const
  {
    const warrior_t* p = cast();

    if ( p -> buff.sudden_execute -> check() )
      return 0;

    return warrior_attack_t::cost();
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      warrior_t* p = cast();
      p -> cooldown.mortal_strike -> adjust( timespan_t::from_seconds( -0.5 ) ); //Overpower reduces the cooldown on mortal strike by 0.5 seconds.
    }
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( ! p -> buff.taste_for_blood -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Pummel ===================================================================

struct pummel_t : public warrior_attack_t
{
  pummel_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "pummel", p, p -> find_class_spell( "Pummel" ) )
  {
    parse_options( NULL, options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();

    if ( p -> glyphs.rude_interruption -> ok() )
      p -> buff.rude_interruption -> trigger();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Raging Blow ==============================================================

struct raging_blow_attack_t : public warrior_attack_t
{
  raging_blow_attack_t( warrior_t* p, const char* name, const spell_data_t* s  ) :
    warrior_attack_t( name, p, s )
  {
    may_miss = may_dodge = may_parry = false;
    background = true;
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    aoe = p -> buff.meat_cleaver -> stack();
    if ( aoe ) ++aoe;

    warrior_attack_t::execute();
  }
};

struct raging_blow_t : public warrior_attack_t
{
  raging_blow_attack_t* mh_attack;
  raging_blow_attack_t* oh_attack;
  off_hand_test_attack_t* oh_test;

  raging_blow_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "raging_blow", p, p -> find_class_spell( "Raging Blow" ) ),
    mh_attack( NULL ), oh_attack( NULL ), oh_test( NULL )
  {
    // Parent attack is only to determine miss/dodge/parry
    base_dd_min = base_dd_max = 0;
    weapon_multiplier = direct_power_mod = 0;
    may_crit = proc = false;
    weapon = &( p -> main_hand_weapon ); // Include the weapon for racial expertise

    parse_options( NULL, options_str );

    mh_attack = new raging_blow_attack_t( p, "raging_blow_mh", data().effectN( 1 ).trigger() );
    mh_attack -> weapon = &( p -> main_hand_weapon );
    add_child( mh_attack );

    oh_attack = new raging_blow_attack_t( p, "raging_blow_oh", data().effectN( 2 ).trigger() );
    oh_attack -> weapon = &( p -> off_hand_weapon );
    add_child( oh_attack );

    oh_test = new off_hand_test_attack_t( p, "raging_blow_oh_test" );
    add_child( oh_test );

    // Needs weapons in both hands
    if ( p -> main_hand_weapon.type == WEAPON_NONE ||
         p -> off_hand_weapon.type == WEAPON_NONE )
      background = true;
  }

  virtual void execute()
  {
    oh_test -> execute(); // perform test OH attack, if either OH or MH test rolls fail, the attack will miss completely.

    // check main hand attack
    attack_t::execute();

    warrior_t* p = cast();

    if ( result_is_hit( execute_state -> result ) && result_is_hit( oh_test -> last_result ) )
    {
      mh_attack -> execute();
      oh_attack -> execute();
      p -> buff.raging_wind -> trigger();
      p -> buff.meat_cleaver -> expire();
    }
    else if ( result_is_miss( oh_test -> last_result ) && result_is_hit( execute_state -> result) )
    // Force a rage refund if oh_test misses and mh hits, as the MH will refund rage if it misses.
    {
      double c = cost();
      c *= 0.8;
      p -> resource_gain( RESOURCE_RAGE, c, p -> gain.avoided_attacks );
    }
    p -> buff.raging_blow -> decrement();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( ! p -> buff.raging_blow -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Revenge ==================================================================

struct revenge_t : public warrior_attack_t
{
  stats_t* absorb_stats;
  double rage_gain;

  revenge_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "revenge", p, p -> find_class_spell( "Revenge" ) ),
    absorb_stats( 0 ), rage_gain( 0 )
  {
    parse_options( NULL, options_str );

    direct_power_mod = data().extra_coeff();

    rage_gain = data().effectN( 2 ).resource( RESOURCE_RAGE );
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      warrior_t* p = cast();

      if ( p -> active_stance == STANCE_DEFENSE )
      {
        warrior_td_t* td = cast_td( target );

        p -> resource_gain( RESOURCE_RAGE, rage_gain, p -> gain.revenge );

        if ( td -> debuffs_demoralizing_shout -> up() && p -> sets.has_set_bonus( SET_T15_4PC_TANK ) )
           p -> resource_gain( RESOURCE_RAGE,
                               rage_gain * p -> sets.set( SET_T15_4PC_TANK ) -> effectN( 1 ).percent(),
                               p -> gain.tier15_4pc_tank );
      }
    }
  }

  virtual double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    const warrior_t* p = cast();

    if ( p -> glyphs.hold_the_line -> ok() && p -> buff.glyph_hold_the_line -> up() )
      am *= 1.0 + p -> buff.glyph_hold_the_line -> data().effectN( 1 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      warrior_t* p = cast();

      if ( rng().roll( p -> sets.set( SET_T15_2PC_TANK ) -> proc_chance() ) )
        p -> buff.tier15_2pc_tank -> trigger();

    }
  }
};

// Second Wind ==============================================================

struct second_wind_t : public heal_t
{
  second_wind_t( warrior_t* p ) :
    heal_t( "second_wind", p, p -> find_spell( 16491 ) )
  {
    num_ticks      = 1;
    base_tick_time = timespan_t::from_seconds( 1.0 ); // hardcoded, not in spell data
    hasted_ticks   = false;
    tick_may_crit  = false;
    harmful        = false;
    background     = true;
    target         = p;
  }

  virtual void tick( dot_t* d )
  {
    warrior_t* p = static_cast<warrior_t*>( player );

    d -> current_tick = 0; // ticks indefinitely

    base_td = 0; // set tick amount to zero

    // if we're below 35% health, adjust tick amount; threshold hardcoded, not in spell data
    if ( p -> resources.current[ RESOURCE_HEALTH ] < p -> resources.max[ RESOURCE_HEALTH ] * 0.35 )
      base_td = p -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();

    // call tick()
    heal_t::tick( d );
  }
};

// Shattering Throw =========================================================

struct shattering_throw_t : public warrior_attack_t
{
  shattering_throw_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shattering_throw", p, p -> find_class_spell( "Shattering Throw" ) )
  {
    parse_options( NULL, options_str );
    direct_power_mod = data().extra_coeff();
    weapon            = &( p -> main_hand_weapon );
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      s -> target -> debuffs.shattering_throw -> trigger();
  }

  virtual bool ready()
  {
    if ( target -> debuffs.shattering_throw -> check() )
      return false;

    return warrior_attack_t::ready();
  }
};

// Shield Slam ==============================================================

struct shield_slam_t : public warrior_attack_t
{
  double rage_gain;

  shield_slam_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shield_slam", p, p -> find_class_spell( "Shield Slam" ) ),
    rage_gain( 0.0 )
  {
    check_spec( WARRIOR_PROTECTION );

    parse_options( NULL, options_str );

    rage_gain = data().effectN( 3 ).resource( RESOURCE_RAGE );

    stats -> add_child( player -> get_stats( "shield_slam_combust" ) );

    // Assumption: player level stays constant
    // Values taken from tooltip: 2013/05/22
    direct_power_mod = ( std::max( p -> level, 85 ) * 0.75 ) + ( std::max( p -> level, 80 ) * 0.4 ) + 0.35;
    direct_power_mod /= 100.0; // assumption, not clear from tooltip
  }

  virtual double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    const warrior_t* p = cast();

    if ( p -> buff.shield_block -> up() )
      am *= 1.0 + p -> glyphs.heavy_repercussions -> effectN( 1 ).percent();

    return am;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = cast();
    warrior_td_t* td = cast_td( target );

    double rage_from_snb = 0;

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p -> active_stance == STANCE_DEFENSE )
      {
        p -> resource_gain( RESOURCE_RAGE,
                            rage_gain,
                            p -> gain.shield_slam );

        if (  p -> buff.sword_and_board -> up() )
        {
          rage_from_snb = p -> buff.sword_and_board -> data().effectN( 1 ).resource( RESOURCE_RAGE );
          p -> resource_gain( RESOURCE_RAGE,
                              rage_from_snb,
                              p -> gain.sword_and_board );
        }
        p -> buff.sword_and_board -> expire();
      }
    }

    if ( td -> debuffs_demoralizing_shout -> up() && p -> sets.has_set_bonus( SET_T15_4PC_TANK ) )
      p -> resource_gain( RESOURCE_RAGE,
                        ( rage_gain + rage_from_snb ) * p -> sets.set( SET_T15_4PC_TANK ) -> effectN( 1 ).percent(),
                          p -> gain.tier15_4pc_tank );
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    warrior_t* p = cast();

    if ( rng().roll( p -> sets.set( SET_T15_2PC_TANK ) -> proc_chance() ) )
      p -> buff.tier15_2pc_tank -> trigger();

    if ( s -> result == RESULT_CRIT )
    {
      p -> enrage();
      p -> buff.ultimatum -> trigger();
    }
  }
};

// Shockwave ================================================================

struct shockwave_t : public warrior_attack_t
{
  timespan_t cd_reduction;
  shockwave_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "shockwave", p, p -> talents.shockwave )
  {
    parse_options( NULL, options_str );

    direct_power_mod  = data().effectN( 3 ).percent();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    aoe               = -1;
    cd_reduction      = timespan_t::zero();
  }

  virtual void update_ready( timespan_t )
  {
   // If shockwave hits 3+ targets, the cooldown is reduced by 20 seconds.
   // When used with a cooldown reduction trinket, this 20 seconds is taken into account after the first reduction, which can lead to
   // a 6-8 second cooldown on Shockwave.
    warrior_t* p = cast();
    cd_reduction = timespan_t::zero();

    if ( result_is_hit( execute_state -> result ) )
      if ( execute_state -> n_targets >= 3 )
        cd_reduction = timespan_t::from_seconds( -20 );

    cd_reduction += cooldown -> duration * p -> buffs.cooldown_reduction -> default_value;
    cd_reduction /= p -> buffs.cooldown_reduction -> default_value;
    warrior_attack_t::update_ready( cd_reduction );
  }
};

// Slam AoE Cleave =========================================================
struct slam_sweeping_strikes_attack_t : public warrior_attack_t
{
  slam_sweeping_strikes_attack_t( warrior_t* p ) :
  warrior_attack_t( "slam_sweeping_strikes", p , p -> find_class_spell( "Slam" ))
  {
    may_miss = may_crit = may_dodge = may_parry = proc = callbacks = false;
    background = true;
    aoe        = -1;
    range      =  2;
    weapon_multiplier=0;             // Do not add weapon damage
    weapon = NULL;                   // Don't double dip on seasoned soldier
    base_costs[ RESOURCE_RAGE ] = 0; // Slam cost already factored in.
  }
  
  // Armor has already been taken into account from the parent attack.
  virtual double target_armor( player_t* ) const
  {
    return 0;
  }

  double composite_da_multiplier() const
  {
    return data().effectN( 3 ).percent(); //does not double dip on anything
  }

  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    tl.clear();

    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      player_t* t = sim -> actor_list[ i ];

      if ( ! t -> is_sleeping() && t -> is_enemy() && ( t != target ) )
        tl.push_back( t );
    }
    return tl.size();
  }
};

// Slam =====================================================================

struct slam_t : public warrior_attack_t
{
  slam_sweeping_strikes_attack_t* extra_sweep;
  
  slam_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "slam", p, p -> find_class_spell( "Slam" ) )
  {
    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );
    
    extra_sweep = new slam_sweeping_strikes_attack_t( p );
    add_child( extra_sweep );
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double am = warrior_attack_t::composite_target_multiplier( t );

    warrior_td_t* td = cast_td( t );

    if ( td && td -> debuffs_colossus_smash )
      am *= 1 + cast() -> spell.colossus_smash -> effectN( 5 ).percent();

    return am;
  }
  
  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    warrior_t *p = cast();

    if ( p -> buff.sweeping_strikes -> up() )
    {
      extra_sweep -> base_dd_min = s -> result_amount;
      extra_sweep -> base_dd_max = s -> result_amount;
      extra_sweep -> execute();
    }
  }
};

// Storm Bolt ===============================================================
struct storm_bolt_off_hand_t : public warrior_attack_t
{
  storm_bolt_off_hand_t( warrior_t* p ) :
    warrior_attack_t( "storm_bolt_off_hand", p, p -> talents.storm_bolt )
  {
    may_dodge  = may_parry = may_block = false;
    background = true;

    weapon = &( p -> off_hand_weapon );

    // assume the target is stun-immune
    base_multiplier = 4.0; // hardcoded in tooltip
  }
};

struct storm_bolt_t : public warrior_attack_t
{
  storm_bolt_off_hand_t* oh_attack;

  storm_bolt_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "storm_bolt", p, p -> talents.storm_bolt )
  {
    parse_options( NULL, options_str );
    may_dodge = false;
    may_parry = false;
    may_block = false;

    // Assuming that our target is stun immune, it gets an additional 300% dmg
    base_multiplier = 4.0;

    if ( p -> specialization() == WARRIOR_FURY )
    {
      oh_attack = new storm_bolt_off_hand_t( p );
      add_child( oh_attack );
    }
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    warrior_attack_t::execute(); // for fury, this is the MH attack

    if ( p -> specialization() == WARRIOR_FURY && result_is_hit( execute_state -> result ) ) // If MH fails to land, OH does not execute.
      oh_attack -> execute();
  }
};

// Sunder Armor =============================================================

struct sunder_armor_t : public warrior_attack_t
{
  sunder_armor_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "sunder_armor", p, p -> find_class_spell( "Sunder Armor" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.furious_sundering -> effectN( 1 ).percent();
    background = ( sim -> overrides.weakened_armor != 0 );
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( ! sim -> overrides.weakened_armor )
        s -> target -> debuffs.weakened_armor -> trigger();
    }
  }
};

// Thunder Clap =============================================================

struct thunder_clap_t : public warrior_attack_t
{
  thunder_clap_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "thunder_clap", p, p -> find_class_spell( "Thunder Clap" ) )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    direct_power_mod  = data().extra_coeff();
    // Pass in weapon so it can be benefited by seasoned solider
    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    if ( p -> spec.unwavering_sentinel -> ok() )
      base_costs[ current_resource() ] *= 1.0 + p -> spec.unwavering_sentinel -> effectN( 2 ).percent();

    if ( p -> glyphs.resonating_power -> ok() )
    {
      cooldown -> duration = data().cooldown();
      cooldown -> duration *= 1 + p -> glyphs.resonating_power -> effectN( 2 ).percent();
    }

    // TC can trigger procs from either weapon, even though it doesn't need a weapon
    proc_ignores_slot = true;

    if ( p -> spec.seasoned_soldier -> ok() )
      base_costs[ current_resource() ] += p -> spec.seasoned_soldier -> effectN( 2 ).resource( current_resource() );
  }

  virtual double action_multiplier() const
  {
    double am = warrior_attack_t::action_multiplier();

    const warrior_t* p = cast();

    if ( p -> glyphs.resonating_power -> ok() )
      am *= 1.0 + p -> glyphs.resonating_power -> effectN( 1 ).percent();

    if ( p ->  spec.blood_and_thunder -> ok() )
      am *= 1.0 + p -> spec.blood_and_thunder -> effectN( 2 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      warrior_t* p = cast();

      if ( ! sim -> overrides.weakened_blows )
        s -> target -> debuffs.weakened_blows -> trigger();

      if ( p ->  spec.blood_and_thunder -> ok() )
      {
        p -> active_deep_wounds -> target = s -> target;
        p -> active_deep_wounds -> execute();
      }
    }
  }
};

// Victory Rush =============================================================

struct victory_rush_heal_t : public heal_t
{
  victory_rush_heal_t( warrior_t* p ) :
    heal_t( "victory_rush_heal", p, p -> find_spell( 118779 ) )
  {
    // Implemented as an actual heal because of spell callbacks ( for Hurricane, etc. )
    background = true;
    may_crit   = false;
    target     = p;
    pct_heal   = data().effectN( 1 ).percent() * ( 1 + p -> glyphs.victory_rush -> effectN( 1 ).percent() );
  }
  virtual resource_e current_resource() const { return RESOURCE_NONE; }
};

struct victory_rush_t : public warrior_attack_t
{
  victory_rush_heal_t* victory_rush_heal;

  victory_rush_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "victory_rush", p, p -> find_class_spell( "Victory Rush" ) ),
    victory_rush_heal( new victory_rush_heal_t( p ) )
  {
    parse_options( NULL, options_str );

    direct_power_mod     = data().extra_coeff();
    weapon               = &( player -> main_hand_weapon );
    cooldown -> duration = timespan_t::from_seconds( 1000.0 );
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = cast();

    if ( result_is_hit( execute_state -> result ) )
      victory_rush_heal -> execute();

    p -> buff.tier15_2pc_tank -> decrement();
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> buff.tier15_2pc_tank -> check() )
      return true;

    return warrior_attack_t::ready();
  }
};

// Whirlwind ================================================================

struct whirlwind_off_hand_t : public warrior_attack_t
{
  whirlwind_off_hand_t( warrior_t* p ) :
    warrior_attack_t( "whirlwind_oh", p, p -> find_spell( 44949 ) )
  {
    background = true;
    aoe = -1;
    weapon = &( p -> off_hand_weapon );
  }
};

struct whirlwind_t : public warrior_attack_t
{
  whirlwind_off_hand_t* oh_attack;

  whirlwind_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "whirlwind_mh" , p, p -> find_class_spell( "Whirlwind" ) )
  {
    parse_options( NULL, options_str );
    aoe = -1;

    if ( p -> specialization() == WARRIOR_FURY )
    {
      oh_attack = new whirlwind_off_hand_t( p );
      add_child( oh_attack );
    }

    if ( p -> spec.seasoned_soldier -> ok() )
      base_costs[ current_resource() ] += p -> spec.seasoned_soldier -> effectN( 2 ).resource( current_resource() );

    weapon = &( p -> main_hand_weapon );
  }

  virtual double action_multiplier() const
  {
    const warrior_t* p = cast();

    double am = warrior_attack_t::action_multiplier();

    if ( p -> buff.raging_wind ->  up() )
      am *= 1.0 + p -> buff.raging_wind -> data().effectN( 1 ).percent();

    return am;
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    warrior_attack_t::execute();

    if ( p -> specialization() == WARRIOR_FURY )
      oh_attack -> execute();

    if ( result_is_hit( execute_state -> result ) && p -> specialization() == WARRIOR_FURY )
      p -> buff.meat_cleaver -> trigger();

      p -> buff.raging_wind -> expire();

    if ( p -> glyphs.raging_whirlwind -> ok() )
      p -> buff.raging_whirlwind -> trigger();
  }
};

// Wild Strike ==============================================================

struct wild_strike_t : public warrior_attack_t
{
  wild_strike_t( warrior_t* p, const std::string& options_str ) :
    warrior_attack_t( "wild_strike", p, p -> find_class_spell( "Wild Strike" ) )
  {
    parse_options( NULL, options_str );

    weapon  = &( player -> off_hand_weapon );
    harmful = true;

    if ( player -> off_hand_weapon.type == WEAPON_NONE )
      background = true;
  }

  virtual double cost() const
  {
    double c = warrior_attack_t::cost();
    const warrior_t* p = cast();

    if ( p -> buff.bloodsurge -> check() )
      c += p -> buff.bloodsurge -> data().effectN( 2 ).resource( RESOURCE_RAGE );

    return c;
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    warrior_t* p = cast();

    if ( p -> buff.bloodsurge -> check() )
      trigger_gcd = timespan_t::from_seconds( 1.0 );
    else
      trigger_gcd = data().gcd();

    warrior_attack_t::schedule_execute( state );
  }

  virtual void execute()
  {
    warrior_t* p = cast();
    p -> buff.bloodsurge -> up();

    warrior_attack_t::execute();

    p -> buff.bloodsurge -> decrement();
  }
};

// ==========================================================================
// Warrior Spells
// ==========================================================================

struct warrior_spell_t : public warrior_action_t< spell_t >
{
  warrior_spell_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
  }

  virtual timespan_t gcd() const
  {
    // Unaffected by haste
    return trigger_gcd;
  }
};

// Avatar ===================================================================

struct avatar_t : public warrior_spell_t
{
  avatar_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "avatar", p, p -> talents.avatar )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    p -> buff.avatar -> trigger();
  }
};

// Battle Shout =============================================================

struct battle_shout_t : public warrior_spell_t
{
  double rage_gain;

  battle_shout_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "battle_shout", p, p -> find_class_spell( "Battle Shout" ) ),
    rage_gain( 0.0 )
  {
    parse_options( NULL, options_str );

    harmful   = false;

    rage_gain = data().effectN( 3 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE );

    cooldown = p -> get_cooldown( "shout" );
    cooldown -> duration = data().cooldown();
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    if ( ! sim -> overrides.attack_power_multiplier )
      sim -> auras.attack_power_multiplier -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );

    p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gain.battle_shout );
  }
};

// Bloodbath ================================================================

struct bloodbath_t : public warrior_spell_t
{
  bloodbath_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "bloodbath", p, p -> talents.bloodbath )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    p -> buff.bloodbath -> trigger();
  }
};

// Commanding Shout =========================================================

struct commanding_shout_t : public warrior_spell_t
{
  double rage_gain;

  commanding_shout_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "commanding_shout", p, p -> find_class_spell( "Commanding Shout" ) ),
    rage_gain( 0.0 )
  {
    parse_options( NULL, options_str );

    harmful   = false;
    rage_gain = data().effectN( 2 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE );

    cooldown = p -> get_cooldown( "shout" );
    cooldown -> duration = data().cooldown();
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    if ( ! sim -> overrides.stamina )
      sim -> auras.stamina -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );

    p -> resource_gain( RESOURCE_RAGE, rage_gain , p -> gain.commanding_shout );
  }
};

// Berserker Rage ===========================================================

struct berserker_rage_t : public warrior_spell_t
{
  berserker_rage_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "berserker_rage", p, p -> find_class_spell( "Berserker Rage" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    warrior_t* p = cast();

    p -> buff.berserker_rage -> trigger();
    p -> enrage();
  }
};

// Recklessness =============================================================

struct recklessness_t : public warrior_spell_t
{
  double bonus_crit;

  recklessness_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "recklessness", p, p -> find_class_spell( "Recklessness" ) ),
    bonus_crit( 0.0 )
  {
    parse_options( NULL, options_str );

    harmful = false;
    bonus_crit = data().effectN( 1 ).percent();

    if ( p -> glyphs.recklessness -> ok() )
      bonus_crit += p -> glyphs.recklessness -> effectN( 1 ).percent();

    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value();
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buff.recklessness -> trigger( 1, bonus_crit );
  }
};

// Shield Barrier ===========================================================

struct shield_barrier_t : public warrior_action_t<absorb_t>
{
  double rage_cost;

  shield_barrier_t( warrior_t* p, const std::string& options_str ) :
    base_t( "shield_barrier", p, p -> find_class_spell( "Shield Barrier" ) ),
    rage_cost( 0 )
  {
    parse_options( NULL, options_str );

    harmful       = false;
    may_crit      = false;
    tick_may_crit = false;
    target        = player;
  }

  virtual void consume_resource()
  {
    resource_consumed = rage_cost;
    player -> resource_loss( current_resource(), resource_consumed, 0, this );

    if ( sim -> log )
      sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    //get rage so we can use it in calc_direct_amount
    rage_cost = std::min( 60.0, std::max( p -> resources.current[ RESOURCE_RAGE ], cost() ) );

    base_t::execute();
  }

  /* stripped down version to calculate s-> result_amount,
   * i.e., how big our shield is, Formula: max(ap_scale*(AP-Str*2), Sta*stam_scale)*RAGE/60
   */
  virtual double calculate_direct_amount( action_state_t* state)
  {
    double amount = sim -> averaged_range( base_dd_min, base_dd_max );

    if ( round_base_dmg ) amount = floor( amount + 0.5 );

    const warrior_t& p = *cast();

    double   ap_scale = data().effectN( 2 ).percent();
    double stam_scale = data().effectN( 3 ).percent();

    amount += std::max( ap_scale * ( p.cache.attack_power() - p.current.stats.attribute[ ATTR_STRENGTH ] * 2 ),
                     p.current.stats.attribute[ ATTR_STAMINA ] * stam_scale )
           * rage_cost / 60;

    amount *= 1.0 + p.sets.set( SET_T14_4PC_TANK ) -> effectN( 2 ).percent();

    if ( ! sim -> average_range ) amount = floor( amount + rng().real() );

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s amount for %s: dd=%.0f",
                     player -> name(), name(), amount );
    }
    // Record initial and total amount to state
    state -> result_raw = state -> result_total = amount;

    return amount;
  }

  virtual void impact( action_state_t* s )
  {
    warrior_t* p = cast();

    //1) Buff does not stack with itself.
    //2) Will overwrite existing buff if new one is bigger.
    if ( ! p -> buff.shield_barrier -> check() ||
         ( p -> buff.shield_barrier -> check() && p -> buff.shield_barrier -> current_value < s -> result_amount )
       )
    {
      p -> buff.shield_barrier -> trigger( 1, s -> result_amount );
      stats -> add_result( 0.0, s -> result_amount, ABSORB, s -> result, s -> block_result, p );
    }
  }
};

// Shield Block =============================================================

struct shield_block_t : public warrior_spell_t
{
  shield_block_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "shield_block", p, p -> find_class_spell( "Shield Block" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    cooldown -> duration = timespan_t::from_seconds( 9.0 );
    cooldown -> charges = 2;

    use_off_gcd = true;
  }

  virtual double cost() const
  {
    double c = warrior_spell_t::cost();
    const warrior_t* p = cast();

    c += p -> sets.set( SET_T14_4PC_TANK ) -> effectN( 1 ).resource( RESOURCE_RAGE );

    return c;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    if ( p -> buff.shield_block -> check() )
      p -> buff.shield_block -> extend_duration( p, timespan_t::from_seconds( 6.0 ) );
    else
      p -> buff.shield_block -> trigger();
  }
};

// Shield Wall ==============================================================

struct shield_wall_t : public warrior_spell_t
{
  shield_wall_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "shield_wall", p, p -> find_class_spell( "Shield Wall" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> spec.bastion_of_defense -> effectN( 2 ).time_value()
                            + ( p -> glyphs.shield_wall -> ok() ? p -> glyphs.shield_wall -> effectN( 1 ).time_value() : timespan_t::zero() );
    use_off_gcd = true;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    double value = p -> buff.shield_wall -> data().effectN( 1 ).percent();

    if ( p -> glyphs.shield_wall -> ok() )
      value += p -> glyphs.shield_wall -> effectN( 2 ).percent();

    p -> buff.shield_wall -> trigger( 1, value );
  }
};

// Skull Banner =============================================================

struct skull_banner_t : public warrior_spell_t
{
  skull_banner_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "skull_banner", p, p -> find_class_spell( "Skull Banner" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  void init()
  {
    warrior_spell_t::init();

    // If a Warrior that is using a Skull Banner is being simulated, we should
    // disable the override in the sim, so the Warrior(s) can optimize Skull
    // Banner use.
    sim -> overrides.skull_banner = 0;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> type == PLAYER_GUARDIAN || p -> is_enemy() )
        continue;

      p -> buffs.skull_banner -> trigger();
    }
  }
};

// The swap/damage taken options are intended to make it easier for players to simulate possible gains/losses from
// swapping stances while in combat, without having to create a bunch of messy actions for it, and then adding in raid_damage from
// the raid_event function. It does not actually hurt the character at the moment, but does include protections to prevent 
// the character from taking fatal damage.
// Stance ==============================================================
struct stance_t : public warrior_spell_t
{
  warrior_stance switch_to_stance;
  warrior_stance starting_stance;
  std::string stance_str;
  double damage_taken;
  double swap;

  stance_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "stance", p ),
    switch_to_stance( STANCE_BATTLE ), stance_str( "" ), damage_taken( 0 ), swap( 0 )
  {
    option_t options[] =
    {
      opt_string( "choose", stance_str ),
      opt_float( "swap", swap ),
      opt_float( "damage_taken", damage_taken ),
      opt_null()
    };
    parse_options( options, options_str );

    starting_stance = p -> active_stance;
    
    if ( ! stance_str.empty() )
    {
      if ( stance_str == "battle" )
        switch_to_stance = STANCE_BATTLE;
      else if ( stance_str == "berserker" || stance_str == "zerker" )
        switch_to_stance = STANCE_BERSERKER;
      else if ( stance_str == "def" || stance_str == "defensive" )
        switch_to_stance = STANCE_DEFENSE;
    }

    if ( swap < 0 )
    {
      sim -> errorf( "%s Cannot have a negative time value for swap. Setting swap to 1.5", player -> name() );
      swap = 1.5;
    }
    else if ( swap < 1.5 && swap > 0 )
    {
      sim -> errorf( "%s is only able to swap stances every 1.5 seconds, swap and damage_taken were both adjusted to 1.5 seconds.", player -> name() );
      damage_taken /= swap;
      swap = 1.5;
      damage_taken *=swap;
    }

    if( swap == 0 )
      cooldown -> duration = p -> cooldown.stance_swap -> duration;
    else
      cooldown -> duration = (timespan_t::from_seconds( swap ) );

    starting_stance = p -> active_stance;

    use_off_gcd = true;
    harmful     = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    warrior_t* p = cast();

    // Damage that will kill the player will error out and not be allowed. Currently set to either one damaging strike that deals more than the players health,
    // Or if the damage taken per second is more than 33% of the players health.
    if( swap > 0 )
    {
      if( damage_taken > p -> resources.max[ RESOURCE_HEALTH ] || damage_taken / swap > p -> resources.max[ RESOURCE_HEALTH ] / 3 )
      {
        sim -> errorf( "%s will die from amount of incoming damage, please set damage_taken so that it is less than max health.", player -> name() );
        sim -> errorf( "Also ensure that the amount of incoming damage per second does not exceed 33 percent of max health. This option has been disabled for the simulation." );
        swap = 0;
        switch_to_stance = starting_stance;
      }
    }

    if( p -> active_stance != switch_to_stance )
    {
      switch ( p -> active_stance )
      {
        case STANCE_BATTLE:     p -> buff.battle_stance    -> expire(); break;
        case STANCE_BERSERKER:  p -> buff.berserker_stance -> expire(); break;
        case STANCE_DEFENSE:    p -> buff.defensive_stance -> expire(); break;
      }
      p -> active_stance = switch_to_stance;

      switch ( p -> active_stance )
      {
        case STANCE_BATTLE:     p -> buff.battle_stance    -> trigger(); break;
        case STANCE_BERSERKER:  p -> buff.berserker_stance -> trigger(); break;
        case STANCE_DEFENSE:    p -> buff.defensive_stance -> trigger(); break;
      }
    p -> cooldown.stance_swap -> start();
    }

    if( swap > 0 )
    {
      if( p -> active_stance == STANCE_BERSERKER )
        p -> resource_gain( RESOURCE_RAGE, floor( damage_taken / p -> resources.max[ RESOURCE_HEALTH ] * 100 ), p -> gain.berserker_stance );
      if( swap >= 3.0 && p -> active_stance != starting_stance )
        switch_to_stance = starting_stance;
      if( swap < 3.0 )
        cooldown -> start();
      if( swap >= 3.0 && p -> active_stance == starting_stance )
      {
        if ( stance_str == "battle" )
          switch_to_stance = STANCE_BATTLE;
        else if ( stance_str == "berserker" || stance_str == "zerker" )
          switch_to_stance = STANCE_BERSERKER;
        else if ( stance_str == "def" || stance_str == "defensive" )
          switch_to_stance = STANCE_DEFENSE;
        cooldown -> start();
        cooldown -> adjust( -1 * p -> cooldown.stance_swap -> duration );
      }
    }
  }

  virtual bool ready()
  {
    warrior_t* p = cast();

    if ( p -> cooldown.stance_swap -> down() || cooldown -> down() || ( swap == 0 && p -> active_stance == switch_to_stance ) )
      return false;

    return warrior_spell_t::ready();
  }
};

// Sweeping Strikes =========================================================

struct sweeping_strikes_t : public warrior_spell_t
{
  sweeping_strikes_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "sweeping_strikes", p, p -> find_spell( 12328 ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buff.sweeping_strikes -> trigger();
  }
};

// Last Stand ===============================================================

struct last_stand_t : public warrior_spell_t
{
  last_stand_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "last_stand", p, p -> find_spell( 12975 ) )
  {
    harmful = false;

    parse_options( NULL, options_str );
    cooldown -> duration = data().cooldown();
    cooldown -> duration += p -> sets.set( SET_T14_2PC_TANK ) -> effectN( 1 ).time_value();
    use_off_gcd = true;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = cast();

    p -> buff.last_stand -> trigger();
  }
};

} // UNNAMED NAMESPACE

namespace buffs {

struct last_stand_t : public buff_t
{
  int health_gain;

  last_stand_t( warrior_t* p, const uint32_t id, const std::string& /* n */ ) :
    buff_t( buff_creator_t( p, "last_stand", p -> find_spell( id ) ) ),
    health_gain( 0 )
  { }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    health_gain = ( int ) floor( player -> resources.max[ RESOURCE_HEALTH ] * 0.3 );
    player -> stat_gain( STAT_MAX_HEALTH, health_gain );

    return buff_t::trigger( stacks, value, chance, duration );
  }

  virtual void expire_override()
  {
    player -> stat_loss( STAT_MAX_HEALTH, health_gain );

    buff_t::expire_override();
  }
};

struct debuff_demo_shout_t : public buff_t
{

  debuff_demo_shout_t( warrior_td_t& wtd ) :
  buff_t( buff_creator_t( wtd, "demo_shout", wtd.source -> find_specialization_spell( "Demoralizing Shout" ) ) )
  {
    default_value = data().effectN( 1 ).percent() ;
  }

  virtual void expire_override()
  {
    warrior_t* p = (warrior_t*) player;

    if ( set_bonus_t::has_set_bonus( p, SET_T16_4PC_TANK ) )
    {
        p -> buff.tier16_reckless_defense -> trigger();
    }
    
    buff_t::expire_override();
  }
};

} // end namespace buffs

// ==========================================================================
// Warrior Character Definition
// ==========================================================================

warrior_td_t::warrior_td_t( player_t* target, warrior_t* p  ) :
  actor_pair_t( target, p )
{
  dots_bloodbath   = target -> get_dot( "bloodbath",   p );
  dots_deep_wounds = target -> get_dot( "deep_wounds", p );

  debuffs_colossus_smash     = buff_creator_t( *this, "colossus_smash" ).duration( p -> find_class_spell( "Colossus Smash" ) -> duration() );

  debuffs_demoralizing_shout = new buffs::debuff_demo_shout_t( *this );

}

// warrior_t::create_action  ================================================

action_t* warrior_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "auto_attack"        ) return new auto_attack_t        ( this, options_str );
  if ( name == "avatar"             ) return new avatar_t             ( this, options_str );
  if ( name == "battle_shout"       ) return new battle_shout_t       ( this, options_str );
  if ( name == "berserker_rage"     ) return new berserker_rage_t     ( this, options_str );
  if ( name == "bladestorm"         ) return new bladestorm_t         ( this, options_str );
  if ( name == "bloodbath"          ) return new bloodbath_t          ( this, options_str );
  if ( name == "bloodthirst"        ) return new bloodthirst_t        ( this, options_str );
  if ( name == "charge"             ) return new charge_t             ( this, options_str );
  if ( name == "cleave"             ) return new cleave_t             ( this, options_str );
  if ( name == "colossus_smash"     ) return new colossus_smash_t     ( this, options_str );

  if ( name == "demoralizing_shout" ) return new demoralizing_shout   ( this, options_str );
  if ( name == "devastate"          ) return new devastate_t          ( this, options_str );
  if ( name == "dragon_roar"        ) return new dragon_roar_t        ( this, options_str );
  if ( name == "execute"            ) return new execute_t            ( this, options_str );
  if ( name == "heroic_leap"        ) return new heroic_leap_t        ( this, options_str );
  if ( name == "heroic_strike"      ) return new heroic_strike_t      ( this, options_str );
  if ( name == "heroic_throw"       ) return new heroic_throw_t       ( this, options_str );
  if ( name == "impending_victory"  ) return new impending_victory_t  ( this, options_str );
  if ( name == "victory_rush"       ) return new victory_rush_t       ( this, options_str );

  if ( name == "last_stand"         ) return new last_stand_t         ( this, options_str );
  if ( name == "mortal_strike"      ) return new mortal_strike_t      ( this, options_str );
  if ( name == "overpower"          ) return new overpower_t          ( this, options_str );
  if ( name == "pummel"             ) return new pummel_t             ( this, options_str );
  if ( name == "raging_blow"        ) return new raging_blow_t        ( this, options_str );
  if ( name == "recklessness"       ) return new recklessness_t       ( this, options_str );
  if ( name == "revenge"            ) return new revenge_t            ( this, options_str );
  if ( name == "shattering_throw"   ) return new shattering_throw_t   ( this, options_str );
  if ( name == "shield_barrier"     ) return new shield_barrier_t     ( this, options_str );
  if ( name == "shield_block"       ) return new shield_block_t       ( this, options_str );
  if ( name == "shield_wall"        ) return new shield_wall_t        ( this, options_str );
  if ( name == "shield_slam"        ) return new shield_slam_t        ( this, options_str );
  if ( name == "shockwave"          ) return new shockwave_t          ( this, options_str );
  if ( name == "skull_banner"       ) return new skull_banner_t       ( this, options_str );
  if ( name == "slam"               ) return new slam_t               ( this, options_str );
  if ( name == "storm_bolt"         ) return new storm_bolt_t         ( this, options_str );
  if ( name == "stance"             ) return new stance_t             ( this, options_str );
  if ( name == "sunder_armor"       ) return new sunder_armor_t       ( this, options_str );
  if ( name == "sweeping_strikes"   ) return new sweeping_strikes_t   ( this, options_str );
  if ( name == "thunder_clap"       ) return new thunder_clap_t       ( this, options_str );
  if ( name == "whirlwind"          ) return new whirlwind_t          ( this, options_str );
  if ( name == "wild_strike"        ) return new wild_strike_t        ( this, options_str );

  return player_t::create_action( name, options_str );
}

// warrior_t::init_spells ===================================================

void warrior_t::init_spells()
{
  player_t::init_spells();

  // Mastery
  mastery.critical_block         = find_mastery_spell( WARRIOR_PROTECTION          );
  mastery.strikes_of_opportunity = find_mastery_spell( WARRIOR_ARMS                );
  mastery.unshackled_fury        = find_mastery_spell( WARRIOR_FURY                );

  // Spec Passives
  spec.bastion_of_defense       = find_specialization_spell( "Bastion of Defense"  );
  spec.blood_and_thunder        = find_specialization_spell( "Blood and Thunder"   );
  spec.bloodsurge               = find_specialization_spell( "Bloodsurge"          );
  spec.crazed_berserker         = find_specialization_spell( "Crazed Berserker"    );
  spec.flurry                   = find_specialization_spell( "Flurry"              );
  spec.meat_cleaver             = find_specialization_spell( "Meat Cleaver"        );
  spec.riposte                  = find_specialization_spell( "Riposte"             );
  spec.seasoned_soldier         = find_specialization_spell( "Seasoned Soldier"    );
  spec.unwavering_sentinel      = find_specialization_spell( "Unwavering Sentinel" );
  spec.single_minded_fury       = find_specialization_spell( "Single-Minded Fury"  );
  spec.sword_and_board          = find_specialization_spell( "Sword and Board"     );
  spec.sudden_death             = find_specialization_spell( "Sudden Death"        );
  spec.taste_for_blood          = find_specialization_spell( "Taste for Blood"     );
  spec.ultimatum                = find_specialization_spell( "Ultimatum"           );

  // Talents
  talents.juggernaut            = find_talent_spell( "Juggernaut"                  );
  talents.double_time           = find_talent_spell( "Double Time"                 );
  talents.warbringer            = find_talent_spell( "Warbringer"                  );

  talents.enraged_regeneration  = find_talent_spell( "Enraged Regeneration"        );
  talents.second_wind           = find_talent_spell( "Second Wind"                 );
  talents.impending_victory     = find_talent_spell( "Impending Victory"           );

  talents.staggering_shout      = find_talent_spell( "Staggering Shout"            );
  talents.piercing_howl         = find_talent_spell( "Piercing Howl"               );
  talents.disrupting_shout      = find_talent_spell( "Disrupting Shout"            );

  talents.bladestorm            = find_talent_spell( "Bladestorm"                  );
  talents.shockwave             = find_talent_spell( "Shockwave"                   );
  talents.dragon_roar           = find_talent_spell( "Dragon Roar"                 );

  talents.mass_spell_reflection = find_talent_spell( "Mass Spell Reflection"       );
  talents.safeguard             = find_talent_spell( "Safeguard"                   );
  talents.vigilance             = find_talent_spell( "Vigilance"                   );

  talents.avatar                = find_talent_spell( "Avatar"                      );
  talents.bloodbath             = find_talent_spell( "Bloodbath"                   );
  talents.storm_bolt            = find_talent_spell( "Storm Bolt"                  );

  // Glyphs
  glyphs.bull_rush              = find_glyph_spell( "Glyph of Bull Rush"           );
  glyphs.colossus_smash         = find_glyph_spell( "Glyph of Colossus Smash"      );
  glyphs.death_from_above       = find_glyph_spell( "Glyph of Death From Above"    );
  glyphs.enraged_speed          = find_glyph_spell( "Glyph of Enraged Speed"       );
  glyphs.furious_sundering      = find_glyph_spell( "Glyph of Forious Sundering"   );
  glyphs.heavy_repercussions    = find_glyph_spell( "Glyph of Heavy Repercussions" );
  glyphs.hold_the_line          = find_glyph_spell( "Glyph of Hold the Line"       );
  glyphs.incite                 = find_glyph_spell( "Glyph of Incite"              );
  glyphs.raging_whirlwind       = find_glyph_spell( "Glyph of the Raging Whirlwind");
  glyphs.raging_wind            = find_glyph_spell( "Glyph of Raging Wind"         );
  glyphs.recklessness           = find_glyph_spell( "Glyph of Recklessness"        );
  glyphs.resonating_power       = find_glyph_spell( "Glyph of Resonating Power"    );
  glyphs.rude_interruption      = find_glyph_spell( "Glyph of Rude Interruption"   );
  glyphs.shield_wall            = find_glyph_spell( "Glyph of Shield Wall"         );
  glyphs.sweeping_strikes       = find_glyph_spell( "Glyph of Sweeping Strikes"    );
  glyphs.unending_rage          = find_glyph_spell( "Glyph of Unending Rage"       );
  glyphs.victory_rush           = find_glyph_spell( "Glyph of Victory Rush"        );

  // Generic spells
  spell.colossus_smash          = find_class_spell( "Colossus Smash"               );


  // Active spells
  active_deep_wounds   = new deep_wounds_t( this );
  active_bloodbath_dot = new bloodbath_dot_t( this );
  active_second_wind   = new second_wind_t( this );
  active_t16_2pc       = new tier16_2pc_tank_heal_t( this );

  if ( mastery.strikes_of_opportunity -> ok() )
    active_opportunity_strike = new opportunity_strike_t( this );

  static const set_bonus_description_t set_bonuses =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P    H2P    H4P
    {     0,     0, 105797, 105907, 105908, 105911,     0,     0 }, // Tier13
    {     0,     0, 123142, 123144, 123146, 123147,     0,     0 }, // Tier14
    {     0,     0, 138120, 138126, 138280, 138281,     0,     0 }, // Tier15
    {     0,     0, 144436, 144441, 144503, 144502,     0,     0 }, // Tier16
  };

  sets.register_spelldata( set_bonuses );
}

// warrior_t::init_defense ==================================================

void warrior_t::init_defense()
{
  player_t::init_defense();
}

// warrior_t::init_base =====================================================

void warrior_t::init_base_stats()
{
  player_t::init_base_stats();

  resources.base[ RESOURCE_RAGE ] = 100;

  if ( glyphs.unending_rage -> ok() )
    resources.base[ RESOURCE_RAGE ] += glyphs.unending_rage -> effectN( 1 ).resource( RESOURCE_RAGE );

  base.attack_power_per_strength = 2.0;
  base.attack_power_per_agility  = 0.0;

  base.stats.attack_power = level * ( level > 80 ? 3.0 : 2.0 );

  // Avoidance diminishing Returns constants/conversions
  base.miss            = 0.030;
  base.dodge           = 0.030; //90
  base.parry           = 0.030; //90
  base.block           = 0.030; //90
  base.block_reduction = 0.300;

  // updated from http://sacredduty.net/2012/09/14/avoidance-diminishing-returns-in-mop-followup/
  // re-confirmed http://www.sacredduty.net/2013/08/09/updated-diminishing-returns-coefficients-all-tanks/
  diminished_kfactor   = 0.9560000;
  diminished_block_cap = 1.5037594692967;
  diminished_dodge_cap = 0.906425;
  diminished_parry_cap = 2.37186;
  
  // note that these conversions are level-specific; these are L90 values
  base.dodge_per_agility  = 1 / 10000.0 / 100.0; // empirically tested
  base.parry_per_strength = 1 / 95115.8596; // exact value given by Blizzard

  if ( spec.unwavering_sentinel -> ok() )
  {
    base.attribute_multiplier[ ATTR_STAMINA ] *= 1.0 + spec.unwavering_sentinel -> effectN( 1 ).percent();
    base.armor_multiplier *= 1.0 + spec.unwavering_sentinel -> effectN( 3 ).percent();
  }

  base_gcd = timespan_t::from_seconds( 1.5 );
}

//Pre-combat Action Priority List============================================

void warrior_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( specialization() == WARRIOR_FURY && 
         main_hand_weapon.swing_time > timespan_t::from_seconds( 3.0 ) )
      flask_action = "elixir,type=mad_hozen"; // TG crit value is out of control at this point. 
    else if ( primary_role() == ROLE_ATTACK )
      flask_action += "winters_bite";
    else if ( primary_role() == ROLE_TANK )
      flask_action += "earth";
    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
    if ( primary_role() == ROLE_ATTACK )
      food_action += "black_pepper_ribs_and_shrimp";
    else if ( primary_role() == ROLE_TANK )
      food_action += "chun_tian_spring_rolls";
    precombat -> add_action( food_action );
  }

  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( specialization() != WARRIOR_PROTECTION )
    precombat -> add_action( "stance,choose=battle" );
  else
    precombat -> add_action( "stance,choose=defensive" );

  precombat -> add_action( "battle_shout", "Battle shout is used  before combat, character starts out with 40/55 rage depending on whether or not bull rush is glyphed." );
  //Pre-pot
  if ( sim -> allow_potions && level >= 80 )
  {
    if ( primary_role() == ROLE_ATTACK )
      precombat -> add_action( "mogu_power_potion" );
    else if ( primary_role() == ROLE_TANK )
      precombat -> add_action( "mountains_potion" );
  }

}

// EXTREMELY IMPORTANT NOTE ABOUT PRIORITY LISTS
// As of 11/11/2013, it is entirely intentional that some of the action lines do not use the  add_action( this, "Avatar", "if=enabled" ) method of action lists. 
// Using that type of syntax does a talent check, and will remove the line from the action list automatically. However, this is annoying when attempting
// To compare talents, as the user would need to either reload the character or add the line manually, hopefully without messing it up. 
// So, I'm bypassing the talent check to leave every line intact, no matter what talents are selected when importing a character.

// Single Minded Fury Warrior Action Priority List ========================================

void warrior_t::apl_smf_fury()
{
  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> profession_actions = get_profession_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"       );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target" );
  action_priority_list_t* two_targets         = get_action_priority_list( "two_targets"   );
  action_priority_list_t* three_targets       = get_action_priority_list( "three_targets" );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe"           );

  default_list -> add_action( "auto_attack" );

  if ( sim -> allow_potions && level >= 80 )
    default_list -> add_action( "mogu_power_potion,if=(target.health.pct<20&buff.recklessness.up)|target.time_to_die<=25" );

  default_list -> add_action( this, "Recklessness", "if=!talent.bloodbath.enabled&((cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5)&(target.time_to_die>(192*buff.cooldown_reduction.value)|target.health.pct<20))|buff.bloodbath.up&(target.time_to_die>(192*buff.cooldown_reduction.value)|target.health.pct<20)|target.time_to_die<=12",
                              "This incredibly long line can be translated to 'Use recklessness on cooldown with colossus smash; unless the boss will die before the ability is usable again, and then combine with execute instead.'" );
  default_list -> add_action( "avatar,if=enabled&(buff.recklessness.up|target.time_to_die<=25)" );
  default_list -> add_action( this, "Skull Banner", "if=buff.skull_banner.down&(((cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5)&target.time_to_die>192&buff.cooldown_reduction.up)|buff.recklessness.up)" );

  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[ i ] + ",if=!talent.bloodbath.enabled&debuff.colossus_smash.up|buff.bloodbath.up" );

  for ( size_t i = 0; i < profession_actions.size(); i++ )
    default_list -> add_action( profession_actions[ i ] + ",if=!talent.bloodbath.enabled&debuff.colossus_smash.up|buff.bloodbath.up" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[ i ] + ",if=buff.cooldown_reduction.down&(buff.bloodbath.up|(!talent.bloodbath.enabled&debuff.colossus_smash.up))|buff.cooldown_reduction.up&buff.recklessness.up" );

  default_list -> add_action( "run_action_list,name=single_target,if=active_enemies=1" );
  default_list -> add_action( "run_action_list,name=two_targets,if=active_enemies=2" );
  default_list -> add_action( "run_action_list,name=three_targets,if=active_enemies=3" );
  default_list -> add_action( "run_action_list,name=aoe,if=active_enemies>3" );

  single_target -> add_action( "bloodbath,if=enabled&(cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5|target.time_to_die<=20)", 
                               "/actions+=/stance,choose=berserker,damage_taken=150000,swap=20\n" 
                               "# If you wish to simulate raid damage with stance swapping, please try something similar to the line above.\n"
                               "# The above line, when the # is removed, will tell the character to swap to berserker stance, give rage based on 150k of damage taken,\n"
                               "# and then swap back to the original stance. This will repeat every 20 seconds.\n"
                               "\n"
                               "# A very brief overview of the fury single target rotation, keep in mind that this does not include various subtle aspects in the action list, and is only intended for newer players.\n"
                               "# Fury is a priority-based rotation with moderate amounts of rage management. It is  played in a 20-21 second cycle, based around usage of Colossus Smash.\n"
                               "# Bloodthirst is used on cooldown, raging blow is a higher priority than bloodsurge-buffed wild strikes, and the player should attempt to save up rage by foregoing usage of 'rage dumps' such as\n"
                               "# Heroic strike and non-bloodsurge buffed wild strikes when colossus smash is not applied to the target. The goal is to go into using colossus smash with 100-115~ rage\n"
                               "# and then expend all of this rage by using heroic strike 3-4 times during colossus smash. It's also a good idea to save 1 charge of raging blow to use inside of this 6.5 second window.\n"
                               "# Cooldowns are stacked whenever possible, and only delayed for the very last use of them.\n" );
  single_target -> add_action( this, "Berserker Rage", "if=buff.enrage.remains<1&cooldown.bloodthirst.remains>1" );
  single_target -> add_action( this, "Heroic Strike", "if=((debuff.colossus_smash.up&rage>=40)&target.health.pct>=20)|rage>=100&buff.enrage.up" );
  single_target -> add_action( this, "Heroic Leap", "if=debuff.colossus_smash.up" );
  single_target -> add_action( "storm_bolt,if=enabled&buff.cooldown_reduction.up&debuff.colossus_smash.up", 
                               "Galakras cooldown reduction trinket allows Storm Bolt to be synced with Colossus Smash. 'buff.cooldown_reduction.up' is a hackish way of seeing if the trinket is equipped." );
  single_target -> add_action( this, "Raging Blow", "if=buff.raging_blow.stack=2&debuff.colossus_smash.up&target.health.pct>=20",
                               "Delay Bloodthirst if 2 stacks of raging blow are available inside Colossus Smash." );
  single_target -> add_action( "storm_bolt,if=enabled&buff.cooldown_reduction.down&debuff.colossus_smash.up" );
  single_target -> add_action( this, "Bloodthirst", "if=!(target.health.pct<20&debuff.colossus_smash.up&rage>=30&buff.enrage.up)", 
                                "Until execute range, Bloodthirst is used on cooldown 95% of the time. When execute range is reached, bloodthirst can be delayed 1-2~ globals as long as the conditions below are met.\n"
                                "# This is done to lower the amount of heroic strike usage, and to increase the amount of executes used." );
  single_target -> add_action( this, "Wild Strike", "if=buff.bloodsurge.react&target.health.pct>=20&cooldown.bloodthirst.remains<=1",
                               "The GCD reduction of the Bloodsurge buff allows 3 Wild Strikes in-between Bloodthirst." );
  single_target -> add_action( "wait,sec=cooldown.bloodthirst.remains,if=!(target.health.pct<20&debuff.colossus_smash.up&rage>=30&buff.enrage.up)&cooldown.bloodthirst.remains<=1" );
  single_target -> add_action( "dragon_roar,if=enabled&(!debuff.colossus_smash.up&(buff.bloodbath.up|!talent.bloodbath.enabled))" );
  single_target -> add_action( this, "Colossus Smash", "" ,
                               "The debuff from Colossus Smash lasts 6.5 seconds and also has 0.25~ seconds of travel time. This allows 4 1.5 second globals to be used inside of it every time now." );
  single_target -> add_action( "storm_bolt,if=enabled&buff.cooldown_reduction.down" );
  single_target -> add_action( this, "Execute", "if=debuff.colossus_smash.up|rage>70|target.time_to_die<12" );
  single_target -> add_action( this, "Raging Blow", "if=target.health.pct<20|buff.raging_blow.stack=2|(debuff.colossus_smash.up|(cooldown.bloodthirst.remains>=1&buff.raging_blow.remains<=3))" );
  single_target -> add_action( "bladestorm,if=enabled" );
  single_target -> add_action( this, "Wild Strike", "if=buff.bloodsurge.up" );
  single_target -> add_action( this, "Raging Blow", "if=cooldown.colossus_smash.remains>=3", 
                               "If Colossus Smash is coming up soon, it's a good idea to save 1 charge of raging blow for it." );
  single_target -> add_action( this, "Shattering Throw", "if=cooldown.colossus_smash.remains>5",
                               "Shattering Throw is a very small personal dps increase, but a fairly significant raid dps increase. Only use to fill empty globals." );
  single_target -> add_action( "shockwave,if=enabled" );
  single_target -> add_action( this, "Heroic Throw", "if=debuff.colossus_smash.down&rage<60" );
  single_target -> add_action( this, "Battle Shout", "if=rage<70&!debuff.colossus_smash.up" );
  single_target -> add_action( this, "Wild Strike", "if=debuff.colossus_smash.up&target.health.pct>=20" );
  single_target -> add_action( this, "Battle Shout", "if=rage<70" );
  single_target -> add_action( this, "Wild Strike", "if=cooldown.colossus_smash.remains>=2&rage>=70&target.health.pct>=20" );
  single_target -> add_action( "impending_victory,if=enabled&target.health.pct>=20&cooldown.colossus_smash.remains>=2" );

  two_targets -> add_action( "bloodbath,if=enabled&((!talent.bladestorm.enabled&(cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5|target.time_to_die<=20))|(talent.bladestorm.enabled))" );
  two_targets -> add_action( this, "Berserker Rage", "if=(talent.bladestorm.enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)&!cooldown.bladestorm.remains&(!talent.storm_bolt.enabled|(talent.storm_bolt.enabled&!debuff.colossus_smash.up)))|(!talent.bladestorm.enabled&buff.enrage.remains<1&cooldown.bloodthirst.remains>1)" );
  two_targets -> add_action( this, "Cleave", "if=(rage>=60&debuff.colossus_smash.up)|rage>110" );
  two_targets -> add_action( this, "Heroic Leap", "if=buff.enrage.up&(debuff.colossus_smash.up&buff.cooldown_reduction.up|!buff.cooldown_reduction.up)" );
  two_targets -> add_action( "bladestorm,if=enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)&(!talent.storm_bolt.enabled|(talent.storm_bolt.enabled&!debuff.colossus_smash.up))" );
  two_targets -> add_action( "dragon_roar,if=enabled&(!debuff.colossus_smash.up&(buff.bloodbath.up|!talent.bloodbath.enabled))",
                             "Generally, if an encounter has any type of AoE, Bladestorm will be the better choice than Dragon Roar." );
  two_targets -> add_action( this, "Colossus Smash" );
  two_targets -> add_action( this, "Bloodthirst", "cycle_targets=1,if=dot.deep_wounds.remains<5",
                             "Keep deep wounds on as many targets as possible." );
  two_targets -> add_action( "storm_bolt,if=enabled&debuff.colossus_smash.up" );
  two_targets -> add_action( this, "Bloodthirst" );
  two_targets -> add_action( "wait,sec=cooldown.bloodthirst.remains,if=!(target.health.pct<20&debuff.colossus_smash.up&rage>=30&buff.enrage.up)&cooldown.bloodthirst.remains<=1" );
  two_targets -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.up" );
  two_targets -> add_action( this, "Whirlwind", "if=!buff.meat_cleaver.up" );
  two_targets -> add_action( "shockwave,if=enabled" );
  two_targets -> add_action( this, "Execute" );
  two_targets -> add_action( this, "Battle Shout" );
  two_targets -> add_action( this, "Heroic Throw" );

  three_targets -> add_action( "bloodbath,if=enabled" );
  three_targets -> add_action( this, "Berserker Rage", "if=(talent.bladestorm.enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)&!cooldown.bladestorm.remains)|(!talent.bladestorm.enabled&buff.enrage.remains<1&cooldown.bloodthirst.remains>1)" );
  three_targets -> add_action( this, "Cleave", "if=(rage>=70&debuff.colossus_smash.up)|rage>90" );
  three_targets -> add_action( this, "Heroic Leap", "if=buff.enrage.up&(debuff.colossus_smash.up&buff.cooldown_reduction.up|!buff.cooldown_reduction.up)" );
  three_targets -> add_action( "bladestorm,if=enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)" );
  three_targets -> add_action( "dragon_roar,if=enabled&!debuff.colossus_smash.up&(buff.bloodbath.up|!talent.bloodbath.enabled)" );
  three_targets -> add_action( this, "Bloodthirst", "cycle_targets=1,if=!dot.deep_wounds.ticking" );
  three_targets -> add_action( this, "Colossus Smash" );
  three_targets -> add_action( "storm_bolt,if=enabled&debuff.colossus_smash.up" );
  three_targets -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.stack=2" );
  three_targets -> add_action( this, "Whirlwind" );
  three_targets -> add_action( "shockwave,if=enabled" );
  three_targets -> add_action( this, "Raging Blow" );
  three_targets -> add_action( this, "Battle Shout" );
  three_targets -> add_action( this, "Heroic Throw" );

  aoe -> add_action( "bloodbath,if=enabled" );
  aoe -> add_action( this, "Berserker Rage", "if=(talent.bladestorm.enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)&!cooldown.bladestorm.remains)|(!talent.bladestorm.enabled&buff.enrage.remains<1&cooldown.bloodthirst.remains>1)" );
  aoe -> add_action( this, "Cleave", "if=rage>90" );
  aoe -> add_action( this, "Heroic Leap", "if=buff.enrage.up" );
  aoe -> add_action( "bladestorm,if=enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)" );
  aoe -> add_action( this, "Bloodthirst", "cycle_targets=1,if=!dot.deep_wounds.ticking&buff.enrage.down", 
                     "Enrage overlaps 4 GCDs, which allows bloodthirst to be used mostly to keep enrage up, as rage income is typically not an issue with the aoe rotation." );
  aoe -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.stack=3" );
  aoe -> add_action( this, "Whirlwind" );
  aoe -> add_action( "dragon_roar,if=enabled&debuff.colossus_smash.down&(buff.bloodbath.up|!talent.bloodbath.enabled)",
                     "Dragon roar is a poor choice on large-scale AoE as the damage it does is reduced with additional targets. The damage it does per target is reduced by the following amounts:\n"
                     "# 1/2/3/4/5+ targets ---> 0%/25%/35%/45%/50%" );
  aoe -> add_action( this, "Bloodthirst", "cycle_targets=1,if=!dot.deep_wounds.ticking" );
  aoe -> add_action( this, "Colossus Smash" );
  aoe -> add_action( "storm_bolt,if=enabled" );
  aoe -> add_action( "shockwave,if=enabled" );
  aoe -> add_action( this, "Battle Shout" );

}

// Titan's Grip Fury Warrior Action Priority List ========================================

void warrior_t::apl_tg_fury()
{
  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> profession_actions = get_profession_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"       );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target" );
  action_priority_list_t* two_targets         = get_action_priority_list( "two_targets"   );
  action_priority_list_t* three_targets       = get_action_priority_list( "three_targets" );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe"           );

  default_list -> add_action( "auto_attack" );

  if ( sim -> allow_potions && level >= 80 )
    default_list -> add_action( "mogu_power_potion,if=(target.health.pct<20&buff.recklessness.up)|target.time_to_die<=25" );

  default_list -> add_action( this, "Recklessness", "if=!talent.bloodbath.enabled&((cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5)&(target.time_to_die>(192*buff.cooldown_reduction.value)|target.health.pct<20))|buff.bloodbath.up&(target.time_to_die>(192*buff.cooldown_reduction.value)|target.health.pct<20)|target.time_to_die<=12",
                              "This incredibly long line can be translated to 'Use recklessness on cooldown with colossus smash; unless the boss will die before the ability is usable again, and then combine with execute instead.'" );
  default_list -> add_action( "avatar,if=enabled&(buff.recklessness.up|target.time_to_die<=25)" );
  default_list -> add_action( this, "Skull Banner", "if=buff.skull_banner.down&(((cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5)&target.time_to_die>192&buff.cooldown_reduction.up)|buff.recklessness.up)" );

  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[ i ] + ",if=!talent.bloodbath.enabled&debuff.colossus_smash.up|buff.bloodbath.up" );

  for ( size_t i = 0; i < profession_actions.size(); i++ )
    default_list -> add_action( profession_actions[ i ] + ",if=!talent.bloodbath.enabled&debuff.colossus_smash.up|buff.bloodbath.up" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[ i ] + ",if=buff.cooldown_reduction.down&(buff.bloodbath.up|(!talent.bloodbath.enabled&debuff.colossus_smash.up))|buff.cooldown_reduction.up&buff.recklessness.up" );

  default_list -> add_action( "run_action_list,name=single_target,if=active_enemies=1" );
  default_list -> add_action( "run_action_list,name=two_targets,if=active_enemies=2" );
  default_list -> add_action( "run_action_list,name=three_targets,if=active_enemies=3" );
  default_list -> add_action( "run_action_list,name=aoe,if=active_enemies>3" );


  single_target -> add_action( "bloodbath,if=enabled&(cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5|target.time_to_die<=20)", 
                               "/actions+=/stance,choose=berserker,damage_taken=150000,swap=20\n" 
                               "# If you wish to simulate raid damage with stance swapping, please try something similar to the line above.\n"
                               "# The above line, when the # is removed, will tell the character to swap to berserker stance, give rage based on 150k of damage taken,\n"
                               "# and then swap back to the original stance. This will repeat every 20 seconds.\n"
                               "\n"
                               "# A very brief overview of the fury single target rotation, keep in mind that this does not include various subtle aspects in the action list, and is only intended for newer players.\n"
                               "# Fury is a priority-based rotation with moderate amounts of rage management. It is  played in a 20-21 second cycle, based around usage of Colossus Smash.\n"
                               "# Bloodthirst is used on cooldown, raging blow is a higher priority than bloodsurge-buffed wild strikes, and the player should attempt to save up rage by foregoing usage of 'rage dumps' such as\n"
                               "# Heroic strike and non-bloodsurge buffed wild strikes when colossus smash is not applied to the target. The goal is to go into using colossus smash with 100-115~ rage\n"
                               "# and then expend all of this rage by using heroic strike 3-4 times during colossus smash. It's also a good idea to save 1 charge of raging blow to use inside of this 6.5 second window.\n"
                               "# Cooldowns are stacked whenever possible, and only delayed for the very last use of them.\n" );
  single_target -> add_action( this, "Berserker Rage", "if=buff.enrage.remains<1&cooldown.bloodthirst.remains>1" );
  single_target -> add_action( this, "Heroic Strike", "if=(debuff.colossus_smash.up&rage>=40|rage>=100)&buff.enrage.up" );
  single_target -> add_action( this, "Heroic Leap", "if=debuff.colossus_smash.up&buff.enrage.up" );
  single_target -> add_action( this, "Bloodthirst", "if=!buff.enrage.up" );
  single_target -> add_action( "storm_bolt,if=enabled&buff.cooldown_reduction.up&debuff.colossus_smash.up", 
                               "Galakras cooldown reduction trinket allows Storm Bolt to be synced with Colossus Smash. 'buff.cooldown_reduction.up' is a hackish way of seeing if the trinket is equipped." );
  single_target -> add_action( this, "Raging Blow", "if=buff.raging_blow.stack=2&debuff.colossus_smash.up",
                               "Delay Bloodthirst if 2 stacks of raging blow are available inside Colossus Smash." );
  single_target -> add_action( "storm_bolt,if=enabled&buff.cooldown_reduction.down&debuff.colossus_smash.up" );
  single_target -> add_action( "dragon_roar,if=enabled&(!debuff.colossus_smash.up&(buff.bloodbath.up|!talent.bloodbath.enabled))" );
  single_target -> add_action( this, "Bloodthirst", "" );
  single_target -> add_action( this, "Wild Strike", "if=buff.bloodsurge.react&cooldown.bloodthirst.remains<=1&cooldown.bloodthirst.remains>0.3",
                               "The GCD reduction of the Bloodsurge buff allows 3 Wild Strikes in-between Bloodthirst." );
  single_target -> add_action( "wait,sec=cooldown.bloodthirst.remains,if=!(debuff.colossus_smash.up&rage>=30&buff.enrage.up)&cooldown.bloodthirst.remains<=1" );
  single_target -> add_action( this, "Colossus Smash", "" ,
                               "The debuff from Colossus Smash lasts 6.5 seconds and also has 0.25~ seconds of travel time. This allows 4 1.5 second globals to be used inside of it every time now." );
  single_target -> add_action( "storm_bolt,if=enabled&buff.cooldown_reduction.down" );
  single_target -> add_action( this, "Execute", "if=buff.raging_blow.stack<2&(((rage>70&!debuff.colossus_smash.up)|debuff.colossus_smash.up)|trinket.proc.strength.up)|target.time_to_die<5" );
  single_target -> add_action( this, "Berserker Rage", "if=buff.raging_blow.stack<=1&target.health.pct>=20" );
  single_target -> add_action( this, "Raging Blow", "if=buff.raging_blow.stack=2|debuff.colossus_smash.up|buff.raging_blow.remains<=3" );
  single_target -> add_action( "bladestorm,if=enabled,interrupt_if=cooldown.bloodthirst.remains<1" );
  single_target -> add_action( this, "Raging Blow", "if=cooldown.colossus_smash.remains>=1" );
  single_target -> add_action( this, "Wild Strike", "if=buff.bloodsurge.up" );
  single_target -> add_action( this, "Shattering Throw", "if=cooldown.colossus_smash.remains>5",
                               "Shattering Throw is a very small personal dps increase, but a fairly significant raid dps increase. Only use to fill empty globals." );
  single_target -> add_action( "shockwave,if=enabled" );
  single_target -> add_action( this, "Heroic Throw", "if=debuff.colossus_smash.down&rage<60" );
  single_target -> add_action( this, "Wild Strike", "if=debuff.colossus_smash.up" );
  single_target -> add_action( this, "Battle Shout", "if=rage<70" );
  single_target -> add_action( "impending_victory,if=enabled&cooldown.colossus_smash.remains>=1.5" );
  single_target -> add_action( this, "Wild Strike", "if=cooldown.colossus_smash.remains>=2&rage>=70" );

  two_targets -> add_action( "bloodbath,if=enabled&((!talent.bladestorm.enabled&(cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5|target.time_to_die<=20))|(talent.bladestorm.enabled))" );
  two_targets -> add_action( this, "Berserker Rage", "if=(talent.bladestorm.enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)&!cooldown.bladestorm.remains&(!talent.storm_bolt.enabled|(talent.storm_bolt.enabled&!debuff.colossus_smash.up)))|(!talent.bladestorm.enabled&buff.enrage.remains<1&cooldown.bloodthirst.remains>1)" );
  two_targets -> add_action( this, "Cleave", "if=(rage>=60&debuff.colossus_smash.up)|rage>110" );
  two_targets -> add_action( this, "Heroic Leap", "if=buff.enrage.up&(debuff.colossus_smash.up&buff.cooldown_reduction.up|!buff.cooldown_reduction.up)" );
  two_targets -> add_action( "bladestorm,if=enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)&(!talent.storm_bolt.enabled|(talent.storm_bolt.enabled&!debuff.colossus_smash.up))" );
  two_targets -> add_action( "dragon_roar,if=enabled&(!debuff.colossus_smash.up&(buff.bloodbath.up|!talent.bloodbath.enabled))",
                             "Generally, if an encounter has any type of AoE, Bladestorm will be the better choice than Dragon Roar." );
  two_targets -> add_action( this, "Colossus Smash" );
  two_targets -> add_action( this, "Bloodthirst", "cycle_targets=1,if=dot.deep_wounds.remains<5",
                             "Keep deep wounds on as many targets as possible." );
  two_targets -> add_action( "storm_bolt,if=enabled&debuff.colossus_smash.up" );
  two_targets -> add_action( this, "Bloodthirst" );
  two_targets -> add_action( "wait,sec=cooldown.bloodthirst.remains,if=!(target.health.pct<20&debuff.colossus_smash.up&rage>=30&buff.enrage.up)&cooldown.bloodthirst.remains<=1" );
  two_targets -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.up" );
  two_targets -> add_action( this, "Whirlwind", "if=!buff.meat_cleaver.up" );
  two_targets -> add_action( "shockwave,if=enabled" );
  two_targets -> add_action( this, "Execute" );
  two_targets -> add_action( this, "Battle Shout" );
  two_targets -> add_action( this, "Heroic Throw" );

  three_targets -> add_action( "bloodbath,if=enabled" );
  three_targets -> add_action( this, "Berserker Rage", "if=(talent.bladestorm.enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)&!cooldown.bladestorm.remains)|(!talent.bladestorm.enabled&buff.enrage.remains<1&cooldown.bloodthirst.remains>1)" );
  three_targets -> add_action( this, "Cleave", "if=(rage>=70&debuff.colossus_smash.up)|rage>90" );
  three_targets -> add_action( this, "Heroic Leap", "if=buff.enrage.up&(debuff.colossus_smash.up&buff.cooldown_reduction.up|!buff.cooldown_reduction.up)" );
  three_targets -> add_action( "bladestorm,if=enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)" );
  three_targets -> add_action( "dragon_roar,if=enabled&!debuff.colossus_smash.up&(buff.bloodbath.up|!talent.bloodbath.enabled)" );
  three_targets -> add_action( this, "Bloodthirst", "cycle_targets=1,if=!dot.deep_wounds.ticking" );
  three_targets -> add_action( this, "Colossus Smash" );
  three_targets -> add_action( "storm_bolt,if=enabled&debuff.colossus_smash.up" );
  three_targets -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.stack=2" );
  three_targets -> add_action( this, "Whirlwind" );
  three_targets -> add_action( "shockwave,if=enabled" );
  three_targets -> add_action( this, "Raging Blow" );
  three_targets -> add_action( this, "Battle Shout" );
  three_targets -> add_action( this, "Heroic Throw" );

  aoe -> add_action( "bloodbath,if=enabled" );
  aoe -> add_action( this, "Berserker Rage", "if=(talent.bladestorm.enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)&!cooldown.bladestorm.remains)|(!talent.bladestorm.enabled&buff.enrage.remains<1&cooldown.bloodthirst.remains>1)" );
  aoe -> add_action( this, "Cleave", "if=rage>90" );
  aoe -> add_action( this, "Heroic Leap", "if=buff.enrage.up" );
  aoe -> add_action( "bladestorm,if=enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)" );
  aoe -> add_action( this, "Bloodthirst", "cycle_targets=1,if=!dot.deep_wounds.ticking&buff.enrage.down", 
                     "Enrage overlaps 4 GCDs, which allows bloodthirst to be used mostly to keep enrage up, as rage income is typically not an issue with the aoe rotation." );
  aoe -> add_action( this, "Raging Blow", "if=buff.meat_cleaver.stack=3" );
  aoe -> add_action( this, "Whirlwind" );
  aoe -> add_action( "dragon_roar,if=enabled&debuff.colossus_smash.down&(buff.bloodbath.up|!talent.bloodbath.enabled)",
                     "Dragon roar is a poor choice on large-scale AoE as the damage it does is reduced with additional targets. The damage it does per target is reduced by the following amounts:\n"
                     "# 1/2/3/4/5+ targets ---> 0%/25%/35%/45%/50%" );
  aoe -> add_action( this, "Bloodthirst", "cycle_targets=1,if=!dot.deep_wounds.ticking" );
  aoe -> add_action( this, "Colossus Smash" );
  aoe -> add_action( "storm_bolt,if=enabled" );
  aoe -> add_action( "shockwave,if=enabled" );
  aoe -> add_action( this, "Battle Shout" );
}

// Arms Warrior Action Priority List ========================================

void warrior_t::apl_arms()
{
  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> profession_actions = get_profession_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default"       );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target" );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe"           );

  default_list -> add_action( "auto_attack" );
  
  if ( sim -> allow_potions && level >= 80 )
    default_list -> add_action( "mogu_power_potion,if=(target.health.pct<20&buff.recklessness.up)|buff.bloodlust.react|target.time_to_die<=25" );

  default_list -> add_action( this, "Recklessness", "if=!talent.bloodbath.enabled&((cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5)&(target.time_to_die>(192*buff.cooldown_reduction.value)|target.health.pct<20))|buff.bloodbath.up&(target.time_to_die>(192*buff.cooldown_reduction.value)|target.health.pct<20)|target.time_to_die<=12",
                              "This incredibly long line (Due to differing talent choices) says 'Use recklessness on cooldown with colossus smash, unless the boss will die before the ability is usable again, and then use it with execute.'" );
  default_list -> add_action( "avatar,if=enabled&(buff.recklessness.up|target.time_to_die<=25)" );
  default_list -> add_action( this, "Skull Banner", "if=buff.skull_banner.down&(((cooldown.colossus_smash.remains<2|debuff.colossus_smash.remains>=5)&target.time_to_die>192&buff.cooldown_reduction.up)|buff.recklessness.up)" );

  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[ i ] + ",if=!talent.bloodbath.enabled&debuff.colossus_smash.up|buff.bloodbath.up" );

  for ( size_t i = 0; i < profession_actions.size(); i++ )
    default_list -> add_action( profession_actions[ i ] + ",if=!talent.bloodbath.enabled&debuff.colossus_smash.up|buff.bloodbath.up" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[ i ] + ",if=buff.cooldown_reduction.down&(buff.bloodbath.up|(!talent.bloodbath.enabled&debuff.colossus_smash.up))|buff.cooldown_reduction.up&buff.recklessness.up" );

  default_list -> add_action( "bloodbath,if=enabled&(debuff.colossus_smash.remains>0.1|cooldown.colossus_smash.remains<5|target.time_to_die<=20)" );
  default_list -> add_action( this, "Berserker Rage", "if=buff.enrage.remains<0.5" );
  default_list -> add_action( this, "Heroic Leap", "if=debuff.colossus_smash.up" );
  default_list -> add_action( "run_action_list,name=aoe,if=active_enemies>=2" );
  default_list -> add_action( "run_action_list,name=single_target,if=active_enemies<2" );

  single_target -> add_action( this, "Heroic Strike", "if=rage>115|(debuff.colossus_smash.up&rage>60&set_bonus.tier16_2pc_melee)",
                                     "/actions+=/stance,choose=berserker,damage_taken=150000,swap=20\n" 
                                     "# If you wish to simulate raid damage with stance swapping, please try something similar to the line above.\n"
                                     "# The above line, when the # is removed, will tell the character to swap to berserker stance, give rage based on 150k of damage taken,\n"
                                     "# and then swap back to the original stance. This will repeat every 20 seconds.\n" );
  single_target -> add_action( this, "Mortal Strike", "if=dot.deep_wounds.remains<1.0|buff.enrage.down|rage<10" ) ;
  single_target -> add_action( this, "Colossus Smash", "if=debuff.colossus_smash.remains<1.0" );
  single_target -> add_action( "bladestorm,if=enabled,interrupt_if=!cooldown.colossus_smash.remains",
                              "Use cancelaura (in-game) to stop bladestorm if CS comes off cooldown during it for any reason." );
  single_target -> add_action( this, "Mortal Strike" );
  single_target -> add_action( "storm_bolt,if=enabled&debuff.colossus_smash.up" );
  single_target -> add_action( "dragon_roar,if=enabled&debuff.colossus_smash.down" );
  single_target -> add_action( this, "Execute", "if=buff.sudden_execute.down|buff.taste_for_blood.down|rage>90|target.time_to_die<12" );
  single_target -> add_action( this, "Slam", "if=target.health.pct>=20&(trinket.stacking_stat.crit.stack>=10|buff.recklessness.up)",
                                     "Slam is preferable to overpower with crit procs/recklessness." );
  single_target -> add_action( this, "Overpower", "if=target.health.pct>=20&rage<100|buff.sudden_execute.up" );
  single_target -> add_action( this, "Execute" );
  single_target -> add_action( this, "Slam", "if=target.health.pct>=20" );
  single_target -> add_action( this, "Heroic Throw" );
  single_target -> add_action( this, "Battle Shout" );

  aoe -> add_action( this, "Sweeping Strikes" );
  aoe -> add_action( this, "Cleave", "if=rage>110&active_enemies<=4" );
  aoe -> add_action( "bladestorm,if=enabled&(buff.bloodbath.up|!talent.bloodbath.enabled)" );
  aoe -> add_action( "dragon_roar,if=enabled&debuff.colossus_smash.down" );
  aoe -> add_action( this, "Colossus Smash", "if=debuff.colossus_smash.remains<1" );
  aoe -> add_action( this, "Thunder Clap", "target=2,if=dot.deep_wounds.attack_power*1.1<stat.attack_power" );
  aoe -> add_action( this, "Mortal Strike", "if=active_enemies=2|rage<50" );
  aoe -> add_action( this, "Execute", "if=buff.sudden_execute.down&active_enemies=2" );
  aoe -> add_action( this, "Slam", "if=buff.sweeping_strikes.up&debuff.colossus_smash.up" );
  aoe -> add_action( this, "Overpower", "if=active_enemies=2" );
  aoe -> add_action( this, "Slam", "if=buff.sweeping_strikes.up" );
  aoe -> add_action( this, "Battle Shout" );

}

// Protection Warrior Action Priority List ========================================

void warrior_t::apl_prot()
{
  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> profession_actions = get_profession_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  action_priority_list_t* dps_cds             = get_action_priority_list( "dps_cds" );
  action_priority_list_t* normal_rotation     = get_action_priority_list( "normal_rotation"   );

  default_list -> add_action( "auto_attack" );
  
  if ( sim -> allow_potions && level >= 80 )
    default_list -> add_action( "mountains_potion,if=incoming_damage_2500ms>health.max*0.6&(buff.shield_wall.down&buff.last_stand.down)" );

  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[ i ] + "" );

  default_list -> add_action( this, "Heroic Strike", "if=buff.ultimatum.up|buff.glyph_incite.up" );
  default_list -> add_action( this, "Berserker Rage", "if=buff.enrage.down&rage<=rage.max-10" );
  default_list -> add_action( this, "Shield Block" );
  default_list -> add_action( this, "Shield Barrier", "if=incoming_damage_1500ms>health.max*0.3|rage>rage.max-20" );
  default_list -> add_action( this, "Shield Wall", "if=incoming_damage_2500ms>health.max*0.6" );
  default_list -> add_action( this, "Last  Stand", "if=incoming_damage_2500ms>health.max*0.6&buff.shield_wall.down" );
  default_list -> add_action( "run_action_list,name=dps_cds,if=buff.vengeance.value>health.max*0.20" );
  default_list -> add_action( "run_action_list,name=normal_rotation" );

  dps_cds -> add_action( "avatar,if=enabled" );
  dps_cds -> add_action( "bloodbath,if=enabled" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    dps_cds -> add_action( racial_actions[ i ] + "" );

  for ( size_t i = 0; i < profession_actions.size(); i++ )
    dps_cds -> add_action( profession_actions[ i ] + "" );

  dps_cds -> add_action( "dragon_roar,if=enabled" );
  dps_cds -> add_action( this, "Shattering Throw" );
  dps_cds -> add_action( this, "Skull Banner" );
  dps_cds -> add_action( this, "Recklessness" );
  dps_cds -> add_action( "storm_bolt,if=enabled" );
  dps_cds -> add_action( "shockwave,if=enabled" );
  dps_cds -> add_action( "bladestorm,if=enabled" );
  dps_cds -> add_action( "run_action_list,name=normal_rotation" );

  normal_rotation -> add_action( this, "Shield Slam" );
  normal_rotation -> add_action( this, "Revenge" );
  normal_rotation -> add_action( this, "Battle Shout", "if=rage<=rage.max-20" );
  normal_rotation -> add_action( this, "Thunder Clap", "if=glyph.resonating_power.enabled|target.debuff.weakened_blows.down" );
  normal_rotation -> add_action( this, "Demoralizing Shout" );
  normal_rotation -> add_action( "impending_victory,if=enabled" );
  normal_rotation -> add_action( "victory_rush,if=!talent.impending_victory.enabled" );
  normal_rotation -> add_action( this, "Devastate" );

}

// NO Spec Combat Action Priority List

void warrior_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( "heroic_strike" );
}


// warrior_t::init_scaling ==================================================

void warrior_t::init_scaling()
{
  player_t::init_scaling();

  if ( specialization() == WARRIOR_FURY )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = true;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = sim -> weapon_speed_scale_factors != 0;
    scales_with[ STAT_HIT_RATING2          ] = true;
  }

  if ( primary_role() == ROLE_TANK )
  {
    scales_with[ STAT_DODGE_RATING ] = true;
    scales_with[ STAT_PARRY_RATING ] = true;
    scales_with[ STAT_BLOCK_RATING ] = true;
  }
}

// warrior_t::init_buffs ====================================================

void warrior_t::create_buffs()
{
  player_t::create_buffs();

  // Haste buffs
  buff.flurry           = haste_buff_creator_t( this, "flurry",     spec.flurry -> effectN( 1 ).trigger() )
                          .chance( spec.flurry -> proc_chance() )
                          .add_invalidate( CACHE_ATTACK_SPEED );

  // Regular buffs
  buff.avatar           = buff_creator_t( this, "avatar",           talents.avatar )
                          .cd( timespan_t::zero() )
                          .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.battle_stance    = buff_creator_t( this, "battle_stance",    find_spell( 21156 ) );
  buff.berserker_rage   = buff_creator_t( this, "berserker_rage",   find_class_spell( "Berserker Rage" ) )
                          .cd( timespan_t::zero() );
  buff.berserker_stance = buff_creator_t( this, "berserker_stance", find_spell( 2458 ) )
                          .chance( 1 );
  buff.bloodbath        = buff_creator_t( this, "bloodbath",        talents.bloodbath )
                          .cd( timespan_t::zero() );
  buff.bloodsurge       = buff_creator_t( this, "bloodsurge",       spec.bloodsurge -> effectN( 1 ).trigger() )
                          .chance( spec.bloodsurge -> effectN( 1 ).percent() );

  buff.defensive_stance = buff_creator_t( this, "defensive_stance", find_spell( 7376 ) );
  buff.enrage           = buff_creator_t( this, "enrage",           find_spell( 12880 ) )
                          .activated( false ) ; //Account for delay in buff application.

  buff.enraged_speed    = buff_creator_t( this, "enraged_speed",    glyphs.enraged_speed -> effectN( 1 ).trigger() );

  buff.glyph_hold_the_line    = buff_creator_t( this, "hold_the_line",    glyphs.hold_the_line -> effectN( 1 ).trigger() );
  buff.glyph_incite           = buff_creator_t( this, "glyph_incite",           glyphs.incite -> effectN( 1 ).trigger() )
                                .chance( glyphs.incite -> ok () ? glyphs.incite -> proc_chance() : 0 )
                                .max_stack( 3 )
                                .default_value( find_spell( 122016 ) -> effectN( 1 ).percent() );
  buff.meat_cleaver     = buff_creator_t( this, "meat_cleaver",     spec.meat_cleaver -> effectN( 1 ).trigger() );

  buff.raging_blow      = buff_creator_t( this, "raging_blow",      find_spell( 131116 ) )
                          .max_stack( find_spell( 131116 ) -> effectN( 1 ).base_value() );

  buff.raging_whirlwind         = buff_creator_t( this, "raging_whirlwind", find_spell( 147297 ) )
                                  .max_stack( 2 )
                                  .chance( glyphs.raging_whirlwind -> ok() ? 1 : 0 );

  buff.raging_wind      = buff_creator_t( this, "raging_wind",      glyphs.raging_wind -> effectN( 1 ).trigger() )
                          .chance( ( glyphs.raging_wind -> ok() ? 1 : 0 ) );
  buff.recklessness     = buff_creator_t( this, "recklessness",     find_class_spell( "Recklessness" ) )
                          .duration( find_class_spell( "Recklessness" ) -> duration() * ( 1.0 + ( glyphs.recklessness -> ok() ? glyphs.recklessness -> effectN( 2 ).percent() : 0 )  ) )
                          .cd( timespan_t::zero() );
  buff.taste_for_blood = buff_creator_t( this, "taste_for_blood" )
                         .spell( find_spell( 60503 ) );

  buff.riposte = stat_buff_creator_t( this, "riposte",   spec.riposte -> effectN( 1 ).trigger() )
                 .cd( spec.riposte -> internal_cooldown() )
                 .chance( spec.riposte -> ok() ? spec.riposte -> proc_chance() : 0 )
                 .add_stat( STAT_CRIT_RATING, 0 );

  buff.shield_block     = buff_creator_t( this, "shield_block" ).spell( find_spell( 132404 ) )
                          .add_invalidate( CACHE_BLOCK );
  buff.shield_wall      = buff_creator_t( this, "shield_wall", find_class_spell( "Shield Wall" ) )
                          .default_value( find_class_spell( "Shield Wall" )-> effectN( 1 ).percent() )
                          .cd( timespan_t::zero() );
  buff.sudden_execute   = buff_creator_t( this, "sudden_execute", find_spell( 139958 ) );

  buff.sweeping_strikes = buff_creator_t( this, "sweeping_strikes",  find_class_spell( "Sweeping Strikes" ) );

  buff.sword_and_board  = buff_creator_t( this, "sword_and_board",   find_spell( 50227 ) )
                          .chance( spec.sword_and_board -> effectN( 1 ).percent() );
  buff.ultimatum        = buff_creator_t( this, "ultimatum",   spec.ultimatum -> effectN( 1 ).trigger() )
                          .chance( spec.ultimatum -> ok() ? spec.ultimatum -> proc_chance() : 0 );

  buff.last_stand       = new buffs::last_stand_t( this, 12975, "last_stand" );
  buff.tier15_2pc_tank  = buff_creator_t( this, "tier15_2pc_tank", find_spell( 138279 ) )
                          .default_value( 0.05 ); // inconsistent information in spellid, hardcode to fix

  
  buff.tier16_reckless_defense  = buff_creator_t( this, "tier16_reckless_defense", find_spell( 144500 ));

  buff.death_sentence   = buff_creator_t( this, "death_sentence", find_spell( 144442 ) ) //T16 4 pc melee buff.
                          .chance( find_spell( 144441 ) -> effectN( 1 ).percent() ); //T16 4 piece proc rate.

  buff.rude_interruption = buff_creator_t( this, "rude_interruption", find_spell( 86663 ) )
                           .default_value( find_spell( 86663 ) -> effectN( 1 ).percent() );

  buff.shield_barrier = absorb_buff_creator_t( this, "shield_barrier", find_spell( 112048 ) )
                        .source( get_stats( "shield_barrier" ) );
}

// warrior_t::init_gains ====================================================

void warrior_t::init_gains()
{
  player_t::init_gains();

  gain.avoided_attacks        = get_gain( "avoided_attacks"       );
  gain.battle_shout           = get_gain( "battle_shout"          );
  gain.berserker_stance       = get_gain( "berserker_stance"      );
  gain.bloodthirst            = get_gain( "bloodthirst"           );
  gain.bull_rush              = get_gain( "bull_rush"             );
  gain.charge                 = get_gain( "charge"                );
  gain.commanding_shout       = get_gain( "commanding_shout"      );
  gain.critical_block         = get_gain( "critical_block"        );
  gain.defensive_stance       = get_gain( "defensive_stance"      );
  gain.enrage                 = get_gain( "enrage"                );
  gain.melee_main_hand        = get_gain( "melee_main_hand"       );
  gain.melee_off_hand         = get_gain( "melee_off_hand"        );
  gain.mortal_strike          = get_gain( "mortal_strike"         );
  gain.raging_whirlwind       = get_gain( "raging_whirlwind"      );
  gain.revenge                = get_gain( "revenge"               );
  gain.shield_slam            = get_gain( "shield_slam"           );
  gain.sweeping_strikes       = get_gain( "sweeping_strikes"      );
  gain.sword_and_board        = get_gain( "sword_and_board"       );

  gain.tier15_4pc_tank        = get_gain( "tier15_4pc_tank"       );
  gain.tier16_2pc_melee       = get_gain( "tier16_2pc_melee"      );
  gain.tier16_4pc_tank        = get_gain( "tier16_4pc_tank"       );
}

// warrior_t::init_procs ====================================================

void warrior_t::init_procs()
{
  player_t::init_procs();

  proc.strikes_of_opportunity  = get_proc( "strikes_of_opportunity"  );
  proc.sudden_death            = get_proc( "sudden_death"            );
  proc.taste_for_blood_wasted  = get_proc( "taste_for_blood_wasted"  );
  proc.raging_blow_wasted      = get_proc( "raging_blow_wasted"      );

  proc.t15_2pc_melee           = get_proc( "t15_2pc_melee"           );
}

// warrior_t::init_rng ======================================================

void warrior_t::init_rng()
{
  player_t::init_rng();

  double rppm;
  //Lookup rppm value according to spec
  switch ( specialization() )
  {
    case WARRIOR_ARMS:
      rppm = 1.6;
      break;
    case WARRIOR_FURY:
      rppm = 0.6;
      break;
    case WARRIOR_PROTECTION:
      rppm = 1;
      break;
    default: rppm = 0.0;
      break;
  }
  t15_2pc_melee.set_frequency( rppm * 1.11 );
}

// warrior_t::init_actions ==================================================

void warrior_t::init_action_list()
{
  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );

    quiet = true;
    return;
  }
  clear_action_priority_lists();

  apl_precombat();

  switch ( specialization() )
  {
    case WARRIOR_FURY:
      if( main_hand_weapon.swing_time < timespan_t::from_seconds( 3.0 ) )
        apl_smf_fury();
      else
        apl_tg_fury();
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

// warrior_t::combat_begin ==================================================

void warrior_t::combat_begin()
{
  player_t::combat_begin();

  if ( initial_rage > 0 )
    resources.current[ RESOURCE_RAGE ] = initial_rage; // User specified rage.
  else
    player_t::resource_gain( RESOURCE_RAGE, ( find_class_spell( "Charge" ) -> effectN( 2 ).resource( RESOURCE_RAGE ) ), gain.charge );
    if ( glyphs.bull_rush -> ok() )
      player_t::resource_gain( RESOURCE_RAGE, ( glyphs.bull_rush -> effectN( 2 ).resource( RESOURCE_RAGE ) ), gain.bull_rush );

  if ( active_stance == STANCE_BATTLE && ! buff.battle_stance -> check() )
    buff.battle_stance -> trigger();

  if ( specialization() == WARRIOR_PROTECTION )
    vengeance_start();

  // if second wind is talented, apply it upon entering combat
  if ( talents.second_wind -> ok() )
    active_second_wind -> execute();
}

// warrior_t::reset =========================================================

void warrior_t::reset()
{
  player_t::reset();

  active_stance = STANCE_BATTLE;

  t15_2pc_melee.reset();
}

// warrior_t::composite_player_multiplier ===================================

double warrior_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buff.avatar -> up() )
    m *= 1.0 + buff.avatar -> data().effectN( 1 ).percent();

  return m;
}

// warrior_t::matching_gear_multiplier ======================================

double warrior_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( ( attr == ATTR_STRENGTH ) && ( specialization() == WARRIOR_ARMS || specialization() == WARRIOR_FURY ) )
    return 0.05;

  if ( ( attr == ATTR_STAMINA ) && ( specialization() == WARRIOR_PROTECTION ) )
    return 0.05;

  return 0.0;
}

// warrior_t::composite_block ==========================================

double warrior_t::composite_block() const
{
  double block_by_rating = current.stats.block_rating / current_rating().block;

  // add mastery block to block_by_rating so we can have DR on it.
  if ( mastery.critical_block -> ok() )
    block_by_rating += composite_mastery()  * mastery.critical_block -> effectN( 2 ).mastery_value();

  double b = initial.block;

  if ( block_by_rating > 0 ) // Formula taken from player_t::composite_tank_block
  {
    //the block by rating gets rounded because that's how blizzard rolls...
    b += 1 / ( 1 / diminished_block_cap + diminished_kfactor / ( util::round( 12800 * block_by_rating ) / 12800 ) );
  }


  b += spec.bastion_of_defense -> effectN( 1 ).percent();

  if ( buff.shield_block -> up() )
    b += buff.shield_block -> data().effectN( 1 ).percent();

  return b;
}

// warrior_t::composite_crit_block =====================================

double warrior_t::composite_crit_block() const
{
  double b = player_t::composite_crit_block();

  if ( mastery.critical_block -> ok() )
    b += cache.mastery_value();

  return b;
}

// warrior_t::composite_crit_avoidance ===========================================

double warrior_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.unwavering_sentinel -> effectN( 4 ).percent();

  return c;
}

// warrior_t::composite_dodge ==========================================

double warrior_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  d += spec.bastion_of_defense -> effectN( 3 ).percent();

  return d;
}

// warrior_t::composite_attack_speed ========================================

double warrior_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  if ( buff.flurry -> up() )
    s *= 1.0 / ( 1.0 + buff.flurry -> data().effectN( 1 ).percent() );

  return s;
}

// warrior_t::composite_rating_multiplier ===================================

double warrior_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      m *= 1.5; // This +50% hidden buff was introduced in 5.2
      break;
    default: break;
  }

  return m;
}

void warrior_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_MASTERY && mastery.critical_block -> ok() )
  {
    player_t::invalidate_cache( CACHE_BLOCK );
    player_t::invalidate_cache( CACHE_CRIT_BLOCK );
  }

}

// warrior_t::regen =========================================================

void warrior_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buff.defensive_stance -> check() )
    player_t::resource_gain( RESOURCE_RAGE, ( periodicity.total_seconds() / 3.0 ), gain.defensive_stance );

  if ( buff.raging_whirlwind -> check() )
    player_t::resource_gain( RESOURCE_RAGE, (periodicity.total_seconds() * 2.5 * buff.raging_whirlwind -> current_stack), gain.raging_whirlwind );
}

// warrior_t::primary_role() ================================================

role_e warrior_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_ATTACK || player_t::primary_role() == ROLE_DPS )
    return ROLE_ATTACK;

  if ( specialization() == WARRIOR_PROTECTION )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// warrior_t::assess_damage =================================================

void warrior_t::assess_damage( school_e school,
                               dmg_e    dtype,
                               action_state_t* s )
{
  if ( s -> result == RESULT_HIT    ||
       s -> result == RESULT_CRIT   ||
       s -> result == RESULT_GLANCE )
  {
    if ( buff.defensive_stance -> check() )
      s -> result_amount *= 1.0 + buff.defensive_stance -> data().effectN( 1 ).percent();

    warrior_td_t* td = get_target_data( s -> action -> player );

    if ( td -> debuffs_demoralizing_shout -> up() )
      s -> result_amount *= 1.0 + td -> debuffs_demoralizing_shout -> value();


    //take care of dmg reduction CDs
    if ( buff.shield_wall -> check() )
      s -> result_amount *= 1.0 + buff.shield_wall -> value();

    if ( s -> block_result == BLOCK_RESULT_CRIT_BLOCKED )
    {
      if ( cooldown.rage_from_crit_block -> up() )
      {
        cooldown.rage_from_crit_block -> start();
        resource_gain( RESOURCE_RAGE, 
                       buff.enrage -> data().effectN( 1 ).resource( RESOURCE_RAGE ),
                       gain.critical_block );
        buff.enrage -> trigger();
      }
    }
  }

  if ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY )
  {
    cooldown.revenge -> reset( true );
    buff.riposte -> stats[ 0 ].amount = ( current.stats.dodge_rating + current.stats.parry_rating ) * spec.riposte -> effectN( 1 ).percent();
    buff.riposte -> trigger();
  }

  if ( s -> result == RESULT_PARRY && glyphs.hold_the_line -> ok())
    buff.glyph_hold_the_line -> trigger();

  player_t::assess_damage( school, dtype, s );

  if ( ( s -> result == RESULT_HIT    ||
         s -> result == RESULT_CRIT   ||
         s -> result == RESULT_GLANCE ) &&
         active_stance == STANCE_BERSERKER )
  {
    player_t::resource_gain( RESOURCE_RAGE,
                             floor( s -> result_amount / resources.max[ RESOURCE_HEALTH ] * 100 ),
                             gain.berserker_stance );
  }


  if ( ( s -> result == RESULT_HIT    ||
         s -> result == RESULT_CRIT   ||
         s -> result == RESULT_GLANCE ) &&
         buff.tier16_reckless_defense -> up() )
  {
    player_t::resource_gain( RESOURCE_RAGE,
                             floor( s -> result_amount / resources.max[ RESOURCE_HEALTH ] * 100 ),
                             gain.tier16_4pc_tank );
  }

  
  if ( sets.has_set_bonus( SET_T16_2PC_TANK ) )
  {
    if (s -> block_result != BLOCK_RESULT_UNBLOCKED) //heal if blocked
    {
      double heal_amount = floor( s -> blocked_amount * sets.set( SET_T16_2PC_TANK ) -> effectN( 1 ).percent() );
      active_t16_2pc -> base_dd_min = active_t16_2pc -> base_dd_max = heal_amount;
      active_t16_2pc -> execute();
    }

    if (s -> self_absorb_amount > 0 )//always heal if shield_barrier absorbed it. This assumes that shield_barrier is our only own absorb spell.
    {
      double heal_amount = floor( s -> self_absorb_amount * sets.set( SET_T16_2PC_TANK ) -> effectN( 2 ).percent() );//FIX: check effectN after dbc update.. Currently the tooltip points to 1 in both cases, but I don't know whether that is intended.
      active_t16_2pc -> base_dd_min = active_t16_2pc -> base_dd_max = heal_amount;
      active_t16_2pc -> execute();
    }

  }
}

// warrior_t::create_options ================================================

void warrior_t::create_options()
{
  player_t::create_options();

  option_t warrior_options[] =
  {
    opt_int( "initial_rage", initial_rage ),
    opt_null()
  };

  option_t::copy( options, warrior_options );
}

struct warrior_flurry_of_xuen_t : public warrior_attack_t // Specialized flurry so that armor reduction from colossus smash will function.
{
  warrior_flurry_of_xuen_t( warrior_t* p ) :
    warrior_attack_t( "flurry_of_xuen", p, p -> find_spell( 147891 ) )
  {
    direct_power_mod = data().extra_coeff();
    background = true;
    proc = false;
    aoe = 5;
    special = may_miss = may_parry = may_block = may_dodge = may_crit = true;
  }

};

// warrior_t::create_proc_action =============================================

action_t* warrior_t::create_proc_action( const std::string& name )
{
  if ( name == "flurry_of_xuen" ) return new warrior_flurry_of_xuen_t( this );

  return 0;
}

// warrior_t::create_profile ================================================

bool warrior_t::create_profile( std::string& profile_str, save_e type, bool save_html )
{
  if ( specialization() == WARRIOR_PROTECTION )
    position_str = "front";

  return player_t::create_profile( profile_str, type, save_html );
}

// warrior_t::copy_from =====================================================

void warrior_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warrior_t* p = debug_cast<warrior_t*>( source );

  initial_rage = p -> initial_rage;
}

// warrior_t::decode_set ====================================================

set_e warrior_t::decode_set( const item_t& item ) const
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

  if ( strstr( s, "resounding_rings" ) )
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

    if ( is_melee ) return SET_T14_MELEE;
    if ( is_tank  ) return SET_T14_TANK;
  }

  if ( strstr( s, "last_mogu" ) )
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

    if ( is_melee ) return SET_T15_MELEE;
    if ( is_tank  ) return SET_T15_TANK;
  }

  if ( strstr( s, "prehistoric_marauder" ) )
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

    if ( is_melee ) return SET_T16_MELEE;
    if ( is_tank  ) return SET_T16_TANK;
  }


  if ( strstr( s, "_gladiators_plate_"   ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// warrior_t::enrage ========================================================

void warrior_t::enrage()
{
  // Crit BT/MS/CS/Block and Berserker Rage give rage, 1 charge of Raging Blow, and refreshes the enrage

  if ( specialization() == WARRIOR_FURY )
  {
    if ( buff.raging_blow -> stack() == 2 )
      proc.raging_blow_wasted -> occur();

    buff.raging_blow -> trigger();
  }

  resource_gain( RESOURCE_RAGE, 
                 buff.enrage -> data().effectN( 1 ).resource( RESOURCE_RAGE ),
                 gain.enrage );

  buff.enrage -> trigger();

  if ( glyphs.enraged_speed -> ok() )
    buff.enraged_speed -> trigger();
}

// WARRIOR MODULE INTERFACE =================================================

struct warrior_module_t : public module_t
{
  warrior_module_t() : module_t( WARRIOR ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  { return new warrior_t( sim, name, r ); }

  virtual bool valid() const { return true; }

  virtual void init( sim_t* sim ) const
  {
    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      p -> debuffs.shattering_throw = buff_creator_t( p, "shattering_throw", p -> find_spell( 64382 ) )
                                      .default_value( std::fabs( p -> find_spell( 64382 ) -> effectN( 2 ).percent() ) )
                                      .cd( timespan_t::zero() )
                                      .add_invalidate( CACHE_ARMOR );

      p -> buffs.skull_banner       = buff_creator_t( p, "skull_banner", p -> find_spell( 114207 ) )
                                     .cd( timespan_t::zero() )
                                     .default_value( p -> find_spell( 114206 ) -> effectN( 1 ).percent() )
                                     .add_invalidate( CACHE_CRIT );
    }
  }

  virtual void combat_begin( sim_t* ) const {}

  virtual void combat_end  ( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::warrior()
{
  static warrior_module_t m;
  return &m;
}
