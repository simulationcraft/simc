// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

struct dancing_rune_weapon_pet_t;

// ==========================================================================
// Death Knight Runes
// ==========================================================================

enum rune_type
{
  RUNE_TYPE_NONE=0, RUNE_TYPE_BLOOD, RUNE_TYPE_FROST, RUNE_TYPE_UNHOLY, RUNE_TYPE_DEATH, RUNE_TYPE_WASDEATH=8
};

const char *rune_symbols = "!bfu!!";

#define RUNE_TYPE_MASK     3
#define RUNE_SLOT_MAX      6

#define RUNIC_POWER_REFUND  0.9

// These macros simplify using the result of count_runes(), which
// returns a number of the form 0x000AABBCC where AA is the number of
// Unholy runes, BB is the number of Frost runes, and CC is the number
// of Blood runes.
#define GET_BLOOD_RUNE_COUNT(x)  ((x >>  0) & 0xff)
#define GET_FROST_RUNE_COUNT(x)  ((x >>  8) & 0xff)
#define GET_UNHOLY_RUNE_COUNT(x) ((x >> 16) & 0xff)

enum rune_state { STATE_DEPLETED, STATE_REGENERATING, STATE_FULL };

struct dk_rune_t
{
  int        type;
  rune_state state;
  double     value;   // 0.0 to 1.0, with 1.0 being full
  int        slot_number;
  bool       permanent_death_rune;
  dk_rune_t* paired_rune;

  dk_rune_t() : type( RUNE_TYPE_NONE ), state( STATE_FULL ), value( 0.0 ), permanent_death_rune( false ), paired_rune( NULL ) {}

  bool is_death()        const { return ( type & RUNE_TYPE_DEATH ) != 0                ; }
  bool is_blood()        const { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_BLOOD  ; }
  bool is_unholy()       const { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_UNHOLY ; }
  bool is_frost()        const { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_FROST  ; }
  bool is_ready()        const { return state == STATE_FULL                            ; }
  bool is_depleted()     const { return state == STATE_DEPLETED                        ; }
  bool is_regenerating() const { return state == STATE_REGENERATING                    ; }
  int  get_type()        const { return type & RUNE_TYPE_MASK                          ; }

  void regen_rune( player_t* p, double periodicity );

  void make_permanent_death_rune()
  {
    permanent_death_rune = true;
    type |= RUNE_TYPE_DEATH;
  }

  void consume( bool convert )
  {
    assert ( value >= 1.0 );
    if ( permanent_death_rune )
    {
      type |= RUNE_TYPE_DEATH;
    }
    else
    {
      type = ( type & RUNE_TYPE_MASK ) | ( ( type << 1 ) & RUNE_TYPE_WASDEATH ) | ( convert ? RUNE_TYPE_DEATH : 0 );
    }
    value = 0.0;
    state = STATE_DEPLETED;
  }

  void fill_rune()
  {
    value = 1.0;
    state = STATE_FULL;
  }

  void reset()
  {
    value = 1.0;
    state = STATE_FULL;
    type = type & RUNE_TYPE_MASK;
    if ( permanent_death_rune )
    {
      type |= RUNE_TYPE_DEATH;
    }
  }
};

// ==========================================================================
// Death Knight
// ==========================================================================

enum death_knight_presence { PRESENCE_BLOOD=1, PRESENCE_FROST, PRESENCE_UNHOLY=4 };

struct death_knight_targetdata_t : public targetdata_t
{
  dot_t* dots_blood_plague;
  dot_t* dots_death_and_decay;
  dot_t* dots_frost_fever;

  buff_t* debuffs_ebon_plaguebringer;

  int diseases()
  {
    int disease_count = 0;
    if ( debuffs_ebon_plaguebringer -> check() ) disease_count++;
    if ( dots_blood_plague -> ticking ) disease_count++;
    if ( dots_frost_fever  -> ticking ) disease_count++;
    return disease_count;
  }

  death_knight_targetdata_t( player_t* source, player_t* target );
};

void register_death_knight_targetdata( sim_t* sim )
{
  player_type t = DEATH_KNIGHT;
  typedef death_knight_targetdata_t type;

  REGISTER_DOT( blood_plague );
  REGISTER_DOT( death_and_decay );
  REGISTER_DOT( frost_fever );

  REGISTER_DEBUFF( ebon_plaguebringer );
}

struct death_knight_t : public player_t
{
  // Active
  int       active_presence;
  action_t* active_blood_caked_blade;
  action_t* active_unholy_blight;

  // Buffs
  buff_t* buffs_blood_presence;
  buff_t* buffs_bloodworms;
  buff_t* buffs_bone_shield;
  buff_t* buffs_crimson_scourge;
  buff_t* buffs_dancing_rune_weapon;
  buff_t* buffs_dark_transformation;
  buff_t* buffs_frost_presence;
  buff_t* buffs_killing_machine;
  buff_t* buffs_pillar_of_frost;
  buff_t* buffs_rime;
  buff_t* buffs_rune_of_cinderglacier;
  buff_t* buffs_rune_of_razorice;
  buff_t* buffs_rune_of_the_fallen_crusader;
  buff_t* buffs_runic_corruption;
  buff_t* buffs_scent_of_blood;
  buff_t* buffs_shadow_infusion;
  buff_t* buffs_sudden_doom;
  buff_t* buffs_tier11_4pc_melee;
  buff_t* buffs_tier13_4pc_melee;
  buff_t* buffs_unholy_presence;

  // Cooldowns
  cooldown_t* cooldowns_howling_blast;

  // Diseases
  spell_t* blood_plague;
  spell_t* frost_fever;

  // Gains
  gain_t* gains_butchery;
  gain_t* gains_chill_of_the_grave;
  gain_t* gains_frost_presence;
  gain_t* gains_horn_of_winter;
  gain_t* gains_improved_frost_presence;
  gain_t* gains_might_of_the_frozen_wastes;
  gain_t* gains_power_refund;
  gain_t* gains_scent_of_blood;
  gain_t* gains_tier12_2pc_melee;
  gain_t* gains_rune;
  gain_t* gains_rune_unholy;
  gain_t* gains_rune_blood;
  gain_t* gains_rune_frost;
  gain_t* gains_rune_unknown;
  gain_t* gains_runic_empowerment;
  gain_t* gains_runic_empowerment_blood;
  gain_t* gains_runic_empowerment_unholy;
  gain_t* gains_runic_empowerment_frost;
  gain_t* gains_empower_rune_weapon;
  gain_t* gains_blood_tap;
  // only useful if the blood rune charts are enabled
  // charts are currently disabled so commenting out
  // gain_t* gains_blood_tap_blood;

  // Glyphs
  struct glyphs_t
  {
    // Prime
    glyph_t* death_and_decay;
    glyph_t* death_coil;
    glyph_t* death_strike;
    glyph_t* frost_strike;
    glyph_t* heart_strike;
    glyph_t* howling_blast;
    glyph_t* icy_touch;
    glyph_t* obliterate;
    glyph_t* raise_dead;
    glyph_t* rune_strike;
    glyph_t* scourge_strike;

    // Minor
    glyph_t* horn_of_winter;

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  // Options
  std::string unholy_frenzy_target_str;

  // Spells
  struct spells_t
  {
    spell_data_t* blood_of_the_north;
    spell_data_t* blood_rites;
    spell_data_t* dreadblade;
    spell_data_t* frozen_heart;
    spell_data_t* icy_talons;
    spell_data_t* master_of_ghouls;
    spell_data_t* plate_specialization;
    spell_data_t* reaping;
    spell_data_t* runic_empowerment;
    spell_data_t* unholy_might;
    spell_data_t* veteran_of_the_third_war;

    spells_t() { memset( ( void* ) this, 0x0, sizeof( spells_t ) ); }
  };
  spells_t spells;

  // Pets and Guardians
  pet_t* active_army_ghoul;
  pet_t* active_bloodworms;
  dancing_rune_weapon_pet_t* active_dancing_rune_weapon;
  pet_t* active_ghoul;
  pet_t* active_gargoyle;

  // Procs
  proc_t* proc_runic_empowerment;
  proc_t* proc_runic_empowerment_wasted;
  proc_t* proc_oblit_killing_machine;
  proc_t* proc_fs_killing_machine;

  // RNGs
  rng_t* rng_blood_caked_blade;
  rng_t* rng_might_of_the_frozen_wastes;
  rng_t* rng_threat_of_thassarian;
  rng_t* rng_butchery_start;

  // Runes
  struct runes_t
  {
    dk_rune_t slot[RUNE_SLOT_MAX];

    runes_t()
    {
      // 6 runes, paired blood, frost and unholy
      slot[0].type = slot[1].type = RUNE_TYPE_BLOOD;
      slot[2].type = slot[3].type = RUNE_TYPE_FROST;
      slot[4].type = slot[5].type = RUNE_TYPE_UNHOLY;
      // each rune pair is paired with each other
      slot[0].paired_rune = &slot[1]; slot[1].paired_rune = &slot[0];
      slot[2].paired_rune = &slot[3]; slot[3].paired_rune = &slot[2];
      slot[4].paired_rune = &slot[5]; slot[5].paired_rune = &slot[4];
      // give each rune a slot number
      for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) { slot[i].slot_number = i; }
    }
    void reset() { for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) slot[i].reset(); }
  };
  runes_t _runes;

  // Talents
  struct talents_t
  {
    // Blood
    talent_t* abominations_might;
    talent_t* bladed_armor;
    talent_t* blood_caked_blade;
    talent_t* blood_parasite;
    talent_t* bone_shield;
    talent_t* butchery;
    talent_t* crimson_scourge;
    talent_t* dancing_rune_weapon;
    talent_t* improved_blood_presence;
    talent_t* improved_blood_tap;
    talent_t* improved_death_strike;
    talent_t* scent_of_blood;

    // Frost
    talent_t* annihilation;
    talent_t* brittle_bones;
    talent_t* chill_of_the_grave;
    talent_t* endless_winter;
    talent_t* howling_blast;
    talent_t* improved_frost_presence;
    talent_t* improved_icy_talons;
    talent_t* killing_machine;
    talent_t* merciless_combat;
    talent_t* might_of_the_frozen_wastes;
    talent_t* nerves_of_cold_steel;
    talent_t* pillar_of_frost;
    talent_t* rime;
    talent_t* runic_power_mastery;
    talent_t* threat_of_thassarian;
    talent_t* toughness;

    // Unholy
    talent_t* dark_transformation;
    talent_t* ebon_plaguebringer;
    talent_t* epidemic;
    talent_t* improved_unholy_presence;
    talent_t* morbidity;
    talent_t* rage_of_rivendare;
    talent_t* runic_corruption;
    talent_t* shadow_infusion;
    talent_t* sudden_doom;
    talent_t* summon_gargoyle;
    talent_t* unholy_blight;
    talent_t* unholy_frenzy;
    talent_t* virulence;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  // Uptimes
  benefit_t* uptimes_rp_cap;

  double tier12_4pc_melee_value;

  death_knight_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) :
    player_t( sim, DEATH_KNIGHT, name, r )
  {
    if ( race == RACE_NONE ) race = RACE_NIGHT_ELF;

    tree_type[ DEATH_KNIGHT_BLOOD  ] = TREE_BLOOD;
    tree_type[ DEATH_KNIGHT_FROST  ] = TREE_FROST;
    tree_type[ DEATH_KNIGHT_UNHOLY ] = TREE_UNHOLY;

    // Active
    active_presence            = 0;
    active_blood_caked_blade   = NULL;
    active_unholy_blight       = NULL;
    blood_plague = NULL;
    frost_fever  = NULL;

    // Pets and Guardians
    active_army_ghoul          = NULL;
    active_bloodworms          = NULL;
    active_dancing_rune_weapon = NULL;
    active_ghoul               = NULL;
    active_gargoyle            = NULL;

    cooldowns_howling_blast = get_cooldown( "howling_blast" );

    create_talents();
    create_glyphs();
    create_options();

    default_distance = 0;
  }

  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target ) {return new death_knight_targetdata_t( source, target );}
  virtual void      init();
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_actions();
  virtual void      init_enchant();
  virtual void      init_rng();
  virtual void      init_defense();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_resources( bool force );
  virtual void      init_benefits();
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_attack_haste() const;
  virtual double    composite_attack_hit() const;
  virtual double    composite_attack_power() const;
  virtual double    composite_attribute_multiplier( int attr ) const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;
  virtual double    composite_spell_hit() const;
  virtual double    composite_tank_parry() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    composite_tank_crit( const school_type school ) const;
  virtual void      regen( double periodicity );
  virtual void      reset();
  virtual double    assess_damage( double amount, const school_type school, int    dmg_type, int result, action_t* a );
  virtual void      combat_begin();
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual action_expr_t* create_expression( action_t*, const std::string& name );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const { return RESOURCE_RUNIC; }
  virtual int       primary_role() const;
  virtual void      trigger_runic_empowerment();
  virtual int       runes_count( rune_type rt, bool include_death, int position );
  virtual double    runes_cooldown_any( rune_type rt, bool include_death, int position );
  virtual double    runes_cooldown_all( rune_type rt, bool include_death, int position );
  virtual double    runes_cooldown_time( dk_rune_t* r );
  virtual bool      runes_depleted( rune_type rt, int position );

  void reset_gcd()
  {
    for ( action_t* a=action_list; a; a = a -> next )
    {
      if ( a -> trigger_gcd != 0 ) a -> trigger_gcd = base_gcd;
    }
  }
};

death_knight_targetdata_t::death_knight_targetdata_t( player_t* source, player_t* target )
  : targetdata_t( source, target )
{
  death_knight_t* p = this->source -> cast_death_knight();
  debuffs_ebon_plaguebringer  = add_aura( new buff_t( this, 65142, "ebon_plaguebringer_track", -1, -1, true ) );
  debuffs_ebon_plaguebringer -> buff_duration += p -> talents.epidemic -> effect1().seconds();
}

static void log_rune_status( death_knight_t* p )
{
  std::string rune_str;
  for ( int j = 0; j < RUNE_SLOT_MAX; ++j )
  {
    char rune_letter = rune_symbols[p -> _runes.slot[j].get_type()];
    if ( p -> _runes.slot[j].is_death() )
      rune_letter = 'd';

    if ( p -> _runes.slot[j].is_ready() )
      rune_letter = toupper( rune_letter );
    rune_str += rune_letter;
  }
  log_t::output( p -> sim, "%s runes: %s", p -> name(), rune_str.c_str() );
}

void dk_rune_t::regen_rune( player_t* p, double periodicity )
{
  // If the other rune is already regening, we don't
  // but if both are full we still continue on to record resource gain overflow
  if ( state == STATE_DEPLETED &&   paired_rune -> state == STATE_REGENERATING ) return;
  if ( state == STATE_FULL     && ! ( paired_rune -> state == STATE_FULL )     ) return;

  // Base rune regen rate is 10 seconds; we want the per-second regen
  // rate, so divide by 10.0.  Haste is a multiplier (so 30% haste
  // means composite_attack_haste is 1/1.3), so we invert it.  Haste
  // linearly scales regen rate -- 100% haste means a rune regens in 5
  // seconds, etc.
  double runes_per_second = 1.0 / 10.0 / p -> composite_attack_haste();

  death_knight_t* o = p -> cast_death_knight();

  if ( o -> buffs_blood_presence -> check() && o -> talents.improved_blood_presence -> rank() )
    runes_per_second *= 1.0 + o -> talents.improved_blood_presence -> effect3().percent();

  // Unholy Presence's 10% (or, talented, 15%) increase is factored in elsewhere as melee haste.
  if ( o -> buffs_runic_corruption -> up() )
    runes_per_second *= 1.0 + ( o -> talents.runic_corruption -> effect1().percent() );

  double regen_amount = periodicity * runes_per_second;

  // record rune gains and overflow
  gain_t* gains_rune      = o -> gains_rune         ;
  gain_t* gains_rune_type =
    is_frost()            ? o -> gains_rune_frost   :
    is_blood()            ? o -> gains_rune_blood   :
    is_unholy()           ? o -> gains_rune_unholy  :
                            o -> gains_rune_unknown ; // should never happen, so if you've seen this in a report happy bug hunting

  // full runes don't regen. if both full, record half of overflow, as the other rune will record the other half
  if ( state == STATE_FULL )
  {
    if ( paired_rune -> state == STATE_FULL )
    {
      gains_rune_type -> add( 0, regen_amount * 0.5 );
      gains_rune      -> add( 0, regen_amount * 0.5 );
    }
    return;
  }

  // Chances are, we will overflow by a small amount.  Toss extra
  // overflow into our paired rune if it is regenerating or depleted.
  value += regen_amount;
  double overflow = 0.0;
  if ( value > 1.0 )
  {
    overflow = value - 1.0;
    value = 1.0;
  }

  if ( value >= 1.0 )
    state = STATE_FULL;
  else
    state = STATE_REGENERATING;

  if ( overflow > 0.0 && ( paired_rune -> state == STATE_REGENERATING || paired_rune -> state == STATE_DEPLETED ) )
  {
    // we shouldn't ever overflow the paired rune, but take care just in case
    paired_rune -> value += overflow;
    if ( paired_rune -> value > 1.0 )
    {
      overflow = paired_rune -> value - 1.0;
      paired_rune -> value = 1.0;
    }
    if ( paired_rune -> value >= 1.0 )
      paired_rune -> state = STATE_FULL;
    else
      paired_rune -> state = STATE_REGENERATING;
  }
  gains_rune_type -> add( regen_amount - overflow, overflow );
  gains_rune      -> add( regen_amount - overflow, overflow );

  if ( p -> sim -> debug )
    log_t::output( p -> sim, "rune %d has %.2f regen time (%.3f per second) with %.2f%% haste",
                   slot_number, 1 / runes_per_second, runes_per_second, 100 * ( 1 / p -> composite_attack_haste() - 1 ) );

  if ( state == STATE_FULL )
  {
    if ( p -> sim -> log )
      log_rune_status( o );

    if ( is_death() )
      o -> buffs_tier11_4pc_melee -> trigger();

    if ( p -> sim -> debug )
      log_t::output( p -> sim, "rune %d regens to full", slot_number );
  }
}


// ==========================================================================
// Dancing Rune Weapon
// ==========================================================================

struct dancing_rune_weapon_pet_t : public pet_t
{
  dot_t* dots_drw_blood_plague;
  dot_t* dots_drw_frost_fever;

  int drw_diseases( player_t* /* t */ )
  {
    int drw_disease_count = 0;
    if ( dots_drw_blood_plague -> ticking ) drw_disease_count++;
    if ( dots_drw_frost_fever  -> ticking ) drw_disease_count++;
    return drw_disease_count;
  }

  struct drw_spell_t : public spell_t
  {
    drw_spell_t( const char* n, uint32_t id, dancing_rune_weapon_pet_t* p ) :
      spell_t( n, id, p )
    { }

    virtual bool ready() { return false; }
  };

  struct drw_blood_boil_t : public drw_spell_t
  {
    drw_blood_boil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "blood_boil", 48721, p )
    {
      background       = true;
      trigger_gcd      = 0;
      aoe              = -1;
      may_crit         = true;
      direct_power_mod = 0.08;
    }

