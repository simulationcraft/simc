// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

/*
 * Log Cataclysm
 *
 * Build 12857
 * - changed veiled shadows to 30s
 * - added sin and punishment
 *
 * Build 12803:
 * - changed darkness
 * - removed surge_of_light
 * - changed talent tree
 * - added Talent Holy Concentration
 * - added Shadowy Apparation
 * - finished Inner Will
 * - Mind Spike Debuff
 *
 *
 *
 * To do List:
 * Shadow:
 *
 * Other:
 * - Talent Revelations
 * - Talent Twirling Light
 * - Spell Holy Word: Chastice
 * - Spell Chakra Cooldown
 * - Talent State of Mind: implement function to increase/refresh duration of chakra by 4s
 */

#include "simulationcraft.h"

// ==========================================================================
// Priest
// ==========================================================================

  struct priest_t : public player_t
  {
    // Buffs
    buff_t* buffs_glyph_of_shadow;
    buff_t* buffs_inner_fire;
    buff_t* buffs_inner_fire_armor;
    buff_t* buffs_inner_will;
    buff_t* buffs_shadow_form;
    buff_t* buffs_vampiric_embrace;
    buff_t* buffs_mind_melt;
    buff_t* buffs_dark_evangelism;
    buff_t* buffs_holy_evangelism;
    buff_t* buffs_shadow_orb;
    buff_t* buffs_dark_archangel;
    buff_t* buffs_holy_archangel;
    buff_t* buffs_chakra_pre;
    buff_t* buffs_chakra;
    buff_t* buffs_mind_spike;

    // Talents

	// Discipline
    talent_t* talent_archangel; // 					complete 12803
    talent_t* talent_evangelism; // 					complete 12803
    talent_t* talent_inner_focus; //      new: 12857
    talent_t* talent_improved_inner_fire; //
    talent_t* talent_mental_agility; // 															open: better implementation
    talent_t* talent_power_infusion; // 				complete 12803
    talent_t* talent_twin_disciplines; // 			complete 12803

	// Holy
    talent_t* talent_divine_fury; // 				complete 12803
    talent_t* talent_chakra; // 						done: basic implementation 12759			incomplete: trigger 60s cooldown on chakra_t just when smite_t hits
    talent_t* talent_state_of_mind; // 															incomplete: implement a function to increase the duration of a buff
    talent_t* talent_holy_concentration; //			complete 12803

	// Shadow
    talent_t* talent_darkness; // 					complete 12803
    talent_t* talent_improved_devouring_plague;  // 	complete 12803
    talent_t* talent_improved_mind_blast; // 		complete 12803
    talent_t* talent_mind_melt; // 					complete 12803
    talent_t* talent_dispersion; // 					complete 12803
    talent_t* talent_improved_shadow_word_pain; // 	complete 12803
    talent_t* talent_pain_and_suffering; // 			complete 12803
    talent_t* talent_masochism; // new 12857

    talent_t* talent_shadow_form; // 				complete 12803
    talent_t* talent_twisted_faith; // 				complete 12803
    talent_t* talent_veiled_shadows; // 				complete 12803
    talent_t* talent_harnessed_shadows; //  			complete 12803  
    talent_t* talent_shadowy_apparation; // 			done: talent function 12803
    talent_t* talent_vampiric_embrace; // 			complete 12803
    talent_t* talent_vampiric_touch; // 				complete 12803
    talent_t* talent_sin_and_punishment;

    // Talent Specializations
    // Coded as talent_t for now but technically they're not.
    spell_ids_t* talent_penance;
    spell_ids_t* talent_chastise;     // incomplete
    spell_ids_t* talent_shadow_power; //       			done: talent function 12803					incomplete: link with main talent tree
    spell_ids_t* talent_mind_flay;
    spell_ids_t* talent_meditation; // 					done: talent function 12803

    // Cooldowns
    cooldown_t* cooldowns_mind_blast;
    cooldown_t* cooldowns_shadow_fiend;
    cooldown_t* cooldowns_archangel;
    cooldown_t* cooldowns_chakra;

    // DoTs
    dot_t* dots_shadow_word_pain;
    dot_t* dots_vampiric_touch;
    dot_t* dots_devouring_plague;
    dot_t* dots_holy_fire;

    // Gains
    gain_t* gains_dispersion;
    gain_t* gains_glyph_of_shadow_word_pain;
    gain_t* gains_shadow_fiend;
    gain_t* gains_archangel;
    gain_t* gains_masochism;

    // Uptimes
    uptime_t* uptimes_mind_spike[ 4 ];
    uptime_t* uptimes_dark_evangelism[ 6 ];
    uptime_t* uptimes_shadow_orb[ 4 ];

    // Procs
    proc_t* procs_shadowy_apparation;

    // Special
    spell_t* shadowy;

    // Random Number Generators
    rng_t* rng_pain_and_suffering;

    // Options
	std::string power_infusion_target_str;

	// Mana Resource Tracker
	struct mana_resource_t
	  {
		double mana_gain;
		double mana_loss;
		void reset() { memset( ( void* ) this, 0x00, sizeof( mana_resource_t ) ); }

		mana_resource_t() { reset(); }
	  };

	mana_resource_t mana_resource;
	double max_mana_cost;
	std::vector<player_t *> party_list;

	// Glyphs
	struct glyphs_t
	  {
		int dispersion;
		int hymn_of_hope;
		int inner_fire;
		int mind_flay;
		int penance;
		int shadow;
		int shadow_word_death;
		int shadow_word_pain;
		int smite;

		glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
	  };
	glyphs_t glyphs;

	// Constants
	struct constants_t
	  {
		double meditation_value;

		// Discipline
		double twin_disciplines_value;
		double dark_evangelism_value;
		double holy_evangelism_damage_value;
		double holy_evangelism_mana_value;
		double dark_archangel_value;
		double holy_archangel_value;
		double archangel_mana_value;
		double inner_will_value;

		// Holy
		double holy_concentration_value;

		// Shadow
		double shadow_power_value;
		double shadow_orb_proc_value;
		double shadow_orb_damage_value;
		double shadow_orb_mastery_value;

		double darkness_value;
		double improved_shadow_word_pain_value;
		double twisted_faith_static_value;
		double twisted_faith_dynamic_value;
		double shadow_form_value;
		double harnessed_shadows_value;
		double pain_and_suffering_value;

		double mind_spike_crit_value;
		double devouring_plague_health_mod;

		constants_t() { memset( ( void * ) this, 0x0, sizeof( constants_t ) ); }
	  };
	constants_t constants;

	// Power Mods
	struct power_mod_t
      {
		double devouring_plague;
		double mind_blast;
		double mind_flay;
		double mind_spike;
		double shadow_word_death;
		double shadow_word_pain;
		double vampiric_touch;
		double holy_fire;
		double smite;

		power_mod_t() { memset( ( void * ) this, 0x0, sizeof( power_mod_t ) ); }
      };
	power_mod_t power_mod;

	bool   use_shadow_word_death;
	int    use_mind_blast;
	int    recast_mind_blast;

	priest_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, PRIEST, name, r )
	  {
		use_shadow_word_death    = false;
		use_mind_blast           = 1;
		recast_mind_blast        = 0;

		distance	= 36;

		max_mana_cost = 0.0;

		// Constants
		// Discipline/Holy
		constants.meditation_value				  = 0.50;

		// Discipline
		constants.twin_disciplines_value          = 0.02;
		constants.dark_evangelism_value			  = 0.01;
		constants.holy_evangelism_damage_value	  = 0.02;
		constants.holy_evangelism_mana_value	  = 0.03;
		constants.dark_archangel_value			  = 0.03;
		constants.holy_archangel_value			  = 0.03;
		constants.archangel_mana_value			  = 0.03;
		constants.inner_will_value				  = 0.15;

		// Holy
		constants.holy_concentration_value		  = 0.10;

		// Shadow Core
		constants.shadow_power_value			  = 0.25;
		constants.shadow_orb_proc_value           = 0.10;
		constants.shadow_orb_damage_value         = 0.20;
		constants.shadow_orb_mastery_value		  = 0.025;

		// Shadow
		constants.darkness_value                  = 0.01;
		constants.improved_shadow_word_pain_value = 0.03;
		constants.twisted_faith_static_value	  = 0.01;
		constants.twisted_faith_dynamic_value	  = 0.50;
		constants.shadow_form_value               = 0.15;
		constants.harnessed_shadows_value         = 0.04;
		constants.pain_and_suffering_value		  = 0.30;

		constants.mind_spike_crit_value			  = 0.30;
		constants.devouring_plague_health_mod	  = 0.15;

		cooldowns_mind_blast = get_cooldown( "mind_blast" );
		cooldowns_shadow_fiend = get_cooldown( "shadow_fiend" );
		cooldowns_archangel    = get_cooldown( "archangel"   );
		cooldowns_chakra       = get_cooldown( "chakra"   );


		cooldowns_shadow_fiend -> duration 		  = 300.0;
		cooldowns_archangel -> duration 		  = 15.0;
		cooldowns_chakra -> duration 			  = 60.0;


		dots_shadow_word_pain = get_dot( "shadow_word_pain" );
		dots_vampiric_touch   = get_dot( "vampiric_touch" );
		dots_devouring_plague = get_dot( "devouring_plague" );
		dots_holy_fire        = get_dot( "holy_fire" );



		shadowy         = 0;

	  // Discipline
    talent_archangel = new talent_t( this, "Archangel" ); // 					complete 12803
    talent_evangelism = new talent_t( this, "Evangelism" ); // 					complete 12803
    talent_inner_focus = new talent_t( this, "Inner Focus" ); //      new: 12857
    talent_improved_inner_fire = new talent_t( this, "Improved Inner Fire <TBR>" ); //
    talent_mental_agility = new talent_t( this, "Mental Agility" ); // 															open: better implementation
    talent_power_infusion = new talent_t( this, "Power Infusion" ); // 				complete 12803
    talent_twin_disciplines = new talent_t( this, "Twin Disciplines" ); // 			complete 12803

	  // Holy
    talent_divine_fury = new talent_t( this, "Divine Fury" ); // 				complete 12803
    talent_chakra = new talent_t( this, "Chakra" ); // 						done: basic implementation 12759			incomplete: trigger 60s cooldown on chakra_t just when smite_t hits
    talent_state_of_mind = new talent_t( this, "State of Mind" ); // 															incomplete: implement a function to increase the duration of a buff
    talent_holy_concentration = new talent_t( this, "Holy Concentration" ); //			complete 12803

	  // Shadow
    talent_darkness = new talent_t( this, "Darkness" ); // 					complete 12803
    talent_improved_devouring_plague = new talent_t( this, "Improved Devouring Plague" );  // 	complete 12803
    talent_improved_mind_blast = new talent_t( this, "Improved Mind Blast" ); // 		complete 12803
    talent_mind_melt = new talent_t( this, "Mind Melt" ); // 					complete 12803
    talent_dispersion = new talent_t( this, "Dispersion" ); // 					complete 12803
    talent_improved_shadow_word_pain = new talent_t( this, "Improved Shadow Word: Pain" ); // 	complete 12803
    talent_pain_and_suffering = new talent_t( this, "Pain and Suffering" ); // 			complete 12803
    talent_masochism = new talent_t( this, "Masochism" ); // new 12857

    talent_shadow_form = new talent_t( this, "Shadowform" ); // 				complete 12803
    talent_twisted_faith = new talent_t( this, "Twisted Faith" ); // 				complete 12803
    talent_veiled_shadows = new talent_t( this, "Veiled Shadows" ); // 				complete 12803
    talent_harnessed_shadows = new talent_t( this, "Harnessed Shadows" ); //  			complete 12803  
    talent_shadowy_apparation = new talent_t( this, "Shadowy Apparition" ); // 			done: talent function 12803
    talent_vampiric_embrace = new talent_t( this, "Vampiric Embrace" ); // 			complete 12803
    talent_vampiric_touch = new talent_t( this, "Vampiric Touch" ); // 				complete 12803
    talent_sin_and_punishment = new talent_t( this, "Sin and Punishment" );

    talent_penance = new spell_ids_t( this, "Penance" );
    talent_chastise = new spell_ids_t( this, "Holy Word: Chastise" );     // incomplete
    talent_shadow_power = new spell_ids_t( this, "Shadow Power" ); //       			done: talent function 12803					incomplete: link with main talent tree
    talent_mind_flay = new spell_ids_t( this, "Mind Flay" );
    talent_meditation = new spell_ids_t( this, "Meditation" ); // 					done: talent function 12803

	  }

	// Character Definition
	virtual void      init_glyphs();
	virtual void      init_race();
	virtual void      init_base();
	virtual void      init_gains();
	virtual void      init_uptimes();
	virtual void      init_rng();
	virtual void      init_talents();
	virtual void      init_buffs();
	virtual void      init_actions();
	virtual void      init_procs();
	virtual void      reset();
	virtual void      init_party();
	virtual std::vector<talent_translation_t>& get_talent_list();
	virtual std::vector<option_t>& get_options();
	virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL );
	virtual action_t* create_action( const std::string& name, const std::string& options );
	virtual pet_t*    create_pet   ( const std::string& name );
	virtual void      create_pets();
	virtual int       decode_set( item_t& item );
	virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
	virtual int       primary_role() SC_CONST     { return ROLE_SPELL; }
	virtual double    composite_armor() SC_CONST;
	virtual double    composite_attribute_multiplier( int attr ) SC_CONST;
	virtual double    composite_spell_power( int school ) SC_CONST;
	virtual double    composite_spell_hit() SC_CONST;
	virtual double    composite_player_multiplier( int school ) SC_CONST;

	virtual void      regen( double periodicity );
	virtual action_expr_t* create_expression( action_t*, const std::string& name );

	virtual double    resource_gain( int resource, double amount, gain_t* source=0, action_t* action=0 );
	virtual double    resource_loss( int resource, double amount, action_t* action=0 );

	virtual int       primary_tree() SC_CONST
	  {
	  if ( level > 10 && level <= 69 )
	    {
		  if (talent_darkness -> rank || talent_improved_shadow_word_pain -> rank || talent_veiled_shadows -> rank ) return TREE_SHADOW;
		  else if (talent_divine_fury -> rank ) return TREE_HOLY;
		  else if (talent_twin_disciplines -> rank || talent_mental_agility -> rank ) return TREE_DISCIPLINE;
		  else return TREE_NONE;

		}
	  else
		{
		  return talent_shadow_form -> rank ? TREE_SHADOW : talent_power_infusion -> rank ? TREE_DISCIPLINE : TREE_HOLY;
		}

	  }
};

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
  }

  virtual double haste() SC_CONST;
  virtual void   execute();
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
  virtual void   assess_damage( double amount, int dmg_type );
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
      p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.03, p -> gains_shadow_fiend );
    }
  };

  melee_t* melee;

  shadow_fiend_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "shadow_fiend" ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 100;
    main_hand_weapon.max_dmg    = 100;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
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

    melee = new melee_t( this );
  }
  virtual double composite_spell_power( int school ) SC_CONST
  {
    priest_t* p = owner -> cast_priest();

    double sp = p -> composite_spell_power( school );
    sp -= owner -> spirit() *   p -> spell_power_per_spirit;
    sp -= owner -> spirit() * ( p -> buffs_glyph_of_shadow -> check() ? 0.3 : 0.0 );
    return sp;
  }
  virtual double composite_attack_hit() SC_CONST
  {
    return owner -> composite_spell_hit();
  }
  virtual double composite_attack_expertise() SC_CONST
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
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



