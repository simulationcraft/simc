// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Priest
// ==========================================================================

struct priest_t : public player_t
{
  action_t* active_devouring_plague;
  action_t* active_holy_fire;
  action_t* active_shadow_word_pain;
  action_t* active_vampiric_touch;
  action_t* active_vampiric_embrace;

  // Buffs
  struct _buffs_t
  {
    int inner_fire;
    int improved_spirit_tap;
    int shadow_weaving;
    int surge_of_light;
    int glyph_of_shadow;
    int devious_mind;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Expirations
  struct _expirations_t
  {
    event_t* glyph_of_shadow;
    event_t* improved_spirit_tap;
    event_t* shadow_weaving;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Cooldowns
  struct _cooldowns_t
  {
    double mind_blast;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _cooldowns_t ) ); }
    _cooldowns_t() { reset(); }
  };
  _cooldowns_t _cooldowns;

  // Gains
  gain_t* gains_shadow_fiend;
  gain_t* gains_dispersion;

  // Up-Times
  uptime_t* uptimes_improved_spirit_tap;
  uptime_t* uptimes_glyph_of_shadow;
  uptime_t* uptimes_devious_mind;
  uptime_t* uptimes_devious_mind_mf;

  // Random Number Generators
  rng_t* rng_pain_and_suffering;
  rng_t* rng_shadow_weaving;
  rng_t* rng_surge_of_light;

  struct talents_t
  {
    int  aspiration;
    int  darkness;
    int  dispersion;
    int  divine_fury;
    int  enlightenment;
    int  focused_mind;
    int  focused_power;
    int  force_of_will;
    int  holy_specialization;
    int  improved_devouring_plague;
    int  improved_inner_fire;
    int  improved_mind_blast;
    int  improved_power_word_fortitude;
    int  improved_shadow_word_pain;
    int  improved_spirit_tap;
    int  improved_vampiric_embrace;
    int  inner_focus;
    int  meditation;
    int  mental_agility;
    int  mental_strength;
    int  mind_flay;
    int  mind_melt;
    int  misery;
    int  pain_and_suffering;
    int  penance;
    int  power_infusion;
    int  searing_light;
    int  shadow_affinity;
    int  shadow_focus;
    int  shadow_form;
    int  shadow_power;
    int  shadow_weaving;
    int  spirit_of_redemption;
    int  spiritual_guidance;
    int  surge_of_light;
    int  twin_disciplines;
    int  twisted_faith;
    int  vampiric_embrace;
    int  vampiric_touch;
    int  veiled_shadows;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int hymn_of_hope;
    int shadow_word_death;
    int shadow_word_pain;
    int shadow;
    int smite;
    int penance;
    int dispersion;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  double devious_mind_delay;
  bool   use_shadow_word_death;

  priest_t( sim_t* sim, const std::string& name ) : player_t( sim, PRIEST, name )
  {
    // Active
    active_devouring_plague  = 0;
    active_holy_fire         = 0;
    active_shadow_word_pain  = 0;
    active_vampiric_touch    = 0;
    active_vampiric_embrace  = 0;

    // Tier 8 4-piece Delay
    devious_mind_delay = 0.0;
    use_shadow_word_death = false;
  }

  // Character Definition
  virtual void      init_glyphs();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_uptimes();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      reset();
  virtual bool      get_talent_trees( std::vector<int*>& discipline, std::vector<int*>& holy, std::vector<int*>& shadow );
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return ROLE_SPELL; }
  virtual int       primary_tree() SC_CONST     { return talents.shadow_form ? TREE_SHADOW : talents.penance ? TREE_DISCIPLINE : TREE_HOLY; }
  virtual void      regen( double periodicity );
};

enum devious_mind_states_t { DEVIOUS_MIND_STATE_NONE=0, DEVIOUS_MIND_STATE_WAITING, DEVIOUS_MIND_STATE_ACTIVE };

namespace   // ANONYMOUS NAMESPACE ==========================================
{

// ==========================================================================
// Priest Spell
// ==========================================================================

struct priest_spell_t : public spell_t
{
  priest_spell_t( const char* n, player_t* player, int s, int t ) :
      spell_t( n, player, RESOURCE_MANA, s, t )
  {
    priest_t* p = player -> cast_priest();
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_crit       += p -> talents.force_of_will * 0.01;
  }

  virtual double haste() SC_CONST;
  virtual void   schedule_execute();
  virtual void   execute();
  virtual void   player_buff();
};

// ==========================================================================
// Pet Shadow Fiend
// ==========================================================================

struct shadow_fiend_pet_t : public pet_t
{
  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) :
        attack_t( "melee", player, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      weapon_multiplier = 0;
      direct_power_mod = 0.379;
      base_spell_power_multiplier = 1.0;
      base_attack_power_multiplier = 0.0;
      base_dd_multiplier = 1.15; // Shadowcrawl
      base_dd_min = 177;
      base_dd_max = 209;
      background = true;
      repeating  = true;
      may_dodge  = false;
      may_miss   = false;
      may_parry  = false;
      may_crit   = true;
      may_block  = true;
    }
    void assess_damage( double amount, int dmg_type )
    {
      attack_t::assess_damage( amount, dmg_type );
      priest_t* p = player -> cast_pet() -> owner -> cast_priest();
      p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.05, p -> gains_shadow_fiend );
    }
  };

  melee_t* melee;

  shadow_fiend_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "shadow_fiend" ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 100;
    main_hand_weapon.swing_time = 1.5;
    main_hand_weapon.school     = SCHOOL_SHADOW;

    stamina_per_owner = 0.51;
    intellect_per_owner = 0.30;
  }
  virtual void init_base()
  {
    pet_t::init_base();

    attribute_base[ ATTR_STRENGTH  ] = 153;
    attribute_base[ ATTR_AGILITY   ] = 108;
    attribute_base[ ATTR_STAMINA   ] = 280;
    attribute_base[ ATTR_INTELLECT ] = 133;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;

    if ( owner -> set_bonus.tier4_2pc() ) attribute_base[ ATTR_STAMINA ] += 75;

    melee = new melee_t( this );
  }
  virtual double composite_spell_power( int school ) SC_CONST
  {
    double sp = owner -> composite_spell_power( school );
    sp -= owner -> spell_power_per_spirit * owner -> spirit();
    return sp;
  }
  virtual void schedule_ready( double delta_time=0,
                               bool   waiting=false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! melee -> execute_event ) melee -> execute();
  }
  virtual void interrupt()
  {
    pet_t::interrupt();
    melee -> cancel();
  }
};

// stack_shadow_weaving =====================================================

static void stack_shadow_weaving( spell_t* s )
{
  priest_t* p = s -> player -> cast_priest();

  if ( p -> talents.shadow_weaving == 0 ) return;

  struct shadow_weaving_expiration_t : public event_t
  {
    shadow_weaving_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Shadow Weaving Expiration";
      sim -> add_event( this, 15.01 );
    }
    virtual void execute()
    {
      if ( sim -> log ) log_t::output( sim, "%s loses Shadow Weaving", sim -> target -> name() );
      priest_t* p = player -> cast_priest();
      p -> _buffs.shadow_weaving = 0;
      p -> _expirations.shadow_weaving = 0;
    }
  };

  if ( p -> rng_shadow_weaving -> roll( p -> talents.shadow_weaving * ( 1.0/3 ) ) )
  {
    sim_t* sim = s -> sim;

    if ( p -> _buffs.shadow_weaving < 5 )
    {
      p -> _buffs.shadow_weaving++;
      if ( sim -> log ) log_t::output( sim, "%s gains Shadow Weaving %d", p -> name(), p -> _buffs.shadow_weaving );
    }

    event_t*& e = p -> _expirations.shadow_weaving;

    if ( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new ( s -> sim ) shadow_weaving_expiration_t( s -> sim, p );
    }
  }
}

// trigger_misery ==============================================

