#include "simulationcraft.hpp"
#include "sc_paladin.hpp"

namespace paladin {
// Beacon of Light ==========================================================

struct beacon_of_light_t : public paladin_heal_t
{
  beacon_of_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "beacon_of_light", p, p -> find_class_spell( "Beacon of Light" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    // Target is required for Beacon
    if ( option.target_str.empty() )
    {
      sim -> errorf( "Warning %s's \"%s\" needs a target", p -> name(), name() );
      sim -> cancel();
    }

    // Remove the 'dot'
    dot_duration = timespan_t::zero();
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    p() -> beacon_target = target;
    target -> buffs.beacon_of_light -> trigger();
  }
};

struct beacon_of_light_heal_t : public heal_t
{
  beacon_of_light_heal_t( paladin_t* p ) :
    heal_t( "beacon_of_light_heal", p, p -> find_spell( 53652 ) )
  {
    background = true;
    may_crit = false;
    proc = true;
    trigger_gcd = timespan_t::zero();

    target = p -> beacon_target;
  }
};

// Divine Protection ========================================================

struct divine_protection_t : public paladin_spell_t
{
  divine_protection_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "divine_protection", p, p -> find_class_spell( "Divine Protection" ) )
  {
    parse_options( options_str );

    harmful = false;
    use_off_gcd = true;
    trigger_gcd = timespan_t::zero();

    // unbreakable spirit reduces cooldown
    if ( p -> talents.unbreakable_spirit -> ok() )
      cooldown -> duration = data().cooldown() * ( 1 + p -> talents.unbreakable_spirit -> effectN( 1 ).percent() );
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    p() -> buffs.divine_protection -> trigger();
  }
};


// Holy Light Spell =========================================================

struct holy_light_t : public paladin_heal_t
{
  holy_light_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_light", p, p -> find_specialization_spell( "Holy Light" ) )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    p() -> buffs.infusion_of_light -> expire();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = paladin_heal_t::execute_time();

    if ( p() -> buffs.infusion_of_light -> check() )
      t += p() -> buffs.infusion_of_light -> data().effectN( 1 ).time_value();

    return t;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    paladin_heal_t::schedule_execute( state );

    p() -> buffs.infusion_of_light -> up(); // Buff uptime tracking
  }
};

// Holy Prism ===============================================================

// Holy Prism AOE (damage) - This is the aoe damage proc that triggers when holy_prism_heal is cast.

struct holy_prism_aoe_damage_t : public paladin_spell_t
{
  holy_prism_aoe_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_prism_aoe_damage", p, p->find_spell( 114871 ) )
  {
    background = true;
    may_crit = true;
    may_miss = false;
    aoe = 5;

  }
};

// Holy Prism AOE (heal) -  This is the aoe healing proc that triggers when holy_prism_damage is cast

struct holy_prism_aoe_heal_t : public paladin_heal_t
{
  holy_prism_aoe_heal_t( paladin_t* p )
    : paladin_heal_t( "holy_prism_aoe_heal", p, p->find_spell( 114871 ) )
  {
    background = true;
    aoe = 5;
  }

};

// Holy Prism (damage) - This is the damage version of the spell; for the heal version, see holy_prism_heal_t

struct holy_prism_damage_t : public paladin_spell_t
{
  holy_prism_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_prism_damage", p, p->find_spell( 114852 ) )
  {
    background = true;
    may_crit = true;
    may_miss = false;
    // this updates the spell coefficients appropriately for single-target damage
    parse_effect_data( p -> find_spell( 114852 ) -> effectN( 1 ) );

    // on impact, trigger a holy_prism_aoe_heal cast
    impact_action = new holy_prism_aoe_heal_t( p );
  }
};


// Holy Prism (heal) - This is the healing version of the spell; for the damage version, see holy_prism_damage_t

struct holy_prism_heal_t : public paladin_heal_t
{
  holy_prism_heal_t( paladin_t* p ) :
    paladin_heal_t( "holy_prism_heal", p, p -> find_spell( 114871 ) )
  {
    background = true;

    // update spell coefficients appropriately for single-target healing
    parse_effect_data( p -> find_spell( 114871 ) -> effectN( 1 ) );

    // on impact, trigger a holy_prism_aoe cast
    impact_action = new holy_prism_aoe_damage_t( p );
  }

};

