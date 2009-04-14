// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_judgement_of_wisdom ==============================================

static void trigger_judgement_of_wisdom( action_t* a )
{
  sim_t* sim = a -> sim;
  
  if( ! sim -> target -> debuffs.judgement_of_wisdom ) return;

  player_t* p = a -> player;

  double base_mana = p -> resource_base[ RESOURCE_MANA ];

  if( base_mana <= 0 )
    return;

  double proc_chance = sim -> jow_chance;

  if( sim -> jow_ppm )
  { 
    if( a -> weapon ) 
    {
      proc_chance = a -> weapon -> proc_chance_on_swing( sim -> jow_ppm,a -> time_to_execute );

      if( sim -> debug )
        report_t::log( sim, "JoW ppm %f, swing time %f, chance %f",
                       sim -> jow_ppm, a -> weapon -> swing_time,
                       proc_chance );
    }
    else
    {
      double time_to_execute = a -> channeled ? a -> time_to_tick : a -> time_to_execute;

      if( time_to_execute == 0 ) time_to_execute = p -> base_gcd;

      proc_chance = sim -> jow_ppm * time_to_execute / 60.0;
    }
  }

  if( ! sim -> roll( proc_chance ) )
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

// init_replenish_targets ================================================

static void init_replenish_targets( sim_t* sim )
{
  if( sim -> replenishment_candidates.empty() )
  {
    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( p -> type != PLAYER_GUARDIAN && 
	  p -> primary_resource() == RESOURCE_MANA )
      {
	sim -> replenishment_candidates.push_back( p );
      }
    }
  }
}

// choose_replenish_targets ==============================================

static void choose_replenish_targets( player_t* provider )
{
  sim_t* sim = provider -> sim;

  init_replenish_targets( sim );

  std::vector<player_t*>& candidates = sim      -> replenishment_candidates;
  std::vector<player_t*>& targets    = provider -> replenishment_targets;

  bool replenishment_missing = false;

  for( int i = candidates.size() - 1; i >= 0; i-- ) 
  {
    player_t* p = candidates[ i ];

    if( ! p -> sleeping && ! p -> buffs.replenishment )
    {
      replenishment_missing = true;
      break;
    }
  }

  // If replenishment is not missing from any of the candidates, then the target list will not change

  if( ! replenishment_missing ) return;

  for( int i = targets.size() - 1; i >= 0; i-- ) 
  {
    targets[ i ] -> buffs.replenishment = -1;
  }
  targets.clear();

  for( int i=0; i < sim -> replenishment_targets; i++ )
  {
    player_t* min_player=0;
    double    min_mana=0;
    
    for( int j = candidates.size() - 1; j >= 0; j-- )
    {
      player_t* p = candidates[ j ];

      if( p -> sleeping || ( p -> buffs.replenishment == 1 ) ) continue;

      if( ! min_player || min_mana > p -> resource_current[ RESOURCE_MANA ] )
      {
	min_player = p;
	min_mana   = p -> resource_current[ RESOURCE_MANA ];
      }
    }
    if( min_player )
    {
      if( min_player -> buffs.replenishment == 0 ) min_player -> aura_gain( "Replenishment" );
      min_player -> buffs.replenishment = 1;
      targets.push_back( min_player );
    }
    else break;
  }

  for( int i = candidates.size() - 1; i >= 0; i-- ) 
  {
    player_t* p = candidates[ i ];

    if( p -> buffs.replenishment < 0 ) 
    {
      p -> aura_loss( "Replenishment" );
      p -> buffs.replenishment = 0;
    }
  }
}

// replenish_targets =====================================================

static void replenish_targets( player_t* provider )
{
  sim_t* sim = provider -> sim;

  choose_replenish_targets( provider );

  if( provider -> replenishment_targets.empty() )
    return;

  struct replenish_tick_t : public event_t
  {
    int ticks_remaining;

    replenish_tick_t( sim_t* sim, player_t* provider, int ticks ) : event_t( sim, provider ), ticks_remaining( ticks )
    {
      name = "Replenishment Tick";
      sim -> add_event( this, 1.0 );
    }
    virtual void execute()
    {
      for( int i = player -> replenishment_targets.size() - 1; i >= 0; i-- )
      {
	player_t* p = player -> replenishment_targets[ i ];
	if( p -> sleeping ) continue;

	p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.0025, p -> gains.replenishment );
      }

      event_t*& e = player -> expirations.replenishment;

      if( --ticks_remaining == 0 )
      {
	e = 0;
      }
      else
      {
	e = new ( sim ) replenish_tick_t( sim, player, ticks_remaining );
      }
    }
    virtual void reschedule( double new_time )
    {
      ticks_remaining = 15; // Just refresh, do not reschedule.
    }
  };

  event_t*& e = provider -> expirations.replenishment;

  if( e )
  {
    e -> reschedule( 0.0 );
  }
  else
  {
    e = new ( sim ) replenish_tick_t( sim, provider, 15 );
  }
}

// replenish_raid ========================================================

