// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

/*
 * To Do:
 * - Check if curse_of_elements is counted for affliction_effects in every possible situation
 *  (optimal_raid=1, or with multiple wl's, dk's/druis), it enhances drain soul damage.
 * - Bane of Agony and haste scaling looks strange (and is crazy as hell, we'll leave it at average base_td for now)
 * - Seed of Corruption with Soulburn: Trigger Corruptions
 * - Execute felguard:felstorm by player, not the pet
 * - Verify if the current incinerate bonus calculation is correct
 * - Investigate pet mp5 - probably needs to scale with owner int
 * - Dismissing a pet drops aura -> demonic_pact. Either don't let it dropt if there is another pet alive, or recheck application when dropping it.
 */

// ==========================================================================
// Warlock
// ==========================================================================

#if SC_WARLOCK == 1

int warlock_targetdata_t::affliction_effects()
{
  int effects = 0;
  if ( dots_curse_of_elements -> ticking    ) effects++;
  if ( dots_bane_of_agony -> ticking        ) effects++;
  if ( dots_bane_of_doom -> ticking         ) effects++;
  if ( dots_corruption -> ticking           ) effects++;
  if ( dots_drain_life -> ticking           ) effects++;
  if ( dots_drain_soul -> ticking           ) effects++;
  if ( dots_unstable_affliction -> ticking  ) effects++;
  if ( debuffs_haunted        -> check()    ) effects++;
  if ( debuffs_shadow_embrace -> check()    ) effects++;
  return effects;
}

int warlock_targetdata_t::active_dots()
{
  int dots = 0;
  if ( dots_bane_of_agony -> ticking            ) dots++;
  if ( dots_bane_of_doom -> ticking             ) dots++;
  if ( dots_corruption -> ticking               ) dots++;
  if ( dots_drain_life -> ticking               ) dots++;
  if ( dots_drain_soul -> ticking               ) dots++;
  if ( dots_immolate -> ticking                 ) dots++;
  if ( dots_shadowflame_dot -> ticking          ) dots++;
  if ( dots_unstable_affliction -> ticking      ) dots++;
  return dots;
}


void register_warlock_targetdata( sim_t* sim )
{
  player_type_e t = WARLOCK;
  typedef warlock_targetdata_t type;

  REGISTER_DOT( corruption );
  REGISTER_DOT( unstable_affliction );
  REGISTER_DOT( bane_of_agony );
  REGISTER_DOT( bane_of_doom );
  REGISTER_DOT( immolate );
  REGISTER_DOT( drain_life );
  REGISTER_DOT( drain_soul );
  REGISTER_DOT( shadowflame_dot );
  REGISTER_DOT( curse_of_elements );
  REGISTER_DOT( burning_embers );

  REGISTER_DEBUFF( haunted );
  REGISTER_DEBUFF( shadow_embrace );
}

warlock_targetdata_t::warlock_targetdata_t( player_t* source, player_t* target )
  : targetdata_t( source, target )
{
  warlock_t* p = this->source->cast_warlock();
  debuffs_haunted               = add_aura( new buff_t( this, p -> talent_haunt -> spell_id(), "haunted", p -> talent_haunt -> rank() ) );
  debuffs_shadow_embrace        = add_aura( new buff_t( this, p -> talent_shadow_embrace -> effect_trigger_spell( 1 ), "shadow_embrace", p -> talent_shadow_embrace -> rank() ) );
}

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_type_e r ) :
  player_t( sim, WARLOCK, name, r )
{

  tree_type[ WARLOCK_AFFLICTION  ] = TREE_AFFLICTION;
  tree_type[ WARLOCK_DEMONOLOGY  ] = TREE_DEMONOLOGY;
  tree_type[ WARLOCK_DESTRUCTION ] = TREE_DESTRUCTION;

  distance = 40;
  default_distance = 40;

  active_pet                  = 0;
  pet_ebon_imp                = 0;
  spells_burning_embers       = 0;

  cooldowns.metamorphosis                   = get_cooldown ( "metamorphosis" );
  cooldowns.infernal                        = get_cooldown ( "summon_infernal" );
  cooldowns.doomguard                       = get_cooldown ( "summon_doomguard" );

  use_pre_soulburn = 1;

  create_talents();
  create_glyphs();
  create_options();
}


namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Warlock Heal
// ==========================================================================

struct warlock_heal_t : public heal_t
{
  warlock_heal_t( const char* n, warlock_t* p, const uint32_t id ) :
    heal_t( n, p, id )
  { }

  warlock_t* p() const
  { return static_cast<warlock_t*>( player ); }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();

    heal_t::player_buff();

    player_multiplier *= 1.0 + p -> buffs.demon_armor -> value();
  }
};

// ==========================================================================
// Warlock Spell
// ==========================================================================

struct warlock_spell_t : public spell_t
{
private:
  void _init_warlock_spell_t()
  {
    may_crit      = true;
    tick_may_crit = true;
    dot_behavior  = DOT_REFRESH;
    weapon_multiplier = 0.0;
    crit_multiplier *= 1.33;
  }

public:
  warlock_spell_t( const char* n, warlock_t* p, const school_type_e s, talent_tree_type_e t ) :
    spell_t( n, p, RESOURCE_MANA, s, t )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const spell_id_t& s, talent_tree_type_e t = TREE_NONE ) :
    spell_t( s, t )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const char* n, warlock_t* p, const char* sname, talent_tree_type_e t = TREE_NONE ) :
    spell_t( n, sname, p, t )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const char* n, warlock_t* p, uint32_t id, talent_tree_type_e t = TREE_NONE ) :
    spell_t( n, id, p, t )
  {
    _init_warlock_spell_t();
  }

  warlock_t* p() const
  { return static_cast<warlock_t*>( player ); }

  // warlock_spell_t::haste =================================================

  virtual double haste() const
  {
    warlock_t* p = player -> cast_warlock();
    double h = spell_t::haste();

    if ( p -> buffs.eradication -> up() )
    {
      h *= 1.0 / ( 1.0 + p -> buffs.eradication -> effect1().percent() );
    }

    if ( p -> buffs.demon_soul_felguard -> up() )
    {
      h *= 1.0 / ( 1.0 + p -> buffs.demon_soul_felguard -> effect1().percent() );
    }

    return h;
  }

  // warlock_spell_t::player_buff ===========================================

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();

    spell_t::player_buff();

    if ( base_execute_time > timespan_t::zero() && s_tree == TREE_DESTRUCTION && p -> buffs.demon_soul_imp -> up() )
    {
      player_crit += p -> buffs.demon_soul_imp -> effect1().percent();
    }

    if ( school == SCHOOL_SHADOW || school == SCHOOL_SHADOWFLAME )
      player_multiplier *= 1.0 + trigger_deaths_embrace( this );
  }

  // warlock_spell_t::target_debuff =========================================

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    warlock_t* p = player -> cast_warlock();

    spell_t::target_debuff( t, dtype );

    if ( p -> buffs.bane_of_havoc -> up() )
      target_multiplier *= 1.0 + p -> buffs.bane_of_havoc -> effect1().percent();
  }

  // warlock_spell_t::total_td_multiplier ===================================

  virtual double total_td_multiplier() const
  {
    double shadow_td_multiplier = 1.0;
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    if ( school == SCHOOL_SHADOW || school == SCHOOL_SHADOWFLAME )
    {
      //FIXME: These should be modeled as debuffs, and we need to check if they affect trinkets (if there is ever a shadow td trinket)
      if ( td -> debuffs_shadow_embrace -> up() )
      {
        shadow_td_multiplier *= 1.0 + td -> debuffs_shadow_embrace -> check() * td -> debuffs_shadow_embrace -> effect1().percent();
      }
      if ( td -> debuffs_haunted -> up() )
      {
        shadow_td_multiplier *= 1.0 + td -> debuffs_haunted -> effect3().percent() + ( p -> glyphs.haunt -> effect1().percent() );
      }
    }

    return spell_t::total_td_multiplier() * shadow_td_multiplier;
  }

  // trigger_impending_doom =================================================

  static void trigger_impending_doom ( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();

    if ( p -> rngs.impending_doom -> roll ( p -> talent_impending_doom -> proc_chance() ) )
    {
      p -> procs.impending_doom -> occur();
      if ( p -> cooldowns.metamorphosis -> remains() > timespan_t::from_seconds( p -> talent_impending_doom -> effect2().base_value() ) )
        p -> cooldowns.metamorphosis -> ready -= timespan_t::from_seconds( p -> talent_impending_doom -> effect2().base_value() );
      else
        p -> cooldowns.metamorphosis -> reset();
    }
  }

  // trigger_soul_leech =====================================================

  static void trigger_soul_leech( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();

    if ( p -> talent_soul_leech -> rank() )
    {
      p -> resource_gain( RESOURCE_HEALTH, p -> resources.max[ RESOURCE_HEALTH ] * p -> talent_soul_leech -> effect1().percent(), p -> gains.soul_leech_health );
      p -> resource_gain( RESOURCE_MANA, p -> resources.max[ RESOURCE_MANA ] * p -> talent_soul_leech -> effect1().percent(), p -> gains.soul_leech );

    }
  }

  // trigger_decimation =====================================================

  static void trigger_decimation( warlock_spell_t* s, int result )
  {
    warlock_t* p = s -> player -> cast_warlock();
    if ( ( result != RESULT_HIT ) && ( result != RESULT_CRIT ) ) return;
    if ( s -> target -> health_percentage() > p -> talent_decimation -> effect2().base_value() ) return;
    p -> buffs.decimation -> trigger();
  }

  // trigger_deaths_embrace =================================================

  static double trigger_deaths_embrace( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();

    if ( ! p -> talent_deaths_embrace -> rank() ) return 0;


    // The target health percentage is ONLY contained in the Rank-1 version of the talent.
    if ( s -> target -> health_percentage() <= p -> talent_deaths_embrace -> spell( 1 ).effect3().base_value() )
    {
      return p -> talent_deaths_embrace -> effect2().percent();
    }

    return 0;
  }

  // trigger_everlasting_affliction =========================================

  static void trigger_everlasting_affliction( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();
    warlock_targetdata_t* td = s -> targetdata() -> cast_warlock();

    if ( ! p -> talent_everlasting_affliction -> rank() ) return;

    if ( ! td -> dots_corruption -> ticking ) return;

    if ( p -> rngs.everlasting_affliction -> roll( p -> talent_everlasting_affliction -> proc_chance() ) )
    {
      td -> dots_corruption -> refresh_duration();
    }
  }
};


// Curse of Elements Spell ==================================================

struct curse_of_elements_t : public warlock_spell_t
{
  curse_of_elements_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "curse_of_the_elements", p, "Curse of the Elements" )
  {
    parse_options( NULL, options_str );

    trigger_gcd -= p -> constants_pandemic_gcd * p -> talent_pandemic -> rank();
    background = ( sim -> overrides.magic_vulnerability != 0 );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      if ( ! sim -> overrides.magic_vulnerability )
        target -> debuffs.magic_vulnerability -> trigger( 1, -1, -1, duration() );
    }
  }
};

// Bane of Agony Spell ======================================================

