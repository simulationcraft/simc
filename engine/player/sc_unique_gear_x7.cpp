// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace unique_gear;

/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace enchants
{
  void mark_of_the_claw( special_effect_t& );
  void mark_of_the_hidden_satyr( special_effect_t& );
  void mark_of_the_distant_army( special_effect_t& );
  void mark_of_the_heavy_hide( special_effect_t& );
  void mark_of_the_ancient_priestess( special_effect_t& );
}

namespace item
{
  // 7.0 Dungeon
  void caged_horror( special_effect_t& );
  void chaos_talisman( special_effect_t& );
  void corrupted_starlight( special_effect_t& );
  void darkmoon_deck( special_effect_t& );
  void elementium_bomb_squirrel( special_effect_t& );
  void figurehead_of_the_naglfar( special_effect_t& );
  void giant_ornamental_pearl( special_effect_t& );
  void horn_of_valor( special_effect_t& );
  void impact_tremor( special_effect_t& );
  void memento_of_angerboda( special_effect_t& );
  void obelisk_of_the_void( special_effect_t& );
  void shivermaws_jawbone( special_effect_t& );
  void spiked_counterweight( special_effect_t& );
  void tiny_oozeling_in_a_jar( special_effect_t& );
  void tirathons_betrayal( special_effect_t& );
  void windscar_whetstone( special_effect_t& );

  // 7.0 Raid
  void natures_call( special_effect_t& );
}

namespace set_bonus
{
  // 7.0 Dungeon
  void march_of_the_legion( special_effect_t& ); // NYI
  void journey_through_time( special_effect_t& ); // NYI

  // Generic passive stat aura adder for set bonuses
  void passive_stat_aura( special_effect_t& );
  // Simple callback creator for set bonuses
  void simple_callback( special_effect_t& );
}

// TODO: Ratings
void set_bonus::passive_stat_aura( special_effect_t& effect )
{
  const spell_data_t* spell = effect.player -> find_spell( effect.spell_id );
  stat_e stat = STAT_NONE;
  // Sanity check for stat-giving aura, either stats or aura type 465 ("bonus armor")
  if ( spell -> effectN( 1 ).subtype() != A_MOD_STAT || spell -> effectN( 1 ).subtype() == A_465 )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  if ( spell -> effectN( 1 ).subtype() == A_MOD_STAT )
  {
    if ( spell -> effectN( 1 ).misc_value1() >= 0 )
    {
      stat = static_cast< stat_e >( spell -> effectN( 1 ).misc_value1() + 1 );
    }
    else if ( spell -> effectN( 1 ).misc_value1() == -1 )
    {
      stat = STAT_ALL;
    }
  }
  else
  {
    stat = STAT_BONUS_ARMOR;
  }

  double amount = util::round( spell -> effectN( 1 ).average( effect.player, std::min( MAX_LEVEL, effect.player -> level() ) ) );

  effect.player -> initial.stats.add_stat( stat, amount );
}

void set_bonus::simple_callback( special_effect_t& effect )
{ new dbc_proc_callback_t( effect.player, effect ); }

void enchants::mark_of_the_hidden_satyr( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "mark_of_the_hidden_satyr" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "mark_of_the_hidden_satyr", effect );
  }

  if ( ! effect.execute_action )
  {
    action_t* a = new spell_t( "mark_of_the_hidden_satyr",
      effect.player, effect.player -> find_spell( 191259 ) );
    a -> background = a -> may_crit = true;
    a -> callbacks = false;
    a -> spell_power_mod.direct = 1.0; // Jun 27 2016
    a -> item = effect.item;

    effect.execute_action = a;
  }

  effect.proc_flags_ = PF_ALL_DAMAGE; // DBC says procs off heals. Let's not.

  new dbc_proc_callback_t( effect.item, effect );
}

struct random_buff_callback_t : public dbc_proc_callback_t
{
  std::vector<buff_t*> buffs;

  random_buff_callback_t( const special_effect_t& effect, std::vector<buff_t*> b ) :
    dbc_proc_callback_t( effect.item, effect ), buffs( b )
  {}

  void execute( action_t* /* a */, action_state_t* /* call_data */ ) override
  {
    int roll = ( int ) ( listener -> sim -> rng().real() * buffs.size() );
    buffs[ roll ] -> trigger();
  }
};

void enchants::mark_of_the_claw( special_effect_t& effect )
{
  effect.custom_buff = stat_buff_creator_t( effect.player, "mark_of_the_claw", effect.player -> find_spell( 190909 ), effect.item );

  new dbc_proc_callback_t( effect.item, effect );
}

struct gaseous_bubble_t : public absorb_buff_t
{
  struct gaseous_explosion_t : public spell_t
  {
    gaseous_explosion_t( special_effect_t& effect ) : 
      spell_t( "gaseous_explosion", effect.player, effect.player -> find_spell( 214972 ) )
    {
      background = may_crit = true;
      callbacks = false;
      aoe = -1;
      item = effect.item;

      base_dd_min = base_dd_max = effect.driver() -> effectN( 2 ).average( item );
    }
  };