static void replenish_raid( player_t* provider )
{
  sim_t* sim = provider -> sim;

  struct replenish_expiration_t : public event_t
  {
    replenish_expiration_t( sim_t* sim, player_t* provider ) : event_t( sim, provider )
    {
      name = "Replenishment Expiration";
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
       	if( p -> buffs.replenishment == 0 ) p -> aura_gain( "Replenishment" );
        p -> buffs.replenishment++;
      }
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
        p -> buffs.replenishment--;
       	if( p -> buffs.replenishment == 0 ) p -> aura_loss( "Replenishment" );
      }
      player -> expirations.replenishment = 0;
    }
  };

  event_t*& e = provider -> expirations.replenishment;

  if( e )
  {
    e -> reschedule( 15.0 );
  }
  else
  {
    e = new ( sim ) replenish_expiration_t( sim, provider );
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

  spell_power[ SCHOOL_MAX ] = (int) ( budget_slots * pow( target - cost, 1.0 / 1.5 ) / spell_power_statmod );
  
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
                    int                t,
                    const std::string& n ) :
  sim(s), name_str(n), next(0), type(t), level(80), party(0), member(0), gcd_ready(0), base_gcd(1.5), sleeping(0), pet_list(0),
  // Haste
  base_haste_rating(0), initial_haste_rating(0), haste_rating(0), spell_haste(1.0), attack_haste(1.0),
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
  mp5_per_intellect(0), 
  mana_regen_base(0), mana_regen_while_casting(0), 
  energy_regen_per_second(0), focus_regen_per_second(0),
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
  // Defense Mechanics
  base_armor(0), initial_armor(0), armor(0), armor_snapshot(0),
  armor_per_agility(2.0), use_armor_snapshot(false),
  // Resources 
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
  player_t** last = &( sim -> player_list );
  while( *last ) last = &( (*last) -> next );
  *last = this;
  next = 0;

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute[ i ] = attribute_base[ i ] = attribute_initial[ i ] = 0;
    attribute_multiplier[ i ] = attribute_multiplier_initial[ i ] = 1.0;
  }

  for( int i=0; i < PROF_MAX; i++ )
  {
    profession[ i ] = 0;    
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
  init_race();
  init_spell();
  init_attack();
  init_defense();
  init_weapon( &main_hand_weapon, main_hand_str );
  init_weapon(  &off_hand_weapon,  off_hand_str );
  init_weapon(    &ranged_weapon,    ranged_str );
  init_unique_gear();
  init_professions();
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

// player_t::init_race ======================================================

void player_t::init_race()
{
  int r;  
  for( r=0; r < RACE_MAX; r++ )
  {
    const char* name = util_t::race_type_string( r );
    if( race_str == name ) break;
  }

  if( r < PROF_MAX )
  {
    race = r;
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
  }

  if( ! is_pet() )
  {
    initial_spell_power[ SCHOOL_MAX ] += sim -> gear_delta.spell_power;
    initial_spell_hit  += sim -> gear_delta. hit_rating / rating.spell_hit;
    initial_spell_crit += sim -> gear_delta.crit_rating / rating.spell_crit;
  }

  double base_60, base_70, base_80;

  if( sim -> patch.after( 3, 1, 0 ) )
  {
    base_60 = 0.006600;
    base_70 = 0.005596;
    base_80 = 0.003345;
  }
  else
  {
    base_60 = 0.011000;
    base_70 = 0.009327;
    base_80 = 0.005575;
  }

  mana_regen_base = rating_t::interpolate( level, base_60, base_70, base_80 );
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

// player_t::init_defense ====================================================

void player_t::init_defense() 
{
  if( initial_armor == 0 )
  {
    initial_armor = base_armor + gear.armor + gear.armor_enchant;
  }

  if( ! is_pet() )
  {
    initial_attack_power += sim -> gear_delta.armor;
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
      else if( parm == "enchant_bonus" )
      {
        w -> enchant_bonus = atoi( value.c_str() );
      }
      else if( parm == "buff_bonus" )
      {
        w -> buff_bonus = atoi( value.c_str() );
      }
      else invalid = true;

      if( invalid )
      {
        printf( "Invalid weapon encoding: %s\n", encoding.c_str() );
        assert(0);
      }
    }
  }

  if( w -> enchant == SCOPE && w -> enchant_bonus == 0 )
  {
    w -> enchant_bonus = util_t::ability_rank( level, 15.0,72,  12.0,67,  7.0,0 );
  }

  if( weapon_dps != 0 ) w -> damage = weapon_dps * w -> swing_time;

  if( w -> slot == SLOT_MAIN_HAND ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_2H );
  if( w -> slot == SLOT_OFF_HAND  ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_2H );
  if( w -> slot == SLOT_RANGED    ) assert( w -> type == WEAPON_NONE || ( w -> type > WEAPON_2H && w -> type < WEAPON_RANGED ) );
}

// player_t::init_unique_gear ==============================================

void player_t::init_unique_gear()
{
  unique_gear_t::init( this );
}

// player_t::init_resources ================================================

void player_t::init_resources( bool force ) 
{
  // The first 20pts of intellect/stamina only provide 1pt of mana/health.
  // Code simplified on the assumption that the minimum player level is 60.
  double adjust = is_pet() ? 0 : 20;

  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    if( force || resource_initial[ i ] == 0 )
    {
      resource_initial[ i ] = resource_base[ i ] + gear.resource[ i ] + gear.resource_enchant[ i ];

      if( i == RESOURCE_MANA   ) resource_initial[ i ] += ( intellect() - adjust ) * mana_per_intellect + adjust;
      if( i == RESOURCE_HEALTH ) resource_initial[ i ] += (   stamina() - adjust ) * health_per_stamina + adjust;
    }
    resource_current[ i ] = resource_max[ i ] = resource_initial[ i ];
  }

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

// player_t::init_professions ==============================================

void player_t::init_professions()                        
{
  if( professions_str.empty() ) return;
  
  bool invalid = false;
  
  std::vector<std::string> splits;
  int size = util_t::string_split( splits, professions_str, "," );

  for( int i=0; i<size; i++)
  {
        std::string& s = splits[ i ];
        std::string parm, value;
        if( 2 != util_t::string_split( s, "=", "S S", &parm, &value ) ) invalid = true;
        
        int p;
    for( p=0; p < PROF_MAX; p++ )
    {
      const char* name = util_t::profession_type_string( p );
      if( parm == name ) break;
    }

        if( p < PROF_MAX ) profession[ p ] = atoi( value.c_str() );
    else invalid = true;
  }
  
  if( invalid )
  {
        printf( "Invalid profession encoding: %s\n", professions_str.c_str() );
        assert(0);
  }     
  
  // Miners gain additional stamina 
  if      ( profession[ PROF_MINING ] >= 450 ) attribute_initial[ ATTR_STAMINA ] += 50.0;
  else if ( profession[ PROF_MINING ] >= 375 ) attribute_initial[ ATTR_STAMINA ] += 30.0;
  else if ( profession[ PROF_MINING ] >= 300 ) attribute_initial[ ATTR_STAMINA ] += 10.0;
  else if ( profession[ PROF_MINING ] >= 225 ) attribute_initial[ ATTR_STAMINA ] +=  7.0;
  else if ( profession[ PROF_MINING ] >= 150 ) attribute_initial[ ATTR_STAMINA ] +=  5.0;
  else if ( profession[ PROF_MINING ] >=  75 ) attribute_initial[ ATTR_STAMINA ] +=  3.0;       
  
  // Skinners gain additional crit rating
  if      ( profession[ PROF_SKINNING ] >= 450 ) initial_attack_crit += 32.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >= 375 ) initial_attack_crit += 20.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >= 300 ) initial_attack_crit += 12.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >= 225 ) initial_attack_crit +=  9.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >= 150 ) initial_attack_crit +=  6.0 / rating.attack_crit;
  else if ( profession[ PROF_SKINNING ] >=  75 ) initial_attack_crit +=  3.0 / rating.attack_crit;        
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

  if ( sim -> P309 )
  {
    rating.armor_penetration *= 1.25;
  }
}

