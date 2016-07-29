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
  void mark_of_the_hidden_satyr( special_effect_t& );
  void mark_of_the_ancient_priestess( special_effect_t& ); // NYI
}

namespace item
{
  // 7.0 Dungeon
  void chaos_talisman( special_effect_t& );
  void corrupted_starlight( special_effect_t& );
  void elementium_bomb_squirrel( special_effect_t& );
  void faulty_countermeasures( special_effect_t& );
  void figurehead_of_the_naglfar( special_effect_t& );
  void giant_ornamental_pearl( special_effect_t& );
  void horn_of_valor( special_effect_t& );
  void impact_tremor( special_effect_t& );
  void memento_of_angerboda( special_effect_t& );
  void moonlit_prism( special_effect_t& );
  void obelisk_of_the_void( special_effect_t& );
  void portable_manacracker( special_effect_t& );
  void shivermaws_jawbone( special_effect_t& );
  void spiked_counterweight( special_effect_t& );
  void stormsinger_fulmination_charge( special_effect_t& );
  void terrorbound_nexus( special_effect_t& ); // NYI
  void tiny_oozeling_in_a_jar( special_effect_t& );
  void tirathons_betrayal( special_effect_t& );
  void windscar_whetstone( special_effect_t& );

  // 7.0 Misc
  void darkmoon_deck( special_effect_t& );
  void infernal_alchemist_stone( special_effect_t& ); // WIP

  // 7.0 Raid
  void bloodthirsty_instinct( special_effect_t& );
  void convergence_of_fates( special_effect_t& );
  void natures_call( special_effect_t& );
  void ravaged_seed_pod( special_effect_t& );
  void spontaneous_appendages( special_effect_t& );
  void twisting_wind( special_effect_t& );
  void wriggling_sinew( special_effect_t& );

  /* NYI ================================================================

  Emerald Nightmare -------------------------

  void bough_of_corruption( special_effect_t& );
  void unstable_horrorslime( special_effect_t& );

  Nighthold ---------------------------------

  Everything

  Healer trinkets / other rubbish -----------

  cocoon_of_enforced_solitude
  goblet_of_nightmarish_ichor
  grotesque_statuette
  horn_of_cenarius
  phantasmal_echo
  vial_of_nightmare_fog
  heightened_senses
  */
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

  effect.player -> passive.add_stat( stat, amount );
}

void set_bonus::simple_callback( special_effect_t& effect )
{ new dbc_proc_callback_t( effect.player, effect ); }

// Mark of the Hidden Satyr =================================================

void enchants::mark_of_the_hidden_satyr( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "mark_of_the_hidden_satyr" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "mark_of_the_hidden_satyr", effect );
  }

  if ( ! effect.execute_action )
  {
    action_t* a = new proc_spell_t( "mark_of_the_hidden_satyr",
      effect.player, effect.player -> find_spell( 191259 ), effect.item );
    a -> spell_power_mod.direct = 1.0; // Jun 27 2016

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

// Giant Ornamental Pearl ===================================================

struct gaseous_bubble_t : public absorb_buff_t
{
  action_t* explosion;

  gaseous_bubble_t( special_effect_t& effect, action_t* a ) : 
    absorb_buff_t( absorb_buff_creator_t( effect.player, "gaseous_bubble", effect.driver(), effect.item ) ),
    explosion( a )
  {
    // Set correct damage amount for explosion.
    a -> base_dd_min = effect.driver() -> effectN( 2 ).min( effect.item );
    a -> base_dd_max = effect.driver() -> effectN( 2 ).max( effect.item );
  }

  void expire_override( int stacks, timespan_t remaining ) override
  {
    absorb_buff_t::expire_override( stacks, remaining );

    explosion -> schedule_execute();
  }
};

void item::giant_ornamental_pearl( special_effect_t& effect )
{
  effect.trigger_spell_id = 214972;

  effect.custom_buff = new gaseous_bubble_t( effect, effect.create_action() );

  // Reset trigger_spell_id so it does not create an execute action.
  effect.trigger_spell_id = 0;
}

// Impact Tremor ============================================================

void item::impact_tremor( special_effect_t& effect )
{
  action_t* stampede = effect.create_action();

  effect.custom_buff = buff_creator_t( effect.player, "devilsaurs_stampede", effect.driver() -> effectN( 1 ).trigger(), effect.item )
    .tick_zero( true )
    .tick_callback( [ stampede ]( buff_t*, int, const timespan_t& ) {
      stampede -> schedule_execute();
    } );

  // Disable automatic creation of a trigger spell.
  effect.trigger_spell_id = 1;

  new dbc_proc_callback_t( effect.item, effect );
}

// Momento of Angerboda =====================================================

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

// Obelisk of the Void ======================================================

void item::obelisk_of_the_void( special_effect_t& effect )
{
  effect.custom_buff = stat_buff_creator_t( effect.player, "collapsing_shadow", effect.player -> find_spell( 215476 ), effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_AGI_INT ), effect.driver() -> effectN( 2 ).average( effect.item ) );
}