  gaseous_explosion_t* explosion;

  gaseous_bubble_t( special_effect_t& effect ) : 
    absorb_buff_t( absorb_buff_creator_t( effect.player, "gaseous_bubble", effect.driver(), effect.item ) ),
    explosion( new gaseous_explosion_t( effect ) )
  {}

  void expire_override( int stacks, timespan_t remaining ) override
  {
    absorb_buff_t::expire_override( stacks, remaining );

    explosion -> schedule_execute();
  }
};

void item::giant_ornamental_pearl( special_effect_t& effect )
{
  effect.custom_buff = new gaseous_bubble_t( effect );
}

struct devilsaurs_stampede_t : public spell_t
{
  devilsaurs_stampede_t( special_effect_t& effect ) : 
    spell_t( "devilsaurs_stampede", effect.player, effect.player -> find_spell( 224061 ) )
  {
    background = may_crit = true;
    callbacks = false;
    aoe = -1;
    item = effect.item;

    base_dd_min = base_dd_max = data().effectN( 1 ).average( item );
  }
};

void item::impact_tremor( special_effect_t& effect )
{
  action_t* stampede = effect.player -> find_action( "devilsaurs_stampede" );
  if ( ! stampede )
  {
    stampede = effect.player -> create_proc_action( "devilsaurs_stampede", effect );
  }

  if ( ! stampede )
  {
    stampede = new devilsaurs_stampede_t( effect );
  }

  effect.custom_buff = buff_creator_t( effect.player, "devilsaurs_stampede", effect.driver() -> effectN( 1 ).trigger(), effect.item )
    .tick_zero( true )
    .tick_callback( [ stampede ]( buff_t*, int, const timespan_t& ) {
      stampede -> schedule_execute();
    } );

  new dbc_proc_callback_t( effect.item, effect );
}

void item::memento_of_angerboda( special_effect_t& effect )
{
  double rating_amount = effect.driver() -> effectN( 2 ).average( effect.item );
  rating_amount *= effect.item -> sim -> dbc.combat_rating_multiplier( effect.item -> item_level() );

  std::vector<buff_t*> buffs;

  buffs.push_back( stat_buff_creator_t( effect.player, "howl_of_ingvar", effect.player -> find_spell( 214802 ) )
      .activated( false )
      .add_stat( STAT_CRIT_RATING, rating_amount ) );
  buffs.push_back( stat_buff_creator_t( effect.player, "wail_of_svala", effect.player -> find_spell( 214803 ) )
     .activated( false )
      .add_stat( STAT_HASTE_RATING, rating_amount ) );
  buffs.push_back( stat_buff_creator_t( effect.player, "dirge_of_angerboda", effect.player -> find_spell( 214807 ) )
      .activated( false )
      .add_stat( STAT_MASTERY_RATING, rating_amount ) );

  new random_buff_callback_t( effect, buffs );
}

void item::obelisk_of_the_void( special_effect_t& effect )
{
  effect.custom_buff = stat_buff_creator_t( effect.player, "collapsing_shadow", effect.player -> find_spell( 215476 ), effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_AGI_INT ), effect.driver() -> effectN( 2 ).average( effect.item ) );
}

struct ice_bomb_t : public spell_t
{
  buff_t* buff;

  ice_bomb_t( special_effect_t& effect ) : 
    spell_t( "ice_bomb", effect.player, effect.driver() )
  {
    background = may_crit = true;
    callbacks = false;
    aoe = -1;
    item = effect.item;

    base_dd_min = base_dd_max = data().effectN( 1 ).average( item );
    buff = stat_buff_creator_t( effect.player, "frigid_armor", effect.player -> find_spell( 214589 ), effect.item );
  }

  void execute() override
  {
    spell_t::execute();

    buff -> trigger( num_targets_hit );
  }
};

void item::shivermaws_jawbone( special_effect_t& effect )
{
  effect.execute_action = new ice_bomb_t( effect );
}

// Spiked Counterweight =====================================================

struct haymaker_damage_t : public spell_t
{
  haymaker_damage_t( const special_effect_t& effect ) :
    spell_t( "brutal_haymaker_vulnerability", effect.player, effect.driver() -> effectN( 2 ).trigger() )
  {
    background = true;
    callbacks = may_crit = may_miss = false;
    dot_duration = timespan_t::zero();
  }