// player_t::init_stats ====================================================

void player_t::init_stats() 
{
  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_lost[ i ] = resource_gained[ i ] = 0;
  }

  gains.ashtongue_talisman     = get_gain( "ashtongue_talisman" );
  gains.blessing_of_wisdom     = get_gain( "blessing_of_wisdom" );
  gains.dark_rune              = get_gain( "dark_rune" );
  gains.energy_regen           = get_gain( "energy_regen" );
  gains.focus_regen            = get_gain( "focus_regen" );
  gains.innervate              = get_gain( "innervate" );
  gains.glyph_of_innervate     = get_gain( "glyph_of_innervate" );
  gains.judgement_of_wisdom    = get_gain( "judgement_of_wisdom" );
  gains.mana_potion            = get_gain( "mana_potion" );
  gains.mana_spring            = get_gain( "mana_spring" );
  gains.mana_tide              = get_gain( "mana_tide" );
  gains.mark_of_defiance       = get_gain( "mark_of_defiance" );
  gains.mirror_of_truth        = get_gain( "mirror_of_truth" );
  gains.mp5_regen              = get_gain( "mp5_regen" );
  gains.replenishment          = get_gain( "replenishment" );
  gains.restore_mana           = get_gain( "restore_mana" );
  gains.spellsurge             = get_gain( "spellsurge" );
  gains.spirit_intellect_regen = get_gain( "spirit_intellect_regen" );
  gains.vampiric_touch         = get_gain( "vampiric_touch" );
  gains.water_elemental        = get_gain( "water_elemental" );
  gains.tier4_2pc              = get_gain( "tier4_2pc" );
  gains.tier4_4pc              = get_gain( "tier4_4pc" );
  gains.tier5_2pc              = get_gain( "tier5_2pc" );
  gains.tier5_4pc              = get_gain( "tier5_4pc" );
  gains.tier6_2pc              = get_gain( "tier6_2pc" );
  gains.tier6_4pc              = get_gain( "tier6_4pc" );
  gains.tier7_2pc              = get_gain( "tier7_2pc" );
  gains.tier7_4pc              = get_gain( "tier7_4pc" );
  gains.tier8_2pc              = get_gain( "tier8_2pc" );
  gains.tier8_4pc              = get_gain( "tier8_4pc" );

  procs.ashtongue_talisman           = get_proc( "ashtongue_talisman" );
  procs.darkmoon_greatness           = get_proc( "darkmoon_greatness" );
  procs.dying_curse                  = get_proc( "dying_curse" );
  procs.elder_scribes                = get_proc( "elder_scribes" );
  procs.elemental_focus_stone        = get_proc( "elemental_focus_stone" );
  procs.embrace_of_the_spider        = get_proc( "embrace_of_the_spider" );
  procs.eternal_sage                 = get_proc( "eternal_sage" );
  procs.extract_of_necromatic_power  = get_proc( "extract_of_necromatic_power" );
  procs.eye_of_magtheridon           = get_proc( "eye_of_magtheridon" );
  procs.flare_of_the_heavens         = get_proc( "flare_of_the_heavens" );
  procs.forge_ember                  = get_proc( "forge_ember" );
  procs.grim_toll                    = get_proc( "grim_toll" );
  procs.honor_among_thieves_donor    = ( is_pet() ? ( cast_pet() -> owner ) : this ) -> get_proc( "honor_among_thieves_donor" );
  procs.judgement_of_wisdom          = get_proc( "judgement_of_wisdom" );
  procs.lightning_capacitor          = get_proc( "lightning_capacitor" );
  procs.mark_of_defiance             = get_proc( "mark_of_defiance" );
  procs.mirror_of_truth              = get_proc( "mirror_of_truth" );
  procs.mystical_skyfire             = get_proc( "mystical_skyfire" );
  procs.pyrite_infuser               = get_proc( "pyrite_infuser" );
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
  procs.tier8_2pc                    = get_proc( "tier8_2pc" );
  procs.tier8_4pc                    = get_proc( "tier8_4pc" );

  uptimes.replenishment = get_uptime( "replenishment" );
  uptimes.berserking_mh = get_uptime( "berserking_mh" );
  uptimes.berserking_oh = get_uptime( "berserking_oh" );
  uptimes.executioner   = get_uptime( "executioner" );
  uptimes.mongoose_mh   = get_uptime( "mongoose_mh" );
  uptimes.mongoose_oh   = get_uptime( "mongoose_oh" );
  uptimes.tier4_2pc     = get_uptime( "tier4_2pc" );
  uptimes.tier4_4pc     = get_uptime( "tier4_4pc" );
  uptimes.tier5_2pc     = get_uptime( "tier5_2pc" );
  uptimes.tier5_4pc     = get_uptime( "tier5_4pc" );
  uptimes.tier6_2pc     = get_uptime( "tier6_2pc" );
  uptimes.tier6_4pc     = get_uptime( "tier6_4pc" );
  uptimes.tier7_2pc     = get_uptime( "tier7_2pc" );
  uptimes.tier7_4pc     = get_uptime( "tier7_4pc" );
  uptimes.tier8_2pc     = get_uptime( "tier8_2pc" );
  uptimes.tier8_4pc     = get_uptime( "tier8_4pc" );
  
  iteration_dps.clear();
  iteration_dps.insert( iteration_dps.begin(), sim -> iterations, 0 );
}