    void target_debuff( player_t* t, int dmg_type )
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      drw_spell_t::target_debuff( t, dmg_type );

      base_dd_adder = ( p -> drw_diseases( t ) ? 95 : 0 );
      direct_power_mod  = 0.08 + ( p -> drw_diseases( t ) ? 0.035 : 0 );
    }
  };

  struct drw_blood_plague_t : public drw_spell_t
  {
    drw_blood_plague_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "blood_plague", 59879, p )
    {
      death_knight_t* o = p -> owner -> cast_death_knight();

      background         = true;
      base_tick_time     = 3.0;
      num_ticks          = 7 + util_t::talent_rank( o -> talents.epidemic -> rank(), 3, 1, 3, 4 );
      direct_power_mod   = 0.055 * 1.15;
      may_miss           = false;
      hasted_ticks       = false;
    }
  };

  struct drw_death_coil_t : public drw_spell_t
  {
    drw_death_coil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "death_coil", 47541, p )
    {
      death_knight_t* o = p -> owner -> cast_death_knight();

      background  = true;
      trigger_gcd = 0;
      direct_power_mod = 0.23;
      base_dd_min      = player -> dbc.effect_min( effect_id( 1 ), p -> level ); // Values are saved in a not automatically parsed sub-effect
      base_dd_max      = player -> dbc.effect_max( effect_id( 1 ), p -> level );
      base_multiplier *= 1 + o -> glyphs.death_coil -> effect1().percent();

      base_crit     += o -> sets -> set( SET_T11_2PC_MELEE ) -> effect1().percent();
    }
  };

  struct drw_attack_t : public attack_t
  {
    drw_attack_t( const char* n, uint32_t id, dancing_rune_weapon_pet_t* p, bool special=false ) :
      attack_t( n, id, p, 0, special )
    { }

    drw_attack_t( const char* n, dancing_rune_weapon_pet_t* p, int r=RESOURCE_NONE, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special = false ) :
      attack_t( n, p, r, s, t, special )
    { }

    virtual bool ready() { return false; }
  };

  struct drw_death_strike_t : public drw_attack_t
  {
    drw_death_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( "death_strike", 49998, p, true )
    {
      death_knight_t* o = p -> owner -> cast_death_knight();

      background  = true;
      trigger_gcd = 0;

      base_crit       +=     o -> talents.improved_death_strike -> mod_additive( P_CRIT );
      base_multiplier *= 1 + o -> talents.improved_death_strike -> mod_additive( P_GENERIC );
    }
  };

  struct drw_frost_fever_t : public drw_spell_t
  {
    drw_frost_fever_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "frost_fever", 59921, p )
    {
      death_knight_t* o = p -> owner -> cast_death_knight();

      background        = true;
      trigger_gcd       = 0;
      base_tick_time    = 3.0;
      hasted_ticks      = false;
      may_miss          = false;
      num_ticks         = 7 + util_t::talent_rank( o -> talents.epidemic -> rank(), 3, 1, 3, 4 );
      direct_power_mod *= 0.055 * 1.15;
      base_multiplier  *= 1.0 + o -> glyphs.icy_touch -> effect1().percent();
    }
  };

  struct drw_heart_strike_t : public drw_attack_t
  {
    drw_heart_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( "heart_strike", 55050, p )
    {
      death_knight_t* o = p -> owner -> cast_death_knight();

      background          = true;
      aoe                 = 2;
      base_add_multiplier = 0.75;
      trigger_gcd         = 0;
      base_multiplier    *= 1 + o -> glyphs.heart_strike -> effect1().percent();
    }

    void target_debuff( player_t* t, int dmg_type )
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      drw_attack_t::target_debuff( t, dmg_type );

      target_multiplier *= 1 + p -> drw_diseases( t ) * effect3().percent();
    }
  };

  struct drw_icy_touch_t : public drw_spell_t
  {
    drw_icy_touch_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "icy_touch", 45477, p )
    {
      background       = true;
      trigger_gcd      = 0;
      direct_power_mod = 0.2;
    }

    virtual void execute()
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;

      drw_spell_t::execute();

      if ( result_is_hit() )
      {
        if ( ! p -> drw_frost_fever )
          p -> drw_frost_fever = new drw_frost_fever_t( p );
        p -> drw_frost_fever -> execute();
      }
    }
  };

  struct drw_pestilence_t : public drw_spell_t
  {
    drw_pestilence_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "pestilence", 50842, p )
    {
      trigger_gcd = 0;
      background = true;
    }
  };

  struct drw_plague_strike_t : public drw_attack_t
  {
    drw_plague_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( "plague_strike", 45462, p, true )
    {
      background       = true;
      trigger_gcd      = 0;
      may_crit         = true;
    }

    virtual void execute()
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;

      drw_attack_t::execute();

      if ( result_is_hit() )
      {
        if ( ! p -> drw_blood_plague )
          p -> drw_blood_plague = new drw_blood_plague_t( p );
        p -> drw_blood_plague -> execute();
      }
    }
  };

  struct drw_melee_t : public drw_attack_t
  {
    drw_melee_t( dancing_rune_weapon_pet_t* p ) :
      drw_attack_t( "drw_melee", p, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
    {
      weapon            = &( p -> owner -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min       = 2; // FIXME: Should these be set?
      base_dd_max       = 322;
      may_crit          = true;
      background        = true;
      repeating         = true;
      weapon_power_mod *= 2.0; //Attack power scaling is unaffected by the DRW 50% penalty.
    }

    virtual bool ready()
    {
      return attack_t::ready();
    }
  };

  double snapshot_spell_crit, snapshot_attack_crit, haste_snapshot, speed_snapshot;
  spell_t*  drw_blood_boil;
  spell_t*  drw_blood_plague;
  spell_t*  drw_death_coil;
  attack_t* drw_death_strike;
  spell_t*  drw_frost_fever;
  attack_t* drw_heart_strike;
  spell_t*  drw_icy_touch;
  spell_t*  drw_pestilence;
  attack_t* drw_plague_strike;
  attack_t* drw_melee;

  dancing_rune_weapon_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "dancing_rune_weapon", true ),
    dots_drw_blood_plague( 0 ), dots_drw_frost_fever( 0 ),
    snapshot_spell_crit( 0.0 ), snapshot_attack_crit( 0.0 ),
    haste_snapshot( 1.0 ), speed_snapshot( 1.0 ), drw_blood_boil( 0 ), drw_blood_plague( 0 ),
    drw_death_coil( 0 ), drw_death_strike( 0 ), drw_frost_fever( 0 ),
    drw_heart_strike( 0 ), drw_icy_touch( 0 ), drw_pestilence( 0 ),
    drw_plague_strike( 0 ), drw_melee( 0 )
  {
    dots_drw_blood_plague  = get_dot( "blood_plague" );
    dots_drw_frost_fever   = get_dot( "frost_fever" );

    main_hand_weapon.type       = WEAPON_SWORD_2H;
    main_hand_weapon.min_dmg    = 685; // FIXME: Should these be hardcoded?
    main_hand_weapon.max_dmg    = 975;
    main_hand_weapon.swing_time = 3.3;
  }

  virtual void init_base()
  {
    // Everything stays at zero.
    // DRW uses a snapshot of the DKs stats when summoned.
    drw_blood_boil    = new drw_blood_boil_t   ( this );
    drw_blood_plague  = new drw_blood_plague_t ( this );
    drw_death_coil    = new drw_death_coil_t   ( this );
    drw_death_strike  = new drw_death_strike_t ( this );
    drw_frost_fever   = new drw_frost_fever_t  ( this );
    drw_heart_strike  = new drw_heart_strike_t ( this );
    drw_icy_touch     = new drw_icy_touch_t    ( this );
    drw_pestilence    = new drw_pestilence_t   ( this );
    drw_plague_strike = new drw_plague_strike_t( this );
    drw_melee         = new drw_melee_t        ( this );
  }

  virtual double composite_attack_crit() const        { return snapshot_attack_crit; }
  virtual double composite_attack_haste() const       { return haste_snapshot; }
  virtual double composite_attack_speed() const       { return speed_snapshot; }
  virtual double composite_attack_power() const       { return attack_power; }
  virtual double composite_spell_crit() const         { return snapshot_spell_crit;  }

  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    snapshot_spell_crit  = o -> composite_spell_crit();
    snapshot_attack_crit = o -> composite_attack_crit();
    haste_snapshot       = o -> composite_attack_haste();
    speed_snapshot       = o -> composite_attack_speed();
    attack_power         = o -> composite_attack_power() * o -> composite_attack_power_multiplier();
    drw_melee -> schedule_execute();
  }
};


namespace  // ANONYMOUS NAMESPACE ===========================================
{

// ==========================================================================
// Guardians
// ==========================================================================

// ==========================================================================
// Army of the Dead Ghoul
// ==========================================================================

struct army_ghoul_pet_t : public pet_t
{
  double snapshot_crit, snapshot_haste, snapshot_speed, snapshot_hit, snapshot_strength;

  army_ghoul_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "army_of_the_dead", true ),
    snapshot_crit( 0 ), snapshot_haste( 0 ), snapshot_speed( 0 ), snapshot_hit( 0 ), snapshot_strength( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 228; // FIXME: Needs further testing
    main_hand_weapon.max_dmg    = 323; // FIXME: Needs further testing
    main_hand_weapon.swing_time = 2.0;

    action_list_str = "snapshot_stats/auto_attack/claw";
  }

  struct army_ghoul_pet_attack_t : public attack_t
  {
    void _init_army_ghoul_pet_attack_t()
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
      base_multiplier *= 8.0; // 8 ghouls
    }

    army_ghoul_pet_attack_t( const char* n, army_ghoul_pet_t* p, const resource_type r=RESOURCE_ENERGY, bool special=true ) :
      attack_t( n, p, r, SCHOOL_PHYSICAL, TREE_NONE, special )
    {
      _init_army_ghoul_pet_attack_t();
    }

    army_ghoul_pet_attack_t( const char* n, uint32_t id, army_ghoul_pet_t* p, bool special=true ) :
      attack_t( n, id, p, TREE_NONE, special )
    {
      _init_army_ghoul_pet_attack_t();
    }
  };

  struct army_ghoul_pet_melee_t : public army_ghoul_pet_attack_t
  {
    army_ghoul_pet_melee_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_attack_t( "melee", p, RESOURCE_NONE, false )
    {
      base_execute_time = weapon -> swing_time;
      background        = true;
      repeating         = true;
      weapon_power_mod  = 0.0055 / weapon -> swing_time; // FIXME: Needs further testing
    }
  };

  struct army_ghoul_pet_auto_attack_t : public army_ghoul_pet_attack_t
  {
    army_ghoul_pet_auto_attack_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_attack_t( "auto_attack", p )
    {
      weapon = &( p -> main_hand_weapon );
      p -> main_hand_attack = new army_ghoul_pet_melee_t( p );
      trigger_gcd = 0;
    }

    virtual void execute()
    {
      army_ghoul_pet_t* p = ( army_ghoul_pet_t* ) player -> cast_pet();
      p -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      army_ghoul_pet_t* p = ( army_ghoul_pet_t* ) player -> cast_pet();
      return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct army_ghoul_pet_claw_t : public army_ghoul_pet_attack_t
  {
    army_ghoul_pet_claw_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_attack_t( "claw", 91776, p )
    {
      weapon_power_mod  = 0.0055 / weapon -> swing_time; // FIXME: Needs further testing
    }
  };

  virtual void init_base()
  {
    // FIXME: Copied from the pet ghoul
    attribute_base[ ATTR_STRENGTH  ] = 476;
    attribute_base[ ATTR_AGILITY   ] = 3343;
    attribute_base[ ATTR_STAMINA   ] = 546;
    attribute_base[ ATTR_INTELLECT ] = 69;
    attribute_base[ ATTR_SPIRIT    ] = 116;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;
    initial_attack_power_per_agility  = 0.0;

    // Ghouls don't appear to gain any crit from agi, they may also just have none
    // initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resource_base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
  }

  virtual double energy_regen_per_second() const
  {
    // Doesn't benefit from haste
    return base_energy_regen_per_second;
  }

  virtual double strength() const
  {
    death_knight_t* o = owner -> cast_death_knight();
    double a = attribute[ ATTR_STRENGTH ];
    // copied from the pet ghoul, tested in unholy and frost.
    double strength_scaling = 1.01 + o -> glyphs.raise_dead -> ok() * 0.4254;
    a += snapshot_strength * strength_scaling;
    a *= composite_attribute_multiplier( ATTR_STRENGTH );
    return a;
  }

  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    snapshot_crit     = o -> composite_attack_crit();
    snapshot_haste    = o -> composite_attack_haste();
    snapshot_speed    = o -> composite_attack_speed();
    snapshot_hit      = o -> composite_attack_hit();
    snapshot_strength = o -> strength();
  }

  virtual double composite_attack_expertise() const
  {
    return ( ( 100.0 * snapshot_hit ) * 26.0 / 8.0 ) / 100.0; // Hit gains equal to expertise
  }

  virtual double composite_attack_crit() const { return snapshot_crit; }

  virtual double composite_attack_speed() const { return snapshot_speed; }

  virtual double composite_attack_haste() const { return snapshot_haste; }

  virtual double composite_attack_hit() const { return snapshot_hit; }

  virtual int primary_resource() const { return RESOURCE_ENERGY; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack"    ) return new  army_ghoul_pet_auto_attack_t( this );
    if ( name == "claw"           ) return new         army_ghoul_pet_claw_t( this );

    return pet_t::create_action( name, options_str );
  }

  double available() const
  {
    double energy = resource_current[ RESOURCE_ENERGY ];

    if ( energy > 40 )
      return 0.1;

    return std::max( ( 40 - energy ) / energy_regen_per_second(), 0.1 );
  }
};

// ==========================================================================
// Bloodworms
// ==========================================================================
struct bloodworms_pet_t : public pet_t
{
  // FIXME: Level 80/85 values
  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) :
      attack_t( "bloodworm_melee", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      may_crit    = true;
      background  = true;
      repeating   = true;
    }
  };

  melee_t* melee;

  bloodworms_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "bloodworms", true /*guardian*/ )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 20;
    main_hand_weapon.max_dmg    = 20;
    main_hand_weapon.swing_time = 2.0;
  }

  virtual void init_base()
  {
    pet_t::init_base();

    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;

    melee = new melee_t( this );
  }

  virtual void summon( double duration=0 )
  {
    pet_t::summon( duration );
    melee -> schedule_execute();
  }

  virtual int primary_resource() const { return RESOURCE_MANA; }
};

// ==========================================================================
// Gargoyle
// ==========================================================================

struct gargoyle_pet_t : public pet_t
{

  // FIXME: Did any of these stats change?
  struct gargoyle_strike_t : public spell_t
  {
    gargoyle_strike_t( pet_t* pet ) :
      spell_t( "gargoyle_strike", 51963, pet, true )
    {
      // FIX ME!
      // Resist (can be partial)? Scaling?
      trigger_gcd = 1.5;
      may_crit    = true;
      min_gcd     = 1.5; // issue961

      base_spell_power_multiplier  = 0;
      base_attack_power_multiplier = 1;
    }
  };

  double snapshot_haste, snapshot_spell_crit, snapshot_power;

  gargoyle_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "gargoyle", true ),
    snapshot_haste( 1 ), snapshot_spell_crit( 0 ), snapshot_power( 0 )
  {
  }

  virtual void init_base()
  {
    // FIX ME!
    attribute_base[ ATTR_STRENGTH  ] = 0;
    attribute_base[ ATTR_AGILITY   ] = 0;
    attribute_base[ ATTR_STAMINA   ] = 0;
    attribute_base[ ATTR_INTELLECT ] = 0;
    attribute_base[ ATTR_SPIRIT    ] = 0;

    action_list_str = "/snapshot_stats/gargoyle_strike";
  }
  virtual double composite_spell_haste() const { return snapshot_haste; }
  virtual double composite_attack_power() const { return snapshot_power; }
  virtual double composite_spell_crit() const { return snapshot_spell_crit; }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "gargoyle_strike" ) return new gargoyle_strike_t( this );

    return pet_t::create_action( name, options_str );
  }

  virtual void summon( double duration=0 )
  {
    pet_t::summon( duration );
    // Haste etc. are taken at the time of summoning
    death_knight_t* o = owner -> cast_death_knight();

    snapshot_haste      = o -> composite_attack_speed();
    snapshot_power      = o -> composite_attack_power() * o -> composite_attack_power_multiplier();
    snapshot_spell_crit = o -> composite_spell_crit();

  }

  virtual void arise()
  {
    if ( sim -> log )
      log_t::output( sim, "%s arises.", name() );

    if ( ! initial_sleeping )
      sleeping = 0;

    if ( sleeping )
      return;

    init_resources( true );

    readying = 0;

    arise_time = sim -> current_time;

    schedule_ready( 3.0, true ); // Gargoyle pet is idle for the first 3 seconds.
  }
};

// ==========================================================================
// Pet Ghoul
// ==========================================================================

struct ghoul_pet_t : public pet_t
{
  double snapshot_crit, snapshot_haste, snapshot_speed, snapshot_hit, snapshot_strength;