// ==========================================================================
// Priest Spell
// ==========================================================================

// priest_spell_t::haste ====================================================

double priest_spell_t::haste() SC_CONST
{
	priest_t* p = player -> cast_priest();

	  double h = spell_t::haste();
	  if ( p -> talent_darkness -> rank )
	  {
	    h *= 1.0 / ( 1.0 + p -> talent_darkness -> rank * p -> constants.darkness_value );
	  }
	  return h;
}



// priest_spell_t::execute ==================================================

void priest_spell_t::execute()
{
  spell_t::execute();
}

// priest_spell_t::player_buff ==============================================

void priest_spell_t::player_buff()
{
	priest_t* p = player -> cast_priest();
	spell_t::player_buff();
		if ( school == SCHOOL_SHADOW )
			{

				player_hit += p -> talent_twisted_faith -> rank * p -> constants.twisted_faith_static_value;
			}
	    for ( int i=0; i < 4; i++ )
			{
				p -> uptimes_mind_spike[ i ] -> update( i == p -> buffs_mind_spike -> stack() );
			}
	    for ( int i=0; i < 6; i++ )
	    	{
				p -> uptimes_dark_evangelism[ i ] -> update( i == p -> buffs_dark_evangelism -> stack() );
	    	}
	    for ( int i=0; i < 4; i++ )
			{
				p -> uptimes_shadow_orb[ i ] -> update( i == p -> buffs_shadow_orb -> stack() );
			}

}

// priest_spell_t::target_debuff ============================================

void priest_spell_t::target_debuff( int dmg_type )
{
	priest_t* p = player -> cast_priest();
	spell_t::target_debuff( dmg_type );

	target_multiplier *= 1.0 + p -> talent_shadow_power -> enabled * p -> constants.shadow_power_value;

		  if ( school == SCHOOL_SHADOW )
		  {
			  if ( dmg_type == DMG_OVER_TIME )
			  {
			  target_multiplier *=  1.0 + p -> talent_evangelism -> rank* p -> buffs_dark_evangelism -> stack () * p -> constants.dark_evangelism_value;
			  }

			  target_crit_bonus_multiplier *= 1.0 + p -> talent_shadow_power -> enabled * 1.0;
			  target_multiplier *= 1.0 + p -> buffs_dark_archangel -> stack() * p -> constants.dark_archangel_value;
			  if ( p -> buffs_chakra -> value() == 4 && p -> buffs_chakra -> up())
			  {
			  target_multiplier *= 1.0 + 0.15;
			  }
		  }
		  if ( school == SCHOOL_FROST)
		  {
			  target_multiplier *= 1.0 + p -> buffs_dark_archangel -> stack() * p -> constants.dark_archangel_value;
		  }

		  if ( school == SCHOOL_HOLY)
		  {
			  if ( p -> buffs_chakra -> value() == 4 && p -> buffs_chakra -> up())
			  {
			  target_multiplier *= 1.0 + 0.15;
			  }
		  }

		  // Twin Discipline
		  if ( execute_time() == 0 && !(dual) )
		  {
			 target_multiplier  *= 1.0 + ( p -> talent_twin_disciplines -> rank * p -> constants.twin_disciplines_value );

		  }
}

