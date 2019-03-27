#include "simulationcraft.hpp"
#include "sc_paladin.hpp"

namespace paladin {

namespace buffs {
  crusade_buff_t::crusade_buff_t( player_t* p ) :
      buff_t( p, "crusade", p -> find_spell( 231895 ) ),
      damage_modifier( 0.0 ),
      haste_bonus( 0.0 )
  {
    set_refresh_behavior( buff_refresh_behavior::DISABLED );
    // TODO(mserrano): fix this when Blizzard turns the spelldata back to sane
    //  values
    damage_modifier = data().effectN( 1 ).percent() / 10.0;
    haste_bonus = data().effectN( 3 ).percent() / 10.0;

    // increase duration if we have Light's Decree
    paladin_t* paladin = static_cast<paladin_t*>( p );
    if ( paladin -> azerite.lights_decree.ok() )
      buff_duration += paladin -> spells.lights_decree -> effectN( 2 ).time_value();


    // let the ability handle the cooldown
    cooldown -> duration = timespan_t::zero();

    // invalidate haste
    add_invalidate( CACHE_HASTE );
  }

  struct shield_of_vengeance_buff_t : public absorb_buff_t
  {
    shield_of_vengeance_buff_t( player_t* p ):
      absorb_buff_t( p, "shield_of_vengeance", p -> find_spell( 184662 ) )
    {
      cooldown -> duration = timespan_t::zero();
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

  struct inquisition_buff_t : public buff_t
  {
    inquisition_buff_t( paladin_t* p ) :
      buff_t( p, "inquisition", p -> find_talent_spell( "Inquisition" ) )
    {
      add_invalidate(CACHE_HASTE);
    }
  };
}

struct holy_power_consumer_t : public paladin_melee_attack_t
{
  struct lights_decree_t : public paladin_spell_t {
    lights_decree_t( paladin_t* p, double parent_cost ) : paladin_spell_t( "lights_decree", p, p -> find_spell( 286229 ) )
    {
      base_dd_min = base_dd_max = p -> azerite.lights_decree.value() * parent_cost;
      aoe = -1;
      may_crit = true;

      // TODO(mserrano): why is this not in spelldata?
      school = SCHOOL_HOLY;
      ret_crusade = avenging_wrath = ret_damage_increase = ret_mastery_direct = ret_execution_sentence = ret_inquisition = true;
    }
  };

  lights_decree_t* lights_decree;
  bool skip_dp = false;

  holy_power_consumer_t( const std::string& n, paladin_t* p,
                          const spell_data_t* s = spell_data_t::nil() ) :
    paladin_melee_attack_t( n, p, s ),
    lights_decree( new lights_decree_t( p, base_cost() ) )
  {
  }

  virtual void execute() override
  {
    double c = cost();
    paladin_melee_attack_t::execute();

    if ( c <= 0.0 )
    {
      if ( !skip_dp )
      {
        if ( p() -> buffs.divine_purpose -> check() )
        {
          p() -> buffs.divine_purpose -> expire();
        }
      }
    }
    else
    {
      if ( p() -> buffs.the_fires_of_justice -> check() )
        p() -> buffs.the_fires_of_justice -> expire();
    }

    if ( p() -> talents.divine_purpose -> ok() &&
         rng().roll( p() -> spells.divine_purpose_ret -> effectN( 1 ).percent() ) )
    {
      p() -> buffs.divine_purpose -> trigger();
      p() -> procs.divine_purpose -> occur();
    }

    if ( p() -> buffs.crusade -> check() )
    {
      int num_stacks = (int)base_cost();
      p() -> buffs.crusade -> trigger( num_stacks );
    }

    if ( p() -> azerite.relentless_inquisitor.ok() )
    {
      int num_stacks = (int)base_cost();
      p() -> buffs.relentless_inquisitor -> trigger( num_stacks );
    }

    if ( p() -> azerite.lights_decree.ok() )
    {
      if ( p() -> buffs.avenging_wrath -> up() || p() -> buffs.crusade -> up() )
      {
        // TODO(mserrano): this is a hack - do we need this?
        lights_decree -> set_target( p() -> target );
        lights_decree -> execute();
      }
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );
    if ( result_is_hit( s -> result ) )
    {
      paladin_td_t* td = this -> td( s -> target );
      if ( td -> buffs.debuffs_judgment -> up() )
        td -> buffs.debuffs_judgment -> expire();
    }
  }