struct bane_of_agony_t : public warlock_spell_t
{
  bane_of_agony_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "bane_of_agony", p, "Bane of Agony" )
  {
    /*
     * BOA DATA:
     * No Glyph
     * 16 Ticks:
     * middle low low low low low    middle middle middle middle middle high high high high high
     * 15 ticks:
     * low    low low low low middle middle middle middle middle high   high high high high
     * 14 ticks:
     * low    low low low low middle middle middle middle high strange strange strange
     * 828 828 403 403 403 486 998 485 485 1167  691 1421 691 1316
     * 374 373 373 373 767 449 449 450 924  526  640  640 640 1316
     * 378 777 777 777 378 461 460 460 460  543 1370  666 666  666
     * 378 378 777 378 378 460 460 946 460  542 1369  667 666 1369
     *
     *
     * 13 ticks:
     * 2487 sp, lvl 80 -> middle = 130.14 + 0.1000000015 * 2487 = 378.84
     * -> difference = base_td / 2, only 1 middle tick at the beginning
     * 378 645 313 313 313 378 379 778 378 443 444 444 443
     * 12 ticks:
     * low low low low    middle middle middle middle   high high high high
     * and it looks like low-tick = middle-tick - base_td / 2
     * and              high-tick = middle-tick + base_td / 2
     * at 12 ticks, everything is very consistent with this logic
     */

    parse_options( NULL, options_str );

    may_crit   = false;

    base_crit += p -> talent_doom_and_gloom -> effect1().percent();
    trigger_gcd -= p -> constants_pandemic_gcd * p -> talent_pandemic -> rank();

    int extra_ticks = ( int ) ( timespan_t::from_millis( p -> glyphs.bane_of_agony -> base_value() ) / base_tick_time );

    if ( extra_ticks > 0 )
    {
      // after patch 3.0.8, the added ticks are double the base damage

      double total_damage = base_td_init * ( num_ticks + extra_ticks * 2 );

      num_ticks += extra_ticks;

      base_td_init = total_damage / num_ticks;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      if ( td -> dots_bane_of_doom -> ticking )
      {
        td -> dots_bane_of_doom -> cancel();
      }
      else if ( p -> buffs.bane_of_havoc -> up() )
      {
        p -> buffs.bane_of_havoc -> expire();
      }
    }
  }
};

// Bane of Doom Spell =======================================================

struct bane_of_doom_t : public warlock_spell_t
{
  bane_of_doom_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "bane_of_doom", p, "Bane of Doom" )
  {
    parse_options( NULL, options_str );

    hasted_ticks = false;
    may_crit     = false;

    trigger_gcd -= p -> constants_pandemic_gcd * p -> talent_pandemic -> rank();
    base_crit += p -> talent_doom_and_gloom -> effect1().percent();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      if ( td -> dots_bane_of_agony -> ticking )
      {
        td -> dots_bane_of_agony -> cancel();
      }
      else if ( p -> buffs.bane_of_havoc -> up() )
      {
        p -> buffs.bane_of_havoc -> expire();
      }
    }
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );
    warlock_t* p = player -> cast_warlock();

    double x = effect2().percent() + p -> talent_impending_doom -> effect1().percent();
    if ( p -> rngs.ebon_imp -> roll ( x ) )
    {
      p -> procs.ebon_imp -> occur();
      p -> pet_ebon_imp -> dismiss();
      p -> pet_ebon_imp -> summon( timespan_t::from_seconds( 14.99 ) );
    }
  }
};

// Bane of Havoc Spell ======================================================

struct bane_of_havoc_t : public warlock_spell_t
{
  bane_of_havoc_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "bane_of_havoc", p, "Bane of Havoc" )
  {
    parse_options( NULL, options_str );

    trigger_gcd -= p -> constants_pandemic_gcd * p -> talent_pandemic -> rank();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      p -> buffs.bane_of_havoc -> trigger();

      if ( td -> dots_bane_of_agony -> ticking )
      {
        td -> dots_bane_of_agony -> cancel();
      }
      else if ( td -> dots_bane_of_doom -> ticking )
      {
        td -> dots_bane_of_doom -> cancel();
      }
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.bane_of_havoc -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Shadow Bolt Spell ========================================================

struct shadow_bolt_t : public warlock_spell_t
{
  int isb;
  bool used_shadow_trance;

  shadow_bolt_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "shadow_bolt", p, "Shadow Bolt" ),
    isb( 0 ), used_shadow_trance( 0 )
  {
    option_t options[] =
    {
      { "isb",   OPT_BOOL, &isb },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_execute_time += p -> talent_bane -> effect1().time_value();
    base_costs[ current_resource() ]  *= 1.0 + p -> glyphs.shadow_bolt -> base_value();
    base_multiplier *= 1.0 + ( p -> talent_shadow_and_flame -> effect2().percent() );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new shadow_bolt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t h = warlock_spell_t::execute_time();
    warlock_t* p = player -> cast_warlock();
    if ( p -> buffs.shadow_trance -> up() ) h = timespan_t::zero();
    if ( p -> buffs.backdraft -> up() )
    {
      h *= 1.0 + p -> buffs.backdraft -> effect1().percent();
    }
    return h;
  }

  virtual void schedule_execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::schedule_execute();

    if ( p -> buffs.shadow_trance -> check() )
    {
      p -> buffs.shadow_trance -> expire();
      used_shadow_trance = true;
    }
    else
    {
      used_shadow_trance = false;
    }
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    for ( int i=0; i < 4; i++ )
    {
      p -> uptimes_backdraft[ i ] -> update( i == p -> buffs.backdraft -> check() );
    }

    if ( ! used_shadow_trance && p -> buffs.backdraft -> check() )
    {
      p -> buffs.backdraft -> decrement();
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    if ( p -> buffs.demon_soul_succubus -> up() )
    {
      player_multiplier *= 1.0 + p -> buffs.demon_soul_succubus -> effect1().percent();
    }
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();

      trigger_decimation( this, impact_result );
      trigger_impending_doom( this );
      td -> debuffs_shadow_embrace -> trigger();
    }
  }
};

// Burning Embers Spell =====================================================

// Chaos Bolt Spell =========================================================

struct chaos_bolt_t : public warlock_spell_t
{
  chaos_bolt_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "chaos_bolt", p, "Chaos Bolt" )
  {
    parse_options( NULL, options_str );

    may_miss = false;

    base_execute_time += p -> talent_bane -> effect1().time_value();
    cooldown -> duration += timespan_t::from_millis( p -> glyphs.chaos_bolt -> base_value() );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new chaos_bolt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t h = warlock_spell_t::execute_time();
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.backdraft -> up() )
      h *= 1.0 + p -> buffs.backdraft -> effect1().percent();

    return h;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    for ( int i=0; i < 4; i++ )
    {
      p -> uptimes_backdraft[ i ] -> update( i == p -> buffs.backdraft -> check() );
    }

    if ( p -> buffs.backdraft -> check() )
      p -> buffs.backdraft -> decrement();
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();
    warlock_spell_t::player_buff();

    if ( td -> dots_immolate -> ticking )
      player_multiplier *= 1 + p -> talent_fire_and_brimstone -> effect1().percent();
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      trigger_soul_leech( this );
  }
};

// Death Coil Spell =========================================================

struct death_coil_t : public warlock_spell_t
{
  death_coil_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "death_coil", p, "Death Coil" )
  {
    parse_options( NULL, options_str );
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      player -> resource_gain( RESOURCE_HEALTH, direct_dmg );
  }
};

// Shadow Burn Spell ========================================================

struct shadowburn_t : public warlock_spell_t
{
  cooldown_t* cd_glyph_of_shadowburn;

  shadowburn_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "shadowburn", p, "Shadowburn" ),
    cd_glyph_of_shadowburn( 0 )
  {
    check_talent( p -> talent_shadowburn -> rank() );

    parse_options( NULL, options_str );


    if ( p -> glyphs.shadowburn -> ok() )
    {
      cd_glyph_of_shadowburn             = p -> get_cooldown ( "glyph_of_shadowburn" );
      cd_glyph_of_shadowburn -> duration = p -> dbc.spell( 91001 ) -> duration();
    }

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new shadowburn_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      trigger_soul_leech( this );
  }

  virtual void update_ready()
  {
    warlock_spell_t::update_ready();
    warlock_t* p = player -> cast_warlock();

    if ( p -> glyphs.shadowburn -> ok() )
    {
      if ( cd_glyph_of_shadowburn -> remains() == timespan_t::zero() && target -> health_percentage() < p -> glyphs.shadowburn -> effect1().base_value() )
      {
        cooldown -> reset();
        cd_glyph_of_shadowburn -> start();
      }
    }
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() >= 20 )
      return false;

    return warlock_spell_t::ready();
  }
};

// Shadowfury Spell =========================================================

struct shadowfury_t : public warlock_spell_t
{
  timespan_t cast_gcd;

  shadowfury_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "shadowfury", p, "Shadowfury" ),
    cast_gcd( timespan_t::zero() )
  {
    check_talent( p -> talent_shadowfury -> rank() );

    option_t options[] =
    {
      { "cast_gcd",    OPT_TIMESPAN,  &cast_gcd    },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    // estimate - measured at ~0.6sec, but lag in there too, plus you need to mouse-click
    trigger_gcd = ( cast_gcd >= timespan_t::zero() ) ? cast_gcd : timespan_t::from_seconds( 0.5 );
  }
};

// Corruption Spell =========================================================

struct corruption_t : public warlock_spell_t
{
  corruption_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "corruption", p, "Corruption" )
  {
    parse_options( NULL, options_str );

    may_crit   = false;
    base_crit += p -> talent_everlasting_affliction -> effect2().percent();
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    player_td_multiplier += p -> talent_improved_corruption -> effect1().percent();
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick( d );

    p -> buffs.eradication -> trigger();

    if ( p -> buffs.shadow_trance -> trigger() )
      p -> procs.shadow_trance -> occur();

    if ( p -> talent_siphon_life -> rank() )
    {
      if ( p -> rngs.siphon_life -> roll ( p -> talent_siphon_life -> proc_chance() ) )
      {
        p -> resource_gain( RESOURCE_HEALTH, p -> resources.max[ RESOURCE_HEALTH ] * 0.02 );
      }
    }
  }
};

// Drain Life Spell =========================================================

struct drain_life_heal_t : public warlock_heal_t
{
  drain_life_heal_t( warlock_t* p ) :
    warlock_heal_t( "drain_life_heal", p, 89653 )
  {
    background = true;
    may_miss = false;
    base_dd_min = base_dd_max = 0; // Is parsed as 2
    init();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    double heal_pct = effect1().percent();

    if ( ( p -> resources.current[ RESOURCE_HEALTH ] / p -> resources.max[ RESOURCE_HEALTH ] ) <= 0.25 )
      heal_pct += p -> talent_deaths_embrace -> effect1().percent();

    base_dd_min = base_dd_max = p -> resources.max[ RESOURCE_HEALTH ] * heal_pct;
    warlock_heal_t::execute();
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();
    warlock_heal_t::player_buff();

    double min_multiplier[] = { 0, 0.03, 0.06 };
    double max_multiplier [3] = {0};
    if ( p -> bugs )
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.12;
      max_multiplier[2] =  0.24;
    }
    else
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.09;
      max_multiplier[2] =  0.18;
    }

    assert( p -> talent_soul_siphon -> rank() <= 2 );

    double min = min_multiplier[ p -> talent_soul_siphon -> rank() ];
    double max = max_multiplier[ p -> talent_soul_siphon -> rank() ];

    double multiplier = td -> affliction_effects() * min;

    if ( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;
  }
};

