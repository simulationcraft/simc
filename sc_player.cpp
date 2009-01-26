// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_judgement_of_wisdom ==============================================

static void trigger_judgement_of_wisdom( action_t* a )
{
  if( ! a -> sim -> target -> debuffs.judgement_of_wisdom ) return;

  player_t* p = a -> player;

  double base_mana = p -> resource_base[ RESOURCE_MANA ];

  if( base_mana <= 0 )
    return;

  if( ! a -> sim -> roll( 0.25 ) )
    return;

  p -> procs.judgement_of_wisdom -> occur();

  p -> resource_gain( RESOURCE_MANA, base_mana * 0.02, p -> gains.judgement_of_wisdom );
}

// trigger_focus_magic_feedback =============================================

static void trigger_focus_magic_feedback( spell_t* spell )
{
  struct expiration_t : public event_t
  {
    player_t* mage;

    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p ), mage( p -> buffs.focus_magic )
    {
      name = "Focus Magic Feedback Expiration";
      mage -> aura_gain( "Focus Magic Feedback" );
      mage -> buffs.focus_magic_feedback = 1;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      mage -> aura_loss( "Focus Magic Feedback" );
      mage -> buffs.focus_magic_feedback = 0;
      mage -> expirations.focus_magic_feedback = 0;
    }
  };

  player_t* p = spell -> player;

  if( ! p -> buffs.focus_magic ) return;

  event_t*& e = p -> buffs.focus_magic -> expirations.focus_magic_feedback;

  if( e )
  {
    e -> reschedule( 10.0 );
  }
  else
  {
    e = new ( p -> sim ) expiration_t( p -> sim, p );
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Player - Gear
// ==========================================================================

// player_t::gear_t::allocate_spell_power_budget( sim_t* sim ) ==============

void player_t::gear_t::allocate_spell_power_budget( sim_t* sim )
{
  if( sim -> debug ) report_t::log( sim, "Allocating spell_power budget...." );   

  if(  hit_rating   == 0 &&
       crit_rating  == 0 &&
       haste_rating == 0 )
  {
    spell_power[ SCHOOL_MAX ] = spell_power_budget;
    return;
  }

  if( budget_slots == 0 ) budget_slots = 16;

  double spell_power_statmod = 0.855;

  double slot_power = spell_power_budget / (double) budget_slots;
  double slot_hit   = hit_rating         / (double) budget_slots;
  double slot_crit  = crit_rating        / (double) budget_slots;
  double slot_haste = haste_rating       / (double) budget_slots;

  double target = pow( slot_power * spell_power_statmod, 1.5 );

  double cost = pow( slot_hit,   1.5 ) +
                pow( slot_crit,  1.5 ) +
                pow( slot_haste, 1.5 );

  spell_power[ SCHOOL_MAX ] = (int16_t) ( budget_slots * pow( target - cost, 1.0 / 1.5 ) / spell_power_statmod );
  
  if( sim -> debug ) report_t::log( sim, "slot_power=%.1f  slot_hit=%.1f  slot_crit=%.1f  slot_haste=%.1f  target=%.1f  cost=%.1f  spell_power=%d\n",
                   slot_power, slot_hit, slot_crit, slot_haste, target, cost, spell_power[ SCHOOL_MAX ] );
}

// player_t::gear_t::allocate_attack_power_budget( sim_t* sim ) =============

void player_t::gear_t::allocate_attack_power_budget( sim_t* sim )
{
  if( sim -> debug ) report_t::log( sim, "Allocating attack_power budget...." );   

  // double attack_power_statmod = 0.5;

  assert(0);
}

// ==========================================================================
// Player
// ==========================================================================

// player_t::player_t =======================================================

player_t::player_t( sim_t*             s, 
                    int8_t             t,
		    const std::string& n ) :
  sim(s), name_str(n), next(0), type(t), level(70), party(0), gcd_ready(0), base_gcd(1.5), sleeping(0), pet_list(0),
  // Haste
  base_haste_rating(0), initial_haste_rating(0), haste_rating(0), haste(1.0),
  // Spell Mechanics
  base_spell_power(0),
  base_spell_hit(0),         initial_spell_hit(0),         spell_hit(0),
  base_spell_crit(0),        initial_spell_crit(0),        spell_crit(0),
  base_spell_penetration(0), initial_spell_penetration(0), spell_penetration(0),
  base_mp5(0),               initial_mp5(0),               mp5(0),
  spell_power_multiplier(1.0),  initial_spell_power_multiplier(1.0),
  spell_power_per_intellect(0), initial_spell_power_per_intellect(0),
  spell_power_per_spirit(0),    initial_spell_power_per_spirit(0),
  spell_crit_per_intellect(0),  initial_spell_crit_per_intellect(0),
  mp5_per_intellect(0), spirit_regen_while_casting(0), energy_regen_per_second(0), focus_regen_per_second(0),
  last_cast(0),
  // Attack Mechanics
  base_attack_power(0),       initial_attack_power(0),        attack_power(0),
  base_attack_hit(0),         initial_attack_hit(0),          attack_hit(0),
  base_attack_expertise(0),   initial_attack_expertise(0),    attack_expertise(0),
  base_attack_crit(0),        initial_attack_crit(0),         attack_crit(0),
  base_attack_penetration(0), initial_attack_penetration(0),  attack_penetration(0),
  attack_power_multiplier(1.0), initial_attack_power_multiplier(1.0),
  attack_power_per_strength(0), initial_attack_power_per_strength(0),
  attack_power_per_agility(0),  initial_attack_power_per_agility(0),
  attack_crit_per_agility(0),   initial_attack_crit_per_agility(0),
  position( POSITION_BACK ),
  // Resources 
  resource_constrained(0),           resource_constrained_count(0),
  resource_constrained_total_dmg(0), resource_constrained_total_time(0),
  mana_per_intellect(0), health_per_stamina(0),
  // Consumables
  elixir_guardian( ELIXIR_NONE ),
  elixir_battle( ELIXIR_NONE ),
  flask( FLASK_NONE ),
  food( FOOD_NONE ),
  // Events
  executing(0), channeling(0), in_combat(false),
  // Actions
  action_list(0),
  // Reporting
  quiet(0), last_foreground_action(0),
  last_action_time(0), total_seconds(0), total_waiting(0), iteration_dmg(0), total_dmg(0), 
  dps(0), dps_min(0), dps_max(0), dps_std_dev(0), dps_error(0), dpr(0), rps_gain(0), rps_loss(0),
  proc_list(0), gain_list(0), stats_list(0), uptime_list(0)
{
  if( sim -> debug ) report_t::log( sim, "Creating Player %s", name() );
  next = sim -> player_list;
  sim -> player_list = this;

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute[ i ] = attribute_base[ i ] = attribute_initial[ i ] = 0;
    attribute_multiplier[ i ] = attribute_multiplier_initial[ i ] = 1.0;
  }
  for( int i=0; i <= SCHOOL_MAX; i++ )
  {
    initial_spell_power[ i ] = spell_power[ i ] = 0;
  }

  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_base[ i ] = resource_initial[ i ] = resource_max[ i ] = resource_current[ i ] = 0;

    resource_lost[ i ] = resource_gained[ i ] = 0;
  }

  // Setup default gear profiles

  if( ! is_pet() )
  {
    for( int i=0; i < ATTRIBUTE_MAX; i++ )
    {
      gear.attribute[ i ] = sim -> gear_default.attribute[ i ];
    }
    gear.spell_power[ SCHOOL_MAX ] = sim -> gear_default.spell_power;
    gear.attack_power              = sim -> gear_default.attack_power;
    gear.expertise_rating          = sim -> gear_default.expertise_rating;
    gear.armor_penetration_rating  = sim -> gear_default.armor_penetration_rating;
    gear.hit_rating                = sim -> gear_default.hit_rating;
    gear.crit_rating               = sim -> gear_default.crit_rating;
    gear.haste_rating              = sim -> gear_default.haste_rating;
  }

  main_hand_weapon.slot = SLOT_MAIN_HAND;
   off_hand_weapon.slot = SLOT_OFF_HAND;
     ranged_weapon.slot = SLOT_RANGED;
}

// player_t::~player_t =====================================================

player_t::~player_t() 
{
  while( action_t* a = action_list )
  {
    action_list = a -> next;
    delete a;
  }
  while( proc_t* p = proc_list )
  {
    proc_list = p -> next;
    delete p;
  }
  while( gain_t* g = gain_list )
  {
    gain_list = g -> next;
    delete g;
  }
  while( stats_t* s = stats_list )
  {
    stats_list = s -> next;
    delete s;
  }
  while( uptime_t* u = uptime_list )
  {
    uptime_list = u -> next;
    delete u;
  }
}

// player_t::init ==========================================================