  virtual double cost() const override
  {
    double base_cost = paladin_melee_attack_t::cost();

    int discounts = 0;
    if ( p() -> buffs.the_fires_of_justice -> up() && base_cost > discounts )
      discounts++;
    return base_cost - discounts;
  }
};

// Crusade
struct crusade_t : public paladin_heal_t
{
  crusade_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "crusade", p, p -> talents.crusade )
  {
    parse_options( options_str );

    if ( ! ( p -> talents.crusade -> ok() ) )
      background = true;

    cooldown -> charges += as<int>( p -> sets -> set( PALADIN_RETRIBUTION, T18, B2 ) -> effectN( 1 ).base_value() );
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

    if ( p() -> azerite.avengers_might.ok() )
      p() -> buffs.avengers_might -> trigger( 1, p() -> buffs.avengers_might -> default_value, -1.0, p() -> buffs.crusade -> buff_duration );
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

struct execution_sentence_t : public holy_power_consumer_t
{
  execution_sentence_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "execution_sentence", p, p -> find_talent_spell( "Execution Sentence" ) )
  {
    parse_options( options_str );

    // disable if not talented
    if ( ! ( p -> talents.execution_sentence -> ok() ) )
      background = true;

    // Spelldata doesn't seem to have this
    hasted_gcd = true;
  }

  virtual void impact( action_state_t* s) override
  {
    holy_power_consumer_t::impact( s );
    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> buffs.execution_sentence -> trigger();
    }
    if ( p() -> talents.divine_judgment -> ok() )
      p() -> buffs.divine_judgment -> trigger();
  }
};


// Blade of Justice =========================================================

struct blade_of_justice_t : public holy_power_generator_t
{
  struct expurgation_t : public paladin_spell_t {
    expurgation_t( paladin_t* p ) : paladin_spell_t( "expurgation", p, p -> find_spell( 273481 ) )
    {
      base_td = p -> azerite.expurgation.value();
      hasted_ticks = false;
      tick_may_crit = true;
    }
  };

  expurgation_t* expurgation;

  blade_of_justice_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "blade_of_justice", p, p -> find_class_spell( "Blade of Justice" ) ),
      expurgation( new expurgation_t( p ) )
  {
    parse_options( options_str );

    if ( p -> azerite.expurgation.ok() )
      add_child( expurgation );
  }

  virtual double action_multiplier() const override
  {
    double am = holy_power_generator_t::action_multiplier();
    if ( p() -> buffs.blade_of_wrath -> up() )
      am *= 1.0 + p() -> buffs.blade_of_wrath -> data().effectN( 1 ).percent();
    return am;
  }

  virtual void execute() override
  {
    holy_power_generator_t::execute();
    if ( p() -> buffs.blade_of_wrath -> up() )
      p() -> buffs.blade_of_wrath -> expire();
  }

  virtual void impact(action_state_t* state) override
  {
    holy_power_generator_t::impact( state );

    if ( state -> result == RESULT_CRIT )
    {
      if ( p() -> azerite.expurgation.ok() )
      {
        expurgation -> set_target( state -> target );
        expurgation -> execute();
      }
    }
  }
};

struct holy_power_consumer_impact_t : public paladin_melee_attack_t
{
  holy_power_consumer_impact_t( const std::string& n, paladin_t* p, const spell_data_t* s )
    : paladin_melee_attack_t( n, p, s ) {
    dual = background = true;
    may_miss = may_dodge = may_parry = false;
  }