  ghoul_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "ghoul", true ),
    snapshot_crit( 0 ), snapshot_haste( 0 ), snapshot_speed( 0 ), snapshot_hit( 0 ), snapshot_strength( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 622.43; // should be exact as of 4.2
    main_hand_weapon.max_dmg    = 933.64; // should be exact as of 4.2
    main_hand_weapon.swing_time = 2.0;

    action_list_str = "auto_attack/sweeping_claws/claw";
  }

  struct ghoul_pet_attack_t : public attack_t
  {
    ghoul_pet_attack_t( const char* n, ghoul_pet_t* p, const resource_type r=RESOURCE_ENERGY, bool special=true ) :
      attack_t( n, p, r, SCHOOL_PHYSICAL, TREE_NONE, special )
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
    }

    ghoul_pet_attack_t( const char* n, uint32_t id, ghoul_pet_t* p, bool special=true ) :
      attack_t( n, id, p, 0, special )
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
    }

    virtual void player_buff()
    {
      attack_t::player_buff();

      ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();


      player_multiplier *= 1.0 + o -> buffs_shadow_infusion -> stack() * o -> buffs_shadow_infusion -> effect1().percent();

      if ( o -> buffs_dark_transformation -> up() )
      {
        player_multiplier *= 1.0 + o -> buffs_dark_transformation -> effect1().percent();
      }
    }
  };

  struct ghoul_pet_melee_t : public ghoul_pet_attack_t
  {
    ghoul_pet_melee_t( ghoul_pet_t* p ) :
      ghoul_pet_attack_t( "melee", p, RESOURCE_NONE, false )
    {
      base_execute_time = weapon -> swing_time;
      background        = true;
      repeating         = true;
      weapon_power_mod  = 0.120 / weapon -> swing_time; // should be exact as of 4.2
    }
  };

  struct ghoul_pet_auto_attack_t : public ghoul_pet_attack_t
  {
    ghoul_pet_auto_attack_t( ghoul_pet_t* p ) :
      ghoul_pet_attack_t( "auto_attack", p )
    {
      weapon = &( p -> main_hand_weapon );
      p -> main_hand_attack = new ghoul_pet_melee_t( p );
      trigger_gcd = 0;
    }

    virtual void execute()
    {
      ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
      p -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
      return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct ghoul_pet_claw_t : public ghoul_pet_attack_t
  {
    ghoul_pet_claw_t( ghoul_pet_t* p ) :
      ghoul_pet_attack_t( "claw", 91776, p )
    {
      weapon_power_mod = 0.120 / weapon -> swing_time; // should be exact as of 4.2
    }
  };

  struct ghoul_pet_sweeping_claws_t : public ghoul_pet_attack_t
  {
    ghoul_pet_sweeping_claws_t( ghoul_pet_t* p ) :
      ghoul_pet_attack_t( "sweeping_claws", 91778, p )
    {
      aoe = 2;
      weapon_power_mod = 0.120 / weapon -> swing_time; // Copied from claw, but seems Ok
    }

    virtual bool ready()
    {
      death_knight_t* o = player -> cast_pet() -> owner -> cast_death_knight();

      if ( ! o -> buffs_dark_transformation -> check() )
        return false;

      return ghoul_pet_attack_t::ready();
    }
  };

  virtual void init_base()
  {
    death_knight_t* o = owner -> cast_death_knight();
    assert( o -> primary_tree() != TREE_NONE );
    if ( o -> primary_tree() == TREE_UNHOLY )
      type = PLAYER_PET;

    // Value for the ghoul of a naked worgen as of 4.2
    attribute_base[ ATTR_STRENGTH  ] = 476;
    attribute_base[ ATTR_AGILITY   ] = 3343;
    attribute_base[ ATTR_STAMINA   ] = 546;
    attribute_base[ ATTR_INTELLECT ] = 69;
    attribute_base[ ATTR_SPIRIT    ] = 116;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;
    initial_attack_power_per_agility  = 0.0;//no AP per agi.

    initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resource_base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
  }

  virtual double energy_regen_per_second() const
  {
    // Doesn't benefit from haste
    return base_energy_regen_per_second;
  }

  virtual double strength() const
  {
    death_knight_t* o = owner -> cast_death_knight();
    double a = attribute[ ATTR_STRENGTH ];
    double strength_scaling = 1.01 + o -> glyphs.raise_dead -> ok() * 0.4254; //weird, but accurate

    // Perma Ghouls are updated constantly
    if ( o -> primary_tree() == TREE_UNHOLY )
    {
      a += o -> strength() * strength_scaling;
    }
    else
    {
      a += snapshot_strength * strength_scaling;
    }
    a *= composite_attribute_multiplier( ATTR_STRENGTH );
    return a;
  }

  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    snapshot_crit     = o -> composite_attack_crit();
    snapshot_haste    = o -> composite_attack_haste();
    snapshot_speed    = o -> composite_attack_speed();
    snapshot_hit      = o -> composite_attack_hit();
    snapshot_strength = o -> strength();
  }

  virtual double composite_attack_crit() const
  {
    death_knight_t* o = owner -> cast_death_knight();

    // Perma Ghouls are updated constantly

    if ( o -> primary_tree() == TREE_UNHOLY )
    {
      return o -> composite_attack_crit();
    }
    else
    {
      return snapshot_crit;
    }
  }

  virtual double composite_attack_expertise() const
  {
    death_knight_t* o = owner -> cast_death_knight();

    // Perma Ghouls are updated constantly
    if ( o -> primary_tree() == TREE_UNHOLY )
    {
      return ( ( 100.0 * o -> attack_hit ) * 26.0 / 8.0 ) / 100.0;
    }
    else
    {
      return ( ( 100.0 * snapshot_hit ) * 26.0 / 8.0 ) / 100.0;
    }
  }

  virtual double composite_attack_haste() const
  {
    // Ghouls receive 100% of their master's haste.
    // http://elitistjerks.com/f72/t42606-pet_discussion_garg_aotd_ghoul/
    death_knight_t* o = owner -> cast_death_knight();

    // Perma Ghouls are updated constantly
    if ( o -> primary_tree() == TREE_UNHOLY )
    {
      return o -> composite_attack_haste();
    }
    else
    {
      return snapshot_haste;
    }
  }

  virtual double composite_attack_speed() const
  {
    // Ghouls receive 100% of their master's haste.
    // http://elitistjerks.com/f72/t42606-pet_discussion_garg_aotd_ghoul/
    death_knight_t* o = owner -> cast_death_knight();

    // Perma Ghouls are updated constantly
    if ( o -> primary_tree() == TREE_UNHOLY )
    {
      return o -> composite_attack_speed();
    }
    else
    {
      return snapshot_speed;
    }
  }

  virtual double composite_attack_hit() const
  {
    death_knight_t* o = owner -> cast_death_knight();

    // Perma Ghouls are updated constantly
    if ( o -> primary_tree() == TREE_UNHOLY )
    {
      return o -> composite_attack_hit();
    }
    else
    {
      return snapshot_hit;
    }
  }

  //Ghoul regen doesn't benefit from haste (even bloodlust/heroism)
  virtual int primary_resource() const
  {
    return RESOURCE_ENERGY;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack"    ) return new    ghoul_pet_auto_attack_t( this );
    if ( name == "claw"           ) return new           ghoul_pet_claw_t( this );
    if ( name == "sweeping_claws" ) return new ghoul_pet_sweeping_claws_t( this );

    return pet_t::create_action( name, options_str );
  }

  double available() const
  {
    double energy = resource_current[ RESOURCE_ENERGY ];

    // Cheapest Ability need 40 Energy
    if ( energy > 40 )
      return 0.1;

    return std::max( ( 40 - energy ) / energy_regen_per_second(), 0.1 );
  }
};
// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_attack_t : public attack_t
{
  bool   requires_weapon;
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  double convert_runes;
  double m_dd_additive; // Multipler for Direct Damage that are all additive with each other
  bool   use[RUNE_SLOT_MAX];
  gain_t* rp_gains;

  death_knight_attack_t( const char* n, death_knight_t* p, bool special = false ) :
    attack_t( n, p, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, special ),
    requires_weapon( true ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( 0 ),
    m_dd_additive( 0 )
  {
    _init_dk_attack();
  }

  death_knight_attack_t( const char* n, uint32_t id, death_knight_t* p ) :
    attack_t( n, id, p, 0, true ),
    requires_weapon( true ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( 0 ),
    m_dd_additive( 0 )
  {
    _init_dk_attack();
  }

  death_knight_attack_t( const char* n, const char* sname, death_knight_t* p ) :
    attack_t( n, sname, p, 0, true ),
    requires_weapon( true ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( 0 ),
    m_dd_additive( 0 )
  {
    _init_dk_attack();
  }

  void _init_dk_attack()
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;

    may_crit   = true;
    may_glance = false;

    death_knight_t* p = player -> cast_death_knight();

    if ( p -> main_hand_weapon.group() == WEAPON_2H )
      m_dd_additive += p -> talents.might_of_the_frozen_wastes -> effect3().percent();

    rp_gains = player -> get_gain( "rp_" + name_str );
  }

  virtual void   reset();
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual bool   ready();
  virtual double swing_haste() const;
  virtual void   target_debuff( player_t* t, int dmg_type );
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

struct death_knight_spell_t : public spell_t
{
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  double convert_runes;
  bool   use[RUNE_SLOT_MAX];
  gain_t* rp_gains;

  death_knight_spell_t( const char* n, uint32_t id, death_knight_t* p ) :
    spell_t( n, id, p ),
    cost_blood( 0 ), cost_frost( 0 ), cost_unholy( 0 ), convert_runes( 0 )
  {
    _init_dk_spell();
  }

  death_knight_spell_t( const char* n, const char* sname, death_knight_t* p ) :
    spell_t( n, sname, p ),
    cost_blood( 0 ), cost_frost( 0 ), cost_unholy( 0 ), convert_runes( 0 )
  {
    _init_dk_spell();
  }

  void _init_dk_spell()
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;

    may_crit = true;
    // DKs have 2.09x spell crits with meta gem so they must use the "hybrid" formula of adjusting the crit-bonus multiplier
    // (As opposed to the native 1.33 crit multiplier used by Mages and Warlocks.)
    crit_bonus_multiplier = 2.0;

    base_spell_power_multiplier = 0;
    base_attack_power_multiplier = 1;

    rp_gains = player -> get_gain( "rp_" + name_str );
  }

  virtual void   reset();
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual void   target_debuff( player_t* t, int dmg_type );
  virtual bool   ready();
};

// ==========================================================================
// Local Utility Functions
// ==========================================================================

static void extract_rune_cost( const spell_id_t* spell, int* cost_blood, int* cost_frost, int* cost_unholy )
{
  // Rune costs appear to be in binary: 0a0b0c where 'c' is whether the ability
  // costs a blood rune, 'b' is whether it costs an unholy rune, and 'a'
  // is whether it costs a frost rune.

  if ( ! spell -> ok() ) return;

  uint32_t rune_cost = spell -> rune_cost();
  *cost_blood  =        rune_cost & 0x1;
  *cost_unholy = ( rune_cost >> 2 ) & 0x1;
  *cost_frost  = ( rune_cost >> 4 ) & 0x1;
}

// Count Runes ==============================================================

// currently not used. but useful. commenting out to get rid of compile warning
//static int count_runes( player_t* player )
//{
//  death_knight_t* p = player -> cast_death_knight();
//  int count_by_type[RUNE_SLOT_MAX / 2] = { 0, 0, 0 }; // blood, frost, unholy
//
//  for ( int i = 0; i < RUNE_SLOT_MAX / 2; ++i )
//    count_by_type[i] = ( ( int )p -> _runes.slot[2 * i].is_ready() +
//                         ( int )p -> _runes.slot[2 * i + 1].is_ready() );
//
//  return count_by_type[0] + ( count_by_type[1] << 8 ) + ( count_by_type[2] << 16 );
//}

// Count Death Runes ========================================================

static int count_death_runes( death_knight_t* p, bool inactive )
{
  // Getting death rune count is a bit more complicated as it depends
  // on talents which runetype can be converted to death runes
  int count = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( ( inactive || r.is_ready() ) && r.is_death() )
      ++count;
  }
  return count;
}

// Consume Runes ============================================================

static void consume_runes( player_t* player, const bool use[RUNE_SLOT_MAX], bool convert_runes = false )
{
  death_knight_t* p = player -> cast_death_knight();

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    if ( use[i] )
    {
      // Show the consumed type of the rune
      // Not the type it is after consumption
      int consumed_type = p -> _runes.slot[i].type;
      p -> _runes.slot[i].consume( convert_runes );

      if ( p -> sim -> log )
        log_t::output( p -> sim, "%s consumes rune #%d, type %d", p -> name(), i, consumed_type );
    }
  }

  if ( p -> sim -> log )
  {
    log_rune_status( p );
  }
}

// Group Runes ==============================================================

static bool group_runes ( player_t* player, int blood, int frost, int unholy, bool group[RUNE_SLOT_MAX] )
{
  death_knight_t* p = player -> cast_death_knight();
  int cost[]  = { blood + frost + unholy, blood, frost, unholy };
  bool use[RUNE_SLOT_MAX] = { false };

  // Selecting available non-death runes to satisfy cost
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready() && ! r.is_death() && cost[r.get_type()] > 0 )
    {
      --cost[r.get_type()];
      --cost[0];
      use[i] = true;
    }
  }

  // Selecting available death runes to satisfy remaining cost
  for ( int i = RUNE_SLOT_MAX; cost[0] > 0 && i--; )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready() && r.is_death() )
    {
      --cost[0];
      use[i] = true;
    }
  }

  if ( cost[0] > 0 ) return false;

  // Storing rune slots selected
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) group[i] = use[i];

  return true;
}

// Refund Power =============================================================

static void refund_power( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( a -> resource_consumed > 0 )
    p -> resource_gain( RESOURCE_RUNIC, a -> resource_consumed * RUNIC_POWER_REFUND, p -> gains_power_refund );
}

// ==========================================================================
// Triggers
// ==========================================================================

// Trigger Blood Caked Blade ================================================

static void trigger_blood_caked_blade( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> talents.blood_caked_blade -> rank() )
    return;

  if ( p -> rng_blood_caked_blade -> roll( p -> talents.blood_caked_blade -> proc_chance() ) )
  {
    struct bcs_t : public death_knight_attack_t
    {
      bcs_t( death_knight_t* p ) :
        death_knight_attack_t( "blood_caked_strike", 50463, p )
      {
        may_crit       = false;
        background     = true;
        proc           = true;
        trigger_gcd    = false;
        weapon = &( player -> main_hand_weapon );
        normalize_weapon_speed = false;
        init();
      }

      virtual void target_debuff( player_t* t, int dmg_type )
      {
        death_knight_attack_t::target_debuff( t, dmg_type );
        death_knight_targetdata_t* td = targetdata() -> cast_death_knight();

        target_multiplier *= 1.0 + td -> diseases() * effect1().percent() / 2.0;
      }
    };

    if ( ! p -> active_blood_caked_blade ) p -> active_blood_caked_blade = new bcs_t( p );
    p -> active_blood_caked_blade -> weapon = a -> weapon;
    p -> active_blood_caked_blade -> execute();
  }
}

// Trigger Brittle Bones ====================================================

static void trigger_brittle_bones( spell_t* s, player_t* t )
{
  death_knight_t* p = s -> player -> cast_death_knight();

  if ( ! p -> talents.brittle_bones -> rank() )
    return;

  double bb_value = p -> talents.brittle_bones -> effect2().percent();

  if ( bb_value >= t -> debuffs.brittle_bones -> current_value )
  {
    t -> debuffs.brittle_bones -> trigger( 1, bb_value );
    t -> debuffs.brittle_bones -> source = p;
  }
}

// Trigger Ebon Plaguebringer ===============================================

static void trigger_ebon_plaguebringer( action_t* a, player_t* t )
{
  death_knight_t* p = a -> player -> cast_death_knight();
  death_knight_targetdata_t* td = a -> targetdata() -> cast_death_knight();

  if ( ! p -> talents.ebon_plaguebringer -> rank() )
    return;

  // Each DK gets their own ebon plaguebringer debuff, but we only track one a time, so fake it
  td -> debuffs_ebon_plaguebringer -> trigger();

  if ( a -> sim -> overrides.ebon_plaguebringer )
    return;

  double duration = 21.0 + p -> talents.epidemic -> effect1().seconds();
  if ( t -> debuffs.ebon_plaguebringer -> remains_lt( duration ) )
  {
    t -> debuffs.ebon_plaguebringer -> buff_duration = duration;
    t -> debuffs.ebon_plaguebringer -> trigger( 1, 8.0 );
    t -> debuffs.ebon_plaguebringer -> source = p;
  }
}

// Trigger Unholy Blight ====================================================

static void trigger_unholy_blight( action_t* a, double death_coil_dmg )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> talents.unholy_blight -> rank() )
    return;

  assert( p -> active_unholy_blight );

  double unholy_blight_dmg = death_coil_dmg * p -> talents.unholy_blight -> effect1().percent();

  // http://code.google.com/p/simulationcraft/issues/detail?id=939
  if ( p -> bugs )
    unholy_blight_dmg *= 0.75;

  dot_t* dot = p -> active_unholy_blight -> dot();

  if ( dot -> ticking )
  {
    unholy_blight_dmg += p -> active_unholy_blight -> base_td * dot -> ticks();

    p -> active_unholy_blight -> dot() -> cancel();
  }
  p -> active_unholy_blight -> base_td = unholy_blight_dmg / p -> active_unholy_blight -> num_ticks;
  p -> active_unholy_blight -> execute();
}

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

// death_knight_attack_t::reset() ===========================================

void death_knight_attack_t::reset()
{
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;

  action_t::reset();
}

// death_knight_attack_t::consume_resource() ================================

void death_knight_attack_t::consume_resource()
{
  death_knight_t* p = player -> cast_death_knight();

  if ( rp_gain > 0 )
  {
    if ( result_is_hit() )
    {
      if ( p -> buffs_frost_presence -> check() )
      {
        p -> resource_gain( RESOURCE_RUNIC,
                            rp_gain * player -> dbc.spell( 48266 ) -> effect2().percent(),
                            p -> gains_frost_presence );
      }
      if ( p -> talents.improved_frost_presence -> rank() && ! p -> buffs_frost_presence -> check() )
      {
        p -> resource_gain( RESOURCE_RUNIC,
                            rp_gain * p -> talents.improved_frost_presence -> effect1().percent(),
                            p -> gains_improved_frost_presence );
      }
      p -> resource_gain( RESOURCE_RUNIC, rp_gain, rp_gains );
    }
  }
  else
  {
    attack_t::consume_resource();
  }

  if ( result_is_hit() )
    consume_runes( player, use, convert_runes == 0 ? false : sim -> roll( convert_runes ) == 1 );
  else
    refund_power( this );
}

// death_knight_attack_t::execute() =========================================

void death_knight_attack_t::execute()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::execute();

  if ( result_is_hit() )
  {
    p -> buffs_bloodworms -> trigger();
    if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
      if ( ! proc )
        p -> buffs_rune_of_cinderglacier -> decrement();
  }
}

// death_knight_attack_t::player_buff() =====================================

void death_knight_attack_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::player_buff();

  if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
    if ( ! proc )
      player_multiplier *= 1.0 + p -> buffs_rune_of_cinderglacier -> value();

  // Add in all m_dd_additive
  player_multiplier *= 1.0 + m_dd_additive;
}

// death_knight_attack_t::ready() ===========================================

bool death_knight_attack_t::ready()
{
  death_knight_t* p = player -> cast_death_knight();

  if ( ! attack_t::ready() )
    return false;

  if ( requires_weapon )
    if ( ! weapon || weapon -> group() == WEAPON_RANGED )
      return false;

  return group_runes( p, cost_blood, cost_frost, cost_unholy, use );
}

// death_knight_attack_t::swing_haste() =====================================

double death_knight_attack_t::swing_haste() const
{
  double haste = attack_t::swing_haste();
  death_knight_t* p = player -> cast_death_knight();

  if ( p -> primary_tree() == TREE_FROST )
    haste *= 1.0 / ( 1.0 + p -> spells.icy_talons -> effect1().percent() );

  if ( p -> talents.improved_icy_talons -> rank() )
    haste *= 1.0 / ( 1.0 + p -> talents.improved_icy_talons -> effect3().percent() );

  return haste;
}

// death_knight_attack_t::target_debuff =====================================

void death_knight_attack_t::target_debuff( player_t* t, int dmg_type )
{
  attack_t::target_debuff( t, dmg_type );
  death_knight_t* p = player -> cast_death_knight();

  if ( school == SCHOOL_FROST  )
  {
    target_multiplier *= 1.0 + p -> buffs_rune_of_razorice -> stack() * p -> buffs_rune_of_razorice -> effect1().percent();
  }
}