static void trigger_misery( action_t* a )
{
  priest_t* p = a -> player -> cast_priest();

  if ( ! p -> talents.misery )
    return;

  if ( a -> sim -> target -> debuffs.misery > p -> talents.misery )
    return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p, int misery_stacks ) : event_t( sim, p )
    {
      sim -> target -> debuffs.misery = misery_stacks;
      sim -> add_event( this, 24.0 );
    }
    virtual void execute()
    {
      sim -> target -> debuffs.misery = 0;
      sim -> target -> expirations.misery = 0;
    }
  };

  event_t*& e = a -> sim -> target -> expirations.misery;

  if ( e )
  {
    e -> reschedule( 24.0 );
  }
  else
  {
    e = new ( a -> sim ) expiration_t( a -> sim, a -> player, p -> talents.misery );
  }
}

// push_tier5_2pc =============================================================

static void push_tier5_2pc( spell_t*s )
{
  priest_t* p = s -> player -> cast_priest();

  assert( p -> buffs.tier5_2pc == 0 );

  if ( p -> set_bonus.tier5_2pc() && p -> rngs.tier5_2pc -> roll( 0.06 ) )
  {
    p -> buffs.tier5_2pc = 1;
    p -> buffs.mana_cost_reduction += 150;
    p -> procs.tier5_2pc -> occur();
  }

}

// pop_tier5_2pc =============================================================

static void pop_tier5_2pc( spell_t*s )
{
  priest_t* p = s -> player -> cast_priest();

  if ( p -> buffs.tier5_2pc )
  {
    p -> buffs.tier5_2pc = 0;
    p -> buffs.mana_cost_reduction -= 150;
  }
}

// push_tier5_4pc =============================================================

static void push_tier5_4pc( spell_t*s )
{
  priest_t* p = s -> player -> cast_priest();

  if ( ! p -> buffs.tier5_4pc &&
       p -> set_bonus.tier5_4pc() &&
       p -> rngs.tier5_4pc -> roll( 0.40 ) )
  {
    p -> buffs.tier5_4pc = 1;
    p -> spell_power[ SCHOOL_MAX ] += 100;
    p -> procs.tier5_4pc -> occur();
  }
}

// pop_tier5_4pc =============================================================

static void pop_tier5_4pc( spell_t*s )
{
  priest_t* p = s -> player -> cast_priest();

  if ( p -> buffs.tier5_4pc )
  {
    p -> buffs.tier5_4pc = 0;
    p -> spell_power[ SCHOOL_MAX ] -= 100;
  }
}

// trigger_surge_of_light =====================================================

static void trigger_surge_of_light( spell_t* s )
{
  priest_t* p = s -> player -> cast_priest();

  if ( p -> talents.surge_of_light )
    if ( p -> rng_surge_of_light -> roll( p -> talents.surge_of_light * 0.25 ) )
      p -> _buffs.surge_of_light = 1;
}

// trigger_improved_spirit_tap ================================================