  virtual double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();
    if ( p() -> buffs.divine_purpose -> up() )
      am *= 1.0 + p() -> buffs.divine_purpose -> data().effectN( 2 ).percent();
    return am;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_melee_attack_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent();
      m *= judgment_multiplier;
    }

    return m;
  }
};

// Divine Storm =============================================================

struct divine_storm_t: public holy_power_consumer_t
{
  struct divine_storm_damage_t : public holy_power_consumer_impact_t
  {
    divine_storm_damage_t( paladin_t* p )
      : holy_power_consumer_impact_t( "divine_storm_dmg", p, p -> find_spell( 224239 ) )
    {}

    double bonus_da( const action_state_t* s ) const override
    {
      double b = holy_power_consumer_impact_t::bonus_da( s );
      b += p() -> buffs.empyrean_power -> value();
      return b;
    }
  };

  divine_storm_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "divine_storm", p, p -> find_class_spell( "Divine Storm" ) )
  {
    parse_options( options_str );

    may_block = false;
    impact_action = new divine_storm_damage_t( p );
    impact_action -> stats = stats;

    weapon = &( p -> main_hand_weapon );

    aoe = -1;

    ret_damage_increase = ret_mastery_direct = ret_execution_sentence = ret_inquisition = true;

    // TODO: Okay, when did this get reset to 1?
    weapon_multiplier = 0;
  }

  virtual void execute() override
  {
    double c = cost();
    if (  p() -> buffs.empyrean_power -> up() )
      skip_dp = true;
    holy_power_consumer_t::execute();
    skip_dp = false;
    if ( p() -> buffs.the_fires_of_justice -> up() && c > 0 )
      p() -> buffs.the_fires_of_justice -> expire();
    if ( p() -> buffs.empyrean_power -> up() )
      p() -> buffs.empyrean_power -> expire();
  }

  virtual void impact(action_state_t* state) override
  {
    holy_power_consumer_t::impact(state);
    if ( p() -> talents.divine_judgment -> ok() )
      p() -> buffs.divine_judgment -> trigger();
  }

  virtual double cost() const
  {
    if ( p() -> buffs.empyrean_power -> up() )
      return 0;
    return holy_power_consumer_t::cost();
  }

  void record_data( action_state_t* ) override {}
};

struct templars_verdict_t : public holy_power_consumer_t
{
  struct templars_verdict_damage_t : public holy_power_consumer_impact_t
  {
    templars_verdict_damage_t( paladin_t *p )
      : holy_power_consumer_impact_t( "templars_verdict_dmg", p, p -> find_spell( 224266 ) )
    {}

    virtual double action_multiplier() const override
    {
      double am = holy_power_consumer_impact_t::action_multiplier();
      if ( p() -> buffs.righteous_verdict -> check() )
        am *= 1.0 + p() -> buffs.righteous_verdict -> data().effectN( 1 ).percent();
      return am;
    }
  };

  templars_verdict_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "templars_verdict", p, p -> find_specialization_spell( "Templar's Verdict" ) )
  {
    parse_options( options_str );

    may_block = false;
    impact_action = new templars_verdict_damage_t( p );
    impact_action -> stats = stats;

    ret_damage_increase = ret_mastery_direct = ret_execution_sentence = ret_inquisition = true;

    // Okay, when did this get reset to 1?
    weapon_multiplier = 0;
  }

  void record_data( action_state_t* ) override {}

  virtual void impact( action_state_t* s ) override
  {
    holy_power_consumer_t::impact( s );
    if ( p() -> talents.divine_judgment -> ok() )
      p() -> buffs.divine_judgment -> trigger();
  }

  virtual void execute() override
  {
    // store cost for potential refunding (see below)
    double c = cost();

    holy_power_consumer_t::execute();

    // TODO: do misses consume fires of justice?
    if ( p() -> buffs.the_fires_of_justice -> up() && c > 0 )
      p() -> buffs.the_fires_of_justice -> expire();

    // missed/dodged/parried TVs do not consume Holy Power
    // check for a miss, and refund the appropriate amount of HP if we spent any
    if ( result_is_miss( execute_state -> result ) && c > 0 )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, c, p() -> gains.hp_templars_verdict_refund );
    }

    if ( p() -> talents.righteous_verdict -> ok() )
    {
      if ( p() -> buffs.righteous_verdict -> up() )
        p() -> buffs.righteous_verdict -> expire();
      p() -> buffs.righteous_verdict -> trigger();
    }
  }
};

