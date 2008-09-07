// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

namespace { // ANONYMOUS NAMESPACE ==========================================

// Mystical Skyfire Diamond =================================================

static void trigger_mystical_skyfire( spell_t* s )
{
  struct mystical_skyfire_expiration_t : public event_t
  {
    mystical_skyfire_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      player -> aura_gain( "Mystical Skyfire" );
      player -> haste_rating += 320;
      player -> recalculate_haste();
      player -> cooldowns.mystical_skyfire = sim -> current_time + 45;
      sim -> add_event( this, 4.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Mystical Skyfire" );
      player -> haste_rating -= 320;
      player -> recalculate_haste();
    }
  };

  if( ! s -> harmful ) return;

  player_t* p = s -> player;

  if( p -> gear.mystical_skyfire                                    &&
      s -> sim -> cooldown_ready( p -> cooldowns.mystical_skyfire ) &&
      rand_t::roll( 0.15 ) )
  {
    p -> procs.mystical_skyfire -> occur();
    new mystical_skyfire_expiration_t( s -> sim, p );
  }
}

// Spellstrike Set Bonus ====================================================

static void trigger_spellstrike( spell_t* s )
{
  struct spellstrike_expiration_t : public event_t
  {
    spellstrike_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Spellstrike Set Bonus Expiration";
      player -> aura_gain( "Spellstrike Set Bonus" );
      player -> spell_power[ SCHOOL_MAX ] += 92;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Spellstrike Set Bonus" );
      player -> spell_power[ SCHOOL_MAX ] -= 92;
      player -> expirations.spellstrike = 0;
    }
  };

  if( ! s -> harmful ) return;

  player_t* p = s -> player;

  if( p -> gear.spellstrike && rand_t::roll( 0.05 ) )
  {
    p -> procs.spellstrike -> occur();

    event_t*& e = p -> expirations.spellstrike;

    if( e )
    {
      e -> reschedule( 10.0 );
    }
    else
    {
      e = new spellstrike_expiration_t( s -> sim, p );
    }
  }
}

// Wrath of Cenarius ====================================================

static void trigger_wrath_of_cenarius( spell_t* s )
{
  struct wrath_of_cenarius_expiration_t : public event_t
  {
    wrath_of_cenarius_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Wrath of Cenarius Expiration";
      player -> aura_gain( "Wrath of Cenarius" );
      player -> spell_power[ SCHOOL_MAX ] += 132;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Wrath of Cenarius" );
      player -> spell_power[ SCHOOL_MAX ] -= 132;
      player -> expirations.wrath_of_cenarius = 0;
    }
  };

  if( ! s -> harmful ) return;

  player_t* p = s -> player;

  if( p -> gear.wrath_of_cenarius && rand_t::roll( 0.05 ) )
  {
    p -> procs.wrath_of_cenarius -> occur();

    event_t*& e = p -> expirations.wrath_of_cenarius;

    if( e )
    {
      e -> reschedule( 10.0 );
    }
    else
    {
      e = new wrath_of_cenarius_expiration_t( s -> sim, p );
    }
  }
}

// Robe of the Elder Scribes ================================================

static void trigger_elder_scribes( spell_t* s )
{
  struct elder_scribes_expiration_t : public event_t
  {
    elder_scribes_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Elder Scribes Expiration";
      player -> aura_gain( "Power of Arcanagos" );
      player -> spell_power[ SCHOOL_MAX ] += 130;
      player -> cooldowns.elder_scribes = sim -> current_time + 60;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Power of Arcanagos" );
      player -> spell_power[ SCHOOL_MAX ] -= 130;
    }
  };

  player_t* p = s -> player;

  if(  p -> gear.elder_scribes                                    && 
       s -> sim -> cooldown_ready( p -> cooldowns.elder_scribes ) &&
       rand_t::roll( 0.05 ) )
  {
    p -> procs.elder_scribes -> occur();
    new elder_scribes_expiration_t( s -> sim, p );
  }
}

// Eternal Sage ============================================================

