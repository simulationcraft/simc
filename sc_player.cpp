// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_moonkin_haste ====================================================

static void trigger_moonkin_haste( spell_t* s )
{
  struct haste_cooldown_t : public event_t
  {
    haste_cooldown_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Moonkin Haste Cooldown";
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( player -> party == p -> party )
        {
	  p -> buffs.improved_moonkin_aura = 0;
	}
      }
      time = 3.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( player -> party == p -> party )
        {
	  p -> buffs.improved_moonkin_aura = 1;
	}
      }
    }
  };

  struct haste_expiration_t : public event_t
  {
    haste_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Moonkin Haste Expiration";
      player -> aura_gain( "Moonkin Haste" );
      player -> buffs.moonkin_haste = 1;
      time = 10.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      player -> aura_loss( "Moonkin Haste" );
      player -> buffs.moonkin_haste = 0;
      player -> expirations.moonkin_haste = 0;
    }
  };

  player_t* p = s -> player;

  if( p -> buffs.improved_moonkin_aura )
  {
    p -> proc( "moonkin_haste" );

    event_t*& e = p -> expirations.moonkin_haste;

    if( e )
    {
      e -> reschedule( s -> sim -> current_time + 10.0 );
    }
    else
    {
      e = new haste_expiration_t( s -> sim, p );
    }

    // Remove the Improved Moonkin aura for 3 seconds to block trigger during cooldown.
    new haste_cooldown_t( s -> sim, p );
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Player - Gear
// ==========================================================================

// player_t::gear_t::allocate_spell_power_budget( sim_t* sim ) ==============

void player_t::gear_t::allocate_spell_power_budget( sim_t* sim )
{
  report_t::debug( sim, "Allocating spell_power budget...." );   

  if(  spell_hit_rating  == 0 &&
       spell_crit_rating == 0 &&
       haste_rating      == 0 )
  {
    spell_power[ SCHOOL_MAX ] = spell_power_budget;
    return;
  }

  if( budget_slots == 0 ) budget_slots = 16;

  double spell_power_statmod = 0.855;

  double slot_power = spell_power_budget / (double) budget_slots;
  double slot_hit   = spell_hit_rating   / (double) budget_slots;
  double slot_crit  = spell_crit_rating  / (double) budget_slots;
  double slot_haste = haste_rating       / (double) budget_slots;

  double target = pow( slot_power * spell_power_statmod, 1.5 );

  double cost = pow( slot_hit,   1.5 ) +
                pow( slot_crit,  1.5 ) +
                pow( slot_haste, 1.5 );

  spell_power[ SCHOOL_MAX ] = (int16_t) ( budget_slots * pow( target - cost, 1.0 / 1.5 ) / spell_power_statmod );
  
  report_t::debug( sim, "slot_power=%.1f  slot_hit=%.1f  slot_crit=%.1f  slot_haste=%.1f  target=%.1f  cost=%.1f  spell_power=%d\n",
                   slot_power, slot_hit, slot_crit, slot_haste, target, cost, spell_power[ SCHOOL_MAX ] );
}

// player_t::gear_t::allocate_attack_power_budget( sim_t* sim ) =============

void player_t::gear_t::allocate_attack_power_budget( sim_t* sim )
{
  report_t::debug( sim, "Allocating attack_power budget...." );   

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
  spell_power_multiplier(1.0),
  spell_power_per_intellect(0),
  spell_power_per_spirit(0),
  spell_crit_per_intellect(0),
  last_cast(0),
  // Attack Mechanics
  base_attack_power(0),       initial_attack_power(0),        attack_power(0),
  base_attack_hit(0),         initial_attack_hit(0),          attack_hit(0),
  base_attack_expertise(0),   initial_attack_expertise(0),    attack_expertise(0),
  base_attack_crit(0),        initial_attack_crit(0),         attack_crit(0),
  base_attack_penetration(0), initial_attack_penetration(0),  attack_penetration(0),
  attack_power_multiplier(1.0),
  attack_power_per_strength(0),
  attack_power_per_agility(0),
  attack_crit_per_agility(0),
  main_hand_weapon(0), off_hand_weapon(0), ranged_weapon(0),
  position( POSITION_FRONT ),
  // Resources 
  resource_constrained(0),           resource_constrained_count(0),
  resource_constrained_total_dmg(0), resource_constrained_total_time(0),
  mana_per_intellect(0), health_per_stamina(0),
  // Events
  executing(0), channeling(0),
  // Actions
  action_list(0),
  // Reporting
  quiet(0), report(0), iteration_dmg(0), total_dmg(0), stats_list(0)
{
  report_t::debug( sim, "Creating Player %s", name() );
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

  main_hand_weapon = new weapon_t();
  off_hand_weapon  = new weapon_t();
  ranged_weapon    = new weapon_t();
}

// player_t::init ==========================================================

void player_t::init() 
{
  report_t::debug( sim, "Initializing player %s", name() );   

  init_rating();
  init_base();
  init_core();
  init_spell();
  init_attack();
  init_resources();
  init_actions();
  init_stats();
}
 
// player_t::init_core ======================================================

void player_t::init_core() 
{
  if( initial_haste_rating == 0 ) initial_haste_rating = base_haste_rating + gear.haste_rating + gear.haste_rating_enchant;

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    if( attribute_initial[ i ] == 0 )
    {
      attribute_initial[ i ] = attribute_base[ i ] + gear.attribute[ i ] + gear.attribute_enchant[ i ];
    }
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
    initial_spell_hit = base_spell_hit;
    initial_spell_hit += ( gear.spell_hit_rating  + 
			   gear.spell_hit_rating_enchant  ) / rating.spell_hit;
  }
  if( initial_spell_crit == 0 )
  {
    initial_spell_crit = base_spell_crit;
    initial_spell_crit += ( gear.spell_crit_rating + 
			    gear.spell_crit_rating_enchant ) / rating.spell_crit;
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
    // FIXME!  Make this an "action" that affects whole raid.
    if( buffs.blessing_of_wisdom ) initial_mp5 += 49;
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
    initial_attack_power = base_attack_power += gear.attack_power + gear.attack_power_enchant;
  }
  if( initial_attack_hit == 0 )
  {
    initial_attack_hit = base_attack_hit;
    initial_attack_hit += ( gear.attack_hit_rating + gear.attack_hit_rating_enchant ) / rating.attack_hit;
  }
  if( initial_attack_expertise == 0 )
  {
    initial_attack_expertise = base_attack_expertise;
    initial_attack_expertise += ( gear.attack_expertise_rating + gear.attack_expertise_rating_enchant ) / rating.attack_expertise;
  }
  if( initial_attack_crit == 0 )
  {
    initial_attack_crit = base_attack_crit;
    initial_attack_crit += ( gear.attack_crit_rating + gear.attack_crit_rating_enchant ) / rating.attack_crit;
  }
  if( initial_attack_penetration == 0 )
  {
    initial_attack_penetration = base_attack_penetration + gear.attack_penetration + gear.attack_penetration_enchant;
  }
}

// player_t::init_resources ================================================

void player_t::init_resources() 
{
  double resource_bonus[ RESOURCE_MAX ];
  for( int i=0; i < RESOURCE_MAX; i++ ) resource_bonus[ i ] = 0;
  resource_bonus[ RESOURCE_MANA   ] = intellect() * mana_per_intellect;
  resource_bonus[ RESOURCE_HEALTH ] = stamina() * health_per_stamina;
  
  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    if( resource_initial[ i ] == 0 )
    {
      resource_initial[ i ] = resource_base[ i ] + resource_bonus[ i ] + gear.resource[ i ] + gear.resource_enchant[ i ];
    }
    resource_current[ i ] = resource_initial[ i ];
  }

  resource_constrained = 0;
  resource_constrained_count = 0;
  resource_constrained_total_dmg = 0;
  resource_constrained_total_time = 0;
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
      report_t::debug( sim, "Player %s: action_list_prefix=%s", name(), action_list_prefix.c_str() );   
      size = wow_string_split( splits, action_list_prefix, "/" );
    }
    if( ! action_list_str.empty() )
    {
      report_t::debug( sim, "Player %s: action_list_str=%s", name(), action_list_str.c_str() );   
      size = wow_string_split( splits, action_list_str, "/" );
    }
    if( ! action_list_postfix.empty() )
    {
      report_t::debug( sim, "Player %s: action_list_postfix=%s", name(), action_list_postfix.c_str() );   
      size = wow_string_split( splits, action_list_postfix, "/" );
    }

    for( int i=0; i < size; i++ )
    {
      std::string action_name    = splits[ i ];
      std::string action_options = "";

      std::string::size_type cut_pt = action_name.find_first_of( "-" );       

      if( cut_pt != action_name.npos )
      {
	action_options = action_name.substr( cut_pt + 1 );
	action_name    = action_name.substr( 0, cut_pt );
      }

      if( ! action_t::create_action( this, action_name, action_options ) )
      {
	printf( "wow_player: Unknown action: %s\n", splits[ i ].c_str() );
	assert( false );
      }
    }
  }

  if( ! action_list_skip.empty() )
  {
    report_t::debug( sim, "Player %s: action_list_skip=%s", name(), action_list_skip.c_str() );   
    std::vector<std::string> splits;
    int size = wow_string_split( splits, action_list_skip, "/" );
    for( int i=0; i < size; i++ )
    {
      action_t* action = find_action( splits[ i ] );
      if( action ) action -> valid = false;
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

  // FIXME! Init the stats structure here.  Makes for cleaner "harmful/channeled" setting.

  for( stats_t* s = stats_list; s; s = s -> next )
  {
    s -> init();
  }
}

// player_t::reset =========================================================

void player_t::reset() 
{
  report_t::debug( sim, "Reseting player %s", name() );   

  gcd_ready = 0;
  sleeping = 0;

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
  
  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_current[ i ] = resource_max[ i ] = resource_initial[ i ];
  }
  resource_constrained = 0;

  executing = 0;
  channeling = 0;
  iteration_dmg = 0;
 
  buffs.reset();
  expirations.reset();
  
  if( action_list )
  {
    for( action_t* a = action_list; a; a = a -> next ) 
    {
      a -> reset();
    }
    new player_ready_event_t( sim, this, 0 );
  }
}

