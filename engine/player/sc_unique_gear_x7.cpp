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
  void caged_horror( special_effect_t& );
  void chaos_talisman( special_effect_t& );
  void corrupted_starlight( special_effect_t& );
  void darkmoon_deck_dominion( special_effect_t& );
  void darkmoon_deck_hellfire( special_effect_t& );
  void darkmoon_deck_immortality( special_effect_t& );
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
}

namespace set_bonus
{
  // Generic passive stat aura adder for set bonuses
  void passive_stat_aura( special_effect_t& );
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

struct random_combat_enhancement_callback_t : public dbc_proc_callback_t
{
  stat_buff_t* crit, *haste, *mastery;

  random_combat_enhancement_callback_t( const item_t* i,
                       const special_effect_t& effect,
                       stat_buff_t* cb,
                       stat_buff_t* hb,
                       stat_buff_t* mb ) :
                 dbc_proc_callback_t( i, effect ),
    crit( cb ), haste( hb ), mastery( mb )
  {}

  void execute( action_t* /* a */, action_state_t* /* call_data */ ) override
  {
    stat_buff_t* buff;

    int p_type = ( int ) ( listener -> sim -> rng().real() * 3.0 );
    switch ( p_type )
    {
      case 0: buff = haste; break;
      case 1: buff = crit; break;
      case 2:
      default:
        buff = mastery; break;
    }

    buff -> trigger();
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

  stat_buff_t* crit_buff =
    stat_buff_creator_t( effect.player, "howl_of_ingvar", effect.player -> find_spell( 214802 ) )
      .activated( false )
      .add_stat( STAT_CRIT_RATING, rating_amount );
  stat_buff_t* haste_buff =
    stat_buff_creator_t( effect.player, "wail_of_svala", effect.player -> find_spell( 214803 ) )
     .activated( false )
      .add_stat( STAT_HASTE_RATING, rating_amount );
  stat_buff_t* mastery_buff =
    stat_buff_creator_t( effect.player, "dirge_of_angerboda", effect.player -> find_spell( 214807 ) )
      .activated( false )
      .add_stat( STAT_MASTERY_RATING, rating_amount );

  new random_combat_enhancement_callback_t( effect.item, effect, crit_buff, haste_buff, mastery_buff );
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

      td -> debuff.brutal_haymaker = buff_creator_t( td -> target, "brutal_haymaker", td -> source -> find_spell( 214169 ) )
        .default_value( td -> source -> find_spell( 214169 ) -> effectN( 2 ).percent() );
      td -> debuff.brutal_haymaker -> reset();
    }
  }
};

struct brutal_haymaker_t : public spell_t
{
  brutal_haymaker_t( special_effect_t& effect ) :
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
  effect.execute_action = new brutal_haymaker_t( effect );

  new dbc_proc_callback_t( effect.item, effect );
}

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

void item::horn_of_valor( special_effect_t& effect )
{
  effect.custom_buff = stat_buff_creator_t( effect.player, "valarjars_path", effect.driver(), effect.item )
    .add_stat( effect.player -> convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver() -> effectN( 1 ).average( effect.item ) );
}

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
  effect.execute_action = new dark_blast_t( effect );

  new dbc_proc_callback_t( effect.item, effect );
}

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

struct darkmoon_deck_t
{
  player_t* player;
  std::array<buff_t*, 8> cards;
  buff_t* top_card;
  timespan_t shuffle_period;

  darkmoon_deck_t( special_effect_t& effect, std::array<unsigned, 8> c ) : 
    player( effect.player ), shuffle_period( effect.driver() -> effectN( 1 ).period() ),
    top_card( nullptr )
  {
    for ( unsigned i = 0; i < 8; i++ )
    {
      const spell_data_t* s = player -> find_spell( c[ i ] );
      assert( s -> found() );

      std::string& n = std::string( s -> name_cstr() );
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

void item::darkmoon_deck_dominion( special_effect_t& effect )
{
  darkmoon_deck_t* d = new darkmoon_deck_t( effect,
    { { 191545, 191548, 191549, 191550, 191551, 191552, 191553, 191554 } } );

  new ( *effect.player -> sim ) shuffle_event_t( d, true );
}

void item::darkmoon_deck_hellfire( special_effect_t& effect )
{
  darkmoon_deck_t* d = new darkmoon_deck_t( effect,
    { { 191603, 191604, 191605, 191606, 191607, 191608, 191609, 191610 } } );

  new ( *effect.player -> sim ) shuffle_event_t( d, true );
}

void item::darkmoon_deck_immortality( special_effect_t& effect )
{
  darkmoon_deck_t* d = new darkmoon_deck_t( effect,
    { { 191624, 191625, 191626, 191627, 191628, 191629, 191630, 191631 } } );

  new ( *effect.player -> sim ) shuffle_event_t( d, true );
}

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

void unique_gear::register_special_effects_x7()
{
  /* Legion 7.0 */
  register_special_effect( 215444, item::caged_horror                   );
  register_special_effect( 214829, item::chaos_talisman                 );
  register_special_effect( 213782, item::corrupted_starlight            );
  register_special_effect( 191563, item::darkmoon_deck_dominion         );
  register_special_effect( 191611, item::darkmoon_deck_hellfire         );
  register_special_effect( 191632, item::darkmoon_deck_immortality      );
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

  /* Legion Enchants */
  register_special_effect( 190888, "190909trigger" );
  register_special_effect( 190889, "191380trigger" );
  register_special_effect( 190890, "191259trigger" );
  register_special_effect( 228398, "228399trigger" );
  register_special_effect( 228400, "228401trigger" );

  /* T19 Generic Order Hall set bonuses */
  register_special_effect( 221533, set_bonus::passive_stat_aura     );
  register_special_effect( 221534, set_bonus::passive_stat_aura     );
  register_special_effect( 221535, set_bonus::passive_stat_aura     );
}

void unique_gear::register_target_data_initializers_x7( sim_t* sim )
{
  std::vector< slot_e > trinkets;
  trinkets.push_back( SLOT_TRINKET_1 );
  trinkets.push_back( SLOT_TRINKET_2 );

  sim -> register_target_data_initializer( spiked_counterweight_constructor_t( 136715, trinkets ) );
  sim -> register_target_data_initializer( figurehead_of_the_naglfar_constructor_t( 137329, trinkets ) );
}

