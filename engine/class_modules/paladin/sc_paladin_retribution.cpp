#include "simulationcraft.hpp"
#include "sc_paladin.hpp"

namespace paladin {

namespace buffs {
  crusade_buff_t::crusade_buff_t( player_t* p ) :
      haste_buff_t( haste_buff_creator_t( p, "crusade", p -> find_spell( 231895 ) )
        .refresh_behavior( BUFF_REFRESH_DISABLED ) ),
      damage_modifier( 0.0 ),
      healing_modifier( 0.0 ),
      haste_bonus( 0.0 )
  {
    // TODO(mserrano): fix this when Blizzard turns the spelldata back to sane
    //  values
    damage_modifier = data().effectN( 1 ).percent() / 10.0;
    haste_bonus = data().effectN( 3 ).percent() / 10.0;
    healing_modifier = 0;

    // let the ability handle the cooldown
    cooldown -> duration = timespan_t::zero();

    // invalidate Damage and Healing for both specs
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_HASTE );
  }

  void crusade_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    paladin_t* p = static_cast<paladin_t*>( player );
    p -> buffs.liadrins_fury_unleashed -> expire(); // Force Liadrin's Fury to fade
  }

  struct shield_of_vengeance_buff_t : public absorb_buff_t
  {
    shield_of_vengeance_buff_t( player_t* p ):
      absorb_buff_t( p, "shield_of_vengeance", p -> find_spell( 184662 ) )
    {
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      absorb_buff_t::expire_override( expiration_stacks, remaining_duration );

      paladin_t* p = static_cast<paladin_t*>( player );
      // do thing
      if ( p -> fake_sov )
      {
        // TODO(mserrano): This is a horrible hack
        p -> active_shield_of_vengeance_proc -> base_dd_max = p -> active_shield_of_vengeance_proc -> base_dd_min = current_value;
        p -> active_shield_of_vengeance_proc -> execute();
      }
    }
  };
}

holy_power_consumer_t::holy_power_consumer_t( const std::string& n, paladin_t* p,
                                              const spell_data_t* s ) : paladin_melee_attack_t( n, p, s )
{
  if ( p -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T19, B2 ) )
  {
    base_multiplier *= 1.0 + p -> sets -> set( PALADIN_RETRIBUTION, T19, B2 ) -> effectN( 1 ).percent();
  }
}

double holy_power_consumer_t::composite_target_multiplier( player_t* t ) const
{
  double m = paladin_melee_attack_t::composite_target_multiplier( t );

  paladin_td_t* td = this -> td( t );

  if ( td -> buffs.debuffs_judgment -> up() )
  {
    double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
    judgment_multiplier += p() -> passives.judgment -> effectN( 1 ).percent();
    m *= judgment_multiplier;
  }

  return m;
}

void holy_power_consumer_t::execute()
{
  double c = cost();
  paladin_melee_attack_t::execute();
  if ( c <= 0.0 )
  {
    if ( p() -> buffs.divine_purpose -> check() )
    {
      p() -> buffs.divine_purpose -> expire();
    }
  }

  if ( p() -> talents.divine_purpose -> ok() )
  {
    bool success = p() -> buffs.divine_purpose -> trigger( 1,
      p() -> buffs.divine_purpose -> default_value,
      p() -> spells.divine_purpose_ret -> proc_chance() );
    if ( success )
      p() -> procs.divine_purpose -> occur();
  }

  if ( p() -> buffs.crusade -> check() )
  {
    int num_stacks = (int)base_cost();
    p() -> buffs.crusade -> trigger( num_stacks );
  }
}

void holy_power_generator_t::execute()
{
  paladin_melee_attack_t::execute();

  if ( p() -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T19, B4 ) )
  {
    // for some reason this is the same spell as the talent
    // leftover nonsense from when this was Conviction?
    if ( p() -> rng().roll( p() -> sets -> set( PALADIN_RETRIBUTION, T19, B4 ) -> proc_chance() ) )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_t19_4p );
      p() -> procs.tfoj_set_bonus -> occur();
    }
  }
}

// Custom events
struct whisper_of_the_nathrezim_event_t : public event_t
{
  paladin_t* paladin;

  whisper_of_the_nathrezim_event_t( paladin_t* p, timespan_t delay ) :
    event_t( *p, delay ), paladin( p )
  {
  }

  const char* name() const override
  { return "whisper_of_the_nathrezim_delay"; }

  void execute() override
  {
    paladin -> buffs.whisper_of_the_nathrezim -> trigger();
  }
};