// Holy Prism - This is the base spell, it chooses the effects to trigger based on the target

struct holy_prism_t : public paladin_spell_t
{
  holy_prism_damage_t* damage;
  holy_prism_heal_t* heal;

  holy_prism_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "holy_prism", p, p->find_spell( 114165 ) )
  {
    parse_options( options_str );

    // create the damage and healing spell effects, designate them as children for reporting
    damage = new holy_prism_damage_t( p );
    add_child( damage );
    heal = new holy_prism_heal_t( p );
    add_child( heal );

    // disable if not talented
    if ( ! ( p -> talents.holy_prism -> ok() ) )
      background = true;
  }

  virtual void execute() override
  {
    paladin_spell_t::execute();

    if ( target -> is_enemy() )
    {
      // damage enemy
      damage -> target = target;
      damage -> schedule_execute();
    }
    else
    {
      // heal friendly
      heal -> target = target;
      heal -> schedule_execute();
    }
  }
};

// Holy Shock ===============================================================

// Holy Shock is another one of those "heals or does damage depending on target" spells.
// The holy_shock_t structure handles global stuff, and calls one of two children
// (holy_shock_damage_t or holy_shock_heal_t) depending on target to handle healing/damage
// find_class_spell returns spell 20473, which has the cooldown/cost/etc stuff, but the actual
// damage and healing information is in spells 25912 and 25914, respectively.

struct holy_shock_damage_t : public paladin_spell_t
{
  double crit_chance_multiplier;

  holy_shock_damage_t( paladin_t* p )
    : paladin_spell_t( "holy_shock_damage", p, p -> find_spell( 25912 ) )
  {
    background = may_crit = true;
    trigger_gcd = timespan_t::zero();

    // this grabs the 100% base crit bonus from 20473
    crit_chance_multiplier = p -> find_class_spell( "Holy Shock" ) -> effectN( 1 ).base_value() / 10.0;
  }

  virtual double composite_crit_chance() const override
  {
    double cc = paladin_spell_t::composite_crit_chance();

    // effect 1 doubles crit chance
    cc *= crit_chance_multiplier;

    return cc;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = paladin_spell_t::composite_target_multiplier( t );

    paladin_td_t* td = this -> td( t );

    if ( td -> buffs.debuffs_judgment -> up() )
    {
      double judgment_multiplier = 1.0 + td -> buffs.debuffs_judgment -> data().effectN( 1 ).percent();
      judgment_multiplier += p() -> passives.judgment -> effectN( 1 ).percent();
      m *= judgment_multiplier;
    }

    return m;
  }
};

// Holy Shock Heal Spell ====================================================

struct holy_shock_heal_t : public paladin_heal_t
{
  double crit_chance_multiplier;

  holy_shock_heal_t( paladin_t* p ) :
    paladin_heal_t( "holy_shock_heal", p, p -> find_spell( 25914 ) )
  {
    background = true;
    trigger_gcd = timespan_t::zero();

    // this grabs the crit multiplier bonus from 20473
    crit_chance_multiplier = p -> find_class_spell( "Holy Shock" ) -> effectN( 1 ).base_value() / 10.0;
  }

  virtual double composite_crit_chance() const override
  {
    double cc = paladin_heal_t::composite_crit_chance();

    // effect 1 doubles crit chance
    cc *= crit_chance_multiplier;

    return cc;
  }

  virtual void execute() override
  {
    paladin_heal_t::execute();

    if ( execute_state -> result == RESULT_CRIT )
      p() -> buffs.infusion_of_light -> trigger();

  }
};

struct holy_shock_t : public paladin_spell_t
{
  holy_shock_damage_t* damage;
  holy_shock_heal_t* heal;
  bool dmg;