// priest_spell_t::assess_damage =============================================

void priest_spell_t::assess_damage( double amount,
                                    int    dmg_type )
{
  priest_t* p = player -> cast_priest();

  spell_t::assess_damage( amount, dmg_type );
  
  if ( p -> buffs_vampiric_embrace -> up() ) 
  {
    p -> resource_gain( RESOURCE_HEALTH, amount * 0.25 * ( 1.0 ), p -> gains.vampiric_embrace );

    pet_t* r = p -> pet_list;

    while ( r )
    {
      r -> resource_gain( RESOURCE_HEALTH, amount * 0.05 * ( 1.0 ), r -> gains.vampiric_embrace );
      r = r -> next_pet;
    }

    int num_players = ( int ) p -> party_list.size();

    for ( int i=0; i < num_players; i++ )
    {
      player_t* q = p -> party_list[ i ];
      
      q -> resource_gain( RESOURCE_HEALTH, amount * 0.05 * ( 1.0 ), q -> gains.vampiric_embrace );
    
      r = q -> pet_list;

      while ( r )
      {
        r -> resource_gain( RESOURCE_HEALTH, amount * 0.05 * ( 1.0 ), r -> gains.vampiric_embrace );
        r = r -> next_pet;
      }
    }    
  }
}

// Shadowy Apparation Spell ============================================================

struct shadowy_apparation_t : public priest_spell_t
{
	shadowy_apparation_t( player_t* player ) :
      priest_spell_t( "shadowy_apparation", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    static rank_t ranks[] =
    {
      { 80, 1, 390, 400, 0, 0.0 }, // estimated +-20 on cataclysm beta, 22082010
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    background        = true;
    proc			  = true;
    trigger_gcd       = 0;
    travel_speed	  = 2.6; // estimated
    base_execute_time = 0;
    may_crit          = false;
    direct_power_mod  = 1.5 / 3.5;
    base_cost         = 0.0;
    reset();
  }
};

// Devouring Plague Spell ======================================================

struct devouring_plague_burst_t : public priest_spell_t
{
  devouring_plague_burst_t( player_t* player ) :
      priest_spell_t( "devouring_plague", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();
    dual       = true;
    proc       = true;
    background = true;
    may_crit   = true;

    // doesn't seem to work on imp. dp
    //base_multiplier  *= 1.0 + ( p -> talents.twin_disciplines          * p -> constants.twin_disciplines_value );

    base_multiplier  *= 1.0 +   p -> set_bonus.tier8_2pc_caster()      * 0.15; // Applies multiplicatively with the Burst dmg, but additively on the ticks.


    // This helps log file and decouples the sooth RNG from the ticks.
    name_str = "devouring_plague_burst";
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();
    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT )
      {
        p -> buffs_glyph_of_shadow -> trigger();
      }
    }
    update_stats( DMG_DIRECT );
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


    id = 2944;
    parse_data( p -> player_data );

    base_cost        *= 1.0 - ( util_t::talent_rank( p -> talent_mental_agility -> rank, 3, 0.04, 0.07, 0.10 )
                        + p -> buffs_inner_will -> stack() * p -> constants.inner_will_value );
    base_cost         = floor( base_cost );
    base_crit        += p -> set_bonus.tier10_2pc_caster() * 0.05;

    if ( p -> talent_improved_devouring_plague -> rank )
    {
      devouring_plague_burst = new devouring_plague_burst_t( p );

    }
  }

  virtual void execute()
  {
    tick_may_crit = true;
    base_crit_bonus_multiplier = 1.0;
    priest_spell_t::execute();
    if ( devouring_plague_burst )
    	{
    	devouring_plague_burst -> base_dd_min	= 0.3 * (base_td + total_power() * tick_power_mod ) * num_ticks;
    	devouring_plague_burst -> base_dd_max	= 0.3 * (base_td + total_power() * tick_power_mod ) * num_ticks;
        devouring_plague_burst -> execute();
    	}
  }

  virtual void tick()
  {
    priest_spell_t::tick();
    priest_t* p = player -> cast_priest();
    player -> resource_gain( RESOURCE_HEALTH, tick_dmg * p -> constants.devouring_plague_health_mod );
  }

  virtual void target_debuff( int dmg_type )
  {
    priest_spell_t::target_debuff( dmg_type );
    target_multiplier *= 1 + sim -> target -> debuffs.crypt_fever -> value() * 0.01;
  }

  virtual void update_stats( int type )
  {
    if ( devouring_plague_burst && type == DMG_DIRECT ) return;
    priest_spell_t::update_stats( type );
  }

  virtual int scale_ticks_with_haste() SC_CONST
  {

      return 0;

  }
};

// Dispersion Spell ============================================================

struct dispersion_t : public priest_spell_t
{
  pet_t* shadow_fiend;

  dispersion_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "dispersion", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
       {
         { NULL, OPT_UNKNOWN, NULL }
       };
    parse_options( options, options_str );

    check_talent( p -> talent_dispersion -> rank );

    base_execute_time = 0.0;
    base_tick_time    = 1.0;
    num_ticks         = 6;
    channeled         = true;
    harmful           = false;
    base_cost         = 0;

    cooldown -> duration = 120;

    if ( p -> glyphs.dispersion ) cooldown -> duration -= 45;

    shadow_fiend = p -> find_pet( "shadow_fiend" );

    id = 47585;
  }

  virtual void tick()
  {
    priest_t* p = player -> cast_priest();
    p -> resource_gain( RESOURCE_MANA, 0.06 * p -> resource_max[ RESOURCE_MANA ], p -> gains_dispersion );
    priest_spell_t::tick();
  }

  virtual bool ready()
  {
    if ( ! priest_spell_t::ready() )
      return false;

    priest_t* p = player -> cast_priest();

    if ( ! shadow_fiend -> sleeping ) return false;

    double sf_cooldown_remains  = p -> cooldowns_shadow_fiend -> remains();
    double sf_cooldown_duration = p -> cooldowns_shadow_fiend -> duration;

    if ( sf_cooldown_remains <= 0 ) return false;

    double     max_mana = p -> resource_max    [ RESOURCE_MANA ];
    double current_mana = p -> resource_current[ RESOURCE_MANA ];

    double consumption_rate = ( p -> mana_resource.mana_loss - p -> mana_resource.mana_gain ) / sim -> current_time;
    double time_to_die = sim -> target -> time_to_die();

    if ( consumption_rate <= 0.00001 ) return false;

    double oom_time = current_mana / consumption_rate;

    if ( oom_time >= time_to_die ) return false;

    double sf_regen = 0.30 * max_mana;

    consumption_rate -= sf_regen / sf_cooldown_duration;

    if ( consumption_rate <= 0.00001 ) return false;
    
    double future_mana = current_mana + sf_regen - consumption_rate * sf_cooldown_remains;

    if ( future_mana > max_mana ) future_mana = max_mana;

    oom_time = future_mana / consumption_rate;

    if ( oom_time >= time_to_die ) return false;

    return ( max_mana - current_mana ) > p -> max_mana_cost;
  }
};

// Fortitude Spell ========================================================

struct fortitude_t : public priest_spell_t
{
  double bonus;

  fortitude_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "fortitude", player, SCHOOL_HOLY, TREE_DISCIPLINE ), bonus( 0 )
  {
    trigger_gcd = 0;

    bonus = util_t::ability_rank( player -> level,  165.0,80,  79.0,70,  54.0,0 );

    id = 48161;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> ooc_buffs() )
      {
	p -> buffs.fortitude -> trigger( 1, bonus );
	p -> init_resources( true );
      }
    }
  }

  virtual bool ready()
  {
    return player -> buffs.fortitude -> current_value < bonus;
  }
};

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
      { 80, 12, 900, 1140, 50, 0.11 }, // Dummy rank for level 80.
      { 78, 11, 890, 1130, 50, 0.11 },
      { 72, 10, 732,  928, 47, 0.11 },
      { 66,  9, 412,  523, 33, 0.11 },
      { 60,  8, 355,  449, 29, 0.13 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48135 );

    may_crit = true;
    base_execute_time = 2.0;
    base_tick_time    = 1.0;
    num_ticks         = 7;
    direct_power_mod  = p -> power_mod.holy_fire;
    tick_power_mod    = 0.1678 / 7;
    cooldown -> duration = 10;

    base_execute_time -= p -> talent_divine_fury -> rank * 0.01;

    // Holy_Evangelism
        target_multiplier *= 1.0 + ( p -> talent_evangelism -> rank * p -> buffs_holy_evangelism -> stack() * p -> constants.holy_evangelism_damage_value );
        base_cost		  *= 1.0 - ( p -> talent_evangelism -> rank * p -> buffs_holy_evangelism -> stack() * p -> constants.holy_evangelism_mana_value );

  }
};

// Inner Fire Spell ======================================================

struct inner_fire_t : public priest_spell_t
{
  double bonus_spell_power;
  double bonus_armor;

