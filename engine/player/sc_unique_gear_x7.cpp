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
  void mark_of_the_loyal_druid( special_effect_t& );
  void mark_of_the_trickster( special_effect_t& );
  void mark_of_the_fallen_sentinels( special_effect_t& );

  const unsigned binding_of_crit    = 5427;
  const unsigned binding_of_haste   = 5428;
  const unsigned binding_of_mastery = 5429;
  const unsigned binding_of_vers    = 5430;
  const unsigned binding_of_str     = 5434;
  const unsigned binding_of_agi     = 5435;
  const unsigned binding_of_int     = 5436;
}

namespace item
{
  void chaos_talisman( special_effect_t& );
  void figurehead_of_the_naglfar( special_effect_t& );
  void giant_ornamental_pearl( special_effect_t& );
  void horn_of_valor( special_effect_t& );
  void impact_tremor( special_effect_t& );
  void memento_of_angerboda( special_effect_t& );
  void nightmare_egg_shell( special_effect_t& );
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


static bool player_has_binding( player_t* player, unsigned binding )
{
  // TOCHECK: Does this work with enchants specified by name?
  player = player -> get_owner_or_self();

  for ( size_t i = 0; i < player -> items.size(); i++ )
  {
    if ( player -> items[ i ].parsed.enchant_id == binding )
      return true;
  }
  return false;
}

struct loyal_druid_pet_t : public pet_t
{
  struct auto_attack_t : public melee_attack_t
  {
    struct melee_t: public melee_attack_t
    {
      melee_t( pet_t* p ) :
        melee_attack_t( "melee", p, spell_data_t::nil() )
      {
        school = SCHOOL_PHYSICAL;
        special = false;
        weapon = &( p -> main_hand_weapon );
        base_execute_time = weapon -> swing_time;
        background = repeating = auto_attack = may_glance = true;
        trigger_gcd = timespan_t::zero();
      }
    };

    melee_t* melee;

    auto_attack_t( pet_t* p, const std::string& options_str ) :
      melee_attack_t( "auto_attack", p, spell_data_t::nil() )
    {
      parse_options( options_str );

      ignore_false_positive = true;
      range                 = 5;
      trigger_gcd           = timespan_t::zero();
      melee                 = new melee_t( p );
    }

    void execute() override
    { melee -> schedule_execute(); }

    bool ready() override
    {
      bool ready = melee_attack_t::ready();

      if ( ready && melee -> execute_event == nullptr )
        return ready;

      return false;
    }
  };

  struct thrash_t : public melee_attack_t
  {
    buff_t* cat_form;
    buff_t* bear_form;

    thrash_t( loyal_druid_pet_t* p, const std::string& option_str ) :
      melee_attack_t( "thrash", p, p -> find_spell( 191121 ) )
    {
      parse_options( option_str );

      cooldown -> duration = timespan_t::from_seconds( 6.0 );
      cooldown -> hasted = true;
      cat_form = p -> cat_form;
      bear_form = p -> bear_form;
    }

    bool init_finished() override
    {
      if ( ! player_has_binding( player, enchants::binding_of_crit ) )
        background = true;

      return melee_attack_t::init_finished();
    }

    bool ready() override
    {
      if ( ! ( cat_form -> check() || bear_form -> check() ) )
        return false;

      return melee_attack_t::ready();
    }
  };

  struct wrath_t : public spell_t
  {
    wrath_t( pet_t* p, const std::string& option_str ) :
      spell_t( "wrath", p, p -> find_spell( 191146 ) )
    {
      parse_options( option_str );
    }
  };

  struct rejuvenation_t : public heal_t
  {
    rejuvenation_t( pet_t* p, const std::string& option_str ) :
      heal_t( "rejuvenation", p, p -> find_spell( 191122 ) )
    {
      parse_options( option_str );
      target = p -> owner;
    }

    bool init_finished() override
    {
      if ( ! player_has_binding( player, enchants::binding_of_haste ) )
        background = true;

      return heal_t::init_finished();
    }
  };

