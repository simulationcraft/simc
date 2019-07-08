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
    cooldown -> duration = 0_ms;

    add_invalidate( CACHE_HASTE );
  }

  struct shield_of_vengeance_buff_t : public absorb_buff_t
  {
    shield_of_vengeance_buff_t( player_t* p ):
      absorb_buff_t( p, "shield_of_vengeance", p -> find_spell( 184662 ) )
    {
      cooldown -> duration = 0_ms;
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      absorb_buff_t::expire_override( expiration_stacks, remaining_duration );

      paladin_t* p = static_cast<paladin_t*>( player );
      // do thing
      if ( p -> fake_sov )
      {
        // TODO(mserrano): This is a horrible hack
        p -> active.shield_of_vengeance_damage -> base_dd_max = p -> active.shield_of_vengeance_damage -> base_dd_min = current_value;
        p -> active.shield_of_vengeance_damage -> execute();
      }
    }
  };
}

// Holy Power Consumer ======================================================

struct lights_decree_t : public paladin_spell_t
{
  int last_holy_power_cost;

  lights_decree_t( paladin_t* p ) :
    paladin_spell_t( "lights_decree", p, p -> find_spell( 286232 ) ),
    last_holy_power_cost( 0 )
  {
    base_dd_min = base_dd_max = p -> azerite.lights_decree.value();
    aoe = -1;
    background = may_crit = true;
  }

  double action_multiplier() const override
  {
    return paladin_spell_t::action_multiplier() * last_holy_power_cost;
  }
};

struct holy_power_consumer_t : public paladin_melee_attack_t
{
  bool is_divine_storm;
  holy_power_consumer_t( const std::string& n, paladin_t* p, const spell_data_t* s ) :
    paladin_melee_attack_t( n, p, s ),
    is_divine_storm ( false )
  { }

  double cost() const override
  {
    if ( ( is_divine_storm && p() -> buffs.empyrean_power -> check() ) ||
         p() -> buffs.divine_purpose -> check() )
    {
      return 0.0;
    }

    double c = paladin_melee_attack_t::cost();

    if ( p() -> buffs.fires_of_justice -> check() )
    {
      c += p() -> buffs.fires_of_justice -> data().effectN( 1 ).base_value();
    }

    return c;
  }

  virtual void execute() override
  {
    paladin_melee_attack_t::execute();

    // Crusade and Relentless Inquisitor gain full stacks from free spells, but reduced stacks with FoJ
    if ( p() -> buffs.crusade -> check() )
    {
      int num_stacks = as<int>( cost() == 0 ? base_costs[ RESOURCE_HOLY_POWER ] : cost() );
      p() -> buffs.crusade -> trigger( num_stacks );
    }

    if ( p() -> azerite.relentless_inquisitor.ok() )
    {
      int num_stacks = as<int>( cost() == 0 ? base_costs[ RESOURCE_HOLY_POWER ] : cost() );
      p() -> buffs.relentless_inquisitor -> trigger( num_stacks );
    }

    // Consume Empyrean Power on Divine Storm, handled here for interaction with DP/FoJ
    // Cost reduction is still in divine_storm_t
    if ( p() -> buffs.empyrean_power -> up() && is_divine_storm )
    {
      p() -> buffs.empyrean_power -> expire();
    }
    // Divine Purpose isn't consumed on DS if EP was consumed
    else if ( p() -> buffs.divine_purpose -> up() )
    {
      p() -> buffs.divine_purpose -> expire();
    }
    // FoJ isn't consumed if EP or DP were consumed
    else if ( p() -> buffs.fires_of_justice -> up() )
    {
      p() -> buffs.fires_of_justice -> expire();
    }

    // Roll for Divine Purpose
    if ( p() -> talents.divine_purpose -> ok() &&
         rng().roll( p() -> talents.divine_purpose -> effectN( 1 ).percent() ) )
    {
      p() -> buffs.divine_purpose -> trigger();
      p() -> procs.divine_purpose -> occur();
    }

    if ( ( p() -> buffs.avenging_wrath -> up() || p() -> buffs.crusade -> up() ) &&
           p() -> azerite.lights_decree.ok() )
    {
      lights_decree_t* ld = debug_cast<lights_decree_t*>( p() -> active.lights_decree );
      // Light's Decree deals damage based on the base cost of the spell
      ld -> last_holy_power_cost = as<int>( base_costs[ RESOURCE_HOLY_POWER ] );
      ld -> execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    paladin_melee_attack_t::impact( s );

    // Apply Divine Judgment on enemy hits if the spell is harmful ( = is not Inquisition )
    if ( p() -> talents.divine_judgment -> ok() && harmful )
      p() -> buffs.divine_judgment -> trigger();
  }

  void consume_resource() override
  {
    paladin_melee_attack_t::consume_resource();

    if ( current_resource() == RESOURCE_HOLY_POWER)
    {
      p() -> trigger_memory_of_lucid_dreams( last_resource_cost );
    }
  }
};

// Crusade
struct crusade_t : public paladin_spell_t
{
  crusade_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "crusade", p, p -> talents.crusade )
  {
    parse_options( options_str );

    if ( ! ( p -> talents.crusade -> ok() ) )
      background = true;

    cooldown -> duration *= 1.0 + azerite::vision_of_perfection_cdr( p -> azerite_essence.vision_of_perfection );
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.crusade -> trigger();

    if ( p() -> azerite.avengers_might.ok() )
      p() -> buffs.avengers_might -> trigger( 1, p() -> buffs.avengers_might -> default_value, -1.0, p() -> buffs.crusade -> buff_duration );
  }