// player_t::composite_attack_power ========================================

double player_t::composite_attack_power() 
{
  double ap = attack_power;

  ap += attack_power_per_strength * strength();
  ap += attack_power_per_agility  * agility();

  ap += std::max( buffs.blessing_of_might, buffs.battle_shout );

  return ap;
}

// player_t::composite_attack_crit =========================================

double player_t::composite_attack_crit()
{
  double ac = attack_crit + attack_crit_per_agility * agility();

  if( type != PLAYER_GUARDIAN )
  {
    if( buffs.leader_of_the_pack || 
        buffs.rampage ) 
    {
      ac += 0.05;
    }
  }

  return ac;
}

// player_t::composite_armor =========================================

double player_t::composite_armor()
{
  return armor + armor_per_agility * agility();
}

// player_t::composite_spell_power ========================================

double player_t::composite_spell_power( int school ) 
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

  if( type != PLAYER_GUARDIAN )
  {
    double best_buff = 0;
    if( buffs.totem_of_wrath )
    {
      if( best_buff < buffs.totem_of_wrath ) best_buff = buffs.totem_of_wrath;
    }
    if( buffs.flametongue_totem ) 
    {
      if( best_buff < buffs.flametongue_totem ) best_buff = buffs.flametongue_totem;
    }
    if( buffs.demonic_pact )
    {
      if( best_buff < buffs.demonic_pact ) best_buff = buffs.demonic_pact;
    }
    if( buffs.improved_divine_spirit )
    {
      if( best_buff < buffs.improved_divine_spirit ) best_buff = buffs.improved_divine_spirit;
    }
    sp += best_buff;
  }

  return sp;
}

// player_t::composite_spell_crit ==========================================

double player_t::composite_spell_crit()
{
  double sc = spell_crit + spell_crit_per_intellect * intellect();

  if( type != PLAYER_GUARDIAN )
  {
    if( buffs.focus_magic ) sc += 0.03;

    if( buffs.elemental_oath ||
	buffs.moonkin_aura   )
    {
      sc += 0.05;
    }
  }

  return sc;
}

// player_t::composite_attack_power_multiplier =============================

double player_t::composite_attack_power_multiplier()
{
  double m = attack_power_multiplier;

  if( buffs.unleashed_rage ||
      buffs.trueshot_aura ||
      buffs.abominations_might )
  {
    m *= 1.10;
  }

  return m;
}

// player_t::composite_attribute_multiplier ================================