  struct entangling_roots_t : public spell_t
  {
    entangling_roots_t( pet_t* p, const std::string& option_str ) :
      spell_t( "entangling_roots", p, p -> find_spell( 191123 ) )
    {
      parse_options( option_str );
    }

    bool init_finished() override
    {
      if ( ! player_has_binding( player, enchants::binding_of_mastery ) )
        background = true;

      return spell_t::init_finished();
    }

    bool ready() override
    {
      bool r = spell_t::ready();

      // Will not cast on immune targets. Let's assume bosses are immune.
      if ( ! target -> is_add() )
        return false;

      return r;
    }
  };

  struct invigorating_roar_t : public spell_t
  {
    invigorating_roar_t( pet_t* p, const std::string& option_str ) :
      spell_t( "invigorating_roar", p, p -> find_spell( 191124 ) )
    {
      parse_options( option_str );

      callbacks = may_crit = may_miss = harmful = false;
    }

    bool init_finished() override
    {
      if ( ! player_has_binding( player, enchants::binding_of_vers ) )
        background = true;

      return spell_t::init_finished();
    }

    void execute() override
    {
      spell_t::execute();

      player -> buffs.invigorating_roar -> trigger();
      debug_cast<pet_t*>( player ) -> owner -> buffs.invigorating_roar -> trigger();
    }

    bool ready() override
    {
      if ( player -> buffs.invigorating_roar -> check() )
        return false;

      return spell_t::ready();
    }
  };

  struct cat_form_t : public spell_t
  {
    buff_t* buff;

    cat_form_t( loyal_druid_pet_t* p, const std::string& option_str ) :
      spell_t( "cat_form", p, p -> find_spell( 191118 ) )
    {
      parse_options( option_str );
      
      callbacks = may_crit = may_miss = harmful = false;
      buff = p -> cat_form;
    }

    bool init_finished() override
    {
      if ( ! player_has_binding( player, enchants::binding_of_agi ) )
        background = true;

      return spell_t::init_finished();
    }

    void execute() override
    {
      spell_t::execute();

      buff -> trigger();
    }

    bool ready() override
    {
      if ( buff -> check() )
        return false;

      return spell_t::ready();
    }
  };

  struct moonkin_form_t : public spell_t
  {
    buff_t* buff;

    moonkin_form_t( loyal_druid_pet_t* p, const std::string& option_str ) :
      spell_t( "moonkin_form", p, p -> find_spell( 191119 ) )
    {
      parse_options( option_str );
      
      callbacks = may_crit = may_miss = harmful = false;
      buff = p -> moonkin_form;
    }

    bool init_finished() override
    {
      if ( ! player_has_binding( player, enchants::binding_of_int ) )
        background = true;

      return spell_t::init_finished();
    }

    void execute() override
    {
      spell_t::execute();

      buff -> trigger();
    }

    bool ready() override
    {
      if ( buff -> check() )
        return false;

      return spell_t::ready();
    }
  };

  struct bear_form_t : public spell_t
  {
    buff_t* buff;

    bear_form_t( loyal_druid_pet_t* p, const std::string& option_str ) :
      spell_t( "bear_form", p, p -> find_spell( 191091 ) )
    {
      parse_options( option_str );
      
      callbacks = may_crit = may_miss = harmful = false;
      buff = p -> bear_form;
    }

    bool init_finished() override
    {
      if ( ! player_has_binding( player, enchants::binding_of_str ) )
        background = true;

      return spell_t::init_finished();
    }

    void execute() override
    {
      spell_t::execute();

      buff -> trigger();
    }

    bool ready() override
    {
      if ( buff -> check() )
        return false;

      return spell_t::ready();
    }
  };

  buff_t* cat_form;
  buff_t* moonkin_form;
  buff_t* bear_form;