  void init() override
  {
    spell_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct haymaker_driver_t : public dbc_proc_callback_t
{
  struct haymaker_event_t;
  
  debuff_t* debuff;
  const special_effect_t& effect;
  double multiplier;
  haymaker_event_t* accumulator;
  haymaker_damage_t* action;

  struct haymaker_event_t : public event_t
  {
    haymaker_damage_t* action;
    haymaker_driver_t* callback;
    debuff_t* debuff;
    double damage;

    haymaker_event_t( haymaker_driver_t* cb, haymaker_damage_t* a, debuff_t* d ) :
      event_t( *a -> player ), action( a ), debuff( d ),
      damage( 0 ), callback( cb )
    {
      add_event( a -> data().effectN( 1 ).period() );
    }

    const char* name() const override
    { return "brutal_haymaker_damage"; }

    void execute() override
    {
      if ( damage > 0 )
      {
        action -> target = debuff -> player;
        damage = std::min( damage, debuff -> current_value );
        action -> base_dd_min = action -> base_dd_max = damage;
        action -> schedule_execute();
        debuff -> current_value -= damage;
      }
      
      if ( debuff -> current_value > 0 && debuff -> check() )
        callback -> accumulator = new ( *action -> player -> sim ) haymaker_event_t( callback, action, debuff );
      else
      {
        callback -> accumulator = nullptr;
        debuff -> expire();
      }
    }
  };

  haymaker_driver_t( const special_effect_t& e, double m, debuff_t* d ) :
    dbc_proc_callback_t( e.player, e ), debuff( d ), effect( e ), multiplier( m ),
    action( debug_cast<haymaker_damage_t*>( e.player -> find_action( "brutal_haymaker_vulnerability" ) ) )
  {}

  void activate() override
  {
    dbc_proc_callback_t::activate();

    accumulator = new ( *effect.player -> sim ) haymaker_event_t( this, action, debuff );
  }

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    if ( trigger_state -> result_amount <= 0 )
      return;

    actor_target_data_t* td = effect.player -> get_target_data( trigger_state -> target );
    
    if ( td && td -> debuff.brutal_haymaker -> check() )
      accumulator -> damage += trigger_state -> result_amount * multiplier;
  }
};

struct spiked_counterweight_constructor_t : public item_targetdata_initializer_t
{
  struct haymaker_debuff_t : public debuff_t
  {
    haymaker_driver_t* callback;

    haymaker_debuff_t( const special_effect_t& effect, actor_target_data_t& td ) :
      debuff_t( buff_creator_t( td, "brutal_haymaker", effect.driver() -> effectN( 1 ).trigger() ) 
      .default_value( effect.driver() -> effectN( 1 ).trigger() -> effectN( 3 ).average( effect.item ) ) )
    {
      // Damage transfer effect & callback. We'll enable this callback whenever the debuff is active.
      special_effect_t* effect2 = new special_effect_t( effect.player );
      effect2 -> name_str = "brutal_haymaker_accumulator";
      effect2 -> proc_chance_ = 1.0;
      effect2 -> proc_flags_ = PF_ALL_DAMAGE;
      effect2 -> proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
      effect.player -> special_effects.push_back( effect2 );

      callback = new haymaker_driver_t( *effect2, data().effectN( 2 ).percent(), this );
      callback -> initialize();
    }

    void start( int stacks, double value, timespan_t duration ) override
    {
      debuff_t::start( stacks, value, duration );

      callback -> activate();
    }

    void expire_override( int stacks, timespan_t remaining ) override
    {
      debuff_t::expire_override( stacks, remaining );
    
      callback -> deactivate();
    }

    void reset() override
    {
      debuff_t::reset();

      callback -> deactivate();
    }
  };

  spiked_counterweight_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( effect == 0 )
    {
      td -> debuff.brutal_haymaker = buff_creator_t( *td, "brutal_haymaker" );
    }
    else
    {
      assert( ! td -> debuff.brutal_haymaker );

      td -> debuff.brutal_haymaker = new haymaker_debuff_t( *effect, *td );
      td -> debuff.brutal_haymaker -> reset();
    }
  }
};

struct brutal_haymaker_initial_t : public spell_t
{
  brutal_haymaker_initial_t( special_effect_t& effect ) :
    spell_t( "brutal_haymaker", effect.player, effect.driver() -> effectN( 1 ).trigger() )
  {
    background = may_crit = true;
    callbacks = false;
    item = effect.item;

    base_dd_min = base_dd_min = data().effectN( 1 ).average( item );
  }

  void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      player -> get_target_data( s -> target ) -> debuff.brutal_haymaker -> trigger();
    }
  }
};

void item::spiked_counterweight( special_effect_t& effect )
{
  effect.execute_action = new brutal_haymaker_initial_t( effect );
  effect.execute_action -> add_child( new haymaker_damage_t( effect ) );

  new dbc_proc_callback_t( effect.item, effect );
}

// Windscar Whetstone =======================================================

struct slicing_maelstrom_t : public spell_t
{
  slicing_maelstrom_t( special_effect_t& effect ) : 
    spell_t( "slicing_maelstrom", effect.player, effect.driver() -> effectN( 1 ).trigger() )
  {
    background = may_crit = true;
    callbacks = false;
    aoe = -1;
    item = effect.item;
    cooldown -> duration = timespan_t::zero(); // damage spell has erroneous cooldown

    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( item );
  }
};

