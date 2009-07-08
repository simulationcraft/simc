// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

struct enchant_data_t 
{ 
  std::string id, name, encoding;
};
static std::vector<enchant_data_t> enchant_db;
static void* enchant_mutex = 0;

namespace { // ANONYMOUS NAMESPACE =========================================

static void init_database()
{
  FILE* file = fopen( "permanentenchant.xml", "r" );
  xml_node_t* root = xml_t::create( file );
  if( ! root )
  {
    printf( "simcraft: Unable to open enchant database 'permanentenchant.xml'\n" );
    exit(0);
  }
  fclose( file );

  std::vector<xml_node_t*> nodes;
  int num_nodes = xml_t::get_nodes( nodes, root, "permanentenchant" );
  enchant_db.resize( num_nodes );

  for( int i=0; i < num_nodes; i++ )
  {
    xml_node_t* node = nodes[ i ];
    enchant_data_t& enchant = enchant_db[ i ];

    if( ! xml_t::get_value( enchant.id,   node, "id"   ) ||
	! xml_t::get_value( enchant.name, node, "name" ) )
    {
      printf( "simcraft: Unable to parse enchant database 'permanentenchant.xml'\n" );
      exit(0);
    }

    std::string stats;
    if( xml_t::get_value( stats, node, "stats/cdata" ) )
    {
      std::vector<std::string> splits;
      int num_splits = util_t::string_split( splits, stats, "," );

      for( int j=0; j < num_splits; j++ )
      {
	std::string abbrev, value;

	if( 2 == util_t::string_split( splits[ j ], ":", "S S", &abbrev, &value ) )
	{
	  int type = util_t::parse_stat_type( abbrev );
	  if( type != STAT_NONE )
	  {
	    if( j > 0 ) enchant.encoding += "_";
	    enchant.encoding += value;
	    enchant.encoding += util_t::stat_type_abbrev( type );
	  }
	}
      }
    }
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// Spellsurge Enchant =======================================================

struct spellsurge_callback_t : public action_callback_t
{
  spell_t* spell;
  rng_t* rng;

  spellsurge_callback_t( player_t* p ) : action_callback_t( p -> sim, p )
  {
    struct spellsurge_t : public spell_t
    {
      spellsurge_t( player_t* p ) :
          spell_t( "spellsurge", p, RESOURCE_MANA, SCHOOL_ARCANE )
      {
        background     = true;
        base_tick_time = 1.0;
        num_ticks      = 10;
        trigger_gcd    = 0;
        cooldown       = 60;
      }
      virtual void execute()
      {
        assert( current_tick == 0 );
        schedule_tick();
      }
      virtual void tick()
      {
        for ( player_t* p = sim -> player_list; p; p = p -> next )
        {
          if ( p -> party == player -> party )
          {
            if ( sim -> log ) log_t::output( sim, "Player %s gains mana from %s 's Spellsurge.", p -> name(), player -> name() );
            p -> resource_gain( RESOURCE_MANA, 10.0, p -> gains.spellsurge );
          }
        }
      }
    };

    spell = new spellsurge_t( p );
    rng = p -> get_rng( "spellsurge" );
  }

  virtual void trigger( action_t* a )
  {
    if ( spell -> ready() && rng -> roll( 0.15 ) )
    {
      for ( player_t* p = a -> sim -> player_list; p; p = p -> next )
      {
        // Invalidate any existing Spellsurge procs.

        if ( p -> party == a -> player -> party )
        {
          action_t* spellsurge = p -> find_action( "spellsurge" );

          if ( spellsurge && spellsurge -> ticking )
          {
            spellsurge -> cancel();
            break;
          }
        }
      }

      spell -> execute();
    }
  }
};

// Berserking Enchant =========================================================

struct berserking_callback_t : public action_callback_t
{
  int       mh_buff, oh_buff;
  event_t*  mh_expiration;
  event_t*  oh_expiration;
  uptime_t* mh_uptime;
  uptime_t* oh_uptime;
  rng_t*    mh_rng;
  rng_t*    oh_rng;

  berserking_callback_t( player_t* p ) : action_callback_t( p -> sim, p ), mh_buff( 0 ), oh_buff( 0 ), mh_expiration( 0 ), oh_expiration( 0 )
  {
    mh_uptime = p -> get_uptime( "berserking_mh" );
    oh_uptime = p -> get_uptime( "berserking_oh" );

    mh_rng = p -> get_rng( "berserking_mh", RNG_DISTRIBUTED );
    oh_rng = p -> get_rng( "berserking_oh", RNG_DISTRIBUTED );
  }

  virtual void reset() { mh_buff = oh_buff = 0; mh_expiration = oh_expiration = 0; }

  virtual void trigger( action_t* a )
  {
    struct berserking_expiration_t : public event_t
    {
      int&   buffs_berserking;
      event_t*& expirations_berserking;

      berserking_expiration_t( sim_t* sim, player_t* player, int& b_m, event_t*& e_m ) :
          event_t( sim, player ), buffs_berserking( b_m ), expirations_berserking( e_m )
      {
        name = "Berserking Expiration";
        player -> aura_gain( "Berserking" );
        player -> attack_power += 400;
        buffs_berserking = 1;
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        player -> aura_loss( "Berserking" );
        player -> attack_power -= 400;
        buffs_berserking = 0;
        expirations_berserking = 0;
      }
    };

    player_t* p = a -> player;
    weapon_t* w = a -> weapon;
    if ( ! w ) return;
    if( w -> enchant != ENCHANT_BERSERKING ) return;

    // Berserking has a 1.2 PPM (proc per minute) which translates into 1 proc every 50sec on average
    // We cannot use the base swing time because that would over-value haste.  Instead, we use
    // the hasted swing time which is represented in the "time_to_execute" field.  When this field
    // is zero, we are dealing with a "special" attack in which case the base swing time is used.

    bool mh = ( w -> slot == SLOT_MAIN_HAND );

    int&       b = mh ? mh_buff       : oh_buff;
    event_t*&  e = mh ? mh_expiration : oh_expiration;
    uptime_t*  u = mh ? mh_uptime     : oh_uptime;
    rng_t*     r = mh ? mh_rng        : oh_rng;

    double PPM = 1.2;
    double swing_time = a -> time_to_execute;

    if ( r -> roll( w -> proc_chance_on_swing( PPM, swing_time ) ) )
    {
      if ( e )
      {
        e -> reschedule( 15.0 );
      }
      else
      {
        e = new ( a -> sim ) berserking_expiration_t( a -> sim, p, b, e );
      }
    }

    u -> update( b != 0 );
  }
};

// Mongoose Enchant =========================================================

struct mongoose_callback_t : public action_callback_t
{
  event_t*  mh_expiration;
  event_t*  oh_expiration;
  uptime_t* mh_uptime;
  uptime_t* oh_uptime;
  rng_t*    mh_rng;
  rng_t*    oh_rng;

  mongoose_callback_t( player_t* p ) : action_callback_t( p -> sim, p ), mh_expiration( 0 ), oh_expiration( 0 )
  {
    mh_uptime = p -> get_uptime( "mongoose_mh" );
    oh_uptime = p -> get_uptime( "mongoose_oh" );

    mh_rng = p -> get_rng( "mongoose_mh", RNG_DISTRIBUTED );
    oh_rng = p -> get_rng( "mongoose_oh", RNG_DISTRIBUTED );
  }

  virtual void reset() { mh_expiration = oh_expiration = 0; }

  virtual void trigger( action_t* a )
  {
    struct mongoose_expiration_t : public event_t
    {
      int& buffs_mongoose;
      event_t*& expirations_mongoose;

      mongoose_expiration_t( sim_t* sim, player_t* player, int& b_m, event_t*& e_m ) :
          event_t( sim, player ), buffs_mongoose( b_m ), expirations_mongoose( e_m )
      {
        name = "Mongoose Expiration";
        player -> aura_gain( "Mongoose Lightning Speed" );
        player -> attribute[ ATTR_AGILITY ] += 120;
        buffs_mongoose = 1;
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        player -> aura_loss( "Mongoose Lightning Speed" );
        player -> attribute[ ATTR_AGILITY ] -= 120;
        buffs_mongoose = 0;
        expirations_mongoose = 0;
      }
    };

    player_t* p = a -> player;
    weapon_t* w = a -> weapon;
    if ( ! w ) return;
    if( w -> enchant != ENCHANT_MONGOOSE ) return;

    // Mongoose has a 1.2 PPM (proc per minute) which translates into 1 proc every 50sec on average
    // We cannot use the base swing time because that would over-value haste.  Instead, we use
    // the hasted swing time which is represented in the "time_to_execute" field.  When this field
    // is zero, we are dealing with a "special" attack in which case the base swing time is used.

    bool mh = ( w -> slot == SLOT_MAIN_HAND );

    int& b = mh ? p -> buffs.mongoose_mh : p -> buffs.mongoose_oh;

    event_t*&  e = mh ? mh_expiration : oh_expiration;
    uptime_t*  u = mh ? mh_uptime     : oh_uptime;
    rng_t*     r = mh ? mh_rng        : oh_rng;

    double PPM = 1.2 - ( ( std::max( p -> level, 70 ) - 70 ) * 0.02 );
    double swing_time = a -> time_to_execute;

    if ( r -> roll( w -> proc_chance_on_swing( PPM, swing_time ) ) )
    {
      if ( e )
      {
        e -> reschedule( 15.0 );
      }
      else
      {
        e = new ( a -> sim ) mongoose_expiration_t( a -> sim, p, b, e );
      }
    }

    u -> update( b != 0 );
  }
};

// Executioner Enchant =========================================================

struct executioner_callback_t : public action_callback_t
{
  int       buff;
  event_t*  expiration;
  uptime_t* uptime;
  rng_t*    rng;

  executioner_callback_t( player_t* p ) : action_callback_t( p -> sim, p ), buff( 0 ), expiration( 0 )
  {
    uptime = p -> get_uptime( "executioner" );
    rng    = p -> get_rng( "executioner", RNG_DISTRIBUTED );
  }

  virtual void reset() { buff = 0; expiration = 0; }

  virtual void trigger( action_t* a )
  {
    struct expiration_t : public event_t
    {
      executioner_callback_t* callback;
      expiration_t( sim_t* sim, player_t* player, executioner_callback_t* cb ) : event_t( sim, player ), callback( cb )
      {
        name = "Executioner Expiration";
        player -> aura_gain( "Executioner" );
        player -> attack_penetration += 120 / player -> rating.armor_penetration;
        callback -> buff = 1;
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        player -> aura_loss( "Executioner" );
        player -> attack_penetration -= 120 / player -> rating.armor_penetration;
        callback -> buff       = 0;
        callback -> expiration = 0;
      }
    };

    player_t* p = a -> player;
    weapon_t* w = a -> weapon;
    if ( ! w ) return;
    if( w -> enchant != ENCHANT_EXECUTIONER ) return;

    // Executioner has a 1.2 PPM (proc per minute) which translates into 1 proc every 50sec on average
    // We cannot use the base swing time because that would over-value haste.  Instead, we use
    // the hasted swing time which is represented in the "time_to_execute" field.  When this field
    // is zero, we are dealing with a "special" attack in which case the base swing time is used.

    double PPM = 1.2;
    double swing_time = a -> time_to_execute;

    if ( rng -> roll( w -> proc_chance_on_swing( PPM, swing_time ) ) )
    {
      if ( expiration )
      {
        expiration -> reschedule( 15.0 );
      }
      else
      {
        expiration = new ( a -> sim ) expiration_t( a -> sim, p, this );
      }
    }

    uptime -> update( buff != 0 );
  }
};

// ==========================================================================
// Enchant
// ==========================================================================

// enchant_t::init ==========================================================

void enchant_t::init( player_t* p )
{
  if( p -> is_pet() ) return;

  p -> main_hand_weapon.enchant = p -> items[ SLOT_MAIN_HAND ].enchant;
  p ->  off_hand_weapon.enchant = p -> items[ SLOT_OFF_HAND  ].enchant;
  p ->    ranged_weapon.enchant = p -> items[ SLOT_RANGED    ].enchant;

  if ( p -> ranged_weapon.enchant == ENCHANT_SCOPE )
  {
    p -> ranged_weapon.enchant_bonus = util_t::ability_rank( p -> level, 15.0,72,  12.0,67,  7.0,0 );
  }
}

// enchant_t::register_callbacks ============================================

void enchant_t::register_callbacks( player_t* p )
{
  if( p -> is_pet() ) return;

  if ( p -> main_hand_weapon.enchant == ENCHANT_BERSERKING ||
       p ->  off_hand_weapon.enchant == ENCHANT_BERSERKING )
  {
    p -> register_attack_result_callback( RESULT_HIT_MASK, new berserking_callback_t( p ) );
  }
  if ( p -> main_hand_weapon.enchant == ENCHANT_EXECUTIONER ||
       p ->  off_hand_weapon.enchant == ENCHANT_EXECUTIONER )
  {
    p -> register_attack_result_callback( RESULT_HIT_MASK, new executioner_callback_t( p ) );
  }
  if ( p -> main_hand_weapon.enchant == ENCHANT_MONGOOSE ||
       p ->  off_hand_weapon.enchant == ENCHANT_MONGOOSE )
  {
    p -> register_attack_result_callback( RESULT_HIT_MASK, new mongoose_callback_t( p ) );
  }
  if ( p -> main_hand_weapon.enchant == ENCHANT_SPELLSURGE ||
       p ->  off_hand_weapon.enchant == ENCHANT_SPELLSURGE )
  {
    p -> register_spell_result_callback( RESULT_ALL_MASK, new spellsurge_callback_t( p ) );
  }
}

// enchant_t::get_encoding ==================================================

bool enchant_t::get_encoding( std::string& name,
			      std::string& encoding,
			      const std::string& enchant_id )
{
  if( enchant_id.empty() || enchant_id == "" || enchant_id == "0" ) 
    return false;

  bool success = false;
  thread_t::mutex_lock( enchant_mutex );

  if( enchant_db.empty() ) init_database();

  int num_enchants = enchant_db.size();
  for( int i = num_enchants-1; i >= 0; i-- )
  {
    enchant_data_t& enchant = enchant_db[ i ];

    if( enchant.id == enchant_id )
    {
      name     = enchant.name;
      encoding = enchant.encoding;
      success  = true;
      break;
    }
  }
  
  thread_t::mutex_unlock( enchant_mutex );
  return success;
}