static void trigger_improved_spirit_tap( spell_t* s )
{
  struct expiration_t : public event_t
  {
    double spirit_bonus;
    expiration_t( sim_t* sim, priest_t* p ) : event_t( sim, p )
    {
      name = "Improved Spirit Tap Expiration";
      p -> _buffs.improved_spirit_tap = 1;
      p -> aura_gain( "improved_spirit_tap" );
      spirit_bonus = p -> spirit() * p -> talents.improved_spirit_tap * 0.05;
      p -> attribute[ ATTR_SPIRIT ] += spirit_bonus;
      sim -> add_event( this, 8.0 );
    }
    virtual void execute()
    {
      priest_t* p = player -> cast_priest();
      p -> _buffs.improved_spirit_tap = 0;
      p -> aura_loss( "improved_spirit_tap" );
      p -> attribute[ ATTR_SPIRIT ] -= spirit_bonus;
      p -> _expirations.improved_spirit_tap = 0;
    }
  };

  priest_t* p = s -> player -> cast_priest();

  if ( p -> talents.improved_spirit_tap )
  {
    event_t*& e = p -> _expirations.improved_spirit_tap;

    if ( e )
    {
      e -> reschedule( 8.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_glyph_of_shadow ====================================================

static void trigger_glyph_of_shadow( spell_t* s )
{
  struct expiration_t : public event_t
  {
    double spellpower_bonus;
    expiration_t( sim_t* sim, priest_t* p ) : event_t( sim, p )
    {
      name = "Glyph of Shadow Expiration";
      p -> _buffs.glyph_of_shadow = 1;
      p -> aura_gain( "glyph_of_shadow" );
      spellpower_bonus = p -> spirit() * 0.1;
      p -> spell_power[ SCHOOL_MAX ] += spellpower_bonus;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      priest_t* p = player -> cast_priest();
      p -> _buffs.glyph_of_shadow = 0;
      p -> aura_loss( "glyph_of_shadow" );
      p -> spell_power[ SCHOOL_MAX ] -= spellpower_bonus;
      p -> _expirations.glyph_of_shadow = 0;
    }
  };

  priest_t* p = s -> player -> cast_priest();

  if ( p -> glyphs.shadow && p -> buffs.shadow_form )
  {
    event_t*& e = p -> _expirations.glyph_of_shadow;

    if ( e )
    {
      e -> reschedule( 10.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_devious_mind ====================================================

static void trigger_devious_mind( spell_t* s )
{
  struct devious_mind_expiration_t : public event_t
  {
    devious_mind_expiration_t( sim_t* sim, priest_t* p ) : event_t( sim, p )
    {
      name = "Devious Mind Expiration";
      p -> _buffs.devious_mind = DEVIOUS_MIND_STATE_ACTIVE;
      p -> aura_gain( "devious_mind" );
      p -> haste_rating += 240;
      p -> recalculate_haste();
      sim -> add_event( this, 4.0 );
    }
    virtual void execute()
    {
      priest_t* p = player -> cast_priest();
      p -> _buffs.devious_mind = DEVIOUS_MIND_STATE_NONE;
      p -> aura_loss( "devious_mind" );
      p -> haste_rating -= 240;
      p -> recalculate_haste();
    }
  };

  struct devious_mind_delay_t : public event_t
  {
    devious_mind_delay_t( sim_t* sim, priest_t* p ) : event_t( sim, p )
    {
      name = "Devious Mind Delay";
      p -> _buffs.devious_mind = DEVIOUS_MIND_STATE_WAITING;
      sim -> add_event( this, p -> devious_mind_delay );
    }
    virtual void execute()
    {
      priest_t* p = player -> cast_priest();
      new ( sim ) devious_mind_expiration_t( sim, p );
    }
  };

  priest_t* p = s -> player -> cast_priest();

  if ( p -> set_bonus.tier8_4pc() )
  {
    if ( p -> devious_mind_delay < 0.01 )
    {
      new ( s -> sim ) devious_mind_expiration_t( s -> sim, p );
    }
    else
    {
      new ( s -> sim ) devious_mind_delay_t( s -> sim, p );
    }
  }
}

// trigger_replenishment ===================================================

static void trigger_replenishment( spell_t* s )
{
  priest_t* p = s -> player -> cast_priest();

  if ( p -> active_vampiric_touch )
  {
    p -> trigger_replenishment();
  }
}

// ==========================================================================
// Priest Spell
// ==========================================================================

// priest_spell_t::haste ====================================================

double priest_spell_t::haste() SC_CONST
{
  priest_t* p = player -> cast_priest();
  double h = spell_t::haste();
  if ( p -> talents.enlightenment )
  {
    h *= 1.0 / ( 1.0 + p -> talents.enlightenment * 0.01 );
  }
  return h;
}

// priest_spell_t::schedule_execute =========================================

void priest_spell_t::schedule_execute()
{
  priest_t* p = player -> cast_priest();

  spell_t::schedule_execute();

  p -> uptimes_devious_mind -> update( p -> _buffs.devious_mind == DEVIOUS_MIND_STATE_ACTIVE );
}

// priest_spell_t::execute ==================================================

void priest_spell_t::execute()
{
  priest_t* p = player -> cast_priest();

  spell_t::execute();

  if ( result_is_hit() )
  {
    if ( school == SCHOOL_SHADOW )
    {
      stack_shadow_weaving( this );
    }
    if ( result == RESULT_CRIT )
    {
      trigger_surge_of_light( this );
      trigger_glyph_of_shadow( this );
    }
  }

  if ( harmful )
  {
    pop_tier5_2pc ( this );
    push_tier5_2pc( this );
  }
  pop_tier5_4pc( this );

  p -> uptimes_improved_spirit_tap -> update( p -> _buffs.improved_spirit_tap != 0 );
  p -> uptimes_glyph_of_shadow     -> update( p -> _buffs.glyph_of_shadow     != 0 );
}

// priest_spell_t::player_buff ==============================================

void priest_spell_t::player_buff()
{
  priest_t* p = player -> cast_priest();

  spell_t::player_buff();

  if ( school == SCHOOL_SHADOW )
  {
    if ( p -> _buffs.shadow_weaving )
    {
      player_multiplier *= 1.0 + p -> _buffs.shadow_weaving * 0.02;
    }
  }

  if ( p -> talents.focused_power )
  {
    player_multiplier *= 1.0 + p -> talents.focused_power * 0.02;
  }
}

// Holy Fire Spell ===========================================================

struct holy_fire_t : public priest_spell_t
{
  holy_fire_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "holy_fire", player, SCHOOL_HOLY, TREE_HOLY )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 12, 900, 1140, 50, 0.11 }
      , // Dummy rank for level 80.
      { 78, 11, 890, 1130, 50, 0.11 },
      { 72, 10, 732,  928, 47, 0.11 },
      { 66,  9, 412,  523, 33, 0.11 },
      { 60,  8, 355,  449, 29, 0.13 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 2.0;
    base_tick_time    = 1.0;
    num_ticks         = 7;
    cooldown          = 10;
    direct_power_mod  = 0.5715;
    tick_power_mod    = 0.1678 / 7;

    may_crit           = true;
    base_execute_time -= p -> talents.divine_fury * 0.01;
    base_multiplier   *= 1.0 + p -> talents.searing_light * 0.05;
    base_crit         += p -> talents.holy_specialization * 0.01;

    observer          = &( p -> active_holy_fire );
  }
};

// Smite Spell ================================================================

struct smite_t : public priest_spell_t
{
  smite_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "smite", player, SCHOOL_HOLY, TREE_HOLY )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 13, 713, 799, 0, 0.15 }
      , // Dummy rank for level 80
      { 79, 12, 707, 793, 0, 0.15 },
      { 75, 11, 604, 676, 0, 0.15 },
      { 69, 10, 545, 611, 0, 0.15 },
      { 61,  9, 405, 455, 0, 0.17 },
      { 54,  8, 371, 415, 0, 0.17 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 2.5;
    direct_power_mod  = base_execute_time / 3.5;
    may_crit          = true;

    base_execute_time -= p -> talents.divine_fury * 0.1;
    base_multiplier   *= 1.0 + p -> talents.searing_light * 0.05;
    base_crit         += p -> talents.holy_specialization * 0.01;

    if ( p -> set_bonus.tier4_4pc() ) base_multiplier *= 1.05;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    player -> cast_priest() -> _buffs.surge_of_light = 0;
  }
  virtual double execute_time() SC_CONST
  {
    priest_t* p = player -> cast_priest();
    return p -> _buffs.surge_of_light ? 0 : priest_spell_t::execute_time();
  }

  virtual double cost() SC_CONST
  {
    priest_t* p = player -> cast_priest();
    return p -> _buffs.surge_of_light ? 0 : priest_spell_t::cost();
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();
    priest_t* p = player -> cast_priest();
    may_crit = ! ( p -> _buffs.surge_of_light );
    if ( p -> active_holy_fire && p -> glyphs.smite ) player_multiplier *= 1.20;
  }
};

// Penance Spell ===============================================================

struct penance_tick_t : public priest_spell_t
{
  penance_tick_t( player_t* player ) :
      priest_spell_t( "penance", player, SCHOOL_HOLY, TREE_HOLY )
  {
    priest_t* p = player -> cast_priest();

    static rank_t ranks[] =
    {
      { 80, 4, 288, 288, 0, 0 },
      { 76, 3, 256, 256, 0, 0 },
      { 68, 2, 224, 224, 0, 0 },
      { 60, 1, 184, 184, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    dual       = true;
    background = true;
    may_crit   = true;

    direct_power_mod  = 0.8 / 3.5;

    base_multiplier *= 1.0 + p -> talents.searing_light * 0.05 + p -> talents.twin_disciplines * 0.01;
    base_crit       += p -> talents.holy_specialization * 0.01;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    tick_dmg = direct_dmg;
    update_stats( DMG_OVER_TIME );
  }
};

struct penance_t : public priest_spell_t
{
  spell_t* penance_tick;

  penance_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "penance", player, SCHOOL_HOLY, TREE_HOLY )
  {
    priest_t* p = player -> cast_priest();
    check_talent( p -> talents.penance );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_miss = may_resist = false;

    base_cost         = 0.16 * p -> resource_base[ RESOURCE_MANA ];
    base_execute_time = 0.0;
    channeled         = true;
    tick_zero         = true;
    num_ticks         = 2;
    base_tick_time    = 1.0;

    cooldown        = 12;

    if ( p -> glyphs.penance )
    {
      cooldown       -= 2;
    }

    cooldown         *= 1.0 - p -> talents.aspiration * 0.10;

    penance_tick = new penance_tick_t( p );
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    penance_tick -> execute();
  }
};

// Shadow Word Pain Spell ======================================================

struct shadow_word_pain_t : public priest_spell_t
{
  int shadow_weaving_wait;

  shadow_word_pain_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "shadow_word_pain", player, SCHOOL_SHADOW, TREE_SHADOW ), shadow_weaving_wait( 0 )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "shadow_weaving_wait", OPT_BOOL, &shadow_weaving_wait },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 12, 0, 0, 230, 0.22 },
      { 75, 11, 0, 0, 196, 0.22 },
      { 70, 10, 0, 0, 186, 0.22  },
      { 65,  9, 0, 0, 151, 0.25  },
      { 58,  8, 0, 0, 128, 0.25  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 6;
    tick_power_mod    = base_tick_time / 15.0;
    tick_power_mod   *= 0.915;  // Nerf Bat! Determined to be 0.915 after more rigorous testing.
    base_cost        *= 1.0 - ( util_t::talent_rank( p -> talents.mental_agility, 3, 0.04, 0.07, 0.10 ) +
                                p -> talents.shadow_focus    * 0.02 );

    base_multiplier *= 1.0 + ( p -> talents.darkness                  * 0.02 +
                               p -> talents.twin_disciplines          * 0.01 +
                               p -> talents.improved_shadow_word_pain * 0.03 );
    base_hit  += p -> talents.shadow_focus * 0.01;
    base_crit += p -> talents.mind_melt * 0.03;

    if ( p -> set_bonus.tier6_2pc() ) num_ticks++;

    observer = &( p -> active_shadow_word_pain );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    tick_may_crit = p -> buffs.shadow_form != 0;
    base_crit_bonus_multiplier = 1.0 + ( p -> buffs.shadow_form != 0 );
    priest_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_misery( this );
    }
  }

  virtual void tick()
  {
    priest_spell_t::tick();
    push_tier5_4pc( this );
  }

  virtual void last_tick()
  {
    priest_spell_t::last_tick();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( ! priest_spell_t::ready() )
      return false;

    if ( shadow_weaving_wait && ( p -> talents.shadow_weaving != 0 ) && p -> _buffs.shadow_weaving < 5 )
      return false;

    return true;
  }
};

// Vampiric Touch Spell ======================================================

struct vampiric_touch_t : public priest_spell_t
{
  vampiric_touch_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "vampiric_touch", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    check_talent( p -> talents.vampiric_touch );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 0, 0, 170, 0.16 },
      { 75, 4, 0, 0, 147, 0.16 },
      { 70, 3, 0, 0, 130, 0.16  },
      { 60, 2, 0, 0, 120, 0.18  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 1.5;
    base_tick_time    = 3.0;
    num_ticks         = 5;
    if ( p -> set_bonus.tier9_2pc() ) num_ticks += 2;
    tick_power_mod    = base_tick_time / 15.0;
    tick_power_mod   *= 2.0;

    base_cost       *= 1.0 - p -> talents.shadow_focus * 0.02;
    base_cost        = floor( base_cost );
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_hit        += p -> talents.shadow_focus * 0.01;
    base_crit       += p -> talents.mind_melt * 0.03;

    observer = &( p -> active_vampiric_touch );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    tick_may_crit = p -> buffs.shadow_form != 0;
    base_crit_bonus_multiplier = 1.0 + ( p -> buffs.shadow_form != 0 );
    priest_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_misery( this );
    }
  }

  virtual void last_tick()
  {
    priest_spell_t::last_tick();
  }
};