// player_t::schedule_ready =================================================

void player_t::schedule_ready( double delta_time )
{
  sleeping = 0;
  executing = 0;
  channeling = 0;

  double gcd_adjust = gcd_ready - ( sim -> current_time + delta_time );
  if( gcd_adjust > 0 ) delta_time += gcd_adjust;

  double lag = sim -> lag;
  if( lag > 0 ) lag += ( ( rand() % 11 ) - 5 ) * 0.01;
  if( lag < 0 ) lag = 0;
  delta_time += lag;
  
  new player_ready_event_t( sim, this, delta_time );

  stats_t::adjust_for_gcd_and_lag( delta_time );
}

// player_t::execute_action =================================================

action_t* player_t::execute_action()
{
  action_t* action=0;

  for( action = action_list; action; action = action -> next )
  {
    if( action -> valid && 
	action -> ready() )
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

  return action;
}

// player_t::resource_loss =================================================

void player_t::resource_loss( int8_t      resource,
			      double      amount,
			      const char* source )
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

  report_t::debug( sim, "Player %s spends %.0f %s", name(), amount, wow_resource_type_string( resource ) );
}

// player_t::resource_gain =================================================

void player_t::resource_gain( int8_t       resource,
			      double       amount,
			      const  char* source )
{
  amount = std::min( amount, resource_max[ resource ] - resource_current[ resource ] );

  if( amount > 0 )
  {
    resource_current[ resource ] += amount;
    resource_gained [ resource ] += amount;

    if( source && sim -> report -> report_gains ) gain( source, amount );

    report_t::log( sim, "%s gains %.0f %s from %s", 
		   name(), amount, wow_resource_type_string( resource ), source ? source : "unknown" );
  }
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
      if( a -> valid && a -> harmful )
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

    report_t::debug( sim, "Player %s is resource constrained.", name() );
  }
}