// Shivermaws Jawbone =======================================================

struct ice_bomb_t : public proc_spell_t
{
  buff_t* buff;

  ice_bomb_t( special_effect_t& effect ) : 
    proc_spell_t( "ice_bomb", effect.player, effect.driver(), effect.item )
  {
    buff = stat_buff_creator_t( effect.player, "frigid_armor", effect.player -> find_spell( 214589 ), effect.item );
  }

  void execute() override
  {
    proc_spell_t::execute();

    buff -> trigger( num_targets_hit );
  }
};

void item::shivermaws_jawbone( special_effect_t& effect )
{
  effect.execute_action = new ice_bomb_t( effect );
}

// Spiked Counterweight =====================================================

struct haymaker_damage_t : public proc_spell_t
{
  haymaker_damage_t( const special_effect_t& effect ) :
    proc_spell_t( "brutal_haymaker_vulnerability", effect.player, effect.driver() -> effectN( 2 ).trigger(), effect.item )
  {
    may_crit = may_miss = false;
    dot_duration = timespan_t::zero();
  }

  void init() override
  {
    proc_spell_t::init();

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
      event_t( *a -> player ), action( a ), callback( cb ), debuff( d ), damage( 0 )
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

  haymaker_driver_t( const special_effect_t& e, double m ) :
    dbc_proc_callback_t( e.player, e ), effect( e ), multiplier( m ),
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
      
      // Vulnerability: Deal x% of the damage dealt to the debuffed target, up to the limit.

      // Create effect for the callback.
      special_effect_t* effect2 = new special_effect_t( effect -> player );
      effect2 -> name_str = "brutal_haymaker_accumulator";
      effect2 -> proc_chance_ = 1.0;
      effect2 -> proc_flags_ = PF_ALL_DAMAGE;
      effect2 -> proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
      effect -> player -> special_effects.push_back( effect2 );

      // Create callback. We'll enable this callback whenever the debuff is active.
      haymaker_driver_t* callback =
        new haymaker_driver_t( *effect2, effect -> trigger() -> effectN( 2 ).percent() );
      callback -> initialize();
      callback -> deactivate();

      // Create debuff with stack callback.
      td -> debuff.brutal_haymaker = buff_creator_t( *td, "brutal_haymaker", effect -> trigger() )
        .stack_change_callback( [ callback ]( buff_t*, int old, int new_ )
        {
          if ( old == 0 ) {
            assert( ! callback -> active );
            callback -> activate();
          } else if ( new_ == 0 )
            callback -> deactivate();
        } )
        .default_value( effect -> driver() -> effectN( 1 ).trigger() -> effectN( 3 ).average( effect -> item ) );
      td -> debuff.brutal_haymaker -> reset();

      // Set pointer to debuff so the callback can do its thing.
      callback -> debuff = td -> debuff.brutal_haymaker;
    }
  }
};

struct brutal_haymaker_initial_t : public proc_spell_t
{
  brutal_haymaker_initial_t( special_effect_t& effect ) :
    proc_spell_t( "brutal_haymaker", effect.player, effect.driver() -> effectN( 1 ).trigger(), effect.item )
  {}

  void impact( action_state_t* s ) override
  {
    proc_spell_t::impact( s );

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

void item::windscar_whetstone( special_effect_t& effect )
{
  action_t* maelstrom = effect.create_action();
  maelstrom -> cooldown -> duration = timespan_t::zero(); // damage spell has erroneous cooldown

  effect.custom_buff = buff_creator_t( effect.player, "slicing_maelstrom", effect.driver(), effect.item )
    .tick_zero( true )
    .tick_callback( [ maelstrom ]( buff_t*, int, const timespan_t& ) {
      maelstrom -> schedule_execute();
    } );

  // Disable automatic creation of a trigger spell.
  effect.trigger_spell_id = 1;
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

    // Set correct absorb amount.
    parse_effect_data( data().effectN( 2 ) );
  }
};

struct darkstrikes_t : public proc_spell_t
{
  action_t* absorb;

  darkstrikes_t( const special_effect_t& effect ) :
    proc_spell_t( "darkstrikes", effect.player, effect.player -> find_spell( 215659 ), effect.item )
  {
    // Set correct damage amount (can accidentally pick up absorb amount instead).
    parse_effect_data( data().effectN( 1 ) );
  }

  void init() override
  {
    proc_spell_t::init();

    absorb = player -> find_action( "darkstrikes_absorb" );
  }