// Judgment - Retribution =================================================================

struct judgment_ret_t : public judgment_t
{
  judgment_ret_t( paladin_t* p, const std::string& options_str )
    : judgment_t( p, options_str )
  {}

  virtual void execute() override
  {
    judgment_t::execute();

    if ( p() -> talents.zeal -> ok() )
    {
      p() -> buffs.zeal -> trigger( as<int>( p() -> talents.zeal -> effectN( 1 ).base_value() ) );
    }
    if ( p() -> talents.divine_judgment -> ok() )
    {
      p() -> buffs.divine_judgment -> expire();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = judgment_t::action_multiplier();
    if ( p() -> buffs.divine_judgment -> up() )
      am *= 1.0 + p() -> buffs.divine_judgment -> data().effectN( 1 ).percent() * p() -> buffs.divine_judgment -> stack();
    return am;
  }

  // Special things that happen when Judgment damages target
  void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> buffs.debuffs_judgment -> trigger();

      p() -> resource_gain( RESOURCE_HOLY_POWER, 1, p() -> gains.judgment );
    }

    judgment_t::impact( s );
  }
};

// Justicar's Vengeance
struct justicars_vengeance_t : public holy_power_consumer_t
{
  justicars_vengeance_t( paladin_t* p, const std::string& options_str )
    : holy_power_consumer_t( "justicars_vengeance", p, p -> talents.justicars_vengeance )
  {
    parse_options( options_str );

    // Spelldata doesn't have this
    hasted_gcd = true;

    weapon_multiplier = 0; // why is this needed?
  }

  virtual void execute() override
  {
    // store cost for potential refunding (see below)
    double c = cost();

    holy_power_consumer_t::execute();

    // TODO: do misses consume fires of justice?
    if ( p() -> buffs.the_fires_of_justice -> up() && c > 0 )
      p() -> buffs.the_fires_of_justice -> expire();

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
    : paladin_spell_t( "shield_of_vengeance_proc", p, p -> find_spell( 184689 ) )
  {
    may_miss = may_dodge = may_parry = may_glance = false;
    background = true;
    split_aoe_damage = true;
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

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );

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
    base_dd_min = base_dd_max = p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent();
    paladin_absorb_t::execute();
    p() -> buffs.shield_of_vengeance -> trigger( 1, base_dd_max );
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
    // spell data doesn't have this
    hasted_gcd = true;
    may_crit = true;
    aoe = -1;
  }
};

struct inquisition_t : public paladin_heal_t
{
  inquisition_t( paladin_t* p, const std::string& options_str )
    : paladin_heal_t( "inquisition", p, p -> find_talent_spell( "Inquisition" ) )
  {
    parse_options( options_str );
    if ( ! ( p -> talents.inquisition -> ok() ) )
      background = true;
    // spelldata doesn't have this
    hasted_gcd = true;
  }

  virtual double cost() const override
  {
    double base_cost = std::max( base_costs[ RESOURCE_HOLY_POWER ], std::min( p() -> resources.current[ RESOURCE_HOLY_POWER ], 3.0 ) );
    if ( p() -> buffs.the_fires_of_justice -> up() )
      return base_cost - 1;
    return base_cost;
  }

  virtual void execute() override
  {
    double c = cost();
    paladin_heal_t::execute();
    if ( p() -> buffs.the_fires_of_justice -> up() )
    {
      p() -> buffs.the_fires_of_justice -> expire();
      c = std::min( c + 1, 3.0 );
    }
    p() -> buffs.inquisition -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, c * ( p() -> buffs.inquisition -> data().duration() ) );
  }
};