static void trigger_eternal_sage( spell_t* s )
{
  struct eternal_sage_expiration_t : public event_t
  {
    eternal_sage_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Eternal Sage Expiration";
      player -> aura_gain( "Eternal Sage" );
      player -> spell_power[ SCHOOL_MAX ] += 95;
      player -> cooldowns.eternal_sage = sim -> current_time + 45;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Eternal Sage" );
      player -> spell_power[ SCHOOL_MAX ] -= 95;
    }
  };

  player_t* p = s -> player;

  if(  p -> gear.eternal_sage                                    && 
       s -> sim -> cooldown_ready( p -> cooldowns.eternal_sage ) &&
       rand_t::roll( 0.10 ) )
  {
    p -> procs.eternal_sage -> occur();
    new eternal_sage_expiration_t( s -> sim, p );
  }
}

// Eye of Magtheridon ======================================================

static void trigger_eye_of_magtheridon( spell_t* s )
{
  struct eye_of_magtheridon_expiration_t : public event_t
  {
    eye_of_magtheridon_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Eye of Magtheridon Expiration";
      player -> aura_gain( "Eye of Magtheridon" );
      player -> spell_power[ SCHOOL_MAX ] += 170;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Eye of Magtheridon" );
      player -> spell_power[ SCHOOL_MAX ] -= 170;
      player -> expirations.eye_of_magtheridon = 0;
    }
  };

  player_t* p = s -> player;

  if( p -> gear.eye_of_magtheridon )
  {
    p -> procs.eye_of_magtheridon -> occur();

    event_t*& e = p -> expirations.eye_of_magtheridon;

    if( e )
    {
      e -> reschedule( 10.0 );
    }
    else
    {
      e = new eye_of_magtheridon_expiration_t( s -> sim, p );
    }
  }
}

// Shiffar's Nexus Horn =====================================================

static void trigger_shiffars_nexus_horn( spell_t* s )
{
  struct shiffars_nexus_horn_expiration_t : public event_t
  {
    shiffars_nexus_horn_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Shiffar's Nexus Horn Expiration";
      player -> aura_gain( "Shiffar's Nexus Horn" );
      player -> spell_power[ SCHOOL_MAX ] += 225;
      player -> cooldowns.shiffars_nexus_horn = sim -> current_time + 45;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Shiffar's Nexus Horn" );
      player -> spell_power[ SCHOOL_MAX ] -= 225;
    }
  };

  player_t* p = s -> player;

  if( p ->        gear.shiffars_nexus_horn                             && 
      s -> sim -> cooldown_ready( p -> cooldowns.shiffars_nexus_horn ) &&
      rand_t::roll( 0.20 ) )
  {
    p -> procs.shiffars_nexus_horn -> occur();
    new shiffars_nexus_horn_expiration_t( s -> sim, p );
  }
}

// Sextant of Unstable Currents ============================================

static void trigger_sextant_of_unstable_currents( spell_t* s )
{
  struct sextant_of_unstable_currents_expiration_t : public event_t
  {
    sextant_of_unstable_currents_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Sextant of Unstable Currents Expiration";
      player -> aura_gain( "Sextant of Unstable Currents" );
      player -> spell_power[ SCHOOL_MAX ] += 190;
      player -> cooldowns.sextant_of_unstable_currents = sim -> current_time + 45;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Sextant of Unstable Currents" );
      player -> spell_power[ SCHOOL_MAX ] -= 190;
    }
  };

  player_t* p = s -> player;

  if( p ->        gear.sextant_of_unstable_currents                             && 
      s -> sim -> cooldown_ready( p -> cooldowns.sextant_of_unstable_currents ) &&
      rand_t::roll( 0.20 ) )
  {
    p -> procs.sextant_of_unstable_currents -> occur();
    new sextant_of_unstable_currents_expiration_t( s -> sim, p );
  }
}

// Quagmirran's Eye ========================================================