void player_t::init() 
{
  if( sim -> debug ) report_t::log( sim, "Initializing player %s", name() );   

  init_rating();
  init_base();
  init_core();
  init_spell();
  init_attack();
  init_weapon( &main_hand_weapon, main_hand_str );
  init_weapon(  &off_hand_weapon,  off_hand_str );
  init_weapon(    &ranged_weapon,    ranged_str );
  init_resources();
  init_consumables();
  init_actions();
  init_stats();
}
 
// player_t::init_core ======================================================

void player_t::init_core() 
{
  if( initial_haste_rating == 0 ) 
  {
    initial_haste_rating = base_haste_rating + gear.haste_rating + gear.haste_rating_enchant;
  }

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    if( attribute_initial[ i ] == 0 )
    {
      attribute_initial[ i ] = attribute_base[ i ] + gear.attribute[ i ] + gear.attribute_enchant[ i ];
    }
    attribute[ i ] = attribute_initial[ i ];
  }

  if( ! is_pet() )
  {
    initial_haste_rating += sim -> gear_delta.haste_rating;

    for( int i=0; i < ATTRIBUTE_MAX; i++ )
    {
      attribute_initial[ i ] += sim -> gear_delta.attribute[ i ];
    }
  }

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute[ i ] = attribute_initial[ i ];
  }
}

// player_t::init_spell =====================================================

void player_t::init_spell() 
{
  if( gear.spell_power_budget != 0 &&
      gear.spell_power[ SCHOOL_MAX ] == 0 ) 
  {
    gear.allocate_spell_power_budget( sim );
  }

  if( initial_spell_power[ SCHOOL_MAX ] == 0 )
  {
    for( int i=0; i <= SCHOOL_MAX; i++ )
    {
      initial_spell_power[ i ] = gear.spell_power[ i ] + gear.spell_power_enchant[ i ];
    }
    initial_spell_power[ SCHOOL_MAX ] += base_spell_power;
  }

  if( initial_spell_hit == 0 )
  {
    initial_spell_hit = base_spell_hit + ( gear.hit_rating + 
					   gear.hit_rating_enchant ) / rating.spell_hit;
  }
  if( initial_spell_crit == 0 )
  {
    initial_spell_crit = base_spell_crit + ( gear.crit_rating + 
					     gear.crit_rating_enchant ) / rating.spell_crit;
  }
  if( initial_spell_penetration == 0 )
  {
    initial_spell_penetration = ( base_spell_penetration +   
				  gear.spell_penetration + 
				  gear.spell_penetration_enchant );
  }
  if( initial_mp5 == 0 ) 
  {
    initial_mp5 = base_mp5 + gear.mp5 + gear.mp5_enchant;

    // FIXME! All these static buffs will go away as class support comes along.
    if( buffs.blessing_of_wisdom ) initial_mp5 += ( level <= 70 ) ? 49 : 91;
  }

  if( ! is_pet() )
  {
    initial_spell_power[ SCHOOL_MAX ] += sim -> gear_delta.spell_power;
    initial_spell_hit  += sim -> gear_delta. hit_rating / rating.spell_hit;
    initial_spell_crit += sim -> gear_delta.crit_rating / rating.spell_crit;
  }
}

// player_t::init_attack ====================================================

void player_t::init_attack() 
{
  if( gear.attack_power_budget != 0 &&
      gear.attack_power == 0 ) 
  {
    gear.allocate_attack_power_budget( sim );
  }

  if( initial_attack_power == 0 )
  {
    initial_attack_power = base_attack_power + gear.attack_power + gear.attack_power_enchant;
  }
  if( initial_attack_hit == 0 )
  {
    initial_attack_hit = base_attack_hit + ( gear.hit_rating + 
					     gear.hit_rating_enchant ) / rating.attack_hit;
  }
  if( initial_attack_crit == 0 )
  {
    initial_attack_crit = base_attack_crit + ( gear.crit_rating + 
					       gear.crit_rating_enchant ) / rating.attack_crit;
  }
  if( initial_attack_expertise == 0 )
  {
    initial_attack_expertise = base_attack_expertise + ( gear.expertise_rating + 
							 gear.expertise_rating_enchant ) / rating.expertise;
  }
  if( initial_attack_penetration == 0 )
  {
    initial_attack_penetration = base_attack_penetration + ( gear.armor_penetration_rating + 
							     gear.armor_penetration_rating_enchant ) / rating.armor_penetration;
  }

  if( ! is_pet() )
  {
    initial_attack_power       += sim -> gear_delta.attack_power;
    initial_attack_hit         += sim -> gear_delta.hit_rating               / rating.attack_hit;
    initial_attack_crit        += sim -> gear_delta.crit_rating              / rating.attack_crit;
    initial_attack_expertise   += sim -> gear_delta.expertise_rating         / rating.expertise;
    initial_attack_penetration += sim -> gear_delta.armor_penetration_rating / rating.armor_penetration;
  }
}

// player_t::init_weapon ===================================================

void player_t::init_weapon( weapon_t*    w, 
			    std::string& encoding )
{
  if( encoding.empty() ) return;

  double weapon_dps = 0;

  std::vector<std::string> splits;
  int size = util_t::string_split( splits, encoding, "," );

  for( int i=0; i < size; i++ )
  {
    std::string& s = splits[ i ];
    int t;

    for( t=0; t < WEAPON_MAX; t++ )
    {
      const char* name = util_t::weapon_type_string( t );
      if( s == name ) break;
    }

    if( t < WEAPON_MAX )
    {
      w -> type = t;
    }
    else
    {
      std::string parm, value;
      bool invalid = false;

      if( 2 != util_t::string_split( s, "=", "S S", &parm, &value ) )
      {
	invalid = true;
      }
      if( parm == "dmg" || parm == "damage" )
      {
	w -> damage = atof( value.c_str() );
	assert( w -> damage != 0 );
      }
      else if( parm == "dps" )
      {
	weapon_dps = atof( value.c_str() );
	assert( weapon_dps != 0 );
      }
      else if( parm == "speed" )
      {
	w -> swing_time = atof( value.c_str() );
	assert( w -> swing_time != 0 );
      }
      else if( parm == "school" )
      {
	for( int j=0; j <= SCHOOL_MAX; j++ )
	{
	  if( j == SCHOOL_MAX ) invalid = true;
	  if( value == util_t::school_type_string( j ) )
	  {
	    w -> school = j;
	    break;
	  }
	}
      }
      else if( parm == "enchant" )
      {
	for( int j=0; j <= WEAPON_ENCHANT_MAX; j++ )
	{
	  if( j == WEAPON_ENCHANT_MAX ) invalid = true;
	  if( value == util_t::weapon_enchant_type_string( j ) )
	  {
	    w -> enchant = j;
	    break;
	  }
	}
      }
      else if( parm == "buff" )
      {
	for( int j=0; j <= WEAPON_BUFF_MAX; j++ )
	{
	  if( j == WEAPON_BUFF_MAX ) invalid = true;
	  if( value == util_t::weapon_buff_type_string( j ) )
	  {
	    w -> buff = j;
	    break;
	  }
	}
      }
      else invalid = true;

      if( invalid )
      {
	printf( "Invalid weapon encoding: %s\n", encoding.c_str() );
	assert(0);
      }
    }
  }

  if( weapon_dps != 0 ) w -> damage = weapon_dps * w -> swing_time;

  if( w -> slot == SLOT_MAIN_HAND ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_2H );
  if( w -> slot == SLOT_OFF_HAND  ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_2H );
  if( w -> slot == SLOT_RANGED    ) assert( w -> type == WEAPON_NONE || ( w -> type > WEAPON_2H && w -> type < WEAPON_RANGED ) );
}

// player_t::init_resources ================================================

void player_t::init_resources( bool force ) 
{
  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    if( force || resource_initial[ i ] == 0 )
    {
      resource_initial[ i ] = resource_base[ i ] + gear.resource[ i ] + gear.resource_enchant[ i ];

      if( i == RESOURCE_MANA   ) resource_initial[ i ] += intellect() * mana_per_intellect;
      if( i == RESOURCE_HEALTH ) resource_initial[ i ] +=   stamina() * health_per_stamina;
    }
    resource_current[ i ] = resource_max[ i ] = resource_initial[ i ];
  }

  resource_constrained = 0;
  resource_constrained_count = 0;
  resource_constrained_total_dmg = 0;
  resource_constrained_total_time = 0;

  if( timeline_resource.empty() )
  {
    int size = (int) sim -> max_time;
    if( size == 0 ) size = 600; // Default to 10 minutes
    size *= 2;
    timeline_resource.insert( timeline_resource.begin(), size, 0 );
  }
}

// player_t::init_consumables ==============================================

void player_t::init_consumables()
{
  consumable_t::init_flask  ( this );
  consumable_t::init_elixirs( this );
  consumable_t::init_food   ( this );
}

// player_t::init_actions ==================================================