// Devouring Plague Spell ======================================================

struct devouring_plague_burst_t : public priest_spell_t
{
  devouring_plague_burst_t( player_t* player ) :
      priest_spell_t( "devouring_plague", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    static rank_t ranks[] =
    {
      { 79, 9, 172, 172, 0, 0 },
      { 73, 8, 143, 143, 0, 0 },
      { 68, 7, 136, 136, 0, 0 },
      { 60, 6, 113, 113, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    // burst = 5%-15% of total periodic damage from 8 ticks

    dual       = true;
    proc       = true;
    background = true;
    may_crit   = true;

    base_multiplier *= 8.0 * p -> talents.improved_devouring_plague * 0.05;

    direct_power_mod  = 3.0 / 15.0;
    direct_power_mod *= 0.925;

    base_multiplier  *= 1.0 + ( p -> talents.darkness                  * 0.02 +
                                p -> talents.twin_disciplines          * 0.01 +
                                p -> talents.improved_devouring_plague * 0.05 );

    base_hit += p -> talents.shadow_focus * 0.01;

    // This helps log file and decouples the sooth RNG from the ticks.
    name_str = "devouring_plague_burst";
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    update_stats( DMG_DIRECT );
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::player_buff();
    player_spell_power -= p -> spell_power_per_spirit * p -> spirit();
  }

  virtual void target_debuff( int dmg_type )
  {
    priest_spell_t::target_debuff( dmg_type );
    if ( sim -> target -> debuffs.crypt_fever ) target_multiplier *= 1.30;
  }
};

struct devouring_plague_t : public priest_spell_t
{
  spell_t* devouring_plague_burst;

  devouring_plague_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "devouring_plague", player, SCHOOL_SHADOW, TREE_SHADOW ), devouring_plague_burst( 0 )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 9, 0, 0, 172, 0.25 },
      { 73, 8, 0, 0, 143, 0.25 },
      { 68, 7, 0, 0, 136, 0.25 },
      { 60, 6, 0, 0, 113, 0.28 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 8;
    cooldown          = 0.0;
    binary            = true;
    tick_power_mod    = base_tick_time / 15.0;
    tick_power_mod   *= 0.925;
    base_cost        *= 1.0 - ( util_t::talent_rank( p -> talents.mental_agility, 3, 0.04, 0.07, 0.10 ) + p -> talents.shadow_focus * 0.02 );
    base_cost         = floor( base_cost );
    base_multiplier  *= 1.0 + ( p -> talents.darkness                  * 0.02 +
                                p -> talents.twin_disciplines          * 0.01 +
                                p -> talents.improved_devouring_plague * 0.05 +
                                p -> set_bonus.tier8_2pc()             * 0.15 );
    base_hit         += p -> talents.shadow_focus * 0.01;
    base_crit        += p -> talents.mind_melt * 0.03;

    if ( p -> talents.improved_devouring_plague )
    {
      devouring_plague_burst = new devouring_plague_burst_t( p );
    }

    observer = &( p -> active_devouring_plague );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    tick_may_crit = p -> buffs.shadow_form != 0;
    base_crit_bonus_multiplier = 1.0 + ( p -> buffs.shadow_form != 0 );
    priest_spell_t::execute();
    if ( devouring_plague_burst ) devouring_plague_burst -> execute();
  }

  virtual void tick()
  {
    priest_spell_t::tick();
    player -> resource_gain( RESOURCE_HEALTH, tick_dmg * 0.15 );
  }

  virtual void target_debuff( int dmg_type )
  {
    priest_spell_t::target_debuff( dmg_type );
    if ( sim -> target -> debuffs.crypt_fever ) target_multiplier *= 1.30;
  }

  virtual void update_stats( int type )
  {
    if ( devouring_plague_burst && type == DMG_DIRECT ) return;
    priest_spell_t::update_stats( type );
  }
};

// Vampiric Embrace Spell ======================================================

struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "vampiric_embrace", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    check_talent( p -> talents.vampiric_embrace );

    static rank_t ranks[] =
    {
      { 1, 1, 0, 0, 0, 0.00 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0;
    base_tick_time    = 300.0;
    num_ticks         = 1;
    cooldown          = 0;
    base_cost         = 0.0;
    base_cost         = floor( base_cost );
    base_multiplier   = 0;
    base_hit          = p -> talents.shadow_focus * 0.01;

    observer = &( p -> active_vampiric_embrace );
  }
};

// Mind Blast Spell ============================================================

struct mind_blast_t : public priest_spell_t
{
  mind_blast_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "mind_blast", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 14, 997, 1053, 0, 0.17 }
      , // Dummy rank for level 80 characters.
      { 79, 13, 992, 1048, 0, 0.17 },
      { 74, 12, 837,  883, 0, 0.17 },
      { 69, 11, 708,  748, 0, 0.17 },
      { 63, 10, 557,  587, 0, 0.19 },
      { 58,  9, 503,  531, 0, 0.19 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 1.5;
    cooldown          = 8.0;
    may_crit          = true;
    direct_power_mod  = base_execute_time / 3.5;

    base_cost        *= 1.0 - ( p -> talents.focused_mind * 0.05 +
                                p -> talents.shadow_focus * 0.02 +
                                p -> set_bonus.tier7_2pc() ? 0.1 : 0.0  );
    base_cost         = floor( base_cost );
    base_multiplier  *= 1.0 + p -> talents.darkness * 0.02;
    base_hit         += p -> talents.shadow_focus * 0.01;
    base_crit        += p -> talents.mind_melt * 0.02;
    cooldown         -= p -> talents.improved_mind_blast * 0.5;
    direct_power_mod *= 1.0 + p -> talents.misery * 0.05;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.shadow_power * 0.20;

    if ( p -> set_bonus.tier6_4pc() ) base_multiplier *= 1.10;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    priest_t* p = player -> cast_priest();
    if ( result_is_hit() )
    {
      trigger_devious_mind( this );
      trigger_replenishment( this );
      if ( result == RESULT_CRIT )
      {
        trigger_improved_spirit_tap( this );
      }
    }
    p -> _cooldowns.mind_blast = cooldown_ready;
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();
    priest_t* p = player -> cast_priest();
    if ( p -> talents.twisted_faith )
    {
      if ( p -> active_shadow_word_pain ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.02;
    }
  }
};

// Shadow Word Death Spell ======================================================

struct shadow_word_death_t : public priest_spell_t
{
  double mb_wait;
  int    mb_priority;
  int    devious_mind_filler;

  shadow_word_death_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "shadow_word_death", player, SCHOOL_SHADOW, TREE_SHADOW ), mb_wait( 0 ), mb_priority( 0 ), devious_mind_filler( 0 )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "mb_wait",             OPT_FLT,  &mb_wait             },
      { "mb_priority",         OPT_BOOL, &mb_priority         },
      { "devious_mind_filler", OPT_BOOL, &devious_mind_filler },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 4, 750, 870, 0, 0.12 },
      { 75, 3, 639, 741, 0, 0.12 },
      { 70, 2, 572, 664, 0, 0.12 },
      { 62, 1, 450, 522, 0, 0.14 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0;
    may_crit          = true;
    cooldown          = 12.0;
    direct_power_mod  = ( 1.5/3.5 );
    base_cost        *= 1.0 - ( util_t::talent_rank( p -> talents.mental_agility, 3, 0.04, 0.07, 0.10 ) +
                                p -> talents.shadow_focus * 0.02 );
    base_cost         = floor( base_cost );
    base_multiplier  *= 1.0 + p -> talents.darkness * 0.02 + p -> talents.twin_disciplines * 0.01;
    base_hit         += p -> talents.shadow_focus * 0.01;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.shadow_power * 0.20;

    if ( p -> set_bonus.tier7_4pc() ) base_crit += 0.1;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::execute();
    if ( result_is_hit() )
    {
      p -> resource_loss( RESOURCE_HEALTH, direct_dmg * ( 1.0 - p -> talents.pain_and_suffering * 0.20 ) );
      if ( result == RESULT_CRIT )
      {
        trigger_improved_spirit_tap( this );
      }
    }
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::player_buff();
    if ( p -> glyphs.shadow_word_death )
    {
      if ( sim -> target -> health_percentage() < 35 )
      {
        player_multiplier *= 1.1;
      }
    }
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( ! priest_spell_t::ready() )
      return false;

    if ( mb_wait )
    {
      if ( ( p -> _cooldowns.mind_blast - sim -> current_time ) < mb_wait )
        return false;
    }

    if ( mb_priority )
    {
      if ( ( p -> _cooldowns.mind_blast - sim -> current_time ) > ( haste() * 1.5 + sim -> gcd_lag + mb_wait ) )
        return false;
    }

    if ( devious_mind_filler )
    {
      if ( p -> _buffs.devious_mind != DEVIOUS_MIND_STATE_WAITING )
        return false;
    }

    return true;
  }
};

