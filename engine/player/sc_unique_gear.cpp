// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

std::string RTV( unsigned tf, unsigned lfr, unsigned flex, unsigned normal,
                        unsigned elite = 0, unsigned heroic = 0, unsigned heroic_elite = 0 )
{
  assert( ( lfr && flex && lfr <= flex ) || ( ! lfr && flex ) || ( lfr && ! flex  ) || ( ! lfr && ! flex ) );
  assert( ( flex && normal && flex <= normal ) || ( ! flex && normal ) || ( flex && ! normal ) || ( ! flex && ! normal ) );
  assert( ( normal && elite && normal <= elite ) || ( ! normal && elite ) || ( normal && ! elite ) || ( ! normal && ! elite ) );
  assert( ( elite && heroic && elite <= heroic ) || ( ! elite && heroic ) || ( elite && ! heroic ) || ( ! elite && ! heroic ) );
  assert( ( heroic && heroic_elite && heroic <= heroic_elite ) || ( ! heroic && heroic_elite ) || ( heroic && ! heroic_elite ) || ( ! heroic && ! heroic_elite ) );

  std::stringstream s;
  if ( item_database::lfr( tf ) )
    s << lfr;
  else if ( item_database::flex( tf ) )
    s << flex;
  else if ( item_database::elite( tf ) && ! item_database::heroic( tf ) )
    s << elite;
  else if ( item_database::heroic( tf ) && ! item_database::elite( tf ) )
    s << heroic;
  else if ( item_database::heroic( tf ) && item_database::elite( tf ) )
    s << heroic_elite;
  else
    s << normal;

  return s.str();
}

#define maintenance_check( ilvl ) static_assert( ilvl >= 372, "unique item below min level, should be deprecated." )

struct stat_buff_proc_t : public buff_proc_callback_t<stat_buff_t>
{
  stat_buff_proc_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    buff_proc_callback_t<stat_buff_t>( p, data, 0, driver )
  {
    buff = stat_buff_creator_t( listener, proc_data.name_str )
           .activated( data.reverse || data.tick != timespan_t::zero() )
           .max_stack( proc_data.max_stacks )
           .duration( proc_data.duration )
           .cd( proc_data.cooldown )
           .reverse( proc_data.reverse )
           .refreshes( ! proc_data.no_refresh )
           .add_stat( proc_data.stat, proc_data.stat_amount );
  }
};

struct cost_reduction_buff_proc_t : public buff_proc_callback_t<cost_reduction_buff_t>
{
  cost_reduction_buff_proc_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    buff_proc_callback_t<cost_reduction_buff_t>( p, data, 0, driver )
  {
    buff = cost_reduction_buff_creator_t( listener, proc_data.name_str )
           .activated( false )
           .max_stack( proc_data.max_stacks )
           .duration( proc_data.duration )
           .cd( proc_data.cooldown )
           .reverse( proc_data.reverse )
           .amount( proc_data.discharge_amount )
           .refreshes( ! proc_data.no_refresh );
  }
};

struct discharge_spell_t : public spell_t
{
  discharge_spell_t( player_t* p, special_effect_t& data ) :
    spell_t( data.name_str, p, spell_data_t::nil() )
  {
    school = ( data.school == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : data.school;
    discharge_proc = true;
    item_proc = true;
    trigger_gcd = timespan_t::zero();
    base_dd_min = data.discharge_amount;
    base_dd_max = data.discharge_amount;
    direct_power_mod = data.discharge_scaling;
    may_crit = ( data.school != SCHOOL_DRAIN ) && ( ( data.override_result_es_mask & RESULT_CRIT_MASK ) ? ( data.result_es_mask & RESULT_CRIT_MASK ) : true ); // Default true
    may_miss = ( data.override_result_es_mask & RESULT_MISS_MASK ) ? ( data.result_es_mask & RESULT_MISS_MASK ) != 0 : may_miss;
    background  = true;
    aoe = data.aoe;
  }
};

struct discharge_attack_t : public attack_t
{
  discharge_attack_t(  player_t* p, special_effect_t& data ) :
    attack_t( data.name_str, p, spell_data_t::nil() )
  {
    school = ( data.school == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : data.school;
    discharge_proc = true;
    item_proc = true;
    trigger_gcd = timespan_t::zero();
    base_dd_min = data.discharge_amount;
    base_dd_max = data.discharge_amount;
    direct_power_mod = data.discharge_scaling;
    may_crit = ( data.school != SCHOOL_DRAIN ) && ( ( data.override_result_es_mask & RESULT_CRIT_MASK ) ? ( data.result_es_mask & RESULT_CRIT_MASK ) : true ); // Default true
    may_miss = ( data.override_result_es_mask & RESULT_MISS_MASK ) ? ( data.result_es_mask & RESULT_MISS_MASK ) != 0 : may_miss;
    may_dodge = ( data.school == SCHOOL_PHYSICAL ) && ( ( data.override_result_es_mask & RESULT_DODGE_MASK ) ? ( data.result_es_mask & RESULT_DODGE_MASK ) : may_dodge );
    may_parry = ( data.school == SCHOOL_PHYSICAL ) && ( ( data.override_result_es_mask & RESULT_PARRY_MASK ) ? ( data.result_es_mask & RESULT_PARRY_MASK ) : may_parry )
                && ( p -> position() == POSITION_FRONT || p ->position() == POSITION_RANGED_FRONT );
    //may_block = ( data.school == SCHOOL_PHYSICAL ) && ( ( data.override_result_es_mask & RESULT_BLOCK_MASK ) ? ( data.result_es_mask & RESULT_BLOCK_MASK ) : may_block )
    //            && ( p -> position() == POSITION_FRONT || p -> position() == POSITION_RANGED_FRONT );
    may_glance = false;
    background  = true;
    aoe = data.aoe;
  }
};

struct discharge_proc_callback_base_t : public discharge_proc_t<action_t>
{
  discharge_proc_callback_base_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    discharge_proc_t<action_t>( p, data, nullptr, driver )
  {
    if ( proc_data.discharge_amount > 0 )
    {
      discharge_action = new discharge_spell_t( p, proc_data );
    }
    else
    {
      discharge_action = new discharge_attack_t( p, proc_data );
    }
  }
};

// discharge_proc_callback ==================================================

struct discharge_proc_callback_t : public discharge_proc_callback_base_t
{

  discharge_proc_callback_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    discharge_proc_callback_base_t( p, data, driver )
  { }
};

// chance_discharge_proc_callback ===========================================

struct chance_discharge_proc_callback_t : public discharge_proc_callback_base_t
{

  chance_discharge_proc_callback_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    discharge_proc_callback_base_t( p, data, driver )
  { }

  virtual double proc_chance()
  {
    if ( discharge_stacks == this -> proc_data.max_stacks )
      return 1.0;

    return discharge_proc_callback_base_t::proc_chance();
  }
};

// stat_discharge_proc_callback =============================================

struct stat_discharge_proc_callback_t : public discharge_proc_t<action_t>
{
  stat_buff_t* buff;

  stat_discharge_proc_callback_t( player_t* p, special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    discharge_proc_t<action_t>( p, data, nullptr, driver )
  {
    if ( proc_data.max_stacks == 0 ) proc_data.max_stacks = 1;
    if ( proc_data.proc_chance == 0 ) proc_data.proc_chance = 1;

    buff = stat_buff_creator_t( p, proc_data.name_str )
           .max_stack( proc_data.max_stacks )
           .duration( proc_data.duration )
           .cd( proc_data.cooldown )
           .chance( proc_data.proc_chance )
           .activated( false /* proc_data.activated */ )
           .add_stat( proc_data.stat, proc_data.stat_amount );

    if ( proc_data.discharge_amount > 0 )
    {
      discharge_action = new discharge_spell_t( p, proc_data );
    }
    else
    {
      discharge_action = new discharge_attack_t( p, proc_data );
    }
  }

  virtual void deactivate()
  {
    action_callback_t::deactivate();
    buff -> expire();
  }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( buff -> trigger( a ) )
    {
      if ( ! allow_self_procs && ( a == discharge_action ) ) return;
      discharge_action -> execute();
    }
  }
};

// Weapon Proc Callback =====================================================

struct weapon_proc_callback_t : public proc_callback_t<action_state_t>
{
  typedef proc_callback_t<action_state_t> base_t;
  weapon_t* weapon;
  bool all_damage;

  weapon_proc_callback_t( player_t* p,
                          special_effect_t& e,
                          weapon_t* w,
                          bool all = false,
                          const spell_data_t* driver = spell_data_t::nil() ) :
    base_t( p, e, driver ),
    weapon( w ), all_damage( all )
  {
  }

  virtual void trigger( action_t* a, void* call_data )
  {
    if ( ! all_damage && a -> proc ) return;
    if ( a -> weapon && weapon && a -> weapon != weapon ) return;

    base_t::trigger( a, call_data );
  }
};

// Weapon Buff Proc Callback ================================================

struct weapon_buff_proc_callback_t : public buff_proc_callback_t<buff_t, action_state_t>
{
public:
  typedef buff_proc_callback_t<buff_t, action_state_t> base_t;
  weapon_t* weapon;
  bool all_damage;

  weapon_buff_proc_callback_t( player_t* p,
                               special_effect_t& e,
                               weapon_t* w,
                               buff_t* b,
                               bool all = false,
                               const spell_data_t* driver = spell_data_t::nil() ) :
    base_t( p, e, b, driver ),
    weapon( w ),
    all_damage( all )
  {  }

  virtual void trigger( action_t* a, void* call_data )
  {
    if ( ! all_damage && a -> proc ) return;
    if ( a -> weapon && weapon && a -> weapon != weapon ) return;

    base_t::trigger( a, call_data );
  }
};

namespace enchants {

struct synapse_spring_action_t : public action_t
{
  double _stat_amount;
  timespan_t _duration;
  timespan_t _cd;
  stat_buff_t* buff;


  synapse_spring_action_t( player_t* p, const std::string& n, double stat_amount, timespan_t duration, timespan_t cd ) :
    action_t( ACTION_USE, n, p ),
    _stat_amount( stat_amount ),
    _duration( duration ),
    _cd( cd ),
    buff( nullptr )
  {

    buff = dynamic_cast<stat_buff_t*>( buff_t::find( player, n, player ) );
    if ( ! buff )
      buff = stat_buff_creator_t( player, n )
             .duration( _duration )
             .cd( _cd );

    background = true;
  }

  virtual void execute()
  {
    static const attribute_e attr[] = { ATTR_STRENGTH, ATTR_AGILITY, ATTR_INTELLECT };

    stat_e max_stat = STAT_INTELLECT;
    double max_value = -1;

    for ( unsigned i = 0; i < sizeof_array( attr ); ++i )
    {
      if ( player -> current.stats.attribute[ attr[ i ] ] > max_value )
      {
        max_value = player -> current.stats.attribute[ attr[ i ] ];
        max_stat = stat_from_attr( attr[ i ] );
      }
    }

    assert( buff );
    buff -> stats.clear(); // clear previous stat
    buff -> stats.push_back( stat_buff_t::buff_stat_t( max_stat, _stat_amount ) ); // add new max stat
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );
    buff -> trigger();


    update_ready();
  }
};
// synapse_springs ==========================================================

void synapse_springs( item_t* item )
{
  player_t* p = item -> player;

  if ( p -> profession[ PROF_ENGINEERING ] < 425 )
  {
    item -> sim -> errorf( "Player %s attempting to use synapse springs mk 2 without 500 in engineering.\n", p -> name() );
    return;
  }

  item -> parsed.use.name_str = "synapse_springs";
  item -> parsed.use.cooldown = timespan_t::from_seconds( 60.0 );
  item -> parsed.use.execute_action = new synapse_spring_action_t( p, "synapse_springs", 480.0, timespan_t::from_seconds( 10.0 ), timespan_t::from_seconds( 60.0 ) );
}

// synapse_springs_2 ========================================================

void synapse_springs_2( item_t* item )
{
  player_t* p = item -> player;

  const spell_data_t* spell1 = p -> find_spell( 126734 );
  const spell_data_t* spell2 = p -> find_spell( 96230 );

  if ( p -> profession[ PROF_ENGINEERING ] < 550 )
  {
    item -> sim -> errorf( "Player %s attempting to use synapse springs mk 2 without 500 in engineering.\n", p -> name() );
    return;
  }

  item -> parsed.use.name_str = "synapse_springs_2";
  item -> parsed.use.cooldown = spell1 -> cooldown();
  item -> parsed.use.execute_action = new synapse_spring_action_t( p,
      "synapse_springs_2",
      spell1 -> effectN( 1 ).base_value(),
      spell2 -> duration(),
      spell1 -> cooldown() );
}

// phase_fingers ============================================================

void phase_fingers( item_t* item )
{
  player_t* p = item -> player;

  const spell_data_t* spell = p -> find_spell( 108788 );

  if ( p -> profession[ PROF_ENGINEERING ] < 500 )
  {
    item -> sim -> errorf( "Player %s attempting to use phase fingers without 500 in engineering.\n", p -> name() );
    return;
  }
  item -> parsed.use.name_str = "phase_fingers";
  item -> parsed.use.stat = STAT_DODGE_RATING;
  item -> parsed.use.stat_amount = spell -> effectN( 1 ).average( p );
  item -> parsed.use.duration = spell -> duration();
  item -> parsed.use.cooldown = spell -> cooldown();
}

// frag_belt ================================================================

void frag_belt( item_t* item )
{
  player_t* p = item -> player;

  const spell_data_t* spell = p -> find_spell( 67890 );

  if ( p -> profession[ PROF_ENGINEERING ] < 380 )
  {
    item -> sim -> errorf( "Player %s attempting to use frag belt without 380 in engineering.\n", p -> name() );
    return;
  }

  item -> parsed.use.name_str = "frag_belt";
  item -> parsed.use.school = spell -> get_school_type();
  item -> parsed.use.discharge_amount = spell -> effectN( 1 ).average( p );
  item -> parsed.use.cooldown = spell -> cooldown();
  item -> parsed.use.aoe = -1;
}

void executioner( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* mhw, weapon_t* ohw )
{
  if ( mh_enchant == "executioner" || oh_enchant == "executioner" )
  {
    // MH-OH trigger/refresh the same Executioner buff.  It does not stack.

    stat_buff_t* buff = stat_buff_creator_t( p, "executioner" )
                        .spell( p -> find_spell( 42976 ) )
                        .cd( timespan_t::zero() )
                        .chance( 0 )
                        .activated( false );

    special_effect_t effect;
    effect.name_str = "executioner";
    effect.ppm = 1.0; // PPM

    if ( mh_enchant == "executioner" )
    {
      p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, mhw,  buff ) );
    }
    if ( oh_enchant == "executioner" )
    {
      p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, ohw,  buff ) );
    }
  }
}