void player_t::init_actions() 
{
  if( ! action_list_prefix.empty() || 
      ! action_list_str.empty()    || 
      ! action_list_postfix.empty() )
  {
    std::vector<std::string> splits;
    int size = 0;

    if( ! action_list_prefix.empty() )
    {
      if( sim -> debug ) report_t::log( sim, "Player %s: action_list_prefix=%s", name(), action_list_prefix.c_str() );   
      size = util_t::string_split( splits, action_list_prefix, "/" );
    }
    if( ! action_list_str.empty() )
    {
      if( sim -> debug ) report_t::log( sim, "Player %s: action_list_str=%s", name(), action_list_str.c_str() );   
      size = util_t::string_split( splits, action_list_str, "/" );
    }
    if( ! action_list_postfix.empty() )
    {
      if( sim -> debug ) report_t::log( sim, "Player %s: action_list_postfix=%s", name(), action_list_postfix.c_str() );   
      size = util_t::string_split( splits, action_list_postfix, "/" );
    }

    for( int i=0; i < size; i++ )
    {
      std::string action_name    = splits[ i ];
      std::string action_options = "";

      std::string::size_type cut_pt = action_name.find_first_of( "," );       

      if( cut_pt != action_name.npos )
      {
	action_options = action_name.substr( cut_pt + 1 );
	action_name    = action_name.substr( 0, cut_pt );
      }

      if( ! action_t::create_action( this, action_name, action_options ) )
      {
	printf( "util_t::player: Unknown action: %s\n", splits[ i ].c_str() );
	assert( false );
      }
    }
  }

  if( ! action_list_skip.empty() )
  {
    if( sim -> debug ) report_t::log( sim, "Player %s: action_list_skip=%s", name(), action_list_skip.c_str() );   
    std::vector<std::string> splits;
    int size = util_t::string_split( splits, action_list_skip, "/" );
    for( int i=0; i < size; i++ )
    {
      action_t* action = find_action( splits[ i ] );
      if( action ) action -> background = true;
    }
  }
}

// player_t::init_rating ===================================================

void player_t::init_rating() 
{
  rating.init( level );
}

// player_t::init_stats ====================================================

void player_t::init_stats() 
{
  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_lost[ i ] = resource_gained[ i ] = 0;
  }

  gains.ashtongue_talisman    = get_gain( "ashtongue_talisman" );
  gains.dark_rune             = get_gain( "dark_rune" );
  gains.energy_regen          = get_gain( "energy_regen" );
  gains.focus_regen           = get_gain( "focus_regen" );
  gains.innervate             = get_gain( "innervate" );
  gains.glyph_of_innervate    = get_gain( "glyph_of_innervate" );
  gains.judgement_of_wisdom   = get_gain( "judgement_of_wisdom" );
  gains.mana_gem              = get_gain( "mana_gem" );
  gains.mana_potion           = get_gain( "mana_potion" );
  gains.mana_spring           = get_gain( "mana_spring" );
  gains.mana_tide             = get_gain( "mana_tide" );
  gains.mark_of_defiance      = get_gain( "mark_of_defiance" );
  gains.mp5_regen             = get_gain( "mp5_regen" );
  gains.replenishment         = get_gain( "replenishment" );
  gains.restore_mana          = get_gain( "restore_mana" );
  gains.spellsurge            = get_gain( "spellsurge" );
  gains.spirit_regen          = get_gain( "spirit_regen" );
  gains.vampiric_touch        = get_gain( "vampiric_touch" );
  gains.water_elemental_regen = get_gain( "water_elemental" );
  gains.tier4_2pc             = get_gain( "tier4_2pc" );
  gains.tier4_4pc             = get_gain( "tier4_4pc" );
  gains.tier5_2pc             = get_gain( "tier5_2pc" );
  gains.tier5_4pc             = get_gain( "tier5_4pc" );
  gains.tier6_2pc             = get_gain( "tier6_2pc" );
  gains.tier6_4pc             = get_gain( "tier6_4pc" );
  gains.tier7_2pc             = get_gain( "tier7_2pc" );
  gains.tier7_4pc             = get_gain( "tier7_4pc" );

  procs.ashtongue_talisman           = get_proc( "ashtongue_talisman" );
  procs.dying_curse                  = get_proc( "dying_curse" );
  procs.elder_scribes                = get_proc( "elder_scribes" );
  procs.embrace_of_the_spider        = get_proc( "embrace_of_the_spider" );
  procs.eternal_sage                 = get_proc( "eternal_sage" );
  procs.extract_of_necromatic_power  = get_proc( "extract_of_necromatic_power" );
  procs.eye_of_magtheridon           = get_proc( "eye_of_magtheridon" );
  procs.forge_ember	             = get_proc( "forge_ember" );
  procs.judgement_of_wisdom          = get_proc( "judgement_of_wisdom" );
  procs.lightning_capacitor          = get_proc( "lightning_capacitor" );
  procs.mark_of_defiance             = get_proc( "mark_of_defiance" );
  procs.mystical_skyfire             = get_proc( "mystical_skyfire" );
  procs.quagmirrans_eye              = get_proc( "quagmirrans_eye" );
  procs.sextant_of_unstable_currents = get_proc( "sextant_of_unstable_currents" );
  procs.shiffars_nexus_horn          = get_proc( "shiffars_nexus_horn" );
  procs.spellstrike                  = get_proc( "spellstrike" );
  procs.sundial_of_the_exiled        = get_proc( "sundial_of_the_exiled" );
  procs.egg_of_mortal_essence        = get_proc( "egg_of_mortal_essence" );
  procs.thunder_capacitor            = get_proc( "thunder_capacitor" );
  procs.timbals_crystal              = get_proc( "timbals_crystal" );
  procs.windfury                     = get_proc( "windfury" );
  procs.wrath_of_cenarius            = get_proc( "wrath_of_cenarius" );
  procs.tier4_2pc                    = get_proc( "tier4_2pc" );
  procs.tier4_4pc                    = get_proc( "tier4_4pc" );
  procs.tier5_2pc                    = get_proc( "tier5_2pc" );
  procs.tier5_4pc                    = get_proc( "tier5_4pc" );
  procs.tier6_2pc                    = get_proc( "tier6_2pc" );
  procs.tier6_4pc                    = get_proc( "tier6_4pc" );
  procs.tier7_2pc                    = get_proc( "tier7_2pc" );
  procs.tier7_4pc                    = get_proc( "tier7_4pc" );

  iteration_dps.clear();
  iteration_dps.insert( iteration_dps.begin(), sim -> iterations, 0 );
}

// player_t::composite_attack_power ========================================

double player_t::composite_attack_power() 
{
  double ap = attack_power;

  ap += attack_power_per_strength * strength();
  ap += attack_power_per_agility  * agility();

  double best_buff=0;
  if( buffs.blessing_of_might > best_buff ) best_buff = buffs.blessing_of_might;
  if( buffs.battle_shout      > best_buff ) best_buff = buffs.battle_shout;
  ap += best_buff;

  return ap;
}

// player_t::composite_attack_crit =========================================

double player_t::composite_attack_crit()
{
  return attack_crit + attack_crit_per_agility * agility();
}

// player_t::composite_spell_power ========================================

double player_t::composite_spell_power( int8_t school ) 
{
  double sp = spell_power[ school ];

  if( school == SCHOOL_FROSTFIRE )
  {
    sp = std::max( spell_power[ SCHOOL_FROST ],
		   spell_power[ SCHOOL_FIRE  ] );
  }

  if( school != SCHOOL_MAX ) sp += spell_power[ SCHOOL_MAX ];

  sp += spell_power_per_intellect * intellect();
  sp += spell_power_per_spirit    * spirit();

  return sp;
}

// player_t::composite_spell_crit ==========================================

double player_t::composite_spell_crit()
{
  return spell_crit + spell_crit_per_intellect * intellect();
}

// player_t::composite_attack_power_multiplier =============================

double player_t::composite_attack_power_multiplier()
{
  double m = attack_power_multiplier;

  if( buffs.unleashed_rage ||
      buffs.trueshot_aura  )
  {
    m *= 1.10;
  }

  return m;
}

// player_t::composite_attribute_multiplier ================================

double player_t::composite_attribute_multiplier( int8_t attr )
{
  double m = attribute_multiplier[ attr ]; 
  if( buffs.blessing_of_kings ) m *= 1.10; 
  return m;
}

// player_t::strength() ====================================================

double player_t::strength()
{ 
  double a = attribute[ ATTR_STRENGTH ];

  a += buffs.mark_of_the_wild;
  a += buffs.strength_of_earth;

  return a * composite_attribute_multiplier( ATTR_STRENGTH ); 
}

// player_t::agility() =====================================================

double player_t::agility()   
{
  double a = attribute[ ATTR_AGILITY ];

  a += buffs.mark_of_the_wild;
  a += buffs.strength_of_earth;

  return a * composite_attribute_multiplier( ATTR_AGILITY );
}

// player_t::stamina() =====================================================