double player_t::composite_attribute_multiplier( int attr )
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

  if( sim -> overrides.abominations_might     ) buffs.abominations_might = 1;
  if( sim -> overrides.battle_shout           ) buffs.battle_shout = 548;
  if( sim -> overrides.blessing_of_kings      ) buffs.blessing_of_kings = 1;
  if( sim -> overrides.blessing_of_might      ) buffs.blessing_of_might = 688;
  if( sim -> overrides.blessing_of_wisdom     ) buffs.blessing_of_wisdom = 91*1.2;
  if( sim -> overrides.sanctified_retribution ) buffs.sanctified_retribution = 1;
  if( sim -> overrides.swift_retribution      ) buffs.swift_retribution = 1;
  if( sim -> overrides.arcane_brilliance      ) buffs.arcane_brilliance = 60;
  if( sim -> overrides.divine_spirit          ) buffs.divine_spirit = 80;
  if( sim -> overrides.fortitude              ) buffs.fortitude = 215;
  if( sim -> overrides.improved_divine_spirit ) buffs.improved_divine_spirit = 80;
  if( sim -> overrides.improved_moonkin_aura  ) buffs.improved_moonkin_aura = 1;
  if( sim -> overrides.leader_of_the_pack     ) buffs.leader_of_the_pack = 1;
  if( sim -> overrides.mana_spring            ) buffs.mana_spring = ( sim -> P309 ? 42.5 : 91.0*1.2 );
  if( sim -> overrides.mark_of_the_wild       ) buffs.mark_of_the_wild = 52;
  if( sim -> overrides.moonkin_aura           ) buffs.moonkin_aura = 1;
  if( sim -> overrides.rampage                ) buffs.rampage = 1;
  if( sim -> overrides.replenishment          ) buffs.replenishment = 1;
  if( sim -> overrides.strength_of_earth      ) buffs.strength_of_earth = 178;
  if( sim -> overrides.totem_of_wrath         ) buffs.totem_of_wrath = 280;
  if( sim -> overrides.trueshot_aura          ) buffs.trueshot_aura = 1;
  if( sim -> overrides.unleashed_rage         ) buffs.unleashed_rage = 1;
  if( sim -> overrides.windfury_totem         ) buffs.windfury_totem = 0.20;
  if( sim -> overrides.wrath_of_air           ) buffs.wrath_of_air = 1;

  init_resources( true );

  if( primary_resource() == RESOURCE_MANA )
  {
    get_gain( "initial_mana" ) -> add( resource_max[ RESOURCE_MANA ] );
  }

  if( use_armor_snapshot )
  {
    assert( sim -> armor_update_interval > 0 );

    struct armor_snapshot_event_t : public event_t
    {
      armor_snapshot_event_t( sim_t* sim, player_t* p, double interval ) : event_t( sim, p )
      {
        name = "Armor Snapshot";
        sim -> add_event( this, interval );
      }
      virtual void execute()
      {
        if( sim -> debug)
          report_t::log( sim, "Armor Snapshot: %.02f -> %.02f", player -> armor_snapshot, player -> composite_armor() );
        player -> armor_snapshot = player -> composite_armor();
        new ( sim ) armor_snapshot_event_t( sim, player, sim -> armor_update_interval );
      }
    };
    armor_snapshot = composite_armor();
    new ( sim ) armor_snapshot_event_t( sim, this, sim -> current_iteration % sim -> armor_update_interval );
  }
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

  armor              = initial_armor;
  armor_snapshot     = 0;

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
  food            = FOOD_NONE;

  buffs.reset();
  expirations.reset();
  cooldowns.reset();

  replenishment_targets.clear();

  init_resources( true );

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

    if( last_foreground_action )
    {
      if( last_foreground_action -> trigger_gcd == 0 )
      {
        lag = 0;
      }
      else if( last_foreground_action -> channeled ) 
      {
        lag = sim -> rng -> range( sim -> channel_lag - sim -> channel_lag_range,
                                   sim -> channel_lag + sim -> channel_lag_range );
      }
      else if( gcd_adjust > 0 ) 
      {
        lag = sim -> rng -> range( sim -> gcd_lag - sim -> gcd_lag_range,
                                   sim -> gcd_lag + sim -> gcd_lag_range );
      }
      else // queued cast
      {
        lag = sim -> rng -> range( sim -> queue_lag - sim -> queue_lag_range,
                                   sim -> queue_lag + sim -> queue_lag_range );
      }
    }

    if( lag < 0 ) lag = 0;

    delta_time += lag;
  }

  if( last_foreground_action ) 
  {
    last_foreground_action -> stats -> total_execute_time += delta_time;
  }

  if( delta_time == 0 ) delta_time = 0.000001;
  
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

    if( action -> terminal() )
      break;
  }

  last_foreground_action = action;

  if( action ) 
  {
    action -> schedule_execute();
  }

  return action;
}

// player_t::regen =========================================================

void player_t::regen( double periodicity )
{
  int resource_type = primary_resource();

  if( sim -> infinite_resource[ resource_type ] == 0 )
  {
    if( resource_type == RESOURCE_ENERGY )
    {
      double energy_regen = periodicity * energy_regen_per_second;

      resource_gain( resource_type, energy_regen, gains.energy_regen );
    }

    if( resource_type == RESOURCE_FOCUS )
    {
      double focus_regen = periodicity * focus_regen_per_second;

      resource_gain( resource_type, focus_regen, gains.focus_regen );
    }

    if( resource_type == RESOURCE_MANA )
    {
      double spirit_regen = periodicity * sqrt( intellect() ) * spirit() * mana_regen_base;

      if( buffs.innervate )
      {
	if( buffs.glyph_of_innervate ) 
        {
	  resource_gain( resource_type, spirit_regen, gains.glyph_of_innervate );
	}

	spirit_regen *= 5.0;

	resource_gain( resource_type, spirit_regen, gains.innervate );      
      }
      else if( buffs.glyph_of_innervate )
      {
	resource_gain( resource_type, spirit_regen, gains.glyph_of_innervate );
      }
      else if( recent_cast() )
      {
	if( mana_regen_while_casting < 1.0 )
	{
	  spirit_regen *= mana_regen_while_casting;
	}

	resource_gain( resource_type, spirit_regen, gains.spirit_intellect_regen );      
      }

      double mp5_regen = periodicity * ( mp5 + intellect() * mp5_per_intellect ) / 5.0;

      resource_gain( resource_type, mp5_regen, gains.mp5_regen );

      if( sim -> overrides.replenishment || ( buffs.replenishment && sim -> replenishment_targets <= 0 ) )
      {
	double replenishment_regen = periodicity * resource_max[ resource_type ] * 0.0025 / 1.0;

	resource_gain( resource_type, replenishment_regen, gains.replenishment );
      }
      uptimes.replenishment -> update( buffs.replenishment != 0 );

      if( sim -> P309 && buffs.water_elemental )
      {
        double water_elemental_regen = periodicity * resource_max[ resource_type ] * 0.006 / 5.0;

        resource_gain( resource_type, water_elemental_regen, gains.water_elemental );
      }

      if( sim -> P309 || ( buffs.blessing_of_wisdom >= buffs.mana_spring ) )
      {
	double wisdom_regen = periodicity * buffs.blessing_of_wisdom / 5.0;

	resource_gain( resource_type, wisdom_regen, gains.blessing_of_wisdom );
      }

      if( sim -> P309 || ( buffs.mana_spring > buffs.blessing_of_wisdom ) )
      {
	double mana_spring_regen = periodicity * buffs.mana_spring / 2.0;

	resource_gain( resource_type, mana_spring_regen, gains.mana_spring );
      }
    }
  }

  if( resource_type != RESOURCE_NONE )
  {
    int index = (int) sim -> current_time;
    int size = timeline_resource.size();

    if( index >= size ) timeline_resource.insert( timeline_resource.begin() + size, size, 0 );

    timeline_resource[ index ] += resource_current[ resource_type ];
  }
}

