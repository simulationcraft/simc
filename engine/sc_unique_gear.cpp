// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// stat_proc_callback =======================================================

struct stat_proc_callback_t : public action_callback_t
{
  std::string name_str;
  int stat;
  double amount;
  double tick;
  stat_buff_t* buff;

  stat_proc_callback_t( const std::string& n, player_t* p, int s, int max_stacks, double a, 
			double proc_chance, double duration, double cooldown, double t, int rng_type=RNG_DEFAULT ) :
      action_callback_t( p -> sim, p ),
      name_str( n ), stat( s ), amount( a ), tick( t )
  {
    if ( max_stacks == 0 ) max_stacks = 1;
    if ( proc_chance == 0 ) proc_chance = 1;
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_DISTRIBUTED;

    buff = new stat_buff_t( p, n, stat, amount, max_stacks, duration, cooldown, proc_chance, false, rng_type );
  }

  virtual void deactivate()
  {
    action_callback_t::deactivate();
    buff -> expire();
  }

  virtual void trigger( action_t* a )
  {
    if ( buff -> trigger() )
    {
      if ( tick > 0 ) // The buff stacks over time.
      {
	struct tick_stack_t : public event_t
	{
	  stat_proc_callback_t* callback;
	  tick_stack_t( sim_t* sim, player_t* p, stat_proc_callback_t* cb ) : event_t( sim, p ), callback( cb )
	  {
	    name = callback -> buff -> name();
	    sim -> add_event( this, callback -> tick );
	  }
	  virtual void execute()
	  {
	    stat_buff_t* b = callback -> buff;
	    if ( b -> current_stack > 0 &&
		 b -> current_stack < b -> max_stack )
	    {
	      b -> bump();
	      new ( sim ) tick_stack_t( sim, player, callback );
	    }
	  }
	};
	
	new ( sim ) tick_stack_t( sim, a -> player, this );
      }
    }
  }
};

// discharge_proc_callback ==================================================

struct discharge_proc_callback_t : public action_callback_t
{
  std::string name_str;
  int stacks, max_stacks;
  double proc_chance;
  double cooldown, cooldown_ready;
  spell_t* spell;
  proc_t* proc;
  rng_t* rng;

  discharge_proc_callback_t( const std::string& n, player_t* p, int ms, int school, double min, double max, double pc, double cd, int rng_type=RNG_DEFAULT ) :
      action_callback_t( p -> sim, p ),
      name_str( n ), stacks( 0 ), max_stacks( ms ), proc_chance( pc ), cooldown( cd ), cooldown_ready( 0 )
  {
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_DISTRIBUTED;

    struct discharge_spell_t : public spell_t
    {
      discharge_spell_t( const char* n, player_t* p, double min, double max, int s ) :
          spell_t( n, p, RESOURCE_NONE, s )
      {
        trigger_gcd = 0;
        base_dd_min = min;
        base_dd_max = max;
        may_crit    = true;
        background  = true;
        base_spell_power_multiplier = 0;
        reset();
      }
    };

    spell = new discharge_spell_t( name_str.c_str(), p, min, max, school );

    proc = p -> get_proc( name_str.c_str(), sim );
    rng  = p -> get_rng ( name_str.c_str(), rng_type );  // default is CYCLIC since discharge should not have duration
  }

  virtual void reset() { stacks=0; cooldown_ready=0; }

  virtual void deactivate() { action_callback_t::deactivate(); stacks=0; }

  virtual void trigger( action_t* a )
  {
    if ( cooldown )
      if ( sim -> current_time < cooldown_ready )
        return;

    if ( proc_chance )
      if ( ! rng -> roll( proc_chance ) )
        return;

    if ( cooldown )
      cooldown_ready = sim -> current_time + cooldown;

    if ( ++stacks < max_stacks )
    {
      listener -> aura_gain( name_str.c_str() );
    }
    else
    {
      stacks = 0;
      spell -> execute();
      proc -> occur();
    }
  }
};

// ==========================================================================
// unique_gear_t::init
// ==========================================================================

