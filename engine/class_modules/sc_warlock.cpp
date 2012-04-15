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
  if ( dots_agony -> ticking                ) effects++;
  if ( dots_doom -> ticking                 ) effects++;
  if ( dots_corruption -> ticking           ) effects++;
  if ( dots_drain_life -> ticking           ) effects++;
  if ( dots_drain_soul -> ticking           ) effects++;
  if ( dots_unstable_affliction -> ticking  ) effects++;
  if ( dots_malefic_grasp -> ticking        ) effects++;
  if ( debuffs_haunt        -> check()      ) effects++;
  return effects;
}

int warlock_targetdata_t::active_dots()
{
  int dots = 0;
  if ( dots_agony -> ticking                    ) dots++;
  if ( dots_doom -> ticking                     ) dots++;
  if ( dots_corruption -> ticking               ) dots++;
  if ( dots_drain_life -> ticking               ) dots++;
  if ( dots_drain_soul -> ticking               ) dots++;
  if ( dots_immolate -> ticking                 ) dots++;
  if ( dots_shadowflame -> ticking              ) dots++;
  if ( dots_unstable_affliction -> ticking      ) dots++;
  if ( dots_malefic_grasp -> ticking            ) dots++;
  return dots;
}


void register_warlock_targetdata( sim_t* sim )
{
  player_type_e t = WARLOCK;
  typedef warlock_targetdata_t type;

  REGISTER_DOT( corruption );
  REGISTER_DOT( unstable_affliction );
  REGISTER_DOT( agony );
  REGISTER_DOT( doom );
  REGISTER_DOT( immolate );
  REGISTER_DOT( drain_life );
  REGISTER_DOT( drain_soul );
  REGISTER_DOT( shadowflame );
  REGISTER_DOT( burning_embers );
  REGISTER_DOT( malefic_grasp );

  REGISTER_DEBUFF( haunt );
}

warlock_targetdata_t::warlock_targetdata_t( warlock_t* p, player_t* target )
  : targetdata_t( p, target )
{
  debuffs_haunt = add_aura( buff_creator_t( this, "haunt", source -> find_spell( "haunt" ) ) );
}

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_type_e r ) :
  player_t( sim, WARLOCK, name, r ),
  pets( pets_t() ),
  buffs( buffs_t() ),
  cooldowns( cooldowns_t() ),
  talents( talents_t() ),
  mastery_spells( mastery_spells_t() ),
  gains( gains_t() ),
  benefits( benefits_t() ),
  procs( procs_t() ),
  rngs( rngs_t() ),
  glyphs( glyphs_t() ),
  use_pre_soulburn( 0 )
{

  distance = 40;
  default_distance = 40;

  cooldowns.metamorphosis                   = get_cooldown ( "metamorphosis" );
  cooldowns.infernal                        = get_cooldown ( "summon_infernal" );
  cooldowns.doomguard                       = get_cooldown ( "summon_doomguard" );

  use_pre_soulburn = 1;

  create_options();
}


namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Warlock Heal
// ==========================================================================

struct warlock_heal_t : public heal_t
{
  warlock_heal_t( const char* n, warlock_t* p, const uint32_t id ) :
    heal_t( n, p, p -> find_spell(id) )
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
  warlock_spell_t( warlock_t* p, const std::string& n, school_type_e sc = SCHOOL_NONE ) : 
    spell_t( "", p, p -> find_spell( n ), sc )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( warlock_t* p, const spell_data_t* s = spell_data_t::nil(), school_type_e sc = SCHOOL_NONE ) : 
    spell_t( "", p, s, sc )
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