  void execute() override
  {
    proc_spell_t::execute();

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

  // Special effect to drive the AOE damage callback
  special_effect_t* dmg_effect = new special_effect_t( effect.player );
  dmg_effect -> name_str = "darkstrikes_driver";
  dmg_effect -> spell_id = effect.driver() -> id();
  dmg_effect -> cooldown_ = timespan_t::zero();
  effect.player -> special_effects.push_back( dmg_effect );

  // And create, initialized and deactivate the callback
  dbc_proc_callback_t* callback = new darkstrikes_driver_t( *dmg_effect );
  callback -> initialize();
  callback -> deactivate();

  effect.custom_buff = buff_creator_t( effect.player, "darkstrikes", effect.driver(), effect.item )
    .chance( 1 ) // overrride RPPM
    .activated( false )
    .stack_change_callback( [ callback ]( buff_t*, int old, int new_ )
    {
      if ( old == 0 ) {
        assert( ! callback -> active );
        callback -> activate();
      } else if ( new_ == 0 )
        callback -> deactivate();
    } );
}

// Horn of Valor ============================================================

void item::horn_of_valor( special_effect_t& effect )
{
  effect.custom_buff = stat_buff_creator_t( effect.player, "valarjars_path", effect.driver(), effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver() -> effectN( 1 ).average( effect.item ) );
}

// Tiny Oozeling in a Jar ===================================================

struct fetid_regurgitation_t : public proc_spell_t
{
  buff_t* driver_buff;

  fetid_regurgitation_t( special_effect_t& effect, buff_t* b ) :
    proc_spell_t( "fetid_regurgitation", effect.player, effect.driver() -> effectN( 1 ).trigger(), effect.item ),
    driver_buff( b )
  {
    // Set damage amount, since the scaled amount is not in the damage spell.
    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( item );
  }

  double action_multiplier() const override
  {
    double am = proc_spell_t::action_multiplier();

    am *= driver_buff -> check_value();

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
    action_t* damage;

    fetid_regurgitation_buff_t( special_effect_t& effect, buff_t* cg ) : 
      buff_t( buff_creator_t( effect.player, "fetid_regurgitation", effect.driver(), effect.item )
        .activated( false )
        .tick_zero( true )
        .tick_callback( [ this ] ( buff_t*, int, const timespan_t& ) {
          damage -> schedule_execute();
        } ) ), congealing_goo( cg )
    {
      damage = effect.player -> find_action( "fetid_regurgitation" );
      if ( ! damage )
      {
        damage = effect.player -> create_proc_action( "fetid_regurgitation", effect );
      }

      if ( ! damage )
      {
        damage = new fetid_regurgitation_t( effect, this );
      }
    }

    bool trigger( int stack, double, double chance, timespan_t duration ) override
    {
      if ( congealing_goo -> stack() > 0 )
      {
        bool s = buff_t::trigger( stack, congealing_goo -> stack(), chance, duration );

        congealing_goo -> expire();

        return s;
      }

      return false;
    }
  };

  effect.custom_buff = new fetid_regurgitation_buff_t( effect, charges );
};

// Figurehead of the Naglfar ================================================

struct taint_of_the_sea_t : public proc_spell_t
{
  taint_of_the_sea_t( const special_effect_t& effect ) :
    proc_spell_t( "taint_of_the_sea", effect.player, effect.player -> find_spell( 215695 ), effect.item )
  {
    base_dd_min = base_dd_max = 0;
    base_multiplier = effect.driver() -> effectN( 1 ).percent();
  }

  void init() override
  {
    proc_spell_t::init();

    // Allow DA multipliers so base_multiplier may take effect.
    snapshot_flags = STATE_MUL_DA;
    update_flags = 0;
  }

  // Override so ONLY base_multiplier takes effect.
  double composite_da_multiplier( const action_state_t* ) const override
  { return base_multiplier; }

  void execute() override
  {
    buff_t* d = player -> get_target_data( target ) -> debuff.taint_of_the_sea;
    
    assert( d && d -> check() );

    base_dd_min = base_dd_max = std::min( base_dd_min, d -> current_value / base_multiplier );

    proc_spell_t::execute();

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

      // Create special effect for the damage transferral.
      special_effect_t* effect2 = new special_effect_t( effect -> player );
      effect2 -> name_str = "taint_of_the_sea_driver";
      effect2 -> proc_chance_ = 1.0;
      effect2 -> proc_flags_ = PF_ALL_DAMAGE;
      effect2 -> proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
      effect -> player -> special_effects.push_back( effect2 );

      // Create callback; we'll enable this callback whenever the debuff is active.
      taint_of_the_sea_driver_t* callback = new taint_of_the_sea_driver_t( *effect2 );
      callback -> initialize();
      callback -> deactivate();

      td -> debuff.taint_of_the_sea = buff_creator_t( *td, "taint_of_the_sea", effect -> driver() )
      .default_value( effect -> driver() -> effectN( 2 ).trigger() -> effectN( 2 ).average( effect -> item ) )
      .stack_change_callback( [ callback ]( buff_t* b, int old, int new_ )
      {
        if ( old == 0 )
        {
          assert( ! callback -> active );
          callback -> active_target = b -> player;
          callback -> activate();
        }
        else if ( new_ == 0 )
        {
          callback -> active_target = nullptr;
          callback -> deactivate();
        }
      } );
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

  struct apply_debuff_t : public action_t
  {
    apply_debuff_t( special_effect_t& effect ) :
      action_t( ACTION_OTHER, "apply_taint_of_the_sea", effect.player, spell_data_t::nil() )
    {
      background = quiet = true;
      callbacks = false;
    }

    // Suppress assertion
    result_e calculate_result( action_state_t* ) const override
    { return RESULT_NONE; }

    void execute() override
    {
      action_t::execute();

      player -> get_target_data( target ) -> debuff.taint_of_the_sea -> trigger();
    }
  };

  effect.execute_action = new apply_debuff_t( effect );
}

// Chaos Talisman ===========================================================
// TODO: Stack decay

void item::chaos_talisman( special_effect_t& effect )
{
  effect.proc_flags_ = PF_MELEE;
  effect.proc_flags2_ = PF_ALL_DAMAGE;
  const spell_data_t* buff_spell = effect.driver() -> effectN( 1 ).trigger();
  effect.custom_buff = stat_buff_creator_t( effect.player, "chaotic_energy", buff_spell, effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI ), buff_spell -> effectN( 1 ).average( effect.item ) )
    .period( timespan_t::zero() ); // disable ticking