// ==========================================================================
// Death Knight Spell Methods
// ==========================================================================

// death_knight_spell_t::reset() ============================================

void death_knight_spell_t::reset()
{
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  spell_t::reset();
}

// death_knight_spell_t::consume_resource() =================================

void death_knight_spell_t::consume_resource()
{
  death_knight_t* p = player -> cast_death_knight();
  if ( rp_gain > 0 )
  {
    if ( result_is_hit() )
    {
      if ( p -> buffs_frost_presence -> check() )
      {
        p -> resource_gain( RESOURCE_RUNIC,
                            rp_gain * player -> dbc.spell( 48266 ) -> effect2().percent(),
                            p -> gains_frost_presence );
      }
      if ( p -> talents.improved_frost_presence -> rank() && ! p -> buffs_frost_presence -> check() )
      {
        p -> resource_gain( RESOURCE_RUNIC,
                            rp_gain * p -> talents.improved_frost_presence -> effect1().percent(),
                            p -> gains_improved_frost_presence );
      }
      p -> resource_gain( RESOURCE_RUNIC, rp_gain, rp_gains );
    }
  }
  else
  {
    spell_t::consume_resource();
  }

  if ( result_is_hit() )
    consume_runes( player, use, convert_runes == 0 ? false : sim -> roll( convert_runes ) == 1 );
}

// death_knight_spell_t::execute() ==========================================

void death_knight_spell_t::execute()
{
  spell_t::execute();

  if ( result_is_hit() )
  {
    if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
    {
      death_knight_t* p = player -> cast_death_knight();
      p -> buffs_rune_of_cinderglacier -> decrement();
    }
  }
}

// death_knight_spell_t::player_buff() ======================================

void death_knight_spell_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  spell_t::player_buff();

  if ( ( school == SCHOOL_FROST || school == SCHOOL_SHADOW ) )
    player_multiplier *= 1.0 + p -> buffs_rune_of_cinderglacier -> value();

  if ( sim -> debug )
    log_t::output( sim, "death_knight_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f, p_mult=%.0f",
                   name(), player_hit, player_crit, player_spell_power, player_penetration, player_multiplier );
}

// death_knight_spell_t::ready() ============================================

bool death_knight_spell_t::ready()
{
  if ( ! spell_t::ready() )
    return false;

  if ( ! player -> in_combat && ! harmful )
    return group_runes( player, 0, 0, 0, use );
  else
    return group_runes( player, cost_blood, cost_frost, cost_unholy, use );
}

// death_knight_spell_t::target_debuff ======================================

void death_knight_spell_t::target_debuff( player_t* t, int dmg_type )
{
  spell_t::target_debuff( t, dmg_type );
  death_knight_t* p = player -> cast_death_knight();

  if ( school == SCHOOL_FROST  )
  {
    target_multiplier *= 1.0 + p -> buffs_rune_of_razorice -> stack() * p -> buffs_rune_of_razorice -> effect1().percent();
  }
}

// Flaming Torment ( Tier 12 4pc ) ==========================================

struct flaming_torment_t : public death_knight_spell_t
{
  flaming_torment_t( const char* n, death_knight_t* player ) :
    death_knight_spell_t( n, 99000, player )
  {
    background       = true;
    may_miss         = false;
    proc             = true;
    may_crit         = false;
  }

  virtual double calculate_direct_damage()
  {
    death_knight_t* p = player -> cast_death_knight();
    double dmg = base_dd_min * p -> sets -> set( SET_T12_4PC_MELEE ) -> effect1().percent();

    if ( ! binary )
    {
      dmg *= 1.0 - resistance();
    }

    return dmg;
  }
};

// ==========================================================================
// Death Knight Attacks
// ==========================================================================

// Melee Attack =============================================================

struct melee_t : public death_knight_attack_t
{
  int sync_weapons;
  melee_t( const char* name, death_knight_t* p, int sw ) :
    death_knight_attack_t( name, p ), sync_weapons( sw )
  {
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;
    base_cost       = 0;

    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

  virtual double execute_time() const
  {
    double t = death_knight_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, 0.2 ) : t/2 ) : 0.01;
    }
    return t;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();

    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      death_knight_targetdata_t* td = targetdata() -> cast_death_knight();

      if ( weapon -> slot == SLOT_MAIN_HAND )
      {
        // T13 2pc gives 2 stacks of SD, otherwise we can only ever have one
        // Ensure that if we have 1 that we only refresh, not add another stack
        int new_stacks = ( p -> set_bonus.tier13_2pc_melee() && sim -> roll( p -> sets -> set( SET_T13_2PC_MELEE ) -> effect1().percent() ) ) ? 2 : 1;

        if ( sim -> roll( weapon -> proc_chance_on_swing( p -> talents.sudden_doom -> rank() ) ) )
        {
          // If we're proccing 2 or we have 0 stacks, trigger like normal
          if ( new_stacks == 2 || p -> buffs_sudden_doom -> check() == 0 )
          {
            p -> buffs_sudden_doom -> trigger( new_stacks );
          }
          // refresh stacks. However if we have a double stack and only 1 procced, it refreshes to 1 stack
          else
          {
            p -> buffs_sudden_doom -> refresh( 0 );
            if ( p -> buffs_sudden_doom -> check() == 2 && new_stacks == 1 )
              p -> buffs_sudden_doom -> decrement( 1 );
          }
        }
      }

      // TODO: Confirm PPM for ranks 1 and 2 http://elitistjerks.com/f72/t110296-frost_dps_|_cataclysm_4_0_3_nothing_lose/p9/#post1869431
      double chance = weapon -> proc_chance_on_swing( util_t::talent_rank( p -> talents.killing_machine -> rank(), 3, 1, 3, 5 ) );
      if ( p -> buffs_killing_machine -> trigger( 1, p -> buffs_killing_machine -> effect1().percent(), chance ) )
        p -> buffs_tier11_4pc_melee -> trigger();

      if ( td -> dots_blood_plague && td -> dots_blood_plague -> ticking )
        p -> buffs_crimson_scourge -> trigger();

      trigger_blood_caked_blade( this );
      if ( p -> buffs_scent_of_blood -> up() )
      {
        p -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_scent_of_blood );
        p -> buffs_scent_of_blood -> decrement();
      }

      if ( p -> rng_might_of_the_frozen_wastes -> roll( p -> talents.might_of_the_frozen_wastes -> proc_chance() ) )
      {
        p -> resource_gain( RESOURCE_RUNIC,
                            p -> dbc.spell( p -> talents.might_of_the_frozen_wastes -> effect1().trigger_spell_id() ) -> effect1().resource( RESOURCE_RUNIC ),
                            p -> gains_might_of_the_frozen_wastes );
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public death_knight_attack_t
{
  int sync_weapons;

  auto_attack_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "auto_attack", p ), sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      p -> off_hand_attack -> base_multiplier *= 1.0 + p -> talents.nerves_of_cold_steel -> effect2().percent();
    }

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    p -> main_hand_attack -> schedule_execute();
    if ( p -> off_hand_attack )
    {
      p -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> is_moving() )
      return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

// Army of the Dead =========================================================

struct army_of_the_dead_t : public death_knight_spell_t
{
  army_of_the_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "army_of_the_dead", "Army of the Dead", p )
  {
    parse_options( NULL, options_str );

    harmful     = false;
    channeled   = true;
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( ! p -> in_combat )
    {
      // Pre-casting it before the fight
      int saved_ticks = num_ticks;
      num_ticks = 0;
      channeled = false;
      death_knight_spell_t::execute();
      channeled = true;
      num_ticks = saved_ticks;
      // Because of the new rune regen system in 4.0, it only makes
      // sense to cast ghouls 7-10s before a fight begins so you don't
      // waste rune regen and enter the fight depleted.  So, the time
      // you get for ghouls is 4-6 seconds less.
      p -> active_army_ghoul -> summon( 35.0 );
    }
    else
    {
      death_knight_spell_t::execute();

      death_knight_t* p = player -> cast_death_knight();

      p -> active_army_ghoul -> summon( 40.0 );
    }
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> active_army_ghoul && ! p -> active_army_ghoul -> sleeping )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Blood Boil ===============================================================

struct blood_boil_t : public death_knight_spell_t
{
  blood_boil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_boil", "Blood Boil", p )
  {
    parse_options( NULL, options_str );

    aoe                = -1;
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    direct_power_mod   = 0.08; // hardcoded into tooltip, 31/10/2011
    player_multiplier *= 1.0 + p -> talents.crimson_scourge -> mod_additive( P_GENERIC );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_blood_boil -> execute();

    if ( p -> buffs_crimson_scourge -> up() )
      p -> buffs_crimson_scourge -> expire();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_spell_t::target_debuff( t, dmg_type );

    death_knight_targetdata_t* td = targetdata() -> cast_death_knight();

    base_dd_adder = ( td -> diseases() ? 95 : 0 );
    direct_power_mod  = 0.08 + ( td -> diseases() ? 0.035 : 0 );
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( ! spell_t::ready() )
      return false;

    if ( ( ! p -> in_combat && ! harmful ) || p -> buffs_crimson_scourge -> check() )
      return group_runes( p, 0, 0, 0, use );
    else
      return group_runes( p, cost_blood, cost_frost, cost_unholy, use );

  }
};

// Blood Plague =============================================================

struct blood_plague_t : public death_knight_spell_t
{
  blood_plague_t( death_knight_t* p ) :
    death_knight_spell_t( "blood_plague", 59879, p )
  {
    crit_bonus            = 1.0;
    crit_bonus_multiplier = 1.0;

    base_td          = effect_average( 1 ) * 1.15;
    base_tick_time   = 3.0;
    tick_may_crit    = true;
    background       = true;
    num_ticks        = 7 + util_t::talent_rank( p -> talents.epidemic -> rank(), 3, 1, 3, 4 );
    tick_power_mod   = 0.055 * 1.15;
    dot_behavior     = DOT_REFRESH;
    base_multiplier *= 1.0 + p -> talents.ebon_plaguebringer -> effect1().percent();
    base_multiplier *= 1.0 + p -> talents.virulence -> mod_additive( P_TICK_DAMAGE );
    may_miss         = false;
    may_crit         = false;
    hasted_ticks     = false;
  }
};

// Blood Strike =============================================================

struct blood_strike_offhand_t : public death_knight_attack_t
{
  blood_strike_offhand_t( death_knight_t* p ) :
    death_knight_attack_t( "blood_strike_offhand", 66215, p )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    rp_gain          = 0;
    cost_blood       = 0;
    base_multiplier *= 1.0 + p -> talents.nerves_of_cold_steel -> effect2().percent();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_targetdata_t* td = targetdata() -> cast_death_knight();
    death_knight_attack_t::target_debuff( t, dmg_type );

    target_multiplier *= 1 + td -> diseases() * 0.1875; // Currently giving a 18.75% increase per disease instead of expected 12.5
  }
};

struct blood_strike_t : public death_knight_attack_t
{
  attack_t* oh_attack;

  blood_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "blood_strike", "Blood Strike", p ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );

    if ( p -> primary_tree() == TREE_FROST ||
         p -> primary_tree() == TREE_UNHOLY )
      convert_runes = 1.0;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new blood_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_attack_t::execute();

    death_knight_t* p = player -> cast_death_knight();

    if ( oh_attack )
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect1().percent() ) )
        oh_attack -> execute();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_attack_t::target_debuff( t, dmg_type );

    death_knight_targetdata_t* td = targetdata() -> cast_death_knight();

    target_multiplier *= 1 + td -> diseases() * 0.1875; // Currently giving a 18.75% increase per disease instead of expected 12.5
  }
};

// Blood Tap ================================================================

struct blood_tap_t : public death_knight_spell_t
{
  blood_tap_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_tap", "Blood Tap", p )
  {
    parse_options( NULL, options_str );

    base_cost = 0.0; // Cost is stored as 6 in the DBC for some odd reason
    harmful   = false;
    if ( p -> talents.improved_blood_tap -> rank() )
      cooldown -> duration += p -> talents.improved_blood_tap -> mod_additive( P_COOLDOWN );
  }

  void execute()
  {
    death_knight_t* p = player -> cast_death_knight();

    // Blood tap has some odd behavior.  One of the oddest is that, if
    // you have a death rune on cooldown and a full blood rune, using
    // it will take the death rune off cooldown and turn the blood
    // rune into a death rune!  This allows for a nice sequence for
    // Unholy Death Knights: "IT PS BS SS tap SS" which replaces a
    // blood strike with a scourge strike.

    // Find a non-death blood rune and refresh it.
    bool rune_was_refreshed = false;
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_death() && ! r.is_ready() )
      {
        p -> gains_blood_tap       -> add( 1 - r.value, r.value );
        // p -> gains_blood_tap_blood -> add(1 - r.value, r.value);
        r.fill_rune();
        rune_was_refreshed = true;
        break;
      }
    }
    // Couldn't find a non-death blood rune needing refreshed?
    // Instead, refresh a death rune that is a blood rune.
    if ( ! rune_was_refreshed )
    {
      for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
      {
        dk_rune_t& r = p -> _runes.slot[i];
        if ( r.get_type() == RUNE_TYPE_BLOOD && r.is_death() && ! r.is_ready() )
        {
          p -> gains_blood_tap       -> add( 1 - r.value, r.value );
          // p -> gains_blood_tap_blood -> add(1 - r.value, r.value);
          r.fill_rune();
          rune_was_refreshed = true;
          break;
        }
      }
    }

    // Now find a ready blood rune and turn it into a death rune.
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_death() && r.is_ready() )
      {
        r.type = ( r.type & RUNE_TYPE_MASK ) | RUNE_TYPE_DEATH;
        break;
      }
    }

    // Called last so we print the correct runes
    death_knight_spell_t::execute();
  }
};

// Bone Shield ==============================================================

struct bone_shield_t : public death_knight_spell_t
{
  bone_shield_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "bone_shield", "Bone Shield", p )
  {
    check_talent( p -> talents.bone_shield -> rank() );

    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    harmful = false;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( ! p -> in_combat )
    {
      // Pre-casting it before the fight, perfect timing would be so
      // that the used rune is ready when it is needed in the
      // rotation.  Assume we casted Bone Shield somewhere between
      // 8-16s before we start fighting.  The cost in this case is
      // zero and we don't cause any cooldown.
      double pre_cast = p -> sim -> range( 8.0, 16.0 );

      cooldown -> duration -= pre_cast;
      p -> buffs_bone_shield -> buff_duration -= pre_cast;

      p -> buffs_bone_shield -> trigger( 1, p -> talents.bone_shield -> effect2().percent() );
      death_knight_spell_t::execute();

      cooldown -> duration += pre_cast;
      p -> buffs_bone_shield -> buff_duration += pre_cast;
    }
    else
    {
      p -> buffs_bone_shield -> trigger( 1, p -> talents.bone_shield -> effect2().percent() );
      death_knight_spell_t::execute();
    }
  }
};

// Dancing Rune Weapon ======================================================

struct dancing_rune_weapon_t : public death_knight_spell_t
{
  dancing_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dancing_rune_weapon", "Dancing Rune Weapon", p )
  {
    check_talent( p -> talents.dancing_rune_weapon -> rank() );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = player -> cast_death_knight();

    p -> buffs_dancing_rune_weapon -> trigger();
    p -> active_dancing_rune_weapon -> summon( duration() );
  }
};

// Dark Transformation ======================================================

struct dark_transformation_t : public death_knight_spell_t
{
  dark_transformation_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dark_transformation", "Dark Transformation", p )
  {
    check_talent( p -> talents.dark_transformation -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    base_cost = 0; // DBC has a value of 9 of an unknown resource
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    p -> buffs_dark_transformation -> trigger();
    p -> buffs_shadow_infusion -> expire();
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_shadow_infusion -> check() != p -> buffs_shadow_infusion -> max_stack )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Death and Decay ==========================================================

struct death_and_decay_t : public death_knight_spell_t
{
  death_and_decay_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_and_decay", "Death and Decay", p )
  {
    parse_options( NULL, options_str );

    aoe              = -1;
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    tick_power_mod   = 0.064;
    base_td          = p -> dbc.effect_average( effect1().id(), p -> level );
    base_tick_time   = 1.0;
    num_ticks        = 10; // 11 with tick_zero
    tick_may_crit    = true;
    tick_zero        = true;
    hasted_ticks     = false;
    base_multiplier *= 1.0 + p -> talents.morbidity -> effect2().percent();
    double n_ticks   = num_ticks * ( 1.0 + p -> glyphs.death_and_decay -> effect1().percent() );
    num_ticks        = ( int ) n_ticks;
  }
};

// Death Coil ===============================================================

struct death_coil_t : public death_knight_spell_t
{
  death_coil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", "Death Coil", p )
  {
    parse_options( NULL, options_str );

    direct_power_mod = 0.23;
    base_dd_min      = p -> dbc.effect_min( effect_id( 1 ), p -> level );
    base_dd_max      = p -> dbc.effect_max( effect_id( 1 ), p -> level );

    base_multiplier *= 1.0 + p -> talents.morbidity -> effect1().percent()
                       + p -> glyphs.death_coil -> effect1().percent();

    base_crit     += p -> sets -> set( SET_T11_2PC_MELEE ) -> effect1().percent();

    if ( p -> talents.runic_corruption -> rank() )
      base_cost += p -> talents.runic_corruption -> mod_additive( P_RESOURCE_COST ) / 10.0;
  }

  virtual double cost() const
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_sudden_doom -> check() ) return 0;

    return death_knight_spell_t::cost();
  }

  void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = player -> cast_death_knight();

    p -> buffs_sudden_doom -> decrement();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_death_coil -> execute();

    if ( result_is_hit() )
    {
      trigger_unholy_blight( this, direct_dmg );
      p -> trigger_runic_empowerment();
    }

    if ( ! p -> buffs_dark_transformation -> check() )
      p -> buffs_shadow_infusion -> trigger(); // Doesn't stack while your ghoul is empowered
  }
};

// Death Strike =============================================================

struct death_strike_offhand_t : public death_knight_attack_t
{
  death_strike_offhand_t( death_knight_t* p ) :
    death_knight_attack_t( "death_strike_offhand", 66188, p )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );

    base_crit       +=     p -> talents.improved_death_strike -> mod_additive( P_CRIT );
    base_multiplier *= 1 + p -> talents.improved_death_strike -> mod_additive( P_GENERIC );
  }

};

struct death_strike_t : public death_knight_attack_t
{
  attack_t* oh_attack;

  death_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "death_strike", "Death Strike", p ),
    oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    if ( p -> primary_tree() == TREE_BLOOD )
      convert_runes = 1.0;

    base_crit       +=     p -> talents.improved_death_strike -> mod_additive( P_CRIT );
    base_multiplier *= 1 + p -> talents.improved_death_strike -> mod_additive( P_GENERIC );

    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new death_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_attack_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_death_strike -> execute();

    if ( oh_attack )
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect1().percent() ) )
        oh_attack -> execute();
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> glyphs.death_strike -> ok() )
    {
      player_multiplier *= 1 + p -> resource_current[ RESOURCE_RUNIC ] / 5 * p -> glyphs.death_strike -> effect1().percent();
    }
  }
};