  loyal_druid_pet_t( player_t* owner ) :
    pet_t( owner -> sim, owner, "loyal_druid", true, true )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    owner_coeff.ap_from_ap = 1.0; // TOCHECK
    owner_coeff.sp_from_sp = 1.0; // TOCHECK

    // Magical constants for base damage
    double damage_range = 0.4;
    double base_dps = owner -> dbc.spell_scaling( PLAYER_SPECIAL_SCALE, owner -> level() ) * 4.725;
    double min_dps = base_dps * ( 1 - damage_range / 2.0 );
    double max_dps = base_dps * ( 1 + damage_range / 2.0 );
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg =  min_dps * main_hand_weapon.swing_time.total_seconds();
    main_hand_weapon.max_dmg =  max_dps * main_hand_weapon.swing_time.total_seconds();
    // FIXME: Melee damage is definitely wrong.

    cat_form     = buff_creator_t( this, "cat_form", find_spell( 191118 ) );
    moonkin_form = buff_creator_t( this, "moonkin_form", find_spell( 191119 ) )
                   .default_value( find_spell( 191119 ) -> effectN( 2 ).percent() )
                   .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    bear_form    = buff_creator_t( this, "bear_form", find_spell( 191091 ) );
  }

  double composite_player_multiplier( school_e s ) const override
  {
    double pm = pet_t::composite_player_multiplier( s );

    if ( dbc::is_school( s, SCHOOL_NATURE ) )
      pm *= 1.0 + moonkin_form -> value();

    return pm;
  }

  void dismiss( bool expired = false ) override
  {
    pet_t::dismiss( expired );

    owner -> buffs.invigorating_roar -> expire();
  }

  void init_action_list() override
  {
    // TOCHECK: Exact priorities.
    action_list_str = "auto_attack";
    action_list_str += "/cat_form";
    action_list_str += "/moonkin_form";
    action_list_str += "/bear_form";
    action_list_str += "/invigorating_roar";
    action_list_str += "/wrath,if=buff.moonkin_form.up";
    action_list_str += "/rejuvenation";
    action_list_str += "/entangling_roots";
    action_list_str += "/thrash";

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "auto_attack"       ) return       new auto_attack_t( this, options_str );
    if ( name == "bear_form"         ) return         new bear_form_t( this, options_str );
    if ( name == "cat_form"          ) return          new cat_form_t( this, options_str );
    if ( name == "entangling_roots"  ) return  new entangling_roots_t( this, options_str );
    if ( name == "invigorating_roar" ) return new invigorating_roar_t( this, options_str );
    if ( name == "moonkin_form"      ) return      new moonkin_form_t( this, options_str );
    if ( name == "rejuvenation"      ) return      new rejuvenation_t( this, options_str );
    if ( name == "thrash"            ) return            new thrash_t( this, options_str );
    if ( name == "wrath"             ) return             new wrath_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

struct summon_loyal_druid_t : public spell_t
{
  pet_t* pet;
  timespan_t duration;

  summon_loyal_druid_t( player_t* p, const spell_data_t* s ) :
    spell_t( s -> name_cstr(), p, s ), duration( s -> duration() )
  {
    background = true;
    may_miss = may_crit = callbacks = harmful = false;
    pet = new loyal_druid_pet_t( p );
  }