double player_t::stamina()
{
  double a = attribute[ ATTR_STAMINA ];
 
  a += buffs.mark_of_the_wild;
  a += buffs.fortitude;

  return a * composite_attribute_multiplier( ATTR_STAMINA );
}

// player_t::intellect() ===================================================

double player_t::intellect() 
{ 
  double a = attribute[ ATTR_INTELLECT ];

  a += buffs.mark_of_the_wild;
  a += buffs.arcane_brilliance;

  return a * composite_attribute_multiplier( ATTR_INTELLECT );
}

// player_t::spirit() ======================================================

double player_t::spirit()
{ 
  double a = attribute[ ATTR_SPIRIT ];

  a += buffs.mark_of_the_wild;
  a += buffs.divine_spirit;

  return a * composite_attribute_multiplier( ATTR_SPIRIT );
}

// player_t::combat_begin ==================================================

void player_t::combat_begin() 
{
  if( sim -> debug ) report_t::log( sim, "Combat begins for player %s", name() );   

  if( action_list && ! is_pet() && ! sleeping ) 
  {
    schedule_ready();
  }

  double max_mana = resource_max[ RESOURCE_MANA ];
  if( max_mana > 0 ) get_gain( "initial_mana" ) -> add( max_mana );
}

// player_t::combat_end ====================================================

void player_t::combat_end() 
{
  if( sim -> debug ) report_t::log( sim, "Combat ends for player %s", name() );   

  double iteration_seconds = last_action_time;

  if( iteration_seconds > 0 )
  {
    total_seconds += iteration_seconds;

    for( pet_t* pet = pet_list; pet; pet = pet -> next_pet ) 
    {
      iteration_dmg += pet -> iteration_dmg;
    }
    iteration_dps[ sim -> current_iteration ] = iteration_dmg / iteration_seconds;
  }
}

// player_t::reset =========================================================

void player_t::reset() 
{
  if( sim -> debug ) report_t::log( sim, "Reseting player %s", name() );   

  last_cast = 0;
  gcd_ready = 0;

  haste_rating = initial_haste_rating;
  recalculate_haste();

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute           [ i ] = attribute_initial           [ i ];
    attribute_multiplier[ i ] = attribute_multiplier_initial[ i ];
  }

  for( int i=0; i <= SCHOOL_MAX; i++ )
  {
    spell_power[ i ] = initial_spell_power[ i ];
  }

  spell_hit         = initial_spell_hit;
  spell_crit        = initial_spell_crit;
  spell_penetration = initial_spell_penetration;
  mp5               = initial_mp5;

  attack_power       = initial_attack_power;
  attack_hit         = initial_attack_hit;
  attack_expertise   = initial_attack_expertise;
  attack_crit        = initial_attack_crit;
  attack_penetration = initial_attack_penetration;

  spell_power_multiplier    = initial_spell_power_multiplier;
  spell_power_per_intellect = initial_spell_power_per_intellect;
  spell_power_per_spirit    = initial_spell_power_per_spirit;
  spell_crit_per_intellect  = initial_spell_crit_per_intellect;
  
  attack_power_multiplier   = initial_attack_power_multiplier;
  attack_power_per_strength = initial_attack_power_per_strength;
  attack_power_per_agility  = initial_attack_power_per_agility;
  attack_crit_per_agility   = initial_attack_crit_per_agility;

  last_foreground_action = 0;
  last_action_time = 0;

  init_resources( true );

  executing = 0;
  channeling = 0;
  in_combat = false;
  iteration_dmg = 0;
 
  main_hand_weapon.buff = WEAPON_BUFF_NONE;
   off_hand_weapon.buff = WEAPON_BUFF_NONE;
     ranged_weapon.buff = WEAPON_BUFF_NONE;

  main_hand_weapon.buff_bonus = 0;
   off_hand_weapon.buff_bonus = 0;
     ranged_weapon.buff_bonus = 0;

  elixir_battle   = ELIXIR_NONE;
  elixir_guardian = ELIXIR_NONE;
  flask           = FLASK_NONE;
  food			  = FOOD_NONE;

  buffs.reset();
  expirations.reset();
  cooldowns.reset();
  
  for( action_t* a = action_list; a; a = a -> next ) 
  {
    a -> reset();
  }

  if( sleeping ) quiet = 1;
}

// player_t::schedule_ready =================================================

void player_t::schedule_ready( double delta_time,
			       bool   waiting )
{
  executing = 0;
  channeling = 0;

  double gcd_adjust = gcd_ready - ( sim -> current_time + delta_time );
  if( gcd_adjust > 0 ) delta_time += gcd_adjust;

  if( waiting )
  {
    total_waiting += delta_time;
  }
  else
  {
    double lag = 0;

    if( type == PLAYER_GUARDIAN )
    {
      lag = 0;
    }
    else if( type == PLAYER_PET )
    {
      lag = sim -> pet_lag;
    }
    else
    {
      lag = sim -> lag;

      if( last_foreground_action && last_foreground_action -> channeled ) 
      {
	lag += sim -> channel_penalty;
      }

      if( gcd_adjust > 0 ) 
      {
	lag += sim -> gcd_penalty;
      }
    }

    if( lag > 0 ) lag += ( sim -> rng -> real() - 0.5 ) * 0.1;
    if( lag < 0 ) lag = 0;

    delta_time += lag;
  }

  if( last_foreground_action ) 
  {
    last_foreground_action -> stats -> total_execute_time += delta_time;
  }
  
  new ( sim ) player_ready_event_t( sim, this, delta_time );
}

// player_t::execute_action =================================================

action_t* player_t::execute_action()
{
  action_t* action=0;

  for( action = action_list; action; action = action -> next )
  {
    if( action -> background )
      continue;

    if( action -> ready() )
      break;
  }

  if( action ) 
  {
    action -> schedule_execute();
  }
  else
  {
    check_resources();
  }

  last_foreground_action = action;

  return action;
}

// player_t::regen =========================================================

void player_t::regen( double periodicity )
{
  if( sim -> infinite_resource[ RESOURCE_ENERGY ] == 0 && resource_max[ RESOURCE_ENERGY ] > 0 )
  {
    double energy_regen = periodicity * energy_regen_per_second;

    resource_gain( RESOURCE_ENERGY, energy_regen, gains.energy_regen );
  }

  if( sim -> infinite_resource[ RESOURCE_FOCUS ] == 0 && resource_max[ RESOURCE_FOCUS ] > 0 )
  {
    double focus_regen = periodicity * focus_regen_per_second;

    resource_gain( RESOURCE_FOCUS, focus_regen, gains.focus_regen );
  }

  if( sim -> infinite_resource[ RESOURCE_MANA ] == 0 && resource_max[ RESOURCE_MANA ] > 0 )
  {
    double spirit_regen = periodicity * spirit_regen_per_second();

    if( buffs.innervate )
    {
      if( buffs.glyph_of_innervate ) 
      {
        resource_gain( RESOURCE_MANA, spirit_regen, gains.glyph_of_innervate );
      }

      spirit_regen *= 4.0;

      resource_gain( RESOURCE_MANA, spirit_regen, gains.innervate );      
    }
    else if( buffs.glyph_of_innervate )
    {
      resource_gain( RESOURCE_MANA, spirit_regen, gains.glyph_of_innervate );
    }
    else if( recent_cast() )
    {
      spirit_regen *= spirit_regen_while_casting;

      resource_gain( RESOURCE_MANA, spirit_regen, gains.spirit_regen );      
    }

    double mp5_regen = periodicity * ( mp5 + intellect() * mp5_per_intellect ) / 5.0;

    resource_gain( RESOURCE_MANA, mp5_regen, gains.mp5_regen );

    if( buffs.replenishment )
    {
      double replenishment_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.0025 / 1.0;

      resource_gain( RESOURCE_MANA, replenishment_regen, gains.replenishment );
    }

    if( buffs.water_elemental_regen )
    {
      double water_elemental_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.006 / 5.0;

      resource_gain( RESOURCE_MANA, water_elemental_regen, gains.water_elemental_regen );
    }
  }

  int8_t resource_type = primary_resource();

  if( resource_type != RESOURCE_NONE )
  {
    int index = (int) sim -> current_time;
    int size = timeline_resource.size();

    if( index >= size ) timeline_resource.insert( timeline_resource.begin() + size, size, 0 );

    timeline_resource[ index ] += resource_current[ resource_type ];
  }
}

// player_t::resource_loss =================================================

void player_t::resource_loss( int8_t resource,
                              double amount )
{
  if( amount == 0 ) return;

  if( sim -> infinite_resource[ resource ] == 0 )
  {
    resource_current[ resource ] -= amount;
  }

  resource_lost[ resource ] += amount;

  if( resource == RESOURCE_MANA )
  {
    last_cast = sim -> current_time;
  }

  if( sim -> debug ) report_t::log( sim, "Player %s spends %.0f %s", name(), amount, util_t::resource_type_string( resource ) );
}

// player_t::resource_gain =================================================