    if ( base_execute_time > timespan_t::zero() && p -> buffs.demon_soul_imp -> up() )
    {
      player_crit += p -> buffs.demon_soul_imp -> effect1().percent();
    }
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
      if ( td -> debuffs_haunt -> up() )
      {
        shadow_td_multiplier *= 1.0 + td -> debuffs_haunt -> effect3().percent() + ( p -> glyphs.haunt -> effect1().percent() );
      }
    }

    return spell_t::total_td_multiplier() * shadow_td_multiplier;
  }

  // trigger_soul_leech =====================================================

  static void trigger_soul_leech( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();

    if ( p -> talents.soul_leech -> ok() )
    {
      p -> resource_gain( RESOURCE_HEALTH, p -> resources.max[ RESOURCE_HEALTH ] * p -> talents.soul_leech -> effect1().percent(), p -> gains.soul_leech_health );
      p -> resource_gain( RESOURCE_MANA, p -> resources.max[ RESOURCE_MANA ] * p -> talents.soul_leech -> effect1().percent(), p -> gains.soul_leech );

    }
  }

  // trigger_decimation =====================================================

  static void trigger_decimation( warlock_spell_t* s, int result )
  {
    warlock_t* p = s -> player -> cast_warlock();
    if ( ( result != RESULT_HIT ) && ( result != RESULT_CRIT ) ) return;
    if ( s -> target -> health_percentage() > p -> spec.decimation -> effect2().base_value() ) return;
    p -> buffs.decimation -> trigger();
  }
};

// Curse of Elements Spell ==================================================

struct curse_of_elements_t : public warlock_spell_t
{
  curse_of_elements_t( warlock_t* p ) :
    warlock_spell_t( p, "Curse of the Elements" )
  {
    background = ( sim -> overrides.magic_vulnerability != 0 );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      if ( ! sim -> overrides.magic_vulnerability )
        target -> debuffs.magic_vulnerability -> trigger( 1, -1, -1, data().duration() );
    }
  }
};

// Bane of Agony Spell ======================================================

struct agony_t : public warlock_spell_t
{
  agony_t( warlock_t* p ) :
    warlock_spell_t( p, "Agony" )
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

    may_crit = false;

    int extra_ticks = ( int ) ( p -> glyphs.bane_of_agony -> effectN( 1 ).time_value() / base_tick_time );

    if ( extra_ticks > 0 )
    {
      // after patch 3.0.8, the added ticks are double the base damage

      double total_damage = base_td_init * ( num_ticks + extra_ticks * 2 );

      num_ticks += extra_ticks;

      base_td_init = total_damage / num_ticks;
    }
  }

};

// Bane of Doom Spell =======================================================

struct doom_t : public warlock_spell_t
{
  doom_t( warlock_t* p ) :
    warlock_spell_t( p, "Doom" )
  {
    hasted_ticks = false;
    may_crit = false;
  }
};

// Bane of Havoc Spell ======================================================

struct bane_of_havoc_t : public warlock_spell_t
{
  bane_of_havoc_t( warlock_t* p ) : warlock_spell_t( p, "Bane of Havoc" ) { }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      p -> buffs.bane_of_havoc -> trigger();
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
  bool used_shadow_trance;

  shadow_bolt_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Shadow Bolt" ),
    used_shadow_trance( 0 )
  {
    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new shadow_bolt_t( p, true );
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
      p -> benefits.backdraft[ i ] -> update( i == p -> buffs.backdraft -> check() );
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
    }
  }
};

// Burning Embers Spell =====================================================

// Chaos Bolt Spell =========================================================

struct chaos_bolt_t : public warlock_spell_t
{
  chaos_bolt_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Chaos Bolt" )
  {
    may_miss = false;

    cooldown -> duration += p -> glyphs.chaos_bolt -> effectN( 1 ).time_value();

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new chaos_bolt_t( p, true );
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
      p -> benefits.backdraft[ i ] -> update( i == p -> buffs.backdraft -> check() );
    }

    if ( p -> buffs.backdraft -> check() )
      p -> buffs.backdraft -> decrement();
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
  death_coil_t( warlock_t* p ) : warlock_spell_t( p, "Death Coil" ) { }

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