static void trigger_quagmirrans_eye( spell_t* s )
{
  struct quagmirrans_eye_expiration_t : public event_t
  {
    quagmirrans_eye_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Quagmirran's Eye Expiration";
      player -> aura_gain( "Quagmirran's Eye" );
      player -> haste_rating += 320;
      player -> recalculate_haste();
      player -> cooldowns.quagmirrans_eye = sim -> current_time + 45;
      sim -> add_event( this, 6.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Quagmirran's Eye" );
      player -> haste_rating -= 320;
      player -> recalculate_haste();
    }
  };

  if( ! s -> harmful ) return;

  player_t* p = s -> player;

  if( p ->        gear.quagmirrans_eye                             && 
      s -> sim -> cooldown_ready( p -> cooldowns.quagmirrans_eye ) && 
      rand_t::roll( 0.10 ) )
  {
    p -> procs.quagmirrans_eye -> occur();
    new quagmirrans_eye_expiration_t( s -> sim, p );
  }
}

// Darkmoon Crusade ========================================================

static void trigger_darkmoon_crusade( spell_t* s )
{
  struct darkmoon_crusade_expiration_t : public event_t
  {
    darkmoon_crusade_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Darkmoon Crusade Expiration";
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Darkmoon Crusade" );
      player -> spell_power[ SCHOOL_MAX ] -= player -> buffs.darkmoon_crusade * 8;
      player -> buffs.darkmoon_crusade = 0;
      player -> expirations.darkmoon_crusade = 0;
    }
  };

  if( ! s -> harmful ) return;

  player_t* p = s -> player;

  if( ! p ->  gear.darkmoon_crusade ) return;

  if( p -> buffs.darkmoon_crusade < 10 )
  {
    p -> buffs.darkmoon_crusade++;
    p -> spell_power[ SCHOOL_MAX ] += 8;
    if( p -> buffs.darkmoon_crusade == 1 ) p -> aura_gain( "Darkmoon Crusade" );
  }
  
  event_t*& e = p -> expirations.darkmoon_crusade;

  if( e )
  {
    e -> reschedule( 10.0 );
  }
  else
  {
    e = new darkmoon_crusade_expiration_t( s -> sim, p );
  }
}

// Darkmoon Wrath ==========================================================

static void clear_darkmoon_wrath( spell_t* s )
{
  event_t*& e = s -> player -> expirations.darkmoon_wrath;

  if( e ) 
  {
    e -> canceled = true;
    e -> execute();
  }
}

static void trigger_darkmoon_wrath( spell_t* s )
{
  struct darkmoon_wrath_expiration_t : public event_t
  {
    darkmoon_wrath_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Darkmoon Wrath Expiration";
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Darkmoon Wrath" );
      player -> spell_crit -= player -> buffs.darkmoon_wrath * ( 17.0 / player -> rating.spell_crit );
      player -> buffs.darkmoon_wrath = 0;
      player -> expirations.darkmoon_wrath = 0;
    }
  };

  if( ! s -> harmful  ) return;
  if( ! s -> may_crit ) return;

  player_t* p = s -> player;

  if( ! p -> gear.darkmoon_wrath ) return;

  event_t*& e = p -> expirations.darkmoon_wrath;

  if( p -> buffs.darkmoon_wrath < 10 )
  {
    p -> buffs.darkmoon_wrath++;
    p -> spell_crit += 17 / p -> rating.spell_crit;
    if( p -> buffs.darkmoon_wrath == 1 ) p -> aura_gain( "Darkmoon Wrath" );
  }
  
  if( e )
  {
    e -> reschedule( 10.0 );
  }
  else
  {
    e = new darkmoon_wrath_expiration_t( s -> sim, p );
  }
}

// Lightning Capacitor =====================================================

static void trigger_lightning_capacitor( spell_t* s )
{
  struct lightning_discharge_t : public spell_t
  {
    lightning_discharge_t( player_t* player ) : 
      spell_t( "lightning_discharge", player, RESOURCE_NONE, SCHOOL_NATURE )
    {
      base_dd     = 750;
      base_dot    = 0;
      base_cost   = 0;
      cooldown    = 2.5;
      may_crit    = true;
      trigger_gcd = 0;
      background  = true;
      reset();
    }
    virtual void player_buff()
    {
      spell_t::player_buff();
      player_power = 0;
    }
  };

  player_t* p = s -> player;

  if(  p -> gear.lightning_capacitor )
  {
    if( ! p -> actions.lightning_discharge ) 
    {
      p -> actions.lightning_discharge = new lightning_discharge_t( p );
    }

    if( p -> buffs.lightning_capacitor < 2 )
    {
      p -> buffs.lightning_capacitor++;
      if( p -> buffs.lightning_capacitor == 1 ) p -> aura_gain( "Lightning Capacitor" );
    }
    else
    {
      if( p -> actions.lightning_discharge -> ready() )
      {
	p -> procs.lightning_capacitor -> occur();
	p -> buffs.lightning_capacitor = 0;
	p -> aura_loss( "Lightning Capacitor" );
	p -> actions.lightning_discharge -> execute();
      }
    }
  } 
}