  new dbc_proc_callback_t( effect.item, effect );
}

// Corrupted Starlight ======================================================
// TOCHECK: Do these void zones stack?

struct nightfall_t : public proc_spell_t
{
  proc_spell_t* damage_spell;

  nightfall_t( special_effect_t& effect ) : 
    proc_spell_t( "nightfall", effect.player, effect.player -> find_spell( 213785 ), effect.item )
  {
    const spell_data_t* tick_spell = effect.player -> find_spell( 213786 );
    proc_spell_t* t = new proc_spell_t( "nightfall_tick", effect.player, tick_spell, effect.item );
    t -> dual = t -> ground_aoe = true;
    t -> stats = stats;
    damage_spell = t;
  }

  void execute() override
  {
    proc_spell_t::execute();

    new ( *sim ) ground_aoe_event_t( player, ground_aoe_params_t()
      .target( execute_state -> target )
      .x( execute_state -> target -> x_position )
      .y( execute_state -> target -> y_position )
      .duration( data().duration() )
      .start_time( sim -> current_time() )
      .action( damage_spell )
      .pulse_time( timespan_t::from_seconds( 1.0 ) ) );
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

struct aw_nuts_t : public proc_spell_t
{
  aw_nuts_t( special_effect_t& effect ) : 
    proc_spell_t( "aw_nuts", effect.player, effect.player -> find_spell( 216099 ), effect.item )
  {
    travel_speed = 7.0; // "Charge"!

    // Set damage amount, as the scaled value isn't stored in the damage spell.
    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( item );
  }
   
  void init() override
  {
    proc_spell_t::init();

    // Don't benefit from player multipliers because, in game, the squirrel is dealing the damage, not you.
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

  // Set trigger spell so we can automatically create the breath action.
  effect.trigger_spell_id = 222520;
  action_t* breath = effect.create_action();

  // Disable trigger spell again
  effect.trigger_spell_id = 0;

  new natures_call_callback_t( effect, buffs, breath );
}

// Moonlit Prism ============================================================
// TOCHECK: Proc mechanics

void item::moonlit_prism( special_effect_t& effect )
{
  // Create stack gain driver
  special_effect_t* effect2 = new special_effect_t( effect.player );
  effect2 -> name_str     = "moonlit_prism_driver";
  effect2 -> proc_chance_ = 1.0;
  effect2 -> spell_id = effect.driver() -> id();
  effect2 -> cooldown_ = timespan_t::zero();
  effect.player -> special_effects.push_back( effect2 );

  // Create callback; it will be enabled when the buff is active.
  dbc_proc_callback_t* callback = new dbc_proc_callback_t( effect.player, *effect2 );
  callback -> initialize();
  callback -> deactivate();

  // Create buff.
  effect.custom_buff = stat_buff_creator_t( effect.player, "elunes_light", effect.driver(), effect.item )
    .cd( timespan_t::zero() )
    .refresh_behavior( BUFF_REFRESH_DISABLED )
    .stack_change_callback( [ callback ]( buff_t*, int old, int new_ )
    {
      if ( old == 0 ) {
        assert( ! callback -> active );
        callback -> activate();
      } else if ( new_ == 0 )
        callback -> deactivate();
    } );

  // Assign buff to our stack gain driver.
  effect2 -> custom_buff = effect.custom_buff;
}

// Faulty Countermeasures ===================================================

void item::faulty_countermeasures( special_effect_t& effect )
{
  // Create effect & callback for the damage proc
  special_effect_t* effect2 = new special_effect_t( effect.player );
  effect2 -> name_str       = "brittle";
  effect2 -> spell_id       = effect.driver() -> id();
  effect2 -> cooldown_      = timespan_t::zero();
  effect.player -> special_effects.push_back( effect2 );

  dbc_proc_callback_t* callback = new dbc_proc_callback_t( effect.player, *effect2 );
  callback -> initialize();
  callback -> deactivate();

  effect.custom_buff = buff_creator_t( effect.player, "sheathed_in_frost", effect.driver(), effect.item )
    .cd( timespan_t::zero() )
    .activated( false )
    .chance( 1 )
    .stack_change_callback( [ callback ]( buff_t*, int old, int new_ )
    {
      if ( old == 0 ) {
        assert( ! callback -> active );
        callback -> activate();
      } else if ( new_ == 0 )
        callback -> deactivate();
    } );
}

// Stormsinger Fulmination Charge ===========================================

// 9 seconds ascending, 2 seconds at max stacks, 9 seconds descending. TOCHECK
struct focused_lightning_t : public stat_buff_t
{
  focused_lightning_t( const special_effect_t& effect ) :
    stat_buff_t( stat_buff_creator_t( effect.player, "focused_lightning", effect.trigger() -> effectN( 1 ).trigger(), effect.item )
      .duration( timespan_t::from_seconds( 20.0 ) ) )
  { }

  void bump( int stacks, double value ) override
  {
    bool waste = current_stack == max_stack();

    stat_buff_t::bump( stacks, value );

    if ( waste )
    {
      if ( sim -> debug )
        sim -> out_debug.printf( "%s buff %s changes to reverse.", player -> name(), name() );

      reverse = true;
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    stat_buff_t::expire_override( expiration_stacks, remaining_duration );

    reverse = false;
  }

  void reset() override
  {
    stat_buff_t::reset();

    reverse = false;
  }
};

void item::stormsinger_fulmination_charge( special_effect_t& effect )
{
  effect.custom_buff = new focused_lightning_t( effect );

  new dbc_proc_callback_t( effect.item, effect );
}

// Portable Manacracker =====================================================

struct volatile_magic_debuff_t : public debuff_t
{
  action_t* damage;

  volatile_magic_debuff_t( const special_effect_t& effect, actor_target_data_t& td ) :
    debuff_t( buff_creator_t( td, "volatile_magic", effect.trigger() ) ),
    damage( effect.player -> find_action( "withering_consumption" ) )
  {}

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool s = debuff_t::trigger( stacks, value, chance, duration );

    if ( current_stack == max_stack() )
    {
      damage -> target = player;
      damage -> schedule_execute();
      expire();
    }

    return s;
  }
};

struct portable_manacracker_constructor_t : public item_targetdata_initializer_t
{
  portable_manacracker_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( effect == 0 )
    {
      td -> debuff.volatile_magic = buff_creator_t( *td, "volatile_magic" );
    }
    else
    {
      assert( ! td -> debuff.volatile_magic );

      td -> debuff.volatile_magic = new volatile_magic_debuff_t( *effect, *td );
      td -> debuff.volatile_magic -> reset();
    }
  }
};

struct volatile_magic_callback_t : public dbc_proc_callback_t
{
  volatile_magic_callback_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.item, effect )
  {}

  void execute( action_t* a, action_state_t* s ) override
  {
    actor_target_data_t* td = a -> player -> get_target_data( s -> target );

    if ( ! td ) return;

    td -> debuff.volatile_magic -> trigger();
  }
};

void item::portable_manacracker( special_effect_t& effect )
{
  // Set spell so we can create the DoT action.
  effect.trigger_spell_id = 215884;
  effect.proc_flags2_ = PF2_CRIT;
  action_t* dot = effect.create_action();
  // Disable trigger spell again.
  effect.trigger_spell_id = 0;
  dot -> base_td = effect.trigger() -> effectN( 1 ).average( effect.item );
  dot -> tick_zero = true;

  new volatile_magic_callback_t( effect );
}

// Infernal Alchemist Stone =================================================

void item::infernal_alchemist_stone( special_effect_t& effect )
{
  const spell_data_t* stat_spell = effect.player -> find_spell( 60229 );

  effect.custom_buff = stat_buff_creator_t( effect.player, "infernal_alchemist_stone", effect.driver(), effect.item )
    .duration( stat_spell -> duration() )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver() -> effectN( 1 ).average( effect.item ) )
    .chance( 1 ); // RPPM is handled by the special effect, so make the buff always go up