  holy_shock_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "holy_shock", p, p -> find_specialization_spell( 20473 ) ),
    dmg( false )
  {
    add_option( opt_bool( "damage", dmg ) );
    parse_options( options_str );

    cooldown = p -> cooldowns.holy_shock;

    // create the damage and healing spell effects, designate them as children for reporting
    damage = new holy_shock_damage_t( p );
    add_child( damage );
    heal = new holy_shock_heal_t( p );
    add_child( heal );
  }

  virtual void execute() override
  {
    if ( dmg )
    {
      // damage enemy
      damage -> set_target( execute_state -> target );
      damage -> schedule_execute();
    }
    else
    {
      // heal friendly
      heal -> set_target( execute_state -> target );
      heal -> schedule_execute();
    }

    paladin_spell_t::execute();

    if ( p() -> buffs.divine_purpose -> check() )
    {
       p() -> buffs.divine_purpose -> expire();
    }

    if ( p() -> talents.divine_purpose -> ok() )
    {
      bool success = p() -> buffs.divine_purpose -> trigger( 1,
                                                            p() -> buffs.divine_purpose -> default_value,
                                                            p() -> spells.divine_purpose_holy -> proc_chance() );
      if ( success ) 
      {
        p() -> procs.divine_purpose -> occur();
        p() -> cooldowns.holy_shock -> reset (true);
      }
    }

  }

  double recharge_multiplier() const override
  {
    double rm = paladin_spell_t::recharge_multiplier();

    if ( p() -> buffs.avenging_wrath -> check() && p() -> talents.sanctified_wrath -> ok() )
      rm *= 1.0 + p() -> talents.sanctified_wrath -> effectN( 2 ).percent();

    return rm;
  }
};


// Holy Avenger
struct holy_avenger_t : public paladin_heal_t
{
  holy_avenger_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "holy_avenger", p, p -> talents.holy_avenger )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    paladin_heal_t::execute();

    p() -> buffs.holy_avenger -> trigger();
  }
};

// Judgment - Holy =================================================================

struct judgment_holy_t : public judgment_t
{
  judgment_holy_t( paladin_t* p, const std::string& options_str )
    : judgment_t( p, options_str )
  {
    base_multiplier *= 1.0 + p -> spec.holy_paladin -> effectN( 11 ).percent();
  }

  virtual void execute() override
  {
    judgment_t::execute();

    if ( p() -> talents.fist_of_justice -> ok() )
    {
      double reduction = p() -> talents.fist_of_justice -> effectN( 1 ).base_value();
      p() -> cooldowns.hammer_of_justice -> adjust( timespan_t::from_seconds( - reduction ) );
    }
  }

  // Special things that happen when Judgment damages target
  void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> buffs.debuffs_judgment -> trigger();
    }

    judgment_t::impact( s );
  }
};

// Light's Hammer =============================================================

struct lights_hammer_damage_tick_t : public paladin_spell_t
{
  lights_hammer_damage_tick_t( paladin_t* p )
    : paladin_spell_t( "lights_hammer_damage_tick", p, p -> find_spell( 114919 ) )
  {
    //dual = true;
    background = true;
    aoe = -1;
    may_crit = true;
    ground_aoe = true;
  }
};

struct lights_hammer_heal_tick_t : public paladin_heal_t
{
  lights_hammer_heal_tick_t( paladin_t* p )
    : paladin_heal_t( "lights_hammer_heal_tick", p, p -> find_spell( 114919 ) )
  {
    dual = true;
    background = true;
    aoe = 6;
    may_crit = true;
  }

  std::vector< player_t* >& target_list() const override
  {
    target_cache.list = paladin_heal_t::target_list();
    target_cache.list = find_lowest_players( aoe );
    return target_cache.list;
  }
};

struct lights_hammer_t : public paladin_spell_t
{
  const timespan_t travel_time_;
  lights_hammer_heal_tick_t* lh_heal_tick;
  lights_hammer_damage_tick_t* lh_damage_tick;