  inner_fire_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "inner_fire", player, SCHOOL_HOLY, TREE_DISCIPLINE ), bonus_spell_power( 0 ), bonus_armor( 0 )
  {
    priest_t* p = player -> cast_priest();

    trigger_gcd = 0;

    bonus_spell_power = util_t::ability_rank( player -> level,  120.0,77,  95.0,71,  0.0,0     );
    bonus_armor       = util_t::ability_rank( player -> level,  2440.0,77, 1800.0,71, 1580.0,0 );

    bonus_spell_power *= 1.0 + p -> talent_improved_inner_fire -> rank * 0.15;
    bonus_armor       *= 1.0 + p -> talent_improved_inner_fire -> rank * 0.15;
    bonus_armor       *= 1.0 + p -> glyphs.inner_fire * 0.5;

    id = 48168;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    
    p -> buffs_inner_will		-> expire ();
    p -> buffs_inner_fire       -> start( 1, bonus_spell_power );
    p -> buffs_inner_fire_armor -> start( 1, bonus_armor       );
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();
    return ! p -> buffs_inner_fire -> check() || ! p -> buffs_inner_fire_armor -> check();
  }
};


// Inner Will Spell ======================================================

struct inner_will_t : public priest_spell_t
{


  inner_will_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "inner_will", player, SCHOOL_HOLY, TREE_DISCIPLINE )
  {
	check_min_level( 83 );
    priest_t* p = player -> cast_priest();

    option_t options[] =
       {
         { NULL, OPT_UNKNOWN, NULL }
       };
    parse_options( options, options_str );

    trigger_gcd = 0;
    base_cost  = 0.07 * p -> resource_base[ RESOURCE_MANA ];
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_inner_fire -> expire();
    p -> buffs_inner_fire_armor -> expire();
    p -> buffs_inner_will -> trigger();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();
    return ! p -> buffs_inner_will -> check();
  }
};

// Mind Blast Spell ============================================================

struct mind_blast_t : public priest_spell_t
{
  mind_blast_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "mind_blast", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id				  = 8092;

    priest_t* p = player -> cast_priest();

    parse_data( p->player_data );

    may_crit          = true;
    base_multiplier  *= 1.0 + ( p -> buffs_shadow_orb -> stack() * ( p -> constants.shadow_orb_damage_value
								+ p -> composite_mastery() * p -> constants.shadow_orb_mastery_value ) );
    cooldown -> duration -= p -> talent_improved_mind_blast -> rank * 0.5;
  }


  virtual void execute()
  {
	  priest_t* p = player -> cast_priest();
	  priest_spell_t::execute();
	  player -> cast_priest() -> buffs_mind_melt -> expire();
	  p -> buffs_mind_spike -> expire();
	  if ( result_is_hit() )
	  {
      p -> recast_mind_blast = 0;
      p -> buffs_shadow_orb -> expire();
      if ( result == RESULT_CRIT )
      {
        p -> buffs_glyph_of_shadow -> trigger();
      }
      if ( p -> dots_vampiric_touch -> ticking() )
      {
	      p -> trigger_replenishment();
      }
    }
  }

  virtual void target_debuff( int dmg_type )
  {
	priest_t* p = player -> cast_priest();
    priest_spell_t::target_debuff( dmg_type );
    target_crit 		  += p -> constants.mind_spike_crit_value * p -> buffs_mind_spike -> stack();
  }

  virtual double execute_time() SC_CONST
  {
    priest_t* p = player -> cast_priest();
    double a = priest_spell_t::execute_time();
    a *= 1 - (p -> buffs_mind_melt -> stack() * 0.5);
    return a;
  }


};

// Mind Flay Spell ============================================================

struct mind_flay_tick_t : public priest_spell_t
{
  mind_flay_tick_t( player_t* player ) :
      priest_spell_t( "mind_flay", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    id = 15407;
    parse_data( p -> player_data );
    base_cost=0;
    base_td=0;
    base_tick_time=0;
    num_ticks=0;
    dual              = true;
    background        = true;
    may_crit          = true;
    direct_tick       = true;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::execute();
    tick_dmg = direct_dmg;
    update_stats( DMG_OVER_TIME );

    if ( result_is_hit() )
    {
      p -> buffs_dark_evangelism  -> trigger( 1, 1.0, 0.4 );
      p -> buffs_shadow_orb  -> trigger( 1, 1, p -> constants.shadow_orb_proc_value + p -> constants.harnessed_shadows_value * p -> talent_harnessed_shadows -> rank );

      if ( p -> dots_shadow_word_pain -> ticking() )
      {
        if ( p -> rng_pain_and_suffering -> roll( p -> talent_pain_and_suffering -> rank * p -> constants.pain_and_suffering_value ) )
        {
          p -> dots_shadow_word_pain -> action -> refresh_duration();
        }
      }
      if ( result == RESULT_CRIT )
      {
   	    p -> buffs_glyph_of_shadow -> trigger();
   	    p -> cooldowns_shadow_fiend -> ready -= 10.0 * p -> talent_sin_and_punishment -> rank;
      }
    }
  }
};

struct mind_flay_t : public priest_spell_t
{
  spell_t* mind_flay_tick;

  double mb_wait;
  int    swp_refresh;
  int    cut_for_mb;

  mind_flay_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "mind_flay", player, SCHOOL_SHADOW, TREE_SHADOW ), mb_wait( 0 ), swp_refresh( 0 ), cut_for_mb( 0 )
  {
    priest_t* p = player -> cast_priest();
    check_talent( p -> talent_mind_flay -> enabled );

    option_t options[] =
    {
      { "cut_for_mb",            OPT_BOOL, &cut_for_mb            },
      { "mb_wait",               OPT_FLT,  &mb_wait               },
      { "swp_refresh",           OPT_BOOL, &swp_refresh           },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    channeled      = true;
    num_ticks      = 3;
    base_tick_time = 1.0;

    if ( p -> set_bonus.tier10_4pc_caster() )
    {
      base_tick_time -= 0.51 / num_ticks;
    }

    base_cost  = 0.09 * p -> resource_base[ RESOURCE_MANA ];
    base_cost  = floor( base_cost );

    mind_flay_tick = new mind_flay_tick_t( p );

    id = 48156;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    if ( cut_for_mb )
    {
      if ( p -> get_cooldown("mind_blast") -> remains() <= ( 2 * base_tick_time * haste() ) )
      {
        num_ticks = 2;
      }
    }
    priest_spell_t::execute();

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

    // Optional check to only cast Mind Flay if there's 2 or less ticks left on SW:P
    // This allows a action+=/mind_flay,swp_refresh to be added as a higher priority to ensure that SW:P doesn't fall off
    // Won't be necessary as part of a standard rotation, but if there's movement or other delays in casting it would have
    // it's uses.
    if ( swp_refresh && ( p -> talent_pain_and_suffering -> rank > 0 ) )
    {
      if ( ! p -> dots_shadow_word_pain -> ticking() )
        return false;

      if ( ( p -> dots_shadow_word_pain -> action -> num_ticks -
             p -> dots_shadow_word_pain -> action -> current_tick ) > 2 )
        return false;
    }

    // If this option is set (with a value in seconds), don't cast Mind Flay if Mind Blast 
    // is about to come off it's cooldown.
    if ( mb_wait )
    {
      if ( p -> cooldowns_mind_blast -> remains() < mb_wait )
        return false;
    }

    return true;
  }
};

// Cataclysm Mind Spike Spell ============================================================

struct mind_spike_t : public priest_spell_t
{

  mind_spike_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "mind_spike", player, SCHOOL_SHADOWFROST, TREE_SHADOW )
  {
	check_min_level( 81 );
    priest_t* p = player -> cast_priest();

    option_t options[] =
       {
         { NULL, OPT_UNKNOWN, NULL }
       };
       parse_options( options, options_str );

    id = 73510;
    parse_data( p -> player_data );

    may_crit          = true;
    base_multiplier  *= 1.0 + ( p -> buffs_shadow_orb -> stack() * ( p -> constants.shadow_orb_damage_value
								+ p -> composite_mastery() * p -> constants.shadow_orb_mastery_value ) );
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    priest_t* p = player -> cast_priest();

    if ( result_is_hit() )
    {
      p -> buffs_mind_melt  -> trigger( 1, 1.0 );
      p -> buffs_shadow_orb -> expire();
      p -> buffs_mind_spike -> trigger();

      if ( result == RESULT_CRIT )
      {
   	    p -> buffs_glyph_of_shadow -> trigger();
      }
    }
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();
  }
};

// Penance Spell ===============================================================

struct penance_tick_t : public priest_spell_t
{
  penance_tick_t( player_t* player ) :
      priest_spell_t( "penance", player, SCHOOL_HOLY, TREE_HOLY )
  {
    static rank_t ranks[] =
    {
      { 80, 4, 288, 288, 0, 0 },
      { 76, 3, 256, 256, 0, 0 },
      { 68, 2, 224, 224, 0, 0 },
      { 60, 1, 184, 184, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 53007 );

    dual        = true;
    background  = true;
    may_crit    = true;
    direct_tick = true;
    direct_power_mod  = 0.8 / 3.5;

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
    check_talent( p -> talent_penance -> enabled );

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

    // Holy_Evangelism
    target_multiplier *= 1.0 + ( p -> talent_evangelism -> rank * p -> buffs_holy_evangelism -> stack() * p -> constants.holy_evangelism_damage_value );
    base_cost		  *= 1.0 - ( p -> talent_evangelism -> rank * p -> buffs_holy_evangelism -> stack() * p -> constants.holy_evangelism_mana_value );

    cooldown -> duration  = 12 - ( p -> glyphs.penance * 2 );

    penance_tick = new penance_tick_t( p );

    id = 53007;
  }
  virtual void execute()
  {
    priest_spell_t::execute();
  }
  virtual void tick()
  {
	if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
	penance_tick -> execute();
	update_time( DMG_OVER_TIME );
  }
};

// Power Infusion Spell =====================================================

struct power_infusion_t : public priest_spell_t
{
  player_t* power_infusion_target;