// Mind Flay Spell ============================================================

struct mind_flay_tick_t : public priest_spell_t
{
  mind_flay_tick_t( player_t* player ) :
      priest_spell_t( "mind_flay", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    static rank_t ranks[] =
    {
      { 80, 9, 196, 196, 0, 0 },
      { 74, 8, 164, 164, 0, 0 },
      { 68, 7, 150, 150, 0, 0 },
      { 60, 6, 121, 121, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    dual              = true;
    background        = true;
    may_crit          = true;
    direct_power_mod  = 1.0 / 3.5;
    direct_power_mod *= 0.9;  // Nerf Bat!
    direct_power_mod *= 1.0 + p -> talents.misery * 0.05;
    base_hit         += p -> talents.shadow_focus * 0.01;
    base_multiplier  *= 1.0 + ( p -> talents.darkness         * 0.02 +
                                p -> talents.twin_disciplines * 0.01 );
    base_crit        += p -> talents.mind_melt * 0.02;

    if ( p -> set_bonus.tier9_4pc() )
    {
      base_crit += 0.05;
    }

    base_crit_bonus_multiplier *= 1.0 + p -> talents.shadow_power * 0.20;

    if ( p -> set_bonus.tier4_4pc() ) base_multiplier *= 1.05;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::execute();
    tick_dmg = direct_dmg;
    update_stats( DMG_OVER_TIME );
    if ( result_is_hit() )
    {
      if ( p -> active_shadow_word_pain )
      {
        if ( p -> rng_pain_and_suffering -> roll( p -> talents.pain_and_suffering * ( 1.0/3.0 ) ) )
        {
          p -> active_shadow_word_pain -> refresh_duration();
        }
      }
    }
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::player_buff();
    if ( p -> talents.twisted_faith )
    {
      if ( p -> active_shadow_word_pain )
      {
        player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.02;
        if ( p -> glyphs.shadow_word_pain ) player_multiplier *= 1.10;
      }
    }
  }
};

struct mind_flay_t : public priest_spell_t
{
  spell_t* mind_flay_tick;

  double mb_wait;
  int    swp_refresh;
  int    devious_mind_wait;
  int    devious_mind_priority;

  mind_flay_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "mind_flay", player, SCHOOL_SHADOW, TREE_SHADOW ), mb_wait( 0 ), swp_refresh( 0 ), devious_mind_wait( 0 ), devious_mind_priority( 0 )
  {
    priest_t* p = player -> cast_priest();
    check_talent( p -> talents.mind_flay );

    option_t options[] =
    {
      { "swp_refresh",           OPT_BOOL, &swp_refresh           },
      { "mb_wait",               OPT_FLT,  &mb_wait               },
      { "devious_mind_wait",     OPT_BOOL, &devious_mind_wait     },
      { "devious_mind_priority", OPT_BOOL, &devious_mind_priority },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    channeled      = true;
    num_ticks      = 3;
    base_tick_time = 1.0;

    base_cost  = 0.09 * p -> resource_base[ RESOURCE_MANA ];
    base_cost *= 1.0 - ( p -> talents.focused_mind * 0.05 +
                         p -> talents.shadow_focus * 0.02 );
    base_cost  = floor( base_cost );
    base_hit  += p -> talents.shadow_focus * 0.01;

    mind_flay_tick = new mind_flay_tick_t( p );
  }

  virtual void schedule_execute()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::schedule_execute();
    p -> uptimes_devious_mind_mf -> update( p -> _buffs.devious_mind == DEVIOUS_MIND_STATE_ACTIVE );
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_misery( this );
    }
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    mind_flay_tick -> execute();
    update_time( DMG_OVER_TIME );
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( ! priest_spell_t::ready() )
      return false;

    if ( swp_refresh )
    {
      if ( ! p -> active_shadow_word_pain )
        return false;

      if ( ( p -> active_shadow_word_pain -> num_ticks -
             p -> active_shadow_word_pain -> current_tick ) > 2 )
        return false;
    }

    if ( mb_wait )
    {
      if ( ( p -> _cooldowns.mind_blast - sim -> current_time ) < mb_wait )
        return false;
    }

    if ( devious_mind_wait )
    {
      if ( p -> set_bonus.tier8_4pc() && p -> _buffs.devious_mind == DEVIOUS_MIND_STATE_WAITING )
        return false;
    }

    if ( devious_mind_priority )
    {
      if ( p -> set_bonus.tier8_4pc() && p -> _buffs.devious_mind != DEVIOUS_MIND_STATE_ACTIVE )
        return false;
    }

    return true;
  }
};

// Dispersion Spell ============================================================

struct dispersion_t : public priest_spell_t
{
  dispersion_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "dispersion", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    check_talent( p -> talents.dispersion );

    base_execute_time = 0.0;
    base_tick_time    = 1.0;
    num_ticks         = 6;
    channeled         = true;
    harmful           = false;
    base_cost         = 0;
    cooldown          = 120;

    if ( p -> glyphs.dispersion )
    {
      cooldown        -= 45;
    }
  }

  virtual void tick()
  {
    priest_t* p = player -> cast_priest();
    p -> resource_gain( RESOURCE_MANA, 0.06 * p -> resource_max[ RESOURCE_MANA ], p -> gains_dispersion );
    priest_spell_t::tick();
  }

  virtual bool ready()
  {
    double consumption_rate, time_to_oom;
    double fudge_factor = 1.1;

    if ( ! priest_spell_t::ready() )
      return false;

    consumption_rate  = ( player -> resource_initial[ RESOURCE_MANA ] -
                          player -> resource_current[ RESOURCE_MANA ] ) /
                        sim -> current_time;
    consumption_rate *= fudge_factor;

    time_to_oom = player -> resource_current[ RESOURCE_MANA ] / consumption_rate;

    if ( sim -> target -> time_to_die() < time_to_oom )
      return false;

    if ( player -> buffs.bloodlust && ( time_to_oom > 45.0 ) )
      return false;

    return player -> resource_current[ RESOURCE_MANA ] < 0.50 * player -> resource_max[ RESOURCE_MANA ];
  }
};

// Power Infusion Spell =====================================================

struct power_infusion_t : public priest_spell_t
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Power Infusion Expiration";
      player -> aura_gain( "Power Infusion" );
      player -> buffs.power_infusion = 1;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Power Infusion" );
      player -> buffs.power_infusion = 0;
    }
  };

  power_infusion_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "power_infusion", player, SCHOOL_ARCANE, TREE_DISCIPLINE )
  {
    priest_t* p = player -> cast_priest();
    check_talent( p -> talents.power_infusion );
    trigger_gcd = 0;
    cooldown = 120.0;
    cooldown *= 1.0 - p -> talents.aspiration * 0.10;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new ( sim ) expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if ( ! priest_spell_t::ready() )
      return false;

    if ( player -> buffs.bloodlust )
      return false;

    return true;
  }
};

// Inner Focus Spell =====================================================

struct inner_focus_t : public priest_spell_t
{
  action_t* free_action;