// player_t::resource_loss =================================================

double player_t::resource_loss( int resource,
                                double amount )
{
  if( amount == 0 ) return 0;

  double actual_amount;

  if( sim -> infinite_resource[ resource ] == 0 )
  {
    resource_current[ resource ] -= amount;
    actual_amount = amount;
  }
  else 
  {
    actual_amount = 0;
  }

  resource_lost[ resource ] += amount;

  if( resource == RESOURCE_MANA )
  {
    last_cast = sim -> current_time;
  }

  if( sim -> debug ) report_t::log( sim, "Player %s spends %.0f %s", name(), amount, util_t::resource_type_string( resource ) );

  return actual_amount;
}

// player_t::resource_gain =================================================

double player_t::resource_gain( int     resource,
                                double  amount,
                                gain_t* source )
{
  if( sleeping ) return 0;

  double actual_amount = std::min( amount, resource_max[ resource ] - resource_current[ resource ] );
  if( actual_amount > 0 )
  {
    resource_current[ resource ] += actual_amount;
  }

  resource_gained [ resource ] += actual_amount;
  if( source ) source -> add( actual_amount, amount - actual_amount );

  if( sim -> log ) 
    report_t::log( sim, "%s gains %.0f (%.0f) %s from %s (%.0f)", 
                   name(), actual_amount, amount, 
                   util_t::resource_type_string( resource ), source ? source -> name() : "unknown", 
                   resource_current[ resource ] );

  return actual_amount;
}

// player_t::resource_available ============================================

bool player_t::resource_available( int    resource,
                                   double cost )
{
  if( resource == RESOURCE_NONE || cost == 0 )
  {
    return true;
  }

  return resource_current[ resource ] >= cost;
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
  if( ! action -> background )
  {
    gcd_ready = sim -> current_time + action -> gcd();
    executing = action -> execute_event;
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
                              int       dmg_type )
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
                                   int      dmg_type )
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
                                    double    amount,
                                    int       dmg_type )
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

// player_t::recalculate_haste ==============================================