  power_infusion_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "power_infusion", player, SCHOOL_HOLY, TREE_DISCIPLINE )
  {
    priest_t* p = player -> cast_priest();
    check_talent( p -> talent_power_infusion -> rank );

    std::string target_str = p -> power_infusion_target_str;
    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( target_str.empty() )
    {
      power_infusion_target = p;
    }
    else
    {
      power_infusion_target = sim -> find_player( target_str );
      assert ( power_infusion_target != 0 );
    }

    trigger_gcd = 0;
    cooldown -> duration = 120.0;

    id = 10060;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    if ( sim -> log ) log_t::output( sim, "%s grants %s Power Infusion", p -> name(), power_infusion_target -> name() );

    power_infusion_target -> buffs.power_infusion -> trigger();

    consume_resource();
    update_ready();
  }

  virtual bool ready()
  {
    if ( ! priest_spell_t::ready() )
      return false;

    if ( power_infusion_target == 0 )
      return false;

    if ( power_infusion_target -> buffs.bloodlust -> check() )
      return false;

    if ( power_infusion_target -> buffs.power_infusion -> check() )
      return false;

    return true;
  }
};

// Shadow Form Spell =======================================================

struct shadow_form_t : public priest_spell_t
{
  shadow_form_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "shadow_form", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();
    check_talent( p -> talent_shadow_form -> rank );
    trigger_gcd = 0;

    id = 15473;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_shadow_form -> trigger();
    sim -> auras.mind_quickening -> trigger();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();
    return( ! p -> buffs_shadow_form -> check() );
  }
};

// Shadow Word Death Spell ======================================================

struct shadow_word_death_t : public priest_spell_t
{
  double mb_min_wait;
  double mb_max_wait;


  shadow_word_death_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "shadow_word_death", player, SCHOOL_SHADOW, TREE_SHADOW ), mb_min_wait( 0 ), mb_max_wait( 0 )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "mb_min_wait",         OPT_FLT,  &mb_min_wait         },
      { "mb_max_wait",         OPT_FLT,  &mb_max_wait         },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 32379;
    parse_data( p -> player_data );

    may_crit = true;

    base_cost        *= 1.0 - ( util_t::talent_rank( p -> talent_mental_agility -> rank, 3, 0.04, 0.07, 0.10 )
								+ p -> buffs_inner_will -> stack() * p -> constants.inner_will_value );
    base_cost         = floor( base_cost );

  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::execute();
    if ( result_is_hit() )
    {
      double health_loss = direct_dmg * ( 1.0 - p -> talent_pain_and_suffering -> rank * 0.10 );

      p -> resource_loss( RESOURCE_HEALTH, health_loss );
      if ( ( health_loss > 0.0 ) && p -> talent_masochism -> rank )
      {
        p -> resource_gain( RESOURCE_MANA, 0.04 * p -> talent_masochism -> rank * p -> resource_max[ RESOURCE_MANA ], p -> gains_masochism );
      }

      if ( result == RESULT_CRIT )
      {
        p -> buffs_glyph_of_shadow -> trigger();
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
    if ( p -> talent_mind_melt -> rank )
       {
         if ( sim -> target -> health_percentage() <= 25 )
         {
           player_multiplier *= 1.0 + p -> talent_mind_melt -> rank * 0.15;
         }
       }
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( ! priest_spell_t::ready() )
      return false;

    if ( mb_min_wait )
      if ( p -> cooldowns_mind_blast -> remains() < mb_min_wait )
        return false;

    if ( mb_max_wait )
      if ( p -> cooldowns_mind_blast -> remains() > mb_max_wait )
        return false;

    return true;
  }
};

// Shadow Word Pain Spell ======================================================

struct shadow_word_pain_t : public priest_spell_t
{

  shadow_word_pain_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "shadow_word_pain", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 589;
    parse_data( p -> player_data );


    base_cost        *= 1.0 - ( util_t::talent_rank( p -> talent_mental_agility -> rank, 3, 0.04, 0.07, 0.10 )
								+ p -> buffs_inner_will -> stack() * p -> constants.inner_will_value );
    base_cost         = floor( base_cost );
    base_multiplier *= 1.0 + ( p -> talent_improved_shadow_word_pain -> rank * p -> constants.improved_shadow_word_pain_value );
    base_crit += p -> set_bonus.tier10_2pc_caster() * 0.05;

    if ( p -> set_bonus.tier6_2pc_caster() ) num_ticks++;
  }

  virtual void execute()
  {
    tick_may_crit = true;
    base_crit_bonus_multiplier = 1.0;
    priest_spell_t::execute();
  }

  virtual void tick()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::tick();

    // Glyph of SW:P
    if ( p -> glyphs.shadow_word_pain )
    	{
			p -> resource_gain( RESOURCE_MANA, p -> resource_base[ RESOURCE_MANA ] * 0.01, p -> gains_glyph_of_shadow_word_pain );
    	}

    // Shadowy Apparation
    if ( p -> talent_shadowy_apparation -> rank )
    {
    	double h = p -> talent_shadowy_apparation -> rank * (0.02 + p -> buffs.moving -> up()* 0.2 );
    	if (  p -> sim -> roll( h ) )
    	{
    		p -> procs_shadowy_apparation -> occur();
    		if ( ! p -> shadowy ) p -> shadowy = new shadowy_apparation_t( p );
    		p -> shadowy -> execute();
    	}
    }

    // Shadow Orb
    if ( result_is_hit() )
    {
    	p -> buffs_shadow_orb  -> trigger( 1, 1, p -> constants.shadow_orb_proc_value + p -> constants.harnessed_shadows_value * p -> talent_harnessed_shadows -> rank );
    }
  }

  virtual int scale_ticks_with_haste() SC_CONST
  {
      return 0;
  }

  virtual void refresh_duration()
  {
    num_ticks++;
    priest_spell_t::refresh_duration();
    num_ticks--;
  }
};


// Chakra_Pre Spell ======================================================

struct chakra_t : public priest_spell_t
{
	chakra_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "chakra", player, SCHOOL_HOLY, TREE_HOLY )
  {

	option_t options[] =
	{
		{ NULL, OPT_UNKNOWN, NULL }
	};
	parse_options( options, options_str );

    priest_t* p = player -> cast_priest();
    check_talent( p -> talent_chakra -> rank );

    static rank_t ranks[] =
    {
      { 1, 1, 0, 0, 0, 0.00 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    trigger_gcd = 0;
    base_cost   = 0.0;
  }

  virtual void execute()
  {
	priest_spell_t::execute();
    priest_t* p = player -> cast_priest();
    p -> buffs_chakra_pre -> trigger( 1 , 1.0, 1.0 );
  }

  virtual bool ready()
   {
	  priest_t* p = player -> cast_priest();

	  if ( p -> buffs_chakra_pre -> up() )
       return false;

	  return true;
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
    init_rank( ranks, 48123 );

    base_execute_time = 2.5;
    direct_power_mod  = p -> power_mod.smite;
    may_crit          = true;

    base_execute_time -= p -> talent_divine_fury -> rank * 0.1;

    // Holy_Evangelism
    target_multiplier *= 1.0 + ( p -> talent_evangelism -> rank * p -> buffs_holy_evangelism -> stack() * p -> constants.holy_evangelism_damage_value );
    base_cost		  *= 1.0 - ( p -> talent_evangelism -> rank * p -> buffs_holy_evangelism -> stack() * p -> constants.holy_evangelism_mana_value );

  }

  virtual void execute()
  {
	priest_t* p = player -> cast_priest();

    priest_spell_t::execute();
    p -> buffs_holy_evangelism  -> trigger( 1, 1.0, 1.0 );
    if ( p -> buffs_chakra_pre -> up())
    {
    	p -> buffs_chakra -> trigger( 1 , 4, 1.0 );

    	// Here the cooldown of chakra should be activated. or chakra could be executed
    	p -> buffs_chakra_pre -> expire();
    }

    // Talent State of Mind
    if ( result_is_hit() )
        {
			if ( p -> buffs_chakra -> up() && p -> talent_chakra -> rank )
			{
				// p -> buffs_chakra -> "add 4 seconds"
			}
        }
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();
    priest_t* p = player -> cast_priest();
    if ( p -> dots_holy_fire -> ticking() && p -> glyphs.smite ) player_multiplier *= 1.20;
  }
};

// Vampiric Embrace Spell ======================================================

struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "vampiric_embrace", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    check_talent( p -> talent_vampiric_embrace -> rank );

    static rank_t ranks[] =
    {
      { 1, 1, 0, 0, 0, 0.00 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 15286 );

    trigger_gcd = 0;
    base_cost   = 0.0;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    if ( p -> talent_vampiric_embrace -> rank )
      p -> buffs_vampiric_embrace -> trigger();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( ! priest_spell_t::ready() )
      return false;

    return p -> talent_vampiric_embrace -> rank && ! p -> buffs_vampiric_embrace -> check();
  }

};

// Vampiric Touch Spell ======================================================

struct vampiric_touch_t : public priest_spell_t
{
  vampiric_touch_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "vampiric_touch", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    check_talent( p -> talent_vampiric_touch -> rank );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 34914;
    effect_nr = 2;
    parse_data( p -> player_data );