void item::windscar_whetstone( special_effect_t& effect )
{
  action_t* maelstrom = effect.player -> find_action( "slicing_maelstrom" );
  if ( ! maelstrom )
  {
    maelstrom = effect.player -> create_proc_action( "slicing_maelstrom", effect );
  }

  if ( ! maelstrom )
  {
    maelstrom = new slicing_maelstrom_t( effect );
  }

  effect.custom_buff = buff_creator_t( effect.player, "slicing_maelstrom", effect.driver(), effect.item )
    .tick_zero( true )
    .tick_callback( [ maelstrom ]( buff_t*, int, const timespan_t& ) {
      maelstrom -> schedule_execute();
    } );
}

// Tirathon's Betrayal ======================================================

struct darkstrikes_absorb_t : public absorb_t
{
  darkstrikes_absorb_t( const special_effect_t& effect ) :
    absorb_t( "darkstrikes_absorb", effect.player, effect.player -> find_spell( 215659 ) )
  {
    background = true;
    callbacks = false;
    item = effect.item;
    target = effect.player;

    base_dd_min = base_dd_max = data().effectN( 2 ).average( item );
  }
};

struct darkstrikes_t : public spell_t
{
  action_t* absorb;

  darkstrikes_t( const special_effect_t& effect ) :
    spell_t( "darkstrikes", effect.player, effect.player -> find_spell( 215659 ) )
  {
    background = may_crit = true;
    callbacks = false;
    item = effect.item;

    base_dd_min = base_dd_max = data().effectN( 1 ).average( item );
  }

  void init() override
  {
    spell_t::init();

    absorb = player -> find_action( "darkstrikes_absorb" );
  }

  void execute() override
  {
    spell_t::execute();

    absorb -> schedule_execute();
  }
};

struct darkstrikes_driver_t : public dbc_proc_callback_t
{
  action_t* damage;

  darkstrikes_driver_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect ),
    damage( effect.player -> find_action( "darkstrikes" ) )
  { }

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    damage -> target = trigger_state -> target;
    damage -> schedule_execute();
  }
};

struct darkstrikes_buff_t : public buff_t
{
  darkstrikes_driver_t* dmg_callback;
  special_effect_t* dmg_effect;

  darkstrikes_buff_t( const special_effect_t& effect ) :
    buff_t( buff_creator_t( effect.player, "darkstrikes", effect.driver(), effect.item ).activated( false ) )
  {
    // Special effect to drive the AOE damage callback
    dmg_effect = new special_effect_t( effect.player );
    dmg_effect -> name_str = "darkstrikes_driver";
    dmg_effect -> ppm_ = -effect.driver() -> real_ppm();
    dmg_effect -> rppm_scale_ = RPPM_HASTE;
    dmg_effect -> proc_flags_ = PF_MELEE | PF_RANGED | PF_MELEE_ABILITY | PF_RANGED_ABILITY;
    dmg_effect -> proc_flags2_ = PF2_ALL_HIT;
    effect.player -> special_effects.push_back( dmg_effect );

    // And create, initialized and deactivate the callback
    dmg_callback = new darkstrikes_driver_t( *dmg_effect );
    dmg_callback -> initialize();
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    dmg_callback -> activate();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    dmg_callback -> deactivate();
  }

  void reset() override
  {
    buff_t::reset();

    dmg_callback -> deactivate();
  }
};

void item::tirathons_betrayal( special_effect_t& effect )
{
  action_t* darkstrikes = effect.player -> find_action( "darkstrikes" );
  if ( ! darkstrikes )
  {
    darkstrikes = effect.player -> create_proc_action( "darkstrikes", effect );
  }

  if ( ! darkstrikes )
  {
    darkstrikes = new darkstrikes_t( effect );
  }

  action_t* absorb = effect.player -> find_action( "darkstrikes_absorb" );
  if ( ! absorb )
  {
    absorb = effect.player -> create_proc_action( "darkstrikes_absorb", effect );
  }

  if ( ! absorb )
  {
    absorb = new darkstrikes_absorb_t( effect );
  }

  effect.custom_buff = new darkstrikes_buff_t( effect );
}

// Horn of Valor ============================================================

void item::horn_of_valor( special_effect_t& effect )
{
  effect.custom_buff = stat_buff_creator_t( effect.player, "valarjars_path", effect.driver(), effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver() -> effectN( 1 ).average( effect.item ) );
}

// Tiny Oozeling in a Jar ===================================================

struct fetid_regurgitation_t : public spell_t
{
  buff_t* driver_buff;

  fetid_regurgitation_t( special_effect_t& effect, buff_t* b ) :
    spell_t( "fetid_regurgitation", effect.player, effect.driver() -> effectN( 1 ).trigger() ),
    driver_buff( b )
  {
    background = may_crit = true;
    callbacks = false;
    aoe = -1;
    item = effect.item;

    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( item );
  }