void hurricane( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* mhw, weapon_t* ohw )
{
  if ( mh_enchant == "hurricane" || oh_enchant == "hurricane" )
  {
    stat_buff_t *mh_buff = 0, *oh_buff = 0;
    if ( mh_enchant == "hurricane" )
    {
      mh_buff = stat_buff_creator_t( p, "hurricane_mh" )
                .spell( p -> find_spell( 74221 ) )
                .cd( timespan_t::zero() )
                .chance( 0 )
                .activated( false );

      special_effect_t effect;
      effect.name_str = "hurricane";
      effect.ppm = 1.0; // PPM

      p -> callbacks.register_direct_damage_callback( SCHOOL_ATTACK_MASK, new weapon_buff_proc_callback_t( p, effect, mhw, mh_buff, true /*ALL*/ ) );
      p -> callbacks.register_tick_damage_callback  ( SCHOOL_ATTACK_MASK, new weapon_buff_proc_callback_t( p, effect, mhw, mh_buff, true /*ALL*/ ) );
    }
    if ( oh_enchant == "hurricane" )
    {
      oh_buff = stat_buff_creator_t( p, "hurricane_oh" )
                .spell( p -> find_spell( 74221 ) )
                .cd( timespan_t::zero() )
                .chance( 0 )
                .activated( false );

      special_effect_t effect;
      effect.name_str = "hurricane_oh";
      effect.ppm = 1.0; // PPM

      p -> callbacks.register_direct_damage_callback( SCHOOL_ATTACK_MASK, new weapon_buff_proc_callback_t( p, effect, ohw, oh_buff, true /*ALL*/ ) );
      p -> callbacks.register_tick_damage_callback  ( SCHOOL_ATTACK_MASK, new weapon_buff_proc_callback_t( p, effect, ohw, oh_buff, true /*ALL*/ ) );
    }
    // Custom proc is required for spell damage procs.
    // If MH buff is up, then refresh it, else
    // IF OH buff is up, then refresh it, else
    // Trigger a new buff not associated with either MH or OH
    // This means that it is possible to have three stacks
    struct hurricane_spell_proc_callback_t : public action_callback_t
    {
      buff_t *mh_buff, *oh_buff, *s_buff;
      hurricane_spell_proc_callback_t( player_t* p, buff_t* mhb, buff_t* ohb, buff_t* sb ) :
        action_callback_t( p ), mh_buff( mhb ), oh_buff( ohb ), s_buff( sb )
      {
      }
      virtual void trigger( action_t* /* a */, void* /* call_data */ )
      {
        if ( s_buff -> cooldown -> down() ) return;
        if ( ! listener -> rng().roll( 0.15 ) ) return;
        if ( mh_buff && mh_buff -> check() )
        {
          mh_buff -> trigger();
          s_buff -> cooldown -> start();
        }
        else if ( oh_buff && oh_buff -> check() )
        {
          oh_buff -> trigger();
          s_buff -> cooldown -> start();
        }
        else s_buff -> trigger();
      }
    };
    stat_buff_t* s_buff = stat_buff_creator_t( p, "hurricane_s" )
                          .spell( p -> find_spell( 74221 ) )
                          .cd( timespan_t::from_seconds( 45.0 ) )
                          .activated( false );
    p -> callbacks.register_spell_direct_damage_callback( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> callbacks.register_spell_tick_damage_callback  ( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> callbacks.register_direct_heal_callback        ( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
    p -> callbacks.register_tick_heal_callback          ( SCHOOL_SPELL_MASK, new hurricane_spell_proc_callback_t( p, mh_buff, oh_buff, s_buff ) );
  }
}

void landslide( player_t* p, const std::string& enchant, weapon_t* w, const std::string& weapon_appendix )
{
  if ( enchant == "landslide" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "landslide" + weapon_appendix )
                        .spell( p -> find_spell( 74245 ) )
                        .activated( false )
                        .add_stat( STAT_ATTACK_POWER, 1000 );

    special_effect_t effect;
    effect.name_str = "landslide" + weapon_appendix;
    effect.ppm = 1.0; // PPM

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect , w, buff ) );
  }
}

void mongoose( player_t* p, const std::string& enchant, weapon_t* w, const std::string& weapon_appendix )
{
  if ( enchant == "mongoose" )
  {
    p -> buffs.mongoose_mh = stat_buff_creator_t( p, "mongoose" + weapon_appendix )
                             .duration( timespan_t::from_seconds( 15 ) )
                             .activated( false )
                             .add_stat( STAT_AGILITY, 120 )
                             .add_invalidate( CACHE_HASTE );
    special_effect_t effect;
    effect.name_str = "mongoose" + weapon_appendix;
    effect.ppm = 1.0; // PPM
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, w, p -> buffs.mongoose_mh ) );
  }
}

void power_torrent( player_t* p, const std::string& enchant, const std::string& weapon_appendix )
{
  if ( enchant == "power_torrent" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "power_torrent" + weapon_appendix )
                        .spell( p -> find_spell( 74241 ) )
                        .cd( timespan_t::from_seconds( 45 ) )
                        .chance( 0.20 )
                        .activated( false )
                        .add_stat( STAT_INTELLECT, 500 );

    special_effect_t effect;
    effect.name_str = "power_torrent";

    action_callback_t* cb = new buff_proc_callback_t<stat_buff_t>( p, effect, buff );

    p -> callbacks.register_spell_tick_damage_callback  ( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_spell_direct_damage_callback( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_tick_heal_callback          ( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_direct_heal_callback        ( SCHOOL_ALL_MASK, cb );
  }
}

struct jade_spirit_check_func
{
  jade_spirit_check_func( player_t* p ) : p(p) {}
  bool operator()(const stat_buff_t&)
  {
    if ( p -> resources.max[ RESOURCE_MANA ] <= 0.0 ) return false;

    return ( p -> resources.current[ RESOURCE_MANA ] / p -> resources.max[ RESOURCE_MANA ] < 0.25 );
  }
  player_t* p;
};

void jade_spirit( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant )
{
  const spell_data_t* driver = p -> find_spell( 120033 );
  const spell_data_t* spell = p -> find_spell( 104993 );

  if ( mh_enchant == "jade_spirit" || oh_enchant == "jade_spirit" )
  {
    stat_buff_t* buff  = stat_buff_creator_t( p, "jade_spirit" )
                         .duration( spell -> duration() )
                         .cd( timespan_t::zero() )
                         .chance( p -> find_spell( 120033 ) -> proc_chance() )
                         .activated( false )
                         .add_stat( STAT_INTELLECT, spell -> effectN( 1 ).base_value() )
                         .add_stat( STAT_SPIRIT,    spell -> effectN( 2 ).base_value(), jade_spirit_check_func( p ) );

    special_effect_t effect;
    effect.name_str = "jade_spirit";
    effect.ppm = -1.0 * driver -> real_ppm();
    action_callback_t* cb = new buff_proc_callback_t<stat_buff_t>( p, effect, buff );

    p -> callbacks.register_spell_tick_damage_callback  ( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_spell_direct_damage_callback( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_tick_heal_callback          ( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_direct_heal_callback        ( SCHOOL_ALL_MASK, cb );
  }
}

struct dancing_steel_agi_check_func
{
  dancing_steel_agi_check_func( player_t* p ) : p(p) {}
  bool operator()(const stat_buff_t&)
  {
    return ( p -> agility() >= p -> strength() );
  }
  player_t* p;
};

struct dancing_steel_str_check_func
{
  dancing_steel_str_check_func( player_t* p ) : p(p) {}
  bool operator()(const stat_buff_t&) const
  {
    return ( p -> agility() < p -> strength() );
  }
  player_t* p;
};

void dancing_steel( player_t* p, const std::string& enchant, weapon_t* /* w */, const std::string& weapon_appendix )
{
  if ( ! util::str_compare_ci( enchant, "dancing_steel" ) )
    return;

  const spell_data_t* driver = p -> find_spell( 118333 );
  const spell_data_t* spell = p -> find_spell( 120032 );

  stat_buff_t* buff  = stat_buff_creator_t( p, "dancing_steel" + weapon_appendix )
                       .duration( spell -> duration() )
                       .activated( false )
                       .add_stat( STAT_STRENGTH, spell -> effectN( 1 ).base_value(), dancing_steel_str_check_func( p ) )
                       .add_stat( STAT_AGILITY,  spell -> effectN( 1 ).base_value(), dancing_steel_agi_check_func( p ) );

  special_effect_t effect;
  effect.name_str = "dancing_steel" + weapon_appendix;
  effect.ppm = -1.0 * driver -> real_ppm();

  buff_proc_callback_t<stat_buff_t>* cb = new buff_proc_callback_t<stat_buff_t>( p, effect, buff );

  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
}

struct bloody_dancing_steel_agi_check_func
{
  bloody_dancing_steel_agi_check_func( player_t* p ) : p(p) {}
  bool operator()(const stat_buff_t&)
  {
    return ( p -> agility() >= p -> strength() );
  }
  player_t* p;
};

struct bloody_dancing_steel_str_check_func
{
  bloody_dancing_steel_str_check_func( player_t* p ) : p(p) {}
  bool operator()(const stat_buff_t&) const
  {
    return ( p -> agility() < p -> strength() );
  }
  player_t* p;
};

void bloody_dancing_steel( player_t* p, const std::string& enchant, weapon_t* /* w */, const std::string& weapon_appendix )
{
  if ( ! util::str_compare_ci( enchant, "bloody_dancing_steel" ) )
    return;

  const spell_data_t* driver = p -> find_spell( 142531 );
  const spell_data_t* spell = p -> find_spell( 142530 );

  stat_buff_t* buff  = stat_buff_creator_t( p, "bloody_dancing_steel" + weapon_appendix )
                       .duration( spell -> duration() )
                       .activated( false )
                       .add_stat( STAT_STRENGTH, spell -> effectN( 1 ).base_value(), bloody_dancing_steel_str_check_func( p ) )
                       .add_stat( STAT_AGILITY,  spell -> effectN( 1 ).base_value(), bloody_dancing_steel_agi_check_func( p ) );

  special_effect_t effect;
  effect.name_str = "bloody_dancing_steel" + weapon_appendix;
  effect.ppm = -1.0 * driver -> real_ppm();

  buff_proc_callback_t<stat_buff_t>* cb = new buff_proc_callback_t<stat_buff_t>( p, effect, buff );

  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
}

// Windsong Proc Callback ===================================================

struct windsong_callback_t : public action_callback_t
{
  stat_buff_t* haste_buff;
  stat_buff_t* crit_buff;
  stat_buff_t* mastery_buff;
  real_ppm_t real_ppm;

  windsong_callback_t( player_t* p, stat_buff_t* hb, stat_buff_t* cb, stat_buff_t* mb ) :
    action_callback_t( p ),
    haste_buff  ( hb ), crit_buff( cb ), mastery_buff( mb ),
    real_ppm( *p )
  {
    const spell_data_t* driver = p -> find_spell( 104561 );
    real_ppm.set_frequency( driver -> real_ppm() );
  }

  virtual void reset()
  {
    real_ppm.reset();
  }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( a -> proc ) return;

    if ( real_ppm.trigger( *a ) )
    {
      stat_buff_t* buff;

      int p_type = ( int ) ( a -> sim -> rng().real() * 3.0 );
      switch ( p_type )
      {
        case 0: buff = haste_buff; break;
        case 1: buff = crit_buff; break;
        case 2:
        default:
          buff = mastery_buff; break;
      }

      buff -> trigger( 1, 0 );
    }
  }
};


void windsong( player_t* p, const std::string& enchant, weapon_t* /* w */, const std::string& enchant_suffix )
{
  if ( ! util::str_compare_ci( enchant, "windsong" ) )
    return;

  const spell_data_t* spell = p -> find_spell( 104509 );
  double amount = spell -> effectN( 1 ).base_value();

  stat_buff_t* haste_buff   = stat_buff_creator_t( p, "windsong_haste" + enchant_suffix )
                              .duration( spell -> duration() )
                              .activated( false )
                              .add_stat( STAT_HASTE_RATING,   amount );
  stat_buff_t* crit_buff    = stat_buff_creator_t( p, "windsong_crit" + enchant_suffix )
                              .duration( spell -> duration() )
                              .activated( false )
                              .add_stat( STAT_CRIT_RATING,    amount );
  stat_buff_t* mastery_buff = stat_buff_creator_t( p, "windsong_mastery" + enchant_suffix )
                              .duration( spell -> duration() )
                              .activated( false )
                              .add_stat( STAT_MASTERY_RATING, amount );

  windsong_callback_t* cb  = new windsong_callback_t( p, haste_buff, crit_buff, mastery_buff );
  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
}

void rivers_song( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* mhw, weapon_t* ohw )
{
  if ( mh_enchant == "rivers_song" || oh_enchant == "rivers_song" )
  {
    const spell_data_t* driver = p -> find_spell( 104441 );
    const spell_data_t* spell = p -> find_spell( 116660 );

    stat_buff_t* buff  = stat_buff_creator_t( p, "rivers_song" )
                         .duration( spell -> duration() )
                         .activated( false )
                         .max_stack( spell -> max_stacks() )
                         .add_stat( STAT_DODGE_RATING, spell -> effectN( 1 ).base_value() );

    special_effect_t effect;
    effect.name_str = "rivers_song";
    effect.ppm = -1.0 * driver -> real_ppm();
    effect.cooldown = driver -> internal_cooldown();
    effect.rppm_scale = RPPM_HASTE;

    if ( mh_enchant == "rivers_song" )
    {
      weapon_buff_proc_callback_t* cb = new weapon_buff_proc_callback_t( p, effect, mhw, buff );

      p -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
    }
    if ( oh_enchant == "rivers_song" )
    {
      weapon_buff_proc_callback_t* cb = new weapon_buff_proc_callback_t( p, effect, ohw, buff );

      p -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
    }
  }
}

void windwalk( player_t* p, const std::string& enchant, weapon_t* w, const std::string& weapon_appendix )
{
  if ( enchant == "windwalk" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "windwalk" + weapon_appendix )
                        .duration( timespan_t::from_seconds( 10 ) )
                        .cd( timespan_t::from_seconds( 45 ) )
                        .chance( 0.15 )
                        .activated( false )
                        .add_stat( STAT_DODGE_RATING, 600 );

    special_effect_t effect;
    effect.name_str = "windwalk" + weapon_appendix;

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK,
                                             new weapon_buff_proc_callback_t( p, effect, w, buff ) );
  }
}

void berserking( player_t* p, const std::string& enchant, weapon_t* w, const std::string& weapon_appendix )
{
  if ( enchant == "berserking" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "berserking" + weapon_appendix )
                        .max_stack( 1 )
                        .duration( timespan_t::from_seconds( 15 ) )
                        .cd( timespan_t::zero() )
                        .chance( 0 )
                        .activated( false )
                        .add_stat( STAT_ATTACK_POWER, 400.0 );

    special_effect_t effect;
    effect.name_str = "berserking" + weapon_appendix;
    effect.ppm = 1.0; // PPM

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, w, buff ) );
  }
}