    base_crit       += p -> set_bonus.tier10_2pc_caster() * 0.05;

    if ( p -> set_bonus.tier9_2pc_caster() ) num_ticks += 2;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    tick_may_crit = true;
    priest_spell_t::execute();
    if ( result_is_hit() )
    {
      p -> recast_mind_blast = 1;
    }
  }

  virtual int scale_ticks_with_haste() SC_CONST
  {
     return 0;
  }
};

// Shadow Fiend Spell ========================================================

struct shadow_fiend_spell_t : public priest_spell_t
{
  int trigger;

  shadow_fiend_spell_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "shadow_fiend", player, SCHOOL_SHADOW, TREE_SHADOW ), trigger( 0 )
  {
	check_min_level( 66 );
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
    init_rank( ranks, 34433 );

    harmful = false;
    cooldown -> duration -= 30.0 * p -> talent_veiled_shadows -> rank;
    base_cost *= 1.0 - ( util_t::talent_rank( p -> talent_mental_agility -> rank, 3, 0.04, 0.07, 0.10 ) );
    base_cost  = floor( base_cost );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    consume_resource();
    update_ready();
    p -> summon_pet( "shadow_fiend", 15.1 );
  }

  virtual bool ready()
  {
    if ( ! priest_spell_t::ready() )
      return false;

    priest_t* p = player -> cast_priest();

    if ( sim -> infinite_resource[ RESOURCE_MANA ] )
      return true;

    double     max_mana = p -> resource_max    [ RESOURCE_MANA ];
    double current_mana = p -> resource_current[ RESOURCE_MANA ];

    if ( trigger > 0 ) return ( max_mana - current_mana ) >= trigger;

    return ( sim -> current_time > 15.0 );
  }
};

// Archangel Spell ======================================================

struct archangel_t : public priest_spell_t
{
	archangel_t( player_t* player, const std::string& options_str ) :
      priest_spell_t( "archangel", player, SCHOOL_HOLY, TREE_DISCIPLINE )
  {
    priest_t* p = player -> cast_priest();

    check_talent( p -> talent_archangel -> rank );

    option_t options[] =
        {
          { NULL, OPT_UNKNOWN, NULL }
        };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 1, 1, 0, 0, 0, 0.00 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    trigger_gcd = 0;
    base_cost   = 0.0;
  }

  virtual void execute()
  {
	priest_spell_t::execute();
    priest_t* p = player -> cast_priest();
    if ( p -> buffs_dark_evangelism -> up())
		{
			p -> buffs_dark_archangel -> trigger( p -> buffs_dark_evangelism -> stack() , 1.0, 1.0 );
    		p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * p -> constants.archangel_mana_value * p -> buffs_dark_evangelism -> stack(), p -> gains_archangel );
    		p -> buffs_dark_evangelism -> expire();
    	}
    else if ( p -> buffs_holy_evangelism -> up())
    	{
    		p -> buffs_holy_archangel -> trigger( p -> buffs_holy_evangelism -> stack() , 1.0, 1.0 );
    		p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * p -> constants.archangel_mana_value * p -> buffs_holy_evangelism -> stack(), p -> gains_archangel );
    		p -> buffs_holy_evangelism -> expire();
    	}
  }

  virtual bool ready()
   {
	  priest_t* p = player -> cast_priest();

	  if ( ! priest_spell_t::ready() )
	       return false;
	  if ( ! p -> buffs_dark_evangelism -> up() && ! p -> buffs_holy_evangelism -> up() )
       return false;

	  return true;

   }
};





} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Priest Character Definition
// ==========================================================================

// priest_t::composite_armor =========================================

double priest_t::composite_armor() SC_CONST
{
  double a = player_t::composite_armor();

  a += buffs_inner_fire_armor -> value();

  return floor( a );
}

// priest_t::composite_attribute_multiplier ================================

double priest_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = player_t::composite_attribute_multiplier( attr );

  return m;
}


// priest_t::composite_spell_power =========================================

double priest_t::composite_spell_power( int school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );

  sp += buffs_inner_fire -> value();

  if ( buffs_glyph_of_shadow -> up() )
  {
    sp += spirit() * 0.3;
  }

  return floor( sp );
}

// priest_t::composite_spell_hit =============================================

double priest_t::composite_spell_hit() SC_CONST
{
  double hit = player_t::composite_spell_hit();

  hit += spirit() * ( talent_twisted_faith -> rank / 2.0 ) / rating.spell_hit;

  return floor( hit * 10000.0 ) / 10000.0;
}
// priest_t::composite_player_multiplier =========================================

double priest_t::composite_player_multiplier( int school ) SC_CONST
{
  double m = player_t::composite_player_multiplier( school );

  if ( school == SCHOOL_SHADOW )
  {
    m *= 1.0 + buffs_shadow_form -> check() * constants.shadow_form_value;
  }

  return m;
}

// priest_t::create_action ===================================================

action_t* priest_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "devouring_plague"  ) return new devouring_plague_t  ( this, options_str );
  if ( name == "dispersion"        ) return new dispersion_t        ( this, options_str );
  if ( name == "fortitude"         ) return new fortitude_t         ( this, options_str );
  if ( name == "holy_fire"         ) return new holy_fire_t         ( this, options_str );
  if ( name == "inner_fire"        ) return new inner_fire_t        ( this, options_str );
  if ( name == "mind_blast"        ) return new mind_blast_t        ( this, options_str );
  if ( name == "mind_flay"         ) return new mind_flay_t         ( this, options_str );
  if ( name == "mind_spike"        ) return new mind_spike_t        ( this, options_str );
  if ( name == "penance"           ) return new penance_t           ( this, options_str );
  if ( name == "power_infusion"    ) return new power_infusion_t    ( this, options_str );
  if ( name == "shadow_word_death" ) return new shadow_word_death_t ( this, options_str );
  if ( name == "shadow_word_pain"  ) return new shadow_word_pain_t  ( this, options_str );
  if ( name == "shadow_form"       ) return new shadow_form_t       ( this, options_str );
  if ( name == "smite"             ) return new smite_t             ( this, options_str );
  if ( name == "shadow_fiend"      ) return new shadow_fiend_spell_t( this, options_str );
  if ( name == "vampiric_embrace"  ) return new vampiric_embrace_t  ( this, options_str );
  if ( name == "vampiric_touch"    ) return new vampiric_touch_t    ( this, options_str );
  if ( name == "archangel"         ) return new archangel_t         ( this, options_str );
  if ( name == "chakra"            ) return new chakra_t            ( this, options_str );
  if ( name == "inner_will"        ) return new inner_will_t        ( this, options_str );

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

// priest_t::create_pets =====================================================

void priest_t::create_pets()
{
  create_pet( "shadow_fiend" );
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

    if      ( n == "dispersion"        ) glyphs.dispersion = 1;
    else if ( n == "hymn_of_hope"      ) glyphs.hymn_of_hope = 1;
    else if ( n == "inner_fire"        ) glyphs.inner_fire = 1;
    else if ( n == "mind_flay"         ) glyphs.mind_flay = 1;
    else if ( n == "penance"           ) glyphs.penance = 1;
    else if ( n == "shadow"            ) glyphs.shadow = 1;
    else if ( n == "shadow_word_death" ) glyphs.shadow_word_death = 1;
    else if ( n == "shadow_word_pain"  ) glyphs.shadow_word_pain = 1;
    else if ( n == "smite"             ) glyphs.smite = 1;
    // Just to prevent warnings....
    else if ( n == "circle_of_healing" ) ;
    else if ( n == "dispel_magic"      ) ;
    else if ( n == "fading"            ) ;
    else if ( n == "flash_heal"        ) ;
    else if ( n == "fortitude"         ) ;
    else if ( n == "guardian_spirit"   ) ;
    else if ( n == "levitate"          ) ;
    else if ( n == "mind_sear"         ) ;
    else if ( n == "pain_suppression"  ) ;
    else if ( n == "power_word_shield" ) ;
    else if ( n == "prayer_of_healing" ) ;
    else if ( n == "psychic_scream"    ) ;
    else if ( n == "renew"             ) ;
    else if ( n == "shackle_undead"    ) ;
    else if ( n == "shadow_protection" ) ;
    else if ( n == "shadowfiend"       ) ;
    else if ( ! sim -> parent ) 
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
  }
}

// priest_t::init_race ======================================================

void priest_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_HUMAN:
  case RACE_DWARF:
  case RACE_NIGHT_ELF:
  case RACE_DRAENEI:
  case RACE_UNDEAD:
  case RACE_TROLL:
  case RACE_BLOOD_ELF:
  case RACE_WORGEN:
  case RACE_GOBLIN:
    break;
  default:
    race = RACE_NIGHT_ELF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

// priest_t::init_base =======================================================