void player_t::resource_gain( int8_t  resource,
                              double  amount,
                              gain_t* source )
{
  if( sleeping ) return;

  double actual_amount = std::min( amount, resource_max[ resource ] - resource_current[ resource ] );
  if( actual_amount > 0 )
  {
    resource_current[ resource ] += actual_amount;
  }

  resource_gained [ resource ] += actual_amount;
  if( source ) source -> add( actual_amount );

  if( sim -> log ) 
    report_t::log( sim, "%s gains %.0f (%.0f) %s from %s", 
                   name(), actual_amount, amount, util_t::resource_type_string( resource ), source ? source -> name() : "unknown" );
}

// player_t::resource_available ============================================

bool player_t::resource_available( int8_t resource,
				   double cost )
{
  if( resource == RESOURCE_NONE || cost == 0 )
  {
    return true;
  }

  return resource_current[ resource ] >= cost;
}

// player_t::check_resources ===============================================

void player_t::check_resources()
{
  return; // FIXME! Work on this later.

  if( ! resource_constrained )
  {
    for( action_t* a = action_list; a; a = a -> next )
    {
      if( ! a -> background && a -> harmful )
      {
	if( resource_available( a -> resource, a -> cost() ) )
	{
	  // Not resource constrained.
	  return;
	}
      }
    }

    resource_constrained = 1;
    resource_constrained_count++;
    resource_constrained_total_time += sim -> current_time;
    resource_constrained_total_dmg  += iteration_dmg;

    if( sim -> debug ) report_t::log( sim, "Player %s is resource constrained.", name() );
  }
}

// player_t::summon_pet =====================================================

void player_t::summon_pet( const char* pet_name )
{
  for( pet_t* p = pet_list; p; p = p -> next_pet )
  {
    if( p -> name_str == pet_name )
    {
      p -> summon();
      return;
    }
  }
  assert(0);
}

// player_t::dismiss_pet ====================================================

void player_t::dismiss_pet( const char* pet_name )
{
  for( pet_t* p = pet_list; p; p = p -> next_pet )
  {
    if( p -> name_str == pet_name )
    {
      p -> dismiss();
      return;
    }
  }
  assert(0);
}

// player_t::action_start ===================================================

void player_t::action_start( action_t* action )
{
  gcd_ready = sim -> current_time + action -> gcd();

  if( ! action -> background )
  {
    executing = action -> event;
  }

  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_start_event( (spell_t*) action );
        enchant_t::spell_start_event( (spell_t*) action );
                   spell_start_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_start_event( (attack_t*) action );
        enchant_t::attack_start_event( (attack_t*) action );
                   attack_start_event( (attack_t*) action );
  }
}

// player_t::action_miss ====================================================

void player_t::action_miss( action_t* action )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_miss_event( (spell_t*) action );
        enchant_t::spell_miss_event( (spell_t*) action );
                   spell_miss_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_miss_event( (attack_t*) action );
        enchant_t::attack_miss_event( (attack_t*) action );
                   attack_miss_event( (attack_t*) action );
  }
}

// player_t::action_hit =====================================================

void player_t::action_hit( action_t* action )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_hit_event( (spell_t*) action );
        enchant_t::spell_hit_event( (spell_t*) action );
                   spell_hit_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_hit_event( (attack_t*) action );
        enchant_t::attack_hit_event( (attack_t*) action );
                   attack_hit_event( (attack_t*) action );
  }
}

// player_t::action_tick ====================================================

void player_t::action_tick( action_t* action )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_tick_event( (spell_t*) action );
        enchant_t::spell_tick_event( (spell_t*) action );
                   spell_tick_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_tick_event( (attack_t*) action );
        enchant_t::attack_tick_event( (attack_t*) action );
	           attack_tick_event( (attack_t*) action );
  }
}

// player_t::action_damage ==================================================

void player_t::action_damage( action_t* action, 
			      double    amount,
			      int8_t    dmg_type )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_damage_event( (spell_t*) action, amount, dmg_type );
        enchant_t::spell_damage_event( (spell_t*) action, amount, dmg_type );
                   spell_damage_event( (spell_t*) action, amount, dmg_type );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_damage_event( (attack_t*) action, amount, dmg_type );
        enchant_t::attack_damage_event( (attack_t*) action, amount, dmg_type );
                   attack_damage_event( (attack_t*) action, amount, dmg_type );
  }
}

// player_t::action_heal ====================================================

void player_t::action_heal( action_t* action, 
			    double    amount )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_heal_event( (spell_t*) action, amount );
        enchant_t::spell_heal_event( (spell_t*) action, amount );
                   spell_heal_event( (spell_t*) action, amount );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_heal_event( (attack_t*) action, amount );
        enchant_t::attack_heal_event( (attack_t*) action, amount );
                   attack_heal_event( (attack_t*) action, amount );
  }
}

// player_t::action_finish ==================================================

void player_t::action_finish( action_t* action )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_finish_event( (spell_t*) action );
        enchant_t::spell_finish_event( (spell_t*) action );
                   spell_finish_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_finish_event( (attack_t*) action );
        enchant_t::attack_finish_event( (attack_t*) action );
                   attack_finish_event( (attack_t*) action );
  }
}

// player_t::spell_start_event ==============================================

void player_t::spell_start_event( spell_t* spell )
{
}

// player_t::spell_miss_event ===============================================

void player_t::spell_miss_event( spell_t* spell )
{
}

// player_t::spell_hit_event ================================================

void player_t::spell_hit_event( spell_t* spell )
{
  trigger_judgement_of_wisdom( spell );

  if ( spell -> result == RESULT_CRIT )
  {
    trigger_focus_magic_feedback( spell );
  }
}

// player_t::spell_tick_event ===============================================

void player_t::spell_tick_event( spell_t* spell )
{
}

// player_t::spell_damage_event =============================================

void player_t::spell_damage_event( spell_t* spell, 
				   double   amount,
				   int8_t   dmg_type )
{
}

// player_t::spell_heal_event ===============================================

void player_t::spell_heal_event( spell_t* spell, 
				 double   amount )
{
}

// player_t::spell_finish_event =============================================

void player_t::spell_finish_event( spell_t* spell )
{
}

// player_t::attack_start_event =============================================

void player_t::attack_start_event( attack_t* attack )
{
}

// player_t::attack_miss_event ==============================================

void player_t::attack_miss_event( attack_t* attack )
{
}

// player_t::attack_hit_event ===============================================

void player_t::attack_hit_event( attack_t* attack )
{
  trigger_judgement_of_wisdom( attack );
}

// player_t::attack_tick_event ==============================================

void player_t::attack_tick_event( attack_t* attack )
{
}

// player_t::attack_damage_event ============================================

void player_t::attack_damage_event( attack_t* attack, 
				    double   amount,
				    int8_t   dmg_type )
{
}

// player_t::attack_heal_event ==============================================

void player_t::attack_heal_event( attack_t* attack, 
				  double    amount )
{
}

// player_t::attack_finish_event ============================================

void player_t::attack_finish_event( attack_t* attack )
{
}

// player_t::share_cooldown =================================================

void player_t::share_cooldown( const std::string& group,
			       double             duration )
{
  double ready = sim -> current_time + duration;

  for( action_t* a = action_list; a; a = a -> next )
  {
    if( group == a -> cooldown_group )
    {
      if( ready > a -> cooldown_ready )
      {
	a -> cooldown_ready = ready;
      }
    }
  }
}

// player_t::share_duration =================================================

void player_t::share_duration( const std::string& group,
			       double             ready )
{
  for( action_t* a = action_list; a; a = a -> next )
  {
    if( a -> duration_group == group )
    {
      if( a -> duration_ready < ready )
      {
	a -> duration_ready = ready;
      }
    }
  }
}

// player_t::recent_cast ====================================================

bool player_t::recent_cast()
{
  return ( last_cast > 0 ) && ( ( last_cast + 5.0 ) > sim -> current_time );
}

// player_t::find_action ====================================================

action_t* player_t::find_action( const std::string& str )
{
  for( action_t* a = action_list; a; a = a -> next )
    if( str == a -> name_str )
      return a;

  return 0;
}

// player_t::calculate_spirit_regen =========================================

double player_t::spirit_regen_per_second()
{
  double base_60 = 0.011000;
  double base_70 = 0.009327;
  double base_80 = 0.005575;

  double base_regen = rating_t::interpolate( level, base_60, base_70, base_80 );

  double mana_per_second  = sqrt( intellect() ) * spirit() * base_regen;

  return mana_per_second;
}

// player_t::aura_gain ======================================================

void player_t::aura_gain( const char* aura_name )
{
  // FIXME! Aura-tracking here.

  if( sim -> log ) report_t::log( sim, "Player %s gains %s", name(), aura_name );
}

// player_t::aura_loss ======================================================

void player_t::aura_loss( const char* aura_name )
{
  // FIXME! Aura-tracking here.

  if( sim -> log ) report_t::log( sim, "Player %s loses %s", name(), aura_name );
}