// Timbals Focusing Crystal =================================================

static void trigger_timbals_crystal( spell_t* s )
{
  struct timbals_discharge_t : public spell_t
  {
    timbals_discharge_t( player_t* player ) : 
      spell_t( "timbals_discharge", player, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      base_dd     = 380;
      base_dot    = 0;
      base_cost   = 0;
      cooldown    = 15;
      may_crit    = true;
      trigger_gcd = 0;
      background  = true;
      reset();
    }
    virtual void player_buff()
    {
      spell_t::player_buff();
      player_power = 0;
    }
    virtual void assess_damage( double amount, int8_t dmg_type )
    {
      // Not considered a "direct-dmg" spell, so ISB charges not consumed.
      spell_t::assess_damage( dd, DMG_OVER_TIME ); 
    }
  };

  player_t* p = s -> player;

  if( p -> gear.timbals_crystal )
  {
    if( ! p -> actions.timbals_discharge ) 
    {
      p -> actions.timbals_discharge = new timbals_discharge_t( p );
    }

    if( p -> actions.timbals_discharge -> ready() && 
	rand_t::roll( 0.10 ) )
    {
      p -> procs.timbals_crystal -> occur();
      p -> actions.timbals_discharge -> execute();
    }
  } 
}

// Talisman of Ascendance ===================================================

static void trigger_talisman_of_ascendance( spell_t* s )
{
  if( ! s -> harmful ) return;

  int16_t& buff = s -> player -> buffs.talisman_of_ascendance;

  if( buff != 0 )
  {
    if( buff < 200 )
    {
      buff = ( buff < 0 ) ? 40 : buff+40;
      s -> player -> spell_power[ SCHOOL_MAX ] += 40;
    }
    else 
    {
      s -> player -> spell_power[ SCHOOL_MAX ] -= buff;
      buff = 0;
    }
  } 
}

// Zandalarian Hero Charm ===================================================

static void trigger_zandalarian_hero_charm( spell_t* s )
{
  if( ! s -> harmful ) return;

  if( s -> player -> buffs.zandalarian_hero_charm > 0 )
  {
    s -> player -> buffs.zandalarian_hero_charm  -= 17;
    s -> player -> spell_power[ SCHOOL_MAX ] -= 17;
  } 
}

// Mark of Defiance =========================================================

static void trigger_mark_of_defiance( spell_t* s )
{
  if( ! s -> harmful ) return;

  player_t* p = s -> player;

  if( p -> gear.mark_of_defiance                                    && 
      s -> sim -> cooldown_ready( p -> cooldowns.mark_of_defiance ) && 
      rand_t::roll( 0.15 ) )
  {
    {
      p -> procs.mark_of_defiance -> occur();
      p -> resource_gain( RESOURCE_MANA, 150.0, p -> gains.mark_of_defiance );
      p -> cooldowns.mark_of_defiance = s -> sim -> current_time + 15;
    }
  }
}

// Violet Eye ================================================================

static void trigger_violet_eye( spell_t* s )
{
  player_t* p = s -> player;

  if( p -> buffs.violet_eye == 0 ) return;

  if( p -> buffs.violet_eye < 0 )
    p -> buffs.violet_eye = 1;
  else
    p -> buffs.violet_eye++;

  p -> mp5 += 21;

  if( p -> buffs.violet_eye == 1 ) p -> aura_gain( "Violet Eye" );
}

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Gear
// ==========================================================================

// unique_gear_t::spell_miss_event ==========================================