  bool ready() override
  {
    // Crusade can not be used if the buff is already active (eg. with Vision of Perfection)
    if ( p() -> buffs.crusade -> check() )
      return false;
    else
      return paladin_spell_t::ready();
  }
};

// Execution Sentence =======================================================

struct execution_sentence_t : public holy_power_consumer_t
{
  execution_sentence_t( paladin_t* p, const std::string& options_str ) :
    holy_power_consumer_t( "execution_sentence", p, p -> talents.execution_sentence )
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
      td( s -> target ) -> debuff.execution_sentence -> trigger();
    }
  }
};

// Blade of Justice =========================================================

struct blade_of_justice_t : public paladin_melee_attack_t
{
  struct expurgation_t : public paladin_spell_t
  {
    expurgation_t( paladin_t* p ) :
      paladin_spell_t( "expurgation", p, p -> find_spell( 273481 ) )
    {
      base_td = p -> azerite.expurgation.value();
      hasted_ticks = false;
      tick_may_crit = true;
    }
  };

  expurgation_t* expurgation;

  blade_of_justice_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "blade_of_justice", p, p -> find_class_spell( "Blade of Justice" ) ),
    expurgation( nullptr )
  {
    parse_options( options_str );

    if ( p -> azerite.expurgation.ok() )
    {
      expurgation = new expurgation_t( p );
      add_child( expurgation );
    }
  }

  virtual double action_multiplier() const override
  {
    double am = paladin_melee_attack_t::action_multiplier();
    if ( p() -> buffs.blade_of_wrath -> up() )
      am *= 1.0 + p() -> buffs.blade_of_wrath -> data().effectN( 1 ).percent();
    return am;
  }

  virtual void execute() override
  {
    paladin_melee_attack_t::execute();
    if ( p() -> buffs.blade_of_wrath -> up() )
      p() -> buffs.blade_of_wrath -> expire();
  }

  virtual void impact( action_state_t* state ) override
  {
    paladin_melee_attack_t::impact( state );

    if ( p() -> azerite.expurgation.ok() && state -> result == RESULT_CRIT )
    {
      expurgation -> set_target( state -> target );
      expurgation -> execute();
    }
  }
};

// Divine Storm =============================================================

struct divine_storm_t: public holy_power_consumer_t
{
  // Divine Storm's damage is stored in a different spell than the player's cast
  struct divine_storm_damage_t : public paladin_melee_attack_t
  {
    divine_storm_damage_t( paladin_t* p ) :
      paladin_melee_attack_t( "divine_storm_dmg", p, p -> find_spell( 224239 ) )
    {
      dual = background = true;
    }

    double bonus_da( const action_state_t* s ) const override
    {
      double b = paladin_melee_attack_t::bonus_da( s );

      if ( p() -> buffs.empyrean_power -> up() )
      {
        b += p() -> azerite.empyrean_power.value();
      }

      return b;
    }
  };