  double action_multiplier() const override
  {
    double am = spell_t::action_multiplier();

    am *= driver_buff -> value();

    return am;
  }
};

// TOCHECK: Does this hit 6 or 7 times? Tooltip suggests 6 but it the buff lasts long enough for 7 ticks.

void item::tiny_oozeling_in_a_jar( special_effect_t& effect )
{
  struct congealing_goo_callback_t : public dbc_proc_callback_t
  {
    buff_t* goo;

    congealing_goo_callback_t( const special_effect_t* effect, buff_t* cg ) :
      dbc_proc_callback_t( effect -> item, *effect ), goo( cg )
    {}

    void execute( action_t* /* action */, action_state_t* /* state */ ) override
    {
      goo -> trigger();
    }
  };

  buff_t* charges = buff_creator_t( effect.player, "congealing_goo", effect.player -> find_spell( 215126 ), effect.item );

  special_effect_t* goo_effect = new special_effect_t( effect.player );
  goo_effect -> item = effect.item;
  goo_effect -> spell_id = 215120;
  goo_effect -> proc_flags_ = PROC1_MELEE | PROC1_MELEE_ABILITY;
  goo_effect -> proc_flags2_ = PF2_ALL_HIT;
  effect.player -> special_effects.push_back( goo_effect );

  new congealing_goo_callback_t( goo_effect, charges );

  struct fetid_regurgitation_buff_t : public buff_t
  {
    buff_t* congealing_goo;

    fetid_regurgitation_buff_t( special_effect_t& effect, action_t* ds, buff_t* cg ) : 
      buff_t( buff_creator_t( effect.player, "fetid_regurgitation", effect.driver(), effect.item )
        .activated( false )
        .tick_zero( true )
        .tick_callback( [ ds ] ( buff_t*, int, const timespan_t& ) {
          ds -> schedule_execute();
        } ) ), congealing_goo( cg )
    {}

    bool trigger( int stack, double, double chance, timespan_t duration ) override
    {
      bool s = buff_t::trigger( stack, congealing_goo -> stack(), chance, duration );

      congealing_goo -> expire();

      return s;
    }
  };

  action_t* fetid_regurgitation = effect.player -> find_action( "fetid_regurgitation" );
  if ( ! fetid_regurgitation )
  {
    fetid_regurgitation = effect.player -> create_proc_action( "fetid_regurgitation", effect );
  }

  if ( ! fetid_regurgitation )
  {
    fetid_regurgitation = new fetid_regurgitation_t( effect, charges );
  }

  effect.custom_buff = new fetid_regurgitation_buff_t( effect, fetid_regurgitation, charges );
};

// Figurehead of the Naglfar ================================================

struct taint_of_the_sea_t : public spell_t
{
  taint_of_the_sea_t( const special_effect_t& effect ) :
    spell_t( "taint_of_the_sea", effect.player, effect.player -> find_spell( 215695 ) )
  {
    background = true;
    callbacks = may_crit = false;
    item = effect.item;

    base_multiplier = effect.driver() -> effectN( 1 ).percent();
  }

  void init() override
  {
    spell_t::init();

    snapshot_flags = STATE_MUL_DA;
    update_flags = 0;
  }

  double composite_da_multiplier( const action_state_t* ) const override
  { return base_multiplier; }

  void execute() override
  {
    buff_t* d = player -> get_target_data( target ) -> debuff.taint_of_the_sea;
    
    assert( d && d -> check() );

    base_dd_min = base_dd_max = std::min( base_dd_min, d -> current_value / base_multiplier );

    spell_t::execute();

    d -> current_value -= execute_state -> result_amount;

    // can't assert on any negative number because precision reasons
    assert( d -> current_value >= -1.0 );

    if ( d -> current_value <= 0.0 )
      d -> expire();
  }
};

struct taint_of_the_sea_driver_t : public dbc_proc_callback_t
{
  action_t* damage;
  player_t* player, *active_target;

  taint_of_the_sea_driver_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect ),
    damage( effect.player -> find_action( "taint_of_the_sea" ) ),
    player( effect.player ), active_target( nullptr )
  {}

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    assert( active_target );

    if ( trigger_state -> target == active_target )
      return;
    if ( trigger_state -> result_amount <= 0 )
      return;

    assert( player -> get_target_data( active_target ) -> debuff.taint_of_the_sea -> check() );

    damage -> target = active_target;
    damage -> base_dd_min = damage -> base_dd_max = trigger_state -> result_amount;
    damage -> execute();
  }
};

struct taint_of_the_sea_debuff_t : public debuff_t
{
  taint_of_the_sea_driver_t* callback;