void unique_gear_t::spell_miss_event( spell_t* s )
{
  trigger_eye_of_magtheridon( s );
}

// unique_gear_t::spell_hit_event ===========================================

void unique_gear_t::spell_hit_event( spell_t* s )
{
  trigger_darkmoon_crusade ( s );
  trigger_darkmoon_wrath   ( s );
  trigger_elder_scribes    ( s );
  trigger_eternal_sage     ( s );
  trigger_mark_of_defiance ( s );
  trigger_mystical_skyfire ( s );
  trigger_quagmirrans_eye  ( s );
  trigger_spellstrike      ( s );
  trigger_violet_eye       ( s );
  trigger_wrath_of_cenarius( s );

  if( s -> result == RESULT_CRIT )
  {
    clear_darkmoon_wrath                ( s );
    trigger_lightning_capacitor         ( s );
    trigger_sextant_of_unstable_currents( s );
    trigger_shiffars_nexus_horn         ( s );
  }
}

// unique_gear_t::spell_tick_event ==========================================

void unique_gear_t::spell_tick_event( spell_t* s )
{
  trigger_timbals_crystal( s );
}

// unique_gear_t::spell_finish_event ========================================

void unique_gear_t::spell_finish_event( spell_t* s )
{
  trigger_talisman_of_ascendance( s );
  trigger_zandalarian_hero_charm( s );
}

// ==========================================================================
// Attack Power Trinket Action
// ==========================================================================

struct attack_power_trinket_t : public action_t
{
  double attack_power, length, cooldown;
  
  attack_power_trinket_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "attack_power_trinket", p )
  {
    option_t options[] =
    {
      { "power",    OPT_FLT, &attack_power },
      { "length",   OPT_FLT, &length       },
      { "cooldown", OPT_FLT, &cooldown     },
      { NULL }
    };
    parse_options( options, options_str );

    if( attack_power <= 0 ||
	length       <= 0 ||
	cooldown     <= 0 )
    {
      fprintf( sim -> output_file, "Expected format: attack_power_trinket,power=X,length=Y,cooldown=Z\n" );
      assert(0);
    }
    trigger_gcd = 0;
    harmful = false;
  }
   
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      attack_power_trinket_t* trinket;

      expiration_t( sim_t* sim, attack_power_trinket_t* t ) : event_t( sim, t -> player ), trinket( t )
      {
	name = "Attack Power Trinket Expiration";
	player -> aura_gain( "Attack Power Trinket" );
	player -> attack_power += trinket -> attack_power;
	sim -> add_event( this, trinket -> length );
      }
      virtual void execute()
      {
	player -> aura_loss( "Attack Power Trinket" );
	player -> attack_power -= trinket -> attack_power;
      }
    };
  
    if( sim -> log ) report_t::log( sim, "Player %s uses Attack Power Trinket", player -> name() );
    cooldown_ready = player -> sim -> current_time + cooldown;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, length );
    new expiration_t( sim, this );
  }
};

// ==========================================================================
// Spell Power Trinket Action
// ==========================================================================

struct spell_power_trinket_t : public action_t
{
  double spell_power, length, cooldown;
  
  spell_power_trinket_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "spell_power_trinket", p )
  {
    option_t options[] =
    {
      { "power",    OPT_FLT, &spell_power },
      { "length",   OPT_FLT, &length      },
      { "cooldown", OPT_FLT, &cooldown    },
      { NULL }
    };
    parse_options( options, options_str );

    if( spell_power <= 0 ||
	length      <= 0 ||
	cooldown    <= 0 )
    {
      fprintf( sim -> output_file, "Expected format: spell_power_trinket,power=X,length=Y,cooldown=Z\n" );
      assert(0);
    }
    trigger_gcd = 0;
    harmful = false;
  }
   
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      spell_power_trinket_t* trinket;

      expiration_t( sim_t* sim, spell_power_trinket_t* t ) : event_t( sim, t -> player ), trinket( t )
      {
	name = "Spell Power Trinket Expiration";
	player -> aura_gain( "Spell Power Trinket" );
	player -> spell_power[ SCHOOL_MAX ] += trinket -> spell_power;
	sim -> add_event( this, trinket -> length );
      }
      virtual void execute()
      {
	player -> aura_loss( "Spell Power Trinket" );
	player -> spell_power[ SCHOOL_MAX ] -= trinket -> spell_power;
      }
    };
  
    if( sim -> log ) report_t::log( sim, "Player %s uses Spell Power Trinket", player -> name() );
    cooldown_ready = player -> sim -> current_time + cooldown;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, length );
    new expiration_t( sim, this );
  }
};