struct scarlet_inquisitors_expurgation_expiry_event_t : public event_t
{
  paladin_t* paladin;

  scarlet_inquisitors_expurgation_expiry_event_t( paladin_t* p, timespan_t delay ) :
    event_t( *p, delay ), paladin( p )
  {
  }

  const char* name() const override
  { return "scarlet_inquisitors_expurgation_expiry_delay"; }

  void execute() override
  {
    paladin -> buffs.scarlet_inquisitors_expurgation -> expire();
  }
};

struct echoed_spell_event_t : public event_t
{
  paladin_melee_attack_t* echo;
  paladin_t* paladin;

  echoed_spell_event_t( paladin_t* p, paladin_melee_attack_t* spell, timespan_t delay ) :
    event_t( *p, delay ), echo( spell ), paladin( p )
  {
  }

  const char* name() const override
  { return "echoed_spell_delay"; }

  void execute() override
  {
    echo -> schedule_execute();
  }
};

// Crusade
struct crusade_t : public paladin_heal_t
{
  crusade_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "crusade", p, p -> talents.crusade )
  {
    parse_options( options_str );

    if ( ! ( p -> talents.crusade_talent -> ok() ) )
      background = true;

    cooldown -> charges += p -> sets -> set( PALADIN_RETRIBUTION, T18, B2 ) -> effectN( 1 ).base_value();
  }

  void tick( dot_t* d ) override
  {
    // override for this just in case Avenging Wrath were to get canceled or removed
    // early, or if there's a duration mismatch (unlikely, but...)
    if ( p() -> buffs.crusade -> check() )
    {
      // call tick()
      heal_t::tick( d );
    }
  }

  void execute() override
  {
    paladin_heal_t::execute();

    p() -> buffs.crusade -> trigger();

    if ( p() -> liadrins_fury_unleashed )
    {
      p() -> buffs.liadrins_fury_unleashed -> trigger();
    }
  }

  bool ready() override
  {
    if ( p() -> buffs.crusade -> check() )
      return false;
    else
      return paladin_heal_t::ready();
  }
};

// Execution Sentence =======================================================

struct execution_sentence_t : public paladin_spell_t
{
  execution_sentence_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "execution_sentence", p, p -> find_talent_spell( "Execution Sentence" ) )
  {
    parse_options( options_str );
    hasted_ticks   = true;
    travel_speed   = 0;
    tick_may_crit  = true;

    // disable if not talented
    if ( ! ( p -> talents.execution_sentence -> ok() ) )
      background = true;

    if ( p -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T19, B2 ) )
    {
      base_multiplier *= 1.0 + p -> sets -> set( PALADIN_RETRIBUTION, T19, B2 ) -> effectN( 2 ).percent();
    }
  }

  void init() override
  {
    paladin_spell_t::init();

    update_flags &= ~STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  virtual double cost() const override
  {
    double base_cost = paladin_spell_t::cost();
    int discounts = 0;
    if ( p() -> buffs.divine_purpose -> up() )
      discounts = base_cost;
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > discounts )
      discounts++;
    if ( p() -> buffs.ret_t21_4p -> up() && base_cost > discounts )
      discounts++;
    return base_cost - discounts;
  }

  void execute() override
  {
    double c = cost();
    paladin_spell_t::execute();

    if ( c <= 0.0 )
    {
      if ( p() -> buffs.divine_purpose -> check() )
      {
        p() -> buffs.divine_purpose -> expire();
      }
    }
    else {
      if ( p() -> buffs.the_fires_of_justice -> up() )
      {
        p() -> buffs.the_fires_of_justice -> expire();
      }
      if ( p() -> buffs.ret_t21_4p -> up() )
      {
        p() -> buffs.ret_t21_4p -> expire();
      }
    }

    if ( p() -> buffs.crusade -> check() )
    {
      int num_stacks = (int)base_cost();
      p() -> buffs.crusade -> trigger( num_stacks );
    }

    if ( p() -> talents.divine_purpose -> ok() )
    {
      bool success = p() -> buffs.divine_purpose -> trigger( 1,
        p() -> buffs.divine_purpose -> default_value,
        p() -> spells.divine_purpose_ret -> proc_chance() );
      if ( success )
        p() -> procs.divine_purpose -> occur();
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_spell_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent() + p() -> get_divine_judgment();
      judgment_multiplier += p() -> passives.judgment -> effectN( 1 ).percent();
      m *= judgment_multiplier;
    }

    return m;
  }
};


// Zeal ==========================================================