  lights_hammer_t( paladin_t* p, const std::string& options_str )
    : paladin_spell_t( "lights_hammer", p, p -> find_talent_spell( "Light's Hammer" ) ),
      travel_time_( timespan_t::from_seconds( 1.5 ) )
  {
    // 114158: Talent spell, cooldown
    // 114918: Periodic 2s dummy, no duration!
    // 114919: Damage/Scale data
    // 122773: 15.5s duration, 1.5s for hammer to land = 14s aoe dot
    parse_options( options_str );
    may_miss = false;

    school = SCHOOL_HOLY; // Setting this allows the tick_action to benefit from Inquistion

    base_tick_time = p -> find_spell( 114918 ) -> effectN( 1 ).period();
    dot_duration      = p -> find_spell( 122773 ) -> duration() - travel_time_;
    hasted_ticks   = false;
    tick_zero = true;
    ignore_false_positive = true;

    dynamic_tick_action = true;
    lh_heal_tick = new lights_hammer_heal_tick_t( p );
    add_child( lh_heal_tick );
    lh_damage_tick = new lights_hammer_damage_tick_t( p );
    add_child( lh_damage_tick );

    // disable if not talented
    if ( ! ( p -> talents.lights_hammer -> ok() ) )
      background = true;
  }

  virtual timespan_t travel_time() const override
  { return travel_time_; }

  virtual void tick( dot_t* d ) override
  {
    paladin_spell_t::tick( d );

    // trigger healing and damage ticks
    lh_heal_tick -> schedule_execute();
    lh_damage_tick -> schedule_execute();
  }
};

// Light of Dawn (Holy) =======================================================

struct light_of_dawn_t : public paladin_heal_t
{
  light_of_dawn_t( paladin_t* p, const std::string& options_str ) :
    paladin_heal_t( "light_of_dawn", p, p -> find_class_spell( "Light of Dawn" ) )
  {
    parse_options( options_str );

    aoe = 6;

    cooldown = p -> cooldowns.light_of_dawn;
  }
};


// Initialization
action_t* paladin_t::create_action_holy( const std::string& name, const std::string& options_str )
{
  if ( name == "beacon_of_light"           ) return new beacon_of_light_t          ( this, options_str );
  if ( name == "divine_protection"         ) return new divine_protection_t        ( this, options_str );
  if ( name == "holy_avenger"              ) return new holy_avenger_t             ( this, options_str );
  if ( name == "holy_light"                ) return new holy_light_t               ( this, options_str );
  if ( name == "holy_prism"                ) return new holy_prism_t               ( this, options_str );
  if ( name == "holy_shock"                ) return new holy_shock_t               ( this, options_str );
  if ( name == "light_of_dawn"             ) return new light_of_dawn_t            ( this, options_str );
  if ( name == "lights_hammer"             ) return new lights_hammer_t            ( this, options_str );

  if ( specialization() == PALADIN_HOLY )
  {
    if ( name == "judgment") return new judgment_holy_t( this, options_str );
  }

  return nullptr;
}

void paladin_t::create_buffs_holy()
{
  buffs.divine_protection      = make_buff( this, "divine_protection", find_class_spell( "Divine Protection" ) )
                               -> set_cooldown( 0_ms ); // Handled by the action
  buffs.holy_avenger           = make_buff( this, "holy_avenger", talents.holy_avenger )
                               -> set_default_value( talents.holy_avenger -> effectN( 1 ).percent() )
                               -> add_invalidate( CACHE_HASTE );
  buffs.infusion_of_light      = make_buff( this, "infusion_of_light", find_spell( 54149 ) );
}