// ==========================================================================
// Haste Trinket Action
// ==========================================================================

struct haste_trinket_t : public action_t
{
  int16_t haste_rating;
  double length, cooldown;
  
  haste_trinket_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "haste_trinket", p )
  {
    option_t options[] =
    {
      { "rating",   OPT_INT16, &haste_rating },
      { "length",   OPT_FLT,   &length       },
      { "cooldown", OPT_FLT,   &cooldown     },
      { NULL }
    };
    parse_options( options, options_str );

    if( haste_rating <= 0 ||
	length       <= 0 ||
	cooldown     <= 0 )
    {
      fprintf( sim -> output_file, "Expected format: haste_trinket,rating=X,length=Y,cooldown=Z\n" );
      assert(0);
    }
    trigger_gcd = 0;
    harmful = false;
  }
   
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      haste_trinket_t* trinket;

      expiration_t( sim_t* sim, haste_trinket_t* t ) : event_t( sim, t -> player ), trinket( t )
      {
	name = "Haste Trinket Expiration";
	player -> aura_gain( "Haste Trinket" );
	player -> haste_rating += trinket -> haste_rating;
	player -> recalculate_haste();
	sim -> add_event( this, trinket -> length );
      }
      virtual void execute()
      {
	player -> aura_loss( "Haste Trinket" );
	player -> haste_rating -= trinket -> haste_rating;
	player -> recalculate_haste();
      }
    };
  
    if( sim -> log ) report_t::log( sim, "Player %s uses Haste Trinket", player -> name() );
    cooldown_ready = player -> sim -> current_time + cooldown;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, length );
    new expiration_t( sim, this );
  }
};

// ==========================================================================
// Talisman of Ascendance Action
// ==========================================================================

struct talisman_of_ascendance_t : public action_t
{
  struct talisman_of_ascendance_expiration_t : public event_t
  {
    talisman_of_ascendance_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Talisman of Ascendance Expiration";
      sim -> add_event( this, 20.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Talisman of Ascendance" );
      int16_t& buff = player -> buffs.talisman_of_ascendance;
      if( buff > 0 ) player -> spell_power[ SCHOOL_MAX ] -= buff;
      buff = 0;
    }
  };
  
  talisman_of_ascendance_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "talisman_of_ascendance", p )
  {
    cooldown_group = "spell_power_trinket";
    trigger_gcd = 0;
    harmful = false;
  }
  
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "Player %s uses Talisman of Ascendance", player -> name() );
    player -> aura_gain( "Talisman of Ascendance" );
    player -> buffs.talisman_of_ascendance = -1;
    cooldown_ready = sim -> current_time + 60.0;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, 20.0 );
    new talisman_of_ascendance_expiration_t( sim, player );
  }
};

// ==========================================================================
// Zandalarian Hero Charm Action
// ==========================================================================

struct zandalarian_hero_charm_t : public action_t
{
  struct zandalarian_hero_charm_expiration_t : public event_t
  {
    zandalarian_hero_charm_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Zandalarian Hero Charm Expiration";
      sim -> add_event( this, 20.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Zandalarian Hero Charm" );
      player -> spell_power[ SCHOOL_MAX ] -= player -> buffs.zandalarian_hero_charm;
      player -> buffs.zandalarian_hero_charm = 0;
    }
  };
  