void player_t::recalculate_haste()
{
   spell_haste = 1.0 / ( 1.0 + haste_rating / rating. spell_haste );
  attack_haste = 1.0 / ( 1.0 + haste_rating / rating.attack_haste );
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

// player_t::reset_sequence =================================================

void player_t::reset_sequence( const std::string& seq_name )
{
  for( action_t* a = action_list; a; a = a -> next )
  {
    if( a -> seq_str.empty() )
      continue;

    if( seq_name.empty() || seq_name == a -> seq_str )
    {
      a -> seq_completed = false;
    }
  }
}

// Reset Sequnce Action ====================================================

struct reset_seq_t : public action_t
{
  std::string seq_name;

  reset_seq_t( player_t* player, const std::string& options_str ) : 
    action_t( ACTION_OTHER, "reset_seq", player )
  {
    option_t options[] =
    {
      { "name", OPT_STRING, &seq_name },
      { NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
  }

  virtual void execute() 
  {
    player -> reset_sequence( seq_name );
  }

  virtual bool ready()
  {
    return true;
  }
};

// Cycle Action ============================================================

struct cycle_t : public action_t
{
  action_t* current_action;

  cycle_t( player_t* player, const std::string& options_str ) : 
    action_t( ACTION_OTHER, "cycle", player ), current_action(0)
  {
  }

  virtual void reset()
  {
    action_t::reset();

    current_action = next;
    if( ! current_action )
    {
      printf( "simcraft: player %s has no actions after 'cycle'\n", player -> name() );
      exit( 0 );
    }
    is_terminal = true;
  }

  virtual void schedule_execute() 
  {
    player -> last_foreground_action = current_action;
    current_action -> schedule_execute();
    current_action = current_action -> next;
    if( ! current_action ) current_action = next;
  }

  virtual bool ready()
  {
    if( ! current_action ) return false;

    return current_action -> ready();
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

// Wait Until Ready Action ===================================================

struct wait_until_ready_t : public action_t
{
  double sec;

  wait_until_ready_t( player_t* player, const std::string& options_str ) : 
    action_t( ACTION_OTHER, "wait", player ), sec(1.0)
  {
    option_t options[] =
    {
      { "sec", OPT_FLT, &sec },
      { NULL }
    };
    parse_options( options, options_str );
  }

  virtual void execute() 
  {
    trigger_gcd = sec;

    for( action_t* a = player -> action_list; a; a = a -> next )
    {
      if( a -> background ) continue;
      
      if( a -> cooldown_ready > sim -> current_time )
      {
	double delta_time = a -> cooldown_ready - sim -> current_time;
	if( delta_time < trigger_gcd ) trigger_gcd = delta_time;
      }

      if( a -> duration_ready > sim -> current_time )
      {
	double delta_time = a -> duration_ready - ( sim -> current_time + a -> execute_time() );
	if( delta_time < trigger_gcd ) trigger_gcd = delta_time;
      }
    }

    player -> total_waiting += trigger_gcd;
  }
};

// player_t::create_action ==================================================

action_t* player_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if( name == "cycle"        ) return new            cycle_t( this, options_str );
  if( name == "reset_seq"    ) return new        reset_seq_t( this, options_str );
  if( name == "restore_mana" ) return new     restore_mana_t( this, options_str );
  if( name == "wait"         ) return new wait_until_ready_t( this, options_str );

  return 0;
}

// player_t::find_pet =======================================================

pet_t* player_t::find_pet( const std::string& pet_name )
{
  for( pet_t* p = pet_list; p; p = p -> next_pet )
    if( p -> name_str == pet_name )
      return p;

  return 0;
}

// player_t::trigger_replenishment ================================================

void player_t::trigger_replenishment()
{
  if( sim -> overrides.replenishment ) 
    return;

  if( sim -> replenishment_targets > 0 )
  {
    replenish_targets( this );
  }
  else
  {
    replenish_raid( this );
  }
}

// player_t::get_talent_trees ===============================================

bool player_t::get_talent_trees( std::vector<int*>& tree1,
                                 std::vector<int*>& tree2,
                                 std::vector<int*>& tree3,
                                 talent_translation_t translation[][3] )
{
  for( int i=0; translation[ i ][ 0 ].index > 0; i++ ) tree1.push_back( translation[ i ][ 0 ].address );
  for( int i=0; translation[ i ][ 1 ].index > 0; i++ ) tree2.push_back( translation[ i ][ 1 ].address );
  for( int i=0; translation[ i ][ 2 ].index > 0; i++ ) tree3.push_back( translation[ i ][ 2 ].address );

  return true;
}

// player_t::parse_talents ==================================================

bool player_t::parse_talents( std::vector<int*>& talent_tree,
                              const std::string& talent_string )
{
  const char* s = talent_string.c_str();

  unsigned int size = std::min( talent_tree.size(), talent_string.size() );

  for( unsigned int i=0; i < size; i++ )
  {
    int* address = talent_tree[ i ];
    if( ! address ) continue;
    *address = s[ i ] - '0';
  }
  
  return true;
}

// player_t::parse_talents ==================================================

bool player_t::parse_talents( const std::string& talent_string )
{
  std::vector<int*> talent_tree1, talent_tree2, talent_tree3;

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

  std::vector<int*> talent_trees[ 3 ];
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
    { "name",                                 OPT_STRING, &( name_str                                       ) },
    { "race",                                 OPT_STRING, &( race_str                                       ) },
    { "level",                                OPT_INT,    &( level                                          ) },
    { "gcd",                                  OPT_FLT,    &( base_gcd                                       ) },
    { "sleeping",                             OPT_INT,    &( sleeping                                       ) },
    // Player - Professions
    { "professions",                          OPT_STRING, &( professions_str                                ) },
    // Player - Haste                                                                                     
    { "haste_rating",                         OPT_INT,    &( initial_haste_rating                           ) },
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
    // Player - Defense Mechanics
    { "armor",                                OPT_FLT,    &( initial_armor                                  ) },
    { "base_armor",                           OPT_FLT,    &( base_armor                                     ) },
    { "armor_per_agility",                    OPT_FLT,    &( armor_per_agility                              ) },
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
    { "quiet",                                OPT_INT,   &( quiet                                          ) },
    // Player - Gear - Attributes                                                                           
    { "gear_strength",                        OPT_INT,  &( gear.attribute        [ ATTR_STRENGTH  ]       ) },
    { "gear_agility",                         OPT_INT,  &( gear.attribute        [ ATTR_AGILITY   ]       ) },
    { "gear_stamina",                         OPT_INT,  &( gear.attribute        [ ATTR_STAMINA   ]       ) },
    { "gear_intellect",                       OPT_INT,  &( gear.attribute        [ ATTR_INTELLECT ]       ) },
    { "gear_spirit",                          OPT_INT,  &( gear.attribute        [ ATTR_SPIRIT    ]       ) },
    { "enchant_strength",                     OPT_INT,  &( gear.attribute_enchant[ ATTR_STRENGTH  ]       ) },
    { "enchant_agility",                      OPT_INT,  &( gear.attribute_enchant[ ATTR_AGILITY   ]       ) },
    { "enchant_stamina",                      OPT_INT,  &( gear.attribute_enchant[ ATTR_STAMINA   ]       ) },
    { "enchant_intellect",                    OPT_INT,  &( gear.attribute_enchant[ ATTR_INTELLECT ]       ) },
    { "enchant_spirit",                       OPT_INT,  &( gear.attribute_enchant[ ATTR_SPIRIT    ]       ) },
    // Player - Gear - Spell                                                                                
    { "gear_spell_power",                     OPT_INT,  &( gear.spell_power[ SCHOOL_MAX    ]              ) },
    { "gear_spell_power_arcane",              OPT_INT,  &( gear.spell_power[ SCHOOL_ARCANE ]              ) },
    { "gear_spell_power_fire",                OPT_INT,  &( gear.spell_power[ SCHOOL_FIRE   ]              ) },
    { "gear_spell_power_frost",               OPT_INT,  &( gear.spell_power[ SCHOOL_FROST  ]              ) },
    { "gear_spell_power_holy",                OPT_INT,  &( gear.spell_power[ SCHOOL_HOLY   ]              ) },
    { "gear_spell_power_nature",              OPT_INT,  &( gear.spell_power[ SCHOOL_NATURE ]              ) },
    { "gear_spell_power_shadow",              OPT_INT,  &( gear.spell_power[ SCHOOL_SHADOW ]              ) },
    { "gear_spell_penetration",               OPT_INT,  &( gear.spell_penetration                         ) },
    { "gear_mp5",                             OPT_INT,  &( gear.mp5                                       ) },
    { "enchant_spell_power",                  OPT_INT,  &( gear.spell_power_enchant[ SCHOOL_MAX    ]      ) },
    { "enchant_spell_power_arcane",           OPT_INT,  &( gear.spell_power_enchant[ SCHOOL_ARCANE ]      ) },
    { "enchant_spell_power_fire",             OPT_INT,  &( gear.spell_power_enchant[ SCHOOL_FIRE   ]      ) },
    { "enchant_spell_power_frost",            OPT_INT,  &( gear.spell_power_enchant[ SCHOOL_FROST  ]      ) },
    { "enchant_spell_power_holy",             OPT_INT,  &( gear.spell_power_enchant[ SCHOOL_HOLY   ]      ) },
    { "enchant_spell_power_nature",           OPT_INT,  &( gear.spell_power_enchant[ SCHOOL_NATURE ]      ) },
    { "enchant_spell_power_shadow",           OPT_INT,  &( gear.spell_power_enchant[ SCHOOL_SHADOW ]      ) },
    { "enchant_spell_penetration",            OPT_INT,  &( gear.spell_penetration_enchant                 ) },
    { "enchant_mp5",                          OPT_INT,  &( gear.mp5_enchant                               ) },
    // Player - Gear - Attack                                                                               
    { "gear_attack_power",                    OPT_INT,  &( gear.attack_power                              ) },
    { "gear_expertise_rating",                OPT_INT,  &( gear.expertise_rating                          ) },
    { "gear_armor_penetration_rating",        OPT_INT,  &( gear.armor_penetration_rating                  ) },
    { "enchant_attack_power",                 OPT_INT,  &( gear.attack_power_enchant                      ) },
    { "enchant_expertise_rating",             OPT_INT,  &( gear.expertise_rating_enchant                  ) },
    { "enchant_armor_penetration",            OPT_INT,  &( gear.armor_penetration_rating_enchant          ) },
    // Player - Gear - Defense
    { "gear_armor",                           OPT_INT,  &( gear.armor                                     ) },
    { "enchant_armor",                        OPT_INT,  &( gear.armor_enchant                             ) },
    // Player - Gear - Common                                                                               
    { "gear_haste_rating",                    OPT_INT,  &( gear.haste_rating                              ) },
    { "gear_hit_rating",                      OPT_INT,  &( gear.hit_rating                                ) },
    { "gear_crit_rating",                     OPT_INT,  &( gear.crit_rating                               ) },
    { "enchant_haste_rating",                 OPT_INT,  &( gear.haste_rating_enchant                      ) },
    { "enchant_hit_rating",                   OPT_INT,  &( gear.hit_rating_enchant                        ) },
    { "enchant_crit_rating",                  OPT_INT,  &( gear.crit_rating_enchant                       ) },
    // Player - Gear - Resource                                                                             
    { "gear_health",                          OPT_INT,  &( gear.resource        [ RESOURCE_HEALTH ]       ) },
    { "gear_mana",                            OPT_INT,  &( gear.resource        [ RESOURCE_MANA   ]       ) },
    { "gear_rage",                            OPT_INT,  &( gear.resource        [ RESOURCE_RAGE   ]       ) },
    { "gear_energy",                          OPT_INT,  &( gear.resource        [ RESOURCE_ENERGY ]       ) },
    { "gear_focus",                           OPT_INT,  &( gear.resource        [ RESOURCE_FOCUS  ]       ) },
    { "gear_runic",                           OPT_INT,  &( gear.resource        [ RESOURCE_RUNIC  ]       ) },
    { "enchant_health",                       OPT_INT,  &( gear.resource_enchant[ RESOURCE_HEALTH ]       ) },
    { "enchant_mana",                         OPT_INT,  &( gear.resource_enchant[ RESOURCE_MANA   ]       ) },
    { "enchant_rage",                         OPT_INT,  &( gear.resource_enchant[ RESOURCE_RAGE   ]       ) },
    { "enchant_energy",                       OPT_INT,  &( gear.resource_enchant[ RESOURCE_ENERGY ]       ) },
    { "enchant_focus",                        OPT_INT,  &( gear.resource_enchant[ RESOURCE_FOCUS  ]       ) },
    { "enchant_runic",                        OPT_INT,  &( gear.resource_enchant[ RESOURCE_RUNIC  ]       ) },
    // Player - Gear - Budgeting                                                                            
    { "spell_power_budget",                   OPT_INT,  &( gear.spell_power_budget                        ) },
    { "attack_power_budget",                  OPT_INT,  &( gear.attack_power_budget                       ) },
    { "budget_slots",                         OPT_INT,  &( gear.budget_slots                              ) },
    // Player - Tier Bonuses
    { "tier4_2pc",                            OPT_INT,  &( gear.tier4_2pc                                 ) },
    { "tier4_4pc",                            OPT_INT,  &( gear.tier4_4pc                                 ) },
    { "tier5_2pc",                            OPT_INT,  &( gear.tier5_2pc                                 ) },
    { "tier5_4pc",                            OPT_INT,  &( gear.tier5_4pc                                 ) },
    { "tier6_2pc",                            OPT_INT,  &( gear.tier6_2pc                                 ) },
    { "tier6_4pc",                            OPT_INT,  &( gear.tier6_4pc                                 ) },
    { "tier7_2pc",                            OPT_INT,  &( gear.tier7_2pc                                 ) },
    { "tier7_4pc",                            OPT_INT,  &( gear.tier7_4pc                                 ) },
    { "tier8_2pc",                            OPT_INT,  &( gear.tier8_2pc                                 ) },
    { "tier8_4pc",                            OPT_INT,  &( gear.tier8_4pc                                 ) },
    // Player - Consumables                                                                                 
    { "flask",                                OPT_STRING, &( flask_str                                    ) },
    { "elixirs",                              OPT_STRING, &( elixirs_str                                  ) },
    { "food",                                 OPT_STRING, &( food_str                                     ) },
    // Deprecated
    { "enchant_attack_expertise_rating", OPT_DEPRECATED,  (void*) "enchant_expertise_rating"  },
    { "enchant_attack_penetration",      OPT_DEPRECATED,  (void*) "enchant_armor_penetration" },
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