// Empower Rune Weapon ======================================================

struct empower_rune_weapon_t : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "empower_rune_weapon", 47568, p )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    double erw_gain = 0.0;
    double erw_over = 0.0;
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      erw_gain += 1-r.value;
      erw_over += r.value;
      r.fill_rune();
    }
    p -> gains_empower_rune_weapon -> add( erw_gain, erw_over );
  }
};

// Festering Strike =========================================================

struct festering_strike_t : public death_knight_attack_t
{
  festering_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "festering_strike", "Festering Strike", p )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    if ( p -> primary_tree() == TREE_UNHOLY )
    {
      convert_runes = 1.0;
    }

    base_multiplier *= 1.0 + p -> talents.rage_of_rivendare -> mod_additive( P_GENERIC );
  }

  virtual void execute()
  {
    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      death_knight_targetdata_t* td = targetdata() -> cast_death_knight();
      td -> dots_blood_plague -> extend_duration_seconds( 8 );
      td -> dots_frost_fever  -> extend_duration_seconds( 8 );
      if ( td -> dots_blood_plague -> ticking || td -> dots_frost_fever -> ticking )
        trigger_ebon_plaguebringer( this, target );
    }
  }
};

// Frost Fever ==============================================================

struct frost_fever_t : public death_knight_spell_t
{
  frost_fever_t( death_knight_t* p ) :
    death_knight_spell_t( "frost_fever", 59921, p )
  {
    base_td          = effect_average( 1 ) * 1.15;
    base_tick_time   = 3.0;
    hasted_ticks     = false;
    may_miss         = false;
    may_crit         = false;
    background       = true;
    tick_may_crit    = true;
    dot_behavior     = DOT_REFRESH;
    num_ticks        = 7 + util_t::talent_rank( p -> talents.epidemic -> rank(), 3, 1, 3, 4 );
    tick_power_mod   = 0.055 * 1.15;
    base_multiplier *= 1.0 + p -> talents.ebon_plaguebringer -> effect1().percent();
    base_multiplier *= 1.0 + p -> glyphs.icy_touch -> effect1().percent()
                       + p -> talents.virulence -> effect1().percent();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    death_knight_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      trigger_brittle_bones( this, t );
  }

  virtual void last_tick( dot_t* d )
  {
    death_knight_spell_t::last_tick( d );

    if ( target -> debuffs.brittle_bones -> check()
         && target -> debuffs.brittle_bones -> source
         && target -> debuffs.brittle_bones -> source == player )
      target -> debuffs.brittle_bones -> expire();
  }
};

// Frost Strike =============================================================

struct frost_strike_offhand_t : public death_knight_attack_t
{
  frost_strike_offhand_t( death_knight_t* p ) :
    death_knight_attack_t( "frost_strike_offhand", 66196, p )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    base_multiplier *= 1.0 + p -> talents.nerves_of_cold_steel -> effect2().percent();

    rp_gain = 0; // Incorrectly set to 10 in the DBC

    base_crit += p -> sets -> set( SET_T11_2PC_MELEE ) -> effect1().percent();
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_attack_t::target_debuff( t, dmg_type );

    death_knight_t* p = player -> cast_death_knight();

    if ( t -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect1().percent();
  }
};

struct frost_strike_t : public death_knight_attack_t
{
  attack_t* oh_attack;

  frost_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "frost_strike", "Frost Strike", p ), oh_attack( 0 )
  {
    check_spec( TREE_FROST );

    parse_options( NULL, options_str );

    weapon     = &( p -> main_hand_weapon );
    base_cost += p -> glyphs.frost_strike -> effect1().base_value() / 10;
    base_crit += p -> sets -> set( SET_T11_2PC_MELEE ) -> effect1().percent();

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new frost_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::execute();

    if ( result_is_hit() )
      p -> trigger_runic_empowerment();

    if ( oh_attack )
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect1().percent() ) )
        oh_attack -> execute();

    if ( p -> buffs_killing_machine -> check() )
      p -> proc_fs_killing_machine -> occur();

    p -> buffs_killing_machine -> expire();
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_attack_t::target_debuff( t, dmg_type );

    death_knight_t* p = player -> cast_death_knight();

    if ( t -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect1().percent();
  }
};

// Heart Strike =============================================================

struct heart_strike_t : public death_knight_attack_t
{
  heart_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "heart_strike", "Heart Strike", p )
  {
    check_spec( TREE_BLOOD );

    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );

    base_multiplier *= 1 + p -> glyphs.heart_strike -> effect1().percent();

    aoe = 2;
    base_add_multiplier *= 0.75;
  }

  void execute()
  {
    death_knight_attack_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    if ( result_is_hit() )
    {
      if ( p -> buffs_dancing_rune_weapon -> check() )
      {
        p -> active_dancing_rune_weapon -> drw_heart_strike -> execute();
      }
    }
  }

  void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_targetdata_t* td = targetdata() -> cast_death_knight();

    death_knight_attack_t::target_debuff( t, dmg_type );

    target_multiplier *= 1 + td -> diseases() * effect3().percent();
  }
};

// Horn of Winter============================================================

struct horn_of_winter_t : public death_knight_spell_t
{
  double bonus;

  horn_of_winter_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "horn_of_winter", 57330, p ), bonus( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;
    bonus   = player -> dbc.effect_average( effect_id( 1 ), player -> level );
  }

  virtual void execute()
  {
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", player -> name(), name() );

    update_ready();

    death_knight_t* p = player -> cast_death_knight();
    if ( ! sim -> overrides.horn_of_winter )
    {
      sim -> auras.horn_of_winter -> buff_duration = 120.0 + p -> glyphs.horn_of_winter -> effect1().seconds();
      sim -> auras.horn_of_winter -> trigger( 1, bonus );
    }

    player -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_horn_of_winter );
  }

  virtual double gcd() const
  {
    return player -> in_combat ? death_knight_spell_t::gcd() : 0;
  }
};

// Howling Blast ============================================================

// FIXME: -3% spell crit suppression? Seems to have the same crit chance as FS in the absence of KM
struct howling_blast_t : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", "Howling Blast", p )
  {
    check_talent( p -> talents.howling_blast -> rank() );

    parse_options( NULL, options_str );

    extract_rune_cost( this , &cost_blood, &cost_frost, &cost_unholy );
    aoe               = 1; // Change to -1 once we support meteor split effect
    direct_power_mod  = 0.4;

    assert( p -> frost_fever );
  }

  virtual void consume_resource() {}

  virtual double cost() const
  {
    // Rime also prevents getting RP because there are no runes used!
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_rime -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( ! p -> buffs_rime -> up() )
    {
      // We only consume resources when rime is not up
      // Rime procs generate no RP from rune abilites, which is handled in consume_resource as well
      death_knight_spell_t::consume_resource();
    }
    death_knight_spell_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> glyphs.howling_blast -> ok() )
        p -> frost_fever -> execute();

      if ( p -> talents.chill_of_the_grave -> rank() )
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.chill_of_the_grave -> effect1().resource( RESOURCE_RUNIC ), p -> gains_chill_of_the_grave );
    }

    p -> buffs_rime -> decrement();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_spell_t::target_debuff( t, dmg_type );

    death_knight_t* p = player -> cast_death_knight();

    if ( t -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect1().percent();
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      cost_frost  = 1;
      return rime_ready;
    }
    return death_knight_spell_t::ready();
  }
};

// Icy Touch ================================================================

struct icy_touch_t : public death_knight_spell_t
{
  icy_touch_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "icy_touch", "Icy Touch", p )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    direct_power_mod = 0.2;

    assert( p -> frost_fever );
  }

  virtual void consume_resource() {}

  virtual double cost() const
  {
    // Rime also prevents getting RP because there are no runes used!
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_rime -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( ! p -> buffs_rime -> up() )
    {
      // We only consume resources when rime is not up
      // Rime procs generate no RP from rune abilites, which is handled in consume_resource as well
      death_knight_spell_t::consume_resource();
    }

    death_knight_spell_t::execute();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_icy_touch -> execute();

    if ( result_is_hit() )
    {
      if ( p -> talents.chill_of_the_grave -> rank() )
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.chill_of_the_grave -> effect1().resource( RESOURCE_RUNIC ), p -> gains_chill_of_the_grave );

      p -> frost_fever -> execute();

      trigger_ebon_plaguebringer( this, target );
    }

    p -> buffs_rime -> decrement();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_spell_t::target_debuff( t, dmg_type );

    death_knight_t* p = player -> cast_death_knight();

    if ( t -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect1().percent();
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      cost_frost  = 1;
      return rime_ready;
    }
    return death_knight_spell_t::ready();
  }
};

// Mind Freeze ==============================================================

struct mind_freeze_t : public death_knight_spell_t
{
  mind_freeze_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "mind_freeze", "Mind Freeze", p )
  {
    parse_options( NULL, options_str );

    if ( p -> talents.endless_winter -> rank() )
      base_cost += p -> talents.endless_winter -> mod_additive( P_RESOURCE_COST ) / 10.0;

    may_miss = may_resist = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Necrotic Strike ==========================================================

struct necrotic_strike_t : public death_knight_attack_t
{
  necrotic_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "necrotic_strike", "Necrotic Strike", player )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
  }
};

// Obliterate ===============================================================

struct obliterate_offhand_t : public death_knight_attack_t
{
  flaming_torment_t* flaming_torment;

  obliterate_offhand_t( death_knight_t* p ) :
    death_knight_attack_t( "obliterate_offhand", 66198, p ), flaming_torment( NULL )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    base_multiplier *= 1.0 + p -> talents.nerves_of_cold_steel -> effect2().percent();

    // These both stack additive with MOTFW
    // http://elitistjerks.com/f72/t110296-frost_dps_cataclysm_4_0_6_my_life/p14/#post1886388
    m_dd_additive += p -> talents.annihilation -> mod_additive( P_GENERIC ) +
                     p -> glyphs.obliterate -> effect1().percent();

    if ( p -> set_bonus.tier12_4pc_melee() )
    {
      flaming_torment = new flaming_torment_t( "obliterate_offhand_flaming_torment", p );
      add_child( flaming_torment );
    }
  }

  virtual void execute()
  {
    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( flaming_torment )
      {
        flaming_torment -> base_dd_min = direct_dmg;
        flaming_torment -> execute();
      }
    }
  }

  virtual void player_buff()
  {
    death_knight_t* p = player -> cast_death_knight();

    death_knight_attack_t::player_buff();

    player_crit += p -> buffs_killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_targetdata_t* td = targetdata() -> cast_death_knight();
    death_knight_attack_t::target_debuff( t, dmg_type );

    target_multiplier *= 1 + td -> diseases() * effect3().percent() / 2.0;

    if ( t -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect1().percent();
  }
};

struct obliterate_t : public death_knight_attack_t
{
  attack_t* oh_attack;
  flaming_torment_t* flaming_torment;

  obliterate_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "obliterate", "Obliterate", p ), oh_attack( 0 ), flaming_torment( 0 )
  {
    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    if ( p -> primary_tree() == TREE_BLOOD )
      convert_runes = 1.0;

    // These both stack additive with MOTFW
    // http://elitistjerks.com/f72/t110296-frost_dps_cataclysm_4_0_6_my_life/p14/#post1886388
    m_dd_additive += p -> talents.annihilation -> mod_additive( P_GENERIC ) +
                     p -> glyphs.obliterate -> effect1().percent();

    if ( p -> set_bonus.tier12_4pc_melee() )
    {
      flaming_torment = new flaming_torment_t( "obliterate_mainhand_flaming_torment", p );
      add_child( flaming_torment );
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new obliterate_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> talents.chill_of_the_grave -> rank() )
      {
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.chill_of_the_grave -> effect1().resource( RESOURCE_RUNIC ), p -> gains_chill_of_the_grave );
      }

      // T13 2pc gives 2 stacks of Rime, otherwise we can only ever have one
      // Ensure that if we have 1 that we only refresh, not add another stack
      int new_stacks = ( p -> set_bonus.tier13_2pc_melee() && sim -> roll( p -> sets -> set( SET_T13_2PC_MELEE ) -> effect2().percent() ) ) ? 2 : 1;

      if ( sim -> roll( p -> talents.rime -> proc_chance() ) )
      {
        // If we're proccing 2 or we have 0 stacks, trigger like normal
        if ( new_stacks == 2 || p -> buffs_rime -> check() == 0 )
        {
          p -> buffs_rime -> trigger( new_stacks );
        }
        // refresh stacks. However if we have a double stack and only 1 procced, it refreshes to 1 stack
        else
        {
          p -> buffs_rime -> refresh( 0 );
          if ( p -> buffs_rime -> check() == 2 && new_stacks == 1 )
            p -> buffs_rime -> decrement( 1 );
        }

        p -> cooldowns_howling_blast -> reset();
        update_ready();
      }

      if ( flaming_torment )
      {
        flaming_torment -> base_dd_min = direct_dmg;
        flaming_torment -> execute();
      }
    }

    if ( oh_attack )
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect1().percent() ) )
        oh_attack -> execute();

    if ( p -> buffs_killing_machine -> check() )
      p -> proc_oblit_killing_machine -> occur();

    p -> buffs_killing_machine -> expire();
  }

  virtual void player_buff()
  {
    death_knight_t* p = player -> cast_death_knight();

    death_knight_attack_t::player_buff();

    player_crit += p -> buffs_killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_targetdata_t* td = targetdata() -> cast_death_knight();
    death_knight_attack_t::target_debuff( t, dmg_type );

    target_multiplier *= 1 + td -> diseases() * effect3().percent() / 2.0;

    if ( t -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect1().percent();
  }
};

// Outbreak =================================================================

struct outbreak_t : public death_knight_spell_t
{
  outbreak_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "outbreak", "Outbreak", p )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    cooldown -> duration += p -> spells.veteran_of_the_third_war -> effect3().seconds();

    assert( p -> blood_plague );
    assert( p -> frost_fever );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    if ( result_is_hit() )
    {
      p -> blood_plague -> execute();

      p -> frost_fever -> execute();
    }
  }
};

// Pestilence ===============================================================

struct pestilence_t : public death_knight_spell_t
{
  pestilence_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pestilence", "Pestilence", p )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );

    if ( p -> primary_tree() == TREE_FROST ||
         p -> primary_tree() == TREE_UNHOLY )
      convert_runes = 1.0;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_pestilence -> execute();
  }
};

// Pillar of Frost ==========================================================

struct pillar_of_frost_t : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pillar_of_frost", "Pillar of Frost", p )
  {
    check_talent( p -> talents.pillar_of_frost -> rank() );

    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    p -> buffs_pillar_of_frost -> trigger( 1, p -> talents.pillar_of_frost -> effect1().percent() );
  }

  bool ready()
  {
    if ( ! spell_t::ready() )
      return false;

    // Always activate runes, even pre-combat.
    return group_runes( player, cost_blood, cost_frost, cost_unholy, use );
  }
};

// Plague Strike ============================================================

struct plague_strike_offhand_t : public death_knight_attack_t
{
  plague_strike_offhand_t( death_knight_t* p ) :
    death_knight_attack_t( "plague_strike_offhand", 66216, p )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    base_multiplier *= 1.0 + p -> talents.nerves_of_cold_steel -> effect2().percent();

    assert( p -> blood_plague );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      p -> blood_plague -> execute();
    }
  }
};

struct plague_strike_t : public death_knight_attack_t
{
  attack_t* oh_attack;

  plague_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "plague_strike", "Plague Strike", p ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    weapon = &( p -> main_hand_weapon );
    base_multiplier *= 1.0 + p -> talents.rage_of_rivendare -> effect1().percent();

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new plague_strike_offhand_t( p );

    assert( p -> blood_plague );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::execute();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_plague_strike -> execute();

    if ( result_is_hit() )
    {
      p -> blood_plague -> execute();

      trigger_ebon_plaguebringer( this, target );
    }

    if ( oh_attack )
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect1().percent() ) )
        oh_attack -> execute();
  }
};

// Presence =================================================================

struct presence_t : public death_knight_spell_t
{
  int switch_to_presence;
  presence_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "presence", ( uint32_t ) 0, p ), switch_to_presence( 0 )
  {
    std::string presence_str;
    option_t options[] =
    {
      { "choose",  OPT_STRING, &presence_str },
      { NULL,     OPT_UNKNOWN, NULL          }
    };
    parse_options( options, options_str );

    if ( ! presence_str.empty() )
    {
      if ( presence_str == "blood" || presence_str == "bp" )
      {
        switch_to_presence = PRESENCE_BLOOD;
      }
      else if ( presence_str == "frost" || presence_str == "fp" )
      {
        switch_to_presence = PRESENCE_FROST;
      }
      else if ( presence_str == "unholy" || presence_str == "up" )
      {
        switch_to_presence = PRESENCE_UNHOLY;
      }
    }
    else
    {
      // Default to Frost Presence
      switch_to_presence = PRESENCE_FROST;
    }

    trigger_gcd = 0;
    cooldown -> duration = 1.0;
    harmful     = false;
    resource    = RESOURCE_RUNIC;
  }

  virtual double cost() const
  {
    death_knight_t* p = player -> cast_death_knight();

    // Presence changes consume all runic power
    return p -> resource_current [ RESOURCE_RUNIC ];
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();

    p -> base_gcd = 1.50;

    switch ( p -> active_presence )
    {
    case PRESENCE_BLOOD:  p -> buffs_blood_presence  -> expire(); break;
    case PRESENCE_FROST:  p -> buffs_frost_presence  -> expire(); break;
    case PRESENCE_UNHOLY: p -> buffs_unholy_presence -> expire(); break;
    }
    p -> active_presence = switch_to_presence;

    switch ( p -> active_presence )
    {
    case PRESENCE_BLOOD:
      p -> buffs_blood_presence  -> trigger();
      break;
    case PRESENCE_FROST:
    {
      double fp_value = p -> dbc.spell( 48266 ) -> effect1().percent();
      if ( p -> talents.improved_frost_presence -> rank() )
        fp_value += p -> talents.improved_frost_presence -> effect2().percent();
      p -> buffs_frost_presence -> trigger( 1, fp_value );
    }
    break;
    case PRESENCE_UNHOLY:
      p -> buffs_unholy_presence -> trigger( 1, 0.10 + p -> talents.improved_unholy_presence -> effect2().percent() );
      p -> base_gcd = 1.0;
      break;
    }

    p -> reset_gcd();

    consume_resource();
    update_ready();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(),name() );
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> active_presence == switch_to_presence )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Raise Dead ===============================================================

struct raise_dead_t : public death_knight_spell_t
{
  raise_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "raise_dead", "Raise Dead", p )
  {
    parse_options( NULL, options_str );

    if ( p -> primary_tree() == TREE_UNHOLY )
      cooldown -> duration += p -> spells.master_of_ghouls -> effect1().seconds();

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    p -> active_ghoul -> summon( ( p -> primary_tree() == TREE_UNHOLY ) ? 0.0 : p -> dbc.spell( effect1().base_value() ) -> duration() );
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> active_ghoul && ! p -> active_ghoul -> sleeping )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Rune Strike ==============================================================