struct zeal_t : public holy_power_generator_t
{
  zeal_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "zeal", p, p -> find_talent_spell( "Zeal" ) )
  {
    parse_options( options_str );

    chain_multiplier = data().effectN( 1 ).chain_multiplier();

    // TODO: figure out wtf happened to this spell data
    hasted_cd = hasted_gcd = true;
  }

  int n_targets() const override
  {
    if ( p() -> buffs.zeal -> stack() )
      return 1 + p() -> buffs.zeal -> stack();
    return holy_power_generator_t::n_targets();
  }

  void execute() override
  {
    holy_power_generator_t::execute();

    p() -> buffs.zeal -> trigger();
  }
};

// Blade of Justice =========================================================

struct blade_of_justice_t : public holy_power_generator_t
{
  blade_of_justice_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "blade_of_justice", p, p -> find_class_spell( "Blade of Justice" ) )
  {
    parse_options( options_str );

    // Guarded by the Light and Sword of Light reduce base mana cost; spec-limited so only one will ever be active
    base_costs[ RESOURCE_MANA ] *= 1.0 +  p -> passives.guarded_by_the_light -> effectN( 5 ).percent();
    base_costs[ RESOURCE_MANA ] = floor( base_costs[ RESOURCE_MANA ] + 0.5 );

    background = ( p -> talents.divine_hammer -> ok() );

    if ( p -> talents.virtues_blade -> ok() )
      crit_bonus_multiplier += p -> talents.virtues_blade -> effectN( 1 ).percent();
  }

  virtual double action_multiplier() const override
  {
    double am = holy_power_generator_t::action_multiplier();
    if ( p() -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T20, B2 ) )
      if ( p() -> buffs.sacred_judgment -> up() )
        am *= 1.0 + p() -> buffs.sacred_judgment -> data().effectN( 1 ).percent();
    return am;
  }

  virtual void execute() override
  {
    holy_power_generator_t::execute();
    if ( p() -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T20, B4 ) )
      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_t20_2p );
  }
};

// Divine Hammer =========================================================

struct divine_hammer_tick_t : public paladin_melee_attack_t
{

  divine_hammer_tick_t( paladin_t* p )
    : paladin_melee_attack_t( "divine_hammer_tick", p, p -> find_spell( 198137 ) )
  {
    aoe         = -1;
    dual        = true;
    direct_tick = true;
    background  = true;
    may_crit    = true;
    ground_aoe = true;
  }
};

struct divine_hammer_t : public paladin_spell_t
{

  divine_hammer_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "divine_hammer", p, p -> find_talent_spell( "Divine Hammer" ) )
  {
    parse_options( options_str );

    hasted_ticks   = true;
    may_miss       = false;
    tick_may_crit  = true;
    tick_zero      = true;
    energize_type      = ENERGIZE_ON_CAST;
    energize_resource  = RESOURCE_HOLY_POWER;
    energize_amount    = data().effectN( 2 ).base_value();

    // TODO: figure out wtf happened to this spell data
    hasted_cd = hasted_gcd = true;

    tick_action = new divine_hammer_tick_t( p );
  }

  void init() override
  {
    paladin_spell_t::init();

    update_flags &= ~STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  virtual double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double am = paladin_spell_t::composite_persistent_multiplier( s );
    if ( p() -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T20, B2 ) )
      if ( p() -> buffs.sacred_judgment -> up() )
        am *= 1.0 + p() -> buffs.sacred_judgment -> data().effectN( 1 ).percent();
    return am;
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();
    if ( p() -> sets -> has_set_bonus( PALADIN_RETRIBUTION, T20, B4 ) )
      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.hp_t20_2p );
  }
};

// Divine Storm =============================================================

struct divine_storm_t: public holy_power_consumer_t
{
  struct divine_storm_damage_t : public paladin_melee_attack_t
  {
    divine_storm_damage_t( paladin_t* p )
      : paladin_melee_attack_t( "divine_storm_dmg", p, p -> find_spell( 224239 ) )
    {
      dual = background = true;
      may_miss = may_dodge = may_parry = false;
    }
  };