  divine_storm_t( paladin_t* p, const std::string& options_str ) :
    holy_power_consumer_t( "divine_storm", p, p -> find_class_spell( "Divine Storm" ) )
  {
    parse_options( options_str );
    is_divine_storm = true;

    may_block = false;
    impact_action = new divine_storm_damage_t( p );
    impact_action -> stats = stats;

    weapon = &( p -> main_hand_weapon );

    aoe = -1;

    // TODO: Okay, when did this get reset to 1?
    weapon_multiplier = 0;
  }

  void record_data( action_state_t* ) override {}
};

struct templars_verdict_t : public holy_power_consumer_t
{
  // Templar's Verdict damage is stored in a different spell
  struct templars_verdict_damage_t : public paladin_melee_attack_t
  {
    templars_verdict_damage_t( paladin_t *p ) :
      paladin_melee_attack_t( "templars_verdict_dmg", p, p -> find_spell( 224266 ) )
    {
      dual = background = true;
    }

    virtual double action_multiplier() const override
    {
      double am = paladin_melee_attack_t::action_multiplier();
      if ( p() -> buffs.righteous_verdict -> check() )
        am *= 1.0 + p() -> buffs.righteous_verdict -> data().effectN( 1 ).percent();
      return am;
    }
  };

  templars_verdict_t( paladin_t* p, const std::string& options_str ) :
    holy_power_consumer_t( "templars_verdict", p, p -> find_specialization_spell( "Templar's Verdict" ) )
  {
    parse_options( options_str );

    may_block = false;
    impact_action = new templars_verdict_damage_t( p );
    impact_action -> stats = stats;

    // Okay, when did this get reset to 1?
    weapon_multiplier = 0;
  }

  void record_data( action_state_t* ) override {}

  virtual void execute() override
  {
    // store cost for potential refunding (see below)
    double c = cost();

    holy_power_consumer_t::execute();

    // missed/dodged/parried TVs do not consume Holy Power
    // check for a miss, and refund the appropriate amount of HP if we spent any
    if ( result_is_miss( execute_state -> result ) && c > 0 )
    {
      p() -> resource_gain( RESOURCE_HOLY_POWER, c, p() -> gains.hp_templars_verdict_refund );
    }

    if ( p() -> talents.righteous_verdict -> ok() )
    {
      p() -> buffs.righteous_verdict -> expire();
      p() -> buffs.righteous_verdict -> trigger();
    }
  }
};

// Judgment - Retribution =================================================================

struct judgment_ret_t : public judgment_t
{
  int holy_power_generation;

  judgment_ret_t( paladin_t* p, const std::string& options_str ) :
    judgment_t( p, options_str ),
    holy_power_generation( as<int>( p -> find_spell( 220637 ) -> effectN( 1 ).base_value() ) )
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
      am *= 1.0 + p() -> buffs.divine_judgment -> stack_value();
    return am;
  }

  // Special things that happen when Judgment damages target
  void impact( action_state_t* s ) override
  {
    judgment_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> spec.judgment_2 -> ok() )
    {
      td( s -> target ) -> debuff.judgment -> trigger();

      p() -> resource_gain( RESOURCE_HOLY_POWER, holy_power_generation, p() -> gains.judgment );
    }
  }
};

// Justicar's Vengeance
struct justicars_vengeance_t : public holy_power_consumer_t
{
  justicars_vengeance_t( paladin_t* p, const std::string& options_str ) :
    holy_power_consumer_t( "justicars_vengeance", p, p -> talents.justicars_vengeance )
  {
    parse_options( options_str );

    // Spelldata doesn't have this
    hasted_gcd = true;

    weapon_multiplier = 0; // why is this needed?

    // Healing isn't implemented
  }
};

// SoV

struct shield_of_vengeance_proc_t : public paladin_spell_t
{
  shield_of_vengeance_proc_t( paladin_t* p ) :
    paladin_spell_t( "shield_of_vengeance_proc", p, p -> find_spell( 184689 ) )
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
  }

  void init() override
  {
    paladin_absorb_t::init();
    snapshot_flags |= (STATE_CRIT | STATE_VERSATILITY);
  }

  virtual void execute() override
  {
    double shield_amount = p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent();
    paladin_absorb_t::execute();
    p() -> buffs.shield_of_vengeance -> trigger( 1, shield_amount );
  }
};