void paladin_t::init_spells_holy()
{
  // Talents
  talents.bestow_faith               = find_talent_spell( "Bestow Faith" );
  talents.lights_hammer              = find_talent_spell( "Light's Hammer" );
  talents.crusaders_might            = find_talent_spell( "Crusader's Might" );
  talents.cavalier                   = find_talent_spell( "Cavalier" );
  talents.unbreakable_spirit         = find_talent_spell( "Unbreakable Spirit" );
  talents.rule_of_law                = find_talent_spell( "Rule of Law" );
  talents.devotion_aura              = find_talent_spell( "Devotion Aura" );
  talents.aura_of_sacrifice          = find_talent_spell( "Aura of Sacrifice" );
  talents.aura_of_mercy              = find_talent_spell( "Aura of Mercy" );
  talents.holy_avenger               = find_talent_spell( "Holy Avenger" );
  talents.holy_prism                 = find_talent_spell( "Holy Prism" );
  talents.fervent_martyr             = find_talent_spell( "Fervent Martyr" );
  talents.sanctified_wrath           = find_talent_spell( "Sanctified Wrath" );
  talents.judgment_of_light          = find_talent_spell( "Judgment of Light" );
  talents.beacon_of_faith            = find_talent_spell( "Beacon of Faith" );
  talents.beacon_of_the_lightbringer = find_talent_spell( "Beacon of the Lightbringer" );
  talents.beacon_of_virtue           = find_talent_spell( "Beacon of Virtue" );

  // Spells
  spells.divine_purpose_holy           = find_spell( 197646 );

  // Mastery
  passives.lightbringer           = find_mastery_spell( PALADIN_HOLY );

  // Holy Passives
  passives.infusion_of_light      = find_specialization_spell( "Infusion of Light" );

  if ( specialization() == PALADIN_HOLY )
    spec.judgment_2 = find_specialization_spell( 231644 );

  if ( find_class_spell( "Beacon of Light" ) -> ok() )
    active_beacon_of_light = new beacon_of_light_heal_t( this );
}

// ==========================================================================
// Action Priority List Generation - Holy
// ==========================================================================

void paladin_t::generate_action_prio_list_holy_dps()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask
  precombat -> add_action( "flask" );

  // Food
  precombat -> add_action( "food" );

  // Augmentation
  precombat -> add_action( "augmentation" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-pot
  precombat -> add_action( "potion" );

  // action priority list
  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* cds = get_action_priority_list( "cooldowns" );
  action_priority_list_t* priority = get_action_priority_list( "priority" );

  def -> add_action( "auto_attack" );
  def -> add_action( "call_action_list,name=cooldowns");
  def -> add_action( "call_action_list,name=priority");

  cds -> add_action("avenging_wrath");
  if ( sim -> allow_potions )
  {
      cds -> add_action("potion,if=(buff.avenging_wrath.up)");
  }
  cds -> add_action("blood_fury,if=(buff.avenging_wrath.up)");
  cds -> add_action("berserking,if=(buff.avenging_wrath.up)");
  cds -> add_action("holy_avenger,if=(buff.avenging_wrath.up)");
  cds -> add_action("use_items,if=(buff.avenging_wrath.up)");

  priority -> add_action("judgment");
  priority -> add_action("holy_shock,damage=1");
  priority -> add_action("crusader_strike");
  priority -> add_action("holy_prism,target=self,if=active_enemies>=2");
  priority -> add_action("holy_prism");
  priority -> add_action("consecration");
  priority -> add_action("light_of_dawn");
}

void paladin_t::generate_action_prio_list_holy()
{
  // currently unsupported

  //precombat first
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  //Flask
  precombat -> add_action( "flask" );

  // Food
  precombat -> add_action( "food" );

  // Augmentation
  precombat -> add_action( "augmentation" );



  precombat -> add_action( this, "Seal of Insight" );
  precombat -> add_action( this, "Beacon of Light" , "target=healing_target");
  // Beacon probably goes somewhere here?
  // Damn right it does, Theckie-poo.

  // Snapshot stats
  precombat -> add_action( "snapshot_stats",  "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Augmentation
  precombat -> add_action( "potion" );

  // action priority list
  action_priority_list_t* def = get_action_priority_list( "default" );

  def -> add_action( "mana_potion,if=mana.pct<=75" );

  def -> add_action( "auto_attack" );

  int num_items = ( int ) items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action ( "/use_item,name=" + items[ i ].name_str );
    }
  }

  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  // this is just sort of made up to test things - a real Holy dev should probably come up with something useful here eventually
  // Workin on it. Phillipuh to-do
  def -> add_action( this, "Avenging Wrath" );
  def -> add_action( this, "Lay on Hands","if=incoming_damage_5s>health.max*0.7" );
  def -> add_action( "wait,if=target.health.pct>=75&mana.pct<=10" );
  def -> add_action( this, "Flash of Light", "if=target.health.pct<=30" );
  def -> add_action( this, "Lay on Hands", "if=mana.pct<5" );
  def -> add_action( this, "Holy Light" );

}

}