void priest_t::init_base()
{
  player_t::init_base();

  base_attack_power = -10;

  initial_attack_power_per_strength = 1.0;
  initial_spell_power_per_intellect = 1.0;

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

// priest_t::init_gains ======================================================

void priest_t::init_gains()
{
  player_t::init_gains();

  gains_dispersion                = get_gain( "dispersion" );
  gains_glyph_of_shadow_word_pain = get_gain( "glyph_of_shadow_word_pain" );
  gains_shadow_fiend              = get_gain( "shadow_fiend" );
  gains_archangel                 = get_gain( "archangel" );
  gains_masochism                 = get_gain( "masochism" );
}

void priest_t::init_procs()
{
  player_t::init_procs();

  procs_shadowy_apparation   = get_proc( "shadowy_apparation_proc" );
}

// priest_t::init_uptimes ====================================================

void priest_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_mind_spike[ 0 ]	= get_uptime( "mind_spike_0" );
  uptimes_mind_spike[ 1 ]	= get_uptime( "mind_spike_1" );
  uptimes_mind_spike[ 2 ]	= get_uptime( "mind_spike_2" );
  uptimes_mind_spike[ 3 ]	= get_uptime( "mind_spike_3" );

  uptimes_dark_evangelism[ 0 ]	= get_uptime( "dark_evangelism_0" );
  uptimes_dark_evangelism[ 1 ]	= get_uptime( "dark_evangelism_1" );
  uptimes_dark_evangelism[ 2 ]	= get_uptime( "dark_evangelism_2" );
  uptimes_dark_evangelism[ 3 ]	= get_uptime( "dark_evangelism_3" );
  uptimes_dark_evangelism[ 4 ]	= get_uptime( "dark_evangelism_4" );
  uptimes_dark_evangelism[ 5 ]	= get_uptime( "dark_evangelism_5" );

  uptimes_shadow_orb[ 0 ]	= get_uptime( "shadow_orb_0" );
  uptimes_shadow_orb[ 1 ]	= get_uptime( "shadow_orb_1" );
  uptimes_shadow_orb[ 2 ]	= get_uptime( "shadow_orb_2" );
  uptimes_shadow_orb[ 3 ]	= get_uptime( "shadow_orb_3" );

}

// priest_t::init_rng ========================================================

void priest_t::init_rng()
{
  player_t::init_rng();

  rng_pain_and_suffering = get_rng( "pain_and_suffering" );
}

// priest_t::init_talents
void priest_t::init_talents()
{
  player_t::init_talents();

  switch( primary_tree() )
  {
  case TREE_SHADOW:
    talent_mind_flay -> enabled = true;
    talent_shadow_power -> enabled = true;
    break;
  case TREE_DISCIPLINE:
    talent_penance -> enabled = true;
    talent_meditation -> enabled = true;
    break;
  case TREE_HOLY:
    talent_chastise -> enabled = true;
    talent_meditation -> enabled = true;
    break;
  default:
    assert( 0 );
    break;
  }
}

// priest_t::init_buffs ======================================================

void priest_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )
  buffs_glyph_of_shadow     = new buff_t( this, "glyph_of_shadow",     1, 10.0, 0.0, glyphs.shadow );
  buffs_inner_fire          = new buff_t( this, "inner_fire"                                       );
  buffs_inner_fire_armor    = new buff_t( this, "inner_fire_armor"                                 );
  buffs_inner_will          = new buff_t( this, "inner_will"                                       );
  buffs_shadow_form         = new buff_t( this, "shadow_form",         1                           );
  buffs_mind_melt           = new buff_t( this, "mind_melt",           2, 6.0, 0,1                 );
  buffs_dark_evangelism     = new buff_t( this, "dark_evangelism",     5, 15.0, 0, 0.4             );
  buffs_holy_evangelism     = new buff_t( this, "holy_evangelism",     5, 15.0, 0, 1.0             );
  buffs_shadow_orb          = new buff_t( this, "shadow_orb",          3, 60.0                     );
  buffs_dark_archangel      = new buff_t( this, "dark_archangel",      5, 18.0                     );
  buffs_holy_archangel      = new buff_t( this, "holy_archangel",      5, 18.0                     );
  buffs_chakra_pre          = new buff_t( this, "chakra_pre",          1                           );
  buffs_chakra              = new buff_t( this, "chakra_buff",         1, 30.0                     );
  buffs_vampiric_embrace    = new buff_t( this, "vampiric_embrace",    1                           );
  buffs_mind_spike          = new buff_t( this, "mind_spike",          3, 12.0                     );

}

// priest_t::init_actions =====================================================

void priest_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    action_list_str = "flask,type=frost_wyrm/food,type=fish_feast/fortitude/inner_fire";

    if ( talent_shadow_form -> rank ) action_list_str += "/shadow_form";

    if ( talent_vampiric_embrace -> rank ) action_list_str += "/vampiric_embrace";

    action_list_str += "/snapshot_stats";

    int num_items = ( int ) items.size();
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
      action_list_str += "/wild_magic_potion,if=!in_combat";
      action_list_str += "/speed_potion,if=buff.bloodlust.react|target.time_to_die<=20";
      action_list_str += "/shadow_fiend";
      if ( talent_archangel -> rank ) action_list_str += "/archangel,if=buff.dark_evangelism.stack>=5";
      action_list_str += "/shadow_word_pain,if=!ticking";
      if ( race == RACE_TROLL ) action_list_str += "/berserking";
      if ( talent_vampiric_touch -> rank ) action_list_str += "/vampiric_touch,if=!ticking/vampiric_touch,if=dot.vampiric_touch.remains<cast_time";
      action_list_str += "/devouring_plague,if=!ticking";
      action_list_str += "/shadow_word_death,health_percentage<=34";
      action_list_str += "/mind_blast";
      if ( race == RACE_BLOOD_ELF ) action_list_str += "/arcane_torrent";
      action_list_str += talent_mind_flay -> enabled ? "/mind_flay" : "/smite";
      action_list_str += "/shadow_word_death,moving=1"; // when moving
      if ( talent_dispersion -> rank ) action_list_str += "/dispersion";
      break;
    case TREE_DISCIPLINE:
      action_list_str += "/mana_potion/shadow_fiend,trigger=10000";
      action_list_str += "/shadow_word_pain,if=!ticking";
      if ( talent_power_infusion -> rank ) action_list_str += "/power_infusion";
      action_list_str += "/holy_fire/mind_blast/";
      if ( talent_penance -> enabled ) action_list_str += "/penance";
      if ( race == RACE_TROLL ) action_list_str += "/berserking";
      if ( race == RACE_BLOOD_ELF ) action_list_str += "/arcane_torrent";
      action_list_str += "/smite";
      action_list_str += "/shadow_word_death,moving=1"; // when moving
      break;
    case TREE_HOLY:
    default:
      action_list_str += "/mana_potion/shadow_fiend,trigger=10000";
      action_list_str += "/shadow_word_pain,if=!ticking";
      if ( talent_power_infusion -> rank ) action_list_str += "/power_infusion";
      action_list_str += "/holy_fire/mind_blast";
      if ( race == RACE_TROLL     ) action_list_str += "/berserking";
      if ( race == RACE_BLOOD_ELF ) action_list_str += "/arcane_torrent";
      action_list_str += "/smite";
      action_list_str += "/shadow_word_death,moving=1"; // when moving
      break;
    }

    action_list_default = 1;
  }

  player_t::init_actions();

  for( action_t* a = action_list; a; a = a -> next )
  {
    double c = a -> cost();
    if ( c > max_mana_cost ) max_mana_cost = c;
  }
}

// priest_t::init_party ======================================================

void priest_t::init_party()
{
  party_list.clear();

  if ( party == 0 )
    return;

  player_t* p = sim -> player_list;
  while ( p )
  {
     if ( ( p != this ) && ( p -> party == party ) && ( ! p -> quiet ) && ( ! p -> is_pet() ) )
     {
       party_list.push_back( p );
     }
     p = p -> next;
  }
}

// priest_t::reset ===========================================================

void priest_t::reset()
{
  player_t::reset();

  recast_mind_blast = 0;

  mana_resource.reset();

  init_party();
}

// priest_t::regen  ==========================================================

void priest_t::regen( double periodicity )
{
	mana_regen_while_casting = 0.00;

	if ( talent_meditation -> enabled )
		{
			mana_regen_while_casting += constants.meditation_value;
		}
	if (talent_holy_concentration -> rank )
		{
			mana_regen_while_casting += talent_holy_concentration -> rank * constants.holy_concentration_value;
		}

	player_t::regen( periodicity );
}


// priest_t::create_expression =================================================