// Wake of Ashes (Retribution) ================================================

struct wake_of_ashes_t : public paladin_spell_t
{
  wake_of_ashes_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "wake_of_ashes", p, p -> talents.wake_of_ashes )
  {
    parse_options( options_str );
    if ( ! ( p -> talents.wake_of_ashes -> ok() ) )
      background = true;

    may_crit = true;
    aoe = -1;
  }
};

struct inquisition_t : public holy_power_consumer_t
{
  inquisition_t( paladin_t* p, const std::string& options_str ) :
    holy_power_consumer_t( "inquisition", p, p -> talents.inquisition )
  {
    parse_options( options_str );
    if ( ! ( p -> talents.inquisition -> ok() ) )
      background = true;
    // spelldata doesn't have this
    hasted_gcd = true;
    harmful = false;

  }

  virtual double cost() const override
  {
    double max_cost = base_costs[ RESOURCE_HOLY_POWER ] + secondary_costs[ RESOURCE_HOLY_POWER ];

    if ( p() -> buffs.fires_of_justice -> check() )
    {
      // FoJ essentially reduces the max cost by 1.
      max_cost += p() -> buffs.fires_of_justice -> data().effectN( 1 ).base_value();
    }

    // Return the available holy power of the player, if it's between 1 and 3 (minus 1 with FoJ up)
    return clamp<double>( p() -> resources.current[ RESOURCE_HOLY_POWER ], base_costs[ RESOURCE_HOLY_POWER ], max_cost );
  }

  virtual void execute() override
  {
    // Fires of Justice reduces the cost by 1 but maintains the overall duration
    // This needs to be computed before FoJ is consumed in holy_power_consumer_t::execute();
    double hp_spent = cost();

    if ( p() -> buffs.fires_of_justice -> up() )
    {
      // Re-add the fires of justice cost reduction
      hp_spent += -1.0 * p() -> buffs.fires_of_justice -> data().effectN( 1 ).base_value();
    }

    holy_power_consumer_t::execute();

    p() -> buffs.inquisition -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, hp_spent * p() -> talents.inquisition -> duration() );
  }
};

struct hammer_of_wrath_t : public paladin_melee_attack_t
{
  hammer_of_wrath_t( paladin_t* p, const std::string& options_str ) :
    paladin_melee_attack_t( "hammer_of_wrath", p, p -> talents.hammer_of_wrath )
  {
    parse_options( options_str );
  }

  virtual bool target_ready( player_t* candidate_target ) override
  {
    if ( ! p() -> get_how_availability( candidate_target ) )
    {
      return false;
    }

    return paladin_melee_attack_t::target_ready( candidate_target );
  }
};

struct zeal_t : public paladin_melee_attack_t
{
  zeal_t( paladin_t* p ) : paladin_melee_attack_t( "zeal", p, p -> find_spell( 269937 ) )
  { background = true; }
};

// Initialization

void paladin_t::create_ret_actions()
{
  if ( azerite.lights_decree.enabled() )
  {
    active.lights_decree = new lights_decree_t( this );
  }

  if ( talents.zeal )
  {
    active.zeal = new zeal_t( this );
  }

  active.shield_of_vengeance_damage = new shield_of_vengeance_proc_t( this );
}