void unique_gear_t::init( player_t* p )
{
  if ( p -> is_pet() ) return;

  int num_items = ( int ) p -> items.size();

  for ( int i=0; i < num_items; i++ )
  {
    item_t& item = p -> items[ i ];

    if ( item.equip.stat )
    {
      register_stat_proc( item, item.equip );
    }
    else if ( item.equip.school )
    {
      register_discharge_proc( item, item.equip );
    }
  }

  if ( item_t* item = p -> find_item( "darkmoon_card_greatness" ) )
  {
    item -> unique = true;

    int attr[] = { ATTR_STRENGTH, ATTR_AGILITY, ATTR_INTELLECT, ATTR_SPIRIT };
    int stat[] = { STAT_STRENGTH, STAT_AGILITY, STAT_INTELLECT, STAT_SPIRIT };

    int max_stat=-1;
    double max_value=0;

    for ( int i=0; i < 4; i++ )
    {
      if ( p -> attribute[ attr[ i ] ] > max_value )
      {
        max_value = p -> attribute[ attr[ i ] ];
        max_stat = stat[ i ];
      }
    }
    action_callback_t* cb = new stat_proc_callback_t( "darkmoon_card_greatness", p, max_stat, 1, 300, 0.35, 15.0, 45.0, 0 );

    p -> register_tick_damage_callback( cb );
    p -> register_direct_damage_callback( cb );
  }

  if ( p -> find_item( "black_bruise" ) )
  {
    // http://ptr.wowhead.com/?item=50035
    buff_t* buff = new buff_t( p, "black_bruise", 1, 10, 0.0, 0.01 ); // FIXME!! Cooldown?

    // http://ptr.wowhead.com/?spell=71876
    // http://ptr.wowhead.com/?spell=71878 <- HEROIC VERSION
    struct black_bruise_spell_t : public spell_t
    {
      black_bruise_spell_t( player_t* player ) : spell_t( "black_bruise", player, RESOURCE_NONE, SCHOOL_SHADOW )
      {
        may_miss    = false;
        background  = true;
        proc        = true;
        trigger_gcd = 0;
        base_cost   = 0;
        base_dd_min = base_dd_max = 1;
	base_dd_multiplier *= 0.10; // FIXME!!  Need better way to distinguish "heroic" items.
        reset();
      }
      virtual void player_buff() { }
    };

    struct black_bruise_trigger_t : public action_callback_t
    {
      buff_t* buff;
      black_bruise_trigger_t( player_t* p, buff_t* b ) : action_callback_t( p -> sim, p ), buff(b) {}
      virtual void trigger( action_t* a )
      {
	if ( ! a -> weapon ) return;
	if ( a -> weapon -> slot != SLOT_MAIN_HAND ) return;
	buff -> trigger();
      }
    };

    struct black_bruise_damage_t : public action_callback_t
    {
      buff_t* buff;
      spell_t* spell;
      black_bruise_damage_t( player_t* p, buff_t* b, spell_t* s ) : action_callback_t( p -> sim, p ), buff(b), spell(s) {}
      virtual void trigger( action_t* a )
      {
	if ( ! a -> weapon ) return;
	if ( buff -> up() )
	{
	  spell -> base_dd_adder = a -> direct_dmg;
	  spell -> execute();
	}
      }
    };

    p -> register_attack_result_callback( RESULT_HIT_MASK, new black_bruise_trigger_t( p, buff ) );
    p -> register_direct_damage_callback( new black_bruise_damage_t( p, buff, new black_bruise_spell_t( p ) ) );
  }

  for ( int i=0; i < num_items; i++ )
  {
    std::string str = p -> items[ i ].name();

    if ( str == "deaths_choice" || str == "deaths_verdict" )
    {
      item_t* item = &( p -> items[ i ] );

      item -> unique = true;

      double value = ( item -> encoded_stats_str == "288ap" ) ? 510.0 : 450.0;

      int stat = ( p -> attribute[ ATTR_STRENGTH ] > p -> attribute[ ATTR_AGILITY ] ) ? STAT_STRENGTH : STAT_AGILITY;

      action_callback_t* cb = new stat_proc_callback_t( str, p, stat, 1, value, 0.35, 15.0, 45.0, 0 );

      p -> register_tick_damage_callback( cb );
      p -> register_direct_damage_callback( cb );
    }
  }

  if ( p -> set_bonus.spellstrike() )
  {
    unique_gear_t::register_stat_proc( PROC_SPELL, RESULT_HIT_MASK, "spellstrike", p, STAT_SPELL_POWER, 1, 92, 0.05, 10.0, 0, 0 );
  }
}