  shadowburn_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Shadowburn" ),
    cd_glyph_of_shadowburn( 0 )
  {
    if ( p -> glyphs.shadowburn -> ok() )
    {
      cd_glyph_of_shadowburn             = p -> get_cooldown ( "glyph_of_shadowburn" );
      cd_glyph_of_shadowburn -> duration = p -> dbc.spell( 91001 ) -> duration();
    }

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new shadowburn_t( p, true );
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

  shadowfury_t( warlock_t* p ) :
    warlock_spell_t( p, "Shadowfury" ),
    cast_gcd( timespan_t::zero() )
  {
    // estimate - measured at ~0.6sec, but lag in there too, plus you need to mouse-click
    trigger_gcd = ( cast_gcd >= timespan_t::zero() ) ? cast_gcd : timespan_t::from_seconds( 0.5 );
  }
};

// Corruption Spell =========================================================

struct corruption_t : public warlock_spell_t
{
  corruption_t( warlock_t* p ) : warlock_spell_t( p, "Corruption" ) {};

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick( d );

    p -> buffs.eradication -> trigger();

    if ( p -> buffs.shadow_trance -> trigger() )
      p -> procs.shadow_trance -> occur();
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

    base_dd_min = base_dd_max = p -> resources.max[ RESOURCE_HEALTH ] * heal_pct;
    warlock_heal_t::execute();
  }
};

struct drain_life_t : public warlock_spell_t
{
  drain_life_heal_t* heal;

  drain_life_t( warlock_t* p ) :
    warlock_spell_t( p, "Drain Life" ), heal( 0 )
  {
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

  virtual timespan_t tick_time( double haste ) const
  {
    warlock_t* p = player -> cast_warlock();
    timespan_t t = warlock_spell_t::tick_time( haste );

    if ( p -> buffs.soulburn -> up() )
      t *= 1.0 - 0.5;

    return t;
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick( d );

    if ( p -> buffs.shadow_trance -> trigger( 1, 1.0, p -> spec.nightfall -> proc_chance() ) )
      p -> procs.shadow_trance -> occur();

    heal -> execute();
  }
};

// Drain Soul Spell =========================================================

struct drain_soul_t : public warlock_spell_t
{
  drain_soul_t( warlock_t* p ) :
    warlock_spell_t( p, "Drain Soul" )
  {
    channeled    = true;
    hasted_ticks = true; // informative
    may_crit     = false;
  }

  virtual void player_buff()
  {
    warlock_spell_t::player_buff();
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    if ( target -> health_percentage() < effect3().base_value() )
    {
      player_multiplier *= 2.0;
    }
  }
};

// Unstable Affliction Spell ================================================

struct unstable_affliction_t : public warlock_spell_t
{
  unstable_affliction_t( warlock_t* p ) :
    warlock_spell_t( p, "Unstable Affliction" )
  {
    may_crit   = false;
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
  haunt_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Haunt" )
  {
    direct_power_mod = 0.5577;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new haunt_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      td -> debuffs_haunt -> trigger();
    }
  }
};

// Immolate Spell ===========================================================

struct immolate_t : public warlock_spell_t
{
  immolate_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Immolate" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new immolate_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    player_td_multiplier += p -> glyphs.immolate -> effectN( 1 ).percent();
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

struct shadowflame_t : public warlock_spell_t
{
  shadowflame_t( warlock_t* p ) :
    warlock_spell_t( p, "Shadowflame" )
  {
    proc       = true;
    background = true;
  }
};

// Conflagrate Spell ========================================================

struct conflagrate_t : public warlock_spell_t
{
  conflagrate_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Conflagrate" )
  {
    base_dd_multiplier *= 1.0 + p -> glyphs.immolate -> effectN( 1 ).percent();

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new conflagrate_t( p, true );
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

  incinerate_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Incinerate" ),
    incinerate_burst_immolate( 0 )
  {
    base_multiplier *= 1.0 + p -> glyphs.incinerate -> effectN( 1 ).percent();

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new incinerate_t( p, true );
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
      p -> benefits.backdraft[ i ] -> update( i == p -> buffs.backdraft -> check() );
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


// Soul Fire Spell ==========================================================

struct soul_fire_t : public warlock_spell_t
{
  soul_fire_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Soul Fire" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new soul_fire_t( p, true );
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
      t *= 1.0 - p -> spec.decimation -> effect1().percent();
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

      trigger_soul_leech( this );

      p -> buffs.improved_soul_fire -> trigger();
    }
  }
};

// Life Tap Spell ===========================================================

struct life_tap_t : public warlock_spell_t
{
  double trigger;
  double max_mana_pct;