struct hammer_of_wrath_t : public holy_power_generator_t
{
  hammer_of_wrath_t( paladin_t* p, const std::string& options_str )
    : holy_power_generator_t( "hammer_of_wrath", p, p -> find_talent_spell( "Hammer of Wrath" ) )
  {
    parse_options( options_str );
    hasted_cd = hasted_gcd = true;
    ret_crusade = avenging_wrath = ret_inquisition = true;
  }

  virtual bool target_ready( player_t* candidate_target ) override
  {
    // TODO(mserrano): this is also probably incorrect
    if ( ! p() -> get_how_availability( candidate_target ) )
    {
      return false;
    }

    return holy_power_generator_t::target_ready( candidate_target );
  }
};

struct zeal_t : public paladin_melee_attack_t
{
  zeal_t( paladin_t* p ) : paladin_melee_attack_t( "zeal", p, p -> find_spell( 269937 ) )
  { background = true; }
};

// Initialization
action_t* paladin_t::create_action_retribution( const std::string& name, const std::string& options_str )
{
  if ( !active_zeal ) active_zeal = new zeal_t( this );

  if ( name == "blade_of_justice"          ) return new blade_of_justice_t         ( this, options_str );
  if ( name == "crusade"                   ) return new crusade_t                  ( this, options_str );
  if ( name == "divine_storm"              ) return new divine_storm_t             ( this, options_str );
  if ( name == "execution_sentence"        ) return new execution_sentence_t       ( this, options_str );
  if ( name == "hammer_of_wrath"           ) return new hammer_of_wrath_t          ( this, options_str );
  if ( name == "inquisition"               ) return new inquisition_t              ( this, options_str );
  if ( name == "templars_verdict"          ) return new templars_verdict_t         ( this, options_str );
  if ( name == "wake_of_ashes"             ) return new wake_of_ashes_t            ( this, options_str );
  if ( name == "justicars_vengeance"       ) return new justicars_vengeance_t      ( this, options_str );
  if ( name == "shield_of_vengeance"       ) return new shield_of_vengeance_t      ( this, options_str );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    if ( name == "judgment") return new judgment_ret_t( this, options_str );
  }

  return nullptr;
}

void paladin_t::create_buffs_retribution()
{
  buffs.crusade                = new buffs::crusade_buff_t( this );

  buffs.zeal                           = make_buff( this, "zeal", find_spell( 269571 ) ) -> add_invalidate( CACHE_ATTACK_SPEED );
  buffs.righteous_verdict              = make_buff( this, "righteous_verdict", find_spell( 267611 ) );
  buffs.inquisition                    = new buffs::inquisition_buff_t( this );
  buffs.the_fires_of_justice           = make_buff( this, "fires_of_justice", find_spell( 209785 ) );
  buffs.blade_of_wrath                 = make_buff( this, "blade_of_wrath", find_spell( 281178 ) );
  buffs.shield_of_vengeance            = new buffs::shield_of_vengeance_buff_t( this );
  buffs.divine_judgment                = make_buff( this, "divine_judgment", find_spell( 271581 ) );

  // Azerite
  buffs.relentless_inquisitor = make_buff<stat_buff_t>(this, "relentless_inquisitor", find_spell( 279204 ))
                                  -> add_stat( STAT_HASTE_RATING, azerite.relentless_inquisitor.value() );
  buffs.empyrean_power = make_buff( this, "empyrean_power", find_spell( 286393 ))
                            -> set_default_value( azerite.empyrean_power.value() );
}

void paladin_t::init_rng_retribution()
{
  art_of_war_rppm = get_rppm( "art_of_war", find_spell( 267344 ) );
  if ( !( talents.blade_of_wrath -> ok() ) )
  {
    art_of_war_rppm -> set_modifier(
      art_of_war_rppm -> get_modifier() / ( 1.0 + spells.blade_of_wrath -> effectN( 1 ).percent() ) );
  }
}