// ==========================================================================
// unique_gear_t::register_stat_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_stat_proc( int                type,
                                                      int                mask,
                                                      const std::string& name,
                                                      player_t*          player,
                                                      int                stat,
                                                      int                max_stacks,
                                                      double             amount,
                                                      double             proc_chance,
                                                      double             duration,
                                                      double             cooldown,
						      double             tick,
                                                      int                rng_type )
{
  action_callback_t* cb = new stat_proc_callback_t( name, player, stat, max_stacks, amount, proc_chance, duration, cooldown, tick, rng_type );

  if ( type == PROC_DAMAGE )
  {
    player -> register_tick_damage_callback( cb );
    player -> register_direct_damage_callback( cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> register_attack_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> register_spell_result_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK_DIRECT )
  {
    player -> register_attack_direct_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT )
  {
    player -> register_spell_direct_result_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear_t::register_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_discharge_proc( int                type,
                                                           int                mask,
                                                           const std::string& name,
                                                           player_t*          player,
                                                           int                max_stacks,
                                                           int                school,
                                                           double             min_dmg,
                                                           double             max_dmg,
                                                           double             proc_chance,
                                                           double             cooldown,
                                                           int                rng_type )
{
  action_callback_t* cb = new discharge_proc_callback_t( name, player, max_stacks, school, min_dmg, max_dmg, proc_chance, cooldown, rng_type );

  if ( type == PROC_DAMAGE )
  {
    player -> register_tick_damage_callback( cb );
    player -> register_direct_damage_callback( cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> register_tick_callback( cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> register_attack_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> register_spell_result_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK_DIRECT )
  {
    player -> register_attack_direct_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT )
  {
    player -> register_spell_direct_result_callback( mask, cb );
  }
  return cb;
}

// ==========================================================================
// unique_gear_t::register_stat_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_stat_proc( item_t& i, 
						      item_t::special_effect_t& e )
{
  const char* name = e.name_str.empty() ? i.name() : e.name_str.c_str();

  return register_stat_proc( e.trigger_type, e.trigger_mask, name, i.player, e.stat, e.max_stacks, e.amount, e.proc_chance, e.duration, e.cooldown, e.tick );
}

// ==========================================================================
// unique_gear_t::register_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_discharge_proc( item_t& i, 
							   item_t::special_effect_t& e )
{
  const char* name = e.name_str.empty() ? i.name() : e.name_str.c_str();

  return register_discharge_proc( e.trigger_type, e.trigger_mask, name, i.player, e.max_stacks, e.school, e.amount, e.amount, e.proc_chance, e.cooldown );
}

// ==========================================================================
// unique_gear_t::get_equip_encoding
// ==========================================================================

bool unique_gear_t::get_equip_encoding( std::string&       encoding,
                                        const std::string& name,
                                        const std::string& id )
{
  std::string e;

  // Stat Procs
  if      ( name == "abyssal_rune"                    ) e = "OnSpellHit_590SP_25%_10Dur_45Cd";
  else if ( name == "banner_of_victory"               ) e = "OnAttackHit_1008AP_20%_10Dur_50Cd";
  else if ( name == "black_magic"                     ) e = "OnSpellDirectHit_250Haste_35%_10Dur_35Cd";
  else if ( name == "blood_of_the_old_god"            ) e = "OnAttackCrit_1284AP_10%_10Dur_50Cd";
  else if ( name == "chuchus_tiny_box_of_horrors"     ) e = "OnAttackHit_258Crit_15%_10Dur_45Cd";
  else if ( name == "comets_trail"                    ) e = "OnAttackHit_726Haste_10%_10Dur_45Cd";
  else if ( name == "dark_matter"                     ) e = "OnAttackHit_612Crit_15%_10Dur_45Cd";
  else if ( name == "darkmoon_card_crusade"           ) e = "OnSpellHit_8SP_10Stack_10Dur";
  else if ( name == "dying_curse"                     ) e = "OnSpellHit_765SP_15%_10Dur_45Cd";
  else if ( name == "elemental_focus_stone"           ) e = "OnSpellHit_522Haste_10%_10Dur_45Cd";
  else if ( name == "embrace_of_the_spider"           ) e = "OnSpellHit_505Haste_10%_10Dur_45Cd";
  else if ( name == "eye_of_magtheridon"              ) e = "OnSpellMiss_170SP_10Dur";
  else if ( name == "eye_of_the_broodmother"          ) e = "OnSpellHit_25SP_5Stack_10Dur";
  else if ( name == "flare_of_the_heavens"            ) e = "OnSpellCast_850SP_10%_10Dur_45Cd";
  else if ( name == "forge_ember"                     ) e = "OnSpellHit_512SP_10%_10Dur_45Cd";
  else if ( name == "fury_of_the_five_flights"        ) e = "OnAttackHit_16AP_20Stack_10Dur";
  else if ( name == "grim_toll"                       ) e = "OnAttackHit_612ArPen_15%_10Dur_45Cd";
  else if ( name == "herkuml_war_token"               ) e = "OnAttackHit_17AP_20Stack_10Dur";
  else if ( name == "illustration_of_the_dragon_soul" ) e = "OnSpellHit_20SP_10Stack_10Dur";
  else if ( name == "mark_of_defiance"                ) e = "OnSpellHit_150Mana_15%_15Cd";
  else if ( name == "mirror_of_truth"                 ) e = "OnAttackCrit_1000AP_10%_10Dur_50Cd";
  else if ( name == "mithril_pocketwatch"             ) e = "OnSpellCast_590SP_10%_10Dur_45Cd";
  else if ( name == "mjolnir_runestone"               ) e = "OnAttackHit_665ArPen_15%_10Dur_45Cd";
  else if ( name == "muradins_spyglass"               ) e = ( id == "50340" ? "OnSpellHit_18SP_10Stack_10Dur" : "OnSpellHit_20SP_10Stack_10Dur" );
  else if ( name == "needle_encrusted_scorpion"       ) e = "OnAttackHit_452ArPen_15%_10Dur_45Cd";
  else if ( name == "pandoras_plea"                   ) e = "OnSpellHit_751SP_10%_10Dur_45Cd";
  else if ( name == "pyrite_infuser"                  ) e = "OnAttackCrit_1234AP_10%_10Dur_50Cd";
  else if ( name == "quagmirrans_eye"                 ) e = "OnSpellHit_320Haste_10%_6Dur_45Cd";
  else if ( name == "sextant_of_unstable_currents"    ) e = "OnSpellCrit_190SP_20%_15Dur_45Cd";
  else if ( name == "shiffars_nexus_horn"             ) e = "OnSpellCrit_225SP_20%_10Dur_45Cd";
  else if ( name == "sundial_of_the_exiled"           ) e = "OnSpellCast_590SP_10%_10Dur_45Cd";
  else if ( name == "wrath_of_cenarius"               ) e = "OnSpellHit_132SP_5%_10Dur";

  // Stat Procs with Tick Increases
  else if ( name == "dislodged_foreign_object" ) e = "OnSpellCast_105SP_10Stack_10%_20Dur_45Cd_2Tick";

  // Discharge Procs
  else if ( name == "bandits_insignia"             ) e = "OnAttackHit_1880Arcane_15%_45Cd";
  else if ( name == "extract_of_necromantic_power" ) e = "OnTick_1050Shadow_10%_15Cd";
  else if ( name == "lightning_capacitor"          ) e = "OnSpellDirectCrit_750Nature_3Stack_2.5Cd";
  else if ( name == "timbals_crystal"              ) e = "OnTick_380Shadow_10%_15Cd";
  else if ( name == "thunder_capacitor"            ) e = "OnSpellDirectCrit_1276Nature_4Stack_2.5Cd";

  // Some Normal/Heroic items have same name
  else if ( name == "reign_of_the_unliving" ) e = ( id == "47182" ? "OnSpellDirectCrit_1882Fire_3Stack_2.0Cd" : "OnSpellDirectCrit_2117Fire_3Stack_2.0Cd" );
  else if ( name == "reign_of_the_dead"     ) e = ( id == "47316" ? "OnSpellDirectCrit_1882Fire_3Stack_2.0Cd" : "OnSpellDirectCrit_2117Fire_3Stack_2.0Cd" );
  
  /* Icecrown Trinkets, names currently not known but the spells are. The % and 
     Cd values are assumptions taken from other trinkets with similar procs
  else if ( name == "" ) e = "OnSpellHit_18SP_10Stack_10Dur";
  else if ( name == "" ) e = "OnSpellHit_20SP_10Stack_10Dur";
  else if ( name == "" ) e = "OnTick_910SP_10%_45Cd";
  else if ( name == "" ) e = "OnSpellHit_100MP5_10%_45Cd_15Dur";
  else if ( name == "" ) e = "OnAttackHit_1250AP_10%_10Dur_50Cd";
  
  * PTR trinkets where it's not fully clear how to function or how they determine
    which of theirs buffs they procc
  
  * Item - Icecrown 25 Normal Caster Trinket 1 Base - Your harmful spells 
    have a chance to increase your spell power by 32 and an additional 32
    every 2 sec for 20 sec.
    Stacks itself over time? Have to see PTR.
  
  * Your attacks have a chance to awaken the powers of the races of Northrend,
    temporarily transforming you and increasing your combat capabilities for 
    30 sec.
    There are _6_ buffs associated to this proc:
    +600 Agi/Str/ArPen/Crit/Haste and +1200 AP (heroic: +700/+1400)
    Have to see if it is random or not at which buffs it procs.
  */

  // Enchants
  else if ( name == "lightweave"            ) e = "OnSpellHit_295SP_35%_15Dur_60Cd";  // temporary for backwards compatibility
  else if ( name == "lightweave_embroidery" ) e = "OnSpellHit_295SP_35%_15Dur_60Cd";  
  else if ( name == "darkglow_embroidery"   ) e = "OnSpellHit_400Mana_35%_60Cd";
  else if ( name == "swordguard_embroidery" ) e = "OnAttackHit_400AP_25%_60Cd";
  else if ( name == "rune_of_the_fallen_crusader" ) e = "1sp";  // FIX-ME: Placeholder I assume.

  if ( e.empty() ) return false;

  armory_t::format( e );
  encoding = e;

  return true;
}

// ==========================================================================
// unique_gear_t::get_hidden_encoding
// ==========================================================================

bool unique_gear_t::get_hidden_encoding( std::string&       encoding,
                                         const std::string& name,
                                         const std::string& id )
{
  std::string e;
  if      ( name == "lightweave"                 ) e = "1spi";
  else if ( name == "lightweave_embroidery"      ) e = "1spi";  
  else if ( name == "darkglow_embroidery"        ) e = "1spi";

  if ( e.empty() ) return false;

  armory_t::format( e );
  encoding = e;

  return true;
}

// ==========================================================================
// unique_gear_t::get_use_encoding
// ==========================================================================

bool unique_gear_t::get_use_encoding( std::string&       encoding,
                                      const std::string& name,
                                      const std::string& id )
{
  std::string e;

  // Simple
  if      ( name == "energy_siphon"               ) e = "408SP_20Dur_120Cd";
  else if ( name == "ephemeral_snowflake"         ) e = "464Haste_20Dur_120Cd";
  else if ( name == "living_flame"                ) e = "505SP_20Dur_120Cd";
  else if ( name == "maghias_misguided_quill"     ) e = "716SP_20Dur_120Cd";
  else if ( name == "mark_of_norgannon"           ) e = "491Haste_20Dur_120Cd";
  else if ( name == "mark_of_supremacy"           ) e = "1024AP_20Dur_120Cd";
  else if ( name == "platinum_disks_of_battle"    ) e = "752AP_20Dur_120Cd";
  else if ( name == "platinum_disks_of_sorcery"   ) e = "440SP_20Dur_120Cd";
  else if ( name == "platinum_disks_of_swiftness" ) e = "375Haste_20Dur_120Cd";
  else if ( name == "scale_of_fates"              ) e = "432Haste_20Dur_120Cd";
  else if ( name == "shard_of_the_crystal_heart"  ) e = "512Haste_20Dur_120Cd";
  else if ( name == "spirit_world_glass"          ) e = "336Spi_20Dur_120Cd";
  else if ( name == "talisman_of_resurgence"      ) e = "599SP_20Dur_120Cd";
  else if ( name == "wrathstone"                  ) e = "856AP_20Dur_120Cd";

  // Hybrid
  else if ( name == "fetish_of_volatile_power"   ) e = ( id == "47879" ? "OnSpellHit_57Haste_8Stack_20Dur_120Cd" : "OnSpellHit_64Haste_8Stack_20Dur_120Cd" );
  else if ( name == "talisman_of_volatile_power" ) e = ( id == "47726" ? "OnSpellHit_57Haste_8Stack_20Dur_120Cd" : "OnSpellHit_64Haste_8Stack_20Dur_120Cd" );
  else if ( name == "vengeance_of_the_forsaken"  ) e = ( id == "47881" ? "OnAttackHit_215AP_5Stack_20Dur_120Cd"  : "OnAttackHit_250AP_5Stack_20Dur_120Cd"  );
  else if ( name == "victors_call"               ) e = ( id == "47725" ? "OnAttackHit_215AP_5Stack_20Dur_120Cd"  : "OnAttackHit_250AP_5Stack_20Dur_120Cd"  );

  // Enchants
  else if ( name == "pyrorocket"                   ) e = "1837Fire_45Cd";  // temporary for backwards compatibility
  else if ( name == "hand_mounted_pyro_rocket"     ) e = "1837Fire_45Cd";
  else if ( name == "hyperspeed_accelerators"      ) e = "340Haste_12Dur_60Cd";

  if ( e.empty() ) return false;

  armory_t::format( e );
  encoding = e;

  return true;
}