  new dbc_proc_callback_t( effect.item, effect );
}

// Spontaneous Appendages ===================================================

void item::spontaneous_appendages( special_effect_t& effect )
{
  // Set trigger spell so we can create an action from it.
  effect.trigger_spell_id = effect.trigger() -> effectN( 1 ).trigger() -> id();
  action_t* slam = effect.create_action();
  // Spell data has no radius, so manually make it an AoE.
  slam -> radius = 8.0;
  slam -> aoe = -1;

  // Reset trigger spell, we don't want the proc to trigger an action.
  effect.trigger_spell_id = 0;

  effect.custom_buff = buff_creator_t( effect.player, "horrific_appendages", effect.trigger(), effect.item )
    .tick_callback( [ slam ]( buff_t*, int, const timespan_t& ) {
      slam -> schedule_execute();
    } );

  new dbc_proc_callback_t( effect.item, effect );
}

// Wriggling Sinew ==========================================================

struct wriggling_sinew_constructor_t : public item_targetdata_initializer_t
{
  struct maddening_whispers_debuff_t : public debuff_t
  {
    action_t* action;

    maddening_whispers_debuff_t( const special_effect_t& effect, actor_target_data_t& td ) : 
      debuff_t( buff_creator_t( td, "maddening_whispers", effect.trigger(), effect.item ) ),
      action( effect.player -> find_action( "maddening_whispers" ) )
    {}