struct drain_life_t : public warlock_spell_t
{
  drain_life_heal_t* heal;

  drain_life_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "drain_life", p, "Drain Life" ), heal( 0 )
  {
    parse_options( NULL, options_str );

    channeled    = true;
    hasted_ticks = false;
    may_crit     = false;

    heal = new drain_life_heal_t( p );
  }

  virtual void last_tick( dot_t* d )
  {
    warlock_spell_t::last_tick( d );
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.soulburn -> check() )
      p -> buffs.soulburn -> expire();
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      trigger_everlasting_affliction( this );
  }

  virtual timespan_t tick_time( double haste ) const
  {
    warlock_t* p = player -> cast_warlock();
    timespan_t t = warlock_spell_t::tick_time( haste );

    if ( p -> buffs.soulburn -> up() )
      t *= 1.0 - 0.5;

    return t;
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();
    warlock_spell_t::player_buff();

    double min_multiplier[] = { 0, 0.03, 0.06 };
    double max_multiplier [3] = {0};
    if ( p -> bugs )
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.12;
      max_multiplier[2] =  0.24;
    }
    else
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.09;
      max_multiplier[2] =  0.18;
    }

    assert( p -> talent_soul_siphon -> rank() <= 2 );

    double min = min_multiplier[ p -> talent_soul_siphon -> rank() ];
    double max = max_multiplier[ p -> talent_soul_siphon -> rank() ];

    double multiplier = td -> affliction_effects() * min;

    if ( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick( d );

    if ( p -> buffs.shadow_trance -> trigger( 1, 1.0, p -> talent_nightfall -> proc_chance() ) )
      p -> procs.shadow_trance -> occur();

    heal -> execute();
  }
};

// Drain Soul Spell =========================================================

struct drain_soul_t : public warlock_spell_t
{
  drain_soul_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "drain_soul", p, "Drain Soul" )
  {
    parse_options( NULL, options_str );

    channeled    = true;
    hasted_ticks = true; // informative
    may_crit     = false;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( result_is_hit() )
    {
      warlock_t* p = player -> cast_warlock();

      trigger_everlasting_affliction( this );

      if ( p -> talent_pandemic -> rank() )
      {
        if ( ( target -> health_percentage() < effect3().base_value() ) && ( p -> rngs.pandemic -> roll( p -> talent_pandemic -> rank() * 0.5 ) ) )
        {
          warlock_targetdata_t* td = targetdata() -> cast_warlock();
          td -> dots_unstable_affliction -> refresh_duration();
        }
      }
    }
  }

  virtual void player_buff()
  {
    warlock_spell_t::player_buff();
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    double min_multiplier[] = { 0, 0.03, 0.06 };
    double max_multiplier [3] = {0};
    if ( p -> bugs )
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.12;
      max_multiplier[2] =  0.24;
    }
    else
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.09;
      max_multiplier[2] =  0.18;
    }

    assert( p -> talent_soul_siphon -> rank() <= 2 );

    double min = min_multiplier[ p -> talent_soul_siphon -> rank() ];
    double max = max_multiplier[ p -> talent_soul_siphon -> rank() ];

    double multiplier = td -> affliction_effects() * min;

    if ( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;

    // FIXME! Hack! Deaths Embrace is additive with Drain Soul "execute".
    // Perhaps it is time to add notion of "execute" into action_t class.

    double de_bonus = trigger_deaths_embrace( this );
    if ( de_bonus ) player_multiplier /= 1.0 + de_bonus;


    if ( target -> health_percentage() < effect3().base_value() )
    {
      player_multiplier *= 2.0 + de_bonus;
    }
  }
};

// Unstable Affliction Spell ================================================

struct unstable_affliction_t : public warlock_spell_t
{
  unstable_affliction_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "unstable_affliction", p, "Unstable Affliction" )
  {
    parse_options( NULL, options_str );

    check_talent( ok() );

    may_crit   = false;
    base_crit += p -> talent_everlasting_affliction -> effect2().percent();
    base_execute_time += timespan_t::from_millis( p -> glyphs.unstable_affliction -> base_value() );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      if ( td -> dots_immolate -> ticking )
        td -> dots_immolate -> cancel();
    }
  }
};

// Haunt Spell ==============================================================

struct haunt_t : public warlock_spell_t
{
  haunt_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "Haunt", p, "Haunt" )
  {
    check_talent( p -> talent_haunt -> rank() );

    parse_options( NULL, options_str );

    direct_power_mod = 0.5577;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new haunt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      td -> debuffs_haunted -> trigger();
      td -> debuffs_shadow_embrace -> trigger();
      trigger_everlasting_affliction( this );
    }
  }
};

// Immolate Spell ===========================================================

struct immolate_t : public warlock_spell_t
{
  immolate_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "immolate", p, "Immolate" )
  {
    parse_options( NULL, options_str );

    base_execute_time += p -> talent_bane -> effect1().time_value();

    base_dd_multiplier *= 1.0 + ( p -> talent_improved_immolate -> effect1().percent() );

    if ( p -> talent_inferno -> rank() ) num_ticks += 2;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new immolate_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    player_td_multiplier += ( p -> glyphs.immolate -> base_value() + p -> talent_improved_immolate -> effect1().percent() );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      if ( td -> dots_unstable_affliction -> ticking )
      {
        td -> dots_unstable_affliction -> cancel();
      }
    }
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );
    warlock_t* p = player -> cast_warlock();

    p -> buffs.molten_core -> trigger( 3 );
  }

};

// Shadowflame DOT Spell ====================================================

struct shadowflame_dot_t : public warlock_spell_t
{
  shadowflame_dot_t( warlock_t* p ) :
    warlock_spell_t( "shadowflame_dot", p, 47960 )
  {
    proc       = true;
    background = true;
  }
};

// Shadowflame Spell ========================================================

struct shadowflame_t : public warlock_spell_t
{
  shadowflame_dot_t* sf_dot;

  shadowflame_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "shadowflame", p, "Shadowflame" ), sf_dot( 0 )
  {
    parse_options( NULL, options_str );

    sf_dot = new shadowflame_dot_t( p );

    add_child( sf_dot );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new shadowflame_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    sf_dot -> execute();
  }
};

// Conflagrate Spell ========================================================

struct conflagrate_t : public warlock_spell_t
{
  conflagrate_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "conflagrate", p, "Conflagrate" )
  {
    check_talent( p -> talent_conflagrate -> ok() );

    parse_options( NULL, options_str );

    base_crit += p -> talent_fire_and_brimstone -> effect2().percent();
    cooldown -> duration += timespan_t::from_millis( p -> glyphs.conflagrate -> base_value() );
    base_dd_multiplier *= 1.0 + ( p -> glyphs.immolate -> base_value() ) + ( p -> talent_improved_immolate -> effect1().percent() );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new conflagrate_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    action_t* a = td -> dots_immolate -> action;

    double periodic_dmg = a -> base_td + total_power() * a -> tick_power_mod;

    int periodic_ticks = td -> dots_immolate -> action -> hasted_num_ticks( player_haste );

    base_dd_min = base_dd_max = periodic_dmg * periodic_ticks * effect2().percent() ;

    warlock_spell_t::execute();
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      p -> buffs.backdraft -> trigger( 3 );
  }

  virtual bool ready()
  {
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    if ( ! ( td -> dots_immolate -> ticking ) )
      return false;

    return warlock_spell_t::ready();
  }
};

// Incinerate Spell =========================================================

struct incinerate_t : public warlock_spell_t
{
  spell_t*  incinerate_burst_immolate;

  incinerate_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "incinerate", p, "Incinerate" ),
    incinerate_burst_immolate( 0 )
  {
    parse_options( NULL, options_str );

    base_multiplier   *= 1.0 + ( p -> talent_shadow_and_flame -> effect2().percent() );
    base_execute_time += p -> talent_emberstorm -> effect3().time_value();
    base_multiplier   *= 1.0 + ( p -> glyphs.incinerate -> base_value() );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new incinerate_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    if ( td -> dots_immolate -> ticking )
    {
      base_dd_adder = ( sim -> range( base_dd_min, base_dd_max ) + direct_power_mod * total_power() ) / 6;
    }

    warlock_spell_t::execute();

    for ( int i=0; i < 4; i++ )
    {
      p -> uptimes_backdraft[ i ] -> update( i == p -> buffs.backdraft -> check() );
    }

    if ( p -> buffs.backdraft -> check() )
    {
      p -> buffs.backdraft -> decrement();
    }
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      trigger_decimation( this, impact_result );
      trigger_impending_doom( this );
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    if ( p -> buffs.molten_core -> check() )
    {
      player_multiplier *= 1 + p -> buffs.molten_core -> effect1().percent();
      p -> buffs.molten_core -> decrement();
    }
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    if ( td -> dots_immolate -> ticking )
    {
      player_multiplier *= 1 + p -> talent_fire_and_brimstone -> effect1().percent();
    }
  }

  virtual timespan_t execute_time() const
  {
    warlock_t* p = player -> cast_warlock();
    timespan_t h = warlock_spell_t::execute_time();

    if ( p -> buffs.molten_core -> up() )
    {
      h *= 1.0 + p -> buffs.molten_core -> effect3().percent();
    }
    if ( p -> buffs.backdraft -> up() )
    {
      h *= 1.0 + p -> buffs.backdraft -> effect1().percent();
    }

    return h;
  }
};

// Searing Pain Spell =======================================================

struct searing_pain_t : public warlock_spell_t
{
  searing_pain_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "searing_pain", p, "Searing Pain" )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new searing_pain_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    if ( target -> health_percentage() <= 25 && p -> talent_improved_searing_pain -> rank() )
    {
      player_crit += p -> talent_improved_searing_pain -> effect1().percent();
    }

    if ( p -> buffs.soulburn -> up() )
      player_crit +=  p -> buffs.soulburn -> effect3().percent();

    if ( p -> buffs.searing_pain_soulburn -> up() )
      player_crit += p -> buffs.searing_pain_soulburn -> effect1().percent();
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.soulburn -> check() )
    {
      p -> buffs.soulburn -> expire();
      p -> buffs.searing_pain_soulburn -> trigger();
    }
  }
};

// Soul Fire Spell ==========================================================