  zandalarian_hero_charm_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "zandalarian_hero_charm", p )
  {
    cooldown_group = "spell_power_trinket";
    trigger_gcd = 0;
    harmful = false;
  }
  
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "Player %s uses Zandalarian Hero Charm", player -> name() );
    player -> aura_gain( "Zandalarian Hero Charm" );
    player -> buffs.zandalarian_hero_charm = 204;
    player -> spell_power[ SCHOOL_MAX ] += player -> buffs.zandalarian_hero_charm;
    cooldown_ready = sim -> current_time + 120.0;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, 20.0 );
    new zandalarian_hero_charm_expiration_t( sim, player );
  }
};

// ==========================================================================
// Hazzrahs Charm Spell
// ==========================================================================

struct hazzrahs_charm_t : public action_t
{
  struct hazzrahs_charm_expiration_t : public event_t
  {
    hazzrahs_charm_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Hazzrahs Charm Expiration";
      p -> aura_gain( "Hazzarahs Charm" );
      double duration=0;
      switch( p -> type )
      {
      case MAGE:    duration = 20.0; p -> spell_power[ SCHOOL_ARCANE ] += 200;            break;
      case PRIEST:  duration = 15.0; p -> haste_rating += 400;  p -> recalculate_haste(); break;
      case WARLOCK: duration = 20.0; p -> spell_crit += 140 / ( p -> rating.spell_crit ); break;
      }
      sim -> add_event( this, duration );
    }
    virtual void execute()
    {
      player_t* p = player;
      p -> aura_loss( "Hazzarahs Charm" );
      switch( p -> type )
      {
      case MAGE:    p -> spell_power[ SCHOOL_ARCANE ] -= 200;            break;
      case PRIEST:  p -> haste_rating -= 400;  p -> recalculate_haste(); break;
      case WARLOCK: p -> spell_crit -= 140 / ( p -> rating.spell_crit ); break;
      }
    }
  };
  
  hazzrahs_charm_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "hazzrahs_charm", p )
  {
    cooldown_group = "spell_power_trinket";
    trigger_gcd = 0;
    harmful = false;

    // This trinket is only availables to Mages, Priests, and Warlocks.
    if( p -> type != MAGE    &&
	p -> type != PRIEST  &&
	p -> type != WARLOCK )
    {
      assert( 0 );
    }
  }
  
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "Player %s uses Hazzrahs Charm", player -> name() );
    cooldown_ready = sim -> current_time + 180.0;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, 20.0 );
    new hazzrahs_charm_expiration_t( sim, player );
  }
};

// ==========================================================================
// Violet Eye Spell
// ==========================================================================

struct violet_eye_t : public action_t
{
  struct violet_eye_expiration_t : public event_t
  {
    violet_eye_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Violet Eye Expiration";
      p -> aura_gain( "Violet Eye" );
      p -> buffs.violet_eye = -1;
      sim -> add_event( this, 20.0 );
    }
    virtual void execute()
    {
      player_t* p = player;
      p -> aura_loss( "Violet Eye" );
      p -> mp5 -= p -> buffs.violet_eye * 21;
      p -> buffs.violet_eye = 0;
    }
  };

  violet_eye_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "violet_eye", p )
  {
    cooldown_group = "mana_trinket";
    harmful = false;
    trigger_gcd = 0;
  }
  
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "Player %s uses Pendant of the Violet Eye", player -> name() );
    cooldown_ready = sim -> current_time + 120.0;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, 20.0 );
    new violet_eye_expiration_t( sim, player );
  }
};

// ==========================================================================
// unique_gear_t::create_action
// ==========================================================================

action_t* unique_gear_t::create_action( player_t*          p,
					const std::string& name, 
					const std::string& options_str )
{
  if( name == "attack_power_trinket"   ) return new attack_power_trinket_t  ( p, options_str );
  if( name == "haste_trinket"          ) return new haste_trinket_t         ( p, options_str );
  if( name == "hazzrahs_charm"         ) return new hazzrahs_charm_t        ( p, options_str );
  if( name == "spell_power_trinket"    ) return new spell_power_trinket_t   ( p, options_str );
  if( name == "talisman_of_ascendance" ) return new talisman_of_ascendance_t( p, options_str );
  if( name == "violet_eye"             ) return new violet_eye_t            ( p, options_str );
  if( name == "zandalarian_hero_charm" ) return new zandalarian_hero_charm_t( p, options_str );

  return 0;
}