    void expire_override( int stacks, timespan_t dur ) override
    {
      debuff_t::expire_override( stacks, dur );

      // Schedule an execute, but snapshot state right now so we can apply the stack multiplier.
      action_state_t* s   = action -> get_state();
      s -> target         = player;
      action -> snapshot_state( s, DMG_DIRECT );
      s -> target_da_multiplier *= stacks;
      action -> schedule_execute( s );
    }
  };

  wriggling_sinew_constructor_t( unsigned iid, const std::vector< slot_e >& s ) :
    item_targetdata_initializer_t( iid, s )
  {}

  void operator()( actor_target_data_t* td ) const override
  {
    const special_effect_t* effect = find_effect( td -> source );
    if ( effect == 0 )
    {
      td -> debuff.maddening_whispers = buff_creator_t( *td, "maddening_whispers" );
    }
    else
    {
      assert( ! td -> debuff.maddening_whispers );

      td -> debuff.maddening_whispers = new maddening_whispers_debuff_t( *effect, *td );
      td -> debuff.maddening_whispers -> reset();
    }
  }
};

struct maddening_whispers_cb_t : public dbc_proc_callback_t
{
  buff_t* buff;

  maddening_whispers_cb_t( const special_effect_t& effect, buff_t* b ) : 
    dbc_proc_callback_t( effect.player, effect ), buff( b )
  {}

  void execute( action_t*, action_state_t* s ) override
  {
    if ( s -> target == s -> action -> player ) return;
    if ( s -> result_amount <= 0 ) return;

    actor_target_data_t* td = s -> action -> player -> get_target_data( s -> target );

    if ( ! td ) return;

    td -> debuff.maddening_whispers -> trigger();
    buff -> decrement();
  }
};

struct maddening_whispers_t : public buff_t
{
  dbc_proc_callback_t* callback;

  maddening_whispers_t( const special_effect_t& effect ) : 
    buff_t( buff_creator_t( effect.player, "maddening_whispers", effect.driver(), effect.item ) )
  {
    // Stack gain effect
    special_effect_t* effect2 = new special_effect_t( effect.player );
    effect2 -> name_str     = "maddening_whispers_driver";
    effect2 -> proc_chance_ = 1.0;
    effect2 -> proc_flags_  = PF_SPELL | PF_AOE_SPELL;
    effect2 -> proc_flags2_  = PF2_ALL_HIT;
    effect.player -> special_effects.push_back( effect2 );

    callback = new maddening_whispers_cb_t( *effect2, this );
    callback -> initialize();
    callback -> deactivate();
  }
  
  void start( int, double value, timespan_t duration ) override
  {
    // Always start at max stacks.
    buff_t::start( max_stack(), value, duration );

    callback -> activate();
  }

  void expire_override( int stacks, timespan_t remaining ) override
  {
    buff_t::expire_override( stacks, remaining );
    
    callback -> deactivate();

    for ( size_t i = 0; i < sim -> target_non_sleeping_list.size(); i++ ) {
      player_t* t = sim -> target_non_sleeping_list[ i ];
      player -> get_target_data( t ) -> debuff.maddening_whispers -> expire();
    }
  }
};

void item::wriggling_sinew( special_effect_t& effect )
{
  // Set triggered spell so we can create an action from it.
  effect.trigger_spell_id = 222050;
  action_t* a = effect.initialize_offensive_spell_action();
  a -> base_dd_min = a -> base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item );
  // Reset triggered spell; we don't want to trigger a spell on use.
  effect.trigger_spell_id = 0;