struct rune_strike_offhand_t : public death_knight_attack_t
{
  rune_strike_offhand_t( death_knight_t* p ) :
    death_knight_attack_t( "rune_strike_offhand", 66217, p )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );

    base_crit       += p -> glyphs.rune_strike -> effect1().percent();
    direct_power_mod = 0.15;
    may_dodge = may_block = may_parry = false;
  }

};

struct rune_strike_t : public death_knight_attack_t
{
  attack_t* oh_attack;

  rune_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "rune_strike", "Rune Strike", p ),
    oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    base_crit       += p -> glyphs.rune_strike -> effect1().percent();
    direct_power_mod = 0.15;
    may_dodge = may_block = may_parry = false;


    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new rune_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_attack_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    if ( result_is_hit() )
      p -> trigger_runic_empowerment();

    if ( oh_attack )
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect1().percent() ) )
        oh_attack -> execute();
  }
};

// Scourge Strike ===========================================================

struct scourge_strike_t : public death_knight_attack_t
{
  spell_t* scourge_strike_shadow;
  flaming_torment_t* flaming_torment;

  struct scourge_strike_shadow_t : public death_knight_spell_t
  {
    flaming_torment_t* flaming_torment;

    scourge_strike_shadow_t( death_knight_t* p ) :
      death_knight_spell_t( "scourge_strike_shadow", 70890, p ), flaming_torment( 0 )
    {
      check_spec( TREE_UNHOLY );

      weapon = &( player -> main_hand_weapon );
      may_miss = may_parry = may_dodge = false;
      may_crit          = false;
      proc              = true;
      background        = true;
      weapon_multiplier = 0;
      base_multiplier  *= 1.0 + p -> glyphs.scourge_strike -> effect1().percent();

      if ( p -> set_bonus.tier12_4pc_melee() )
      {
        flaming_torment = new flaming_torment_t( "scourge_strike_shadow_flaming_torment", p );
        add_child( flaming_torment );
      }
    }

    virtual void target_debuff( player_t* t, int /* dmg_type */ )
    {
      // Shadow portion doesn't double dips in debuffs, other than EP/E&M/CoE below
      // death_knight_spell_t::target_debuff( t, dmg_type );

      death_knight_targetdata_t* td = targetdata() -> cast_death_knight();

      target_multiplier = td -> diseases() * 0.18;

      if ( t -> debuffs.earth_and_moon -> up() || t -> debuffs.ebon_plaguebringer -> up() || t -> debuffs.curse_of_elements -> up() )
        target_multiplier *= 1.08;
    }

    void execute()
    {
      death_knight_spell_t::execute();

      if ( result_is_hit() )
      {
        if ( flaming_torment )
        {
          flaming_torment -> base_dd_min = direct_dmg;
          flaming_torment -> execute();
        }
      }
    }
  };

  scourge_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_attack_t( "scourge_strike", "Scourge Strike", p ),
    scourge_strike_shadow( 0 ), flaming_torment( 0 )
  {
    parse_options( NULL, options_str );

    scourge_strike_shadow = new scourge_strike_shadow_t( p );
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );

    base_multiplier *= 1.0 + p -> talents.rage_of_rivendare -> mod_additive( P_GENERIC );

    if ( p -> set_bonus.tier12_4pc_melee() )
    {
      flaming_torment = new flaming_torment_t( "scourge_strike_flaming_torment", p );
      add_child( flaming_torment );
    }
  }

  void execute()
  {
    death_knight_attack_t::execute();
    if ( result_is_hit() )
    {
      if ( flaming_torment )
      {
        flaming_torment -> base_dd_min = direct_dmg;
        flaming_torment -> execute();
      }
      // We divide out our composite_player_multiplier here because we
      // don't want to double dip; in particular, 3% damage from ret
      // paladins, arcane mages, and beastmaster hunters do not affect
      // scourge_strike_shadow.
      double modified_dd = direct_dmg / player -> player_t::composite_player_multiplier( SCHOOL_SHADOW, this );
      scourge_strike_shadow -> base_dd_max = scourge_strike_shadow -> base_dd_min = modified_dd;
      scourge_strike_shadow -> execute();
    }
  }
};

// Summon Gargoyle ==========================================================

struct summon_gargoyle_t : public death_knight_spell_t
{
  summon_gargoyle_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "summon_gargoyle", "Summon Gargoyle", p )
  {
    check_talent( p -> talents.summon_gargoyle -> rank() );
    rp_gain = 0.0;  // For some reason, the inc file thinks we gain RP for this spell
    parse_options( NULL, options_str );

    harmful = false;
    num_ticks = 0;
    base_tick_time = 0.0;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    p -> active_gargoyle -> summon( p -> dbc.spell( effect3().trigger_spell_id() ) -> duration() );
  }
};

// Unholy Blight ====================================================

struct unholy_blight_t : public death_knight_spell_t
{
  unholy_blight_t( death_knight_t* p ) :
    death_knight_spell_t( "unholy_blight", p -> talents.unholy_blight -> spell_id(), p )
  {
    base_tick_time = 1.0;
    num_ticks      = 10;
    background     = true;
    proc           = true;
    may_crit       = false;
    may_resist     = false;
    may_miss       = false;
    hasted_ticks   = false;
  }

  void target_debuff( player_t* /* t */, int /* dmg_type */ )
  {
    // no debuff effect
  }

  void player_buff()
  {
    // no buffs
  }
};

// Unholy Frenzy ============================================================

struct unholy_frenzy_t : public spell_t
{
  unholy_frenzy_t( death_knight_t* p, const std::string& options_str ) :
    spell_t( "unholy_frenzy", "Unholy Frenzy", p )
  {
    check_talent( p -> talents.unholy_frenzy -> rank() );

    std::string target_str = p -> unholy_frenzy_target_str;
    parse_options( NULL, options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target -> is_enemy() || target -> is_add() )
    {
      target = p;
    }

    harmful = false;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    spell_t::execute();
    target -> buffs.unholy_frenzy -> trigger( 1 );
  }
};

struct butchery_event_t : public event_t
{
  butchery_event_t( player_t* player, double tick_time ) :
    event_t( player -> sim, player, "butchery_regen" )
  {
    if ( tick_time < 0 ) tick_time = 0;
    if ( tick_time > 5 ) tick_time = 5;
    sim -> add_event( this, tick_time );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();

    // p -> talents.butchery -> effect2().base_value() / 10.0 == p -> talents.butchery -> rank()
    // Just work with hardcoded numbers here. With rank 1 you gain 1 RP every 5s
    // with rank 2 you gain 1 RP every 2.5s
    p -> resource_gain( RESOURCE_RUNIC, 1, p -> gains_butchery );

    new ( sim ) butchery_event_t( player, 5.0 / p -> talents.butchery -> rank()  );
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  // General Actions
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "presence"                 ) return new presence_t                 ( this, options_str );

  // Blood Actions
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "blood_strike"             ) return new blood_strike_t             ( this, options_str );
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "pestilence"               ) return new pestilence_t               ( this, options_str );

  // Frost Actions
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
  if ( name == "icy_touch"                ) return new icy_touch_t                ( this, options_str );
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
  if ( name == "pillar_of_frost"          ) return new pillar_of_frost_t          ( this, options_str );
  if ( name == "rune_strike"              ) return new rune_strike_t              ( this, options_str );

  // Unholy Actions
  if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
  if ( name == "bone_shield"              ) return new bone_shield_t              ( this, options_str );
  if ( name == "dark_transformation"      ) return new dark_transformation_t      ( this, options_str );
  if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
  if ( name == "festering_strike"         ) return new festering_strike_t         ( this, options_str );
  if ( name == "outbreak"                 ) return new outbreak_t                 ( this, options_str );
  if ( name == "necrotic_strike"          ) return new necrotic_strike_t          ( this, options_str );
  if ( name == "plague_strike"            ) return new plague_strike_t            ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );
  if ( name == "unholy_frenzy"            ) return new unholy_frenzy_t            ( this, options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

action_expr_t* death_knight_t::create_expression( action_t* a, const std::string& name_str )
{
  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, name_str, "." );

  if ( util_t::str_compare_ci( splits[ 0 ], "rune" ) )
  {
    rune_type rt = RUNE_TYPE_NONE;
    bool include_death = true; // whether to include death runes
    switch ( splits[ 1 ][0] )
    {
    case 'B': include_death = false;
    case 'b': rt = RUNE_TYPE_BLOOD; break;
    case 'U': include_death = false;
    case 'u': rt = RUNE_TYPE_UNHOLY; break;
    case 'F': include_death = false;
    case 'f': rt = RUNE_TYPE_FROST; break;
    case 'D': include_death = false;
    case 'd': rt = RUNE_TYPE_DEATH; break;
    }
    int position = 0; // any
    switch( splits[1][splits[1].size()-1] )
    {
    case '1': position = 1; break;
    case '2': position = 2; break;
    }

    int act = 0;
    if ( num_splits == 3 && util_t::str_compare_ci( splits[ 2 ], "cooldown_remains" ) )
      act = 1;
    else if ( num_splits == 3 && util_t::str_compare_ci( splits[ 2 ], "cooldown_remains_all" ) )
      act = 2;
    else if ( num_splits == 3 && util_t::str_compare_ci( splits[ 2 ], "depleted" ) )
      act = 3;

    struct rune_inspection_expr_t : public action_expr_t
    {
      rune_type r;
      bool include_death;
      int position;
      int myaction; // -1 count, 0 cooldown remains, 1 cooldown_remains_all

      rune_inspection_expr_t( action_t* a, rune_type r, bool include_death, int position, int myaction )
        : action_expr_t( a, "rune_evaluation", TOK_NUM ), r( r ),
          include_death( include_death ), position( position ), myaction( myaction )
      { }

      virtual int evaluate()
      {
        death_knight_t* dk = action -> player -> cast_death_knight();
        switch( myaction )
        {
        case 0: result_num = dk -> runes_count( r, include_death, position ); break;
        case 1: result_num = dk -> runes_cooldown_any( r, include_death, position ); break;
        case 2: result_num = dk -> runes_cooldown_all( r, include_death, position ); break;
        case 3: result_num = dk -> runes_depleted( r, position ); break;
        }
        return TOK_NUM;
      }
    };
    return new rune_inspection_expr_t( a, rt, include_death, position, act );
  }
  else if ( num_splits == 2 )
  {
    rune_type rt = RUNE_TYPE_NONE;
    if ( util_t::str_compare_ci( splits[ 0 ], "blood" ) || util_t::str_compare_ci( splits[ 0 ], "b" ) )
      rt = RUNE_TYPE_BLOOD;
    else if ( util_t::str_compare_ci( splits[ 0 ], "frost" ) || util_t::str_compare_ci( splits[ 0 ], "f" ) )
      rt = RUNE_TYPE_FROST;
    else if ( util_t::str_compare_ci( splits[ 0 ], "unholy" ) || util_t::str_compare_ci( splits[ 0 ], "u" ) )
      rt = RUNE_TYPE_UNHOLY;
    else if ( util_t::str_compare_ci( splits[ 0 ], "death" ) || util_t::str_compare_ci( splits[ 0 ], "d" ) )
      rt = RUNE_TYPE_DEATH;

    if ( rt != RUNE_TYPE_NONE && util_t::str_compare_ci( splits[ 1 ], "cooldown_remains" ) )
    {
      struct rune_cooldown_expr_t : public action_expr_t
      {
        rune_type r;

        rune_cooldown_expr_t( action_t* a, rune_type r ) : action_expr_t( a, "rune_cooldown_remains", TOK_NUM ), r( r ) {}
        virtual int evaluate()
        {
          death_knight_t* dk = action -> player -> cast_death_knight();
          result_num = dk -> runes_cooldown_any( r, true, 0 );
          return TOK_NUM;
        }
      };

      return new rune_cooldown_expr_t( a, rt );
    }
  }
  else
  {
    rune_type rt = RUNE_TYPE_NONE;
    if ( util_t::str_compare_ci( splits[ 0 ], "blood" ) || util_t::str_compare_ci( splits[ 0 ], "b" ) )
      rt = RUNE_TYPE_BLOOD;
    else if ( util_t::str_compare_ci( splits[ 0 ], "frost" ) || util_t::str_compare_ci( splits[ 0 ], "f" ) )
      rt = RUNE_TYPE_FROST;
    else if ( util_t::str_compare_ci( splits[ 0 ], "unholy" ) || util_t::str_compare_ci( splits[ 0 ], "u" ) )
      rt = RUNE_TYPE_UNHOLY;
    else if ( util_t::str_compare_ci( splits[ 0 ], "death" ) || util_t::str_compare_ci( splits[ 0 ], "d" ) )
      rt = RUNE_TYPE_DEATH;

    struct rune_expr_t : public action_expr_t
    {
      rune_type r;
      rune_expr_t( action_t* a, rune_type r ) : action_expr_t( a, "rune", TOK_NUM ), r( r ) { }
      virtual int evaluate()
      {
        result_num = action -> player -> cast_death_knight() -> runes_count( r, false, 0 ); return TOK_NUM;
      }
    };
    if ( rt ) return new rune_expr_t( a, rt );

    if ( name_str == "inactive_death" )
    {
      struct death_expr_t : public action_expr_t
      {
        std::string name;
        death_expr_t( action_t* a, const std::string name_in ) : action_expr_t( a, name_in ), name( name_in ) { result_type = TOK_NUM; }
        virtual int evaluate()
        {
          result_num = count_death_runes( action -> player -> cast_death_knight(), name == "inactive_death" ); return TOK_NUM;
        }
      };
      return new death_expr_t( a, name_str );
    }
  }

  return player_t::create_expression( a, name_str );
}

// death_knight_t::create_pets ==============================================

void death_knight_t::create_pets()
{
  active_army_ghoul           = create_pet( "army_of_the_dead" );
  active_bloodworms           = create_pet( "bloodworms" );
  active_dancing_rune_weapon  = new dancing_rune_weapon_pet_t ( sim, this );
  active_gargoyle             = create_pet( "gargoyle" );
  active_ghoul                = create_pet( "ghoul" );
}

// death_knight_t::create_pet ===============================================

pet_t* death_knight_t::create_pet( const std::string& pet_name,
                                   const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "army_of_the_dead"         ) return new army_ghoul_pet_t          ( sim, this );
  if ( pet_name == "bloodworms"               ) return new bloodworms_pet_t          ( sim, this );
  if ( pet_name == "gargoyle"                 ) return new gargoyle_pet_t            ( sim, this );
  if ( pet_name == "ghoul"                    ) return new ghoul_pet_t               ( sim, this );

  return 0;
}

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_attack_haste() const
{
  double haste = player_t::composite_attack_haste();

  haste *= 1.0 / ( 1.0 + buffs_unholy_presence -> value() );

  return haste;
}

// death_knight_t::composite_attack_hit() ===================================

double death_knight_t::composite_attack_hit() const
{
  double hit = player_t::composite_attack_hit();

  // Factor in the hit from NoCS here so it shows up in the report, to match the character sheet
  if ( main_hand_weapon.group() == WEAPON_1H || off_hand_weapon.group() == WEAPON_1H )
  {
    if ( talents.nerves_of_cold_steel -> rank() )
    {
      hit += talents.nerves_of_cold_steel -> effect1().percent();
    }
  }

  return hit;
}
// death_knight_t::init =====================================================

void death_knight_t::init()
{
  player_t::init();

  if ( active_ghoul )
  {
    if ( primary_tree() == TREE_UNHOLY )
    {
      active_ghoul -> type = PLAYER_PET;
    }
    else
    {
      active_ghoul -> type = PLAYER_GUARDIAN;
    }
  }

  if ( ( primary_tree() == TREE_FROST ) )
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      if ( _runes.slot[i].type == RUNE_TYPE_BLOOD )
      {
        _runes.slot[i].make_permanent_death_rune();
      }
    }
  }
}

// death_knight_t::init_rng =================================================

void death_knight_t::init_rng()
{
  player_t::init_rng();

  rng_blood_caked_blade          = get_rng( "blood_caked_blade"          );
  rng_might_of_the_frozen_wastes = get_rng( "might_of_the_frozen_wastes" );
  rng_threat_of_thassarian       = get_rng( "threat_of_thassarian"       );
  rng_butchery_start             = get_rng( "butchery_start"             );
}

// death_knight_t::init_defense =============================================