void gnomish_xray( player_t* p, const std::string& enchant, weapon_t* w )
{
  if ( enchant == "gnomish_xray" )
  {
    //FIXME: 1.0 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    stat_buff_t* buff = stat_buff_creator_t( p, "xray_targeting" )
                        .spell( p -> find_spell( 95712 ) )
                        .cd( timespan_t::from_seconds( 40 ) )
                        .activated( false );

    special_effect_t effect;
    effect.name_str = "xray_targeting";
    effect.ppm = 1.0; // PPM

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, w, buff ) );
  }
}

void lord_blastingtons_scope_of_doom( player_t* p, const std::string& enchant, weapon_t* w )
{
  if ( enchant == "lord_blastingtons_scope_of_doom" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "lord_blastingtons_scope_of_doom" )
                        .spell( p -> find_spell( 109085 ) )
                        .activated( false );

    special_effect_t effect;
    effect.name_str = "lord_blastingtons_scope_of_doom";
    effect.ppm = 1.0; // PPM

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, w, buff ) );
  }
}

void mirror_scope( player_t* p, const std::string& enchant, weapon_t* w )
{
  if ( enchant == "mirror_scope" )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "mirror_scope" )
                        .spell( p -> find_spell( 109092 ) )
                        .activated( false );

    special_effect_t effect;
    effect.name_str = "mirror_scope";
    effect.ppm = 1.0; // PPM

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, w, buff ) );
  }
}

void elemental_force( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* /* mhw */, weapon_t* /* ohw */ )
{
  if ( p -> is_enemy() )
    return;

  const spell_data_t* driver = p -> find_spell( 104428 );
  const spell_data_t* elemental_force_spell = p -> find_spell( 116616 );

  double amount = ( elemental_force_spell -> effectN( 1 ).min( p ) + elemental_force_spell -> effectN( 1 ).max( p ) ) / 2;

  special_effect_t effect;
  effect.name_str = "elemental_force";
  effect.ppm = -1.0 * driver -> real_ppm();
  effect.max_stacks = 1;
  effect.school = SCHOOL_ELEMENTAL;
  effect.discharge_amount = amount;
  effect.rppm_scale = RPPM_HASTE;

  if ( mh_enchant == "elemental_force" )
  {
    action_callback_t* cb  = new discharge_proc_callback_t( p, effect );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
    p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
    p -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  }

  if ( oh_enchant == "elemental_force" )
  {
    action_callback_t* cb  = new discharge_proc_callback_t( p, effect );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
    p -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
    p -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
    p -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
  }
}

void colossus( player_t* p, const std::string& mh_enchant, const std::string& oh_enchant, weapon_t* mhw, weapon_t* ohw )
{
  if ( mh_enchant == "colossus" || oh_enchant == "colossus" )
  {
    const spell_data_t* driver = p -> find_spell( 118314 );
    const spell_data_t* ts = p -> find_spell( 116631 ); // trigger spell

    absorb_buff_t* buff = absorb_buff_creator_t( p, "colossus" )
                          .spell( ts -> effectN( 1 ).trigger() )
                          .activated( false )
                          .source( p -> get_stats( "colossus" ) )
                          .cd( timespan_t::from_seconds( 3.0 ) )
                          .chance( ts -> proc_chance() );

    special_effect_t effect;
    effect.name_str = "colossus";
    effect.ppm = -1.0 * driver -> real_ppm();
    effect.rppm_scale = RPPM_HASTE;

    if ( mh_enchant == "colossus" )
    {
      weapon_buff_proc_callback_t* cb = new weapon_buff_proc_callback_t( p, effect, mhw, buff );

      p -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
    }
    if ( oh_enchant == "colossus" )
    {
      weapon_buff_proc_callback_t* cb = new weapon_buff_proc_callback_t( p, effect, ohw, buff );

      p -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
    }
  }
}

} // END enchant NAMESPACE

namespace meta_gems {

void thundering_skyfire( player_t* p, weapon_t* mhw, weapon_t* ohw )
{
  if ( p -> meta_gem == META_THUNDERING_SKYFIRE )
  {
    //FIXME: 0.2 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics
    stat_buff_t* buff = stat_buff_creator_t( p, "skyfire_swiftness" )
                        .spell( p -> find_spell( 39959 ) )
                        .cd( timespan_t::from_seconds( 40 ) )
                        .activated( false );

    special_effect_t effect;
    effect.name_str = "skyfire_swiftness";
    effect.ppm = 0.2; // PPM

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, mhw, buff ) );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, ohw, buff ) );
  }
}

void thundering_skyflare( player_t* p, weapon_t* mhw, weapon_t* ohw )
{
  if ( p -> meta_gem == META_THUNDERING_SKYFLARE )
  {
    stat_buff_t* buff = stat_buff_creator_t( p, "skyflare_swiftness" )
                        .spell( p -> find_spell( 55379 ) )
                        .cd( timespan_t::from_seconds( 40 ) )
                        .activated( false );

    special_effect_t effect;
    effect.name_str = "skyflare_swiftness";
    effect.ppm = 0.2; // PPM
    //FIXME: 0.2 ppm and 40 second icd seems to roughly match in-game behavior, but we need to verify the exact mechanics

    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, mhw, buff ) );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, new weapon_buff_proc_callback_t( p, effect, ohw, buff ) );
  }
}

void sinister_primal( player_t* p )
{
  if ( p -> meta_gem == META_SINISTER_PRIMAL )
  {
    struct sinister_primal_proc_t : public buff_proc_callback_t<buff_t>
    {
      sinister_primal_proc_t( player_t* p, const special_effect_t& data, const spell_data_t* driver ) :
        buff_proc_callback_t<buff_t>( p, data, p -> buffs.tempus_repit, driver )
      {  }
    };

    const spell_data_t* driver = p -> find_spell( 137592 );

    special_effect_t data;
    data.name_str = "tempus_repit";
    data.ppm      = -1.0 * driver -> real_ppm();
    data.cooldown = driver -> internal_cooldown(); 

    sinister_primal_proc_t* cb = new sinister_primal_proc_t( p, data, driver );
    p -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
    p -> callbacks.register_tick_damage_callback( SCHOOL_ALL_MASK, cb );
  }
}

void indomitable_primal( player_t *p )
{
  if ( p -> meta_gem == META_INDOMITABLE_PRIMAL )
  {
    const spell_data_t* driver = p -> find_spell( 137594 );
    
    special_effect_t data;
    data.name_str = "fortitude";
    data.ppm      = -1.0 * driver -> real_ppm();

    buff_proc_callback_t<buff_t> *cb = new buff_proc_callback_t<buff_t>( p, data, p -> buffs.fortitude );
    p -> callbacks.register_incoming_attack_callback( RESULT_ALL_MASK, cb );
  }
}

void capacitive_primal( player_t* p )
{
  if ( p -> meta_gem == META_CAPACITIVE_PRIMAL )
  {
    
    struct lightning_strike_t : public attack_t
    {
      lightning_strike_t( player_t* p ) :
        attack_t( "lightning_strike", p, p -> find_spell( 137597 ) )
      {
        may_crit = special = background = true;
        may_parry = may_dodge = false;
        proc = false;
        direct_power_mod = data().extra_coeff();
      }
    };

    struct capacitive_primal_proc_t : public discharge_proc_t<action_t>
    {
      capacitive_primal_proc_t( player_t* p, const special_effect_t& data, action_t* a, const spell_data_t* spell ) :
        discharge_proc_t<action_t>( p, data, a, spell )
      {
        // Unfortunately the weapon-based RPPM modifiers have to be hardcoded,
        // as they will not show on the client tooltip data.
        if ( listener -> main_hand_weapon.group() != WEAPON_2H )
        {
          if ( listener -> specialization() == WARRIOR_FURY )
            rppm.set_modifier( 1.152 );
          else if ( listener -> specialization() == DEATH_KNIGHT_FROST )
            rppm.set_modifier( 1.134 );
        }
      }

      void trigger( action_t* action, void* call_data )
      {
        // Flurry of Xuen and Capacitance cannot proc Capacitance
        if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
          return;

        discharge_proc_t<action_t>::trigger( action, call_data );
      }
    };

    const spell_data_t* driver = p -> find_spell( 137595 );
    special_effect_t data;
    data.name_str   = "lightning_strike";
    data.max_stacks = 5;
    data.ppm        = -1.0 * driver -> real_ppm();
    data.rppm_scale = RPPM_HASTE;
    data.cooldown   = driver -> internal_cooldown();

    action_t* ls = p -> create_proc_action( "lightning_strike" );
    if ( ! ls )
      ls = new lightning_strike_t( p );
    action_callback_t* cb = new capacitive_primal_proc_t( p, data, ls, driver );
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  }
}

void courageous_primal_diamond( player_t* p )
{
  if ( p -> meta_gem == META_COURAGEOUS_PRIMAL )
  {
    struct courageous_primal_diamond_proc_t : public buff_proc_callback_t<buff_t>
    {
      courageous_primal_diamond_proc_t( player_t* p, const special_effect_t& data ) :
        buff_proc_callback_t<buff_t>( p, data, p -> buffs.courageous_primal_diamond_lucidity )
      { }

      void execute( action_t* action, action_state_t* call_data )
      {
        spell_base_t* spell = debug_cast<spell_base_t*>( action );
        if ( spell -> procs_courageous_primal_diamond )
        {
          if ( listener -> sim -> debug )
            listener -> sim -> out_debug.printf( "%s procs %s from action %s.",
                                       listener -> name(), buff -> name(), spell -> name() );

          buff_proc_callback_t<buff_t>::execute( action, call_data );
        }
      }
    };

    const spell_data_t* driver = p -> find_spell( 137248 );
    special_effect_t data;
    data.name_str = "courageous_primal_diamond_lucidity";
    data.ppm      = -1.0 * driver -> real_ppm();
    data.cooldown = driver -> internal_cooldown();

    courageous_primal_diamond_proc_t* cb = new courageous_primal_diamond_proc_t( p, data );
    p -> callbacks.register_spell_callback( RESULT_ALL_MASK, cb );
  }
}

void meta_gems( player_t* p, weapon_t* mhw, weapon_t* ohw )
{
  // Special Meta Gem "Enchants"
  thundering_skyfire( p, mhw, ohw );
  thundering_skyflare( p, mhw, ohw );

  // Disable legendary meta gem procs in challenge mode
  if ( ! p -> sim -> challenge_mode )
  {
    sinister_primal( p );
    capacitive_primal( p );
    indomitable_primal( p );
    courageous_primal_diamond( p );
  }
}

} // end meta_gems namespace