struct soul_fire_t : public warlock_spell_t
{
  soul_fire_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "soul_fire", p, "Soul Fire" )
  {
    parse_options( NULL, options_str );

    base_execute_time += p -> talent_emberstorm -> effect1().time_value();

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new soul_fire_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.empowered_imp -> check() )
      p -> buffs.empowered_imp -> expire();
    else if ( p -> buffs.soulburn -> check() )
    {
      p -> buffs.soulburn -> expire();
      if ( p -> set_bonus.tier13_4pc_caster() )
      {
        p -> resource_gain( RESOURCE_SOUL_SHARD, 1, p -> gains.tier13_4pc );
      }
    }
  }

  virtual timespan_t execute_time() const
  {
    warlock_t* p = player -> cast_warlock();
    timespan_t t = warlock_spell_t::execute_time();

    if ( p -> buffs.decimation -> up() )
    {
      t *= 1.0 - p -> talent_decimation -> effect1().percent();
    }
    if ( p -> buffs.empowered_imp -> up() )
    {
      t = timespan_t::zero();
    }
    if ( p -> buffs.soulburn -> up() )
    {
      t = timespan_t::zero();
    }

    return t;
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_t* p = player -> cast_warlock();

      trigger_decimation( this, impact_result );

      trigger_impending_doom( this );

      trigger_soul_leech( this );

      warlock_t::trigger_burning_embers( this, travel_dmg );

      p -> buffs.improved_soul_fire -> trigger();
    }
  }
};

// Life Tap Spell ===========================================================

struct life_tap_t : public warlock_spell_t
{
  double trigger;
  double max_mana_pct;

  life_tap_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "life_tap", p, "Life Tap" ),
    trigger( 0 ), max_mana_pct( 0 )
  {
    option_t options[] =
    {
      { "mana_percentage<", OPT_FLT,  &max_mana_pct     },
      { "trigger",          OPT_FLT,  &trigger          },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;

    if ( p -> glyphs.life_tap -> ok() )
      trigger_gcd += p -> glyphs.life_tap -> effect1().time_value();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    double life = p -> resources.max[ RESOURCE_HEALTH ] * effect3().percent();
    double mana = life * effect2().percent() * ( 1.0 + p -> talent_improved_life_tap -> base_value() / 100.0 );
    p -> resource_loss( RESOURCE_HEALTH, life );
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains.life_tap );
    if ( p -> talent_mana_feed -> rank() && p -> active_pet )
    {
      p -> active_pet -> resource_gain( RESOURCE_MANA, mana * p -> talent_mana_feed -> effect1().percent(), p -> active_pet -> gains_mana_feed );
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if (  max_mana_pct > 0 )
      if ( ( 100.0 * p -> resources.current[ RESOURCE_MANA ] / p -> resources.max[ RESOURCE_MANA ] ) > max_mana_pct )
        return false;

    if ( trigger > 0 )
      if ( p -> resources.current[ RESOURCE_MANA ] > trigger )
        return false;

    return warlock_spell_t::ready();
  }
};

// Demon Armor ==============================================================

struct demon_armor_t : public warlock_spell_t
{
  demon_armor_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "demon_armor", p, "Demon Armor" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    warlock_spell_t::execute();

    p -> buffs.demon_armor -> trigger( 1, effect2().percent() + p -> talent_demonic_aegis -> effect1().percent() );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.demon_armor -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Fel Armor Spell ==========================================================

struct fel_armor_t : public warlock_spell_t
{
  double bonus_spell_power;

  fel_armor_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "fel_armor", p, "Fel Armor" ), bonus_spell_power( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;

    bonus_spell_power = p -> buffs.fel_armor -> effect_min( 1 );

    // Model the passive health tick.....
    // FIXME: For PTR need to add new healing mechanic
#if 0
    if ( ! p -> dbc.ptr )
    {
      base_tick_time = effect_period( 2 );
      num_ticks = 1;
    }
#endif
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    p -> buffs.fel_armor -> trigger( 1, bonus_spell_power );

    warlock_spell_t::execute();
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();

    d -> current_tick = 0; // ticks indefinitely

    p -> resource_gain( RESOURCE_HEALTH,
                        p -> resources.max[ RESOURCE_HEALTH ] * effect2().percent() * ( 1.0 + p -> talent_demonic_aegis -> effect1().percent() ),
                        p -> gains.fel_armor, this );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.fel_armor -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Summon Pet Spell =========================================================

struct summon_pet_t : public warlock_spell_t
{
  timespan_t summoning_duration;
  pet_t* pet;

private:
  void _init_summon_pet_t( const std::string& options_str, const char* pet_name )
  {
    parse_options( NULL, options_str );

    warlock_t* p = player -> cast_warlock();
    harmful = false;
    base_execute_time += p -> talent_master_summoner -> effect1().time_value();
    base_costs[ current_resource() ]         *= 1.0 + p -> talent_master_summoner -> effect2().percent();

    pet = p -> find_pet( pet_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p -> name(), pet_name );
      sim -> cancel();
    }
  }

public:
  summon_pet_t( const char* n, warlock_t* p, const char* sname, const std::string& options_str="" ) :
    warlock_spell_t( n, p, sname ), summoning_duration ( timespan_t::zero() ), pet( 0 )
  {
    _init_summon_pet_t( options_str, n );
  }

  summon_pet_t( const char* n, warlock_t* p, int id, const std::string& options_str="" ) :
    warlock_spell_t( n, p, id ), summoning_duration ( timespan_t::zero() ), pet( 0 )
  {
    _init_summon_pet_t( options_str, n );
  }

  virtual void execute()
  {
    pet -> summon( summoning_duration );

    warlock_spell_t::execute();
  }
};

// Summon Main Pet Spell ====================================================

struct summon_main_pet_t : public summon_pet_t
{

  summon_main_pet_t( const char* n, warlock_t* p, const char* sname, const std::string& options_str ) :
    summon_pet_t( n, p, sname, options_str )
  { }

  virtual void schedule_execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::schedule_execute();

    if ( p -> active_pet )
      p -> active_pet -> dismiss();
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> active_pet == pet )
      return false;

    return summon_pet_t::ready();
  }

  virtual timespan_t execute_time() const
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.soulburn -> up() )
      return timespan_t::zero();

    return warlock_spell_t::execute_time();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    summon_pet_t::execute();
    if ( p -> buffs.soulburn -> check() )
      p -> buffs.soulburn -> expire();
  }
};

struct summon_felhunter_t : public summon_main_pet_t
{
  summon_felhunter_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "felhunter", p, "Summon Felhunter", options_str )
  { }
};

struct summon_felguard_t : public summon_main_pet_t
{
  summon_felguard_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "felguard", p, "Summon Felguard", options_str )
  {
    check_talent( p -> talent_summon_felguard -> ok() );
  }
};

struct summon_succubus_t : public summon_main_pet_t
{
  summon_succubus_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "succubus", p, "Summon Succubus", options_str )
  { }
};

struct summon_imp_t : public summon_main_pet_t
{
  summon_imp_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "imp", p, "Summon Imp", options_str )
  { }
};

struct summon_voidwalker_t : public summon_main_pet_t
{
  summon_voidwalker_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "voidwalker", p, "Summon Voidwalker", options_str )
  { }
};

// Infernal Awakening =======================================================

struct infernal_awakening_t : public warlock_spell_t
{
  infernal_awakening_t( warlock_t* p ) :
    warlock_spell_t( "Infernal_Awakening", p, 22703 )
  {
    aoe        = -1;
    background = true;
    proc       = true;
    trigger_gcd= timespan_t::zero();
  }
};

// Summon Infernal Spell ====================================================

struct summon_infernal_t : public summon_pet_t
{
  infernal_awakening_t* infernal_awakening;

  summon_infernal_t( warlock_t* p, const std::string& options_str  ) :
    summon_pet_t( "infernal", p, "Summon Infernal", options_str ),
    infernal_awakening( 0 )
  {
    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? timespan_t::from_millis( p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 3 ) ) : timespan_t::zero();

    summoning_duration = ( duration() + p -> talent_ancient_grimoire -> effect1().time_value() );
    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ?
                          ( p -> talent_summon_felguard -> ok() ?
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 1 ) ) :
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 2 ) )
                          ) : timespan_t::zero();
    infernal_awakening = new infernal_awakening_t( p );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    if ( infernal_awakening )
      infernal_awakening -> execute();

    p -> cooldowns.doomguard -> start();

    summon_pet_t::execute();
  }
};

// Summon Doomguard2 Spell ==================================================

struct summon_doomguard2_t : public summon_pet_t
{
  summon_doomguard2_t( warlock_t* p ) :
    summon_pet_t( "doomguard", p, 60478 )
  {
    harmful = false;
    background = true;
    summoning_duration = ( duration() + p -> talent_ancient_grimoire -> effect1().time_value() );
    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ?
                          ( p -> talent_summon_felguard -> ok() ?
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 1 ) ) :
                            timespan_t::from_seconds( p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 2 ) )
                          ) : timespan_t::zero();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    p -> cooldowns.infernal -> start();

    summon_pet_t::execute();
  }
};

// Summon Doomguard Spell ===================================================

struct summon_doomguard_t : public warlock_spell_t
{
  summon_doomguard2_t* summon_doomguard2;

  summon_doomguard_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "summon_doomguard", p, "Summon Doomguard" ),
    summon_doomguard2( 0 )
  {
    parse_options( NULL, options_str );

    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? timespan_t::from_millis( p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 3 ) ) : timespan_t::zero();

    harmful = false;
    summon_doomguard2 = new summon_doomguard2_t( p );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    consume_resource();
    update_ready();

    p -> cooldowns.infernal -> start();

    summon_doomguard2 -> execute();
  }
};

// Immolation Damage Spell ==================================================

struct immolation_damage_t : public warlock_spell_t
{
  immolation_damage_t( warlock_t* p ) :
    warlock_spell_t( "immolation_dmg", p, 50590 )
  {
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;
    may_crit    = false;
  }
};

// Immolation Aura Spell ====================================================

struct immolation_aura_t : public warlock_spell_t
{
  immolation_damage_t* immolation_damage;

  immolation_aura_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "immolation_aura", p, "Immolation Aura" ),
    immolation_damage( 0 )
  {
    parse_options( NULL, options_str );

    harmful = true;
    tick_may_crit = false;
    immolation_damage = new immolation_damage_t( p );
  }

  virtual void init()
  {
    warlock_spell_t::init();

    immolation_damage -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.metamorphosis -> check() )
    {
      immolation_damage -> execute();
    }
    else
    {
      // Cancel the aura
      d -> current_tick = dot() -> num_ticks;
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! p -> buffs.metamorphosis -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Metamorphosis Spell ======================================================

struct metamorphosis_t : public warlock_spell_t
{
  metamorphosis_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "metamorphosis", p, "Metamorphosis" )
  {
    check_talent( p -> talent_metamorphosis -> rank() );

    parse_options( NULL, options_str );

    trigger_gcd = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    p -> buffs.metamorphosis -> trigger( 1, p -> composite_mastery() );
  }
};

// Demonic Empowerment Spell ================================================

struct demonic_empowerment_t : public warlock_spell_t
{
  demonic_empowerment_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "demonic_empowerment", p, "Demonic Empowerment" )
  {
    check_talent( p -> talent_demonic_empowerment -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    if ( p -> active_pet -> pet_type == PET_FELGUARD )
      p -> active_pet -> buffs.stunned -> expire();
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! p -> active_pet )
      return false;

    return warlock_spell_t::ready();
  }
};

// Hand of Gul'dan Spell ====================================================