void death_knight_t::init_defense()
{
  player_t::init_defense();

  initial_parry_rating_per_strength = 0.27;
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base()
{
  player_t::init_base();

  double str_mult = ( talents.abominations_might -> effect2().percent() +
                      talents.brittle_bones      -> effect2().percent() );

  if ( primary_tree() == TREE_UNHOLY )
    str_mult += spells.unholy_might -> effect1().percent();

  attribute_multiplier_initial[ ATTR_STRENGTH ] *= 1.0 + str_mult;

  if ( primary_tree() == TREE_BLOOD )
  {
    attribute_multiplier_initial[ ATTR_STAMINA ]  *= 1.0 + spells.veteran_of_the_third_war -> effect1().percent();
    base_attack_expertise = spells.veteran_of_the_third_war -> effect2().percent();
  }

  if ( talents.toughness -> rank() )
    initial_armor_multiplier *= 1.0 + talents.toughness -> effect1().percent();

  base_attack_power = level * ( level > 80 ? 3.0 : 2.0 );

  initial_attack_power_per_strength = 2.0;

  if ( primary_tree() == TREE_BLOOD )
    vengeance_enabled = true;

  resource_base[ RESOURCE_RUNIC ] = 100;

  if ( talents.runic_power_mastery -> rank() )
    resource_base[ RESOURCE_RUNIC ] += talents.runic_power_mastery -> effect1().resource( RESOURCE_RUNIC );

  base_gcd = 1.5;

  diminished_kfactor    = 0.009560;
  diminished_dodge_capi = 0.01523660;
  diminished_parry_capi = 0.01523660;
}

// death_knight_t::init_talents =============================================

void death_knight_t::init_talents()
{
  // Blood
  talents.abominations_might          = find_talent( "Abomination's Might" );
  talents.bladed_armor                = find_talent( "Bladed Armor" );
  talents.blood_caked_blade           = find_talent( "Blood-Caked Blade" );
  talents.blood_parasite              = find_talent( "Blood Parasite" );
  talents.bone_shield                 = find_talent( "Bone Shield" );
  talents.toughness                   = find_talent( "Toughness" );
  talents.butchery                    = find_talent( "Butchery" );
  talents.crimson_scourge             = find_talent( "Crimson Scourge" );
  talents.dancing_rune_weapon         = find_talent( "Dancing Rune Weapon" );
  talents.improved_blood_presence     = find_talent( "Improved Blood Presence" );
  talents.improved_blood_tap          = find_talent( "Improved Blood Tap" );
  talents.improved_death_strike       = find_talent( "Improved Death Strike" );
  talents.scent_of_blood              = find_talent( "Scent of Blood" );

  // Frost
  talents.annihilation                = find_talent( "Annihilation" );
  talents.brittle_bones               = find_talent( "Brittle Bones" );
  talents.chill_of_the_grave          = find_talent( "Chill of the Grave" );
  talents.endless_winter              = find_talent( "Endless Winter" );
  talents.howling_blast               = find_talent( "Howling Blast" );
  talents.improved_frost_presence     = find_talent( "Improved Frost Presence" );
  talents.improved_icy_talons         = find_talent( "Improved Icy Talons" );
  talents.killing_machine             = find_talent( "Killing Machine" );
  talents.merciless_combat            = find_talent( "Merciless Combat" );
  talents.might_of_the_frozen_wastes  = find_talent( "Might of the Frozen Wastes" );
  talents.nerves_of_cold_steel        = find_talent( "Nerves of Cold Steel" );
  talents.pillar_of_frost             = find_talent( "Pillar of Frost" );
  talents.rime                        = find_talent( "Rime" );
  talents.runic_power_mastery         = find_talent( "Runic Power Mastery" );
  talents.threat_of_thassarian        = find_talent( "Threat of Thassarian" );

  // Unholy
  talents.dark_transformation         = find_talent( "Dark Transformation" );
  talents.ebon_plaguebringer          = find_talent( "Ebon Plaguebringer" );
  talents.epidemic                    = find_talent( "Epidemic" );
  talents.improved_unholy_presence    = find_talent( "Improved Unholy Presence" );
  talents.morbidity                   = find_talent( "Morbidity" );
  talents.rage_of_rivendare           = find_talent( "Rage of Rivendare" );
  talents.runic_corruption            = find_talent( "Runic Corruption" );
  talents.shadow_infusion             = find_talent( "Shadow Infusion" );
  talents.sudden_doom                 = find_talent( "Sudden Doom" );
  talents.summon_gargoyle             = find_talent( "Summon Gargoyle" );
  talents.unholy_blight               = find_talent( "Unholy Blight" );
  talents.unholy_frenzy               = find_talent( "Unholy Frenzy" );
  talents.virulence                   = find_talent( "Virulence" );

  player_t::init_talents();
}

// death_knight_t::init_spells ==============================================

void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Blood
  spells.blood_rites              = spell_data_t::find( 50034, "Blood Rites",              dbc.ptr );
  spells.veteran_of_the_third_war = spell_data_t::find( 50029, "Veteran of the Third War", dbc.ptr );

  // Frost
  spells.blood_of_the_north = spell_data_t::find( 54637, "Blood of the North", dbc.ptr );
  spells.icy_talons         = spell_data_t::find( 50887, "Icy Talons",         dbc.ptr );
  spells.frozen_heart       = spell_data_t::find( 77514, "Frozen Heart",       dbc.ptr );

  // Unholy
  spells.dreadblade       = spell_data_t::find( 77515, "Dreadblade",       dbc.ptr );
  spells.master_of_ghouls = spell_data_t::find( 52143, "Master of Ghouls", dbc.ptr );
  spells.reaping          = spell_data_t::find( 56835, "Reaping",          dbc.ptr );
  spells.unholy_might     = spell_data_t::find( 91107, "Unholy Might",     dbc.ptr );

  // General
  spells.plate_specialization = spell_data_t::find( 86524, "Plate Specialization", dbc.ptr );
  spells.runic_empowerment    = spell_data_t::find( 81229, "Runic Empowerment",    dbc.ptr );

  // Glyphs
  glyphs.death_and_decay = find_glyph( "Glyph of Death and Decay" );
  glyphs.death_coil      = find_glyph( "Glyph of Death Coil" );
  glyphs.death_strike    = find_glyph( "Glyph of Death Strike" );
  glyphs.frost_strike    = find_glyph( "Glyph of Frost Strike" );
  glyphs.heart_strike    = find_glyph( "Glyph of Heart Strike" );
  glyphs.horn_of_winter  = find_glyph( "Glyph of Horn of Winter" );
  glyphs.howling_blast   = find_glyph( "Glyph of Howling Blast" );
  glyphs.icy_touch       = find_glyph( "Glyph of Icy Touch" );
  glyphs.obliterate      = find_glyph( "Glyph of Obliterate" );
  glyphs.raise_dead      = find_glyph( "Glyph of Raise Dead" );
  glyphs.rune_strike     = find_glyph( "Glyph of Rune Strike" );
  glyphs.scourge_strike  = find_glyph( "Glyph of Scourge Strike" );

  // Active Spells
  blood_plague = new blood_plague_t( this );
  frost_fever = new frost_fever_t( this );
  if ( talents.unholy_blight -> rank() )
    active_unholy_blight = new unholy_blight_t( this );

  // Tier Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P    H2P    H4P
    {     0,     0,  90457,  90459,  90454,  90456,     0,     0 }, // Tier11
    {     0,     0,  98970,  98996,  98956,  98966,     0,     0 }, // Tier12
    {     0,     0, 105609, 105646, 105552, 105587,     0,     0 }, // Tier13
    {     0,     0,      0,      0,      0,      0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// death_knight_t::init_actions =============================================

void death_knight_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    int tree = primary_tree();

    if ( tree == TREE_FROST || tree == TREE_UNHOLY || ( tree == TREE_BLOOD && primary_role() != ROLE_TANK ) )
    {
      // Flask
      if ( level >= 80 )
        action_list_str += "/flask,type=titanic_strength";
      else if ( level >= 75 )
        action_list_str += "/flask,type=endless_rage";

      // Food
      if ( level >= 80 )
        action_list_str += "/food,type=beer_basted_crocolisk";
      else if ( level >= 70 )
        action_list_str += "/food,type=dragonfin_filet";

      // Stance
      action_list_str += "/presence,choose=unholy";
    }
    else if ( tree == TREE_BLOOD && primary_role() == ROLE_TANK )
    {
      // Flask
      if ( level >= 80 )
        action_list_str += "/flask,type=steelskin";
      else if ( level >= 75 )
        action_list_str += "/flask,type=stoneblood";

      // Food
      if ( level >= 80 )
        action_list_str += "/food,type=beer_basted_crocolisk";
      else if ( level >= 70 )
        action_list_str += "/food,type=dragonfin_filet";

      // Stance
      action_list_str += "/presence,choose=blood";
    }

    action_list_str += "/army_of_the_dead";

    action_list_str += "/snapshot_stats";

    switch ( tree )
    {
    case TREE_BLOOD:
      action_list_str += init_use_item_actions( ",time>=10" );
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions( ",time>=10" );
      action_list_str += "/golemblood_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      if ( talents.bone_shield -> rank() )
        action_list_str += "/bone_shield,if=!buff.bone_shield.up";
      action_list_str += "/raise_dead,time>=10";
      action_list_str += "/icy_touch,if=dot.frost_fever.remains<=2";
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<=2";
      action_list_str += "/heart_strike";
      action_list_str += "/death_strike";
      if ( talents.dancing_rune_weapon -> rank() )
      {
        action_list_str += "/dancing_rune_weapon,time<=150,if=dot.frost_fever.remains<=5|dot.blood_plague.remains<=5";
        action_list_str += "/dancing_rune_weapon,if=(dot.frost_fever.remains<=5|dot.blood_plague.remains<=5)&buff.bloodlust.react";
      }
      action_list_str += "/empower_rune_weapon,if=blood=0&unholy=0&frost=0";
      action_list_str += "/death_coil";
      break;
    case TREE_FROST:
    {
      action_list_str += init_use_item_actions( ",time>=10" );
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions( ",time>=10" );
      action_list_str += "/golemblood_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      if ( talents.pillar_of_frost -> rank() )
      {
        action_list_str += "/pillar_of_frost";
      }
      action_list_str += "/blood_tap,if=death.cooldown_remains>2.0";
      // this results in a dps loss. which is odd, it probalby shouldn't. although it only ever affects the very first ghoul summon
      // leaving it here until further testing.
      if ( false )
      {
        // Try and time a better ghoul
        bool has_heart_of_rage = false;
        for ( int i=0, n=items.size(); i < n; i++ )
        {
          // check for Heart of Rage
          if ( strstr( items[ i ].name(), "heart_of_rage" ) )
          {
            has_heart_of_rage = true;
            break;
          }
        }
        action_list_str += "/raise_dead,if=buff.rune_of_the_fallen_crusader.react";
        if ( has_heart_of_rage )
          action_list_str += "&buff.heart_of_rage.react";
      }

      action_list_str += "/raise_dead,time>=15";
      // priority:
      // Diseases
      // Obliterate if 2 rune pair are capped, or there is no candidate for RE
      // FS if RP > 110 - avoid RP capping (value varies. going with 110)
      // Rime
      // OBL if any pair are capped
      // FS to avoid RP capping (maxRP - OBL rp generation + GCD generation. Lets say 100)
      // OBL
      // FS
      // HB (it turns out when resource starved using a lonely death/frost rune to generate RP/FS/RE is better than waiting for OBL

      // optimal timing for diseases depends on points in epidemic, and if using improved blood tap
      // players with only 2 points in epidemic want a 0 second refresh to avoid two PS in one minute instead of 1
      // IBT players use 2 PS every minute
      std::string drefresh = "0";
      if ( talents.improved_blood_tap -> rank() )
        drefresh = "2";
      if ( talents.epidemic -> rank() == 3 )
        drefresh = "1";
      if ( talents.epidemic -> rank() == 2 )
        drefresh = "0";
      if ( level > 81 )
        action_list_str += "/outbreak,if=dot.frost_fever.remains<=" + drefresh + "|dot.blood_plague.remains<=" + drefresh;
      action_list_str += "/howling_blast,if=dot.frost_fever.remains<=" + drefresh;
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<=" + drefresh;
      action_list_str += "/obliterate,if=death>=1&frost>=1&unholy>=1";
      action_list_str += "/obliterate,if=(death=2&frost=2)|(death=2&unholy=2)|(frost=2&unholy=2)";
      // XXX TODO 110 is based on MAXRP - FSCost + a little, as a break point. should be varialble based on RPM GoFS etc
      action_list_str += "/frost_strike,if=runic_power>=110";
      if ( talents.howling_blast -> rank() && talents.rime -> rank() )
        action_list_str += "/howling_blast,if=buff.rime.react";
      action_list_str += "/obliterate,if=(death=2|unholy=2|frost=2)";
      action_list_str += "/frost_strike,if=runic_power>=100";
      action_list_str += "/obliterate";
      action_list_str += "/empower_rune_weapon,if=target.time_to_die<=45";
      action_list_str +="/frost_strike";
      if ( talents.howling_blast -> rank() )
        action_list_str += "/howling_blast";
      // avoid using ERW if runes are almost ready
      action_list_str += "/empower_rune_weapon,if=(blood.cooldown_remains+frost.cooldown_remains+unholy.cooldown_remains)>8";
      action_list_str += "/horn_of_winter";
      // add in goblin rocket barrage when nothing better to do. 40dps or so.
      if ( race == RACE_GOBLIN )
        action_list_str += "/rocket_barrage";
      break;
    }
    case TREE_UNHOLY:
      action_list_str += init_use_item_actions( ",time>=2" );
      action_list_str += init_use_profession_actions();
      if ( talents.unholy_frenzy -> rank() && race == RACE_TROLL )
      {
        action_list_str += init_use_racial_actions( ",sync=unholy_frenzy" );
      }
      else
      {
        action_list_str += init_use_racial_actions( ",time>=2" );
      }
      action_list_str += "/raise_dead";
      action_list_str += "/golemblood_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      if ( talents.unholy_frenzy -> rank() )
        action_list_str += "/unholy_frenzy,if=!buff.bloodlust.react|target.time_to_die<=45";
      if ( level > 81 )
        action_list_str += "/outbreak,if=dot.frost_fever.remains<=2|dot.blood_plague.remains<=2";
      action_list_str += "/icy_touch,if=dot.frost_fever.remains<2&cooldown.outbreak.remains>2";
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<2&cooldown.outbreak.remains>2";
      if ( talents.dark_transformation -> rank() )
        action_list_str += "/dark_transformation";
      if ( talents.summon_gargoyle -> rank() )
      {
        action_list_str += "/summon_gargoyle,time<=60";
        action_list_str += "/summon_gargoyle,if=buff.bloodlust.react|buff.unholy_frenzy.react";
      }
      action_list_str += "/death_and_decay,if=unholy=2&runic_power<110";
      action_list_str += "/scourge_strike,if=unholy=2&runic_power<110";
      action_list_str += "/festering_strike,if=blood=2&frost=2&runic_power<110";
      action_list_str += "/death_coil,if=runic_power>90";
      if ( talents.sudden_doom -> rank() )
        action_list_str += "/death_coil,if=buff.sudden_doom.react";
      action_list_str += "/death_and_decay";
      action_list_str += "/scourge_strike";
      action_list_str += "/festering_strike";
      action_list_str += "/death_coil";
      action_list_str += "/blood_tap";
      action_list_str += "/empower_rune_weapon";
      action_list_str += "/horn_of_winter";
      break;
    default: break;
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// death_knight_t::init_enchant =============================================

void death_knight_t::init_enchant()
{
  player_t::init_enchant();

  std::string& mh_enchant = items[ SLOT_MAIN_HAND ].encoded_enchant_str;
  std::string& oh_enchant = items[ SLOT_OFF_HAND  ].encoded_enchant_str;

  // Rune of Cinderglacier ==================================================
  struct cinderglacier_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;

    cinderglacier_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot( s ), buff( b ) {}

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      weapon_t* w = a -> weapon;
      if ( ! w || w -> slot != slot ) return;

      // FIX ME: What is the proc rate? For now assuming the same as FC
      buff -> trigger( 2, 0.2, w -> proc_chance_on_swing( 2.0 ) );

      // FIX ME: This should roll the benefit when casting DND, it does not
    }
  };

  // Rune of the Fallen Crusader ============================================
  struct fallen_crusader_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;

    fallen_crusader_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot( s ), buff( b ) {}

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;

      // RotFC is 2 PPM.
      buff -> trigger( 1, 0.15, w -> proc_chance_on_swing( 2.0 ) );
    }
  };

  // Rune of the Razorice ===================================================

  // Damage Proc
  struct razorice_spell_t : public death_knight_spell_t
  {
    razorice_spell_t( death_knight_t* player ) : death_knight_spell_t( "razorice", 50401, player )
    {
      may_miss    = false;
      may_crit    = false;
      may_resist  = true;
      background  = true;
      proc        = true;
    }

    void target_debuff( player_t* t, int dmg_type )
    {
      death_knight_spell_t::target_debuff( t, dmg_type );
      death_knight_t* p = player -> cast_death_knight();

      target_multiplier /= 1.0 + p -> buffs_rune_of_razorice -> check() * p -> buffs_rune_of_razorice -> effect1().percent();
    }
  };

  struct razorice_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;
    spell_t* razorice_damage_proc;

    razorice_callback_t( death_knight_t* p, int s, buff_t* b ) :
      action_callback_t( p -> sim, p ), slot( s ), buff( b ), razorice_damage_proc( 0 )
    {
      razorice_damage_proc = new razorice_spell_t( p );
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;

      // http://elitistjerks.com/f72/t64830-dw_builds_3_2_revenge_offhand/p28/#post1332820
      // double PPM        = 2.0;
      // double swing_time = a -> time_to_execute;
      // double chance     = w -> proc_chance_on_swing( PPM, swing_time );
      buff -> trigger();

      razorice_damage_proc -> execute();
    }
  };

  buffs_rune_of_cinderglacier       = new buff_t( this, "rune_of_cinderglacier",       2, 30.0 );
  buffs_rune_of_razorice            = new buff_t( this, 51714, "rune_of_razorice" );
  buffs_rune_of_the_fallen_crusader = new buff_t( this, "rune_of_the_fallen_crusader", 1, 15.0 );

  if ( mh_enchant == "rune_of_the_fallen_crusader" )
  {
    register_attack_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_the_fallen_crusader ) );
  }
  else if ( mh_enchant == "rune_of_razorice" )
  {
    register_attack_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_razorice ) );
  }
  else if ( mh_enchant == "rune_of_cinderglacier" )
  {
    register_attack_callback( RESULT_HIT_MASK, new cinderglacier_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_cinderglacier ) );
  }

  if ( oh_enchant == "rune_of_the_fallen_crusader" )
  {
    register_attack_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_the_fallen_crusader ) );
  }
  else if ( oh_enchant == "rune_of_razorice" )
  {
    register_attack_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_razorice ) );
  }
  else if ( oh_enchant == "rune_of_cinderglacier" )
  {
    register_attack_callback( RESULT_HIT_MASK, new cinderglacier_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_cinderglacier ) );
  }
}

// death_knight_t::init_scaling =============================================

void death_knight_t::init_scaling()
{
  player_t::init_scaling();

  if ( talents.bladed_armor -> rank() > 0 )
    scales_with[ STAT_ARMOR ] = 1;

  if ( off_hand_weapon.type != WEAPON_NONE )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = 1;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = sim -> weapon_speed_scale_factors;
    scales_with[ STAT_HIT_RATING2          ] = 1;
  }

  if ( primary_role() == ROLE_TANK )
    scales_with[ STAT_PARRY_RATING ] = 1;
}

// death_knight_t::init_buffs ===============================================

void death_knight_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )

  buffs_blood_presence      = new buff_t( this, "blood_presence", "Blood Presence" );
  buffs_bone_shield         = new buff_t( this, "bone_shield", "Bone Shield" );
  buffs_crimson_scourge     = new buff_t( this, 81141, "crimson_scourge", talents.crimson_scourge -> proc_chance() );
  buffs_dancing_rune_weapon = new buff_t( this, "dancing_rune_weapon", "Dancing Rune Weapon", -1, -1.0, true ); // quiet=true
  buffs_dark_transformation = new buff_t( this, "dark_transformation", "Dark Transformation" );
  buffs_frost_presence      = new buff_t( this, "frost_presence", "Frost Presence" );
  buffs_killing_machine     = new buff_t( this, 51124, "killing_machine" ); // PPM based!
  buffs_pillar_of_frost     = new buff_t( this, "pillar_of_frost", "Pillar of Frost" );
  buffs_rime                = new buff_t( this, "rime", ( set_bonus.tier13_2pc_melee() ) ? 2 : 1, 30.0, 0.0, 1.0 ); // Trigger controls proc chance
  buffs_runic_corruption    = new buff_t( this, 51460, "runic_corruption" );
  buffs_scent_of_blood      = new buff_t( this, "scent_of_blood",      talents.scent_of_blood -> rank(),  20.0,  0.0, talents.scent_of_blood -> proc_chance() );
  buffs_shadow_infusion     = new buff_t( this, 91342, "shadow_infusion", talents.shadow_infusion -> proc_chance() );
  buffs_sudden_doom         = new buff_t( this, "sudden_doom", ( set_bonus.tier13_2pc_melee() ) ? 2 : 1, 10.0, 0.0, 1.0 );
  buffs_tier11_4pc_melee    = new buff_t( this, 90507, "tier11_4pc_melee", set_bonus.tier11_4pc_melee() );
  buffs_tier13_4pc_melee    = new stat_buff_t( this, 105647, "tier13_4pc_melee", STAT_MASTERY_RATING, dbc.spell( 105647 ) -> effect1().base_value() );
  buffs_unholy_presence     = new buff_t( this, "unholy_presence", "Unholy Presence" );

  struct bloodworms_buff_t : public buff_t
  {
    bloodworms_buff_t( death_knight_t* p ) :
      buff_t( p, "bloodworms", 1, 19.99, 0.0, p -> talents.blood_parasite -> rank() * 0.05 ) {}
    virtual void start( int stacks, double value )
    {
      buff_t::start( stacks, value );
      death_knight_t* p = player -> cast_death_knight();
      p -> active_bloodworms -> summon();
    }
    virtual void expire()
    {
      buff_t::expire();
      death_knight_t* p = player -> cast_death_knight();
      if ( p -> active_bloodworms ) p -> active_bloodworms -> dismiss();
    }
  };
  buffs_bloodworms = new bloodworms_buff_t( this );
}