namespace unique_items {

// touch_of_the_grave =======================================================

void touch_of_the_grave( player_t* p )
{
  assert( p );

  const spell_data_t* s = p -> find_racial_spell( "Touch of the Grave" );

  if ( ! s -> ok() )
  {
    return;
  }

  struct touch_of_the_grave_discharge_spell_t : public spell_t
  {
    touch_of_the_grave_discharge_spell_t( player_t* p, const spell_data_t* s ) :
      spell_t( "touch_of_the_grave", p, s )
    {
      school           = ( s -> effectN( 1 ).trigger() -> get_school_type() == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s -> effectN( 1 ).trigger() -> get_school_type();
      discharge_proc   = true;
      trigger_gcd      = timespan_t::zero();
      base_dd_min      = s -> effectN( 1 ).trigger() -> effectN( 1 ).average( p );
      base_dd_max      = s -> effectN( 1 ).trigger() -> effectN( 1 ).average( p );
      direct_power_mod = s -> effectN( 1 ).trigger() -> effectN( 1 )._coeff;
      may_crit         = false;
      may_miss         = false;
      background       = true;
      aoe              = 0;
    }

    virtual void impact( action_state_t* s )
    {
      spell_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        player -> resource_gain( RESOURCE_HEALTH, s -> result_amount, player -> gains.touch_of_the_grave );
      }
    }
  };


  struct touch_of_the_grave_proc_callback_t : public discharge_proc_t<action_t>
  {
    touch_of_the_grave_proc_callback_t( player_t* p, special_effect_t& data, const spell_data_t* s ) :
      discharge_proc_t<action_t>( p, data, new touch_of_the_grave_discharge_spell_t( p, s ) )
    { }
  };

  special_effect_t se_data;
  se_data.name_str = "touch_of_the_grave";
  se_data.proc_chance = s -> proc_chance();
  se_data.cooldown = timespan_t::from_seconds( 15.0 );

  action_callback_t* cb = new touch_of_the_grave_proc_callback_t( p, se_data, s );

  p -> callbacks.register_attack_callback       ( RESULT_HIT_MASK, cb );
  p -> callbacks.register_harmful_spell_callback( RESULT_HIT_MASK, cb );
}

// jikuns_rising_winds=======================================================

void jikuns_rising_winds( item_t* item )
{
  maintenance_check( 502 );

  player_t* p = item -> player;

  item -> unique = true;

  const spell_data_t* spell = item -> player -> find_spell( 138973 );


  struct jikuns_rising_winds_heal_t : public heal_t
  {
    jikuns_rising_winds_heal_t( item_t& i, const spell_data_t  *spell ) :
      heal_t( "jikuns_rising_winds", i.player, spell )
    {
      trigger_gcd = timespan_t::zero();
      background  = true;
      may_miss    = false;
      may_crit    = true;
      callbacks   = false;
      const random_prop_data_t& budget = i.player -> dbc.random_property( i.item_level() );

      base_dd_min = budget.p_epic[ 0 ]  * spell  -> effectN( 1 ).m_average();
      base_dd_max = base_dd_min;
      init();
    }
  };

  struct jikuns_rising_winds_callback_t : public action_callback_t
  {
    heal_t* heal;
    cooldown_t* cd;
    proc_t* proc;

    jikuns_rising_winds_callback_t( player_t* p, heal_t* s ) :
      action_callback_t( p ), heal( s ), proc( 0 )
    {
      proc = p -> get_proc( "jikuns_rising_winds" );
      cd = p -> get_cooldown( "jikuns_rising_winds_callback" );
      cd -> duration = timespan_t::from_seconds( 30.0 );
    }

    virtual void trigger( action_t*, void* )
    {
      if ( cd -> remains() <= timespan_t::zero() && listener -> health_percentage() < 35 )
      {
        heal -> target = listener;
        heal -> execute();
        proc -> occur();
        cd -> start();
      }

    }
  };

  p -> callbacks.register_incoming_attack_callback( RESULT_HIT_MASK,  new jikuns_rising_winds_callback_t( p, new jikuns_rising_winds_heal_t( *item, spell ) )  );
}

// delicate_vial_of_the_sanguinaire =========================================

void delicate_vial_of_the_sanguinaire( item_t* item )
{
  maintenance_check( 502 );

  player_t* p = item -> player;

  item -> unique = true;


  const spell_data_t* spell = item -> player -> find_spell( 138865 );

  special_effect_t data;
  data.name_str    = "delicate_vial_of_the_sanguinaire";
  data.duration    = spell -> duration();
  data.max_stacks  = spell -> max_stacks();
  data.proc_chance = spell -> proc_chance();

  struct delicate_vial_of_the_sanguinaire_callback_t : public proc_callback_t<action_state_t>
  {
    stat_buff_t* buff;

    delicate_vial_of_the_sanguinaire_callback_t( item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data ),
      buff( nullptr )
    {
      const spell_data_t* spell = listener -> find_spell( 138864 );

      const random_prop_data_t& budget = listener -> dbc.random_property( i.item_level() );

      buff   = stat_buff_creator_t( listener, "blood_of_power" )
               .duration( spell -> duration() )
               .add_stat( STAT_MASTERY_RATING, budget.p_epic[ 0 ]  * spell  -> effectN( 1 ).m_average() );
    }

    void execute( action_t* /* action */, action_state_t* /* state */ )
    {
      buff -> trigger();
    }
  };

  p -> callbacks.register_incoming_attack_callback( RESULT_DODGE_MASK, new delicate_vial_of_the_sanguinaire_callback_t( *item, data ) );
}

// zen_alchemist_stone ======================================================

void zen_alchemist_stone( item_t* item )
{
  struct zen_alchemist_stone_callback : public proc_callback_t<action_state_t>
  {
    stat_buff_t* buff_str;
    stat_buff_t* buff_agi;
    stat_buff_t* buff_int;

    zen_alchemist_stone_callback( item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      const spell_data_t* spell = listener -> find_spell( 105574 );

      struct common_buff_creator : public stat_buff_creator_t
      {
        common_buff_creator( player_t* p, const std::string& n, const spell_data_t* spell ) :
          stat_buff_creator_t ( p, "zen_alchemist_stone_" + n, spell  )
        {
          duration( p -> find_spell( 60229 ) -> duration() );
          chance( 1.0 );
          activated( false );
        }
      };

      const random_prop_data_t& budget = listener -> dbc.random_property( i.item_level() );
      double value = budget.p_rare[ 0 ] * spell -> effectN( 1 ).m_average();

      buff_str = common_buff_creator( listener, "str", spell )
                 .add_stat( STAT_STRENGTH, value );
      buff_agi = common_buff_creator( listener, "agi", spell )
                 .add_stat( STAT_AGILITY, value );
      buff_int = common_buff_creator( listener, "int", spell )
                 .add_stat( STAT_INTELLECT, value );
    }

    void execute( action_t* a, action_state_t* /* state */ )
    {
      player_t* p = a -> player;

      if ( p -> strength() > p -> agility() )
      {
        if ( p -> strength() > p -> intellect() )
          buff_str -> trigger();
        else
          buff_int -> trigger();
      }
      else if ( p -> agility() > p -> intellect() )
        buff_agi -> trigger();
      else
        buff_int -> trigger();
    }
  };

  maintenance_check( 450 );

  item -> unique = true;

  special_effect_t data;
  data.name_str    = "zen_alchemist_stone";
  data.cooldown    = timespan_t::from_seconds( 55.0 );
  data.proc_chance = 1;

  zen_alchemist_stone_callback* cb = new zen_alchemist_stone_callback( *item, data );
  item -> player -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  item -> player -> callbacks.register_tick_damage_callback( SCHOOL_ALL_MASK, cb );
  item -> player -> callbacks.register_direct_heal_callback( RESULT_ALL_MASK, cb );
}

// bad_juju =================================================================

void bad_juju( item_t* item )
{
  // TODO: Gnomes of Doom
  struct bad_juju_callback_t : public stat_buff_proc_t
  {
    std::vector<pet_t*> gnomes;

    bad_juju_callback_t( item_t& i, const special_effect_t& data ) :
      stat_buff_proc_t( i.player, data )
    {
      const spell_data_t* spell = listener -> find_spell( 138939 );

      gnomes.resize( static_cast< int >( spell -> effectN( 1 ).base_value() ) );
    }

    virtual void execute( action_t* action, action_state_t* state )
    {
      stat_buff_proc_t::execute( action, state );
      for ( size_t i = 0; i < gnomes.size(); i++ )
        if ( gnomes[ i ] ) gnomes[ i ] -> summon( buff -> buff_duration );
    }
  };

  maintenance_check( 502 );

  item -> unique = true;

  const spell_data_t* driver = item -> player -> find_spell( 138939 );
  const spell_data_t* spell = item -> player -> find_spell( 138938 );
  const random_prop_data_t& budget = item -> player -> dbc.random_property( item -> item_level() );

  special_effect_t data;
  data.name_str    = "juju_madness";
  data.ppm         = -1.0 * driver -> real_ppm();
  data.stat        = STAT_AGILITY;
  data.stat_amount = util::round( budget.p_epic[ 0 ] * spell -> effectN( 1 ).m_average() );
  data.duration    = spell -> duration();
  data.cooldown    = driver -> internal_cooldown();

  item -> player -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, new bad_juju_callback_t( *item, data ) );
}

// rune_of_reorigination ====================================================

// TODO: How does this interact with rating multipliers
void rune_of_reorigination( item_t* item )
{
  struct rune_of_reorigination_callback_t : public proc_callback_t<action_state_t>
  {
    enum
    {
      BUFF_CRIT = 0,
      BUFF_HASTE,
      BUFF_MASTERY
    };

    stat_buff_t* buff;

    rune_of_reorigination_callback_t( item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      buff = stat_buff_creator_t( listener, proc_data.name_str )
             .activated( false )
             .duration( proc_data.duration )
             .add_stat( STAT_CRIT_RATING, 0 )
             .add_stat( STAT_HASTE_RATING, 0 )
             .add_stat( STAT_MASTERY_RATING, 0 );
    }

    virtual void execute( action_t* action, action_state_t* /* state */ )
    {
      // We can never allow this trinket to refresh, so force the trinket to 
      // always expire, before we proc a new one.
      buff -> expire();

      player_t* p = action -> player;

      // Determine highest stat based on rating multipliered stats
      double chr = p -> composite_melee_haste_rating();
      if ( p -> sim -> scaling -> scale_stat == STAT_HASTE_RATING )
        chr -= p -> sim -> scaling -> scale_value * p -> composite_rating_multiplier( RATING_MELEE_HASTE );

      double ccr = p -> composite_melee_crit_rating();
      if ( p -> sim -> scaling -> scale_stat == STAT_CRIT_RATING )
        ccr -= p -> sim -> scaling -> scale_value * p -> composite_rating_multiplier( RATING_MELEE_CRIT );

      double cmr = p -> composite_mastery_rating();
      if ( p -> sim -> scaling -> scale_stat == STAT_MASTERY_RATING )
        cmr -= p -> sim -> scaling -> scale_value * p -> composite_rating_multiplier( RATING_MASTERY );

      // Give un-multipliered stats so we don't double dip anywhere.
      chr /= p -> composite_rating_multiplier( RATING_MELEE_HASTE );
      ccr /= p -> composite_rating_multiplier( RATING_MELEE_CRIT );
      cmr /= p -> composite_rating_multiplier( RATING_MASTERY );
    
      if ( p -> sim -> debug )
        p -> sim -> out_debug.printf( "%s rune_of_reorigination procs crit=%.0f haste=%.0f mastery=%.0f",
                            p -> name(), ccr, chr, cmr );

      if ( ccr >= chr )
      {
        // I choose you, crit
        if ( ccr >= cmr )
        {
          buff -> stats[ BUFF_CRIT    ].amount = 2 * ( chr + cmr );
          buff -> stats[ BUFF_HASTE   ].amount = -chr;
          buff -> stats[ BUFF_MASTERY ].amount = -cmr;
        }
        // I choose you, mastery
        else
        {
          buff -> stats[ BUFF_CRIT    ].amount = -ccr;
          buff -> stats[ BUFF_HASTE   ].amount = -chr;
          buff -> stats[ BUFF_MASTERY ].amount = 2 * ( ccr + chr );
        }
      }
      // I choose you, haste
      else if ( chr >= cmr )
      {
        buff -> stats[ BUFF_CRIT    ].amount = -ccr;
        buff -> stats[ BUFF_HASTE   ].amount = 2 * ( ccr + cmr );
        buff -> stats[ BUFF_MASTERY ].amount = -cmr;
      }
      // I choose you, mastery
      else
      {
        buff -> stats[ BUFF_CRIT    ].amount = -ccr;
        buff -> stats[ BUFF_HASTE   ].amount = -chr;
        buff -> stats[ BUFF_MASTERY ].amount = 2 * ( ccr + chr );
      }

      buff -> trigger();
    }
  };

  maintenance_check( 502 );

  item -> unique = true;

  const spell_data_t* driver = item -> player -> find_spell( 139116 );
  const spell_data_t* spell = item -> player -> find_spell( 139120 );

  special_effect_t data;
  data.name_str    = "rune_of_reorigination";
  data.ppm         = -1.0 * driver -> real_ppm();
  data.ppm        *= item_database::approx_scale_coefficient( 528, item -> item_level() );
  data.cooldown    = driver -> internal_cooldown(); 
  data.duration    = spell -> duration();

  item -> player -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, new rune_of_reorigination_callback_t( *item, data ) );
}

// spark_of_zandalar ========================================================

void spark_of_zandalar( item_t* item )
{
  maintenance_check( 502 );

  item -> unique = true;

  const spell_data_t* driver = item -> player -> find_spell( 138957 );
  const spell_data_t* spell = item -> player -> find_spell( 138958 );

  special_effect_t data;
  data.name_str    = "spark_of_zandalar";
  data.ppm         = -1.0 * driver -> real_ppm();
  data.duration    = spell -> duration();
  data.max_stacks  = spell -> max_stacks();

  struct spark_of_zandalar_callback_t : public proc_callback_t<action_state_t>
  {
    buff_t*      sparks;
    stat_buff_t* buff;

    spark_of_zandalar_callback_t( item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      sparks = buff_creator_t( listener, proc_data.name_str )
               .activated( false )
               .duration( proc_data.duration )
               .max_stack( proc_data.max_stacks );

      const spell_data_t* spell = listener -> find_spell( 138960 );
      const random_prop_data_t& budget = listener -> dbc.random_property( i.item_level() );

      buff = stat_buff_creator_t( listener, "zandalari_warrior" )
             .duration( spell -> duration() )
             .add_stat( STAT_STRENGTH, budget.p_epic[ 0 ] * spell -> effectN( 2 ).m_average() );
    }

    void execute( action_t* /* action */, action_state_t* /* state */ )
    {
      sparks -> trigger();

      if ( sparks -> stack() == sparks -> max_stack() )
      {
        sparks -> expire();
        buff   -> trigger();
      }
    }
  };

  item -> player -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, new spark_of_zandalar_callback_t( *item, data ) );
};

// unnerring_vision_of_leishen ==============================================