struct hand_of_guldan_t : public warlock_spell_t
{
  hand_of_guldan_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "hand_of_guldan", p, "Hand of Gul'dan" )
  {
    check_talent( p -> talent_hand_of_guldan -> rank() );

    parse_options( NULL, options_str );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new hand_of_guldan_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();

      if ( t -> debuffs.flying -> check() )
      {
        if ( sim -> debug ) log_t::output( sim, "%s can not apply its debuff to flying target %s", name(), t -> name_str.c_str() );
      }
      else
      {
        p -> buffs.hand_of_guldan -> trigger();
      }

      trigger_impending_doom( this );

      if ( td -> dots_immolate -> ticking && p -> talent_cremation -> rank() )
      {
        if ( p -> rngs.cremation -> roll( p -> talent_cremation -> proc_chance() ) )
        {
          td -> dots_immolate -> refresh_duration();
        }
      }
    }
  }

  virtual timespan_t travel_time()
  {
    return timespan_t::from_seconds( 0.2 );
  }
};

// Fel Flame Spell ==========================================================

struct fel_flame_t : public warlock_spell_t
{
  fel_flame_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "fel_flame", p, "Fel Flame" )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new fel_flame_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t,  result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      td -> dots_immolate            -> extend_duration( 2, true );
      td -> dots_unstable_affliction -> extend_duration( 2, true );
    }
  }
};

// Dark Intent Spell ========================================================

struct dark_intent_t : public warlock_spell_t
{
  dark_intent_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "dark_intent", p, "Dark Intent" )
  {
    parse_options( 0, options_str );

    harmful = false;
    background = ( sim -> overrides.spell_power_multiplier != 0 );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    
    if ( ! sim -> overrides.spell_power_multiplier )
      sim -> auras.spell_power_multiplier -> trigger();
  }
};

// Soulburn Spell ===========================================================

struct soulburn_t : public warlock_spell_t
{
  soulburn_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "soulburn", p, "Soulburn" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> use_pre_soulburn || p -> in_combat )
    {
      p -> buffs.soulburn -> trigger();
      p -> buffs.tier13_4pc_caster -> trigger();
      // If this was a pre-combat soulburn, ensure we model the 3 seconds needed to regenerate the soul shard
      if ( ! p -> in_combat )
      {
        p -> buffs.soulburn -> extend_duration( p, timespan_t::from_seconds( -3 ) );
        if ( p -> buffs.tier13_4pc_caster -> check() ) p -> buffs.tier13_4pc_caster -> extend_duration( p, timespan_t::from_seconds( -3 ) );
      }
    }

    warlock_spell_t::execute();
  }
};

// Demon Soul Spell =========================================================

struct demon_soul_t : public warlock_spell_t
{
  demon_soul_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "demon_soul", p, "Demon Soul" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    assert ( p -> active_pet );

    if ( p -> active_pet -> pet_type == PET_IMP )
    {
      p -> buffs.demon_soul_imp -> trigger();
    }
    if ( p -> active_pet -> pet_type == PET_FELGUARD )
    {
      p -> buffs.demon_soul_felguard -> trigger();
    }
    if ( p -> active_pet -> pet_type == PET_FELHUNTER )
    {
      p -> buffs.demon_soul_felhunter -> trigger();
    }
    if ( p -> active_pet -> pet_type == PET_SUCCUBUS )
    {
      p -> buffs.demon_soul_succubus -> trigger();
    }
    if ( p -> active_pet -> pet_type == PET_VOIDWALKER )
    {
      p -> buffs.demon_soul_voidwalker -> trigger();
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! p ->  active_pet )
      return false;

    return warlock_spell_t::ready();
  }
};

// Hellfire Effect Spell ====================================================

struct hellfire_tick_t : public warlock_spell_t
{
  hellfire_tick_t( warlock_t* p ) :
    warlock_spell_t( "hellfire_tick", p, 5857 )
  {
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;

    base_multiplier *= 1.0 + p -> talent_cremation -> effect1().percent();
  }
};

// Hellfire Spell ===========================================================

struct hellfire_t : public warlock_spell_t
{
  hellfire_tick_t* hellfire_tick;

  hellfire_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "hellfire", p, 1949 ), hellfire_tick( 0 )
  {
    parse_options( NULL, options_str );

    // Hellfire has it's own damage effect, which is actually the damage to the player himself, so harmful is set to false.
    harmful = false;

    channeled    = true;
    hasted_ticks = false;

    hellfire_tick = new hellfire_tick_t( p );

    if ( p -> talent_inferno -> rank() )
    {
      range += p -> talent_inferno -> effect1().base_value();
    }
  }

  virtual void init()
  {
    warlock_spell_t::init();

    hellfire_tick -> stats = stats;
  }

  virtual bool usable_moving()
  {
    warlock_t* p = player -> cast_warlock();

    return p -> talent_inferno -> rank() > 0;
  }

  virtual void tick( dot_t* /* d */ )
  {
    hellfire_tick -> execute();
  }
};
// Seed of Corruption AOE Spell =============================================

struct seed_of_corruption_aoe_t : public warlock_spell_t
{
  seed_of_corruption_aoe_t( warlock_t* p ) :
    warlock_spell_t( "seed_of_corruption_aoe", p, 27285 )
  {
    proc       = true;
    background = true;
    aoe        = -1;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.soulburn -> check() && p -> talent_soulburn_seed_of_corruption -> rank() )
    {
      // Trigger Multiple Corruptions
      p -> buffs.soulburn -> expire();
    }
  }
};

// Seed of Corruption Spell =================================================

struct seed_of_corruption_t : public warlock_spell_t
{
  seed_of_corruption_aoe_t* seed_of_corruption_aoe;
  double dot_damage_done;

  seed_of_corruption_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "seed_of_corruption", p, "Seed of Corruption" ),
    seed_of_corruption_aoe( 0 ), dot_damage_done( 0 )
  {
    parse_options( NULL, options_str );

    seed_of_corruption_aoe = new seed_of_corruption_aoe_t( p );
    add_child( seed_of_corruption_aoe );

    base_crit += p -> talent_everlasting_affliction -> effect2().percent();
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      dot_damage_done = t -> iteration_dmg_taken;
      if ( td -> dots_corruption -> ticking )
      {
        td -> dots_corruption -> cancel();
      }
    }
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( target -> iteration_dmg_taken - dot_damage_done > effect2().base_value() )
    {
      dot_damage_done=0.0;
      seed_of_corruption_aoe -> execute();
      spell_t::cancel();
    }
  }
};

// Rain of Fire Tick Spell ==================================================

struct rain_of_fire_tick_t : public warlock_spell_t
{
  rain_of_fire_tick_t( warlock_t* p ) :
    warlock_spell_t( "rain_of_fire_tick", p, 42223 )
  {
    background  = true;
    aoe         = -1;
    direct_tick = true;
  }
};

// Rain of Fire Spell =======================================================

struct rain_of_fire_t : public warlock_spell_t
{
  rain_of_fire_tick_t* rain_of_fire_tick;

  rain_of_fire_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "rain_of_fire", p, "Rain of Fire" ),
    rain_of_fire_tick( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;
    channeled = true;

    rain_of_fire_tick = new rain_of_fire_tick_t( p );

    add_child( rain_of_fire_tick );
  }

  virtual void init()
  {
    warlock_spell_t::init();

    rain_of_fire_tick -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    rain_of_fire_tick -> execute();
  }
};


} // ANONYMOUS NAMESPACE ====================================================

// trigger_mana_feed ========================================================

void warlock_t::trigger_mana_feed( action_t* s, double impact_result )
{
  warlock_pet_t* a = ( warlock_pet_t* ) s -> player -> cast_pet();
  warlock_t* p = a -> owner -> cast_warlock();

  if ( p -> talent_mana_feed -> rank() )
  {
    if ( impact_result == RESULT_CRIT )
    {
      double mana = p -> resources.max[ RESOURCE_MANA ] * p -> talent_mana_feed -> effect3().percent();
      if ( p -> active_pet -> pet_type == PET_FELGUARD || p -> active_pet -> pet_type == PET_FELHUNTER ) mana *= 4;
      p -> resource_gain( RESOURCE_MANA, mana, p -> gains.mana_feed );
      a -> procs_mana_feed -> occur();
    }
  }
}


// Trigger Burning Embers ===================================================

void warlock_t::trigger_burning_embers ( spell_t* s, double dmg )
{
  warlock_t* p;
  if ( s -> player -> type == WARLOCK )
  {
    p = s -> player -> cast_warlock();
  }
  else
  {
    pet_t* a = ( pet_t* ) s -> player -> cast_pet();
    p = a -> owner -> cast_warlock();
  }

  if ( p -> talent_burning_embers -> rank() )
  {
    struct burning_embers_t : public warlock_spell_t
    {
      burning_embers_t( warlock_t* p ) :
        warlock_spell_t( "burning_embers", p, 85421 )
      {
        background = true;
        tick_may_crit = false;
        hasted_ticks = false;
        may_trigger_dtr = false;
        init();
      }

      virtual double calculate_tick_damage( result_type_e, double, double ) { return base_td; }
    };

    if ( ! p -> spells_burning_embers ) p -> spells_burning_embers = new burning_embers_t( p );

    if ( ! p -> spells_burning_embers -> dot() -> ticking ) p -> spells_burning_embers -> base_td = 0;

    int num_ticks = p -> spells_burning_embers -> num_ticks;

    double spmod = 0.7;

    //FIXME: The 1.2 modifier to the adder was experimentally observed on live realms 2011/10/14
    double cap = ( spmod * p -> talent_burning_embers -> rank() * p -> composite_spell_power( SCHOOL_FIRE ) * p -> composite_spell_power_multiplier() + p -> talent_burning_embers -> effect_min( 2 ) * 1.2 ) / num_ticks;

    p -> spells_burning_embers -> base_td += ( dmg * p -> talent_burning_embers -> effect1().percent() ) / num_ticks;

    if ( p -> spells_burning_embers -> base_td > cap ) p -> spells_burning_embers -> base_td = cap;

    p -> spells_burning_embers -> execute();
  }
}


// ==========================================================================
// Warlock Character Definition
// ==========================================================================

// warlock_t::composite_armor ===============================================

double warlock_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buffs.demon_armor -> up() )
    a += buffs.demon_armor ->effect1().base_value();

  return a;
}

// warlock_t::composite_spell_power =========================================

double warlock_t::composite_spell_power( const school_type_e school ) const
{
  double sp = player_t::composite_spell_power( school );

  sp += buffs.fel_armor -> value();

  return sp;
}

// warlock_t::composite_spell_power_multiplier ==============================

double warlock_t::composite_spell_power_multiplier() const
{
  double m = player_t::composite_spell_power_multiplier();

  if ( buffs.tier13_4pc_caster -> up() )
  {
    m *= 1.0 + dbc.spell( sets -> set ( SET_T13_4PC_CASTER ) -> effect_trigger_spell( 1 ) ) -> effect1().percent();
  }

  return m;
}

// warlock_t::composite_player_multiplier ===================================

