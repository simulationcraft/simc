// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

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

  virtual void trigger( action_t* a, void* call_data )
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

  virtual void trigger( action_t* a, void* call_data )
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
    bool mh = ( w -> slot == SLOT_MAIN_HAND );

    // Berserking has a 1.2 PPM (proc per minute) which translates into 1 proc every 50sec on average
    // We cannot use the base swing time because that would over-value haste.  Instead, we use
    // the hasted swing time which is represented in the "time_to_execute" field.  When this field
    // is zero, we are dealing with a "special" attack in which case the base swing time is used.


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

  virtual void trigger( action_t* a, void* call_data )
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
    bool mh = ( w -> slot == SLOT_MAIN_HAND );

    // Mongoose has a 1.2 PPM (proc per minute) which translates into 1 proc every 50sec on average
    // We cannot use the base swing time because that would over-value haste.  Instead, we use
    // the hasted swing time which is represented in the "time_to_execute" field.  When this field
    // is zero, we are dealing with a "special" attack in which case the base swing time is used.

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

  virtual void trigger( action_t* a, void* call_data )
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

// enchant_t::parse_option ==================================================

bool enchant_t::parse_option( player_t*          p,
                              const std::string& name,
                              const std::string& value )
{
  option_t options[] =
    {
      // @option_doc loc=player/all/enchant/stats title="Stat Enchants"
      { "enchant_strength",          OPT_FLT,  &( p -> enchant -> stats.attribute[ ATTR_STRENGTH  ] ) },
      { "enchant_agility",           OPT_FLT,  &( p -> enchant -> stats.attribute[ ATTR_AGILITY   ] ) },
      { "enchant_stamina",           OPT_FLT,  &( p -> enchant -> stats.attribute[ ATTR_STAMINA   ] ) },
      { "enchant_intellect",         OPT_FLT,  &( p -> enchant -> stats.attribute[ ATTR_INTELLECT ] ) },
      { "enchant_spirit",            OPT_FLT,  &( p -> enchant -> stats.attribute[ ATTR_SPIRIT    ] ) },
      { "enchant_spell_power",       OPT_FLT,  &( p -> enchant -> stats.spell_power                 ) },
      { "enchant_mp5",               OPT_FLT,  &( p -> enchant -> stats.mp5                         ) },
      { "enchant_attack_power",      OPT_FLT,  &( p -> enchant -> stats.attack_power                ) },
      { "enchant_expertise_rating",  OPT_FLT,  &( p -> enchant -> stats.expertise_rating            ) },
      { "enchant_armor_penetration", OPT_FLT,  &( p -> enchant -> stats.armor_penetration_rating    ) },
      { "enchant_armor",             OPT_FLT,  &( p -> enchant -> stats.armor                       ) },
      { "enchant_haste_rating",      OPT_FLT,  &( p -> enchant -> stats.haste_rating                ) },
      { "enchant_hit_rating",        OPT_FLT,  &( p -> enchant -> stats.hit_rating                  ) },
      { "enchant_crit_rating",       OPT_FLT,  &( p -> enchant -> stats.crit_rating                 ) },
      { "enchant_health",            OPT_FLT,  &( p -> enchant -> stats.resource[ RESOURCE_HEALTH ] ) },
      { "enchant_mana",              OPT_FLT,  &( p -> enchant -> stats.resource[ RESOURCE_MANA   ] ) },
      { "enchant_rage",              OPT_FLT,  &( p -> enchant -> stats.resource[ RESOURCE_RAGE   ] ) },
      { "enchant_energy",            OPT_FLT,  &( p -> enchant -> stats.resource[ RESOURCE_ENERGY ] ) },
      { "enchant_focus",             OPT_FLT,  &( p -> enchant -> stats.resource[ RESOURCE_FOCUS  ] ) },
      { "enchant_runic",             OPT_FLT,  &( p -> enchant -> stats.resource[ RESOURCE_RUNIC  ] ) },
      // @option_doc loc=player/all/enchant/procs title="Proc Enchants"
      { "spellsurge",                OPT_BOOL, &( p -> enchant -> spellsurge ) },
      { NULL, OPT_UNKNOWN }
    };

  if ( name.empty() )
  {
    option_t::print( p -> sim -> output_file, options );
    return false;
  }

  return option_t::parse( p -> sim, options, name, value );
}

// enchant_t::init ==========================================================

void enchant_t::init( player_t* p )
{}

// enchant_t::register_callbacks ============================================

void enchant_t::register_callbacks( player_t* p )
{
  if ( p -> main_hand_weapon.enchant == BERSERKING ||
       p ->  off_hand_weapon.enchant == BERSERKING )
  {
    p -> register_attack_result_callback( RESULT_HIT_MASK, new berserking_callback_t( p ) );
  }
  if ( p -> main_hand_weapon.enchant == EXECUTIONER ||
       p ->  off_hand_weapon.enchant == EXECUTIONER )
  {
    p -> register_attack_result_callback( RESULT_HIT_MASK, new executioner_callback_t( p ) );
  }
  if ( p -> main_hand_weapon.enchant == MONGOOSE ||
       p ->  off_hand_weapon.enchant == MONGOOSE )
  {
    p -> register_attack_result_callback( RESULT_HIT_MASK, new mongoose_callback_t( p ) );
  }
  if ( p -> enchant -> spellsurge )
  {
    p -> register_spell_result_callback( RESULT_ALL_MASK, new spellsurge_callback_t( p ) );
  }
}