void unerring_vision_of_leishen( item_t* item )
{
  struct perfect_aim_buff_t : public buff_t
  {
    perfect_aim_buff_t( player_t* p, const spell_data_t* s ) :
      buff_t( buff_creator_t( p, "perfect_aim", s ).activated( false ) )
    { }

    void execute( int stacks, double value, timespan_t duration )
    {
      if ( current_stack == 0 )
      {
        player -> current.spell_crit  += data().effectN( 1 ).percent();
        player -> current.attack_crit += data().effectN( 1 ).percent();
        player -> invalidate_cache( CACHE_CRIT );
      }

      buff_t::execute( stacks, value, duration );
    }

    void expire_override()
    {
      buff_t::expire_override();

      player -> current.spell_crit  -= data().effectN( 1 ).percent();
      player -> current.attack_crit -= data().effectN( 1 ).percent();
      player -> invalidate_cache( CACHE_CRIT );
    }
  };

  struct unerring_vision_of_leishen_callback_t : public proc_callback_t<action_state_t>
  {
    perfect_aim_buff_t* buff;

    unerring_vision_of_leishen_callback_t( item_t& i, const special_effect_t& data, const spell_data_t* driver ) :
      proc_callback_t<action_state_t>( i.player, data, driver )
    {
      buff = new perfect_aim_buff_t( listener, listener -> find_spell( 138963 ) );
      if ( i.player -> type == WARLOCK )
        rppm.set_modifier( 0.6 );
    }

    void execute( action_t* /* action */, action_state_t* /* state */ )
    { buff -> trigger(); }
  };

  maintenance_check( 502 );

  item -> unique = true;

  const spell_data_t* driver = item -> player -> find_spell( 138964 );

  special_effect_t data;
  data.name_str    = "perfect_aim";
  data.ppm         = -1.0 * driver -> real_ppm();
  data.ppm        *= item_database::approx_scale_coefficient( 528, item -> item_level() );
  data.cooldown    = driver -> internal_cooldown();

  unerring_vision_of_leishen_callback_t* cb = new unerring_vision_of_leishen_callback_t( *item, data, driver );

  item -> player -> callbacks.register_spell_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  item -> player -> callbacks.register_spell_tick_damage_callback( SCHOOL_ALL_MASK, cb );
}

// Cleave trinkets

template <typename T>
struct cleave_t : public T
{
  cleave_t( item_t* item, const char* name, school_e s ) :
    T( name, item -> player )
  {
    this -> callbacks = false;
    this -> may_crit = false;
    this -> may_glance = false;
    this -> may_miss = true;
    this -> special = true;
    this -> proc = true;
    this -> background = true;
    this -> school = s;
    this -> aoe = 5;
    this -> snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;
    if ( this -> type == ACTION_ATTACK )
    {
      this -> may_dodge = true;
      this -> may_parry = true;
      this -> may_block = true;
    }
  }

  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    tl.clear();

    for ( size_t i = 0, actors = this -> sim -> target_non_sleeping_list.size(); i < actors; i++ )
    {
      player_t* t = this -> sim -> target_non_sleeping_list[ i ];

      if ( t -> is_enemy() && ( t != this -> target ) )
        tl.push_back( t );
    }

    return tl.size();
  }

  double composite_target_multiplier( player_t* ) const
  { return 1.0; }

  double composite_da_multiplier() const
  { return 1.0; }

  double target_armor( player_t* ) const
  { return 0.0; }
};

void cleave_trinket( item_t* item )
{
  maintenance_check( 528 );

  struct cleave_callback_t : public proc_callback_t<action_state_t>
  {
    cleave_t<spell_t>* cleave_spell;
    cleave_t<attack_t>* cleave_attack;

    cleave_callback_t( item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      cleave_spell = new cleave_t<spell_t>( &i, "cleave_spell", SCHOOL_NATURE );
      cleave_attack = new cleave_t<attack_t>( &i, "cleave_attack", SCHOOL_PHYSICAL );
    }

    void execute( action_t* action, action_state_t* state )
    {
      action_t* a = 0;

      if ( action -> type == ACTION_ATTACK )
        a = cleave_attack;
      else if ( action -> type == ACTION_SPELL )
        a = cleave_spell;
      // TODO: Heal

      if ( a )
      {
        a -> base_dd_min = a -> base_dd_max = state -> result_amount;
        a -> target = state -> target;
        a -> schedule_execute();
      }
    }
  };

  player_t* p = item -> player;
  const random_prop_data_t& budget = p -> dbc.random_property( item -> item_level() );
  const spell_data_t* cleave_driver_spell = spell_data_t::nil();
  const spell_data_t* stat_driver_spell = spell_data_t::nil();

  for ( size_t i = 0; i < sizeof_array( item -> parsed.data.id_spell ); i++ )
  {
    if ( item -> parsed.data.id_spell[ i ] <= 0 ||
         item -> parsed.data.trigger_spell[ i ] != ITEM_SPELLTRIGGER_ON_EQUIP )
      continue;

    const spell_data_t* s = p -> find_spell( item -> parsed.data.id_spell[ i ] );
    if ( s -> effectN( 1 ).trigger() -> id() != 0 )
      stat_driver_spell = s;
    else
      cleave_driver_spell = s;
  }

  std::string name = cleave_driver_spell -> name_cstr();
  util::tokenize( name );
  special_effect_t effect;
  effect.name_str = name;
  effect.proc_chance = budget.p_epic[ 0 ] * cleave_driver_spell -> effectN( 1 ).m_average() / 10000.0;

  cleave_callback_t* cb = new cleave_callback_t( *item, effect );
  p -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  p -> callbacks.register_tick_damage_callback( SCHOOL_ALL_MASK, cb );

  const spell_data_t* stat_spell = stat_driver_spell -> effectN( 1 ).trigger();
  effect.clear();
  name = stat_spell -> name_cstr();
  util::tokenize( name );
  effect.name_str = name;
  effect.duration = stat_spell -> duration();
  effect.proc_chance = stat_driver_spell -> proc_chance();
  effect.cooldown = stat_driver_spell -> internal_cooldown();
  effect.stat = static_cast< stat_e >( stat_spell -> effectN( 1 ).misc_value1() + 1 );
  effect.stat_amount = util::round( budget.p_epic[ 0 ] * stat_spell -> effectN( 1 ).m_average() );

  stat_buff_proc_t* stat_cb = new stat_buff_proc_t( item -> player, effect );
  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, stat_cb );
  p -> callbacks.register_harmful_spell_callback( RESULT_HIT_MASK, stat_cb );
  p -> items[ item -> slot ].parsed.equip = effect;
}

// Multistrike trinkets
void multistrike_trinket( item_t* item )
{
  maintenance_check( 528 );

  // TODO: Healing multistrike

  struct multistrike_attack_t : public attack_t
  {
    multistrike_attack_t( item_t* item ) :
      attack_t( "multistrike_attack", item -> player )
    {
      callbacks = may_crit = may_glance = false;
      proc = background = special = true;
      school = SCHOOL_PHYSICAL;
      snapshot_flags |= STATE_MUL_DA;
    }

    double composite_target_multiplier( player_t* ) const
    { return 1.0; }

    double composite_da_multiplier() const
    { return 1.0 / 3.0; }

    double target_armor( player_t* ) const
    { return 0.0; }
  };

  struct multistrike_spell_t : public spell_t
  {
    multistrike_spell_t( item_t* item ) :
      spell_t( "multistrike_spell", item -> player )
    {
      callbacks = may_crit = false;
      proc = background = true;
      school = SCHOOL_NATURE; // Multiple schools in reality, but any school would work
      snapshot_flags |= STATE_MUL_DA;
    }

    double composite_target_multiplier( player_t* ) const
    { return 1.0; }

    double composite_da_multiplier() const
    { return 1.0 / 3.0; }

    double target_armor( player_t* ) const
    { return 0.0; }
  };

  struct multistrike_callback_t : public proc_callback_t<action_state_t>
  {
    action_t* strike_attack;
    action_t* strike_spell;

    multistrike_callback_t( item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      if ( ! ( strike_attack = listener -> create_proc_action( "multistrike_attack" ) ) )
      {
        strike_attack = new multistrike_attack_t( &i );
        strike_attack -> init();
      }

      if ( ! ( strike_spell = listener -> create_proc_action( "multistrike_spell" ) ) )
      {
        strike_spell = new multistrike_spell_t( &i );
        strike_spell -> init();
      }
    }

    void execute( action_t* action, action_state_t* state )
    {
      action_t* a = 0;

      if ( action -> type == ACTION_ATTACK )
        a = strike_attack;
      else if ( action -> type == ACTION_SPELL )
        a = strike_spell;
      // TODO: Heal

      if ( a )
      {
        a -> base_dd_min = a -> base_dd_max = state -> result_amount;
        a -> target = state -> target;
        a -> schedule_execute();
      }
    }
  };

  player_t* p = item -> player;
  const random_prop_data_t& budget = p -> dbc.random_property( item -> item_level() );
  const spell_data_t* ms_driver_spell = spell_data_t::nil();
  const spell_data_t* stat_driver_spell = spell_data_t::nil();

  for ( size_t i = 0; i < sizeof_array( item -> parsed.data.id_spell ); i++ )
  {
    if ( item -> parsed.data.id_spell[ i ] <= 0 ||
         item -> parsed.data.trigger_spell[ i ] != ITEM_SPELLTRIGGER_ON_EQUIP )
      continue;

    const spell_data_t* s = p -> find_spell( item -> parsed.data.id_spell[ i ] );
    if ( s -> effectN( 1 ).trigger() -> id() != 0 )
      stat_driver_spell = s;
    else
      ms_driver_spell = s;
  }

  if ( ms_driver_spell -> id() == 0 || stat_driver_spell -> id() == 0 )
    return;

  // Multistrike
  std::string name = ms_driver_spell -> name_cstr();
  util::tokenize( name );
  special_effect_t effect;
  effect.name_str = name;
  effect.proc_chance = budget.p_epic[ 0 ] * ms_driver_spell -> effectN( 1 ).m_average() / 1000.0;

  multistrike_callback_t* cb = new multistrike_callback_t( *item, effect );
  p -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  p -> callbacks.register_tick_damage_callback( SCHOOL_ALL_MASK, cb );

  // Stat buff (Int or Agi)
  effect.clear();
  const spell_data_t* stat_spell = stat_driver_spell -> effectN( 1 ).trigger();
  name = stat_spell -> name_cstr();
  util::tokenize( name );

  effect.name_str = name;
  effect.ppm = -1.0 * stat_driver_spell -> real_ppm();
  effect.duration = stat_spell -> duration();
  effect.stat = static_cast< stat_e >( stat_spell -> effectN( 1 ).misc_value1() + 1 );
  effect.stat_amount = util::round( budget.p_epic[ 0 ] * stat_spell -> effectN( 1 ).m_average() );
  effect.cooldown = stat_driver_spell -> internal_cooldown();

  stat_buff_proc_t* stat_cb = new stat_buff_proc_t( item -> player, effect );
  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, stat_cb );
  p -> callbacks.register_harmful_spell_callback( RESULT_HIT_MASK, stat_cb );
  p -> items[ item -> slot ].parsed.equip = effect;
}

// CDR trinkets
void cooldown_reduction_trinket( item_t* item )
{
  maintenance_check( 528 );

  struct cooldowns_t
  {
    specialization_e spec;
    const char*      cooldowns[8];
  };

  static const cooldowns_t __cd[] =
  {
    // NOTE: Spells that trigger buffs must have the cooldown of their buffs removed if they have one, or this trinket may cause undesirable results.
    { ROGUE_ASSASSINATION, { "evasion", "vanish", "cloak_of_shadows", "vendetta", "shadow_blades", 0, 0 } },
    { ROGUE_COMBAT,        { "evasion", "adrenaline_rush", "cloak_of_shadows", "killing_spree", "shadow_blades", 0, 0 } },
    { ROGUE_SUBTLETY,      { "evasion", "vanish", "cloak_of_shadows", "shadow_dance", "shadow_blades", 0, 0 } },
    { SHAMAN_ENHANCEMENT,  { "spiritwalkers_grace", "earth_elemental_totem", "fire_elemental_totem", "shamanistic_rage", "ascendance", "feral_spirit", 0 } },
    { DRUID_FERAL,         { "tigers_fury", "berserk", "barkskin", "survival_instincts", 0, 0, 0 } },
    { DRUID_GUARDIAN,      { "might_of_ursoc", "berserk", "barkskin", "survival_instincts", 0, 0, 0 } },
    { WARRIOR_FURY,        { "dragon_roar", "bladestorm", "shockwave", "avatar", "bloodbath", "recklessness", "storm_bolt", "heroic_leap" } },
    { WARRIOR_ARMS,        { "dragon_roar", "bladestorm", "shockwave", "avatar", "bloodbath", "recklessness", "storm_bolt", "heroic_leap" } },
    { WARRIOR_PROTECTION,  { "shield_wall", "demoralizing_shout", "last_stand", "recklessness", "heroic_leap", 0, 0 } },
    { DEATH_KNIGHT_BLOOD,  { "antimagic_shell", "dancing_rune_weapon", "icebound_fortitude", "outbreak", "vampiric_blood", "bone_shield", 0 } },
    { DEATH_KNIGHT_FROST,  { "antimagic_shell", "army_of_the_dead", "icebound_fortitude", "empower_rune_weapon", "outbreak", "pillar_of_frost", 0  } },
    { DEATH_KNIGHT_UNHOLY, { "antimagic_shell", "army_of_the_dead", "icebound_fortitude", "unholy_frenzy", "outbreak", "summon_gargoyle", 0 } },
    { MONK_BREWMASTER,	   { "fortifying_brew", "guard", "zen_meditation", 0, 0, 0, 0 } }, 
    { MONK_WINDWALKER,     { "energizing_brew", "fists_of_fury", "fortifying_brew", "zen_meditation", 0, 0, 0 } },
	{ PALADIN_PROTECTION,  { "ardent_defender", "avenging_wrath", "divine_protection", "divine_shield", "guardian_of_ancient_kings", 0 } },
    { PALADIN_RETRIBUTION, { "avenging_wrath", "divine_protection", "divine_shield", "guardian_of_ancient_kings", 0, 0 } },
    { HUNTER_BEAST_MASTERY,{ "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", "bestial_wrath", 0 } },
    { HUNTER_MARKSMANSHIP, { "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", 0, 0 } },
    { HUNTER_SURVIVAL,     { "black_arrow", "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", 0 } },
    { SPEC_NONE,           { 0 } }
  };

  const spell_data_t* cdr_spell = spell_data_t::nil();
  const spell_data_t* proc_driver_spell = spell_data_t::nil();
  player_t* p = item -> player;

  // Find the CDR spell on the trinket. Presume that the trinkets have one or
  // two on-equip spells, and one of them procs a trigger spell, while the CDR
  // part is passive.
  for ( size_t i = 0; i < sizeof_array( item -> parsed.data.id_spell ); i++ )
  {
    if ( item -> parsed.data.id_spell[ i ] <= 0 ||
         item -> parsed.data.trigger_spell[ i ] != ITEM_SPELLTRIGGER_ON_EQUIP )
      continue;

    const spell_data_t* spell = p -> find_spell( item -> parsed.data.id_spell[ i ] );
    if ( spell -> proc_flags() == 0 )
      cdr_spell = spell;
    else
      proc_driver_spell = spell;
  }

  if ( cdr_spell == spell_data_t::nil() )
    return;

  const random_prop_data_t& budget = p -> dbc.random_property( item -> item_level() );
  double cdr = 1.0 / ( 1.0 + budget.p_epic[ 0 ] * cdr_spell -> effectN( 1 ).m_average() / 100.0 );

  p -> buffs.cooldown_reduction -> s_data = cdr_spell;
  p -> buffs.cooldown_reduction -> default_value = cdr;
  p -> buffs.cooldown_reduction -> default_chance = 1;

  const cooldowns_t* cd = &( __cd[ 0 ] );
  do
  {
    if ( p -> specialization() != cd -> spec )
    {
      cd++;
      continue;
    }

    for ( size_t i = 0; i < 7; i++ )
    {
      if ( cd -> cooldowns[ i ] == 0 )
        break;

      cooldown_t* ability_cd = p -> get_cooldown( cd -> cooldowns[ i ] );
      ability_cd -> set_recharge_multiplier( cdr );
    }

    break;
  } while ( cd -> spec != SPEC_NONE );

  // Tank trinket has no separate proc, so bail out here.
  if ( proc_driver_spell == spell_data_t::nil() )
    return;

  const spell_data_t* proc_spell = proc_driver_spell -> effectN( 1 ).trigger();
  if ( proc_spell == spell_data_t::nil() )
    return;

  std::string name = proc_spell -> name_cstr();
  util::tokenize( name );
  special_effect_t effect;
  effect.name_str = name;
  effect.proc_chance = proc_driver_spell -> proc_chance();
  effect.cooldown = proc_driver_spell -> internal_cooldown();
  effect.duration = proc_spell -> duration();
  effect.stat = static_cast< stat_e >( proc_spell -> effectN( 1 ).misc_value1() + 1 );
  effect.stat_amount = util::round( budget.p_epic[ 0 ] * proc_spell -> effectN( 1 ).m_average() );

  stat_buff_proc_t* cb = new stat_buff_proc_t( p, effect );
  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  p -> callbacks.register_harmful_spell_callback( RESULT_HIT_MASK, cb );
  p -> items[ item -> slot ].parsed.equip = effect;
}