double warlock_t::composite_player_multiplier( const school_type_e school, action_t* a ) const
{
  double player_multiplier = player_t::composite_player_multiplier( school, a );

  double mastery_value = mastery_spells.master_demonologist -> effect_base_value( 3 );

  if ( buffs.metamorphosis -> up() )
  {
    player_multiplier *= 1.0 + buffs.metamorphosis -> effect3().percent()
                         + ( buffs.metamorphosis -> value() * mastery_value / 10000.0 );
  }

  player_multiplier *= 1.0 + ( talent_demonic_pact -> effect3().percent() );

  if ( ( school == SCHOOL_FIRE || school == SCHOOL_SHADOW ) && buffs.demon_soul_felguard -> up() )
  {
    player_multiplier *= 1.0 + buffs.demon_soul_felguard -> effect2().percent();
  }

  double fire_multiplier   = 1.0;
  double shadow_multiplier = 1.0;

  // Fire
  fire_multiplier *= 1.0 + ( passive_spells.cataclysm -> effect_base_value( 1 ) * 0.01 );
  if ( mastery_spells.fiery_apocalypse -> ok() )
    fire_multiplier *= 1.0 + ( composite_mastery() * mastery_spells.fiery_apocalypse -> effect_base_value( 2 ) / 10000.0 );
  fire_multiplier *= 1.0 + passive_spells.demonic_knowledge -> effect_base_value( 1 ) * 0.01 ;

  // Shadow
  shadow_multiplier *= 1.0 + ( passive_spells.demonic_knowledge -> effect_base_value( 1 ) * 0.01 );
  shadow_multiplier *= 1.0 + ( passive_spells.shadow_mastery -> effect_base_value( 1 ) * 0.01 );

  if ( buffs.improved_soul_fire -> up() )
  {
    fire_multiplier *= 1.0 + talent_improved_soul_fire -> rank() * 0.04;
    shadow_multiplier *= 1.0 + talent_improved_soul_fire -> rank() * 0.04;
  }

  if ( school == SCHOOL_FIRE )
    player_multiplier *= fire_multiplier;
  else if ( school == SCHOOL_SHADOW )
    player_multiplier *= shadow_multiplier;
  else if ( school == SCHOOL_SHADOWFLAME )
  {
    if ( fire_multiplier > shadow_multiplier )
      player_multiplier *= fire_multiplier;
    else
      player_multiplier *= shadow_multiplier;
  }

  return player_multiplier;
}

// warlock_t::composite_player_td_multiplier ================================

double warlock_t::composite_player_td_multiplier( const school_type_e school, action_t* a ) const
{
  double player_multiplier = player_t::composite_player_td_multiplier( school, a );

  if ( school == SCHOOL_SHADOW || school == SCHOOL_SHADOWFLAME )
  {
    // Shadow TD
    if ( mastery_spells.potent_afflictions -> ok() )
    {
      player_multiplier += floor ( ( composite_mastery() * mastery_spells.potent_afflictions -> effect_base_value( 2 ) / 10000.0 ) * 1000 ) / 1000;
    }
    if ( buffs.demon_soul_felhunter -> up() )
    {
      player_multiplier += buffs.demon_soul_felhunter -> effect1().percent();
    }
  }

  return player_multiplier;
}

// warlock_t::matching_gear_multiplier ======================================

double warlock_t::matching_gear_multiplier( const attribute_type_e attr ) const
{
  if ( ( attr == ATTR_INTELLECT ) && passive_spells.nethermancy -> ok() )
    return ( passive_spells.nethermancy -> effect_base_value( 1 ) * 0.01 );

  return 0.0;
}

// warlock_t::create_action =================================================

action_t* warlock_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "chaos_bolt"          ) return new          chaos_bolt_t( this, options_str );
  if ( name == "conflagrate"         ) return new         conflagrate_t( this, options_str );
  if ( name == "corruption"          ) return new          corruption_t( this, options_str );
  if ( name == "bane_of_agony"       ) return new       bane_of_agony_t( this, options_str );
  if ( name == "bane_of_doom"        ) return new        bane_of_doom_t( this, options_str );
  if ( name == "curse_of_elements"   ) return new   curse_of_elements_t( this, options_str );
  if ( name == "death_coil"          ) return new          death_coil_t( this, options_str );
  if ( name == "demon_armor"         ) return new         demon_armor_t( this, options_str );
  if ( name == "demonic_empowerment" ) return new demonic_empowerment_t( this, options_str );
  if ( name == "drain_life"          ) return new          drain_life_t( this, options_str );
  if ( name == "drain_soul"          ) return new          drain_soul_t( this, options_str );
  if ( name == "fel_armor"           ) return new           fel_armor_t( this, options_str );
  if ( name == "haunt"               ) return new               haunt_t( this, options_str );
  if ( name == "immolate"            ) return new            immolate_t( this, options_str );
  if ( name == "immolation_aura"     ) return new     immolation_aura_t( this, options_str );
  if ( name == "shadowflame"         ) return new         shadowflame_t( this, options_str );
  if ( name == "incinerate"          ) return new          incinerate_t( this, options_str );
  if ( name == "life_tap"            ) return new            life_tap_t( this, options_str );
  if ( name == "metamorphosis"       ) return new       metamorphosis_t( this, options_str );
  if ( name == "shadow_bolt"         ) return new         shadow_bolt_t( this, options_str );
  if ( name == "shadowburn"          ) return new          shadowburn_t( this, options_str );
  if ( name == "shadowfury"          ) return new          shadowfury_t( this, options_str );
  if ( name == "searing_pain"        ) return new        searing_pain_t( this, options_str );
  if ( name == "soul_fire"           ) return new           soul_fire_t( this, options_str );
  if ( name == "summon_felhunter"    ) return new    summon_felhunter_t( this, options_str );
  if ( name == "summon_felguard"     ) return new     summon_felguard_t( this, options_str );
  if ( name == "summon_succubus"     ) return new     summon_succubus_t( this, options_str );
  if ( name == "summon_voidwalker"   ) return new   summon_voidwalker_t( this, options_str );
  if ( name == "summon_imp"          ) return new          summon_imp_t( this, options_str );
  if ( name == "summon_infernal"     ) return new     summon_infernal_t( this, options_str );
  if ( name == "summon_doomguard"    ) return new    summon_doomguard_t( this, options_str );
  if ( name == "unstable_affliction" ) return new unstable_affliction_t( this, options_str );
  if ( name == "hand_of_guldan"      ) return new      hand_of_guldan_t( this, options_str );
  if ( name == "fel_flame"           ) return new           fel_flame_t( this, options_str );
  if ( name == "dark_intent"         ) return new         dark_intent_t( this, options_str );
  if ( name == "soulburn"            ) return new            soulburn_t( this, options_str );
  if ( name == "demon_soul"          ) return new          demon_soul_t( this, options_str );
  if ( name == "bane_of_havoc"       ) return new       bane_of_havoc_t( this, options_str );
  if ( name == "hellfire"            ) return new            hellfire_t( this, options_str );
  if ( name == "seed_of_corruption"  ) return new  seed_of_corruption_t( this, options_str );
  if ( name == "rain_of_fire"        ) return new        rain_of_fire_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// warlock_t::create_pet ====================================================

pet_t* warlock_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "felguard"     ) return new    felguard_pet_t( sim, this );
  if ( pet_name == "felhunter"    ) return new   felhunter_pet_t( sim, this );
  if ( pet_name == "imp"          ) return new         imp_pet_t( sim, this );
  if ( pet_name == "succubus"     ) return new    succubus_pet_t( sim, this );
  if ( pet_name == "voidwalker"   ) return new  voidwalker_pet_t( sim, this );
  if ( pet_name == "infernal"     ) return new    infernal_pet_t( sim, this );
  if ( pet_name == "doomguard"    ) return new   doomguard_pet_t( sim, this );
  if ( pet_name == "ebon_imp"     ) return new    ebon_imp_pet_t( sim, this );

  return 0;
}

// warlock_t::create_pets ===================================================

void warlock_t::create_pets()
{
  create_pet( "felguard"  );
  create_pet( "felhunter" );
  create_pet( "imp"       );
  create_pet( "succubus"  );
  create_pet( "voidwalker" );
  create_pet( "infernal"  );
  create_pet( "doomguard" );
  pet_ebon_imp = create_pet( "ebon_imp"  );
}

// warlock_t::init_talents ==================================================

void warlock_t::init_talents()
{
  // Affliction
  talent_unstable_affliction          = new spell_id_t( this, "unstable_affliction", "Unstable Affliction" );
  talent_doom_and_gloom               = find_talent( "Doom and Gloom" );
  talent_improved_life_tap            = find_talent( "Improved Life Tap" );
  talent_improved_corruption          = find_talent( "Improved Corruption" );
  talent_jinx                         = find_talent( "Jinx" );
  talent_soul_siphon                  = find_talent( "Soul Siphon" );
  talent_siphon_life                  = find_talent( "Siphon Life" );
  talent_eradication                  = find_talent( "Eradication" );
  talent_soul_swap                    = find_talent( "Soul Swap" );
  talent_shadow_embrace               = find_talent( "Shadow Embrace" );
  talent_deaths_embrace               = find_talent( "Death's Embrace" );
  talent_nightfall                    = find_talent( "Nightfall" );
  talent_soulburn_seed_of_corruption  = find_talent( "Soulburn: Seed of Corruption" );
  talent_everlasting_affliction       = find_talent( "Everlasting Affliction" );
  talent_pandemic                     = find_talent( "Pandemic" );
  talent_haunt                        = find_talent( "Haunt" );

  // Demonology
  talent_summon_felguard      = new spell_id_t( this, "summon_felguard", "Summon Felguard" );
  talent_demonic_embrace      = find_talent( "Demonic Embrace" );
  talent_dark_arts            = find_talent( "Dark Arts" );
  talent_mana_feed            = find_talent( "Mana Feed" );
  talent_demonic_aegis        = find_talent( "Demonic Aegis" );
  talent_master_summoner      = find_talent( "Master Summoner" );
  talent_impending_doom       = find_talent( "Impending Doom" );
  talent_demonic_empowerment  = find_talent( "Demonic Empowerment" );
  talent_molten_core          = find_talent( "Molten Core" );
  talent_hand_of_guldan       = find_talent( "Hand of Gul'dan" );
  talent_aura_of_foreboding   = find_talent( "Aura of Foreboding" );
  talent_ancient_grimoire     = find_talent( "Ancient Grimoire" );
  talent_inferno              = find_talent( "Inferno" );
  talent_decimation           = find_talent( "Decimation" );
  talent_cremation            = find_talent( "Cremation" );
  talent_demonic_pact         = find_talent( "Demonic Pact" );
  talent_metamorphosis        = find_talent( "Metamorphosis" );

  // Destruction
  talent_conflagrate            = new spell_id_t( this, "conflagrate", "Conflagrate" );
  talent_bane                   = find_talent( "Bane" );
  talent_shadow_and_flame       = find_talent( "Shadow and Flame" );
  talent_improved_immolate      = find_talent( "Improved Immolate" );
  talent_improved_soul_fire     = find_talent( "Improved Soul Fire" );
  talent_emberstorm             = find_talent( "Emberstorm" );
  talent_improved_searing_pain  = find_talent( "Improved Searing Pain" );
  talent_backdraft              = find_talent( "Backdraft" );
  talent_shadowburn             = find_talent( "Shadowburn" );
  talent_burning_embers         = find_talent( "Burning Embers" );
  talent_soul_leech             = find_talent( "Soul Leech" );
  talent_fire_and_brimstone     = find_talent( "Fire and Brimstone" );
  talent_shadowfury             = find_talent( "Shadowfury" );
  talent_empowered_imp          = find_talent( "Empowered Imp" );
  talent_bane_of_havoc          = find_talent( "Bane of Havoc" );
  talent_chaos_bolt             = find_talent( "Chaos Bolt" );

  player_t::init_talents();
}