  taint_of_the_sea_debuff_t( player_t* target, const special_effect_t* effect ) : 
    debuff_t( buff_creator_t( actor_pair_t( target, effect -> player ), "taint_of_the_sea", effect -> driver() )
      .default_value( effect -> driver() -> effectN( 2 ).trigger() -> effectN( 2 ).average( effect -> item ) ) )
  {
    // Damage transfer effect & callback. We'll enable this callback whenever the debuff is active.
    special_effect_t* effect2 = new special_effect_t( effect -> player );
    effect2 -> name_str = "taint_of_the_sea_driver";
    effect2 -> proc_chance_ = 1.0;
    effect2 -> proc_flags_ = PF_ALL_DAMAGE;
    effect2 -> proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
    effect -> player -> special_effects.push_back( effect2 );

    callback = new taint_of_the_sea_driver_t( *effect2 );
    callback -> initialize();
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    debuff_t::start( stacks, value, duration );

    callback -> active_target = player;
    callback -> activate();
  }

  void expire_override( int stacks, timespan_t remaining ) override
  {
    debuff_t::expire_override( stacks, remaining );
    
    callback -> active_target = nullptr;
    callback -> deactivate();
  }

  void reset() override
  {
    debuff_t::reset();

    callback -> active_target = nullptr;
    callback -> deactivate();
  }
};

struct figurehead_of_the_naglfar_constructor_t : public item_targetdata_initializer_t
{
  figurehead_of_the_naglfar_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( effect == 0 )
    {
      td -> debuff.taint_of_the_sea = buff_creator_t( *td, "taint_of_the_sea" );
    }
    else
    {
      assert( ! td -> debuff.taint_of_the_sea );

      td -> debuff.taint_of_the_sea = new taint_of_the_sea_debuff_t( td -> target, effect );
      td -> debuff.taint_of_the_sea -> reset();
    }
  }
};

void item::figurehead_of_the_naglfar( special_effect_t& effect )
{
  action_t* damage_spell = effect.player -> find_action( "taint_of_the_sea" );
  if ( ! damage_spell )
  {
    damage_spell = effect.player -> create_proc_action( "taint_of_the_sea", effect );
  }

  if ( ! damage_spell )
  {
    damage_spell = new taint_of_the_sea_t( effect );
  }

  struct apply_debuff_t : public spell_t
  {
    apply_debuff_t( special_effect_t& effect ) :
      spell_t( "apply_taint_of_the_sea", effect.player, spell_data_t::nil() )
    {
      background = quiet = true;
      callbacks = false;
    }

    void execute() override
    {
      spell_t::execute();

      player -> get_target_data( target ) -> debuff.taint_of_the_sea -> trigger();
    }
  };

  effect.execute_action = new apply_debuff_t( effect );
}

// Chaos Talisman ===========================================================

void item::chaos_talisman( special_effect_t& effect )
{
  // TODO: Stack decay
  effect.proc_flags_ = PF_MELEE;
  effect.proc_flags2_ = PF_ALL_DAMAGE;
  const spell_data_t* buff_spell = effect.driver() -> effectN( 1 ).trigger();
  effect.custom_buff = stat_buff_creator_t( effect.player, "chaotic_energy", buff_spell, effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI ), buff_spell -> effectN( 1 ).average( effect.item ) );

  new dbc_proc_callback_t( effect.item, effect );
}

// Caged Horror =============================================================

struct dark_blast_t : public spell_t
{
  dark_blast_t( special_effect_t& effect ) : 
    spell_t( "dark_blast", effect.player, effect.player -> find_spell( 215407 ) )
  {
    background = may_crit = true;
    callbacks = false;
    item = effect.item;
    aoe = -1;

    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( item );
  }
};

void item::caged_horror( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "dark_blast" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "dark_blast", effect );
  }

  if ( ! effect.execute_action )
  {
    effect.execute_action = new dark_blast_t( effect );
  }

  new dbc_proc_callback_t( effect.item, effect );
}

// Corrupted Starlight ======================================================

struct nightfall_t : public spell_t
{
  spell_t* damage_spell;

  nightfall_t( special_effect_t& effect ) : 
    spell_t( "nightfall", effect.player, effect.player -> find_spell( 213785 ) )
  {
    background = true;
    callbacks = false;
    item = effect.item;

    const spell_data_t* tick_spell = effect.player -> find_spell( 213786 );
    spell_t* t = new spell_t( "nightfall_tick", effect.player, tick_spell );
    t -> aoe = -1;
    t -> background = t -> dual = t -> ground_aoe = t -> may_crit = true;
    t -> callbacks = false;
    t -> item = effect.item;
    t -> base_dd_min = t -> base_dd_max = tick_spell -> effectN( 1 ).average( item );
    t -> stats = stats;
    damage_spell = t;
  }

  void execute() override
  {
    spell_t::execute();

    new ( *sim ) ground_aoe_event_t( player, ground_aoe_params_t()
      .target( execute_state -> target )
      .x( execute_state -> target -> x_position )
      .y( execute_state -> target -> y_position )
      .duration( data().duration() )
      .start_time( sim -> current_time() )
      .action( damage_spell )
      .pulse_time( data().duration() / 10 ) );
  }
};