  inner_focus_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "inner_focus", player, SCHOOL_HOLY, TREE_DISCIPLINE )
  {
    priest_t* p = player -> cast_priest();
    check_talent( p -> talents.inner_focus );

    cooldown = 180.0;
    cooldown *= 1.0 - p -> talents.aspiration * 0.10;

    std::string spell_name    = options_str;
    std::string spell_options = "";

    std::string::size_type cut_pt = spell_name.find_first_of( "," );

    if ( cut_pt != spell_name.npos )
    {
      spell_options = spell_name.substr( cut_pt + 1 );
      spell_name    = spell_name.substr( 0, cut_pt );
    }

    free_action = p -> create_action( spell_name.c_str(), spell_options.c_str() );
    free_action -> base_cost = 0;
    free_action -> background = true;
    free_action -> base_crit += 0.25;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> aura_gain( "Inner Focus" );
    p -> last_foreground_action = free_action;
    free_action -> execute();
    p -> aura_loss( "Inner Focus" );
    update_ready();
  }

  virtual bool ready()
  {
    return( priest_spell_t::ready() && free_action -> ready() );
  }
};

// Divine Spirit Spell =====================================================

struct divine_spirit_t : public priest_spell_t
{
  double bonus;

  divine_spirit_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "divine_spirit", player, SCHOOL_HOLY, TREE_DISCIPLINE ), bonus( 0 )
  {
    trigger_gcd = 0;
    bonus = util_t::ability_rank( player -> level,  80.0,80,  50.0,70,  40.0,0 );
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      p -> buffs.divine_spirit = bonus;
      p -> init_resources( true );
    }
  }

  virtual bool ready()
  {
    return player -> buffs.divine_spirit < bonus;
  }
};

// Fortitude Spell ========================================================

struct fortitude_t : public priest_spell_t
{
  double bonus;

  fortitude_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "fortitude", player, SCHOOL_HOLY, TREE_DISCIPLINE ), bonus( 0 )
  {
    priest_t* p = player -> cast_priest();

    trigger_gcd = 0;

    bonus = util_t::ability_rank( player -> level,  165.0,80,  79.0,70,  54.0,0 );

    bonus *= 1.0 + p -> talents.improved_power_word_fortitude * 0.15;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      p -> buffs.fortitude = bonus;
      p -> init_resources( true );
    }
  }

  virtual bool ready()
  {
    return player -> buffs.fortitude < bonus;
  }
};

// Inner Fire Spell ======================================================

struct inner_fire_t : public priest_spell_t
{
  inner_fire_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "inner_fire", player, SCHOOL_HOLY, TREE_DISCIPLINE )
  {
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    double bonus_power = util_t::ability_rank( p -> level,  120.0,77,  95.0,71,  0.0,0  );

    bonus_power *= 1.0 + p -> talents.improved_inner_fire * 0.15;

    p -> spell_power[ SCHOOL_MAX ] += bonus_power;
    p -> _buffs.inner_fire = 1;
  }

  virtual bool ready()
  {
    return ! player -> cast_priest() -> _buffs.inner_fire;
  }
};

// Shadow Form Spell =======================================================

struct shadow_form_t : public priest_spell_t
{
  shadow_form_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "shadow_form", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();
    check_talent( p -> talents.shadow_form );
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    player -> buffs.shadow_form = 1;
  }

  virtual bool ready()
  {
    return( player -> buffs.shadow_form == 0 );
  }
};

// Shadow Fiend Spell ========================================================

struct shadow_fiend_spell_t : public priest_spell_t
{
  int trigger;

  shadow_fiend_spell_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "shadow_fiend", player, SCHOOL_SHADOW, TREE_SHADOW ), trigger( 0 )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "trigger", OPT_INT, &trigger },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 1, 1, 0, 0, 0, 0.00 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    harmful    = false;
    cooldown   = 300.0;
    cooldown  -= 60.0 * p -> talents.veiled_shadows;
    base_cost *= 1.0 - ( util_t::talent_rank( p -> talents.mental_agility, 3, 0.04, 0.07, 0.10 ) +
                         p -> talents.shadow_focus * 0.02 );
    base_cost  = floor( base_cost );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    consume_resource();
    update_ready();
    double duration = 15.1;
    if ( p -> set_bonus.tier4_2pc() ) duration += 3.0;
    p -> summon_pet( "shadow_fiend", duration );
  }

  virtual bool ready()
  {
    if ( ! priest_spell_t::ready() )
      return false;

    if ( sim -> infinite_resource [ RESOURCE_MANA ] )
      return true;

    return( player -> resource_max    [ RESOURCE_MANA ] -
            player -> resource_current[ RESOURCE_MANA ] ) >= trigger;
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Priest Character Definition
// ==========================================================================

// priest_t::create_action ===================================================

action_t* priest_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "devouring_plague" ) return new devouring_plague_t  ( this, options_str );
  if ( name == "dispersion"       ) return new dispersion_t        ( this, options_str );
  if ( name == "divine_spirit"    ) return new divine_spirit_t     ( this, options_str );
  if ( name == "fortitude"        ) return new fortitude_t         ( this, options_str );
  if ( name == "holy_fire"        ) return new holy_fire_t         ( this, options_str );
  if ( name == "inner_fire"       ) return new inner_fire_t        ( this, options_str );
  if ( name == "inner_focus"      ) return new inner_focus_t       ( this, options_str );
  if ( name == "mind_blast"       ) return new mind_blast_t        ( this, options_str );
  if ( name == "mind_flay"        ) return new mind_flay_t         ( this, options_str );
  if ( name == "penance"          ) return new penance_t           ( this, options_str );
  if ( name == "power_infusion"   ) return new power_infusion_t    ( this, options_str );
  if ( name == "shadow_word_death" ) return new shadow_word_death_t ( this, options_str );
  if ( name == "shadow_word_pain" ) return new shadow_word_pain_t  ( this, options_str );
  if ( name == "shadow_form"      ) return new shadow_form_t       ( this, options_str );
  if ( name == "smite"            ) return new smite_t             ( this, options_str );
  if ( name == "shadow_fiend"     ) return new shadow_fiend_spell_t( this, options_str );
  if ( name == "vampiric_embrace" ) return new vampiric_embrace_t  ( this, options_str );
  if ( name == "vampiric_touch"   ) return new vampiric_touch_t    ( this, options_str );

  return player_t::create_action( name, options_str );
}

// priest_t::create_pet ======================================================

pet_t* priest_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "shadow_fiend" ) return new shadow_fiend_pet_t( sim, this );

  return 0;
}

// priest_t::init_glyphs =====================================================

void priest_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if     ( n == "dispersion" )        glyphs.dispersion = 1;
    else if ( n == "penance"           ) glyphs.penance = 1;
    else if ( n == "shadow_word_death" ) glyphs.shadow_word_death = 1;
    else if ( n == "shadow_word_pain"  ) glyphs.shadow_word_pain = 1;
    else if ( n == "shadow"            ) glyphs.shadow = 1;
    else if ( n == "smite"             ) glyphs.smite = 1;
    // Just to prevent warnings....
    else if ( n == "circle_of_healing" ) ;
    else if ( n == "dispel_magic"      ) ;
    else if ( n == "fading"            ) ;
    else if ( n == "flash_heal"        ) ;
    else if ( n == "fortitude"         ) ;
    else if ( n == "guardian_spirit"   ) ;
    else if ( n == "hymn_of_hope"      ) ;
    else if ( n == "inner_fire"        ) ;
    else if ( n == "levitate"          ) ;
    else if ( n == "mind_flay"         ) ;
    else if ( n == "pain_suppression"  ) ;
    else if ( n == "power_word_shield" ) ;
    else if ( n == "prayer_of_healing" ) ;
    else if ( n == "psychic_scream"    ) ;
    else if ( n == "renew"             ) ;
    else if ( n == "shackle_undead"    ) ;
    else if ( n == "shadow_protection" ) ;
    else if ( n == "shadowfiend"       ) ;
    else if ( ! sim -> parent ) util_t::printf( "simcraft: Player %s has unrecognized glyph %s\n", name(), n.c_str() );
  }
}

// priest_t::init_base =======================================================