action_t* paladin_t::create_action_retribution( const std::string& name, const std::string& options_str )
{
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
  buffs.blade_of_wrath = make_buff( this, "blade_of_wrath", find_spell( 281178 ) )
                       -> set_default_value( find_spell( 281178 ) -> effectN( 1 ).percent() );
  buffs.crusade = new buffs::crusade_buff_t( this );
  buffs.divine_judgment = make_buff( this, "divine_judgment", talents.divine_judgment -> effectN( 1 ).trigger() )
                        -> set_default_value( talents.divine_judgment -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  buffs.fires_of_justice = make_buff( this, "fires_of_justice", talents.fires_of_justice -> effectN( 1 ).trigger() )
                         -> set_chance( talents.fires_of_justice -> ok() ? talents.fires_of_justice -> proc_chance() : 0 );
  buffs.inquisition = make_buff( this, "inquisition", talents.inquisition )
                    -> add_invalidate( CACHE_HASTE );
  buffs.righteous_verdict = make_buff( this, "righteous_verdict", find_spell( 267611 ) );

  buffs.shield_of_vengeance = new buffs::shield_of_vengeance_buff_t( this );
  buffs.zeal = make_buff( this, "zeal", find_spell( 269571 ) )
             -> add_invalidate( CACHE_ATTACK_SPEED );

  // Azerite
  buffs.empyrean_power = make_buff( this, "empyrean_power", find_spell( 286393 ) )
                       -> set_default_value( azerite.empyrean_power.value() );
  buffs.relentless_inquisitor = make_buff<stat_buff_t>(this, "relentless_inquisitor", find_spell( 279204 ) )
                              -> add_stat( STAT_HASTE_RATING, azerite.relentless_inquisitor.value() );
}

void paladin_t::init_rng_retribution()
{
  art_of_war_rppm = get_rppm( "art_of_war", find_spell( 267344 ) );

  // The base RPPM of Art of War in spelldata is 4+haste, reduced by half when BoW isn't talented
  // Have to call the BoW talent spell ID's directly because it's not talented
  if ( ! talents.blade_of_wrath -> ok() )
  {
    art_of_war_rppm -> set_modifier(
      art_of_war_rppm -> get_modifier() / ( 1.0 + find_spell( 231832 ) -> effectN( 1 ).percent() ) );
  }
}

void paladin_t::init_spells_retribution()
{
  // Talents
  talents.zeal                = find_talent_spell( "Zeal" );
  talents.righteous_verdict   = find_talent_spell( "Righteous Verdict" );
  talents.execution_sentence  = find_talent_spell( "Execution Sentence" );

  talents.fires_of_justice    = find_talent_spell( "Fires of Justice" );
  talents.blade_of_wrath      = find_talent_spell( "Blade of Wrath" );
  talents.hammer_of_wrath     = find_talent_spell( "Hammer of Wrath" );

  talents.divine_judgment     = find_talent_spell( "Divine Judgment" );
  talents.consecration        = find_talent_spell( "Consecration" );
  talents.wake_of_ashes       = find_talent_spell( "Wake of Ashes" );

  //talents.unbreakable_spirit   = find_talent_spell( "Unbreakable Spirit" );
  //talents.cavalier             = find_talent_spell( "Cavalier" );
  talents.eye_for_an_eye      = find_talent_spell( "Eye for an Eye" );

  talents.selfless_healer     = find_talent_spell( "Selfless Healer" ); // Healing, NYI
  talents.justicars_vengeance = find_talent_spell( "Justicar's Vengeance" );
  talents.word_of_glory       = find_talent_spell( "Word of Glory" );

  talents.divine_purpose      = find_talent_spell( "Divine Purpose" );
  talents.crusade             = find_talent_spell( "Crusade" );
  talents.inquisition         = find_talent_spell( "Inquisition" );

  // Spec passives and useful spells
  spec.retribution_paladin = find_specialization_spell( "Retribution Paladin" );
  mastery.hand_of_light = find_mastery_spell( PALADIN_RETRIBUTION );

  if ( specialization() == PALADIN_RETRIBUTION )
  {
    spec.judgment_2 = find_specialization_spell( 231663 );

    spells.judgment_debuff = find_spell( 197277 );
    spells.divine_purpose_buff = find_spell( 223819 );
  }

  passives.boundless_conviction = find_spell( 115675 );

  spells.lights_decree = find_spell( 286231 );
  spells.execution_sentence_debuff = talents.execution_sentence -> effectN( 2 ).trigger();

  // Azerite traits
  azerite.expurgation           = find_azerite_spell( "Expurgation" );
  azerite.relentless_inquisitor = find_azerite_spell( "Relentless Inquisitor" );
  azerite.empyrean_power        = find_azerite_spell( "Empyrean Power" );
  azerite.lights_decree         = find_azerite_spell( "Light's Decree" );
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
  action_priority_list_t* cds = get_action_priority_list( "cooldowns" );
  action_priority_list_t* generators = get_action_priority_list( "generators" );
  action_priority_list_t* finishers = get_action_priority_list( "finishers" );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Rebuke" );
  def -> add_action( "call_action_list,name=cooldowns" );
  def -> add_action( "call_action_list,name=generators" );

  if ( sim -> allow_potions )
  {
    if ( true_level > 100 )
      cds -> add_action( "potion,if=(cooldown.guardian_of_azeroth.remains>90|!essence.condensed_lifeforce.major)&(buff.bloodlust.react|buff.avenging_wrath.up|buff.crusade.up&buff.crusade.remains<25)" );
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
  cds -> add_action( this, "Shield of Vengeance", "if=buff.seething_rage.down&buff.memory_of_lucid_dreams.down" );

  // Items
  bool has_knot = false;
  bool has_lurkers = false;
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

      if ( items[i].name_str == "lurkers_insidious_gift" )
      {
        has_lurkers = true;
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
      else if ( items[i].name_str == "azsharas_font_of_power" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=!talent.crusade.enabled&cooldown.avenging_wrath.remains<5|talent.crusade.enabled&cooldown.crusade.remains<5&time>10|holy_power>=3&time<10&talent.wake_of_ashes.enabled";
      }
      else if ( items[i].name_str == "vision_of_demise" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10|cooldown.avenging_wrath.remains>=30|cooldown.crusade.remains>=30";
      }
      else if ( items[i].name_str == "ashvanes_razor_coral" )
      {
        item_str = "use_item,name=" + items[i].name_str + ",if=debuff.razor_coral_debuff.down|buff.avenging_wrath.remains>=20&(cooldown.guardian_of_azeroth.remains>90|target.time_to_die<30)|buff.crusade.up&buff.crusade.stack=10&buff.crusade.remains>15&(cooldown.guardian_of_azeroth.remains>90||target.time_to_die<30)";
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

  cds -> add_action( "the_unbound_force,if=time<=2|buff.reckless_force.up" );
  cds -> add_action( "blood_of_the_enemy,if=buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10" );
  cds -> add_action( "guardian_of_azeroth,if=!talent.crusade.enabled&(cooldown.avenging_wrath.remains<5&holy_power>=3&(buff.inquisition.up|!talent.inquisition.enabled)|cooldown.avenging_wrath.remains>=45)|(talent.crusade.enabled&cooldown.crusade.remains<gcd&holy_power>=4|holy_power>=3&time<10&talent.wake_of_ashes.enabled|cooldown.crusade.remains>=45)" );
  cds -> add_action( "worldvein_resonance,if=cooldown.avenging_wrath.remains<gcd&holy_power>=3|cooldown.crusade.remains<gcd&holy_power>=4|cooldown.avenging_wrath.remains>=45|cooldown.crusade.remains>=45" );
  cds -> add_action( "focused_azerite_beam,if=(!raid_event.adds.exists|raid_event.adds.in>30|spell_targets.divine_storm>=2)&(buff.avenging_wrath.down|buff.crusade.down)&(cooldown.blade_of_justice.remains>gcd*3&cooldown.judgment.remains>gcd*3)" );
  cds -> add_action( "memory_of_lucid_dreams,if=(buff.avenging_wrath.up|buff.crusade.up&buff.crusade.stack=10)&holy_power<=3" );
  cds -> add_action( "purifying_blast,if=(!raid_event.adds.exists|raid_event.adds.in>30|spell_targets.divine_storm>=2)" );
  cds -> add_action( this, "Avenging Wrath", "if=(!talent.inquisition.enabled|buff.inquisition.up)&holy_power>=3" );
  cds -> add_talent( this, "Crusade", "if=holy_power>=4|holy_power>=3&time<10&talent.wake_of_ashes.enabled" );

  if ( has_lurkers )
  {
    cds -> add_action( "use_item,name=lurkers_insidious_gift,if=buff.avenging_wrath.up|buff.crusade.up" );
  }

  finishers -> add_action( "variable,name=wings_pool,value=!equipped.169314&(!talent.crusade.enabled&cooldown.avenging_wrath.remains>gcd*3|cooldown.crusade.remains>gcd*3)|equipped.169314&(!talent.crusade.enabled&cooldown.avenging_wrath.remains>gcd*6|cooldown.crusade.remains>gcd*6)" );
  finishers -> add_action( "variable,name=ds_castable,value=spell_targets.divine_storm>=2&!talent.righteous_verdict.enabled|spell_targets.divine_storm>=3&talent.righteous_verdict.enabled|buff.empyrean_power.up&debuff.judgment.down&buff.divine_purpose.down&buff.avenging_wrath_autocrit.down" );
  finishers -> add_talent( this, "Inquisition", "if=buff.avenging_wrath.down&(buff.inquisition.down|buff.inquisition.remains<8&holy_power>=3|talent.execution_sentence.enabled&cooldown.execution_sentence.remains<10&buff.inquisition.remains<15|cooldown.avenging_wrath.remains<15&buff.inquisition.remains<20&holy_power>=3)" );
  finishers -> add_talent( this, "Execution Sentence", "if=spell_targets.divine_storm<=2&(!talent.crusade.enabled&cooldown.avenging_wrath.remains>10|talent.crusade.enabled&buff.crusade.down&cooldown.crusade.remains>10|buff.crusade.stack>=7)" );
  finishers -> add_action( this, "Divine Storm", "if=variable.ds_castable&variable.wings_pool&((!talent.execution_sentence.enabled|(spell_targets.divine_storm>=2|cooldown.execution_sentence.remains>gcd*2))|(cooldown.avenging_wrath.remains>gcd*3&cooldown.avenging_wrath.remains<10|cooldown.crusade.remains>gcd*3&cooldown.crusade.remains<10|buff.crusade.up&buff.crusade.stack<10))" );
  finishers -> add_action( this, "Templar's Verdict", "if=variable.wings_pool&(!talent.execution_sentence.enabled|cooldown.execution_sentence.remains>gcd*2|cooldown.avenging_wrath.remains>gcd*3&cooldown.avenging_wrath.remains<10|cooldown.crusade.remains>gcd*3&cooldown.crusade.remains<10|buff.crusade.up&buff.crusade.stack<10)" );

  generators -> add_action( "variable,name=HoW,value=(!talent.hammer_of_wrath.enabled|target.health.pct>=20&(buff.avenging_wrath.down|buff.crusade.down))" );
  generators -> add_action( "call_action_list,name=finishers,if=holy_power>=5|buff.memory_of_lucid_dreams.up|buff.seething_rage.up|buff.inquisition.down&holy_power>=3" );
  generators -> add_talent( this, "Wake of Ashes", "if=(!raid_event.adds.exists|raid_event.adds.in>15|spell_targets.wake_of_ashes>=2)&(holy_power<=0|holy_power=1&cooldown.blade_of_justice.remains>gcd)&(cooldown.avenging_wrath.remains>10|talent.crusade.enabled&cooldown.crusade.remains>10)" );
  generators -> add_action( this, "Blade of Justice", "if=holy_power<=2|(holy_power=3&(cooldown.hammer_of_wrath.remains>gcd*2|variable.HoW))" );
  generators -> add_action( this, "Judgment", "if=holy_power<=2|(holy_power<=4&(cooldown.blade_of_justice.remains>gcd*2|variable.HoW))" );
  generators -> add_talent( this, "Hammer of Wrath", "if=holy_power<=4" );
  generators -> add_talent( this, "Consecration", "if=holy_power<=2|holy_power<=3&cooldown.blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>gcd*2" );
  generators -> add_action( "call_action_list,name=finishers,if=talent.hammer_of_wrath.enabled&target.health.pct<=20|buff.avenging_wrath.up|buff.crusade.up" );
  generators -> add_action( this, "Crusader Strike", "if=cooldown.crusader_strike.charges_fractional>=1.75&(holy_power<=2|holy_power<=3&cooldown.blade_of_justice.remains>gcd*2|holy_power=4&cooldown.blade_of_justice.remains>gcd*2&cooldown.judgment.remains>gcd*2&cooldown.consecration.remains>gcd*2)" );
  generators -> add_action( "call_action_list,name=finishers" );
  generators -> add_action( "concentrated_flame" );
  generators -> add_action( this, "Crusader Strike", "if=holy_power<=4" );
  generators -> add_action( "arcane_torrent,if=holy_power<=4" );
}

} // end namespace paladin