  effect.custom_buff = new maddening_whispers_t( effect );
}

// Convergence of Fates =====================================================

struct convergence_cd_t
{
  specialization_e spec;
  const char* cooldowns[3];
};

static const convergence_cd_t convergence_cds[] =
{
  /* !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!!

    Spells that trigger buffs must have the cooldown of their buffs removed
    if they have one, or this trinket may cause undesirable results.

  !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! */
  { DEATH_KNIGHT_FROST,   { "pillar_of_frost" } },
  { DEATH_KNIGHT_UNHOLY,  { "summon_gargoyle" } },
  { DRUID_FERAL,          { "berserk", "incarnation_king_of_the_jungle" } },
  { HUNTER_BEAST_MASTERY, { "aspect_of_the_wild" } },
  { HUNTER_MARKSMANSHIP,  { "trueshot" } },
  { HUNTER_SURVIVAL,      { "aspect_of_the_eagle" } },
  { MONK_WINDWALKER,      { "storm_earth_and_fire" } },
  { PALADIN_RETRIBUTION,  { "avenging_wrath" } },
  { ROGUE_SUBTLETY,       { "vendetta" } },
  { ROGUE_OUTLAW,         { "adrenaline_rush" } },
  { ROGUE_ASSASSINATION,  { "shadow_blades" } },
  { SHAMAN_ENHANCEMENT,   { "feral_spirit" } },
  { WARRIOR_ARMS,         { "battle_cry" } },
  { WARRIOR_FURY,         { "battle_cry" } },
  { DEMON_HUNTER_HAVOC,   { "metamorphosis" } },
  { SPEC_NONE,            { 0 } }
};

struct convergence_of_fates_callback_t : public dbc_proc_callback_t
{
  std::vector<cooldown_t*> cooldowns;
  timespan_t amount;

  convergence_of_fates_callback_t( const special_effect_t& effect ) : 
    dbc_proc_callback_t( effect.item, effect ),
    amount( timespan_t::from_seconds( -effect.driver() -> effectN( 1 ).base_value() ) )
  {
    const convergence_cd_t* cd = &( convergence_cds[ 0 ] );
    do
    {
      if ( effect.player -> specialization() != cd -> spec )
      {
        cd++;
        continue;
      }
      
      for ( size_t i = 0; i < 3; i++ )
      {
        if ( ! cd -> cooldowns[ i ] )
          continue;

        cooldown_t* c = effect.player -> get_cooldown( cd -> cooldowns[ i ] );

        if ( c )
          cooldowns.push_back( c );
      }

      break;
    } while ( cd -> spec != SPEC_NONE );
  }

  void execute( action_t*, action_state_t* ) override
  {
    assert( cooldowns.size() > 0 );

    for ( size_t i = 0; i < cooldowns.size(); i++ )
    {
      cooldowns[ i ] -> adjust( amount );
    }
  }
};

void item::convergence_of_fates( special_effect_t& effect )
{
  new convergence_of_fates_callback_t( effect );
}

// Twisting Wind ============================================================
/* FIXME: How many times does this really hit? It has a small radius and
  moves around so it doesn't hit every tick.
   TOCHECK: Does this AoE stack? */

struct tormenting_cyclone_t : public proc_spell_t
{
  proc_spell_t* damage_spell;

  tormenting_cyclone_t( special_effect_t& effect ) : 
    proc_spell_t( "tormenting_cyclone", effect.player, effect.trigger(), effect.item )
  {
    damage_spell = new proc_spell_t( "tormenting_cyclone_tick", effect.player, effect.player -> find_spell( 221865 ), effect.item );
    damage_spell -> dual = true;
    damage_spell -> stats = stats;
    damage_spell -> base_dd_min = damage_spell -> base_dd_max = 
      effect.driver() -> effectN( 1 ).average( effect.item );
  }

  void execute() override
  {
    proc_spell_t::execute();

    new ( *sim ) ground_aoe_event_t( player, ground_aoe_params_t()
      .target( execute_state -> target )
      .x( execute_state -> target -> x_position )
      .y( execute_state -> target -> y_position )
      .duration( data().duration() )
      .start_time( sim -> current_time() )
      .action( damage_spell )
      .pulse_time( timespan_t::from_seconds( 1.0 ) ) );
  }
};

void item::twisting_wind( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "tormenting_cyclone" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "tormenting_cyclone", effect );
  }

  if ( ! effect.execute_action )
  {
    effect.execute_action = new tormenting_cyclone_t( effect );
  }

  new dbc_proc_callback_t( effect.item, effect );
}

// Bloodthirsty Instinct ====================================================

struct bloodthirsty_instinct_cb_t : public dbc_proc_callback_t
{
  bloodthirsty_instinct_cb_t( special_effect_t& effect ) :
    dbc_proc_callback_t( effect.item, effect )
  {}

  void trigger( action_t* a, void* call_data ) override
  {
    assert( rppm );

    action_state_t* s = static_cast<action_state_t*>( call_data );

    assert( s -> target );
    
    // Set RPPM modifier by target's health percentage. Linear chance increase from 0% to 100% health deficit.
    double mod = ( 200.0 - s -> target -> health_percentage() ) / 100.0;

    if ( effect.player -> sim -> debug )
      effect.player -> sim -> out_debug.printf( "Player %s adjusts %s rppm modifier: old=%.3f new=%.3f",
        effect.player -> name(), effect.name().c_str(), rppm -> get_modifier(), mod );

    rppm -> set_modifier( mod );

    dbc_proc_callback_t::trigger( a, call_data );
  }
};