// warlock_t::init_spells ===================================================

void warlock_t::init_spells()
{
  player_t::init_spells();

  // New set bonus system
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    { 105888, 105787,     0,     0,     0,     0,     0,     0 }, // Tier13
    {      0,      0,     0,     0,     0,     0,     0,     0 },
  };
  sets                        = new set_bonus_array_t( this, set_bonuses );

  // passive_spells =========================================================

  // Core
  passive_spells.shadow_mastery       = new spell_id_t( this, "shadow_mastery", "Shadow Mastery" );
  passive_spells.demonic_knowledge    = new spell_id_t( this, "demonic_knowledge", "Demonic Knowledge" );
  passive_spells.cataclysm            = new spell_id_t( this, "cataclysm", "Cataclysm" );
  passive_spells.nethermancy          = new spell_id_t( this, "nethermancy", 86091 );

  //Affliction
  passive_spells.doom_and_gloom       = new spell_id_t( this, "doom_and_gloom", "Doom and Gloom", talent_doom_and_gloom );
  passive_spells.pandemic             = new spell_id_t( this, "pandemic", "Pandemic", talent_pandemic );

  // Mastery
  mastery_spells.fiery_apocalypse     = new mastery_t( this, "fiery_apocalypse", "Fiery Apocalypse", TREE_DESTRUCTION );
  mastery_spells.potent_afflictions   = new mastery_t( this, "potent_afflictions", "Potent Afflictions", TREE_AFFLICTION );
  mastery_spells.master_demonologist  = new mastery_t( this, "master_demonologist", "Master Demonologist", TREE_DEMONOLOGY );

  // Constants
  constants_pandemic_gcd              = timespan_t::from_seconds( 0.25 );

  // Prime
  glyphs.metamorphosis        = find_glyph( "Glyph of Metamorphosis" );
  glyphs.conflagrate          = find_glyph( "Glyph of Conflagrate" );
  glyphs.chaos_bolt           = find_glyph( "Glyph of Chaos Bolt" );
  glyphs.corruption           = find_glyph( "Glyph of Corruption" );
  glyphs.bane_of_agony        = find_glyph( "Glyph of Bane of Agony" );
  glyphs.felguard             = find_glyph( "Glyph of Felguard" );
  glyphs.haunt                = find_glyph( "Glyph of Haunt" );
  glyphs.immolate             = find_glyph( "Glyph of Immolate" );
  glyphs.imp                  = find_glyph( "Glyph of Imp" );
  glyphs.lash_of_pain         = find_glyph( "Glyph of Lash of Pain" );
  glyphs.incinerate           = find_glyph( "Glyph of Incinerate" );
  glyphs.shadowburn           = find_glyph( "Glyph of Shadowburn" );
  glyphs.unstable_affliction  = find_glyph( "Glyph of Unstable Affliction" );

  // Major
  glyphs.life_tap             = find_glyph( "Glyph of Life Tap" );
  glyphs.shadow_bolt          = find_glyph( "Glyph of Shadow Bolt" );
}

// warlock_t::init_base =====================================================

void warlock_t::init_base()
{
  player_t::init_base();

  attribute_multiplier_initial[ ATTR_STAMINA ] *= 1.0 + talent_demonic_embrace -> effect1().percent();

  base_attack_power = -10;
  initial_attack_power_per_strength = 2.0;
  initial_spell_power_per_intellect = 1.0;

  mana_per_intellect = 15;

  resources.base[ RESOURCE_SOUL_SHARD ] = 3;

  diminished_kfactor    = 0.009830;
  diminished_dodge_capi = 0.006650;
  diminished_parry_capi = 0.006650;
}

// warlock_t::init_scaling ==================================================

void warlock_t::init_scaling()
{
  player_t::init_scaling();
  scales_with[ STAT_SPIRIT ] = 0;
  scales_with[ STAT_STAMINA ] = 0;
}

// warlock_t::init_buffs ====================================================

void warlock_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.backdraft             = new buff_t( this, talent_backdraft -> effect_trigger_spell( 1 ), "backdraft" );
  buffs.decimation            = new buff_t( this, talent_decimation -> effect_trigger_spell( 1 ), "decimation", talent_decimation -> ok() );
  buffs.demon_armor           = new buff_t( this,   687, "demon_armor" );
  buffs.demonic_empowerment   = new buff_t( this, "demonic_empowerment",   1 );
  buffs.empowered_imp         = new buff_t( this, 47283, "empowered_imp", talent_empowered_imp -> effect1().percent() );
  buffs.eradication           = new buff_t( this, talent_eradication -> effect_trigger_spell( 1 ), "eradication", talent_eradication -> proc_chance() );
  buffs.metamorphosis         = new buff_t( this, 47241, "metamorphosis", talent_metamorphosis -> rank() );
  buffs.metamorphosis -> buff_duration += timespan_t::from_millis( glyphs.metamorphosis -> base_value() );
  buffs.metamorphosis -> cooldown -> duration = timespan_t::zero();
  buffs.molten_core           = new buff_t( this, talent_molten_core -> effect_trigger_spell( 1 ), "molten_core", talent_molten_core -> rank() * 0.02 );
  buffs.shadow_trance         = new buff_t( this, 17941, "shadow_trance", talent_nightfall -> proc_chance() +  glyphs.corruption -> base_value() / 100.0 );

  buffs.hand_of_guldan        = new buff_t( this, "hand_of_guldan",        1, timespan_t::from_seconds( 15.0 ), timespan_t::zero(), talent_hand_of_guldan -> rank() );
  buffs.improved_soul_fire    = new buff_t( this, 85383, "improved_soul_fire", ( talent_improved_soul_fire -> rank() > 0 ) );
  buffs.soulburn              = new buff_t( this, 74434, "soulburn" );
  buffs.demon_soul_imp        = new buff_t( this, 79459, "demon_soul_imp" );
  buffs.demon_soul_imp        -> activated = false;
  buffs.demon_soul_felguard   = new buff_t( this, 79462, "demon_soul_felguard" );
  buffs.demon_soul_felguard   -> activated = false;
  buffs.demon_soul_felhunter  = new buff_t( this, 79460, "demon_soul_felhunter" );
  buffs.demon_soul_felhunter  -> activated = false;
  buffs.demon_soul_succubus   = new buff_t( this, 79463, "demon_soul_succubus" );
  buffs.demon_soul_succubus   -> activated = false;
  buffs.demon_soul_voidwalker = new buff_t( this, 79464, "demon_soul_voidwalker" );
  buffs.demon_soul_voidwalker -> activated = false;
  buffs.bane_of_havoc         = new buff_t( this, 80240, "bane_of_havoc" );
  buffs.searing_pain_soulburn = new buff_t( this, 79440, "searing_pain_soulburn" );
  buffs.fel_armor             = new buff_t( this, "fel_armor", "Fel Armor" );
  buffs.tier13_4pc_caster     = new buff_t( this, sets -> set ( SET_T13_4PC_CASTER ) -> effect_trigger_spell( 1 ), "tier13_4pc_caster", sets -> set ( SET_T13_4PC_CASTER ) -> proc_chance() );
}

// warlock_t::init_values ======================================================

void warlock_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 90;
}

// warlock_t::init_gains ====================================================

void warlock_t::init_gains()
{
  player_t::init_gains();

  gains.fel_armor         = get_gain( "fel_armor"         );
  gains.felhunter         = get_gain( "felhunter"         );
  gains.life_tap          = get_gain( "life_tap"          );
  gains.mana_feed         = get_gain( "mana_feed"         );
  gains.soul_leech        = get_gain( "soul_leech"        );
  gains.soul_leech_health = get_gain( "soul_leech_health" );
  gains.tier13_4pc        = get_gain( "tier13_4pc"        );
}

// warlock_t::init_uptimes ==================================================

void warlock_t::init_benefits()
{
  player_t::init_benefits();

  uptimes_backdraft[ 0 ]  = get_benefit( "backdraft_0" );
  uptimes_backdraft[ 1 ]  = get_benefit( "backdraft_1" );
  uptimes_backdraft[ 2 ]  = get_benefit( "backdraft_2" );
  uptimes_backdraft[ 3 ]  = get_benefit( "backdraft_3" );
}

// warlock_t::init_procs ====================================================

void warlock_t::init_procs()
{
  player_t::init_procs();

  procs.ebon_imp         = get_proc( "ebon_imp"       );
  procs.empowered_imp    = get_proc( "empowered_imp"  );
  procs.impending_doom   = get_proc( "impending_doom" );
  procs.shadow_trance    = get_proc( "shadow_trance"  );
}

// warlock_t::init_rng ======================================================

void warlock_t::init_rng()
{
  player_t::init_rng();

  rngs.cremation               = get_rng( "cremation"              );
  rngs.ebon_imp                = get_rng( "ebon_imp_proc"          );
  rngs.everlasting_affliction  = get_rng( "everlasting_affliction" );
  rngs.impending_doom          = get_rng( "impending_doom"         );
  rngs.pandemic                = get_rng( "pandemic"               );
  rngs.siphon_life             = get_rng( "siphon_life"            );
  rngs.soul_leech              = get_rng( "soul_leech"             );
}

// warlock_t::init_actions ==================================================