void item::corrupted_starlight( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "nightfall" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "nightfall", effect );
  }

  if ( ! effect.execute_action )
  {
    effect.execute_action = new nightfall_t( effect );
  }

  new dbc_proc_callback_t( effect.item, effect );
}

// Darkmoon Decks ===========================================================

struct darkmoon_deck_t
{
  player_t* player;
  std::array<stat_buff_t*, 8> cards;
  buff_t* top_card;
  timespan_t shuffle_period;

  darkmoon_deck_t( special_effect_t& effect, std::array<unsigned, 8> c ) : 
    player( effect.player ), top_card( nullptr ),
    shuffle_period( effect.driver() -> effectN( 1 ).period() )
  {
    for ( unsigned i = 0; i < 8; i++ )
    {
      const spell_data_t* s = player -> find_spell( c[ i ] );
      assert( s -> found() );

      std::string n = s -> name_cstr();
      util::tokenize( n );

      cards[ i ] = stat_buff_creator_t( player, n, s, effect.item );
    }
  }

  void shuffle()
  {
    if ( top_card )
      top_card -> expire();

    top_card = cards[ ( int ) player -> rng().range( 0, 8 ) ];

    top_card -> trigger();
  }
};

struct shuffle_event_t : public event_t
{
  darkmoon_deck_t* deck;

  shuffle_event_t( darkmoon_deck_t* d, bool initial = false ) : 
    event_t( *d -> player ), deck( d )
  {
    /* Shuffle when we schedule an event instead of when it executes.
    This will assure the deck starts shuffled */
    deck -> shuffle();

    if ( initial )
    {
      add_event( deck -> shuffle_period * rng().real() );
    }
    else
    {
      add_event( deck -> shuffle_period );
    }
  }
  
  const char* name() const override
  { return "shuffle_event"; }

  void execute() override
  {
    new ( sim() ) shuffle_event_t( deck );
  }
};

// TODO: The sim could use an "arise" and "demise" callback, it's kinda wasteful to call these
// things per every player actor shifting in the non-sleeping list. Another option would be to make
// (yet another list) that holds active, non-pet players.
// TODO: Also, the darkmoon_deck_t objects are not cleaned up at the end
void item::darkmoon_deck( special_effect_t& effect )
{
  std::array<unsigned, 8> cards;
  switch( effect.spell_id )
  {
  case 191632: // Immortality
    cards = { { 191624, 191625, 191626, 191627, 191628, 191629, 191630, 191631 } };
    break;
  case 191611: // Hellfire
    cards = { { 191603, 191604, 191605, 191606, 191607, 191608, 191609, 191610 } };
    break;
  case 191563: // Dominion
    cards = { { 191545, 191548, 191549, 191550, 191551, 191552, 191553, 191554 } };
    break;
  default:
    assert( false );
  }

  darkmoon_deck_t* d = new darkmoon_deck_t( effect, cards );

  effect.player -> sim -> player_non_sleeping_list.register_callback([ d, &effect ]( player_t* player ) {
    // Arise time gets set to timespan_t::min() in demise, before the actor is removed from the 
    // non-sleeping list. In arise, the arise_time is set to current time before the actor is added
    // to the non-sleeping list.
    if ( player != effect.player || player -> arise_time < timespan_t::zero() )
    {
      return;
    }

    new ( *effect.player -> sim ) shuffle_event_t( d, true );
  });
}

// Elementium Bomb Squirrel =================================================

struct aw_nuts_t : public spell_t
{
  aw_nuts_t( special_effect_t& effect ) : 
    spell_t( "aw_nuts", effect.player, effect.player -> find_spell( 216099 ) )
  {
    background = may_crit = true;
    callbacks = false;
    aoe = -1;
    item = effect.item;
    travel_speed = 7.0; // "Charge"!

    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( item );
  }
   
  void init() override
  {
    spell_t::init();

    snapshot_flags &= ~( STATE_MUL_DA | STATE_MUL_PERSISTENT | STATE_TGT_MUL_DA );
  }
};

void item::elementium_bomb_squirrel( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "aw_nuts" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "aw_nuts", effect );
  }

  if ( ! effect.execute_action )
  {
    effect.execute_action = new aw_nuts_t( effect );
  }

  new dbc_proc_callback_t( effect.item, effect );
}

// Nature's Call ============================================================

struct natures_call_callback_t : public dbc_proc_callback_t
{
  std::vector<stat_buff_t*> buffs;
  action_t* breath;

  natures_call_callback_t( const special_effect_t& effect, std::vector<stat_buff_t*> b, action_t* a ) :
    dbc_proc_callback_t( effect.item, effect ), buffs( b ), breath( a )
  {}

  void execute( action_t*, action_state_t* trigger_state ) override
  {
    int roll = ( int ) ( listener -> sim -> rng().real() * ( buffs.size() + 1 ) );
    switch ( roll )
    {
      case 3:
        breath -> target = trigger_state -> target;
        breath -> schedule_execute();
        break;
      default:
        buffs[ roll ] -> trigger();
        break;
    }
  }
};