void priest_t::init_base()
{
  // Dwarf Priest base stats
  static base_stats_t base_stats_60 = { 60, 1397, 1376, 37, 36, 53, 119, 124, 0.0123765, 0.0317927 };
  static base_stats_t base_stats_70 = { 70, 3391, 2620, 41, 41, 61, 144, 150, 0.0123765, 0.0317927 };
  static base_stats_t base_stats_80 = { 80, 6960, 3863, 45, 47, 70, 173, 180, 0.0123765, 0.0317927 };

  attribute_base[ ATTR_STRENGTH  ] = rating_t::interpolate( level, base_stats_60.strength,  base_stats_70.strength,  base_stats_80.strength  );
  attribute_base[ ATTR_AGILITY   ] = rating_t::interpolate( level, base_stats_60.agility,   base_stats_70.agility,   base_stats_80.agility   );
  attribute_base[ ATTR_STAMINA   ] = rating_t::interpolate( level, base_stats_60.stamina,   base_stats_70.stamina,   base_stats_80.stamina   );
  attribute_base[ ATTR_INTELLECT ] = rating_t::interpolate( level, base_stats_60.intellect, base_stats_70.intellect, base_stats_80.intellect );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::interpolate( level, base_stats_60.spirit,    base_stats_70.spirit,    base_stats_80.spirit    );

  attribute_multiplier_initial[ ATTR_STAMINA   ] *= 1.0 + talents.improved_power_word_fortitude * 0.02;
  attribute_multiplier_initial[ ATTR_SPIRIT    ] *= 1.0 + talents.enlightenment * 0.02;
  attribute_multiplier_initial[ ATTR_SPIRIT    ] *= 1.0 + talents.spirit_of_redemption * 0.05;
  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.mental_strength * 0.03;

  base_spell_crit = rating_t::interpolate( level, base_stats_60.spell_crit, base_stats_70.spell_crit, base_stats_80.spell_crit );

  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.79732 );
  initial_spell_power_per_spirit = ( talents.spiritual_guidance * 0.05 +
                                     talents.twisted_faith      * 0.02 );

  base_attack_power = -10;
  base_attack_crit  = rating_t::interpolate( level, base_stats_60.melee_crit, base_stats_70.melee_crit, base_stats_80.melee_crit );
  initial_attack_power_per_strength = 1.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/21.92982456, 0.01/24.93765586, 0.01/52.19533582 );

  resource_base[ RESOURCE_HEALTH ] = rating_t::interpolate( level, base_stats_60.health, base_stats_70.health, base_stats_80.health );
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, base_stats_60.mana,   base_stats_70.mana,   base_stats_80.mana   );

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

// priest_t::init_gains ======================================================

void priest_t::init_gains()
{
  player_t::init_gains();

  gains_dispersion   = get_gain( "dispersion" );
  gains_shadow_fiend = get_gain( "shadow_fiend" );
}

// priest_t::init_uptimes ====================================================

void priest_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_improved_spirit_tap = get_uptime( "improved_spirit_tap" );
  uptimes_glyph_of_shadow     = get_uptime( "glyph_of_shadow" );
  uptimes_devious_mind        = get_uptime( "devious_mind" );
  uptimes_devious_mind_mf     = get_uptime( "devious_mind_mf" );
}

// priest_t::init_rng ========================================================

void priest_t::init_rng()
{
  player_t::init_rng();

  rng_pain_and_suffering = get_rng( "pain_and_suffering" );
  rng_shadow_weaving     = get_rng( "shadow_weaving" );
  rng_surge_of_light     = get_rng( "surge_of_light" );
}

// priest_t::init_actions =====================================================