// player_t::summon_pet =====================================================

void player_t::summon_pet( const std::string& pet_name )
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

void player_t::dismiss_pet( const std::string& pet_name )
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
  if( action -> trigger_gcd ) 
  {
    gcd_ready =  sim -> current_time + gcd();
  }

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
  if( spell -> result == RESULT_CRIT )
  {
    trigger_moonkin_haste( spell );
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

// player_t::gcd ============================================================

double player_t::gcd()
{
  double t = base_gcd;
  t *= haste;
  if( buffs.bloodlust     ) t *= 0.70;
  if( buffs.moonkin_haste ) t *= 0.80;
  if( t < 1.0 ) t = 1.0;
  return t;
}

// player_t::recent_cast ====================================================

bool player_t::recent_cast()
{
  return last_cast > 0 && last_cast + 5.0 > sim -> current_time;
}

// player_t::gain ===========================================================

void player_t::gain( const std::string& name,
		     double             amount )
{
  if( gain_list.find( name ) == gain_list.end() )
  {
    gain_list[ name ] = amount;
  }
  else gain_list[ name ] += amount;
}

// player_t::proc ===========================================================

void player_t::proc( const std::string& name )
{
  if( proc_list.find( name ) == proc_list.end() )
  {
    proc_list[ name ] = 1;
  }
  else proc_list[ name ]++;
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
  double base_60 = 0.010999;
  double base_70 = 0.009327;
  double base_80 = 0.007655;

  double base_regen = 0;

  if( level <= 60 )
  {
    base_regen = base_60;
  }
  else if( level == 70 )
  {
    base_regen = base_70;
  }
  else if( level >= 80 )
  {
    base_regen = base_80;
  }
  else if( level < 70 )
  {
    base_regen = base_70 + ( base_60 - base_70 ) * ( level - 60 ) / 10;
  }
  else if( level > 70 )
  {
    base_regen = base_80 + ( base_70 - base_80 ) * ( level - 70 ) / 10;
  }
  assert( base_regen != 0 );

  double mana_per_second  = sqrt( intellect() ) * spirit() * base_regen;

  return mana_per_second;
}

// player_t::aura_gain ======================================================

void player_t::aura_gain( const char* aura_name )
{
  // FIXME! Aura-tracking here.

  report_t::log( sim, "Player %s gains %s", name(), aura_name );
}

// player_t::aura_loss ======================================================

void player_t::aura_loss( const char* aura_name )
{
  // FIXME! Aura-tracking here.

  report_t::log( sim, "Player %s loses %s", name(), aura_name );
}

// player_t::parse_talents ==================================================

void player_t::parse_talents( talent_translation_t* translation,
			      const std::string&    talent_string )
{
  for( int i=0; translation[ i ].index != 0; i++ )
  {
    talent_translation_t& t = translation[ i ];

    *( t.address ) = talent_string[ t.index - 1 ] - '0';
  }
}

// player_t::parse_option ======================================================

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
    { "spell_power_multiplier",               OPT_FLT,    &( spell_power_multiplier                         ) },
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
    { "attack_power_multiplier",              OPT_FLT,    &( attack_power_multiplier                        ) },
    { "attack_power_per_strength",            OPT_FLT,    &( attack_power_per_strength                      ) },
    { "attack_power_per_agility",             OPT_FLT,    &( attack_power_per_agility                       ) },
    { "attack_crit_per_agility",              OPT_FLT,    &( attack_crit_per_agility                        ) },
    // Player - Weapons
      // main_hand
      // off_hand
      // ranged
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
    { "post_actions",                         OPT_STRING, &( action_list_postfix                            ) },
    { "skip_actions",                         OPT_STRING, &( action_list_skip                               ) },
    // Player - Reporting
    { "quiet",                                OPT_INT8,   &( quiet                                          ) },
    // Player - Gear - Haste									            
    { "gear_haste",                           OPT_INT16,  &( gear.haste_rating                              ) },
    { "enchant_haste",                        OPT_INT16,  &( gear.haste_rating_enchant                      ) },
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
    { "gear_spell_hit_rating",                OPT_INT16,  &( gear.spell_hit_rating                          ) },
    { "gear_spell_crit_rating",               OPT_INT16,  &( gear.spell_crit_rating                         ) },
    { "gear_spell_penetration",               OPT_INT16,  &( gear.spell_penetration                         ) },
    { "gear_mp5",                             OPT_INT16,  &( gear.mp5                                       ) },
    { "enchant_spell_power",                  OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_MAX    ]      ) },
    { "enchant_spell_power_arcane",           OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_ARCANE ]      ) },
    { "enchant_spell_power_fire",             OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_FIRE   ]      ) },
    { "enchant_spell_power_frost",            OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_FROST  ]      ) },
    { "enchant_spell_power_holy",             OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_HOLY   ]      ) },
    { "enchant_spell_power_nature",           OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_NATURE ]      ) },
    { "enchant_spell_power_shadow",           OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_SHADOW ]      ) },
    { "enchant_spell_hit_rating",             OPT_INT16,  &( gear.spell_hit_rating_enchant                  ) },
    { "enchant_spell_crit_rating",            OPT_INT16,  &( gear.spell_crit_rating_enchant                 ) },
    { "enchant_spell_penetration",            OPT_INT16,  &( gear.spell_penetration_enchant                 ) },
    { "enchant_mp5",                          OPT_INT16,  &( gear.mp5_enchant                               ) },
    // Player - Gear - Attack									            
    { "gear_attack_power",                    OPT_INT16,  &( gear.attack_power                              ) },
    { "gear_attack_expertise_rating",         OPT_INT16,  &( gear.attack_expertise_rating                   ) },
    { "gear_attack_hit_rating",               OPT_INT16,  &( gear.attack_hit_rating                         ) },
    { "gear_attack_crit_rating",              OPT_INT16,  &( gear.attack_crit_rating                        ) },
    { "gear_attack_penetration",              OPT_INT16,  &( gear.attack_penetration                        ) },
    { "enchant_attack_power",                 OPT_INT16,  &( gear.attack_power_enchant                      ) },
    { "enchant_attack_expertise_rating",      OPT_INT16,  &( gear.attack_expertise_rating_enchant           ) },
    { "enchant_attack_hit_rating",            OPT_INT16,  &( gear.attack_hit_rating_enchant                 ) },
    { "enchant_attack_crit_rating",           OPT_INT16,  &( gear.attack_crit_rating_enchant                ) },
    { "enchant_attack_penetration",           OPT_INT16,  &( gear.attack_penetration_enchant                ) },
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
    // Player - Gear - Unique									            
    { "ashtongue_talisman",                   OPT_INT8,   &( gear.ashtongue_talisman                        ) },
    { "chaotic_skyfire",                      OPT_INT8,   &( gear.chaotic_skyfire                           ) },
    { "darkmoon_crusade",                     OPT_INT8,   &( gear.darkmoon_crusade                          ) },
    { "darkmoon_wrath",                       OPT_INT8,   &( gear.darkmoon_wrath                            ) },
    { "elder_scribes",                        OPT_INT8,   &( gear.elder_scribes                             ) },
    { "eternal_sage",                         OPT_INT8,   &( gear.eternal_sage                              ) },
    { "eye_of_magtheridon",                   OPT_INT8,   &( gear.eye_of_magtheridon                        ) },
    { "lightning_capacitor",                  OPT_INT8,   &( gear.lightning_capacitor                       ) },
    { "mark_of_defiance",                     OPT_INT8,   &( gear.mark_of_defiance                          ) },
    { "mystical_skyfire",                     OPT_INT8,   &( gear.mystical_skyfire                          ) },
    { "quagmirrans_eye",                      OPT_INT8,   &( gear.quagmirrans_eye                           ) },
    { "sextant_of_unstable_currents",         OPT_INT8,   &( gear.sextant_of_unstable_currents              ) },
    { "shiffars_nexus_horn",                  OPT_INT8,   &( gear.shiffars_nexus_horn                       ) },
    { "spellstrike",                          OPT_INT8,   &( gear.spellstrike                               ) },
    { "wrath_of_cenarius",                    OPT_INT8,   &( gear.wrath_of_cenarius                         ) },
    { "spellsurge",                           OPT_INT8,   &( gear.spellsurge                                ) },
    { "talisman_of_ascendance",               OPT_INT8,   &( gear.talisman_of_ascendance                    ) },
    { "timbals_crystal",                      OPT_INT8,   &( gear.timbals_crystal                           ) },
    { "zandalarian_hero_charm",               OPT_INT8,   &( gear.zandalarian_hero_charm                    ) },
    { "tier4_2pc",                            OPT_INT8,   &( gear.tier4_2pc                                 ) },
    { "tier4_4pc",                            OPT_INT8,   &( gear.tier4_4pc                                 ) },
    { "tier5_2pc",                            OPT_INT8,   &( gear.tier5_2pc                                 ) },
    { "tier5_4pc",                            OPT_INT8,   &( gear.tier5_4pc                                 ) },
    { "tier6_2pc",                            OPT_INT8,   &( gear.tier6_2pc                                 ) },
    { "tier6_4pc",                            OPT_INT8,   &( gear.tier6_4pc                                 ) },
    // Player - Consumables									            
    { "arcane_elixir",                        OPT_INT8,   &( consumables.arcane_elixir                      ) },
    { "brilliant_mana_oil",                   OPT_INT8,   &( consumables.brilliant_mana_oil                 ) },
    { "brilliant_wizard_oil",                 OPT_INT8,   &( consumables.brilliant_wizard_oil               ) },
    { "elixir_of_major_shadow_power",         OPT_INT8,   &( consumables.elixir_of_major_shadow_power       ) },
    { "elixir_of_shadow_power",               OPT_INT8,   &( consumables.elixir_of_shadow_power             ) },
    { "flask_of_supreme_power",               OPT_INT8,   &( consumables.flask_of_supreme_power             ) },
    { "greater_arcane_elixir",                OPT_INT8,   &( consumables.greater_arcane_elixir              ) },
    { "mageblood_potion",                     OPT_INT8,   &( consumables.mageblood_potion                   ) },
    { "nightfin_soup",                        OPT_INT8,   &( consumables.nightfin_soup                      ) },
    // Player - Buffs
    // FIXME! These will go away eventually, and be converted into player actions.
    { "blessing_of_salvation",                OPT_INT8,   &( buffs.blessing_of_salvation                    ) },
    { "blessing_of_wisdom",                   OPT_INT8,   &( buffs.blessing_of_wisdom                       ) },
    { "fel_armor",                            OPT_INT8,   &( buffs.fel_armor                                ) },
    { "improved_divine_spirit",               OPT_INT8,   &( buffs.improved_divine_spirit                   ) },
    { "mage_armor",                           OPT_INT8,   &( buffs.mage_armor                               ) },
    { "molten_armor",                         OPT_INT8,   &( buffs.molten_armor                             ) },
    { "sacrifice_pet",                        OPT_INT8,   &( buffs.sacrifice_pet                            ) },
    { "sanctity_aura",                        OPT_INT8,   &( buffs.sanctity_aura                            ) },
    { NULL, OPT_UNKNOWN }
  };
  
  if( name.empty() )
  {
    option_t::print( options );
    return false;
  }

  return option_t::parse( options, name, value );
}