  divine_storm_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "divine_storm", p, p -> find_class_spell( "Divine Storm" ) )
  {
    parse_options( options_str );

    hasted_gcd = true;

    may_block = false;
    impact_action = new divine_storm_damage_t( p );
    impact_action -> stats = stats;

    weapon = &( p -> main_hand_weapon );

    if ( p -> talents.final_verdict -> ok() )
      base_multiplier *= 1.0 + p -> talents.final_verdict -> effectN( 2 ).percent();

    aoe = -1;

    ret_damage_increase = true;

    // TODO: Okay, when did this get reset to 1?
    weapon_multiplier = 0;
  }

  virtual double cost() const override
  {
    double base_cost = holy_power_consumer_t::cost();
    int discounts = 0;
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > discounts )
      discounts++;
    if ( p() -> buffs.ret_t21_4p -> up() && base_cost > discounts )
      discounts++;
    return base_cost - discounts;
  }

  virtual double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();
    if ( p() -> buffs.whisper_of_the_nathrezim -> check() )
      am *= 1.0 + p() -> buffs.whisper_of_the_nathrezim -> data().effectN( 1 ).percent();
    if ( p() -> buffs.scarlet_inquisitors_expurgation -> up() )
      am *= 1.0 + p() -> buffs.scarlet_inquisitors_expurgation -> check_stack_value();
    return am;
  }

  virtual void execute() override
  {
    double c = cost();
    holy_power_consumer_t::execute();

    if ( p() -> buffs.the_fires_of_justice -> up() && c > 0 )
      p() -> buffs.the_fires_of_justice -> expire();
    if ( p() -> buffs.ret_t21_4p -> up() && c > 0 )
      p() -> buffs.ret_t21_4p -> expire();

    if ( p() -> whisper_of_the_nathrezim )
    {
      if ( p() -> buffs.whisper_of_the_nathrezim -> up() )
        p() -> buffs.whisper_of_the_nathrezim -> expire();

      make_event<whisper_of_the_nathrezim_event_t>( *sim, p(), timespan_t::from_millis( 300 ) );
    }

    if ( p() -> scarlet_inquisitors_expurgation )
    {
      make_event<scarlet_inquisitors_expurgation_expiry_event_t>( *sim, p(), timespan_t::from_millis( 800 ) );
    }
  }

  void record_data( action_state_t* ) override {}
};

struct templars_verdict_t : public holy_power_consumer_t
{
  struct templars_verdict_damage_t : public paladin_melee_attack_t
  {
    templars_verdict_damage_t( paladin_t *p )
      : paladin_melee_attack_t( "templars_verdict_dmg", p, p -> find_spell( 224266 ) )
    {
      dual = background = true;
      may_miss = may_dodge = may_parry = false;
    }
  };

  templars_verdict_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "templars_verdict", p, p -> find_specialization_spell( "Templar's Verdict" ) )
  {
    parse_options( options_str );

    hasted_gcd = true;

    may_block = false;
    impact_action = new templars_verdict_damage_t( p );
    impact_action -> stats = stats;

    if ( p -> talents.final_verdict -> ok() )
      base_multiplier *= 1.0 + p -> talents.final_verdict -> effectN( 1 ).percent();

    ret_damage_increase = true;

    // Okay, when did this get reset to 1?
    weapon_multiplier = 0;
  }

  void record_data( action_state_t* ) override {}

  virtual double cost() const override
  {
    double base_cost = holy_power_consumer_t::cost();
    int discounts = 0;
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > discounts )
      discounts++;
    if ( p() -> buffs.ret_t21_4p -> up() && base_cost > discounts )
      discounts++;
    return base_cost - discounts;
  }

  virtual double action_multiplier() const override
  {
    double am = holy_power_consumer_t::action_multiplier();
    if ( p() -> buffs.whisper_of_the_nathrezim -> check() )
      am *= 1.0 + p() -> buffs.whisper_of_the_nathrezim -> data().effectN( 1 ).percent();
    return am;
  }

  virtual void execute() override
  {
    // store cost for potential refunding (see below)
    double c = cost();

    holy_power_consumer_t::execute();

    // TODO: do misses consume fires of justice?
    if ( p() -> buffs.the_fires_of_justice -> up() && c > 0 )
      p() -> buffs.the_fires_of_justice -> expire();
    if ( p() -> buffs.ret_t21_4p -> up() && c > 0 )
      p() -> buffs.ret_t21_4p -> expire();

    // missed/dodged/parried TVs do not consume Holy Power
    // check for a miss, and refund the appropriate amount of HP if we spent any
    if ( result_is_miss( execute_state -> result ) && c > 0 )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, c, p() -> gains.hp_templars_verdict_refund );
    }

    if ( p() -> whisper_of_the_nathrezim )
    {
      if ( p() -> buffs.whisper_of_the_nathrezim -> up() )
        p() -> buffs.whisper_of_the_nathrezim -> expire();
      make_event<whisper_of_the_nathrezim_event_t>( *sim, p(), timespan_t::from_millis( 300 ) );
    }
  }
};