// player_t::get_gain =======================================================

gain_t* player_t::get_gain( const std::string& name )
{
  gain_t* g=0;

  for( g = gain_list; g; g = g -> next )
  {
    if( g -> name_str == name )
      return g;
  }

  g = new gain_t( name );

  gain_t** tail = &gain_list;

  while( *tail && name > ( (*tail) -> name_str ) )
  {
    tail = &( (*tail) -> next );
  }

  g -> next = *tail;
  *tail = g;

  return g;
}

// player_t::get_proc =======================================================

proc_t* player_t::get_proc( const std::string& name )
{
  proc_t* p=0;

  for( p = proc_list; p; p = p -> next )
  {
    if( p -> name_str == name )
      return p;
  }

  p = new proc_t( name );

  proc_t** tail = &proc_list;

  while( *tail && name > ( (*tail) -> name_str ) )
  {
    tail = &( (*tail) -> next );
  }

  p -> next = *tail;
  *tail = p;

  return p;
}

// player_t::get_stats ======================================================

stats_t* player_t::get_stats( const std::string& n )
{
  stats_t* stats=0;

  for( stats = stats_list; stats; stats = stats -> next )
  {
    if( stats -> name_str == n )
      return stats;
  }

  if( ! stats )
  {
    stats = new stats_t( n, this );
    stats -> init();
    stats_t** tail= &stats_list;
    while( *tail && n > ( (*tail) -> name_str ) )
    {
      tail = &( (*tail) -> next );
    }
    stats -> next = *tail;
    *tail = stats;
  }

  return stats;
}

// player_t::get_uptime =====================================================

uptime_t* player_t::get_uptime( const std::string& name )
{
  uptime_t* u=0;

  for( u = uptime_list; u; u = u -> next )
  {
    if( u -> name_str == name )
      return u;
  }

  u = new uptime_t( name );

  uptime_t** tail = &uptime_list;

  while( *tail && name > ( (*tail) -> name_str ) )
  {
    tail = &( (*tail) -> next );
  }

  u -> next = *tail;
  *tail = u;

  return u;
}

// Wait Until Ready Action ===================================================

struct wait_until_ready_t : public action_t
{
  wait_until_ready_t( player_t* player, const std::string& options_str ) : 
    action_t( ACTION_OTHER, "wait", player )
  {
  }

  virtual void execute() 
  {
    trigger_gcd = 0.1;
    
    for( action_t* a = player -> action_list; a; a = a -> next )
    {
      if( a -> background ) continue;
      
      if( a -> cooldown_ready > sim -> current_time )
      {
	double delta_time = a -> cooldown_ready - sim -> current_time;
	if( delta_time > trigger_gcd ) trigger_gcd = delta_time;
      }

      if( a -> duration_ready > sim -> current_time )
      {
	double delta_time = a -> duration_ready - ( sim -> current_time + a -> execute_time() );
	if( delta_time > trigger_gcd ) trigger_gcd = delta_time;
      }
    }

    player -> total_waiting += trigger_gcd;
  }
};

// Restore Mana Action =====================================================

struct restore_mana_t : public action_t
{
  double mana;