void paladin_t::init_spells_retribution()
{
  // talents
  talents.zeal                       = find_talent_spell( "Zeal" );
  talents.righteous_verdict          = find_talent_spell( "Righteous Verdict" );
  talents.execution_sentence         = find_talent_spell( "Execution Sentence" );
  talents.fires_of_justice           = find_talent_spell( "Fires of Justice" );
  talents.blade_of_wrath             = find_talent_spell( "Blade of Wrath" );
  talents.hammer_of_wrath            = find_talent_spell( "Hammer of Wrath" );
  talents.fist_of_justice            = find_talent_spell( "Fist of Justice" );
  talents.repentance                 = find_talent_spell( "Repentance" );
  talents.blinding_light             = find_talent_spell( "Blinding Light" );
  talents.divine_judgment            = find_talent_spell( "Divine Judgment" );
  talents.consecration               = find_talent_spell( "Consecration" );
  talents.wake_of_ashes              = find_talent_spell( "Wake of Ashes" );
  talents.justicars_vengeance        = find_talent_spell( "Justicar's Vengeance" );
  talents.eye_for_an_eye             = find_talent_spell( "Eye for an Eye" );
  talents.word_of_glory              = find_talent_spell( "Word of Glory" );
  talents.divine_purpose             = find_talent_spell( "Divine Purpose" ); // TODO: fix this
  talents.crusade                    = find_talent_spell( "Crusade" );
  talents.inquisition                = find_talent_spell( "Inquisition" );

  // misc spells
  spells.divine_purpose_ret            = find_spell( 223817 );
  spells.blade_of_wrath                = find_spell( 231832 );

  // Mastery
  passives.hand_of_light             = find_mastery_spell( PALADIN_RETRIBUTION );
  passives.execution_sentence        = find_spell( 267799 );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    spec.judgment_2 = find_specialization_spell( 231663 );
  }

  // azerite stuff
  azerite.avengers_might        = find_azerite_spell( 125 );
  azerite.expurgation           = find_azerite_spell( 187 );
  azerite.grace_of_the_justicar = find_azerite_spell( 393 );
  azerite.relentless_inquisitor = find_azerite_spell( 154 );
  azerite.empyrean_power        = find_azerite_spell( 453 );
  azerite.lights_decree         = find_azerite_spell( 396 );
  spells.lights_decree          = find_spell( 286231 );
}