void item::bloodthirsty_instinct( special_effect_t& effect )
{
  new bloodthirsty_instinct_cb_t( effect );
}

// Ravaged Seed Pod =========================================================

struct infested_ground_t : public proc_spell_t
{
  proc_spell_t* damage_spell;

  infested_ground_t( special_effect_t& effect ) : 
    proc_spell_t( "infested_ground", effect.player, effect.driver(), effect.item )
  {
    damage_spell = new proc_spell_t( "infested_ground_tick", effect.player, effect.player -> find_spell( 221804 ), effect.item );
    damage_spell -> dual = damage_spell -> ground_aoe = true;
    damage_spell -> stats = stats;
    damage_spell -> base_dd_min = damage_spell -> base_dd_max = 
      effect.driver() -> effectN( 2 ).average( effect.item );
  }

  void execute() override
  {
    proc_spell_t::execute();

    new ( *sim ) ground_aoe_event_t( player, ground_aoe_params_t()
      .target( execute_state -> target )
      .x( player -> x_position )
      .y( player -> y_position )
      .duration( data().duration() )
      .start_time( sim -> current_time() )
      .action( damage_spell )
      .pulse_time( timespan_t::from_seconds( 1.0 ) ) );
  }
};

void item::ravaged_seed_pod( special_effect_t& effect )
{
  effect.execute_action = effect.player -> find_action( "infested_ground" );

  if ( ! effect.execute_action )
  {
    effect.execute_action = effect.player -> create_proc_action( "infested_ground", effect );
  }

  if ( ! effect.execute_action )
  {
    effect.execute_action = new infested_ground_t( effect );
  }

  // FIXME: Only while in the area of effect.
  effect.custom_buff = stat_buff_creator_t( effect.player, "leeching_pestilence", effect.player -> find_spell( 221805 ), effect.item )
    .add_stat( STAT_LEECH_RATING, effect.driver() -> effectN( 3 ).average( effect.item ) );
}

// March of the Legion ======================================================

void set_bonus::march_of_the_legion( special_effect_t& /* effect */ ) {}

// Journey Through Time =====================================================

void set_bonus::journey_through_time( special_effect_t& /* effect */ ) {}

void unique_gear::register_special_effects_x7()
{
  /* Legion 7.0 Dungeon */
  register_special_effect( 214829, item::chaos_talisman                 );
  register_special_effect( 213782, item::corrupted_starlight            );
  register_special_effect( 216085, item::elementium_bomb_squirrel       );
  register_special_effect( 214962, item::faulty_countermeasures         );
  register_special_effect( 215670, item::figurehead_of_the_naglfar      );
  register_special_effect( 214971, item::giant_ornamental_pearl         );
  register_special_effect( 215956, item::horn_of_valor                  );
  register_special_effect( 224059, item::impact_tremor                  );
  register_special_effect( 214798, item::memento_of_angerboda           );
  register_special_effect( 215648, item::moonlit_prism                  );
  register_special_effect( 215467, item::obelisk_of_the_void            );
  register_special_effect( 215857, item::portable_manacracker           );
  register_special_effect( 214584, item::shivermaws_jawbone             );
  register_special_effect( 214168, item::spiked_counterweight           );
  register_special_effect( 215630, item::stormsinger_fulmination_charge );
  register_special_effect( 215127, item::tiny_oozeling_in_a_jar         );
  register_special_effect( 215658, item::tirathons_betrayal             );
  register_special_effect( 214980, item::windscar_whetstone             );
  register_special_effect( 214054, "214052Trigger"                      );
  register_special_effect( 215444, "215407Trigger"                      );
  register_special_effect( 215813, "ProcOn/Hit_1Tick_215816Trigger"     );
  register_special_effect( 214492, "ProcOn/Hit_1Tick_214494Trigger"     );
  register_special_effect( 214340, "ProcOn/Hit_1Tick_214342Trigger"     );

  /* Legion 7.0 Raid */
  register_special_effect( 221786, item::bloodthirsty_instinct  );
  register_special_effect( 225139, item::convergence_of_fates   );
  register_special_effect( 222512, item::natures_call           );
  register_special_effect( 222167, item::spontaneous_appendages );
  register_special_effect( 221803, item::ravaged_seed_pod       );
  register_special_effect( 221845, item::twisting_wind          );
  register_special_effect( 222046, item::wriggling_sinew        );
  register_special_effect( 221767, "ProcOn/crit" );

  /* Legion 7.0 Misc */
  register_special_effect( 188026, item::infernal_alchemist_stone       );
  register_special_effect( 191563, item::darkmoon_deck                  );
  register_special_effect( 191611, item::darkmoon_deck                  );
  register_special_effect( 191632, item::darkmoon_deck                  );

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
  sim -> register_target_data_initializer( portable_manacracker_constructor_t( 137398, trinkets ) );
  sim -> register_target_data_initializer( wriggling_sinew_constructor_t( 139326, trinkets ) );
}