  life_tap_t( warlock_t* p ) :
    warlock_spell_t( p, "Life Tap" ),
    trigger( 0 ), max_mana_pct( 0 )
  {
    harmful = false;

    if ( p -> glyphs.life_tap -> ok() )
      trigger_gcd += p -> glyphs.life_tap -> effect1().time_value();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    double life = p -> resources.max[ RESOURCE_HEALTH ] * effect3().percent();
    double mana = life * effect2().percent();
    p -> resource_loss( RESOURCE_HEALTH, life );
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains.life_tap );
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
  demon_armor_t( warlock_t* p ) :
    warlock_spell_t( p, "Demon Armor" )
  {
    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    warlock_spell_t::execute();
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

  fel_armor_t( warlock_t* p ) :
    warlock_spell_t( p, "Fel Armor" ), bonus_spell_power( 0 )
  {
    harmful = false;

    bonus_spell_power = p -> buffs.fel_armor -> data().effectN( 1 ).base_value();

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
                        p -> resources.max[ RESOURCE_HEALTH ] * effect2().percent(),
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
  void _init_summon_pet_t( const std::string& pet_name )
  {
    warlock_t* p = player -> cast_warlock();
    harmful = false;

    pet = p -> find_pet( pet_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p -> name(), pet_name );
      sim -> cancel();
    }
  }

public:
  summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname ) :
    warlock_spell_t( p, sname ), summoning_duration ( timespan_t::zero() ), pet( 0 )
  {
    _init_summon_pet_t( n );
  }

  summon_pet_t( const std::string& n, warlock_t* p, int id ) :
    warlock_spell_t( p, p -> find_spell( id, n ) ), summoning_duration ( timespan_t::zero() ), pet( 0 )
  {
    _init_summon_pet_t( n );
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

  summon_main_pet_t( const char* n, warlock_t* p, const char* sname ) :
    summon_pet_t( n, p, sname )
  { }

  virtual void schedule_execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::schedule_execute();

    if ( p -> pets.active )
      p -> pets.active -> dismiss();
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> pets.active == pet )
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
  summon_felhunter_t( warlock_t* p ) :
    summon_main_pet_t( "felhunter", p, "Summon Felhunter" )
  { }
};

struct summon_felguard_t : public summon_main_pet_t
{
  summon_felguard_t( warlock_t* p ) :
    summon_main_pet_t( "felguard", p, "Summon Felguard" )
  { }
};

struct summon_succubus_t : public summon_main_pet_t
{
  summon_succubus_t( warlock_t* p ) :
    summon_main_pet_t( "succubus", p, "Summon Succubus" )
  { }
};

struct summon_imp_t : public summon_main_pet_t
{
  summon_imp_t( warlock_t* p ) :
    summon_main_pet_t( "imp", p, "Summon Imp" )
  { }
};

struct summon_voidwalker_t : public summon_main_pet_t
{
  summon_voidwalker_t( warlock_t* p ) :
    summon_main_pet_t( "voidwalker", p, "Summon Voidwalker" )
  { }
};

// Infernal Awakening =======================================================

struct infernal_awakening_t : public warlock_spell_t
{
  infernal_awakening_t( warlock_t* p ) :
    warlock_spell_t( p, p -> find_spell( 22703, "infernal_awakening" ) )
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

  summon_infernal_t( warlock_t* p  ) :
    summon_pet_t( "infernal", p, "Summon Infernal" ),
    infernal_awakening( 0 )
  {
    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 3 ).time_value() : timespan_t::zero();

    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ?
                          ( p -> primary_tree() == WARLOCK_DEMONOLOGY ?
                            p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).time_value() :
                            p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 2 ).time_value() ) : timespan_t::zero();
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
    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ?
                          ( p -> primary_tree() == WARLOCK_DEMONOLOGY ?
                            p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).time_value() :
                            p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 2 ).time_value() ) : timespan_t::zero();
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

  summon_doomguard_t( warlock_t* p ) :
    warlock_spell_t( p, "Summon Doomguard" ),
    summon_doomguard2( 0 )
  {
    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 3 ).time_value() : timespan_t::zero();

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