// Justicar's Vengeance
struct justicars_vengeance_t : public holy_power_consumer_t
{
  justicars_vengeance_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "justicars_vengeance", p, p -> talents.justicars_vengeance )
  {
    parse_options( options_str );

    hasted_gcd = true;

    weapon_multiplier = 0; // why is this needed?
  }

  virtual double cost() const override
  {
    double base_cost = holy_power_consumer_t::cost();
    int discounts = 0;
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > discounts )
      discounts++;
    if ( p() -> buffs.ret_t21_4p -> up() && base_cost > discounts )
      discounts++;
    return base_cost - discounts;
  }

  virtual void execute() override
  {
    // store cost for potential refunding (see below)
    double c = cost();

    holy_power_consumer_t::execute();

    // TODO: do misses consume fires of justice?
    if ( p() -> buffs.the_fires_of_justice -> up() && c > 0 )
      p() -> buffs.the_fires_of_justice -> expire();

    if ( p() -> buffs.ret_t21_4p -> up() && c > 0 )
      p() -> buffs.ret_t21_4p -> expire();

    // missed/dodged/parried TVs do not consume Holy Power
    // check for a miss, and refund the appropriate amount of HP if we spent any
    if ( result_is_miss( execute_state -> result ) && c > 0 )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, c, p() -> gains.hp_templars_verdict_refund );
    }
  }
};

// SoV

struct shield_of_vengeance_proc_t : public paladin_spell_t
{
  shield_of_vengeance_proc_t( paladin_t* p )
    : paladin_spell_t( "shield_of_vengeance_proc", p, spell_data_t::nil() )
  {
    school = SCHOOL_HOLY;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    may_glance  = false;
    background  = true;
    trigger_gcd = timespan_t::zero();
    id = 184689;

    split_aoe_damage = true;
    may_crit = true;
    aoe = -1;
  }

  void init() override {
    paladin_spell_t::init();
    snapshot_flags = 0;
  }

  proc_types proc_type() const override
  {
    return PROC1_MELEE_ABILITY;
  }
};

struct shield_of_vengeance_t : public paladin_absorb_t
{
  shield_of_vengeance_t( paladin_t* p, const std::string& options_str ) :
    paladin_absorb_t( "shield_of_vengeance", p, p -> find_spell( 184662 ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    may_crit = true;
    attack_power_mod.direct = 20;

    if ( ! ( p -> active_shield_of_vengeance_proc ) )
    {
      p -> active_shield_of_vengeance_proc = new shield_of_vengeance_proc_t( p );
    }
  }

  void init() override
  {
    paladin_absorb_t::init();
    snapshot_flags |= (STATE_CRIT | STATE_VERSATILITY);
  }

  virtual void execute() override
  {
    paladin_absorb_t::execute();

    p() -> buffs.shield_of_vengeance -> trigger();
  }
};


// Wake of Ashes (Retribution) ================================================

struct wake_of_ashes_t : public paladin_spell_t
{
  wake_of_ashes_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "wake_of_ashes", p, p -> find_talent_spell( "Wake of Ashes" ) )
  {
    parse_options( options_str );
    if ( ! ( p -> talents.wake_of_ashes -> ok() ) )
      background = true;
  }
};

// Initialization
action_t* paladin_t::create_action_retribution( const std::string& name, const std::string& options_str )
{
  if ( name == "crusade"                   ) return new crusade_t                  ( this, options_str );
  if ( name == "zeal"                      ) return new zeal_t                     ( this, options_str );
  if ( name == "blade_of_justice"          ) return new blade_of_justice_t         ( this, options_str );
  if ( name == "divine_hammer"             ) return new divine_hammer_t            ( this, options_str );
  if ( name == "divine_storm"              ) return new divine_storm_t             ( this, options_str );
  if ( name == "execution_sentence"        ) return new execution_sentence_t       ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );
  if ( name == "wake_of_ashes"             ) return new wake_of_ashes_t            ( this, options_str );
  if ( name == "justicars_vengeance"       ) return new justicars_vengeance_t      ( this, options_str );
  if ( name == "shield_of_vengeance"       ) return new shield_of_vengeance_t      ( this, options_str );

  return nullptr;
}