  restore_mana_t( player_t* player, const std::string& options_str ) : 
    action_t( ACTION_OTHER, "restore_mana", player ), mana(0)
  {
    option_t options[] =
    {
      { "mana", OPT_FLT, &mana },
      { NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
  }

  virtual void execute() 
  {
    double mana_missing = player -> resource_max[ RESOURCE_MANA ] - player -> resource_current[ RESOURCE_MANA ];
    double mana_gain = mana;

    if( mana_gain == 0 || mana_gain > mana_missing ) mana_gain = mana_missing;

    if( mana_gain > 0 )
    {
      player -> resource_gain( RESOURCE_MANA, mana_gain, player -> gains.restore_mana );
    }
  }
};

// player_t::create_action ==================================================

action_t* player_t::create_action( const std::string& name,
				   const std::string& options_str )
{
  if( name == "restore_mana" ) return new     restore_mana_t( this, options_str );
  if( name == "wait"         ) return new wait_until_ready_t( this, options_str );

  return 0;
}

// player_t::get_talent_trees ===============================================

bool player_t::get_talent_trees( std::vector<int8_t*>& tree1,
				 std::vector<int8_t*>& tree2,
				 std::vector<int8_t*>& tree3,
				 talent_translation_t translation[][3] )
{
  for( int i=0; translation[ i ][ 0 ].index > 0; i++ ) tree1.push_back( translation[ i ][ 0 ].address );
  for( int i=0; translation[ i ][ 1 ].index > 0; i++ ) tree2.push_back( translation[ i ][ 1 ].address );
  for( int i=0; translation[ i ][ 2 ].index > 0; i++ ) tree3.push_back( translation[ i ][ 2 ].address );

  return true;
}

// player_t::parse_talents ==================================================

bool player_t::parse_talents( std::vector<int8_t*>& talent_tree,
			      const std::string&  talent_string )
{
  const char* s = talent_string.c_str();

  unsigned int size = std::min( talent_tree.size(), talent_string.size() );

  for( unsigned int i=0; i < size; i++ )
  {
    int8_t* address = talent_tree[ i ];
    if( ! address ) continue;
    *address = s[ i ] - '0';
  }
  
  return true;
}

// player_t::parse_talents ==================================================

bool player_t::parse_talents( const std::string& talent_string )
{
  std::vector<int8_t*> talent_tree1, talent_tree2, talent_tree3;

  if( ! get_talent_trees( talent_tree1, talent_tree2, talent_tree3 ) )
    return false;

  int size1 = talent_tree1.size();
  int size2 = talent_tree2.size();

  std::string talent_string1( talent_string,     0,  size1 );
  std::string talent_string2( talent_string, size1,  size2 );
  std::string talent_string3( talent_string, size1 + size2 );
  
  parse_talents( talent_tree1, talent_string1 );
  parse_talents( talent_tree2, talent_string2 );
  parse_talents( talent_tree3, talent_string3 );

  return true;
}

// player_t::parse_talents_mmo ==============================================

bool player_t::parse_talents_mmo( const std::string& talent_string )
{
  return parse_talents( talent_string );
}

// player_t::parse_talents_wowhead ==========================================

bool player_t::parse_talents_wowhead( const std::string& talent_string )
{
  // wowhead format: [tree_1]Z[tree_2]Z[tree_3] where the trees are character encodings
  // each character expands to a pair of numbers [0-5][0-5]
  // unused deeper talents are simply left blank instead of filling up the string with zero-zero encodings

  struct decode_t
  {
    char key, first, second;
  } decoding[] = {
    { '0', '0', '0' },  { 'z', '0', '1' },  { 'M', '0', '2' },  { 'c', '0', '3' }, { 'm', '0', '4' },  { 'V', '0', '5' },
    { 'o', '1', '0' },  { 'k', '1', '1' },  { 'R', '1', '2' },  { 's', '1', '3' }, { 'a', '1', '4' },  { 'q', '1', '5' },
    { 'b', '2', '0' },  { 'd', '2', '1' },  { 'r', '2', '2' },  { 'f', '2', '3' }, { 'w', '2', '4' },  { 'i', '2', '5' },
    { 'h', '3', '0' },  { 'u', '3', '1' },  { 'G', '3', '2' },  { 'I', '3', '3' }, { 'N', '3', '4' },  { 'A', '3', '5' },
    { 'L', '4', '0' },  { 'p', '4', '1' },  { 'T', '4', '2' },  { 'j', '4', '3' }, { 'n', '4', '4' },  { 'y', '4', '5' },
    { 'x', '5', '0' },  { 't', '5', '1' },  { 'g', '5', '2' },  { 'e', '5', '3' }, { 'v', '5', '4' },  { 'E', '5', '5' },
    { '\0', '\0', '\0' }
  };

  std::vector<int8_t*> talent_trees[ 3 ];
  unsigned int tree_size[ 3 ];

  if( ! get_talent_trees( talent_trees[ 0 ], talent_trees[ 1 ] , talent_trees[ 2 ] ) )
    return false;

  for( int i=0; i < 3; i++ ) 
  {
    tree_size[ i ] = talent_trees[ i ].size();
  }

  std::string talent_strings[ 3 ];
  int tree=0;

  for( unsigned int i=1; i < talent_string.length(); i++ )
  {
    if( tree > 2 )
    {
      fprintf( sim -> output_file, "Malformed wowhead talent string. Too many trees specified.\n" );
      assert(0);
    }

    char c = talent_string[ i ];

    if( c == 'Z' )
    {
      tree++;
      continue;
    }

    decode_t* decode = 0;
    for( int j=0; decoding[ j ].key != '\0' && ! decode; j++ )
    {
      if( decoding[ j ].key == c ) decode = decoding + j;
    }

    if( ! decode )
    {
      fprintf( sim -> output_file, "Malformed wowhead talent string. Translation for '%c' unknown.\n", c );
      assert(0);
    }

    talent_strings[ tree ] += decode -> first;
    talent_strings[ tree ] += decode -> second;

    if( talent_strings[ tree ].size() >= tree_size[ tree ] ) 
      tree++;
  }

  if( sim -> debug )
  {
    fprintf( sim -> output_file, "%s tree1: %s\n", name(), talent_strings[ 0 ].c_str() );
    fprintf( sim -> output_file, "%s tree2: %s\n", name(), talent_strings[ 1 ].c_str() );
    fprintf( sim -> output_file, "%s tree3: %s\n", name(), talent_strings[ 2 ].c_str() );
  }

  for( int i=0; i < 3; i++ ) 
  {
    parse_talents( talent_trees[ i ], talent_strings[ i ] );
  }

  return true;
}

// player_t::parse_talents ==================================================

bool player_t::parse_talents( const std::string& talent_string,
			      int                encoding )
{
  if( encoding == ENCODING_BLIZZARD ) return parse_talents        ( talent_string );
  if( encoding == ENCODING_MMO      ) return parse_talents_mmo    ( talent_string );
  if( encoding == ENCODING_WOWHEAD  ) return parse_talents_wowhead( talent_string );

  return false;
}

// player_t::parse_option ===================================================

bool player_t::parse_option( const std::string& name,
			     const std::string& value )
{
  option_t options[] =
  {
    // Player - General
    { "level",                                OPT_INT8,   &( level                                          ) },
    { "party",                                OPT_INT8,   &( party                                          ) },
    { "gcd",                                  OPT_FLT,    &( base_gcd                                       ) },
    { "sleeping",                             OPT_INT8,   &( sleeping                                       ) },
    // Player - Haste										          
    { "haste_rating",                         OPT_INT16,  &( initial_haste_rating                           ) },
    // Player - Attributes									          
    { "strength",                             OPT_FLT,    &( attribute_initial[ ATTR_STRENGTH  ]            ) },
    { "agility",                              OPT_FLT,    &( attribute_initial[ ATTR_AGILITY   ]            ) },
    { "stamina",                              OPT_FLT,    &( attribute_initial[ ATTR_STAMINA   ]            ) },
    { "intellect",                            OPT_FLT,    &( attribute_initial[ ATTR_INTELLECT ]            ) },
    { "spirit",                               OPT_FLT,    &( attribute_initial[ ATTR_SPIRIT    ]            ) },
    { "base_strength",                        OPT_FLT,    &( attribute_base   [ ATTR_STRENGTH  ]            ) },
    { "base_agility",                         OPT_FLT,    &( attribute_base   [ ATTR_AGILITY   ]            ) },
    { "base_stamina",                         OPT_FLT,    &( attribute_base   [ ATTR_STAMINA   ]            ) },
    { "base_intellect",                       OPT_FLT,    &( attribute_base   [ ATTR_INTELLECT ]            ) },
    { "base_spirit",                          OPT_FLT,    &( attribute_base   [ ATTR_SPIRIT    ]            ) },
    { "strength_multiplier",                  OPT_FLT,    &( attribute_multiplier_initial[ ATTR_STRENGTH  ] ) },
    { "agility_multiplier",                   OPT_FLT,    &( attribute_multiplier_initial[ ATTR_AGILITY   ] ) },
    { "stamina_multiplier",                   OPT_FLT,    &( attribute_multiplier_initial[ ATTR_STAMINA   ] ) },
    { "intellect_multiplier",                 OPT_FLT,    &( attribute_multiplier_initial[ ATTR_INTELLECT ] ) },
    { "spirit_multiplier",                    OPT_FLT,    &( attribute_multiplier_initial[ ATTR_SPIRIT    ] ) },
    // Player - Spell Mechanics
    { "spell_power",                          OPT_FLT,    &( initial_spell_power[ SCHOOL_MAX    ]           ) },
    { "spell_power_arcane",                   OPT_FLT,    &( initial_spell_power[ SCHOOL_ARCANE ]           ) },
    { "spell_power_fire",                     OPT_FLT,    &( initial_spell_power[ SCHOOL_FIRE   ]           ) },
    { "spell_power_frost",                    OPT_FLT,    &( initial_spell_power[ SCHOOL_FROST  ]           ) },
    { "spell_power_holy",                     OPT_FLT,    &( initial_spell_power[ SCHOOL_HOLY   ]           ) },
    { "spell_power_nature",                   OPT_FLT,    &( initial_spell_power[ SCHOOL_NATURE ]           ) },
    { "spell_power_shadow",                   OPT_FLT,    &( initial_spell_power[ SCHOOL_SHADOW ]           ) },
    { "spell_hit",                            OPT_FLT,    &( initial_spell_hit                              ) },
    { "spell_crit",                           OPT_FLT,    &( initial_spell_crit                             ) },
    { "spell_penetration",                    OPT_FLT,    &( initial_spell_penetration                      ) },
    { "mp5",                                  OPT_FLT,    &( initial_mp5                                    ) },
    { "base_spell_power",                     OPT_FLT,    &( base_spell_power                               ) },
    { "base_spell_hit",                       OPT_FLT,    &( base_spell_hit                                 ) },
    { "base_spell_crit",                      OPT_FLT,    &( base_spell_crit                                ) },
    { "base_spell_penetration",               OPT_FLT,    &( base_spell_penetration                         ) },
    { "base_mp5",                             OPT_FLT,    &( base_mp5                                       ) },
    { "spell_power_per_intellect",            OPT_FLT,    &( spell_power_per_intellect                      ) },
    { "spell_power_per_spirit",               OPT_FLT,    &( spell_power_per_spirit                         ) },
    { "spell_crit_per_intellect",             OPT_FLT,    &( spell_crit_per_intellect                       ) },
    // Player - Attack Mechanics							       
    { "attack_power",                         OPT_FLT,    &( initial_attack_power                           ) },
    { "attack_hit",                           OPT_FLT,    &( initial_attack_hit                             ) },
    { "attack_expertise",                     OPT_FLT,    &( initial_attack_expertise                       ) },
    { "attack_crit",                          OPT_FLT,    &( initial_attack_crit                            ) },
    { "attack_penetration",                   OPT_FLT,    &( initial_attack_penetration                     ) },
    { "base_attack_power",                    OPT_FLT,    &( base_attack_power                              ) },
    { "base_attack_hit",                      OPT_FLT,    &( base_attack_hit                                ) },
    { "base_attack_expertise",                OPT_FLT,    &( base_attack_expertise                          ) },
    { "base_attack_crit",                     OPT_FLT,    &( base_attack_crit                               ) },
    { "base_attack_penetration",              OPT_FLT,    &( base_attack_penetration                        ) },
    { "attack_power_per_strength",            OPT_FLT,    &( attack_power_per_strength                      ) },
    { "attack_power_per_agility",             OPT_FLT,    &( attack_power_per_agility                       ) },
    { "attack_crit_per_agility",              OPT_FLT,    &( attack_crit_per_agility                        ) },
    // Player - Weapons
    { "main_hand",                            OPT_STRING, &( main_hand_str                                  ) },
    { "off_hand",                             OPT_STRING, &( off_hand_str                                   ) },
    { "ranged",                               OPT_STRING, &( ranged_str                                     ) },
    // Player - Resources
    { "health",                               OPT_FLT,    &( resource_initial[ RESOURCE_HEALTH ]            ) },
    { "mana",                                 OPT_FLT,    &( resource_initial[ RESOURCE_MANA   ]            ) },
    { "rage",                                 OPT_FLT,    &( resource_initial[ RESOURCE_RAGE   ]            ) },
    { "energy",                               OPT_FLT,    &( resource_initial[ RESOURCE_ENERGY ]            ) },
    { "focus",                                OPT_FLT,    &( resource_initial[ RESOURCE_FOCUS  ]            ) },
    { "runic",                                OPT_FLT,    &( resource_initial[ RESOURCE_RUNIC  ]            ) },
    { "base_health",                          OPT_FLT,    &( resource_base   [ RESOURCE_HEALTH ]            ) },
    { "base_mana",                            OPT_FLT,    &( resource_base   [ RESOURCE_MANA   ]            ) },
    { "base_rage",                            OPT_FLT,    &( resource_base   [ RESOURCE_RAGE   ]            ) },
    { "base_energy",                          OPT_FLT,    &( resource_base   [ RESOURCE_ENERGY ]            ) },
    { "base_focus",                           OPT_FLT,    &( resource_base   [ RESOURCE_FOCUS  ]            ) },
    { "base_runic",                           OPT_FLT,    &( resource_base   [ RESOURCE_RUNIC  ]            ) },
    // Player - Action Priority List								        
    { "pre_actions",                          OPT_STRING, &( action_list_prefix                             ) },
    { "actions",                              OPT_STRING, &( action_list_str                                ) },
    { "actions+",                             OPT_APPEND, &( action_list_str                                ) },
    { "post_actions",                         OPT_STRING, &( action_list_postfix                            ) },
    { "skip_actions",                         OPT_STRING, &( action_list_skip                               ) },
    // Player - Reporting
    { "quiet",                                OPT_INT8,   &( quiet                                          ) },
    // Player - Gear - Attributes								            
    { "gear_strength",                        OPT_INT16,  &( gear.attribute        [ ATTR_STRENGTH  ]       ) },
    { "gear_agility",                         OPT_INT16,  &( gear.attribute        [ ATTR_AGILITY   ]       ) },
    { "gear_stamina",                         OPT_INT16,  &( gear.attribute        [ ATTR_STAMINA   ]       ) },
    { "gear_intellect",                       OPT_INT16,  &( gear.attribute        [ ATTR_INTELLECT ]       ) },
    { "gear_spirit",                          OPT_INT16,  &( gear.attribute        [ ATTR_SPIRIT    ]       ) },
    { "enchant_strength",                     OPT_INT16,  &( gear.attribute_enchant[ ATTR_STRENGTH  ]       ) },
    { "enchant_agility",                      OPT_INT16,  &( gear.attribute_enchant[ ATTR_AGILITY   ]       ) },
    { "enchant_stamina",                      OPT_INT16,  &( gear.attribute_enchant[ ATTR_STAMINA   ]       ) },
    { "enchant_intellect",                    OPT_INT16,  &( gear.attribute_enchant[ ATTR_INTELLECT ]       ) },
    { "enchant_spirit",                       OPT_INT16,  &( gear.attribute_enchant[ ATTR_SPIRIT    ]       ) },
    // Player - Gear - Spell									            
    { "gear_spell_power",                     OPT_INT16,  &( gear.spell_power[ SCHOOL_MAX    ]              ) },
    { "gear_spell_power_arcane",              OPT_INT16,  &( gear.spell_power[ SCHOOL_ARCANE ]              ) },
    { "gear_spell_power_fire",                OPT_INT16,  &( gear.spell_power[ SCHOOL_FIRE   ]              ) },
    { "gear_spell_power_frost",               OPT_INT16,  &( gear.spell_power[ SCHOOL_FROST  ]              ) },
    { "gear_spell_power_holy",                OPT_INT16,  &( gear.spell_power[ SCHOOL_HOLY   ]              ) },
    { "gear_spell_power_nature",              OPT_INT16,  &( gear.spell_power[ SCHOOL_NATURE ]              ) },
    { "gear_spell_power_shadow",              OPT_INT16,  &( gear.spell_power[ SCHOOL_SHADOW ]              ) },
    { "gear_spell_penetration",               OPT_INT16,  &( gear.spell_penetration                         ) },
    { "gear_mp5",                             OPT_INT16,  &( gear.mp5                                       ) },
    { "enchant_spell_power",                  OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_MAX    ]      ) },
    { "enchant_spell_power_arcane",           OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_ARCANE ]      ) },
    { "enchant_spell_power_fire",             OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_FIRE   ]      ) },
    { "enchant_spell_power_frost",            OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_FROST  ]      ) },
    { "enchant_spell_power_holy",             OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_HOLY   ]      ) },
    { "enchant_spell_power_nature",           OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_NATURE ]      ) },
    { "enchant_spell_power_shadow",           OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_SHADOW ]      ) },
    { "enchant_spell_penetration",            OPT_INT16,  &( gear.spell_penetration_enchant                 ) },
    { "enchant_mp5",                          OPT_INT16,  &( gear.mp5_enchant                               ) },
    // Player - Gear - Attack									            
    { "gear_attack_power",                    OPT_INT16,  &( gear.attack_power                              ) },
    { "gear_expertise_rating",                OPT_INT16,  &( gear.expertise_rating                          ) },
    { "gear_armor_penetration_rating",        OPT_INT16,  &( gear.armor_penetration_rating                  ) },
    { "enchant_attack_power",                 OPT_INT16,  &( gear.attack_power_enchant                      ) },
    { "enchant_attack_expertise_rating",      OPT_INT16,  &( gear.expertise_rating_enchant                  ) },
    { "enchant_attack_penetration",           OPT_INT16,  &( gear.armor_penetration_rating_enchant          ) },
    // Player - Gear - Common									            
    { "gear_haste_rating",                    OPT_INT16,  &( gear.haste_rating                              ) },
    { "gear_hit_rating",                      OPT_INT16,  &( gear.hit_rating                                ) },
    { "gear_crit_rating",                     OPT_INT16,  &( gear.crit_rating                               ) },
    { "enchant_haste_rating",                 OPT_INT16,  &( gear.haste_rating_enchant                      ) },
    { "enchant_hit_rating",                   OPT_INT16,  &( gear.hit_rating_enchant                        ) },
    { "enchant_crit_rating",                  OPT_INT16,  &( gear.crit_rating_enchant                       ) },
    // Player - Gear - Resource									            
    { "gear_health",                          OPT_INT16,  &( gear.resource        [ RESOURCE_HEALTH ]       ) },
    { "gear_mana",                            OPT_INT16,  &( gear.resource        [ RESOURCE_MANA   ]       ) },
    { "gear_rage",                            OPT_INT16,  &( gear.resource        [ RESOURCE_RAGE   ]       ) },
    { "gear_energy",                          OPT_INT16,  &( gear.resource        [ RESOURCE_ENERGY ]       ) },
    { "gear_focus",                           OPT_INT16,  &( gear.resource        [ RESOURCE_FOCUS  ]       ) },
    { "gear_runic",                           OPT_INT16,  &( gear.resource        [ RESOURCE_RUNIC  ]       ) },
    { "enchant_health",                       OPT_INT16,  &( gear.resource_enchant[ RESOURCE_HEALTH ]       ) },
    { "enchant_mana",                         OPT_INT16,  &( gear.resource_enchant[ RESOURCE_MANA   ]       ) },
    { "enchant_rage",                         OPT_INT16,  &( gear.resource_enchant[ RESOURCE_RAGE   ]       ) },
    { "enchant_energy",                       OPT_INT16,  &( gear.resource_enchant[ RESOURCE_ENERGY ]       ) },
    { "enchant_focus",                        OPT_INT16,  &( gear.resource_enchant[ RESOURCE_FOCUS  ]       ) },
    { "enchant_runic",                        OPT_INT16,  &( gear.resource_enchant[ RESOURCE_RUNIC  ]       ) },
    // Player - Gear - Budgeting								            
    { "spell_power_budget",                   OPT_INT16,  &( gear.spell_power_budget                        ) },
    { "attack_power_budget",                  OPT_INT16,  &( gear.attack_power_budget                       ) },
    { "budget_slots",                         OPT_INT16,  &( gear.budget_slots                              ) },
    // Player - Tier Bonuses
    { "tier4_2pc",                            OPT_INT8,   &( gear.tier4_2pc                                 ) },
    { "tier4_4pc",                            OPT_INT8,   &( gear.tier4_4pc                                 ) },
    { "tier5_2pc",                            OPT_INT8,   &( gear.tier5_2pc                                 ) },
    { "tier5_4pc",                            OPT_INT8,   &( gear.tier5_4pc                                 ) },
    { "tier6_2pc",                            OPT_INT8,   &( gear.tier6_2pc                                 ) },
    { "tier6_4pc",                            OPT_INT8,   &( gear.tier6_4pc                                 ) },
    { "tier7_2pc",                            OPT_INT8,   &( gear.tier7_2pc                                 ) },
    { "tier7_4pc",                            OPT_INT8,   &( gear.tier7_4pc                                 ) },
    // Player - Consumables									            
    { "flask",                                OPT_STRING, &( flask_str                                      ) },
    { "elixirs",                              OPT_STRING, &( elixirs_str                                    ) },
    { "food",                                 OPT_STRING, &( food_str                                       ) },
    // Player - Buffs - FIXME! These will go away eventually, and be converted into player actions
    { "battle_shout",                         OPT_INT16,  &( buffs.battle_shout                             ) },
    { "blessing_of_kings",                    OPT_INT8,   &( buffs.blessing_of_kings                        ) },
    { "blessing_of_might",                    OPT_INT16,  &( buffs.blessing_of_might                        ) },
    { "blessing_of_wisdom",                   OPT_INT16,  &( buffs.blessing_of_wisdom                       ) },
    { "leader_of_the_pack",                   OPT_INT8,   &( buffs.leader_of_the_pack                       ) },
    { "sanctified_retribution",               OPT_INT8,   &( buffs.sanctified_retribution                   ) },
    { "swift_retribution",                    OPT_INT8,   &( buffs.swift_retribution                        ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    unique_gear_t::parse_option( this, name, value );
    option_t::print( sim, options );
    return false;
  }

  if( unique_gear_t::parse_option( this, name, value ) ) return true;
  
  return option_t::parse( sim, options, name, value );
}