void empyrean_power( special_effect_t& effect )
{
  azerite_power_t power = effect.player -> find_azerite_spell( effect.driver() -> name_cstr() );
  if ( !power.enabled() )
    return;

  const spell_data_t* driver = effect.player -> find_spell( 286392 );

  buff_t* buff = buff_t::find( effect.player, "empyrean_power" );

  effect.custom_buff = buff;
  effect.spell_id = driver -> id();

  struct empyrean_power_cb_t : public dbc_proc_callback_t
  {
    empyrean_power_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      proc_buff -> trigger();
    }
  };

  new empyrean_power_cb_t( effect );
}

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

  precombat -> add_action( "arcane_torrent,if=!talent.wake_of_ashes.enabled" );

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
  def -> add_action( "call_action_list,name=opener" );
  def -> add_action( "call_action_list,name=cooldowns" );
  def -> add_action( "call_action_list,name=generators" );

  // Items
  bool has_knot = false;
  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      if ( items[i].name_str == "knot_of_ancient_fury" )
      {
        has_knot = true;
        continue;
      }

      std::string item_str;
      if ( items[i].name_str == "razdunks_big_red_button" )
      {
        item_str = "use_item,name=" + items[i].name_str;
      }
      else if ( items[i].name_str == "jes_howler" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10";
      }
      else if ( items[i].name_str == "vial_of_animated_blood" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<18)|(cooldown.avenging_wrath.remains>30|cooldown.crusade.remains>30)";
      }
      else if ( items[i].name_str == "dooms_fury" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<18";
      }
      else if ( items[i].name_str == "galecallers_beak" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<15";
      }
      else if ( items[i].name_str == "bygone_bee_almanac" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up";
      }
      else if ( items[i].name_str == "merekthas_fang" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(!raid_event.adds.exists|raid_event.adds.in>15)|spell_targets.divine_storm>=2";
      }
      else if ( items[i].name_str == "plunderbeards_flask" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>45|!buff.crusade.up&cooldown.crusade.remains>45";
      }
      else if ( items[i].name_str == "berserkers_juju" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>45|!buff.crusade.up&cooldown.crusade.remains>45";
      }
      else if ( items[i].name_str == "endless_tincture_of_fractional_power" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>45|cooldown.crusade.remains>45";
      }
      else if ( items[i].name_str == "pearl_divers_compass" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10";
      }
      else if ( items[i].name_str == "first_mates_spyglass" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<=15";
      }
      else if ( items[i].name_str == "whirlwings_plumage" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<=20";
      }
      else if ( items[i].name_str == "dread_gladiators_badge" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<=20";
      }
      else if ( items[i].name_str == "dread_aspirants_medallion" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<=20";
      }
      else if ( items[i].name_str == "grongs_primal_rage" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=!buff.avenging_wrath.up&!buff.crusade.up";
      }
      else if ( items[i].name_str == "sinister_gladiators_badge" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<=15";
      }
      else if ( items[i].name_str == "sinister_gladiators_medallion" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<=20";
      }

      else if ( items[i].slot != SLOT_WAIST )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=(buff.avenging_wrath.up|buff.crusade.up)";
      }

      if ( items[i].slot != SLOT_WAIST )
      {
        cds -> add_action( item_str );
      }
    }
  }

  if ( has_knot )
  {
    cds -> add_action( "use_item,name=knot_of_ancient_fury,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack>=10|cooldown.avenging_wrath.remains>30|!buff.crusade.up&cooldown.crusade.remains>30" );
  }

  if ( sim -> allow_potions )
  {
    if ( true_level > 100 )
      cds -> add_action( "potion,if=buff.bloodlust.react|buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<25" );
    else if ( true_level >= 80 )
      cds -> add_action( "potion,if=buff.bloodlust.react|buff.avenging_wrath.up" );
  }

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {

    if (racial_actions[i] == "lights_judgment" )
    {
      cds -> add_action( "lights_judgment,if=spell_targets.lights_judgment>=2|(!raid_event.adds.exists|raid_event.adds.in>75)" );
    }

    if ( racial_actions[i] == "fireblood" )
    {
      cds -> add_action( "fireblood,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10" );
    }

    /*
    TODO(mserrano): work out if the other racials are worth using
    else
    {
      opener -> add_action( racial_actions[ i ] );
      cds -> add_action( racial_actions[ i ] );
    } */
  }
  cds -> add_action( this, "Shield of Vengeance" );
  cds -> add_action( this, "Avenging Wrath", "if=buff.inquisition.up|!talent.inquisition.enabled" );
  cds -> add_talent( this, "Crusade", "if=holy_power>=4" );

  opener -> add_action( "sequence,if=talent.wake_of_ashes.enabled&talent.crusade.enabled&talent.execution_sentence.enabled&!talent.hammer_of_wrath.enabled,name=wake_opener_ES_CS:shield_of_vengeance:blade_of_justice:judgment:crusade:templars_verdict:wake_of_ashes:templars_verdict:crusader_strike:execution_sentence" );
  opener -> add_action( "sequence,if=talent.wake_of_ashes.enabled&talent.crusade.enabled&!talent.execution_sentence.enabled&!talent.hammer_of_wrath.enabled,name=wake_opener_CS:shield_of_vengeance:blade_of_justice:judgment:crusade:templars_verdict:wake_of_ashes:templars_verdict:crusader_strike:templars_verdict" );
  opener -> add_action( "sequence,if=talent.wake_of_ashes.enabled&talent.crusade.enabled&talent.execution_sentence.enabled&talent.hammer_of_wrath.enabled,name=wake_opener_ES_HoW:shield_of_vengeance:blade_of_justice:judgment:crusade:templars_verdict:wake_of_ashes:templars_verdict:hammer_of_wrath:execution_sentence" );
  opener -> add_action( "sequence,if=talent.wake_of_ashes.enabled&talent.crusade.enabled&!talent.execution_sentence.enabled&talent.hammer_of_wrath.enabled,name=wake_opener_HoW:shield_of_vengeance:blade_of_justice:judgment:crusade:templars_verdict:wake_of_ashes:templars_verdict:hammer_of_wrath:templars_verdict" );
  opener -> add_action( "sequence,if=talent.wake_of_ashes.enabled&talent.inquisition.enabled,name=wake_opener_Inq:shield_of_vengeance:blade_of_justice:judgment:inquisition:avenging_wrath:wake_of_ashes" );

  finishers -> add_action( "variable,name=ds_castable,value=spell_targets.divine_storm>=2&!talent.righteous_verdict.enabled|spell_targets.divine_storm>=3&talent.righteous_verdict.enabled" );
  finishers -> add_talent( this, "Inquisition", "if=buff.inquisition.down|buff.inquisition.remains<5&holy_power>=3|talent.execution_sentence.enabled&cooldown.execution_sentence.remains<10&buff.inquisition.remains<15|cooldown.avenging_wrath.remains<15&buff.inquisition.remains<20&holy_power>=3" );
  finishers -> add_talent( this, "Execution Sentence", "if=spell_targets.divine_storm<=2&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*2)" );
  finishers -> add_action( this, "Divine Storm", "if=variable.ds_castable&buff.divine_purpose.react" );
  finishers -> add_action( this, "Divine Storm", "if=variable.ds_castable&(!talent.crusade.enabled|cooldown.crusade.remains>gcd*2)|buff.empyrean_power.up&debuff.judgment.down&buff.divine_purpose.down" );
  finishers -> add_action( this, "Templar's Verdict", "if=buff.divine_purpose.react" );
  finishers -> add_action( this, "Templar's Verdict", "if=(!talent.crusade.enabled|cooldown.crusade.remains>gcd*3)&(!talent.execution_sentence.enabled|buff.crusade.up&buff.crusade.stack<10|cooldown.execution_sentence.remains>gcd*2)" );

  generators -> add_action( "variable,name=HoW,value=(!talent.hammer_of_wrath.enabled|target.health.pct>=20&(buff.avenging_wrath.down|buff.crusade.down))" );
  generators -> add_action( "call_action_list,name=finishers,if=holy_power>=5" );
  generators -> add_talent( this, "Wake of Ashes", "if=(!raid_event.adds.exists|raid_event.adds.in>15|spell_targets.wake_of_ashes>=2)&(holy_power<=0|holy_power=1&cooldown.blade_of_justice.remains>gcd)" );
  generators -> add_action( this, "Blade of Justice", "if=holy_power<=2|(holy_power=3&(cooldown.hammer_of_wrath.remains>gcd*2|variable.HoW))" );
  generators -> add_action( this, "Judgment", "if=holy_power<=2|(holy_power<=4&(cooldown.blade_of_justice.remains>gcd*2|variable.HoW))" );
  generators -> add_talent( this, "Hammer of Wrath", "if=holy_power<=4" );
  generators -> add_talent( this, "Consecration", "if=holy_power<=2|holy_power<=3&cooldown.blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>gcd*2" );
  generators -> add_action( "call_action_list,name=finishers,if=talent.hammer_of_wrath.enabled&(target.health.pct<=20|buff.avenging_wrath.up|buff.crusade.up)" );
  generators -> add_action( this, "Crusader Strike", "if=cooldown.crusader_strike.charges_fractional>=1.75&(holy_power<=2|holy_power<=3&cooldown.blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>gcd*2&cooldown.consecration.remains>gcd*2)" );
  generators -> add_action( "call_action_list,name=finishers" );
  generators -> add_action( this, "Crusader Strike", "if=holy_power<=4" );
  generators -> add_action( "arcane_torrent,if=holy_power<=4" );
}

} // end namespace paladin