void paladin_t::create_buffs_retribution()
{
  buffs.crusade                = new buffs::crusade_buff_t( this );

  buffs.zeal                           = make_buff( this, "zeal", find_spell( 217020 ) );
  buffs.the_fires_of_justice           = make_buff( this, "the_fires_of_justice", find_spell( 209785 ) );
  buffs.blade_of_wrath               = make_buff( this, "blade_of_wrath", find_spell( 231843 ) );
  buffs.whisper_of_the_nathrezim       = make_buff( this, "whisper_of_the_nathrezim", find_spell( 207635 ) );
  buffs.liadrins_fury_unleashed        = new buffs::liadrins_fury_unleashed_t( this );
  buffs.shield_of_vengeance            = new buffs::shield_of_vengeance_buff_t( this );
  buffs.sacred_judgment                = make_buff( this, "sacred_judgment", find_spell( 246973 ) );

  buffs.scarlet_inquisitors_expurgation = make_buff( this, "scarlet_inquisitors_expurgation", find_spell( 248289 ) )
    ->set_default_value( find_spell( 248289 ) -> effectN( 1 ).percent() );
  buffs.scarlet_inquisitors_expurgation_driver = make_buff( this, "scarlet_inquisitors_expurgation_driver", find_spell( 248103 ) )
    ->set_period( find_spell( 248103 ) -> effectN( 1 ).period() )
                                                 ->set_quiet( true )
                                                 ->set_tick_callback([this](buff_t*, int, const timespan_t&) { buffs.scarlet_inquisitors_expurgation -> trigger(); })
                                                 ->set_tick_time_behavior( BUFF_TICK_TIME_UNHASTED );

  buffs.last_defender = make_buff( this, "last_defender", talents.last_defender  )
    ->set_chance( talents.last_defender -> ok() )
    ->set_max_stack( 99 ) //Spell doesn't cite any limits, just has diminishing returns.
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Tier Bonuses
  buffs.ret_t21_4p             = buff_creator_t( this, "hidden_retribution_t21_4p", find_spell( 253806 ) );
}

void paladin_t::init_rng_retribution()
{
  blade_of_wrath_rppm = get_rppm( "blade_of_wrath", find_spell( 231832 ) );
}

void paladin_t::init_spells_retribution()
{
  // talents
  talents.final_verdict              = find_talent_spell( "Final Verdict" );
  talents.execution_sentence         = find_talent_spell( "Execution Sentence" );
  talents.consecration               = find_talent_spell( "Consecration" );
  talents.fires_of_justice           = find_talent_spell( "The Fires of Justice" );
  talents.greater_judgment           = find_talent_spell( "Greater Judgment" );
  talents.zeal                       = find_talent_spell( "Zeal" );
  talents.fist_of_justice            = find_talent_spell( "Fist of Justice" );
  talents.repentance                 = find_talent_spell( "Repentance" );
  talents.blinding_light             = find_talent_spell( "Blinding Light" );
  talents.virtues_blade              = find_talent_spell( "Virtue's Blade" );
  talents.blade_of_wrath             = find_talent_spell( "Blade of Wrath" );
  talents.divine_hammer              = find_talent_spell( "Divine Hammer" );
  talents.justicars_vengeance        = find_talent_spell( "Justicar's Vengeance" );
  talents.eye_for_an_eye             = find_talent_spell( "Eye for an Eye" );
  talents.word_of_glory              = find_talent_spell( "Word of Glory" );
  talents.divine_intervention        = find_talent_spell( "Divine Intervention" );
  talents.divine_steed               = find_talent_spell( "Divine Steed" );
  talents.divine_purpose             = find_talent_spell( "Divine Purpose" ); // TODO: fix this
  talents.crusade                    = find_spell( 231895 );
  talents.crusade_talent             = find_talent_spell( "Crusade" );
  talents.wake_of_ashes              = find_talent_spell( "Wake of Ashes" );

  // misc spells
  spells.divine_purpose_ret            = find_spell( 223817 );
  spells.liadrins_fury_unleashed       = find_spell( 208408 );
  spells.justice_gaze                  = find_spell( 211557 );
  spells.chain_of_thrayn               = find_spell( 206338 );
  spells.ashes_to_dust                 = find_spell( 236106 );
  spells.blessing_of_the_ashbringer    = find_spell( 242981 );

  // Mastery
  passives.divine_judgment             = find_mastery_spell( PALADIN_RETRIBUTION );

  // Spec aura
  spec.retribution_paladin = find_specialization_spell( "Retribution Paladin" );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    spec.judgment_2 = find_specialization_spell( 231661 );
    spec.judgment_3 = find_specialization_spell( 231663 );
  }
}

void paladin_t::init_assessors_retribution()
{ }