  void execute() override
  {
    spell_t::execute();

    pet -> summon( duration );
  }
};

void enchants::mark_of_the_loyal_druid( special_effect_t& effect )
{
  const spell_data_t* summon_spell = effect.driver() -> effectN( 1 ).trigger();

  action_t* action = effect.player -> find_action( summon_spell -> name_cstr() );
  if ( ! action )
  {
    action = effect.player -> create_proc_action( summon_spell -> name_cstr(), effect );
  }

  if ( ! action )
  {
    action = new summon_loyal_druid_t( effect.player, summon_spell );
  }

  effect.execute_action = action;
  effect.type = SPECIAL_EFFECT_EQUIP;

  new dbc_proc_callback_t( effect.item, effect );
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

// Custom buff to model a 5 seconds of climbing + 5 seconds of full stacks + 5 seconds of decreasing
struct down_draft_t : public stat_buff_t
{
  struct dd_event_t : public event_t
  {
    down_draft_t* buff;

    dd_event_t( down_draft_t* b, const timespan_t& d ) :
      event_t( *b -> player ), buff( b )
    {
      add_event( d );
    }

    void execute() override
    {
      if ( sim().debug )
      {
        sim().out_debug.printf( "%s buff %s swap to decreasing",
            buff -> player -> name(), buff -> name() );
      }
      buff -> reverse = true;
      buff -> event = nullptr;
    }
  };

  event_t* event;

  down_draft_t( const special_effect_t& effect ) :
    stat_buff_t( stat_buff_creator_t( effect.player, "down_draft", effect.player -> find_spell( 214342 ), effect.item )
      // Double the duration, in reality this should not be doubled but rather increased by
      // max_stack * period
      .duration( effect.player -> find_spell( 214342 ) -> duration() * 2 ) ), event( nullptr )
  { }

  void execute( int stacks = 1, double value = DEFAULT_VALUE(), timespan_t duration = timespan_t::min() )
  {
    stat_buff_t::execute( stacks, value, duration );

    if ( event )
    {
      event_t::cancel( event );
    }

    // Allow 5 seconds of full buff, and then start the decrease
    event = new ( *sim ) dd_event_t( this, timespan_t::from_seconds( 10.001 ) );
    reverse = false;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    stat_buff_t::expire_override( expiration_stacks, remaining_duration );

    event_t::cancel( event );
    reverse = false;
  }

  void reset() override
  {
    stat_buff_t::reset();
    event_t::cancel( event );
    reverse = false;
  }
};

void item::nightmare_egg_shell( special_effect_t& effect )
{
  effect.custom_buff = new down_draft_t( effect );

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

  buff_t* b = buff_creator_t( effect.player, "devilsaurs_stampede", effect.driver() -> effectN( 1 ).trigger(), effect.item )
    .tick_zero( true )
    .tick_callback( [ stampede ]( buff_t*, int, const timespan_t& ) {
      stampede -> schedule_execute();
    } );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item, effect );
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
  action_t* slicing_maelstrom = effect.player -> find_action( "slicing_maelstrom" );
  if ( ! slicing_maelstrom )
  {
    slicing_maelstrom = effect.player -> create_proc_action( "slicing_maelstrom", effect );
  }

  if ( ! slicing_maelstrom )
  {
    slicing_maelstrom = new slicing_maelstrom_t( effect );
  }

  effect.custom_buff = buff_creator_t( effect.player, "slicing_maelstrom", effect.driver(), effect.item )
    .tick_zero( true )
    .tick_callback( [ slicing_maelstrom ]( buff_t*, int, const timespan_t& ) {
      slicing_maelstrom -> schedule_execute();
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

void unique_gear::register_special_effects_x7()
{
  /* Legion 7.0 */
  register_special_effect( 214829, item::chaos_talisman                 );
  register_special_effect( 215670, item::figurehead_of_the_naglfar      );
  register_special_effect( 214971, item::giant_ornamental_pearl         );
  register_special_effect( 215956, item::horn_of_valor                  );
  register_special_effect( 224059, item::impact_tremor                  );
  register_special_effect( 214798, item::memento_of_angerboda           );
  register_special_effect( 214340, item::nightmare_egg_shell            );
  register_special_effect( 215467, item::obelisk_of_the_void            );
  register_special_effect( 214584, item::shivermaws_jawbone             );
  register_special_effect( 214168, item::spiked_counterweight           );
  register_special_effect( 215127, item::tiny_oozeling_in_a_jar         );
  register_special_effect( 215658, item::tirathons_betrayal             );
  register_special_effect( 214980, item::windscar_whetstone             );

  /* Legion Enchants */
  register_special_effect( 190888, enchants::mark_of_the_loyal_druid    );

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