action_expr_t* priest_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "recast_mind_blast" )
  {
    struct recast_mind_blast_expr_t : public action_expr_t
    {
      recast_mind_blast_expr_t( action_t* a ) : action_expr_t( a, "recast_mind_blast" ) { result_type = TOK_NUM; }
      virtual int evaluate() { result_num = action -> player -> cast_priest() -> recast_mind_blast; return TOK_NUM; }
    };
    return new recast_mind_blast_expr_t( a );
  }
  if ( name_str == "use_mind_blast" )
  {
    struct use_mind_blast_expr_t : public action_expr_t
    {
      use_mind_blast_expr_t( action_t* a ) : action_expr_t( a, "use_mind_blast" ) { result_type = TOK_NUM; }
      virtual int evaluate() { result_num = action -> player -> cast_priest() -> use_mind_blast; return TOK_NUM; }
    };
    return new use_mind_blast_expr_t( a );
  }
  if ( name_str == "use_shadow_word_death" )
  {
    struct use_shadow_word_death_expr_t : public action_expr_t
    {
      use_shadow_word_death_expr_t( action_t* a ) : action_expr_t( a, "use_shadow_word_death" ) { result_type = TOK_NUM; }
      virtual int evaluate() { result_num = action -> player -> cast_priest() -> use_shadow_word_death; return TOK_NUM; }
    };
    return new use_shadow_word_death_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// priest_t::resource_gain ===================================================

double priest_t::resource_gain( int       resource,
                                double    amount,
                                gain_t*   source,
                                action_t* action )
{
  double actual_amount = player_t::resource_gain( resource, amount, source, action );

  if ( resource == RESOURCE_MANA )
  {
    if ( source != gains_shadow_fiend &&
         source != gains_dispersion )
    {
      mana_resource.mana_gain += actual_amount;
    }
  }

  return actual_amount;
}

// priest_t::resource_loss ===================================================

double priest_t::resource_loss( int       resource,
                                double    amount,
                                action_t* action )
{
  double actual_amount = player_t::resource_loss( resource, amount, action );

  if ( resource == RESOURCE_MANA )
  {
    mana_resource.mana_loss += actual_amount;
  }

  return actual_amount;
}

// priest_t::get_talent_trees ===============================================

std::vector<talent_translation_t>& priest_t::get_talent_list()
{
  talent_list.clear();
  return talent_list;
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
      { "archangel",                                OPT_INT,    &( talent_archangel->rank                     ) },
      { "chakra",                                   OPT_INT,    &( talent_chakra->rank                        ) },    
      { "darkness",                                 OPT_INT,    &( talent_darkness->rank                      ) },
      { "dispersion",                               OPT_INT,    &( talent_dispersion->rank                    ) },
      { "divine_fury",                              OPT_INT,    &( talent_divine_fury->rank                   ) },
      { "evangelism",                               OPT_INT,    &( talent_evangelism->rank                    ) },     
      { "harnessed_shadows",                        OPT_INT,    &( talent_harnessed_shadows->rank             ) },
      { "holy_concentration",                       OPT_INT,    &( talent_holy_concentration->rank            ) },
      { "holy_word_chastise",                       OPT_INT,    &( talent_chastise->enabled                   ) },
      { "improved_devouring_plague",                OPT_INT,    &( talent_improved_devouring_plague->rank     ) },
      { "improved_inner_fire",                      OPT_INT,    &( talent_improved_inner_fire->rank           ) },
      { "improved_mind_blast",                      OPT_INT,    &( talent_improved_mind_blast->rank           ) },
      { "improved_shadow_word_pain",                OPT_INT,    &( talent_improved_shadow_word_pain->rank     ) },
      { "inner_focus",                              OPT_INT,    &( talent_inner_focus->rank                   ) },
      { "masochism",                                OPT_INT,    &( talent_masochism->rank                     ) },
      { "meditation",                               OPT_INT,    &( talent_meditation->enabled                 ) },
      { "mental_agility",                           OPT_INT,    &( talent_mental_agility->rank                ) },
      { "mind_flay",                                OPT_INT,    &( talent_mind_flay->enabled                  ) },
      { "mind_melt",                                OPT_INT,    &( talent_mind_melt->rank                     ) },
      { "pain_and_suffering",                       OPT_INT,    &( talent_pain_and_suffering->rank            ) },
      { "penance",                                  OPT_INT,    &( talent_penance->enabled                    ) },
      { "power_infusion",                           OPT_INT,    &( talent_power_infusion->rank                ) },
      { "shadow_form",                              OPT_INT,    &( talent_shadow_form->rank                   ) },
      { "shadow_power",                             OPT_INT,    &( talent_shadow_power->enabled               ) },
      { "shadowy_apparation",                       OPT_INT,    &( talent_shadowy_apparation->rank            ) },
      { "sin_and_punishment",                       OPT_INT,    &( talent_sin_and_punishment->rank            ) },
      { "state_of_mind",                            OPT_INT,    &( talent_state_of_mind->rank                 ) },
      { "twin_disciplines",                         OPT_INT,    &( talent_twin_disciplines->rank              ) },
      { "twisted_faith",                            OPT_INT,    &( talent_twisted_faith->rank                 ) },
      { "vampiric_embrace",                         OPT_INT,    &( talent_vampiric_embrace->rank              ) },
      { "vampiric_touch",                           OPT_INT,    &( talent_vampiric_touch->rank                ) },
      { "veiled_shadows",                           OPT_INT,    &( talent_veiled_shadows->rank                ) },
      // @option_doc loc=player/priest/glyphs title="Glyphs"
      { "glyph_hymn_of_hope",                       OPT_BOOL,   &( glyphs.hymn_of_hope                        ) },
      { "glyph_mind_flay",                          OPT_BOOL,   &( glyphs.mind_flay                           ) },
      { "glyph_penance",                            OPT_BOOL,   &( glyphs.penance                             ) },
      { "glyph_shadow_word_death",                  OPT_BOOL,   &( glyphs.shadow_word_death                   ) },
      { "glyph_shadow_word_pain",                   OPT_BOOL,   &( glyphs.shadow_word_pain                    ) },
      { "glyph_shadow",                             OPT_BOOL,   &( glyphs.shadow                              ) },
      { "glyph_smite",                              OPT_BOOL,   &( glyphs.smite                               ) },
      // @option_doc loc=player/priest/constants title="Constants"
      { "const.darkness_value",                     OPT_FLT,    &( constants.darkness_value                   ) },
      { "const.improved_shadow_word_pain_value",    OPT_FLT,    &( constants.improved_shadow_word_pain_value  ) },
      { "const.shadow_form_value",                  OPT_FLT,    &( constants.shadow_form_value                ) },
      { "const.twin_disciplines_value",             OPT_FLT,    &( constants.twin_disciplines_value           ) },
      // @option_doc loc=player/priest/coefficients title="Coefficients"
      { "power_mod.mind_blast",                     OPT_FLT,    &( power_mod.mind_blast                       ) },
      { "power_mod.mind_flay",                      OPT_FLT,    &( power_mod.mind_flay                        ) },
      { "power_mod.mind_spike",                     OPT_FLT,    &( power_mod.mind_spike                       ) },
      { "power_mod.shadow_word_death",              OPT_FLT,    &( power_mod.shadow_word_death                ) },
      { "power_mod.shadow_word_pain",               OPT_FLT,    &( power_mod.shadow_word_pain                 ) },
      { "power_mod.vampiric_touch",                 OPT_FLT,    &( power_mod.vampiric_touch                   ) },
      // @option_doc loc=player/priest/misc title="Misc"
      { "use_shadow_word_death",                    OPT_BOOL,   &( use_shadow_word_death                      ) },
      { "use_mind_blast",                           OPT_INT,    &( use_mind_blast                             ) },
      { "power_infusion_target",                    OPT_STRING, &( power_infusion_target_str                  ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, priest_options );
  }

  return options;
}

// priest_t::create_profile ===================================================

bool priest_t::create_profile( std::string& profile_str, int save_type )
{
  player_t::create_profile( profile_str, save_type );

  if ( save_type == SAVE_ALL )
  {
    std::string temp_str;
    if ( ! power_infusion_target_str.empty() ) profile_str += "power_infusion_target=" + power_infusion_target_str + "\n";
    if ( ( use_shadow_word_death ) || ( use_mind_blast != 1 ) )
    {
      profile_str += "## Variables\n";
    }
    if ( use_shadow_word_death ) 
    { 
      profile_str += "use_shadow_word_death=1\n"; 
    }
    if ( use_mind_blast != 1 ) 
    { 
      temp_str = util_t::to_string( use_mind_blast );
      profile_str += "use_mind_blast=" + temp_str + "\n"; 
    }
  }

  return true;
}

// priest_t::decode_set =====================================================

int priest_t::decode_set( item_t& item )
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

  bool is_caster = ( strstr( s, "circlet"   ) ||
		     strstr( s, "mantle"    ) ||
		     strstr( s, "raiments"  ) ||
		     strstr( s, "handwraps" ) ||
		     strstr( s, "pants"     ) );

  if ( strstr( s, "faith" ) )
  {
    if ( is_caster ) return SET_T7_CASTER;
  }
  if ( strstr( s, "sanctification" ) )
  {
    if ( is_caster ) return SET_T8_CASTER;
  }
  if ( strstr( s, "zabras" ) ||
       strstr( s, "velens" ) )
  {
    if ( is_caster ) return SET_T9_CASTER;
  }
  if ( strstr( s, "crimson_acolyte" ) )
  {
    is_caster = ( strstr( s, "cowl"      ) ||
		          strstr( s, "mantle"    ) ||
		          strstr( s, "raiments"  ) ||
		          strstr( s, "handwraps" ) ||
		          strstr( s, "pants"     ) );
    if ( is_caster ) return SET_T10_CASTER;
  }

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_priest  =================================================

player_t* player_t::create_priest( sim_t* sim, const std::string& name, race_type r )
{
  return new priest_t( sim, name, r );
}

// player_t::priest_init =====================================================

void player_t::priest_init( sim_t* sim )
{
  sim -> auras.mind_quickening = new aura_t( sim, "mind_quickening", 1, 0.0 );
  
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.fortitude      = new stat_buff_t( p, "fortitude",       STAT_STAMINA, 165.0, 1 );
    p -> buffs.power_infusion = new      buff_t( p, "power_infusion",             1,  15.0, 0 );
  }

}

// player_t::priest_combat_begin =============================================

void player_t::priest_combat_begin( sim_t* sim )
{
	if ( sim -> overrides.mind_quickening ) sim -> auras.mind_quickening -> override();
  
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.fortitude     ) p -> buffs.fortitude     -> override( 1, 165.0 * 1.30 );
    }
  }
}