// Action Priority List Generation
void paladin_t::generate_action_prio_list_ret()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Raid consumables
  precombat -> add_action( "flask" );
  precombat -> add_action( "food" );
  precombat -> add_action( "augmentation" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-potting
  precombat -> add_action( "potion" );

  ///////////////////////
  // Action Priority List
  ///////////////////////

  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* opener = get_action_priority_list( "opener" );
  action_priority_list_t* cds = get_action_priority_list( "cooldowns" );
  action_priority_list_t* generators = get_action_priority_list( "generators" );
  action_priority_list_t* finishers = get_action_priority_list( "finishers" );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Rebuke" );
  def -> add_action( "call_action_list,name=opener,if=time<2" );
  def -> add_action( "call_action_list,name=cooldowns" );
  def -> add_action( "call_action_list,name=generators" );

  // Items
  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      std::string item_str;
      if ( items[i].name_str == "forgefiends_fabricator" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=equipped.144358&dot.wake_of_ashes.remains<gcd*2|(buff.crusade.up&buff.crusade.remains<gcd*2|buff.avenging_wrath.up&buff.avenging_wrath.remains<gcd*2)";
        cds -> add_action( item_str );
      }
      if ( items[i].name_str == "draught_of_souls" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=15|cooldown.crusade.remains>20&!buff.crusade.up)";
        cds -> add_action( item_str );
      }
      else if ( items[i].name_str == "might_of_krosus" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=15|cooldown.crusade.remains>5&!buff.crusade.up)";
        cds -> add_action( item_str );
      }
      else if ( items[i].name_str == "kiljaedens_burning_wish" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=!raid_event.adds.exists|raid_event.adds.in>35";
        cds -> add_action( item_str );
      }
      else if ( items[i].name_str == "specter_of_betrayal" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.crusade.up&buff.crusade.stack>=15|cooldown.crusade.remains>gcd*2)|(buff.avenging_wrath.up|cooldown.avenging_wrath.remains>gcd*2)";
        cds -> add_action( item_str );
      }
      else if ( items[i].name_str == "umbral_moonglaives" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=15)";
        cds -> add_action( item_str );
      }
      else if ( items[i].name_str == "vial_of_ceaseless_toxins" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=15)|(cooldown.crusade.remains>30&!buff.crusade.up|cooldown.avenging_wrath.remains>30)";
        cds -> add_action( item_str );
      }
      else if ( items[i].slot != SLOT_WAIST )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up)";
        cds -> add_action( item_str );
      }
    }
  }

  if ( sim -> allow_potions )
  {
    if ( true_level > 100 )
      cds -> add_action( "potion,if=(buff.bloodlust.react|buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<25|target.time_to_die<=40)" );
    else if ( true_level >= 80 )
      cds -> add_action( "potion,if=(buff.bloodlust.react|buff.avenging_wrath.up|target.time_to_die<=40)" );
  }

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {

    if ( racial_actions[i] == "arcane_torrent" )
    {
      opener -> add_action( "arcane_torrent,if=!set_bonus.tier20_2pc" );
      cds -> add_action( "arcane_torrent,if=(buff.crusade.up|buff.avenging_wrath.up)&holy_power=2&(cooldown.blade_of_justice.remains>gcd|cooldown.divine_hammer.remains>gcd)" );
    }
    else if (racial_actions[i] == "lights_judgment" )
    {
      cds -> add_action( "lights_judgment,if=spell_targets.lights_judgment>=2|(!raid_event.adds.exists|raid_event.adds.in>15)&cooldown.judgment.remains>gcd&(cooldown.divine_hammer.remains>gcd|cooldown.blade_of_justice.remains>gcd)&(buff.avenging_wrath.up|buff.crusade.stack>=15)" );
    }
    else
    {
      opener -> add_action( racial_actions[ i ] );
      cds -> add_action( racial_actions[ i ] );
    }
  }
  cds -> add_action( this, "Shield of Vengeance" );
  cds -> add_action( this, "Avenging Wrath" );
  cds -> add_talent( this, "Crusade", "if=holy_power>=3|((equipped.137048|race.blood_elf)&holy_power>=2)" );

  opener -> add_action( this, "Judgment" );
  opener -> add_action( this, "Blade of Justice", "if=equipped.137048|race.blood_elf|!cooldown.wake_of_ashes.up" );
  opener -> add_talent( this, "Divine Hammer", "if=equipped.137048|race.blood_elf|!cooldown.wake_of_ashes.up" );
  opener -> add_action( this, "Wake of Ashes" );

  finishers -> add_talent( this, "Execution Sentence", "if=spell_targets.divine_storm<=3&(cooldown.judgment.remains<gcd*4.25|debuff.judgment.remains>gcd*4.25)" );
  finishers -> add_action( this, "Divine Storm", "if=debuff.judgment.up&variable.ds_castable&buff.divine_purpose.react" );
  finishers -> add_action( this, "Divine Storm", "if=debuff.judgment.up&variable.ds_castable&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*2)" );
  finishers -> add_talent( this, "Justicar's Vengeance", "if=debuff.judgment.up&buff.divine_purpose.react&!equipped.137020&!talent.final_verdict.enabled" );
  finishers -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&buff.divine_purpose.react" );
  finishers -> add_action( this, "Templar's Verdict", "if=debuff.judgment.up&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*2)&(!talent.execution_sentence.enabled|cooldown.execution_sentence.remains>gcd)" );

  generators -> add_action( "variable,name=ds_castable,value=spell_targets.divine_storm>=2|(buff.scarlet_inquisitors_expurgation.stack>=29&(equipped.144358&(dot.wake_of_ashes.ticking&time>10|dot.wake_of_ashes.remains<gcd))|(buff.scarlet_inquisitors_expurgation.stack>=29&(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=15|cooldown.crusade.remains>15&!buff.crusade.up)|cooldown.avenging_wrath.remains>15)&!equipped.144358)" );
  generators -> add_action( this, "Judgment", "if=set_bonus.tier21_4pc" );
  generators -> add_action( "call_action_list,name=finishers,if=(buff.crusade.up&buff.crusade.stack<15|buff.liadrins_fury_unleashed.up)|(talent.wake_of_ashes.enabled&cooldown.wake_of_ashes.remains<gcd*2)" );
  generators -> add_action( "call_action_list,name=finishers,if=talent.execution_sentence.enabled&(cooldown.judgment.remains<gcd*4.25|debuff.judgment.remains>gcd*4.25)&cooldown.execution_sentence.up|buff.whisper_of_the_nathrezim.up&buff.whisper_of_the_nathrezim.remains<gcd*1.5" );
  generators -> add_action( this, "Judgment", "if=dot.execution_sentence.ticking&dot.execution_sentence.remains<gcd*2&debuff.judgment.remains<gcd*2" );
  generators -> add_action( this, "Blade of Justice", "if=holy_power<=2&(set_bonus.tier20_2pc|set_bonus.tier20_4pc)" );
  generators -> add_talent( this, "Divine Hammer", "if=holy_power<=2&(set_bonus.tier20_2pc|set_bonus.tier20_4pc)" );
  generators -> add_action( this, "Wake of Ashes", "if=(!raid_event.adds.exists|raid_event.adds.in>15)&(holy_power<=0|holy_power=1&(cooldown.blade_of_justice.remains>gcd|cooldown.divine_hammer.remains>gcd)|holy_power=2&((cooldown.zeal.charges_fractional<=0.65|cooldown.crusader_strike.charges_fractional<=0.65)))" );
  generators -> add_action( this, "Blade of Justice", "if=holy_power<=3&!set_bonus.tier20_4pc" );
  generators -> add_talent( this, "Divine Hammer", "if=holy_power<=3&!set_bonus.tier20_4pc" );
  generators -> add_action( this, "Judgment" );
  generators -> add_action( "call_action_list,name=finishers,if=buff.divine_purpose.up" );
  generators -> add_talent( this, "Zeal", "if=cooldown.zeal.charges_fractional>=1.65&holy_power<=4&(cooldown.blade_of_justice.remains>gcd*2|cooldown.divine_hammer.remains>gcd*2)&debuff.judgment.remains>gcd" );
  generators -> add_action( this, "Crusader Strike", "if=cooldown.crusader_strike.charges_fractional>=1.65&holy_power<=4&(cooldown.blade_of_justice.remains>gcd*2|cooldown.divine_hammer.remains>gcd*2)&debuff.judgment.remains>gcd&(talent.greater_judgment.enabled|!set_bonus.tier20_4pc&talent.the_fires_of_justice.enabled)" );
  generators -> add_talent( this, "Consecration" );
  generators -> add_action( this, "Hammer of Justice", "if=equipped.137065&target.health.pct>=75&holy_power<=4" );
  generators -> add_action( "call_action_list,name=finishers" );
  generators -> add_talent( this, "Zeal" );
  generators -> add_action( this, "Crusader Strike" );
}

} // end namespace paladin