// death_knight_t::init_values ====================================================

void death_knight_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_melee() )
    attribute_initial[ ATTR_STRENGTH ]   += 70;

  if ( set_bonus.pvp_4pc_melee() )
    attribute_initial[ ATTR_STRENGTH ]   += 90;
}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains_butchery                         = get_gain( "butchery"                   );
  gains_chill_of_the_grave               = get_gain( "chill_of_the_grave"         );
  gains_frost_presence                   = get_gain( "frost_presence"             );
  gains_horn_of_winter                   = get_gain( "horn_of_winter"             );
  gains_improved_frost_presence          = get_gain( "improved_frost_presence"    );
  gains_might_of_the_frozen_wastes       = get_gain( "might_of_the_frozen_wastes" );
  gains_power_refund                     = get_gain( "power_refund"               );
  gains_scent_of_blood                   = get_gain( "scent_of_blood"             );
  gains_tier12_2pc_melee                 = get_gain( "tier12_2pc_melee"           );
  gains_rune                             = get_gain( "rune_regen_all"             );
  gains_rune_unholy                      = get_gain( "rune_regen_unholy"          );
  gains_rune_blood                       = get_gain( "rune_regen_blood"           );
  gains_rune_frost                       = get_gain( "rune_regen_frost"           );
  gains_rune_unknown                     = get_gain( "rune_regen_unknown"         );
  gains_runic_empowerment                = get_gain( "runic_empowerment"          );
  gains_runic_empowerment_blood          = get_gain( "runic_empowerment_blood"    );
  gains_runic_empowerment_frost          = get_gain( "runic_empowerment_frost"    );
  gains_runic_empowerment_unholy         = get_gain( "runic_empowerment_unholy"   );
  gains_empower_rune_weapon              = get_gain( "empower_rune_weapon"        );
  gains_blood_tap                        = get_gain( "blood_tap"                  );
  // gains_blood_tap_blood                  = get_gain( "blood_tap_blood"            );
  gains_rune                     -> type = ( resource_type ) RESOURCE_RUNE         ;
  gains_rune_unholy              -> type = ( resource_type ) RESOURCE_RUNE_UNHOLY  ;
  gains_rune_blood               -> type = ( resource_type ) RESOURCE_RUNE_BLOOD   ;
  gains_rune_frost               -> type = ( resource_type ) RESOURCE_RUNE_FROST   ;
  gains_runic_empowerment        -> type = ( resource_type ) RESOURCE_RUNE         ;
  gains_runic_empowerment_blood  -> type = ( resource_type ) RESOURCE_RUNE_BLOOD   ;
  gains_runic_empowerment_unholy -> type = ( resource_type ) RESOURCE_RUNE_UNHOLY  ;
  gains_runic_empowerment_frost  -> type = ( resource_type ) RESOURCE_RUNE_FROST   ;
  gains_empower_rune_weapon      -> type = ( resource_type ) RESOURCE_RUNE         ;
  gains_blood_tap                -> type = ( resource_type ) RESOURCE_RUNE         ;
  //gains_blood_tap_blood          -> type = ( resource_type ) RESOURCE_RUNE_BLOOD   ;
}

// death_knight_t::init_procs ===============================================

void death_knight_t::init_procs()
{
  player_t::init_procs();

  proc_runic_empowerment        = get_proc( "runic_empowerment"            );
  proc_runic_empowerment_wasted = get_proc( "runic_empowerment_wasted"     );
  proc_oblit_killing_machine    = get_proc( "oblit_killing_machine"        );
  proc_fs_killing_machine       = get_proc( "frost_strike_killing_machine" );
}

// death_knight_t::init_resources ===========================================

void death_knight_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resource_current[ RESOURCE_RUNIC ] = 0;
}

// death_knight_t::init_uptimes =============================================

void death_knight_t::init_benefits()
{
  player_t::init_benefits();

  uptimes_rp_cap = get_benefit( "rp_cap" );
}

// death_knight_t::reset ====================================================

void death_knight_t::reset()
{
  player_t::reset();

  // Active
  active_presence = 0;

  _runes.reset();
}

// death_knight_t::combat_begin =============================================

void death_knight_t::combat_begin()
{
  player_t::combat_begin();

  double am_value = talents.abominations_might -> effect1().percent();
  if ( am_value > 0 && am_value >= sim -> auras.abominations_might -> current_value )
    sim -> auras.abominations_might -> trigger( 1, am_value );

  new ( sim ) butchery_event_t( this, 5.0 / talents.butchery -> rank() );
}

// death_knight_t::assess_damage ============================================

double death_knight_t::assess_damage( double            amount,
                                      const school_type school,
                                      int               dmg_type,
                                      int               result,
                                      action_t*         action )
{
  if ( buffs_blood_presence -> check() )
    amount *= 1.0 - dbc.spell( 61261 ) -> effect1().percent();

  if ( result != RESULT_MISS )
    buffs_scent_of_blood -> trigger();

  return player_t::assess_damage( amount, school, dmg_type, result, action );
}

// death_knight_t::composite_armor_multiplier ===============================

double death_knight_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buffs_blood_presence -> check() )
    a += buffs_blood_presence -> effect1().percent();

  return a;
}

// death_knight_t::composite_attack_power ===================================

double death_knight_t::composite_attack_power() const
{
  double ap = player_t::composite_attack_power();
  if ( talents.bladed_armor -> rank() )
    ap += composite_armor() / talents.bladed_armor -> effect1().base_value();

  if ( buffs_tier11_4pc_melee -> check() )
    ap *= 1.0 + buffs_tier11_4pc_melee -> stack() * buffs_tier11_4pc_melee -> effect1().percent();

  return ap;
}

// death_knight_t::composite_attribute_multiplier ===========================

double death_knight_t::composite_attribute_multiplier( int attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    m *= 1.0 + buffs_rune_of_the_fallen_crusader -> value();
    m *= 1.0 + buffs_pillar_of_frost -> value();
  }

  if ( attr == ATTR_STAMINA )
    if ( buffs_blood_presence -> check() )
      m *= 1.0 + buffs_blood_presence -> effect3().percent();

  return m;
}

// death_knight_t::matching_gear_multiplier =================================

double death_knight_t::matching_gear_multiplier( const attribute_type attr ) const
{
  int tree = primary_tree();

  if ( tree == TREE_UNHOLY || tree == TREE_FROST )
    if ( attr == ATTR_STRENGTH )
      return spells.plate_specialization -> effect1().percent();

  if ( tree == TREE_BLOOD )
    if ( attr == ATTR_STAMINA )
      return spells.plate_specialization -> effect1().percent();

  return 0.0;
}

// death_knight_t::composite_spell_hit ======================================

double death_knight_t::composite_spell_hit() const
{
  double hit = player_t::composite_spell_hit();

  hit += .09; // Not in Runic Empowerment's data yet

  return hit;
}

// death_knight_t::composite_tank_parry =====================================

double death_knight_t::composite_tank_parry() const
{
  double parry = player_t::composite_tank_parry();

  if ( buffs_dancing_rune_weapon -> up() )
    parry += 0.20;

  return parry;
}

// death_knight_t::composite_player_multiplier ==============================

double death_knight_t::composite_player_multiplier( const school_type school, action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  // Factor flat multipliers here so they effect procs, grenades, etc.
  m *= 1.0 + buffs_frost_presence -> value();
  m *= 1.0 + buffs_bone_shield -> value();

  if ( primary_tree() == TREE_UNHOLY && school == SCHOOL_SHADOW )
    m *= 1.0 + spells.dreadblade -> effect1().coeff() * 0.01 * composite_mastery();

  if ( primary_tree() == TREE_FROST && school == SCHOOL_FROST )
    m *= 1.0 + spells.frozen_heart -> effect1().coeff() * 0.01 * composite_mastery();

  return m;
}

// death_knight_t::composite_tank_crit ======================================

double death_knight_t::composite_tank_crit( const school_type school ) const
{
  double c = player_t::composite_tank_crit( school );

  if ( school == SCHOOL_PHYSICAL && talents.improved_blood_presence -> rank() )
    c += talents.improved_blood_presence -> effect2().percent();

  return c;
}

// death_knight_t::primary_role =============================================

int death_knight_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( primary_tree() == TREE_BLOOD )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// death_knight_t::regen ====================================================

void death_knight_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( set_bonus.tier12_2pc_melee() && sim -> auras.horn_of_winter -> check() )
    resource_gain( RESOURCE_RUNIC, 3.0 / 5.0 * periodicity, gains_tier12_2pc_melee );

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    _runes.slot[i].regen_rune( this, periodicity );

  uptimes_rp_cap -> update( resource_current[ RESOURCE_RUNIC ] ==
                            resource_max    [ RESOURCE_RUNIC] );
}

// death_knight_t::create_options ===========================================

void death_knight_t::create_options()
{
  player_t::create_options();

  option_t death_knight_options[] =
  {
    { "unholy_frenzy_target", OPT_STRING, &( unholy_frenzy_target_str ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, death_knight_options );
}

// death_knight_t::decode_set ===============================================

int death_knight_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "magma_plated" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T11_MELEE;
    if ( is_tank  ) return SET_T11_TANK;
  }

  if ( strstr( s, "elementium_deathplate" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "breastplate"   ) ||
                      strstr( s, "greaves"       ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T12_MELEE;
    if ( is_tank  ) return SET_T12_TANK;
  }

  if ( strstr( s, "necrotic_boneplate" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "breastplate"   ) ||
                      strstr( s, "greaves"       ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T13_MELEE;
    if ( is_tank  ) return SET_T13_TANK;
  }

  if ( strstr( s, "_gladiators_dreadplate_" ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// death_knight_t::trigger_runic_empowerment ================================

void death_knight_t::trigger_runic_empowerment()
{
  if ( ! sim -> roll( spells.runic_empowerment -> proc_chance() ) )
    return;

  if ( talents.runic_corruption -> rank() )
  {
    if ( buffs_runic_corruption -> check() )
      buffs_runic_corruption -> extend_duration( this, 3 );
    else
      buffs_runic_corruption -> trigger();

    if ( set_bonus.tier13_4pc_melee() )
      buffs_tier13_4pc_melee -> trigger( 1, 0, 0.40 );

    return;
  }

  int depleted_runes[RUNE_SLOT_MAX];
  int num_depleted=0;

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( _runes.slot[i].is_depleted() )
      depleted_runes[ num_depleted++ ] = i;

  if ( num_depleted > 0 )
  {
    int rune_to_regen = depleted_runes[ ( int ) ( sim -> rng -> real() * num_depleted * 0.9999 ) ];
    dk_rune_t* regen_rune = &_runes.slot[rune_to_regen];
    regen_rune -> fill_rune();
    if      ( regen_rune -> is_blood()  ) gains_runic_empowerment_blood  -> add ( 1,0 );
    else if ( regen_rune -> is_unholy() ) gains_runic_empowerment_unholy -> add ( 1,0 );
    else if ( regen_rune -> is_frost()  ) gains_runic_empowerment_frost  -> add ( 1,0 );

    gains_runic_empowerment -> add ( 1,0 );
    if ( sim -> log ) log_t::output( sim, "runic empowerment regen'd rune %d", rune_to_regen );
    proc_runic_empowerment -> occur();

    if ( set_bonus.tier13_4pc_melee() )
      buffs_tier13_4pc_melee -> trigger( 1, 0, 0.25 );
  }
  else
  {
    // If there were no available runes to refresh
    proc_runic_empowerment_wasted -> occur();
    gains_runic_empowerment -> add ( 0,1 );
  }
}

// death_knight_t rune inspections ==========================================

// death_knight_t::runes_count ==============================================
// how many runes of type rt are available
int death_knight_t::runes_count( rune_type rt, bool include_death, int position )
{
  int result = 0;
  // positional checks first
  if ( position > 0 && ( rt == RUNE_TYPE_BLOOD || rt == RUNE_TYPE_FROST || rt == RUNE_TYPE_UNHOLY ) )
  {
    dk_rune_t* r = &_runes.slot[( ( rt-1 )*2 ) + ( position - 1 ) ];
    if ( r -> is_ready() )
      result = 1;
  }
  else
  {
    int rpc = 0;
    for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
    {
      dk_rune_t* r = &_runes.slot[ i ];
      // query a specific position death rune.
      if ( position != 0 && rt == RUNE_TYPE_DEATH && r -> is_death() )
      {
        if ( ++rpc == position )
        {
          if ( r -> is_ready() )
            result = 1;
          break;
        }
      }
      // just count the runes
      else if ( ( ( ( include_death || rt == RUNE_TYPE_DEATH ) && r -> is_death() ) || ( r -> get_type() == rt ) )
                && r -> is_ready() )
      {
        result++;
      }
    }
  }
  return result;
}

// death_knight_t::runes_cooldown_any =======================================

double death_knight_t::runes_cooldown_any( rune_type rt, bool include_death, int position )
{
  dk_rune_t* rune = 0;
  int rpc = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
  {
    dk_rune_t* r = &_runes.slot[i];
    if ( position == 0 && include_death && r -> is_death() && r -> is_ready() )
      return 0;
    if ( position == 0 && r -> get_type() == rt && r -> is_ready() )
      return 0;
    if ( ( ( include_death && r -> is_death() ) || ( r -> get_type() == rt ) ) )
    {
      if ( position != 0 && ++rpc == position )
      {
        rune = r;
        break;
      }
      if ( !rune || ( r -> value > rune -> value ) )
        rune = r;
    }
  }

  assert( rune );

  double time = this -> runes_cooldown_time( rune );
  // if it was a  specified rune and is depleted, we have to add its paired rune to the time
  if ( rune -> is_depleted() )
    time += this -> runes_cooldown_time( rune -> paired_rune );

  return time;
}

// death_knight_t::runes_cooldown_all =======================================

double death_knight_t::runes_cooldown_all( rune_type rt, bool include_death, int position )
{
  // if they specified position then they only get 1 answer. duh. handled in the other function
  if ( position > 0 )
    return this -> runes_cooldown_any( rt, include_death, position );

  // find all matching runes. total the pairs. Return the highest number.

  double max = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; i+=2 )
  {
    double total = 0;
    dk_rune_t* r = &_runes.slot[i];
    if ( ( ( rt == RUNE_TYPE_DEATH && r -> is_death() ) || r -> get_type() == rt ) && !r -> is_ready() )
    {
      total += this->runes_cooldown_time( r );
    }
    r = r -> paired_rune;
    if ( ( ( rt == RUNE_TYPE_DEATH && r -> is_death() ) || r -> get_type() == rt ) && !r -> is_ready() )
    {
      total += this->runes_cooldown_time( r );
    }
    if ( max < total )
      max = total;
  }
  return max;
}

// death_knight_t::runes_cooldown_time ======================================

double death_knight_t::runes_cooldown_time( dk_rune_t* rune )
{
  double result_num;
  death_knight_t* dk = this -> cast_death_knight();

  double runes_per_second = 1.0 / 10.0 / dk -> composite_attack_haste();
  if ( dk -> buffs_blood_presence -> check() && dk -> talents.improved_blood_presence -> rank() )
    runes_per_second *= 1.0 + dk -> talents.improved_blood_presence -> effect3().percent();
  // Unholy Presence's 10% (or, talented, 15%) increase is factored in elsewhere as melee haste.
  if ( dk -> buffs_runic_corruption -> check() )
    runes_per_second *= 1.0 + ( dk -> talents.runic_corruption -> effect1().percent() );
  result_num = ( 1.0 - rune -> value ) / runes_per_second;

  return result_num;
}

// death_knight_t::runes_depleted ===========================================

bool death_knight_t::runes_depleted( rune_type rt, int position )
{
  dk_rune_t* rune = 0;
  int rpc = 0;
  // iterate, to allow finding death rune slots as well
  for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
  {
    dk_rune_t* r = &_runes.slot[i];
    if ( r -> get_type() == rt && ++rpc == position )
    {
      rune = r;
      break;
    }
  }
  if ( ! rune ) return false;
  return rune -> is_depleted();
}

// ==========================================================================
// player_t implementations
// ==========================================================================

// player_t::create_death_knight ============================================

player_t* player_t::create_death_knight( sim_t* sim, const std::string& name, race_type r )
{
  return new death_knight_t( sim, name, r );
}

// player_t::death_knight_init ==============================================

void player_t::death_knight_init( sim_t* sim )
{
  sim -> auras.abominations_might  = new aura_t( sim, "abominations_might",  1,   0.0 );
  sim -> auras.horn_of_winter      = new aura_t( sim, "horn_of_winter",      1, 120.0 );
  sim -> auras.improved_icy_talons = new aura_t( sim, "improved_icy_talons", 1,   0.0 );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.unholy_frenzy        = new   buff_t( p, 49016, "unholy_frenzy" );
    p -> debuffs.brittle_bones      = new debuff_t( p, "brittle_bones",      1 );
    p -> debuffs.ebon_plaguebringer = new debuff_t( p, 65142, "ebon_plaguebringer" );
    p -> debuffs.scarlet_fever      = new debuff_t( p, "scarlet_fever",      1, 21.0 );
  }
}

// player_t::death_knight_combat_begin ======================================

void player_t::death_knight_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.abominations_might  )
    sim -> auras.abominations_might  -> override( 1, 0.10 );

  if ( sim -> overrides.horn_of_winter      )
    sim -> auras.horn_of_winter      -> override( 1, sim -> dbc.effect_average( sim -> dbc.spell( 57330 ) -> effect1().id(), sim -> max_player_level ) );

  if ( sim -> overrides.improved_icy_talons )
    sim -> auras.improved_icy_talons -> override( 1, 0.10 );

  for ( player_t* t = sim -> target_list; t; t = t -> next )
  {
    if ( sim -> overrides.brittle_bones      ) t -> debuffs.brittle_bones      -> override( 1, 0.04 );
    if ( sim -> overrides.ebon_plaguebringer ) t -> debuffs.ebon_plaguebringer -> override( 1,  8.0 );
    if ( sim -> overrides.scarlet_fever      ) t -> debuffs.scarlet_fever      -> override();
  }
}
