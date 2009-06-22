// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Paladin
// ==========================================================================

enum { SEAL_NONE, SEAL_OF_BLOOD, SEAL_MAX };

struct paladin_t : public player_t
{
  // Active
  int           active_seal;
  attack_t*     active_seal_of_blood;
  attack_t*     active_righteous_vengeance;

  // Buffs
  struct _buffs_t
  {
    int avenging_wrath;
    int vengeance;
    void reset() { memset( (void*) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Cooldowns
  struct _cooldowns_t
  {
    void reset() { memset( (void*) this, 0x00, sizeof( _cooldowns_t ) ); }
    _cooldowns_t() { reset(); }
  };
  _cooldowns_t _cooldowns;

  // Expirations
  struct _expirations_t
  {
    event_t*    vengeance;
    void reset() { memset( (void*) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_jotw;

  // Procs

  // Uptimes

  // Random Number Generation

  // Auto-Attack
  attack_t* auto_attack;

  struct talents_t
  {
    int benediction;            // -2-10% mana on instants
    int conviction;             // +1-5% crit
    int crusade;                // +1-3% dmg, +1-3% on humanoids, demons, undead, elemental
    int crusader_strike;
    int divine_storm;
    int divine_strength;
    int fanaticism;             // +6/12/18% crit on judgement
    int improved_judgements;    // -1/2 cd on judgements
    int judgements_of_the_wise; // 15% basemana
    int righteous_vengeance;    // judgement/DS crit -> 30% damage over 8s
    int sanctity_of_battle;     // +1-3% crit, +5-15% dmg on exorcism/CS
    int seal_of_command;
    int sheath_of_light;        // 10/20/30% AP as SP
    int the_art_of_war;         // +5-10% dmg on judgement, CS, DS
    int two_handed_weapon_spec; // +2-6% dmg with 2h weapons
    int vengeance;              // +1-3% dmg for 30s after crit, stacks 3 times

    // NYI
    int heart_of_the_crusader;  // 1-3%crit debuff on judgement
    int improved_bom;           // +12/25% on BoM
    int sanctified_retribution; // aura gives +3% dmg
    int sanctified_wrath;       // +25/50% crit on HoW, -30/60s AW cd
    int swift_retribution;      // auras give 1-3% haste
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int consecration;
    int judgement;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  paladin_t( sim_t* sim, const std::string& name ) : player_t( sim, PALADIN, name )
  {
    active_seal                = SEAL_NONE;
    active_seal_of_blood       = 0;
    active_righteous_vengeance = 0;

    auto_attack = 0;
  }

  virtual void      init_base();
  virtual void      init_gains();
  //virtual void      init_procs();
  //virtual void      init_uptimes();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_actions();
  virtual void      reset();
  virtual void      interrupt();
  virtual double    composite_attack_power();
  virtual double    composite_spell_power( int school );
  virtual bool      get_talent_trees( std::vector<int*>& holy, std::vector<int*>& protection, std::vector<int*>& retribution );
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual int       primary_resource() { return RESOURCE_MANA; }
  virtual int       primary_role()     { return ROLE_HYBRID; }
  virtual int       primary_tree();
};

struct paladin_attack_t : public attack_t
{
  bool trigger_seal;

  paladin_attack_t( const char* n, paladin_t* p, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
    attack_t( n, p, RESOURCE_MANA, s, t, special ),
    trigger_seal(true)
  {
    may_crit = true;
    base_dd_min = base_dd_max = 1;
    base_crit       += 0.01 * ( p -> talents.conviction + p -> talents.sanctity_of_battle );
    base_multiplier *= ( 1.0 + 0.01 * p -> talents.crusade * 2 ); // FIXME: target type
    base_multiplier *= ( 1.0 + 0.02 * p -> talents.two_handed_weapon_spec ); // FIXME: anything it doesn't affect?
  }

  virtual void execute();

  virtual double cost()
  {
    paladin_t* p = player -> cast_paladin();
    double c = attack_t::cost();
    return c * ( 1 - 0.02 * p -> talents.benediction );
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();

    attack_t::player_buff();

    player_multiplier *= 1.0 + 0.01 * p -> talents.vengeance * p -> _buffs.vengeance;
    if( p -> _buffs.avenging_wrath )
      player_multiplier *= 1.20;
  }
};

struct paladin_spell_t : public spell_t
{
  paladin_spell_t( const char* n, paladin_t* p, int s=SCHOOL_HOLY, int t=TREE_NONE ) :
    spell_t( n, p, RESOURCE_MANA, s, t )
  {
    base_crit       += 0.01 * ( p -> talents.conviction + p -> talents.sanctity_of_battle );
    base_multiplier *= ( 1.0 + 0.01 * p -> talents.crusade * 2 ); // FIXME: target type
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();

    spell_t::player_buff();

    player_multiplier *= 1.0 + 0.01 * p -> talents.vengeance * p -> _buffs.vengeance;
    if( p -> _buffs.avenging_wrath )
      player_multiplier *= 1.20;
  }
};

static void trigger_judgements_of_the_wise( attack_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if( ! p -> talents.judgements_of_the_wise ) return;
  if( ! a -> sim -> roll( p -> talents.judgements_of_the_wise / 3.0 ) ) return;

  p -> resource_gain( RESOURCE_MANA, 0.25 * p -> resource_base[ RESOURCE_MANA ], p -> gains_jotw );
}

static void trigger_righteous_vengeance( attack_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if( ! p -> talents.righteous_vengeance ) return;

  struct righteous_vengeance_t : public attack_t
  {
    righteous_vengeance_t( paladin_t* p ) : attack_t( "righteous_vengeance", p, RESOURCE_NONE, SCHOOL_HOLY )
    {
      may_miss    = false;
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_cost   = 0;

      base_tick_time = 1;
      num_ticks      = 8;
      tick_power_mod = 0;
    }
    void player_buff()   {}
    void target_debuff( int dmg_type ) {}
  };

  double dmg = p -> talents.righteous_vengeance * 0.1 * a -> direct_dmg;

  if( ! p -> active_righteous_vengeance ) p -> active_righteous_vengeance = new righteous_vengeance_t( p );

  if( p -> active_righteous_vengeance -> ticking )
  {
    int num_ticks = p -> active_righteous_vengeance -> num_ticks;
    int remaining_ticks = num_ticks - p -> active_righteous_vengeance -> current_tick;

    dmg += p -> active_righteous_vengeance -> base_td * remaining_ticks;

    p -> active_righteous_vengeance -> cancel();
  }
  p -> active_righteous_vengeance -> base_td = dmg / 8;
  p -> active_righteous_vengeance -> schedule_tick();
}

static void trigger_seal_of_blood( attack_t* a )
{
  if( a -> proc ) return;

  struct seal_of_blood_attack_t : public paladin_attack_t
  {
    seal_of_blood_attack_t( paladin_t* p ) :
      paladin_attack_t( "seal_of_blood_attack", p, SCHOOL_HOLY )
    {
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_cost   = 0;

      weapon                 = &( p -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier      = 0.48;
    }
  };

  paladin_t* p = a -> player -> cast_paladin();

  if( p -> active_seal != SEAL_OF_BLOOD ) return;

  if( ! p -> active_seal_of_blood ) p -> active_seal_of_blood = new seal_of_blood_attack_t( p );

  p -> active_seal_of_blood -> schedule_execute();
}

static void trigger_vengeance( attack_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.vengeance ) return;

  struct vengeance_expiration_t : public event_t
  {
    vengeance_expiration_t( sim_t* sim, paladin_t* p ) : event_t( sim, p )
    {
      name = "Vengeance Expiration";
      p -> aura_gain( "Vengeance" );
      sim -> add_event( this, 30.0 );
    }
    virtual void execute()
    {
      paladin_t* p = player -> cast_paladin();
      p -> aura_loss( "Vengeance" );
      p -> _buffs.vengeance = 0;
      p -> _expirations.vengeance = 0;
    }
  };

  if( p -> _buffs.vengeance < 3 ) p -> _buffs.vengeance++;

  event_t*& e = p -> _expirations.vengeance;

  if( e )
  {
    e -> reschedule( 30.0 );
  }
  else
  {
    e = new ( a -> sim ) vengeance_expiration_t( a -> sim, p );
  }
}

void paladin_attack_t::execute()
{
  attack_t::execute();

  if( result_is_hit() )
  {
    if( result == RESULT_CRIT )
    {
      trigger_vengeance( this );
    }

    if( trigger_seal )
    {
      trigger_seal_of_blood( this );
    }
  }
}

// Melee Attack ============================================================

struct melee_t : public paladin_attack_t
{
  melee_t( paladin_t* p ) : 
    paladin_attack_t( "melee", p, SCHOOL_PHYSICAL, TREE_NONE, false )
  {
    background        = true;
    repeating         = true;
    trigger_gcd       = 0;
    base_cost         = 0;
    weapon            = &( p -> main_hand_weapon );
    base_execute_time = p -> main_hand_weapon.swing_time;
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public paladin_attack_t
{
  auto_attack_t( paladin_t* p, const std::string& options_str ) : 
    paladin_attack_t( "auto_attack", p)
  {
    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> auto_attack = new melee_t( p );

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    p -> auto_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if( p -> moving ) return false;
    return( p -> auto_attack -> execute_event == 0 ); // not swinging
  }
};


// Seals last 30min, don't bother tracking expiration
// Seal of Blood ===========================================================

struct seal_of_blood_t : public paladin_attack_t
{
  seal_of_blood_t( paladin_t* p, const std::string& options_str ) : 
    paladin_attack_t( "seal_of_blood", p )
  {
    harmful = false;
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.14;
  }

  virtual void execute()
  {
    if( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    consume_resource();

    paladin_t* p = player -> cast_paladin();
    p -> active_seal = SEAL_OF_BLOOD;
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if( p -> active_seal == SEAL_OF_BLOOD )
      return false;

    return paladin_attack_t::ready();
  }
};

// Avenging Wrath ==========================================================

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "avenging_wrath", p )
  {
    harmful     = false;
    cooldown    = 180 - 30 * p -> talents.sanctified_wrath;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.08;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    struct avenging_wrath_expiration_t : public event_t
    {
      avenging_wrath_expiration_t( sim_t* sim, paladin_t* p ) : event_t( sim, p )
      {
        name = "Avenging Wrath Expiration";
        p -> aura_gain( "Avenging Wrath" );
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        player -> aura_loss( "Avenging Wrath" );
        player -> cast_paladin() -> _buffs.avenging_wrath = 0;
      }
    };

    if( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    consume_resource();
    update_ready();

    paladin_t* p = player -> cast_paladin();
    p -> _buffs.avenging_wrath = 1;

    new ( sim ) avenging_wrath_expiration_t( sim, p );
  }
};

// Crusader Strike =========================================================

// 110% wpn damage, 6s cd, 8%basemana
struct crusader_strike_t : public paladin_attack_t
{
  crusader_strike_t( paladin_t* p, const std::string& options_str ) : 
    paladin_attack_t( "crusader_strike", p )
  {
    assert( p -> talents.crusader_strike );

    weapon                 = &( p -> main_hand_weapon );
    weapon_multiplier     *= 1.1;
    normalize_weapon_speed = true;

    cooldown    = 6;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.08;

    base_multiplier *= 1.0 + 0.05 * p -> talents.sanctity_of_battle;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;
    if( p -> set_bonus.tier8_4pc() ) base_crit += 0.10;
  }
  
  virtual void execute()
  {
    paladin_attack_t::execute();

    if( result == RESULT_CRIT )
    {
      trigger_righteous_vengeance( this );
    }
  }
};

// Divine Storm ============================================================

// 110% damage, 10s cd, 12%basemana
struct divine_storm_t : public paladin_attack_t
{
  divine_storm_t( paladin_t* p, const std::string& options_str ) : 
    paladin_attack_t( "divine_storm", p)
  {
    assert( p -> talents.divine_storm );

    weapon                 = &( p -> main_hand_weapon );
    weapon_multiplier     *= 1.1;
    normalize_weapon_speed = true;

    cooldown    = 10;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.12;

    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;
    if( p -> set_bonus.tier7_2pc() ) base_multiplier *= 1.1;
    if( p -> set_bonus.tier8_4pc() ) base_crit += 0.10;
  }
  
  virtual void execute()
  {
    paladin_attack_t::execute();

    if( result == RESULT_CRIT )
    {
      trigger_righteous_vengeance( this );
    }
  }
};

// Hammer of Wrath =========================================================

struct hammer_of_wrath_t : public paladin_attack_t
{
  hammer_of_wrath_t( paladin_t* p, const std::string& options_str ) :
    paladin_attack_t( "hammer_of_wrath", p, SCHOOL_HOLY )
  {
    static rank_t ranks[] =
    {
      { 80, 6, 1139, 1257, 0, 0.12 },
      { 74, 5,  878,  970, 0, 0.12 },
      { 68, 4,  733,  809, 0, 0.12 },
      { 60, 3,  570,  628, 0, 0.14 },
      { 52, 2,  459,  507, 0, 0.14 },
      { 44, 1,  351,  387, 0, 0.14 },
    };
    init_rank( ranks );

    may_parry = false;
    may_dodge = false;

    trigger_seal = false;

    cooldown         = 6;
    direct_power_mod = 0.15;

    base_crit += 0.25 * p -> talents.sanctified_wrath;
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();

    paladin_attack_t::player_buff();

    // uglyyy FIXME
    player_dd_adder += 0.15 * p -> composite_spell_power( SCHOOL_HOLY );
  }

  virtual bool ready()
  {
    if( sim -> target -> health_percentage() > 20 )
      return false;

    return paladin_attack_t::ready();
  }
};

// Judgement ===============================================================

struct judgement_t : public paladin_attack_t
{
  judgement_t( paladin_t* p, const std::string& options_str ) : 
    paladin_attack_t( "judgement", p, SCHOOL_HOLY )
  {
    may_parry = false;
    may_dodge = false;
    may_block = false;

    trigger_seal = false;

    cooldown         = 10 - p -> talents.improved_judgements - p -> set_bonus.tier7_4pc();
    base_cost        = p -> resource_base[ RESOURCE_MANA ] * 0.05;

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;
    base_multiplier *= 1.0 + 0.1 * p -> glyphs.judgement;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();

    switch( p -> active_seal )
    {
      case SEAL_OF_BLOOD:
        weapon = &( p -> main_hand_weapon );
        weapon_multiplier = 0.26;
        base_dd_min = base_dd_max = 0.11 * p -> composite_attack_power_multiplier() * p -> composite_attack_power()
                                  + 0.18 * p -> composite_spell_power_multiplier()  * p -> composite_spell_power( SCHOOL_HOLY );
        // FIXME scaling, normalization
        break;
      default:
        assert(0);
        break;
    }

    paladin_attack_t::execute();

    if( result_is_hit() )
    {
      trigger_judgements_of_the_wise( this );

      if( result == RESULT_CRIT )
      {
        trigger_righteous_vengeance( this );
      }
    }
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if( p -> active_seal == SEAL_NONE )
      return false;

    return paladin_attack_t::ready();
  }
};




void paladin_t::init_base()
{
  // FIXME: Verify
  attribute_base[ ATTR_STRENGTH  ] = 153;
  attribute_base[ ATTR_AGILITY   ] =  86;
  attribute_base[ ATTR_STAMINA   ] = 146;
  attribute_base[ ATTR_INTELLECT ] =  97;
  attribute_base[ ATTR_SPIRIT    ] =  80;

  attribute_multiplier_initial[ ATTR_STRENGTH ] *= 1.0 + talents.divine_strength * 0.03;

  base_spell_crit = 0.0334;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.6 );

  base_attack_power = ( level * 3 ) - 20;
  base_attack_crit  = 0.0327;
  initial_attack_power_per_strength = 2.0;
  initial_attack_power_per_agility  = 0.0;
  initial_attack_crit_per_agility = 0.01/52.08; // FIXME only level 80 value

  resource_base[ RESOURCE_HEALTH ] = 3185; // FIXME
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1500/*FIXME*/, 2953, 4394 );

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

void paladin_t::init_gains()
{
  player_t::init_gains();

  gains_jotw = get_gain( "judgements_of_the_wise" );
}

void paladin_t::init_rng()
{
  player_t::init_rng();
}

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  // scales_with[ STAT_STAMINA ] = <- TODO: prot scales with stamina
  scales_with[ STAT_SPIRIT ] = 0;
}

void paladin_t::init_actions()
{
  if( action_list_str.empty() )
  {
    action_list_str = "flask,type=endless_rage/food,type=fish_feast/seal_of_blood/auto_attack";
    int num_items = items.size();
    for( int i=0; i < num_items; i++ )
    {
      if( items[ i ].use.active() )
      {
	action_list_str += "/use_item,name=";
	action_list_str += items[ i ].name();
      }
    }
    action_list_str += "/avenging_wrath/crusader_strike/hammer_of_wrath/judgement/divine_storm";
//    action_list_str += "/consecration/exorcism";

    action_list_default = 1;
  }

  player_t::init_actions();
}

void paladin_t::reset()
{
  player_t::reset();

  active_seal = SEAL_NONE;

  _buffs.reset();
  _cooldowns.reset();
  _expirations.reset();
}

void paladin_t::interrupt()
{
  player_t::interrupt();

  if( auto_attack ) auto_attack -> cancel();
}

double paladin_t::composite_attack_power()
{
  return player_t::composite_attack_power();
}

double paladin_t::composite_spell_power( int school )
{
  double sp = player_t::composite_spell_power( school );
  sp += composite_attack_power_multiplier() * composite_attack_power() * talents.sheath_of_light * 0.1;
  return sp;
}

bool paladin_t::get_talent_trees( std::vector<int*>& holy, std::vector<int*>& protection, std::vector<int*>& retribution )
{
  talent_translation_t translation[][3] =
  {
    { {  1, NULL                                     }, {  1, NULL                                      }, {  1, NULL                                  } },
    { {  2, NULL                                     }, {  2, &( talents.divine_strength              ) }, {  2, &( talents.benediction              ) } },
    { {  3, NULL                                     }, {  3, NULL                                      }, {  3, &( talents.improved_judgements      ) } },
    { {  4, NULL                                     }, {  4, NULL                                      }, {  4, &( talents.heart_of_the_crusader    ) } },
    { {  5, NULL                                     }, {  5, NULL                                      }, {  5, &( talents.improved_bom             ) } },
    { {  6, NULL                                     }, {  6, NULL                                      }, {  6, NULL                                  } },
    { {  7, NULL                                     }, {  7, NULL                                      }, {  7, &( talents.conviction               ) } },
    { {  8, NULL                                     }, {  8, NULL                                      }, {  8, &( talents.seal_of_command          ) } },
    { {  9, NULL                                     }, {  9, NULL                                      }, {  9, NULL                                  } },
    { { 10, NULL                                     }, { 10, NULL                                      }, { 10, NULL                                  } },
    { { 11, NULL                                     }, { 11, NULL                                      }, { 11, &( talents.sanctity_of_battle       ) } },
    { { 12, NULL                                     }, { 12, NULL                                      }, { 12, &( talents.crusade                  ) } },
    { { 13, NULL                                     }, { 13, NULL                                      }, { 13, &( talents.two_handed_weapon_spec   ) } },
    { { 14, NULL                                     }, { 14, NULL                                      }, { 14, &( talents.sanctified_retribution   ) } },
    { { 15, NULL                                     }, { 15, NULL                                      }, { 15, &( talents.vengeance                ) } },
    { { 16, NULL                                     }, { 16, NULL                                      }, { 16, NULL                                  } },
    { { 17, NULL                                     }, { 17, NULL                                      }, { 17, &( talents.the_art_of_war           ) } },
    { { 18, NULL                                     }, { 18, NULL                                      }, { 18, NULL                                  } },
    { { 19, NULL                                     }, { 19, NULL                                      }, { 19, &( talents.judgements_of_the_wise   ) } },
    { { 20, NULL                                     }, { 20, NULL                                      }, { 20, &( talents.fanaticism               ) } },
    { { 21, NULL                                     }, { 21, NULL                                      }, { 21, &( talents.sanctified_wrath         ) } },
    { { 22, NULL                                     }, { 22, NULL                                      }, { 22, &( talents.swift_retribution        ) } },
    { { 23, NULL                                     }, { 23, NULL                                      }, { 23, &( talents.crusader_strike          ) } },
    { { 24, NULL                                     }, { 24, NULL                                      }, { 24, &( talents.sheath_of_light          ) } },
    { { 25, NULL                                     }, { 25, NULL                                      }, { 25, &( talents.righteous_vengeance      ) } },
    { { 26, NULL                                     }, { 26, NULL                                      }, { 26, &( talents.divine_storm             ) } },
    { {  0, NULL                                     }, {  0, NULL                                      }, {  0, NULL                                  } }
  };

  return player_t::get_talent_trees( holy, protection, retribution, translation );
}

std::vector<option_t>& paladin_t::get_options()
{
  if( options.empty() )
  {
    player_t::get_options();

    option_t paladin_options[] =
    {
      // @option_doc loc=player/paladin/talents title="Talents"
      { "benediction",                    OPT_INT, &( talents.benediction                 ) },
      { "conviction",                     OPT_INT, &( talents.conviction                  ) },
      { "crusade",                        OPT_INT, &( talents.crusade                     ) },
      { "crusader_strike",                OPT_INT, &( talents.crusader_strike             ) },
      { "divine_storm",                   OPT_INT, &( talents.divine_storm                ) },
      { "divine_strength",                OPT_INT, &( talents.divine_strength             ) },
      { "fanaticism",                     OPT_INT, &( talents.fanaticism                  ) },
      { "heart_of_the_crusader",          OPT_INT, &( talents.heart_of_the_crusader       ) },
      { "improved_bom",                   OPT_INT, &( talents.improved_bom                ) },
      { "improved_judgements",            OPT_INT, &( talents.improved_judgements         ) },
      { "judgements_of_the_wise",         OPT_INT, &( talents.judgements_of_the_wise      ) },
      { "righteous_vengeance",            OPT_INT, &( talents.righteous_vengeance         ) },
      { "sanctified_retribution",         OPT_INT, &( talents.sanctified_retribution      ) },
      { "sanctified_wrath",               OPT_INT, &( talents.sanctified_wrath            ) },
      { "sanctity_of_battle",             OPT_INT, &( talents.sanctity_of_battle          ) },
      { "seal_of_command",                OPT_INT, &( talents.seal_of_command             ) },
      { "sheath_of_light",                OPT_INT, &( talents.sheath_of_light             ) },
      { "swift_retribution",              OPT_INT, &( talents.swift_retribution           ) },
      { "the_art_of_war",                 OPT_INT, &( talents.the_art_of_war              ) },
      { "two_handed_weapon_spec",         OPT_INT, &( talents.two_handed_weapon_spec      ) },
      { "vengeance",                      OPT_INT, &( talents.vengeance                   ) },
      // @option_doc loc=player/paladin/glyphs title="Glyphs"
      { "glyph_consecration",             OPT_INT, &( glyphs.consecration                 ) },
      { "glyph_judgement",                OPT_INT, &( glyphs.judgement                    ) },

      { NULL, OPT_UNKNOWN }
    };

    option_t::copy( options, paladin_options );
  }
  return options;
}

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
  if( name == "auto_attack"             ) return new auto_attack_t              ( this, options_str );
  if( name == "avenging_wrath"          ) return new avenging_wrath_t           ( this, options_str );
  if( name == "crusader_strike"         ) return new crusader_strike_t          ( this, options_str );
  if( name == "divine_storm"            ) return new divine_storm_t             ( this, options_str );
  if( name == "hammer_of_wrath"         ) return new hammer_of_wrath_t          ( this, options_str );
  if( name == "judgement"               ) return new judgement_t                ( this, options_str );
  if( name == "seal_of_blood"           ) return new seal_of_blood_t            ( this, options_str );
  if( name == "seal_of_the_martyr"      ) return new seal_of_blood_t            ( this, options_str );
  return player_t::create_action( name, options_str );
}

int paladin_t::primary_tree()
{
  return TREE_RETRIBUTION;
}

player_t* player_t::create_paladin( sim_t* sim, const std::string& name )
{
  return new paladin_t( sim, name );
}