// Metamorphosis Spell ======================================================

struct metamorphosis_t : public warlock_spell_t
{
  metamorphosis_t( warlock_t* p ) :
    warlock_spell_t( p, "Metamorphosis" )
  {
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

// Hand of Gul'dan Spell ====================================================

struct hand_of_guldan_t : public warlock_spell_t
{
  hand_of_guldan_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Hand of Gul'dan" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new hand_of_guldan_t( p, true );
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
    }
  }

  virtual timespan_t travel_time() const
  {
    return timespan_t::from_seconds( 0.2 );
  }
};

// Fel Flame Spell ==========================================================

struct fel_flame_t : public warlock_spell_t
{
  fel_flame_t( warlock_t* p, bool dtr = false ) :
    warlock_spell_t( p, "Fel Flame" )
  {
    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new fel_flame_t( p, true );
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
  dark_intent_t( warlock_t* p ) :
    warlock_spell_t( p, "Dark Intent" )
  {
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
  soulburn_t( warlock_t* p ) :
    warlock_spell_t( p, "Soulburn" )
  {
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
  demon_soul_t( warlock_t* p ) :
    warlock_spell_t( p, "Demon Soul" )
  {
    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    assert ( p -> pets.active );

    if ( p -> pets.active -> pet_type == PET_IMP )
    {
      p -> buffs.demon_soul_imp -> trigger();
    }
    if ( p -> pets.active -> pet_type == PET_FELGUARD )
    {
      p -> buffs.demon_soul_felguard -> trigger();
    }
    if ( p -> pets.active -> pet_type == PET_FELHUNTER )
    {
      p -> buffs.demon_soul_felhunter -> trigger();
    }
    if ( p -> pets.active -> pet_type == PET_SUCCUBUS )
    {
      p -> buffs.demon_soul_succubus -> trigger();
    }
    if ( p -> pets.active -> pet_type == PET_VOIDWALKER )
    {
      p -> buffs.demon_soul_voidwalker -> trigger();
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! p ->  pets.active )
      return false;

    return warlock_spell_t::ready();
  }
};

// Hellfire Effect Spell ====================================================

struct hellfire_tick_t : public warlock_spell_t
{
  hellfire_tick_t( warlock_t* p ) :
    warlock_spell_t( p, p -> find_spell( 5857, "hellfire_tick" ) )
  {
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;
  }
};

// Hellfire Spell ===========================================================

struct hellfire_t : public warlock_spell_t
{
  hellfire_tick_t* hellfire_tick;

  hellfire_t( warlock_t* p ) :
    warlock_spell_t( p, "Hellfire" ), hellfire_tick( 0 )
  {
    // Hellfire has it's own damage effect, which is actually the damage to the player himself, so harmful is set to false.
    harmful = false;

    channeled    = true;
    hasted_ticks = false;

    hellfire_tick = new hellfire_tick_t( p );
  }

  virtual void init()
  {
    warlock_spell_t::init();

    hellfire_tick -> stats = stats;
  }

  virtual bool usable_moving()
  {
    warlock_t* p = player -> cast_warlock();

    return false;
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
    warlock_spell_t( p, p -> find_spell( 27285, "seed_of_corruption_aoe" ) )
  {
    proc       = true;
    background = true;
    aoe        = -1;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs.soulburn -> check() )
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

  seed_of_corruption_t( warlock_t* p ) :
    warlock_spell_t( p, "Seed of Corruption" ),
    seed_of_corruption_aoe( 0 ), dot_damage_done( 0 )
  {
    seed_of_corruption_aoe = new seed_of_corruption_aoe_t( p );
    add_child( seed_of_corruption_aoe );
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
    warlock_spell_t( p, p -> find_spell( 42223, "rain_of_fire_tick" ) )
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

  rain_of_fire_t( warlock_t* p ) :
    warlock_spell_t( p, "Rain of Fire" ),
    rain_of_fire_tick( 0 )
  {
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
    m *= 1.0 + sets -> set ( SET_T13_4PC_CASTER ) -> effect1().percent();
  }

  return m;
}

// warlock_t::composite_player_multiplier ===================================

double warlock_t::composite_player_multiplier( school_type_e school, const action_t* a ) const
{
  double player_multiplier = player_t::composite_player_multiplier( school, a );

  double mastery_value = mastery_spells.master_demonologist -> effectN( 3 ).base_value();

  if ( buffs.metamorphosis -> up() )
  {
    player_multiplier *= 1.0 + buffs.metamorphosis -> effect3().percent()
                         + ( buffs.metamorphosis -> value() * mastery_value / 10000.0 );
  }

  if ( ( school == SCHOOL_FIRE || school == SCHOOL_SHADOW ) && buffs.demon_soul_felguard -> up() )
  {
    player_multiplier *= 1.0 + buffs.demon_soul_felguard -> effect2().percent();
  }

  double fire_multiplier   = 1.0;
  double shadow_multiplier = 1.0;

  // Fire
  if ( mastery_spells.fiery_apocalypse -> ok() )
    fire_multiplier *= 1.0 + ( composite_mastery() * mastery_spells.fiery_apocalypse -> effectN( 2 ).base_value() / 10000.0 );

  // Shadow

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

double warlock_t::composite_player_td_multiplier( school_type_e school, const action_t* a ) const
{
  double player_multiplier = player_t::composite_player_td_multiplier( school, a );

  if ( school == SCHOOL_SHADOW || school == SCHOOL_SHADOWFLAME )
  {
    // Shadow TD
    if ( mastery_spells.potent_afflictions -> ok() )
    {
      player_multiplier += floor ( ( composite_mastery() * mastery_spells.potent_afflictions -> effectN( 2 ).base_value() / 10000.0 ) * 1000 ) / 1000;
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
  if ( ( attr == ATTR_INTELLECT ) && spec.nethermancy -> ok() )
    return ( spec.nethermancy -> effectN( 1 ).base_value() * 0.01 );

  return 0.0;
}

// warlock_t::create_action =================================================

action_t* warlock_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  action_t* a;
  
  if      ( name == "chaos_bolt"          ) a = new          chaos_bolt_t( this );
  else if ( name == "conflagrate"         ) a = new         conflagrate_t( this );
  else if ( name == "corruption"          ) a = new          corruption_t( this );
  else if ( name == "agony"               ) a = new               agony_t( this );
  else if ( name == "doom"                ) a = new                doom_t( this );
  else if ( name == "curse_of_elements"   ) a = new   curse_of_elements_t( this );
  else if ( name == "death_coil"          ) a = new          death_coil_t( this );
  else if ( name == "demon_armor"         ) a = new         demon_armor_t( this );
  else if ( name == "drain_life"          ) a = new          drain_life_t( this );
  else if ( name == "drain_soul"          ) a = new          drain_soul_t( this );
  else if ( name == "fel_armor"           ) a = new           fel_armor_t( this );
  else if ( name == "haunt"               ) a = new               haunt_t( this );
  else if ( name == "immolate"            ) a = new            immolate_t( this );
  else if ( name == "shadowflame"         ) a = new         shadowflame_t( this );
  else if ( name == "incinerate"          ) a = new          incinerate_t( this );
  else if ( name == "life_tap"            ) a = new            life_tap_t( this );
  else if ( name == "metamorphosis"       ) a = new       metamorphosis_t( this );
  else if ( name == "shadow_bolt"         ) a = new         shadow_bolt_t( this );
  else if ( name == "shadowburn"          ) a = new          shadowburn_t( this );
  else if ( name == "shadowfury"          ) a = new          shadowfury_t( this );
  else if ( name == "soul_fire"           ) a = new           soul_fire_t( this );
  else if ( name == "summon_felhunter"    ) a = new    summon_felhunter_t( this );
  else if ( name == "summon_felguard"     ) a = new     summon_felguard_t( this );
  else if ( name == "summon_succubus"     ) a = new     summon_succubus_t( this );
  else if ( name == "summon_voidwalker"   ) a = new   summon_voidwalker_t( this );
  else if ( name == "summon_imp"          ) a = new          summon_imp_t( this );
  else if ( name == "summon_infernal"     ) a = new     summon_infernal_t( this );
  else if ( name == "summon_doomguard"    ) a = new    summon_doomguard_t( this );
  else if ( name == "unstable_affliction" ) a = new unstable_affliction_t( this );
  else if ( name == "hand_of_guldan"      ) a = new      hand_of_guldan_t( this );
  else if ( name == "fel_flame"           ) a = new           fel_flame_t( this );
  else if ( name == "dark_intent"         ) a = new         dark_intent_t( this );
  else if ( name == "soulburn"            ) a = new            soulburn_t( this );
  else if ( name == "demon_soul"          ) a = new          demon_soul_t( this );
  else if ( name == "bane_of_havoc"       ) a = new       bane_of_havoc_t( this );
  else if ( name == "hellfire"            ) a = new            hellfire_t( this );
  else if ( name == "seed_of_corruption"  ) a = new  seed_of_corruption_t( this );
  else if ( name == "rain_of_fire"        ) a = new        rain_of_fire_t( this );
  else return player_t::create_action( name, options_str );

  a -> parse_options( NULL, options_str );
  if ( a -> dtr_action ) a -> dtr_action -> parse_options( NULL, options_str );

  return a;
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
  pets.ebon_imp = create_pet( "ebon_imp"  );
}

// warlock_t::init_talents ==================================================

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

  // Spec spells =========================================================

  // General
  spec.nethermancy = find_spell( "Nethermancy" );

  // Affliction
  spec.nightfall = find_spell( "Nightfall" );
  
  // Demonology
  spec.decimation = find_spell( "Decimation" );

  // Destruction
  spec.burning_embers = find_spell( "Burning Embers" );

  // Mastery
  mastery_spells.fiery_apocalypse     = find_mastery_spell( "Fiery Apocalypse" );
  mastery_spells.potent_afflictions   = find_mastery_spell( "Potent Afflictions" );
  mastery_spells.master_demonologist  = find_mastery_spell( "Master Demonologist" );

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

// helpers

buff_t* warlock_t::create_buff( const char* name )
{
  std::string sname = std::string( name );
  return buff_creator_t( this, armory_t::format( sname ), find_spell( name ) );
}

buff_t* warlock_t::create_buff( int id, const std::string& token )
{
  return buff_creator_t( this, token, find_spell( id, token ) );
}

void warlock_t::init_buffs()
{
  player_t::init_buffs();

  buffs.backdraft             = create_buff( "Backdraft" );
  buffs.decimation            = create_buff( "Decimation" );
  buffs.metamorphosis         = create_buff( "Metamorphosis" );
  buffs.molten_core           = create_buff( "Molten Core" );
  buffs.shadow_trance         = create_buff( "Shadow Trance" );
  buffs.soulburn              = create_buff( "Soulburn" );
  buffs.fel_armor             = create_buff( "Fel Armor" );
  buffs.tier13_4pc_caster     = create_buff( sets -> set ( SET_T13_4PC_CASTER ) -> id(), "tier13_4pc_caster" );
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

  for ( size_t i = 0; i < 4; ++i )
    benefits.backdraft[ i ] = get_benefit( "backdraft_" + util_t::to_string( i ) );
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
    if ( primary_tree() == WARLOCK_DESTRUCTION )
    {
      action_list_str += "/summon_imp";
    }
    else if ( primary_tree() == WARLOCK_DEMONOLOGY )
    {
      if ( has_mwc )
        action_list_str += "/summon_felguard,if=cooldown.demon_soul.remains<5&cooldown.metamorphosis.remains<5&!pet.felguard.active";
      else
        action_list_str += "/summon_felguard,if=!in_combat";
    }
    else if ( primary_tree() == WARLOCK_AFFLICTION )
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
        if ( primary_tree() == WARLOCK_DEMONOLOGY )
          action_list_str += ",if=cooldown.metamorphosis.remains=0|cooldown.metamorphosis.remains>cooldown";
      }
    }

    action_list_str += init_use_profession_actions();
    action_list_str += init_use_racial_actions();

    // Choose Potion
    if ( level >= 80 )
    {
      if ( primary_tree() == WARLOCK_DEMONOLOGY )
        action_list_str += "/volcanic_potion,if=buff.metamorphosis.up|!in_combat";
      else
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react|!in_combat|target.health_pct<=20";
    }
    else if ( level >= 70 )
    {
      if ( primary_tree() == WARLOCK_AFFLICTION )
        action_list_str += "/speed_potion,if=buff.bloodlust.react|!in_combat";
      else
        action_list_str += "/wild_magic_potion,if=buff.bloodlust.react|!in_combat";
    }

    switch ( primary_tree() )
    {

    case WARLOCK_AFFLICTION:
      if ( level >= 85 && ! glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soulburn";
      action_list_str += "/corruption,if=(!ticking|remains<tick_time)&miss_react";
      action_list_str += "/unstable_affliction,if=(!ticking|remains<(cast_time+tick_time))&target.time_to_die>=5&miss_react";
      if ( level >= 12 ) action_list_str += "/bane_of_doom,if=target.time_to_die>15&!ticking&miss_react";
      action_list_str += "/haunt";
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
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
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soul_fire,if=buff.soulburn.up";
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
      break;

    case WARLOCK_DESTRUCTION:
      if ( level >= 85 && ! glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul";
      if ( set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/soulburn";
        action_list_str += "/soul_fire,if=buff.soulburn.up&!in_combat";
      }
      else
      {
        action_list_str += "/soulburn,if=buff.bloodlust.down";
        if ( level >= 54 )
        {
          action_list_str += "/soul_fire,if=buff.soulburn.up&!in_combat";
        }
      }
      action_list_str += "/immolate,if=(remains<cast_time+gcd|!ticking)&target.time_to_die>=4&miss_react";
      action_list_str += "/conflagrate";
      action_list_str += "/immolate,if=buff.bloodlust.react&buff.bloodlust.remains>32&cooldown.conflagrate.remains<=3&remains<12";
      if ( level >= 20 ) action_list_str += "/bane_of_doom,if=!ticking&target.time_to_die>=15&miss_react";
      action_list_str += "/corruption,if=(!ticking|dot.corruption.remains<tick_time)&miss_react";
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soul_fire,if=buff.soulburn.up";
      action_list_str += "/shadowburn";
      if ( level >= 64 ) action_list_str += "/incinerate"; else action_list_str += "/shadow_bolt";

      break;

    case WARLOCK_DEMONOLOGY:
      action_list_str += "/metamorphosis";
      if ( has_mwc ) action_list_str += ",if=buff.moonwell_chalice.up&pet.felguard.active";
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
        action_list_str += "|(buff.metamorphosis.up&remains<45)";
        action_list_str += ")&target.time_to_die>=15&miss_react";
      }
      action_list_str += "/corruption,if=(remains<tick_time|!ticking)&target.time_to_die>=6&miss_react";
      if ( level >= 75 ) action_list_str += "/shadowflame";
      action_list_str += "/hand_of_guldan";
      if ( level >= 60 ) action_list_str += "/immolation_aura,if=buff.metamorphosis.remains>10";
      if ( glyphs.corruption -> ok() ) action_list_str += "/shadow_bolt,if=buff.shadow_trance.react";
      if ( level >= 64 ) action_list_str += "/incinerate,if=buff.molten_core.react";
      if ( level >= 54 ) action_list_str += "/soul_fire,if=buff.decimation.up";
      action_list_str += "/life_tap,if=mana_pct<=30&buff.bloodlust.down&buff.metamorphosis.down&buff.demon_soul_felguard.down";
      action_list_str += "/shadow_bolt";

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

  if ( pets.active )
    pets.active -> init_resources( force );
}

// warlock_t::reset =========================================================

void warlock_t::reset()
{
  player_t::reset();

  // Active
  pets.active = 0;
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