void amplify_trinket( item_t* item )
{
  maintenance_check( 528 );

  const spell_data_t* amplify_spell = spell_data_t::nil();
  const spell_data_t* stat_driver_spell = spell_data_t::nil();
  player_t* p = item -> player;

  for ( size_t i = 0; i < sizeof_array( item -> parsed.data.id_spell ); i++ )
  {
    if ( item -> parsed.data.id_spell[ i ] <= 0 ||
         item -> parsed.data.trigger_spell[ i ] != ITEM_SPELLTRIGGER_ON_EQUIP )
      continue;

    const spell_data_t* spell = p -> find_spell( item -> parsed.data.id_spell[ i ] );
    if ( spell -> proc_flags() == 0 )
      amplify_spell = spell;
    else
      stat_driver_spell = spell;
  }

  if ( stat_driver_spell -> id() == 0 || amplify_spell -> id() == 0 )
    return;

  const random_prop_data_t& budget = p -> dbc.random_property( item -> item_level() );
  p -> buffs.amplified -> default_value = budget.p_epic[ 0 ] * amplify_spell -> effectN( 2 ).m_average() / 100.0;
  p -> buffs.amplified -> default_chance = 1.0;

  const spell_data_t* stat_spell = stat_driver_spell -> effectN( 1 ).trigger();

  std::string name = stat_spell -> name_cstr();
  util::tokenize( name );
  special_effect_t effect;
  effect.name_str = name;
  effect.proc_chance = stat_driver_spell -> proc_chance();
  effect.cooldown = stat_driver_spell -> internal_cooldown();
  effect.duration = stat_spell -> duration();
  effect.stat = static_cast< stat_e >( stat_spell -> effectN( 1 ).misc_value1() + 1 );
  effect.stat_amount = util::round( budget.p_epic[ 0 ] * stat_spell -> effectN( 1 ).m_average() );

  stat_buff_proc_t* cb = new stat_buff_proc_t( p, effect );

  if ( util::str_compare_ci( item -> name(), "prismatic_prison_of_pride" ) )
  {
    p -> callbacks.register_direct_heal_callback( RESULT_ALL_MASK, cb );
    p -> callbacks.register_tick_heal_callback( RESULT_ALL_MASK, cb );
  }
  else
  {
    p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
    p -> callbacks.register_harmful_spell_callback( RESULT_HIT_MASK, cb );
  }
  p -> items[ item -> slot ].parsed.equip = effect;
}

struct flurry_of_xuen_melee_t : public attack_t
{
  flurry_of_xuen_melee_t( player_t* player ) : 
    attack_t( "flurry_of_xuen", player, player -> find_spell( 147891 ) )
  {
    direct_power_mod = data().extra_coeff();
    background = true;
    proc = false;
    aoe = 5;
    special = may_crit = true;
  }
};

struct flurry_of_xuen_ranged_t : public ranged_attack_t
{
  flurry_of_xuen_ranged_t( player_t* player ) : 
    ranged_attack_t( "flurry_of_xuen", player, player -> find_spell( 147891 ) )
  {
    direct_power_mod = data().extra_coeff();
    background = true;
    proc = false;
    aoe = 5;
    special = may_crit = true;
  }
};

struct flurry_of_xuen_driver_t : public attack_t
{
  flurry_of_xuen_driver_t( player_t* player, action_t* action = 0 ) :
    attack_t( "flurry_of_xuen", player, player -> find_spell( 146194 ) )
  {
    hasted_ticks = may_miss = may_dodge = may_parry = may_crit = may_block = callbacks = false;
    proc = background = dual = true;

    if ( ! action )
    {
      if ( player -> type == HUNTER )
        action = new flurry_of_xuen_ranged_t( player );
      else
        action = new flurry_of_xuen_melee_t( player );
    }
    tick_action = action;
    dynamic_tick_action = true;
  }
};

struct flurry_of_xuen_cb_t : public proc_callback_t<action_state_t>
{
  action_t* action;

  flurry_of_xuen_cb_t( item_t* item, const special_effect_t& effect, const spell_data_t* driver ) :
    proc_callback_t<action_state_t>( item -> player, effect, driver ),
    action( new flurry_of_xuen_driver_t( listener, listener -> create_proc_action( effect.name_str ) ) )

  { }

  void trigger( action_t* action, void* call_data )
  {
    // Flurry of Xuen, and Lightning Strike cannot proc Flurry of Xuen
    if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
      return;

    proc_callback_t<action_state_t>::trigger( action, call_data );
  }

  void execute( action_t*, action_state_t* state )
  {
    action -> target = state -> target;
    action -> schedule_execute(); 
  }
};

void flurry_of_xuen( item_t* item )
{
  maintenance_check( 600 );

  player_t* p = item -> player;
  const spell_data_t* driver = p -> find_spell( 146195 );
  std::string name = driver -> name_cstr();
  util::tokenize( name );

  special_effect_t effect;
  effect.name_str   = name;
  effect.ppm        = -1.0 * driver -> real_ppm();
  effect.ppm       *= item_database::approx_scale_coefficient( item -> parsed.data.level, item -> item_level() );
  effect.rppm_scale = RPPM_HASTE;
  effect.cooldown   = driver -> internal_cooldown();

  flurry_of_xuen_cb_t* cb = new flurry_of_xuen_cb_t( item, effect, driver );

  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

// Xin-Ho, Breath of Yu'lon

struct essence_of_yulon_t : public spell_t
{
  essence_of_yulon_t( player_t* p, const spell_data_t& driver ) :
    spell_t( "essence_of_yulon", p, p -> find_spell( 148008 ) )
  {
    background = may_crit = true;
    proc = false;
    aoe = 5;
    direct_power_mod /= driver.duration().total_seconds() + 1;
  }
};

struct essence_of_yulon_driver_t : public spell_t
{
  essence_of_yulon_driver_t( player_t* player ) :
    spell_t( "essence_of_yulon", player, player -> find_spell( 146198 ) )
  {
    hasted_ticks = may_miss = may_dodge = may_parry = may_block = callbacks = false;
    tick_zero = proc = background = dual = true;
    travel_speed = 0;

    tick_action = new essence_of_yulon_t( player, data() );
    dynamic_tick_action = true;
  }
};

struct essence_of_yulon_cb_t : public proc_callback_t<action_state_t>
{
  action_t* action;

  essence_of_yulon_cb_t( item_t* item, const special_effect_t& effect, const spell_data_t* driver ) :
    proc_callback_t<action_state_t>( item -> player, effect, driver ),
    action( new essence_of_yulon_driver_t( item -> player ) )
  { }

  void execute( action_t*, action_state_t* state )
  {
    action -> target = state -> target;
    action -> schedule_execute();
  }

  void trigger( action_t* action, void* call_data )
  {
    if ( action -> id == 148008 )
      return;

    proc_callback_t<action_state_t>::trigger( action, call_data );
  }
};

void essence_of_yulon( item_t* item )
{
  maintenance_check( 600 );

  player_t* p = item -> player;
  const spell_data_t* driver = p -> find_spell( 146197 );
  std::string name = driver -> name_cstr();
  util::tokenize( name );

  special_effect_t effect;
  effect.name_str = name;
  effect.ppm = -1.0 * driver -> real_ppm();
  effect.ppm        *= item_database::approx_scale_coefficient( item -> parsed.data.level, item -> item_level() );
  effect.rppm_scale = RPPM_HASTE;

  essence_of_yulon_cb_t* cb = new essence_of_yulon_cb_t( item, effect, driver );
  p -> callbacks.register_spell_direct_damage_callback( SCHOOL_ALL_MASK, cb );
}

// Qiang-Ying, Fortitude of Niuzao and Qian-Le, Courage of Niuzao

void endurance_of_niuzao( item_t* item )
{
  maintenance_check( 600 );

  player_t* p = item -> player;

  p -> legendary_tank_cloak_cd = p -> get_cooldown( "endurance_of_niuzao" );
  p -> legendary_tank_cloak_cd -> duration = p -> find_spell( 148010 ) -> duration();
  //  max_absorb = player -> find_spell( 146193 ) -> effectN( 1 ).base_value();

}

} // end unique_gear namespace

using namespace enchants;
using namespace unique_items;

} // UNNAMED NAMESPACE

// ==========================================================================
// unique_gear::init
// ==========================================================================

void unique_gear::init( player_t* p )
{
  if ( p -> is_pet() ) return;

  if ( p -> race == RACE_UNDEAD )
  {
    touch_of_the_grave( p );
  }

  for ( size_t i = 0; i < p -> items.size(); i++ )
  {
    item_t& item = p -> items[ i ];

    if ( item.parsed.equip.stat && item.parsed.equip.school )
    {
      register_stat_discharge_proc( item, item.parsed.equip );
    }
    else if ( item.parsed.equip.stat )
    {
      register_stat_proc( p, item.parsed.equip );
    }
    else if ( item.parsed.equip.cost_reduction && item.parsed.equip.school )
    {
      register_cost_reduction_proc( p, item.parsed.equip );
    }
    else if ( item.parsed.equip.school && item.parsed.equip.proc_chance && item.parsed.equip.chance_to_discharge )
    {
      register_chance_discharge_proc( item, item.parsed.equip );
    }
    else if ( item.parsed.equip.school )
    {
      register_discharge_proc( item, item.parsed.equip );
    }

    if ( ! strcmp( item.name(), "zen_alchemist_stone"                 ) ) zen_alchemist_stone               ( &item );
    if ( ! strcmp( item.name(), "bad_juju"                            ) ) bad_juju                          ( &item );
    if ( ! strcmp( item.name(), "delicate_vial_of_the_sanguinaire"    ) ) delicate_vial_of_the_sanguinaire  ( &item );
    if ( ! strcmp( item.name(), "jikuns_rising_winds"                 ) ) jikuns_rising_winds               ( &item );
    if ( ! strcmp( item.name(), "rune_of_reorigination"               ) ) rune_of_reorigination             ( &item );
    if ( ! strcmp( item.name(), "spark_of_zandalar"                   ) ) spark_of_zandalar                 ( &item );
    if ( ! strcmp( item.name(), "unerring_vision_of_lei_shen"         ) ) unerring_vision_of_leishen        ( &item );

    if ( util::str_compare_ci( item.name(), "assurance_of_consequence"  ) ||
         util::str_compare_ci( item.name(), "evil_eye_of_galakras"      ) ||
         util::str_compare_ci( item.name(), "vial_of_living_corruption" ) )
      cooldown_reduction_trinket( &item );

    // No healing trinket for now
    if ( util::str_compare_ci( item.name(), "haromms_talisman"    ) ||
         util::str_compare_ci( item.name(), "kardris_toxic_totem" ) )
      multistrike_trinket( &item );

    // No healing trinket for now
    if ( util::str_compare_ci( item.name(), "sigil_of_rampage"         ) ||
         util::str_compare_ci( item.name(), "frenzied_crystal_of_rage" ) ||
         util::str_compare_ci( item.name(), "fusionfire_core"          ) )
      cleave_trinket( &item );

    if ( util::str_compare_ci( item.name(), "thoks_tail_tip"                 ) ||
         util::str_compare_ci( item.name(), "purified_bindings_of_immerseus" ) || 
         util::str_compare_ci( item.name(), "prismatic_prison_of_pride"      ) )
      amplify_trinket( &item );

    // Disable legendary cloaks in challenge mode
    if ( ! item.sim -> challenge_mode )
    {
      if ( util::str_compare_ci( item.name(), "fenyu_fury_of_xuen"      ) || 
          util::str_compare_ci( item.name(), "gonglu_strength_of_xuen" ) )
        flurry_of_xuen( &item );

      if ( util::str_compare_ci( item.name(), "xingho_breath_of_yulon" ) )
        essence_of_yulon( &item );

      if ( util::str_compare_ci( item.name(), "qianle_courage_of_niuzao" ) || 
          util::str_compare_ci( item.name(), "qianying_fortitude_of_niuzao" ) )
        endurance_of_niuzao( &item );
    }
  }
}