void item::natures_call( special_effect_t& effect )
{
  double rating_amount = effect.driver() -> effectN( 2 ).average( effect.item );
  rating_amount *= effect.item -> sim -> dbc.combat_rating_multiplier( effect.item -> item_level() );

  std::vector<stat_buff_t*> buffs;
  buffs.push_back( stat_buff_creator_t( effect.player, "cleansed_ancients_blessing", effect.player -> find_spell( 222517 ) )
      .activated( false )
      .add_stat( STAT_CRIT_RATING, rating_amount ) );
  buffs.push_back( stat_buff_creator_t( effect.player, "cleansed_sisters_blessing", effect.player -> find_spell( 222519 ) )
      .activated( false )
      .add_stat( STAT_HASTE_RATING, rating_amount ) );
  buffs.push_back( stat_buff_creator_t( effect.player, "cleansed_wisps_blessing", effect.player -> find_spell( 222518 ) )
      .activated( false )
      .add_stat( STAT_MASTERY_RATING, rating_amount ) );

  action_t* breath = effect.player -> find_action( "cleansed_drakes_breath" );

  if ( ! breath )
  {
    breath = effect.player -> create_proc_action( "cleansed_drakes_breath", effect );
  }

  if ( ! breath )
  {
    breath = new spell_t( "cleansed_drakes_breath", effect.player, effect.player -> find_spell( 222520 ) );
    breath -> callbacks = false;
    breath -> background = breath -> may_crit = true;
    breath -> aoe = -1;
    breath -> base_dd_min = breath -> base_dd_max =
      effect.driver() -> effectN( 1 ).average( effect.item );
  }

  new natures_call_callback_t( effect, buffs, breath );
}

// March of the Legion ======================================================

void set_bonus::march_of_the_legion( special_effect_t& effect ) {}

// Journey Through Time =====================================================

void set_bonus::journey_through_time( special_effect_t& effect ) {}

void unique_gear::register_special_effects_x7()
{
  /* Legion 7.0 Dungeon */
  register_special_effect( 215444, item::caged_horror                   );
  register_special_effect( 214829, item::chaos_talisman                 );
  register_special_effect( 213782, item::corrupted_starlight            );
  register_special_effect( 191563, item::darkmoon_deck                  );
  register_special_effect( 191611, item::darkmoon_deck                  );
  register_special_effect( 191632, item::darkmoon_deck                  );
  register_special_effect( 215670, item::figurehead_of_the_naglfar      );
  register_special_effect( 214971, item::giant_ornamental_pearl         );
  register_special_effect( 215956, item::horn_of_valor                  );
  register_special_effect( 224059, item::impact_tremor                  );
  register_special_effect( 214798, item::memento_of_angerboda           );
  register_special_effect( 215467, item::obelisk_of_the_void            );
  register_special_effect( 214584, item::shivermaws_jawbone             );
  register_special_effect( 214168, item::spiked_counterweight           );
  register_special_effect( 215127, item::tiny_oozeling_in_a_jar         );
  register_special_effect( 215658, item::tirathons_betrayal             );
  register_special_effect( 214980, item::windscar_whetstone             );
  register_special_effect( 216085, item::elementium_bomb_squirrel       );

  /* Legion 7.0 Raid */
  register_special_effect( 222512, item::natures_call );

  /* Legion Enchants */
  register_special_effect( 190888, "190909trigger" );
  register_special_effect( 190889, "191380trigger" );
  register_special_effect( 190890, enchants::mark_of_the_hidden_satyr );
  register_special_effect( 228398, "228399trigger" );
  register_special_effect( 228400, "228401trigger" );

  /* T19 Generic Order Hall set bonuses */
  register_special_effect( 221533, set_bonus::passive_stat_aura     );
  register_special_effect( 221534, set_bonus::passive_stat_aura     );
  register_special_effect( 221535, set_bonus::passive_stat_aura     );

  /* 7.0 Dungeon 2 Set Bonuses */
  register_special_effect( 228445, set_bonus::march_of_the_legion );
  register_special_effect( 228447, set_bonus::journey_through_time );
  register_special_effect( 224146, set_bonus::simple_callback );
  register_special_effect( 224148, set_bonus::simple_callback );
  register_special_effect( 224150, set_bonus::simple_callback );
  register_special_effect( 228448, set_bonus::simple_callback );
}

void unique_gear::register_target_data_initializers_x7( sim_t* sim )
{
  std::vector< slot_e > trinkets;
  trinkets.push_back( SLOT_TRINKET_1 );
  trinkets.push_back( SLOT_TRINKET_2 );

  sim -> register_target_data_initializer( spiked_counterweight_constructor_t( 136715, trinkets ) );
  sim -> register_target_data_initializer( figurehead_of_the_naglfar_constructor_t( 137329, trinkets ) );
}