void warlock_t::init_actions()
{
  if ( action_list_str.empty() )
  {

    // Trinket check
    bool has_mwc = false;
    bool has_wou = false;
    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ i ];
      if ( strstr( item.name(), "moonwell_chalice" ) )
      {
        has_mwc = true;
      }
      if ( strstr( item.name(), "will_of_unbinding" ) )
      {
        has_wou = true;
      }
    }

    // Flask
    if ( level >= 80 )
      action_list_str = "flask,type=draconic_mind";
    else if ( level >= 75 )
      action_list_str = "flask,type=frost_wyrm";

    // Food
    if ( level >= 80 ) action_list_str += "/food,type=seafood_magnifique_feast";
    else if ( level >= 70 ) action_list_str += "/food,type=fish_feast";

    // Armor
    if ( level >= 62 ) action_list_str += "/fel_armor";

    // Choose Pet
    if ( primary_tree() == TREE_DESTRUCTION )
    {
      action_list_str += "/summon_imp";
    }
    else if ( primary_tree() == TREE_DEMONOLOGY )
    {
      if ( has_mwc )
        action_list_str += "/summon_felguard,if=cooldown.demon_soul.remains<5&cooldown.metamorphosis.remains<5&!pet.felguard.active";
      else
        action_list_str += "/summon_felguard,if=!in_combat";
    }
    else if ( primary_tree() == TREE_AFFLICTION )
    {
      if ( glyphs.lash_of_pain -> ok() )
        action_list_str += "/summon_succubus";
      else if ( glyphs.imp -> ok() )
        action_list_str += "/summon_imp";
      else
        action_list_str += "/summon_felhunter";
    }
    else
      action_list_str += "/summon_imp";

    // Dark Intent
    if ( level >= 83 )
      action_list_str += "/dark_intent,if=!aura.spell_power_multiplier.up";

    // Pre soulburn
    if ( use_pre_soulburn && !set_bonus.tier13_4pc_caster() )
      action_list_str += "/soulburn,if=!in_combat";

    // Snapshot Stats
    action_list_str += "/snapshot_stats";

    // Usable Item
    int num_items = ( int ) items.size();
    for ( int i = num_items - 1; i >= 0; i-- )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
        if ( primary_tree() == TREE_DEMONOLOGY )
          action_list_str += ",if=cooldown.metamorphosis.remains=0|cooldown.metamorphosis.remains>cooldown";
      }
    }

    action_list_str += init_use_profession_actions();
    action_list_str += init_use_racial_actions();

    // Choose Potion
    if ( level >= 80 )
    {
      if ( primary_tree() == TREE_DEMONOLOGY )
        action_list_str += "/volcanic_potion,if=buff.metamorphosis.up|!in_combat";
      else
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react|!in_combat|target.health_pct<=20";
    }
    else if ( level >= 70 )
    {
      if ( primary_tree() == TREE_AFFLICTION )
        action_list_str += "/speed_potion,if=buff.bloodlust.react|!in_combat";
      else
        action_list_str += "/wild_magic_potion,if=buff.bloodlust.react|!in_combat";
    }

    switch ( primary_tree() )
    {

    case TREE_AFFLICTION:
      if ( level >= 85 && ! glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soulburn";
      action_list_str += "/corruption,if=(!ticking|remains<tick_time)&miss_react";
      action_list_str += "/unstable_affliction,if=(!ticking|remains<(cast_time+tick_time))&target.time_to_die>=5&miss_react";
      if ( level >= 12 ) action_list_str += "/bane_of_doom,if=target.time_to_die>15&!ticking&miss_react";
      if ( talent_haunt -> rank() ) action_list_str += "/haunt";
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      if ( talent_soul_siphon -> rank() )
      {
        action_list_str += "/drain_soul,interrupt=1,if=target.health_pct<=25";
        // If the profile has the Will of Unbinding, we need to make sure the stacks don't drop during execute phase
        if ( has_wou )
        {
          // Attempt to account for non-default channel_lag settings
          char delay = ( char ) ( sim -> channel_lag.total_seconds() * 20 + 48 );
          if ( delay > 57 ) delay = 57;
          action_list_str += ",interrupt_if=buff.will_of_unbinding.up&cooldown.haunt.remains<tick_time&buff.will_of_unbinding.remains<action.haunt.cast_time+tick_time+0.";
          action_list_str += delay;
        }
      }
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soul_fire,if=buff.soulburn.up";
      if ( talent_bane -> rank() == 3 )
      {
        action_list_str += "/life_tap,mana_percentage<=35";
        if ( ! set_bonus.tier13_4pc_caster() )
        {
          if ( glyphs.lash_of_pain -> ok() )
          {
            action_list_str += "/soulburn,if=buff.demon_soul_succubus.down";
          }
          else
          {
            action_list_str += "/soulburn,if=buff.demon_soul_felhunter.down";
          }
          action_list_str += "/soul_fire,if=buff.soulburn.up";
        }
        if ( level >= 85 && glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul";
        action_list_str += "/shadow_bolt";
      }
      else
      {
        if ( level >= 85 && glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul,if=buff.shadow_trance.react";
        action_list_str += "/shadow_bolt,if=buff.shadow_trance.react";
        action_list_str += "/life_tap,mana_percentage<=5";
        action_list_str += "/soulburn";
        action_list_str += "/drain_life";
      }

      break;

    case TREE_DESTRUCTION:
      if ( level >= 85 && ! glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul";
      if ( set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/soulburn";
        action_list_str += "/soul_fire,if=buff.soulburn.up&!in_combat";
      }
      else
      {
        action_list_str += "/soulburn,if=buff.bloodlust.down";
        if ( talent_improved_soul_fire -> ok() && level >= 54 )
        {
          action_list_str += "/soul_fire,if=buff.soulburn.up&!in_combat";
        }
      }
      action_list_str += "/immolate,if=(remains<cast_time+gcd|!ticking)&target.time_to_die>=4&miss_react";
      if ( talent_conflagrate -> ok() ) action_list_str += "/conflagrate";
      action_list_str += "/immolate,if=buff.bloodlust.react&buff.bloodlust.remains>32&cooldown.conflagrate.remains<=3&remains<12";
      if ( level >= 20 ) action_list_str += "/bane_of_doom,if=!ticking&target.time_to_die>=15&miss_react";
      action_list_str += "/corruption,if=(!ticking|dot.corruption.remains<tick_time)&miss_react";
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( talent_chaos_bolt -> ok() ) action_list_str += "/chaos_bolt,if=cast_time>0.9";
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soul_fire,if=buff.soulburn.up";
      if ( talent_improved_soul_fire -> ok() && level >= 54 )
      {
        action_list_str += "/soul_fire,if=((buff.empowered_imp.react&buff.empowered_imp.remains<(buff.improved_soul_fire.remains+action.soul_fire.travel_time))|buff.improved_soul_fire.remains<(cast_time+travel_time+action.incinerate.cast_time+gcd))&!in_flight";
      }
      if ( talent_shadowburn -> ok() ) action_list_str += "/shadowburn";
      if ( level >= 64 ) action_list_str += "/incinerate"; else action_list_str += "/shadow_bolt";

      break;

    case TREE_DEMONOLOGY:
      if ( talent_metamorphosis -> ok() )
      {
        action_list_str += "/metamorphosis";
        if ( has_mwc ) action_list_str += ",if=buff.moonwell_chalice.up&pet.felguard.active";
      }
      if ( level >= 85 )
      {
        action_list_str += "/demon_soul";
        if ( has_mwc ) action_list_str += ",if=buff.metamorphosis.up";
      }
      if ( level >= 20 )
      {
        action_list_str += "/bane_of_doom,if=!ticking&time<10";
      }
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      action_list_str += "/felguard:felstorm";
      action_list_str += "/soulburn,if=pet.felguard.active&!pet.felguard.dot.felstorm.ticking";
      action_list_str += "/summon_felhunter,if=!pet.felguard.dot.felstorm.ticking&pet.felguard.active";
      if ( set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/soulburn,if=pet.felhunter.active";
        if ( has_mwc ) action_list_str += "&cooldown.metamorphosis.remains>60";
        action_list_str += "/soul_fire,if=pet.felhunter.active&buff.soulburn.up";
        if ( has_mwc ) action_list_str += "&cooldown.metamorphosis.remains>60";
      }
      action_list_str += "/immolate,if=!ticking&target.time_to_die>=4&miss_react";
      if ( level >= 20 )
      {
        action_list_str += "/bane_of_doom,if=(!ticking";
        if ( talent_metamorphosis -> ok() )
          action_list_str += "|(buff.metamorphosis.up&remains<45)";
        action_list_str += ")&target.time_to_die>=15&miss_react";
      }
      action_list_str += "/corruption,if=(remains<tick_time|!ticking)&target.time_to_die>=6&miss_react";
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( talent_hand_of_guldan -> ok() ) action_list_str += "/hand_of_guldan";
      if ( level >= 60 ) action_list_str += "/immolation_aura,if=buff.metamorphosis.remains>10";
      if ( glyphs.corruption -> ok() ) action_list_str += "/shadow_bolt,if=buff.shadow_trance.react";
      if ( level >= 64 ) action_list_str += "/incinerate,if=buff.molten_core.react";
      if ( level >= 54 ) action_list_str += "/soul_fire,if=buff.decimation.up";
      action_list_str += "/life_tap,if=mana_pct<=30&buff.bloodlust.down&buff.metamorphosis.down&buff.demon_soul_felguard.down";
      action_list_str += ( talent_bane -> ok() ) ? "/shadow_bolt" : "/incinerate";

      break;

    default:
      action_list_str += "/bane_of_doom,if=(remains<3|!ticking)&target.time_to_die>=15&miss_react";
      action_list_str += "/corruption,if=(!ticking|remains<tick_time)&miss_react";
      action_list_str += "/immolate,if=(!ticking|remains<(cast_time+tick_time))&miss_react";
      if ( level >= 50 ) action_list_str += "/summon_infernal";
      if ( level >= 64 ) action_list_str += "/incinerate"; else action_list_str += "/shadow_bolt";
      if ( sim->debug ) log_t::output( sim, "Using generic action string for %s.", name() );
      break;
    }

    // Movement
    action_list_str += "/life_tap,moving=1,if=mana_pct<80&mana_pct<target.health_pct";
    action_list_str += "/fel_flame,moving=1";

    action_list_str += "/life_tap,if=mana_pct_nonproc<100"; // to use when no mana or nothing else is possible

    action_list_default = 1;
  }

  player_t::init_actions();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );
  if ( active_pet ) active_pet -> init_resources( force );
}

// warlock_t::reset =========================================================

void warlock_t::reset()
{
  player_t::reset();

  // Active
  active_pet = 0;
}

// warlock_t::create_expression =============================================

action_expr_t* warlock_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "shards" )
  {
    struct shards_expr_t : public action_expr_t
    {
      shards_expr_t( action_t* a ) : action_expr_t( a, "shards", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> cast_warlock() -> resources.current[ RESOURCE_SOUL_SHARD ]; return TOK_NUM; }
    };
    return new shards_expr_t( a );
  }
  return player_t::create_expression( a, name_str );
}

// warlock_t::create_options ================================================

void warlock_t::create_options()
{
  player_t::create_options();

  option_t warlock_options[] =
  {
    { "use_pre_soulburn",    OPT_BOOL,   &( use_pre_soulburn       ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, warlock_options );
}

// warlock_t::create_profile ================================================

bool warlock_t::create_profile( std::string& profile_str, save_type_e stype, bool save_html )
{
  player_t::create_profile( profile_str, stype, save_html );

  if ( stype == SAVE_ALL )
  {
    if ( use_pre_soulburn ) profile_str += "use_pre_soulburn=1\n";
  }

  return true;
}

// warlock_t::copy_from =====================================================

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  warlock_t* p = source -> cast_warlock();
  use_pre_soulburn       = p -> use_pre_soulburn;
}

// warlock_t::decode_set ====================================================

int warlock_t::decode_set( const item_t& item ) const
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

  if ( strstr( s, "_of_the_faceless_shroud" ) ) return SET_T13_CASTER;

  if ( strstr( s, "_gladiators_felweave_"   ) ) return SET_PVP_CASTER;

  return SET_NONE;
}

#endif // SC_WARLOCK

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_warlock =================================================

player_t* player_t::create_warlock( sim_t* sim, const std::string& name, race_type_e r )
{
  SC_CREATE_WARLOCK( sim, name, r );
}

// player_t::warlock_init ===================================================

void player_t::warlock_init( sim_t* )
{
}

// player_t::warlock_combat_begin ===========================================

void player_t::warlock_combat_begin( sim_t* )
{
}