void priest_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    action_list_str = "flask,type=frost_wyrm/food,type=tender_shoveltusk_steak/fortitude/divine_spirit/inner_fire";

    int num_items = items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }

    switch ( primary_tree() )
    {
    case TREE_SHADOW:
      if ( talents.shadow_form ) action_list_str += "/shadow_form";
      action_list_str += "/wild_magic_potion";
      action_list_str += "/shadow_fiend";
      if ( talents.dispersion ) action_list_str += "/dispersion";
      action_list_str += "/shadow_word_pain,shadow_weaving_wait=1";
      if ( talents.vampiric_touch ) action_list_str += "/vampiric_touch";
      action_list_str += "/devouring_plague/mind_blast";
      if ( talents.vampiric_embrace ) action_list_str += "/vampiric_embrace";
      if ( use_shadow_word_death ) action_list_str += "/shadow_word_death,mb_wait=0,mb_priority=0";
      action_list_str += talents.mind_flay ? "/mind_flay" : "/smite";
      action_list_str += "/shadow_word_death"; // when moving
      break;
    case TREE_DISCIPLINE:
      action_list_str += "/mana_potion/shadow_fiend,trigger=10000";
      if ( talents.inner_focus ) action_list_str += "/inner_focus,shadow_word_pain";
      action_list_str += "/shadow_word_pain";
      if ( talents.power_infusion ) action_list_str += "/power_infusion";
      action_list_str += "/holy_fire/mind_blast/";
      if ( talents.penance ) action_list_str += "/penance";
      action_list_str += "/smite";
      action_list_str += "/shadow_word_death"; // when moving
      break;
    case TREE_HOLY:
    default:
      action_list_str += "/mana_potion/shadow_fiend,trigger=10000";
      if ( talents.inner_focus ) action_list_str += "/inner_focus,shadow_word_pain";
      action_list_str += "/shadow_word_pain";
      if ( talents.power_infusion ) action_list_str += "/power_infusion";
      action_list_str += "/holy_fire/mind_blast/smite";
      action_list_str += "/shadow_word_death"; // when moving
      break;
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// priest_t::reset ===========================================================

void priest_t::reset()
{
  player_t::reset();

  // Active
  active_devouring_plague = 0;
  active_holy_fire        = 0;
  active_shadow_word_pain = 0;
  active_vampiric_touch   = 0;
  active_vampiric_embrace = 0;

  _buffs.reset();
  _expirations.reset();
  _cooldowns.reset();
}

// priest_t::regen  ==========================================================

void priest_t::regen( double periodicity )
{
  mana_regen_while_casting = util_t::talent_rank( talents.meditation, 3, 0.17, 0.33, 0.50 );

  if ( _buffs.improved_spirit_tap )
  {
    mana_regen_while_casting += util_t::talent_rank( talents.improved_spirit_tap, 2, 0.17, 0.33 );
  }

  player_t::regen( periodicity );
}

// priest_t::get_talent_trees ===============================================

bool priest_t::get_talent_trees( std::vector<int*>& discipline,
                                 std::vector<int*>& holy,
                                 std::vector<int*>& shadow )
{
  talent_translation_t translation[][3] =
  {
    { {  1, NULL                                       }, {  1, NULL                              }, {  1, NULL                                   } },
    { {  2, &( talents.twin_disciplines )              }, {  2, NULL                              }, {  2, &( talents.improved_spirit_tap )       } },
    { {  3, NULL                                       }, {  3, &( talents.holy_specialization )  }, {  3, &( talents.darkness )                  } },
    { {  4, &( talents.improved_inner_fire )           }, {  4, NULL                              }, {  4, &( talents.shadow_affinity )           } },
    { {  5, &( talents.improved_power_word_fortitude ) }, {  5, &( talents.divine_fury )          }, {  5, &( talents.improved_shadow_word_pain ) } },
    { {  6, NULL                                       }, {  6, NULL                              }, {  6, &( talents.shadow_focus )              } },
    { {  7, &( talents.meditation )                    }, {  7, NULL                              }, {  7, NULL                                   } },
    { {  8, &( talents.inner_focus )                   }, {  8, NULL                              }, {  8, &( talents.improved_mind_blast )       } },
    { {  9, NULL                                       }, {  9, NULL                              }, {  9, &( talents.mind_flay )                 } },
    { { 10, NULL                                       }, { 10, NULL                              }, { 10, &( talents.veiled_shadows )            } },
    { { 11, &( talents.mental_agility )                }, { 11, &( talents.searing_light )        }, { 11, NULL                                   } },
    { { 12, NULL                                       }, { 12, NULL                              }, { 12, &( talents.shadow_weaving )            } },
    { { 13, NULL                                       }, { 13, &( talents.spirit_of_redemption ) }, { 13, NULL                                   } },
    { { 14, &( talents.mental_strength )               }, { 14, &( talents.spiritual_guidance )   }, { 14, &( talents.vampiric_embrace )          } },
    { { 15, NULL                                       }, { 15, &( talents.surge_of_light )       }, { 15, &( talents.improved_vampiric_embrace ) } },
    { { 16, &( talents.focused_power )                 }, { 16, NULL                              }, { 16, &( talents.focused_mind )              } },
    { { 17, &( talents.enlightenment )                 }, { 17, NULL                              }, { 17, &( talents.mind_melt )                 } },
    { { 18, NULL                                       }, { 18, NULL                              }, { 18, &( talents.improved_devouring_plague ) } },
    { { 19, &( talents.power_infusion )                }, { 19, NULL                              }, { 19, &( talents.shadow_form )               } },
    { { 20, NULL                                       }, { 20, NULL                              }, { 20, &( talents.shadow_power )              } },
    { { 21, NULL                                       }, { 21, NULL                              }, { 21, NULL                                   } },
    { { 22, NULL                                       }, { 22, NULL                              }, { 22, &( talents.misery )                    } },
    { { 23, &( talents.aspiration )                    }, { 23, NULL                              }, { 23, NULL                                   } },
    { { 24, NULL                                       }, { 24, NULL                              }, { 24, &( talents.vampiric_touch )            } },
    { { 25, NULL                                       }, { 25, NULL                              }, { 25, &( talents.pain_and_suffering )        } },
    { { 26, NULL                                       }, { 26, NULL                              }, { 26, &( talents.twisted_faith )             } },
    { { 27, NULL                                       }, { 27, NULL                              }, { 27, &( talents.dispersion )                } },
    { { 28, &( talents.penance )                       }, {  0, NULL                              }, {  0, NULL                                   } },
    { {  0, NULL                                       }, {  0, NULL                              }, {  0, NULL                                   } }
  };

  return get_talent_translation( discipline, holy, shadow, translation );
}

// priest_t::get_options ===================================================

std::vector<option_t>& priest_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t priest_options[] =
    {
      // @option_doc loc=player/priest/talents title="Talents"
      { "aspiration",                    OPT_INT,  &( talents.aspiration                    ) },
      { "darkness",                      OPT_INT,  &( talents.darkness                      ) },
      { "dispersion",                    OPT_INT,  &( talents.dispersion                    ) },
      { "divine_fury",                   OPT_INT,  &( talents.divine_fury                   ) },
      { "enlightenment",                 OPT_INT,  &( talents.enlightenment                 ) },
      { "focused_mind",                  OPT_INT,  &( talents.focused_mind                  ) },
      { "focused_power",                 OPT_INT,  &( talents.focused_power                 ) },
      { "force_of_will",                 OPT_INT,  &( talents.force_of_will                 ) },
      { "holy_specialization",           OPT_INT,  &( talents.holy_specialization           ) },
      { "improved_devouring_plague",     OPT_INT,  &( talents.improved_devouring_plague     ) },
      { "improved_inner_fire",           OPT_INT,  &( talents.improved_inner_fire           ) },
      { "improved_mind_blast",           OPT_INT,  &( talents.improved_mind_blast           ) },
      { "improved_power_word_fortitude", OPT_INT,  &( talents.improved_power_word_fortitude ) },
      { "improved_shadow_word_pain",     OPT_INT,  &( talents.improved_shadow_word_pain     ) },
      { "improved_spirit_tap",           OPT_INT,  &( talents.improved_spirit_tap           ) },
      { "improved_vampiric_embrace",     OPT_INT,  &( talents.improved_vampiric_embrace     ) },
      { "inner_focus",                   OPT_INT,  &( talents.inner_focus                   ) },
      { "meditation",                    OPT_INT,  &( talents.meditation                    ) },
      { "mental_agility",                OPT_INT,  &( talents.mental_agility                ) },
      { "mental_strength",               OPT_INT,  &( talents.mental_strength               ) },
      { "mind_flay",                     OPT_INT,  &( talents.mind_flay                     ) },
      { "mind_melt",                     OPT_INT,  &( talents.mind_melt                     ) },
      { "misery",                        OPT_INT,  &( talents.misery                        ) },
      { "pain_and_suffering",            OPT_INT,  &( talents.pain_and_suffering            ) },
      { "penance",                       OPT_INT,  &( talents.penance                       ) },
      { "power_infusion",                OPT_INT,  &( talents.power_infusion                ) },
      { "searing_light",                 OPT_INT,  &( talents.searing_light                 ) },
      { "shadow_affinity",               OPT_INT,  &( talents.shadow_affinity               ) },
      { "shadow_focus",                  OPT_INT,  &( talents.shadow_focus                  ) },
      { "shadow_form",                   OPT_INT,  &( talents.shadow_form                   ) },
      { "shadow_power",                  OPT_INT,  &( talents.shadow_power                  ) },
      { "shadow_weaving",                OPT_INT,  &( talents.shadow_weaving                ) },
      { "spirit_of_redemption",          OPT_INT,  &( talents.spirit_of_redemption          ) },
      { "spiritual_guidance",            OPT_INT,  &( talents.spiritual_guidance            ) },
      { "surge_of_light",                OPT_INT,  &( talents.surge_of_light                ) },
      { "twin_disciplines",              OPT_INT,  &( talents.twin_disciplines              ) },
      { "twisted_faith",                 OPT_INT,  &( talents.twisted_faith                 ) },
      { "vampiric_embrace",              OPT_INT,  &( talents.vampiric_embrace              ) },
      { "vampiric_touch",                OPT_INT,  &( talents.vampiric_touch                ) },
      { "veiled_shadows",                OPT_INT,  &( talents.veiled_shadows                ) },
      // @option_doc loc=player/priest/glyphs title="Glyphs"
      { "glyph_hymn_of_hope",            OPT_BOOL, &( glyphs.hymn_of_hope                   ) },
      { "glyph_penance",                 OPT_BOOL, &( glyphs.penance                        ) },
      { "glyph_shadow_word_death",       OPT_BOOL, &( glyphs.shadow_word_death              ) },
      { "glyph_shadow_word_pain",        OPT_BOOL, &( glyphs.shadow_word_pain               ) },
      { "glyph_shadow",                  OPT_BOOL, &( glyphs.shadow                         ) },
      { "glyph_smite",                   OPT_BOOL, &( glyphs.smite                          ) },
      // @option_doc loc=player/priest/misc title="Misc"
      { "devious_mind_delay",            OPT_FLT,  &( devious_mind_delay                    ) },
      { "use_shadow_word_death",         OPT_BOOL, &( use_shadow_word_death                 ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, priest_options );
  }

  return options;
}

// priest_t::decode_set =====================================================

int priest_t::decode_set( item_t& item )
{
  if ( strstr( item.name(), "circlet_of_faith"            ) ) return SET_T7;
  if ( strstr( item.name(), "mantle_of_faith"             ) ) return SET_T7;
  if ( strstr( item.name(), "raiments_of_faith"           ) ) return SET_T7;
  if ( strstr( item.name(), "handwraps_of_faith"          ) ) return SET_T7;
  if ( strstr( item.name(), "pants_of_faith"              ) ) return SET_T7;
  if ( strstr( item.name(), "circlet_of_sanctification"   ) ) return SET_T8;
  if ( strstr( item.name(), "mantle_of_sanctification"    ) ) return SET_T8;
  if ( strstr( item.name(), "raiments_of_sanctification"  ) ) return SET_T8;
  if ( strstr( item.name(), "handwraps_of_sanctification" ) ) return SET_T8;
  if ( strstr( item.name(), "pants_of_sanctification"     ) ) return SET_T8;
  if ( strstr( item.name(), "zabras_circlet_"             ) ) return SET_T9;
  if ( strstr( item.name(), "zabras_mantle_"              ) ) return SET_T9;
  if ( strstr( item.name(), "zabras_raiments_"            ) ) return SET_T9;
  if ( strstr( item.name(), "zabras_handwraps_"           ) ) return SET_T9;
  if ( strstr( item.name(), "zabras_pants_"               ) ) return SET_T9;
  if ( strstr( item.name(), "velens_circlet_"             ) ) return SET_T9;
  if ( strstr( item.name(), "velens_mantle_"              ) ) return SET_T9;
  if ( strstr( item.name(), "velens_raiments_"            ) ) return SET_T9;
  if ( strstr( item.name(), "velens_handwraps_"           ) ) return SET_T9;
  if ( strstr( item.name(), "velens_pants_"               ) ) return SET_T9;
  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_priest  =================================================

player_t* player_t::create_priest( sim_t* sim, const std::string& name )
{
  priest_t* p =  new priest_t( sim, name );

  new shadow_fiend_pet_t( sim, p );

  return p;
}