// ==========================================================================
// unique_gear::stat_proc
// ==========================================================================

action_callback_t* unique_gear::register_stat_proc( player_t* player,
                                                    special_effect_t& effect )
{
  action_callback_t* cb = new stat_buff_proc_t( player, effect );

  if ( effect.trigger_type == PROC_DAMAGE || effect.trigger_type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( effect.trigger_mask, cb );
    player -> callbacks.register_direct_damage_callback( effect.trigger_mask, cb );
  }
  if ( effect.trigger_type == PROC_HEAL || effect.trigger_type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( effect.trigger_mask, cb );
    player -> callbacks.register_direct_heal_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( effect.trigger_mask, cb );
    player -> callbacks.register_tick_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HARMFUL_SPELL_LANDING )
  {
    player -> callbacks.register_harmful_spell_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DAMAGE_HEAL_SPELL )
  {
    player -> callbacks.register_spell_callback( effect.trigger_mask, cb );
    player -> callbacks.register_heal_callback( effect.trigger_mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear::cost_reduction_proc
// ==========================================================================

action_callback_t* unique_gear::register_cost_reduction_proc( player_t* player,
                                                              special_effect_t& effect )
{
  action_callback_t* cb = new cost_reduction_buff_proc_t( player, effect );

  if ( effect.trigger_type == PROC_DAMAGE || effect.trigger_type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( effect.trigger_mask, cb );
    player -> callbacks.register_direct_damage_callback( effect.trigger_mask, cb );
  }
  if ( effect.trigger_type == PROC_HEAL || effect.trigger_type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( effect.trigger_mask, cb );
    player -> callbacks.register_direct_heal_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( effect.trigger_mask, cb );
    player -> callbacks.register_tick_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( effect.trigger_mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear::discharge_proc
// ==========================================================================

action_callback_t* unique_gear::register_discharge_proc( player_t* player,
                                                         special_effect_t& effect )
{
  action_callback_t* cb = new discharge_proc_callback_t( player, effect );

  const proc_e& type = effect.trigger_type;
  const int64_t& mask = effect.trigger_mask;

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( mask, cb );
    player -> callbacks.register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_AND_TICK )
  {
    player -> callbacks.register_spell_callback( mask, cb );
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear::chance_discharge_proc
// ==========================================================================

action_callback_t* unique_gear::register_chance_discharge_proc( player_t* player,
                                                                special_effect_t& effect )
{
  action_callback_t* cb = new chance_discharge_proc_callback_t( player, effect );

  const proc_e& type = effect.trigger_type;
  const int64_t& mask = effect.trigger_mask;

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL  || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( mask, cb );
    player -> callbacks.register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_AND_TICK )
  {
    player -> callbacks.register_spell_callback( mask, cb );
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear::stat_discharge_proc
// ==========================================================================

action_callback_t* unique_gear::register_stat_discharge_proc( player_t* player,
                                                              special_effect_t& effect )
{
  action_callback_t* cb = new discharge_proc_callback_t( player, effect );

  const proc_e& type = effect.trigger_type;
  const int64_t& mask = effect.trigger_mask;

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL  || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( mask, cb );
    player -> callbacks.register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear::discharge_proc
// ==========================================================================

action_callback_t* unique_gear::register_discharge_proc( item_t& i,
                                                         special_effect_t& e )
{
  return register_discharge_proc( i.player, e );
}

// ==========================================================================
// unique_gear::chance_discharge_proc
// ==========================================================================

action_callback_t* unique_gear::register_chance_discharge_proc( item_t& i,
                                                                special_effect_t& e )
{
  return register_chance_discharge_proc( i.player, e );
}

// ==========================================================================
// unique_gear::stat_discharge_proc
// ==========================================================================

action_callback_t* unique_gear::register_stat_discharge_proc( item_t& i,
                                                              special_effect_t& e )
{
  return register_stat_discharge_proc( i.player, e );
}

// ==========================================================================
// unique_gear::get_equip_encoding
// ==========================================================================

bool unique_gear::get_equip_encoding( std::string&       encoding,
                                      const std::string& name,
                                      unsigned           tf,
                                      bool /* ptr */,
                                      unsigned item_id )
{
  std::string e;

  // MoP
  if ( name == "vision_of_the_predator"                   ) e = "OnSpellDamage_3386Crit_15%_30Dur_115Cd";
  else if ( name == "carbonic_carbuncle"                  ) e = "OnDirectDamage_3386Crit_15%_30Dur_115Cd";
  else if ( name == "windswept_pages"                     ) e = "OnDirectDamage_3386Haste_15%_20Dur_65Cd";
  else if ( name == "searing_words"                       ) e = "OnDirectCrit_3386Agi_45%_25Dur_85Cd";
  else if ( name == "essence_of_terror"                   ) e = "OnHarmfulSpellHit_" + RTV( tf, 6121, 0, 6908, 0, 7796 ) + "Haste_15%_20Dur_115Cd";
  else if ( name == "terror_in_the_mists"                 ) e = "OnDirectDamage_"    + RTV( tf, 6121, 0, 6908, 0, 7796 ) + "Crit_15%_20Dur_115Cd";
  else if ( name == "darkmist_vortex"                     ) e = "OnDirectDamage_"    + RTV( tf, 6121, 0, 6908, 0, 7796 ) + "Haste_15%_20Dur_115Cd";
  else if ( name == "spirits_of_the_sun"                  ) e = "OnHeal_"            + RTV( tf, 6121, 0, 6908, 0, 7796 ) + "Spi_15%_20Dur_115Cd";
  else if ( name == "stuff_of_nightmares"                 ) e = "OnAttackHit_"       + RTV( tf, 6121, 0, 6908, 0, 7796 ) + "Dodge_15%_20Dur_115Cd";
  else if ( name == "light_of_the_cosmos"                 ) e = "OnSpellTickDamage_" + RTV( tf, 2866, 0, 3236, 0, 3653 ) + "Int_15%_20Dur_55Cd";
  else if ( name == "bottle_of_infinite_stars"            ) e = "OnAttackHit_"       + RTV( tf, 2866, 0, 3236, 0, 3653 ) + "Agi_15%_20Dur_55Cd";
  else if ( name == "vial_of_dragons_blood"               ) e = "OnAttackHit_"       + RTV( tf, 2866, 0, 3236, 0, 3653 ) + "Dodge_15%_20Dur_55Cd";
  else if ( name == "lei_shens_final_orders"              ) e = "OnAttackHit_"       + RTV( tf, 2866, 0, 3236, 0, 3653 ) + "Str_15%_20Dur_55Cd"; 
  else if ( name == "qinxis_polarizing_seal"              ) e = "OnHeal_"            + RTV( tf, 2866, 0, 3236, 0, 3653 ) + "Int_15%_20Dur_55Cd";

  else if ( name == "relic_of_yulon"                      ) e = "OnSpellDamage_3027Int_20%_15Dur_55Cd";
  else if ( name == "relic_of_chiji"                      ) e = "OnHealCast_3027Spi_20%_15Dur_55Cd";
  else if ( name == "relic_of_xuen" && item_id == 79327   ) e = "OnAttackHit_3027Str_20%_15Dur_55Cd";
  else if ( name == "relic_of_xuen" && item_id == 79328   ) e = "OnAttackCrit_3027Agi_20%_15Dur_55Cd";

  //MoP Tank Trinkets
  else if ( name == "iron_protector_talisman"             ) e = "OnAttackHit_3386Dodge_15%_15Dur_55Cd";

  // 5.4 Trinkets
  else if ( name == "discipline_of_xuen"           ) e = "OnAttackHit_" +   RTV( tf, 0, 0, 6914, 9943 ) + "Mastery_15%_20Dur_115Cd";
  else if ( name == "yulons_bite"                  ) e = "OnSpellDamage_" + RTV( tf, 0, 0, 6914, 9943 ) + "Crit_15%_20Dur_115Cd";
  else if ( name == "alacrity_of_xuen"             ) e = "OnAttackHit_" +   RTV( tf, 0, 0, 6914, 9943 ) + "Haste_15%_20Dur_115Cd";
  
  else if ( name == "ticking_ebon_detonator"       ) e = "OnDirectDamage_1RPPM_10Cd_10Dur_0.5Tick_20Stack_" + RTV( tf, 847, 947, 1069, 1131, 1207, 1276 )  + "Agi_Reverse_NoRefresh";
  else if ( name == "black_blood_of_yshaarj"       ) e = "OnHarmfulSpellHit_0.92RPPM_10Cd_10Dur_1.0Tick_10Stack_" + RTV( tf, 1862, 2082, 2350, 2485, 2652, 2805 ) + "Int_NoRefresh";
  else if ( name == "skeers_bloodsoaked_talisman"  ) e = "OnAttackHit_0.92RPPM_10Cd_10Dur_0.5Tick_20Stack_"    + RTV( tf, 931, 1041, 1175, 1242, 1326, 1402 ) + "Crit_NoRefresh";

  // 5.2 Trinkets
  else if ( name == "talisman_of_bloodlust"               ) e = "OnDirectDamage_"    + RTV( tf, 1277, 0, 1538, 1625, 1736, 1834 ) + "Haste_3.5RPPM_5Stack_10Dur_5Cd";
  else if ( name == "primordius_talisman_of_rage"         ) e = "OnDirectDamage_"    + RTV( tf, 1277, 0, 1538, 1625, 1736, 1834 ) + "Str_3.5RPPM_5Stack_10Dur_5Cd";
  else if ( name == "gaze_of_the_twins"                   ) e = "OnAttackCrit_"      + RTV( tf, 2381, 0, 2868, 3032, 3238, 3423 ) + "Crit_0.72RPPMAttackCrit_3Stack_20Dur_10Cd";
  else if ( name == "renatakis_soul_charm"                ) e = "OnDirectDamage_"    + RTV( tf, 1107, 0, 1333, 1410, 1505, 1592 ) + "Agi_1.21RPPM_10Stack_10Dur_1Tick_10Cd_NoRefresh";
  else if ( name == "wushoolays_final_choice"             ) e = "OnSpellDamage_"     + RTV( tf, 1107, 0, 1333, 1410, 1505, 1592 ) + "Int_1.21RPPM_10Stack_10Dur_1Tick_10Cd_NoRefresh";
  else if ( name == "fabled_feather_of_jikun"             ) e = "OnDirectDamage_"    + RTV( tf, 1107, 0, 1333, 1410, 1505, 1592 ) + "Str_1.21RPPM_10Stack_10Dur_1Tick_10Cd_NoRefresh";

  else if ( name == "breath_of_the_hydra"                 ) e = "OnSpellTickDamage_" + RTV( tf, 6088, 0, 7333, 7754, 8279, 8753 ) + "Int_1.1RPPM_10Dur_10Cd";
  else if ( name == "chayes_essence_of_brilliance"        ) e = "OnHarmfulSpellCrit_" + RTV( tf, 6088, 0, 7333, 7754, 8279, 8753 ) + "Int_0.85RPPMSpellCrit_10Dur_10Cd";

  else if ( name == "brutal_talisman_of_the_shadopan_assault" ) e = "OnDirectDamage_8800Str_15%_15Dur_85Cd";
  else if ( name == "vicious_talisman_of_the_shadopan_assault" ) e = "OnDirectDamage_8800Agi_15%_20Dur_115Cd";
  else if ( name == "volatile_talisman_of_the_shadopan_assault" ) e = "OnHarmfulSpellHit_8800Haste_15%_10Dur_55Cd";

  //MoP PvP Trinkets

  //540
  else if ( name == "prideful_gladiators_insignia_of_victory" ) e = "OnAttackHit_6125Str_15%_20Dur_55Cd";
  else if ( name == "prideful_gladiators_insignia_of_conquest" ) e = "OnAttackHit_6125Agi_15%_20Dur_55Cd";
  else if ( name == "prideful_gladiators_insignia_of_dominance" ) e = "OnSpellDamage_6125Int_15%_20Dur_55Cd";

  //522
  else if ( name == "grievous_gladiators_insignia_of_victory" ) e = "OnAttackHit_5179Str_15%_20Dur_55Cd";
  else if ( name == "grievous_gladiators_insignia_of_conquest" ) e = "OnAttackHit_5179Agi_15%_20Dur_55Cd";
  else if ( name == "grievous_gladiators_insignia_of_dominance" ) e = "OnSpellDamage_5179SP_15%_20Dur_55Cd";

  //483
  else if ( name == "malevolent_gladiators_insignia_of_victory" ) e = "OnAttackHit_3603Str_15%_20Dur_55Cd";
  else if ( name == "malevolent_gladiators_insignia_of_conquest" ) e = "OnAttackHit_3603Agi_15%_20Dur_55Cd";
  else if ( name == "malevolent_gladiators_insignia_of_dominance" ) e = "OnSpellDamage_3603SP_25%_20Dur_55Cd";
  //464
  else if ( name == "dreadful_gladiators_insignia_of_victory" ) e = "OnAttackHit_3017Str_15%_20Dur_55Cd";
  else if ( name == "dreadful_gladiators_insignia_of_conquest" ) e = "OnAttackHit_3017Agi_15%_20Dur_55Cd";
  else if ( name == "dreadful_gladiators_insignia_of_dominance" ) e = "OnSpellDamage_3017SP_25%_20Dur_55Cd";

  // Enchants
  if      ( name == "lightweave_1"                        ) e = "OnSpellCast_295SP_35%_15Dur_60Cd";
  else if ( name == "lightweave_embroidery_1"             ) e = "OnSpellCast_295SP_35%_15Dur_60Cd";
  else if ( name == "lightweave_2" ||
            name == "lightweave_embroidery_2"             ) e = "OnSpellDamageHeal_580Int_25%_15Dur_64Cd";
  else if ( name == "lightweave_3" ||
            name == "lightweave_embroidery_3"             ) e = "OnSpellDamageHeal_2000Int_25%_15Dur_57Cd";
  else if ( name == "darkglow_1"                          ) e = "OnSpellCast_250Spi_35%_15Dur_60Cd";
  else if ( name == "darkglow_embroidery_1"               ) e = "OnSpellCast_250Spi_35%_15Dur_60Cd";
  else if ( name == "darkglow_2"                          ) e = "OnSpellCast_580Spi_30%_15Dur_45Cd";
  else if ( name == "darkglow_embroidery_2"               ) e = "OnSpellCast_580Spi_30%_15Dur_45Cd";
  else if ( name == "darkglow_3"                          ) e = "OnSpellCast_3000Spi_25%_15Dur_57Cd";
  else if ( name == "darkglow_embroidery_3"               ) e = "OnSpellCast_3000Spi_25%_15Dur_57Cd";
  else if ( name == "swordguard_1"                        ) e = "OnAttackHit_400AP_20%_15Dur_60Cd";
  else if ( name == "swordguard_embroidery_1"             ) e = "OnAttackHit_400AP_20%_15Dur_60Cd";
  else if ( name == "swordguard_2"                        ) e = "OnAttackHit_1000AP_15%_15Dur_55Cd";
  else if ( name == "swordguard_embroidery_2"             ) e = "OnAttackHit_1000AP_15%_15Dur_55Cd";
  else if ( name == "swordguard_3"                        ) e = "OnAttackHit_4000AP_15%_15Dur_57Cd";
  else if ( name == "swordguard_embroidery_3"             ) e = "OnAttackHit_4000AP_15%_15Dur_57Cd";
  else if ( name == "flintlockes_woodchucker"             ) e = "OnAttackHit_1100Physical_300Agi_10%_10Dur_40Cd_nocrit"; // TO-DO: Confirm ICD.

  // DK Runeforges
  else if ( name == "rune_of_cinderglacier"               ) e = "custom";
  else if ( name == "rune_of_razorice"                    ) e = "custom";
  else if ( name == "rune_of_the_fallen_crusader"         ) e = "custom";

  if ( e.empty() ) return false;

  util::tolower( e );

  encoding = e;

  return true;
}

// ==========================================================================
// unique_gear::get_use_encoding
// ==========================================================================

bool unique_gear::get_use_encoding( std::string& encoding,
                                    const std::string& name,
                                    unsigned           tf,
                                    bool         /* ptr */,
                                    unsigned     item_id )
{
  std::string e;

  // Mists of Pandaria

  if ( name == "flashfrozen_resin_globule"    ) e = "4232Int_25Dur_150Cd";
  else if ( name == "flashing_steel_talisman"      ) e = "4232Agi_15Dur_90Cd";
  else if ( name == "vial_of_ichorous_blood"       ) e = "4241Spi_20Dur_120Cd";
  else if ( name == "lessons_of_the_darkmaster"    ) e = "4232Str_20Dur_120Cd";
  else if ( name == "daelos_final_words"           ) e = "5633Str_10Dur_90Cd";
  else if ( name == "gerps_perfect_arrow"          ) e = "3480Agi_20Dur_120Cd";
  else if ( name == "jade_bandit_figurine"         ) e = RTV( tf, 3184, 0, 3595, 0, 4059 ) + "Haste_15Dur_60Cd";
  else if ( name == "jade_charioteer_figurine"     ) e = RTV( tf, 3184, 0, 3595, 0, 4059 ) + "Haste_15Dur_60Cd";
  else if ( name == "jade_magistrate_figurine"     ) e = RTV( tf, 3184, 0, 3595, 0, 4059 ) + "Crit_15Dur_60Cd";
  else if ( name == "jade_courtesan_figurine"      ) e = RTV( tf, 3184, 0, 3595, 0, 4059 ) + "Spi_15Dur_60Cd";
  else if ( name == "jade_warlord_figurine"        ) e = RTV( tf, 3184, 0, 3595, 0, 4059 ) + "Mastery_15Dur_60Cd";
  else if ( name == "hawkmasters_talon"            ) e = "3595Haste_15Dur_60Cd";
  else if ( name == "laochins_liquid_courage"      ) e = "2822Mastery_15Dur_60Cd";
  else if ( name == "relic_of_niuzao"              ) e = "8871Dodge_12Dur_60Cd";
  else if ( name == "brawlers_statue"              ) e = "4576Dodge_20Dur_120Cd";
  else if ( name == "heart_of_fire"                ) e = "4232Dodge_20Dur_120Cd";
  else if ( name == "blossom_of_pure_snow"         ) e = "3595Crit_15Dur_60Cd";
  else if ( name == "iron_belly_wok"               ) e = "3595Haste_15Dur_60Cd";
  else if ( name == "shockcharger_medallion"       ) e = "3838Int_15Dur_60Cd";
  else if ( name == "helmbreaker_medallion"        ) e = "3838Crit_15Dur_60Cd";
  else if ( name == "arrowflight_medallion"        ) e = "3838Crit_15Dur_60Cd";
  else if ( name == "vaporshield_medallion"        ) e = "3838Mastery_15Dur_60Cd";
  else if ( name == "heartwarmer_medallion"        ) e = "3838Spirit_15Dur_60Cd";
  else if ( name == "cutstitcher_medallion"        ) e = "3838Spirit_15Dur_60Cd";
  else if ( name == "medallion_of_mystifying_vapors" ) e = "3838Mastery_15Dur_60Cd";
  else if ( name == "skullrender_medallion"        ) e = "3838Crit_15Dur_60Cd";
  else if ( name == "staticcasters_medallion"      ) e = "3838Int_15Dur_60Cd";
  else if ( name == "woundripper_medallion"        ) e = "3838Crit_15Dur_60Cd";
  else if ( name == "mending_badge_of_the_shieldwall"  ) e = "2693Spirit_15Dur_60Cd";
  else if ( name == "knightly_badge_of_the_shieldwall" ) e = "2693Mastery_15Dur_60Cd";
  else if ( name == "arcane_badge_of_the_shieldwall"   ) e = "2693Haste_15Dur_60Cd";
  else if ( name == "deadeye_badge_of_the_shieldwall"  ) e = "2693Mastery_15Dur_60Cd";
  else if ( name == "durable_badge_of_the_shieldwall"  ) e = "2693Mastery_15Dur_60Cd";
  else if ( name == "dominators_mending_badge"     ) e = "2693Spirit_15Dur_60Cd";
  else if ( name == "dominators_knightly_badge"    ) e = "2693Mastery_15Dur_60Cd";
  else if ( name == "dominators_arcane_badge"      ) e = "2693Haste_15Dur_60Cd";
  else if ( name == "dominators_deadeye_badge"     ) e = "2693Mastery_15Dur_60Cd";
  else if ( name == "dominators_durable_badge"     ) e = "2693Mastery_15Dur_60Cd";

  else if ( name == "resolve_of_niuzao"            ) e = RTV( tf, 0, 0, 5758, 8281 ) + "Dodge_20Dur_120Cd";
  else if ( name == "contemplation_of_chiji"       ) e = RTV( tf, 0, 0, 5758, 8281 ) + "Spi_15Dur_90Cd";

  else if ( ( name == "curse_of_hubris" || name == "hellscreams_hubris" ) ) e = RTV( tf, 7758, 8676, 9793, 10356, 11054, 11690 ) + "Crit_15Dur_90Cd";
  else if ( name == "steadfast_talisman_of_the_shadopan_assault" ) e = "10400Dodge_20Dur_120Cd"; //I am too stupid to do this right: Should actually be counting downwards from a 10 stack to a 0 stack (each 1600dodge). Instead we just take an average stack of 11/2 ->
  else if ( name == "fortitude_of_the_zandalari"   ) e = RTV( tf, 61308, 0, 73844, 78092, 83364, 88147 ) + "Maxhealth_15Dur_120CD";

  else if ( item_id == 102314                      ) e = "9793Spi_15Dur_90Cd";
  else if ( item_id == 102316                      ) e = "9793Dodge_20Dur_120Cd";

  // MoP PvP
  else if ( name == "dreadful_gladiators_badge_of_dominance"   ) e = "4275SP_20Dur_120Cd";
  else if ( name == "dreadful_gladiators_badge_of_victory"     ) e = "4275Str_20Dur_120Cd";
  else if ( name == "dreadful_gladiators_badge_of_conquest"    ) e = "4275Agi_20Dur_120Cd";
  else if ( name == "malevolent_gladiators_badge_of_dominance" ) e = "5105SP_20Dur_120Cd";
  else if ( name == "malevolent_gladiators_badge_of_victory"   ) e = "5105Str_20Dur_120Cd";
  else if ( name == "malevolent_gladiators_badge_of_conquest"  ) e = "5105Agi_20Dur_120Cd";
  //522
  else if ( name == "grievous_gladiators_badge_of_dominance" ) e = "3670Int_20Dur_60Cd";
  else if ( name == "grievous_gladiators_badge_of_victory"   ) e = "3670Str_20Dur_60Cd";
  else if ( name == "grievous_gladiators_badge_of_conquest"  ) e = "3670Agi_20Dur_60Cd";
  //540
  else if ( name == "prideful_gladiators_badge_of_dominance" ) e = "4340Int_20Dur_60Cd";
  else if ( name == "prideful_gladiators_badge_of_victory"   ) e = "4340Str_20Dur_60Cd";
  else if ( name == "prideful_gladiators_badge_of_conquest"  ) e = "4340Agi_20Dur_60Cd";

  // Engineering Tinkers
  else if ( name == "pyrorocket"                   ) e = "1165Fire_45Cd";  // temporary for backwards compatibility
  else if ( name == "hand_mounted_pyro_rocket"     ) e = "1165Fire_45Cd";
  else if ( name == "hyperspeed_accelerators"      ) e = "240Haste_12Dur_60Cd";
  else if ( name == "tazik_shocker"                ) e = "4800Nature_120Cd";
  else if ( name == "quickflip_deflection_plates"  ) e = "1500Armor_12Dur_60Cd";

  if ( e.empty() ) return false;

  util::tolower( e );

  encoding = e;

  return true;
}

// ==========================================================================
// Enchant
// ==========================================================================

void unique_gear::initialize_special_effects( player_t* p )
{
  if ( p -> is_pet() ) return;

  // Special Weapn Enchants
  std::string& mh_enchant     = p -> items[ SLOT_MAIN_HAND ].parsed.enchant.name_str;
  std::string& oh_enchant     = p -> items[ SLOT_OFF_HAND  ].parsed.enchant.name_str;

  weapon_t* mhw = &( p -> main_hand_weapon );
  weapon_t* ohw = &( p -> off_hand_weapon );

  windsong( p, mh_enchant, mhw, "" );
  windsong( p, oh_enchant, ohw, "_oh" );

  elemental_force( p, mh_enchant, oh_enchant, mhw, ohw );

  executioner( p, mh_enchant, oh_enchant, mhw, ohw );

  hurricane( p, mh_enchant, oh_enchant, mhw, ohw );

  berserking( p, mh_enchant, mhw, "" );
  berserking( p, oh_enchant, ohw, "_oh" );

  landslide( p, mh_enchant, mhw, "" );
  landslide( p, oh_enchant, ohw, "_oh" );

  dancing_steel( p, mh_enchant, mhw, "" );
  dancing_steel( p, oh_enchant, ohw, "_oh" );

  bloody_dancing_steel( p, mh_enchant, mhw, "" );
  bloody_dancing_steel( p, oh_enchant, ohw, "" ) ;

  rivers_song( p, mh_enchant, oh_enchant, mhw, ohw );

  colossus( p, mh_enchant, oh_enchant, mhw, ohw );

  mongoose( p, mh_enchant, mhw, "" );
  mongoose( p, oh_enchant, ohw, "_oh" );

  power_torrent( p, mh_enchant, "" );
  power_torrent( p, oh_enchant, "_oh" );

  jade_spirit( p, mh_enchant, oh_enchant );

  windwalk( p, mh_enchant, mhw, "" );
  windwalk( p, oh_enchant, ohw, "_oh" );

  gnomish_xray( p, mh_enchant, mhw );
  lord_blastingtons_scope_of_doom( p, mh_enchant, mhw );
  mirror_scope( p, mh_enchant, mhw );

  meta_gems::meta_gems( p, mhw, ohw );

  // Special Item Enchants
  for ( size_t i = 0; i < p -> items.size(); i++ )
  {
    item_t& item = p -> items[ i ];

    if ( item.parsed.enchant.stat && item.parsed.enchant.school )
    {
      unique_gear::register_stat_discharge_proc( item, item.parsed.enchant );
    }
    else if ( item.parsed.enchant.stat )
    {
      unique_gear::register_stat_proc( p, item.parsed.enchant );
    }
    else if ( item.parsed.enchant.school )
    {
      unique_gear::register_discharge_proc( item, item.parsed.enchant );
    }
    else if ( item.parsed.addon.name_str == "synapse_springs" )
    {
      synapse_springs( &item );
      item.parsed.addon.unique = true;
    }
    else if ( item.parsed.addon.name_str == "synapse_springs_2" )
    {
      synapse_springs_2( &item );
      item.parsed.addon.unique = true;
    }
    else if ( item.parsed.addon.name_str == "synapse_springs_mark_ii" )
    {
      synapse_springs_2( &item );
      item.parsed.addon.unique = true;
    }
    else if ( item.parsed.addon.name_str == "phase_fingers" )
    {
      phase_fingers( &item );
      item.parsed.addon.unique = true;
    }
    else if ( item.parsed.addon.name_str == "frag_belt" )
    {
      frag_belt( &item );
      item.parsed.addon.unique = true;
    }
  }
}

