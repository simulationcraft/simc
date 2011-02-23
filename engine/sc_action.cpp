// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Action
// ==========================================================================

// action_t::action_t =======================================================

void action_t::_init_action_t()
{
  sim                            = s_player->sim;
  name_str                       = s_token;
  player                         = s_player;
  target                         = s_player -> sim -> target;
  id                             = 0;
  result                         = RESULT_NONE;
  aoe                            = 0;
  dual                           = false;
  callbacks                      = true;
  binary                         = false;
  channeled                      = false;
  background                     = false;
  sequence                       = false;
  direct_tick                    = false;
  repeating                      = false;
  harmful                        = true;
  proc                           = false;
  auto_cast                      = false;
  may_hit                        = true;
  may_miss                       = false;
  may_resist                     = false;
  may_dodge                      = false;
  may_parry                      = false;
  may_glance                     = false;
  may_block                      = false;
  may_crush                      = false;
  may_crit                       = false;
  tick_may_crit                  = false;
  tick_zero                      = false;
  hasted_ticks                   = false;
  dot_behavior                   = DOT_CLIP;
  rp_gain                        = 0.0;
  min_gcd                        = 0.0;
  trigger_gcd                    = player -> base_gcd;
  range                          = -1.0;
  weapon_power_mod               = 1.0/14.0;
  direct_power_mod               = 0.0;
  tick_power_mod                 = 0.0;
  base_execute_time              = 0.0;
  base_tick_time                 = 0.0;
  base_cost                      = 0.0;
  base_dd_min                    = 0.0;
  base_dd_max                    = 0.0;
  base_td                        = 0.0;
  base_td_init                   = 0.0;
  base_td_multiplier             = 1.0;
  base_dd_multiplier             = 1.0;
  base_multiplier                = 1.0;
  base_hit                       = 0.0;
  base_crit                      = 0.0;
  base_penetration               = 0.0;
  player_multiplier              = 1.0;
  player_td_multiplier           = 1.0;
  player_dd_multiplier           = 1.0;
  player_hit                     = 0.0;
  player_crit                    = 0.0;
  player_penetration             = 0.0;
  rp_gain                        = 0.0;
  target_multiplier              = 1.0;
  target_hit                     = 0.0;
  target_crit                    = 0.0;
  target_penetration             = 0.0;
  base_spell_power               = 0.0;
  base_attack_power              = 0.0;
  player_spell_power             = 0.0;
  player_attack_power            = 0.0;
  target_spell_power             = 0.0;
  target_attack_power            = 0.0;
  base_spell_power_multiplier    = 0.0;
  base_attack_power_multiplier   = 0.0;
  player_spell_power_multiplier  = 1.0;
  player_attack_power_multiplier = 1.0;
  base_crit_multiplier           = 1.0;
  base_crit_bonus_multiplier     = 1.0;
  player_crit_multiplier         = 1.0;
  player_crit_bonus_multiplier   = 1.0;
  target_crit_multiplier         = 1.0;
  target_crit_bonus_multiplier   = 1.0;
  base_dd_adder                  = 0.0;
  player_dd_adder                = 0.0;
  target_dd_adder                = 0.0;
  player_haste                   = 1.0;
  resource_consumed              = 0.0;
  direct_dmg                     = 0.0;
  tick_dmg                       = 0.0;
  num_ticks                      = 0;
  weapon                         = NULL;
  weapon_multiplier              = 1.0;
  base_add_multiplier            = 1.0;
  normalize_weapon_damage        = false;
  normalize_weapon_speed         = false;
  rng_travel                     = NULL;
  stats                          = NULL;
  execute_event                  = NULL;
  travel_event                   = NULL;
  time_to_execute                = 0.0;
  time_to_tick                   = 0.0;
  time_to_travel                 = 0.0;
  travel_speed                   = 0.0;
  rank_index                     = -1;
  bloodlust_active               = 0;
  max_haste                      = 0.0;
  haste_gain_percentage          = 0.0;
  min_current_time               = 0.0;
  max_current_time               = 0.0;
  min_time_to_die                = 0.0;
  max_time_to_die                = 0.0;
  min_health_percentage          = 0.0;
  max_health_percentage          = 0.0;
  moving                         = -1;
  vulnerable                     = 0;
  invulnerable                   = 0;
  wait_on_ready                  = -1;
  round_base_dmg                 = true;
  if_expr_str                    = "";
  if_expr                        = NULL;
  sync_str                       = "";
  sync_action                    = NULL;
  next                           = NULL;
  marker                         = 0;
  target_str                     = "";
 
  if ( sim -> debug ) log_t::output( sim, "Player %s creates action %s", player -> name(), name() );

  if ( ! player -> initialized )
  {
    sim -> errorf( "Actions must not be created before player_t::init().  Culprit: %s %s\n", player -> name(), name() );
    sim -> cancel();
  }

  action_t** last = &( player -> action_list );
  while ( *last ) last = &( ( *last ) -> next );
  *last = this;

  for ( int i=0; i < RESULT_MAX; i++ ) rng[ i ] = 0;


  cooldown = player -> get_cooldown( name_str );
  dot      = player -> get_dot     ( name_str );

  stats = player -> get_stats( name_str );
  stats -> school = school;
  stats -> resource = resource;
  
  id = spell_id();
  tree = util_t::talent_tree(s_tree, player -> type );

  parse_data ( player -> player_data );

  if ( id && player -> player_data.spell_exists(id) && ! player -> player_data.spell_is_level( id, player -> level ) && player -> player_data.spell_level(id) <= MAX_LEVEL)
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required level (%d < %d).\n",
                   player -> name(), name(), player -> level, player -> player_data.spell_level(id) );

    background = true; // prevent action from being executed
  }


}

action_t::action_t( int               ty,
                    const char*       n,
                    player_t*         p,
                    int               r,
                    const school_type s,
                    int               tr,
                    bool              sp ) :
  spell_id_t( p, n ),
  sim( s_player->sim ), type( ty ), name_str( s_token ), 
  player( s_player ), target( s_player -> sim -> target ), school( s ), resource( r ), 
  tree( tr ), special( sp )
{
  _init_action_t();
}

action_t::action_t( int ty, const char* name, const char* sname, player_t* p, int t, bool sp ) :
  spell_id_t( p, name, sname ),
  sim( s_player->sim ), type( ty ), name_str( s_token ), 
  player( s_player ), target( s_player -> sim -> target ), school( get_school_type() ), resource( power_type() ), 
  tree( t ), special( sp )
{
  _init_action_t();
}

action_t::action_t( int ty, const active_spell_t& s, int t, bool sp ) :
  spell_id_t( s ), 
  sim( s_player->sim ), type( ty ), name_str( s_token ), 
  player( s_player ), target( s_player -> sim -> target ), school( get_school_type() ), resource( power_type() ), 
  tree( t ), special( sp )
{
  _init_action_t();
}

action_t::action_t( int type, const char* name, const uint32_t id, player_t* p, int t, bool sp ) :
  spell_id_t( p, name, id ),
  sim( s_player->sim ), type( type ), name_str( s_token ), 
  player( s_player ), target( s_player -> sim -> target ), school( get_school_type() ), resource( power_type() ), 
  tree( t ), special( sp )
{
  _init_action_t();
}

action_t::~action_t()
{
  if ( if_expr )
    delete if_expr;
}

// action_t::parse_data ====================================================

void action_t::parse_data( sc_data_access_t& pData )
{
  if ( pData.spell_exists(id) )
  {
    base_execute_time    = pData.spell_cast_time ( id, player -> level );
    cooldown -> duration = pData.spell_cooldown ( id );
    if ( cooldown -> duration > ( sim -> wheel_seconds - 2.0 ) )
      cooldown -> duration = sim -> wheel_seconds - 2.0;
    range                = pData.spell_max_range ( id );
    travel_speed         = pData.spell_missile_speed ( id );
    trigger_gcd          = pData.spell_gcd ( id );
    school               = spell_id_t::get_school_type( pData.spell_school_mask( id ) );
    stats -> school      = school;
    resource             = pData.spell_power_type( id );
    rp_gain              = pData.spell_runic_power_gain( id );

    // For mana it returns the % of base mana, not the absolute cost
    if ( resource == RESOURCE_MANA )
      base_cost = floor ( pData.spell_cost( id ) * player -> resource_base[ RESOURCE_MANA ] );
    else
      base_cost = pData.spell_cost( id );

    for ( int i=1; i <= MAX_EFFECTS; i++)
    {
      parse_effect_data(pData, id, i);
    }
  }
}

// action_t::parse_effect_data ==============================================
void action_t::parse_effect_data( sc_data_access_t& pData, int spell_id, int effect_nr )
{
  if (!spell_id)
    return;

  assert(pData.spell_exists(spell_id));

  int effect = pData.spell_effect_id ( spell_id, effect_nr );
  if (pData.effect_exists ( effect ) )
  {
    switch ( pData.effect_type ( effect) )
    {
      // Direct Damage
    case E_HEAL:
    case E_SCHOOL_DAMAGE:
      direct_power_mod = pData.effect_coeff( effect );
      base_dd_min      = pData.effect_min ( effect, pData.spell_scaling_class( spell_id ), player -> level );
      base_dd_max      = pData.effect_max ( effect, pData.spell_scaling_class( spell_id ), player -> level );
      break;

    case E_NORMALIZED_WEAPON_DMG:
      normalize_weapon_speed = true;
    case E_WEAPON_DAMAGE:         
      base_dd_min      = pData.effect_min ( effect, pData.spell_scaling_class( spell_id ), player -> level );
      base_dd_max      = pData.effect_max ( effect, pData.spell_scaling_class( spell_id ), player -> level );
      weapon = &( player -> main_hand_weapon );
      break;

    case E_WEAPON_PERCENT_DAMAGE:
      weapon = &( player -> main_hand_weapon );
      weapon_multiplier = pData.effect_min ( effect, pData.spell_scaling_class( spell_id ), player -> level );
      break;

      // Dot
    case E_APPLY_AURA:
      switch ( pData.effect_subtype ( effect) )
      {
      case A_PERIODIC_DAMAGE:
        tick_power_mod   = pData.effect_coeff( effect );
        base_td_init     = pData.effect_average ( effect, pData.spell_scaling_class( spell_id ), player -> level );
        base_td          = base_td_init;
        base_tick_time   = pData.effect_period ( effect );
        num_ticks        = (int) ( pData.spell_duration ( spell_id ) / base_tick_time );
        if ( school == SCHOOL_PHYSICAL )
          school = stats -> school = SCHOOL_BLEED;
        break;
      case A_PERIODIC_LEECH:
        tick_power_mod   = pData.effect_coeff( effect );
        base_td_init     = pData.effect_min ( effect, pData.spell_scaling_class( spell_id ), player -> level );
        base_td          = base_td_init;
        base_tick_time   = pData.effect_period ( effect );
        num_ticks        = (int) ( pData.spell_duration ( spell_id ) / base_tick_time );
        break;
      case A_PERIODIC_TRIGGER_SPELL:
        base_tick_time   = pData.effect_period ( effect );
        num_ticks        = (int) ( pData.spell_duration ( spell_id ) / base_tick_time );
        break;
      case A_SCHOOL_ABSORB:
        direct_power_mod   = pData.effect_coeff( effect );
        base_dd_min      = pData.effect_min ( effect, pData.spell_scaling_class( spell_id ), player -> level );
        base_dd_max      = pData.effect_max ( effect, pData.spell_scaling_class( spell_id ), player -> level );
        break;
      case A_PERIODIC_HEAL:
        tick_power_mod   = pData.effect_coeff( effect );
        base_td_init     = pData.effect_min ( effect, pData.spell_scaling_class( spell_id ), player -> level );
        base_td          = base_td_init;
        base_tick_time   = pData.effect_period ( effect );
        num_ticks        = (int) ( pData.spell_duration ( spell_id ) / base_tick_time );
        break;
      case A_ADD_FLAT_MODIFIER:
        switch ( pData.effect_misc_value1( effect ) )
        {
        case P_CRIT:
          base_crit += 0.01 * pData.effect_base_value( effect );
          break;
        case P_COOLDOWN:
          cooldown -> duration += 0.001 * pData.effect_base_value( effect );
          break;
        }
        break;
      case A_ADD_PCT_MODIFIER:
        switch ( pData.effect_misc_value1( effect ) )
        {
        case P_RESOURCE_COST:
          base_cost *= 1 + 0.01 * pData.effect_base_value( effect );
          break;
        }
        break;
      }
      break;
    }
  }
}

// action_t::merge_options ==================================================

option_t* action_t::merge_options( std::vector<option_t>& merged_options,
                                   option_t*              options1,
                                   option_t*              options2 )
{
  merged_options.clear();

  if ( options1 )
  {
    for ( int i=0; options1[ i ].name; i++ ) merged_options.push_back( options1[ i ] );
  }

  if ( options2 )
  {
    for ( int i=0; options2[ i ].name; i++ ) merged_options.push_back( options2[ i ] );
  }

  option_t null_option;
  null_option.name = 0;
  merged_options.push_back( null_option );

  return &( merged_options[ 0 ] );
}

// action_t::parse_options =================================================

void action_t::parse_options( option_t*          options,
                              const std::string& options_str )
{
  option_t base_options[] =
  {
    { "bloodlust",              OPT_BOOL,   &bloodlust_active      },
    { "haste<",                 OPT_FLT,    &max_haste             },
    { "haste_gain_percentage>", OPT_FLT,    &haste_gain_percentage },
    { "health_percentage<",     OPT_FLT,    &max_health_percentage },
    { "health_percentage>",     OPT_FLT,    &min_health_percentage },
    { "if",                     OPT_STRING, &if_expr_str           },
    { "invulnerable",           OPT_BOOL,   &invulnerable          },
    { "moving",                 OPT_BOOL,   &moving                },
    { "rank",                   OPT_INT,    &rank_index            },
    { "sync",                   OPT_STRING, &sync_str              },
    { "time<",                  OPT_FLT,    &max_current_time      },
    { "time>",                  OPT_FLT,    &min_current_time      },
    { "time_to_die<",           OPT_FLT,    &max_time_to_die       },
    { "time_to_die>",           OPT_FLT,    &min_time_to_die       },
    { "travel_speed",           OPT_FLT,    &travel_speed          },
    { "vulnerable",             OPT_BOOL,   &vulnerable            },
    { "wait_on_ready",          OPT_BOOL,   &wait_on_ready         },
    { "target",                 OPT_STRING, &target_str            },
    { NULL,                     0,          NULL                   }
  };

  std::vector<option_t> merged_options;
  merge_options( merged_options, options, base_options );

  std::string::size_type cut_pt = options_str.find_first_of( ":" );

  std::string options_buffer;
  if ( cut_pt != options_str.npos )
  {
    options_buffer = options_str.substr( cut_pt + 1 );
  }
  else options_buffer = options_str;

  if ( options_buffer.empty()     ) return;
  if ( options_buffer.size() == 0 ) return;

  if ( ! option_t::parse( sim, name(), merged_options, options_buffer ) )
  {
    sim -> errorf( "%s %s: Unable to parse options str '%s'.\n", player -> name(), name(), options_str.c_str() );
    sim -> cancel();
  }

  // FIXME: Move into constructor when parse_action is called from there.
  if ( ! target_str.empty() )
  {
    player_t* p = sim -> find_player( target_str );

    if ( p )
      target = p;
  }
}

// action_t::init_rank ======================================================

rank_t* action_t::init_rank( rank_t* rank_list,
                             int     id_override )
{
  if ( resource == RESOURCE_MANA )
  {
    for ( int i=0; rank_list[ i ].level; i++ )
    {
      rank_t& r = rank_list[ i ];

      // Look for ranks in which the cost of an action is a percentage of base mana
      if ( r.cost > 0 && r.cost < 1 )
      {
        r.cost *= player -> resource_base[ RESOURCE_MANA ];
      }
    }
  }

  for ( int i=0; rank_list[ i ].level; i++ )
  {
    if ( ( rank_index <= 0 && player -> level >= rank_list[ i ].level ) ||
         ( rank_index >  0 &&      rank_index == rank_list[ i ].index  ) )
    {
      rank_t* rank = &( rank_list[ i ] );

      base_dd_min  = rank -> dd_min;
      base_dd_max  = rank -> dd_max;
      base_td_init = rank -> tick;
      base_td      = base_td_init;
      base_cost    = rank -> cost;

      if (id_override) id = id_override;

      return rank;
    }
  }

  sim -> errorf( "%s unable to find valid rank for %s\n", player -> name(), name() );
  sim -> cancel();

  return 0;
}

// action_t::cost ======================================================

double action_t::cost() SC_CONST
{
  if ( ! harmful && ! player -> in_combat )
    return 0;

  double c = base_cost;

  c -= player -> resource_reduction[ school ];
  if ( c < 0 ) c = 0;

  if ( resource == RESOURCE_MANA )
  {
    if ( player -> buffs.power_infusion -> check() ) c *= 0.80;
  }

  if ( sim -> debug ) log_t::output( sim, "action_t::cost: %s %.2f %.2f %s", name(), base_cost, c, util_t::resource_type_string( resource ) );

  return floor( c );
}

// action_t::gcd =============================================================

double action_t::gcd() SC_CONST
{
  if ( ! harmful && ! player -> in_combat )
    return 0;

  return trigger_gcd;
}

// action_t::travel_time =====================================================

double action_t::travel_time()
{
  if ( travel_speed == 0 ) return 0;

  if ( player -> distance == 0 ) return 0;

  double t = player -> distance / travel_speed;

  double v = sim -> travel_variance;

  if ( v )
  {
    if ( ! rng_travel )
    {
      std::string buffer = name_str + "_travel";
      rng_travel = player -> get_rng( buffer, RNG_DISTRIBUTED );
    }
    t = rng_travel -> gauss( t, v );
  }

  return t;
}

// action_t::player_buff =====================================================

void action_t::player_buff()
{
  player_multiplier              = 1.0;
  player_dd_multiplier           = 1.0;
  player_td_multiplier           = 1.0;
  player_hit                     = 0;
  player_crit                    = 0;
  player_crit_multiplier         = 1.0;
  player_crit_bonus_multiplier   = 1.0;
  player_penetration             = 0;
  player_dd_adder                = 0;
  player_spell_power             = 0;
  player_attack_power            = 0;
  player_spell_power_multiplier  = 1.0;
  player_attack_power_multiplier = 1.0;

  player_t* p = player;

  if ( school == SCHOOL_BLEED )
  {
    // Not applicable
  }

  if ( school == SCHOOL_PHYSICAL )
  {
    if ( p -> debuffs.demoralizing_roar  -> up() ||
         p -> debuffs.demoralizing_shout -> up() ||
         p -> debuffs.scarlet_fever      -> up() ||
         p -> debuffs.vindication        -> up() )
    {
      player_multiplier *= 0.90;
    }
  }

  else if ( school != SCHOOL_PHYSICAL )
  {
    player_penetration = p -> composite_spell_penetration();
  }

  player_multiplier    = p -> composite_player_multiplier   ( school );
  player_dd_multiplier = p -> composite_player_dd_multiplier( school );
  player_td_multiplier = p -> composite_player_td_multiplier( school );

  if ( base_attack_power_multiplier > 0 )
  {
    player_attack_power            = p -> composite_attack_power();
    player_attack_power_multiplier = p -> composite_attack_power_multiplier();
  }

  if ( base_spell_power_multiplier > 0 )
  {
    player_spell_power            = p -> composite_spell_power( school );
    player_spell_power_multiplier = p -> composite_spell_power_multiplier();
  }

  player_haste = haste();

  if ( sim -> debug )
    log_t::output( sim, "action_t::player_buff: %s hit=%.2f crit=%.2f penetration=%.0f spell_power=%.2f attack_power=%.2f ",
                   name(), player_hit, player_crit, player_penetration, player_spell_power, player_attack_power );
}

// action_t::target_debuff ==================================================

void action_t::target_debuff( player_t* t, int dmg_type )
{
  target_multiplier            = 1.0;
  target_hit                   = 0;
  target_crit                  = 0;
  target_crit_multiplier       = 1.0;
  target_crit_bonus_multiplier = 1.0;
  target_attack_power          = 0;
  target_spell_power           = 0;
  target_penetration           = 0;
  target_dd_adder              = 0;


  if ( school == SCHOOL_PHYSICAL ||
       school == SCHOOL_BLEED    )
  {
    if ( t -> debuffs.savage_combat -> up() )
    {
      target_multiplier *= 1.04;
    }
    else if ( t -> debuffs.blood_frenzy_physical -> value() || t -> debuffs.brittle_bones -> value() )
    {
      target_multiplier *= 1.0 + std::max( t -> debuffs.blood_frenzy_physical -> value() * 0.01,
                                           t -> debuffs.brittle_bones         -> value() );
    }
  }
  else
  {
    target_multiplier *= 1.0 + ( std::max( t -> debuffs.curse_of_elements  -> value(), 
                                 std::max( t -> debuffs.earth_and_moon     -> value(),
                                 std::max( t -> debuffs.ebon_plaguebringer -> value(),
                                           t -> debuffs.lightning_breath   -> value() ) ) ) * 0.01 );

    if ( t -> debuffs.curse_of_elements -> check() ) target_penetration += 88;
  }

  if ( school == SCHOOL_BLEED )
  {
    if ( t -> debuffs.mangle -> up() || t -> debuffs.hemorrhage -> up() )
    {
      target_multiplier *= 1.30;
    }
    else if ( t -> debuffs.blood_frenzy_bleed -> value() )
    {
      target_multiplier *= 1.0 + t -> debuffs.blood_frenzy_bleed -> value() * 0.01;
    }
  }

  if ( base_attack_power_multiplier > 0 )
  {
    bool ranged = ( player -> position == POSITION_RANGED_FRONT || 
		    player -> position == POSITION_RANGED_BACK );

    if ( ranged )
    {
      target_attack_power += t -> debuffs.hunters_mark -> value();
    }
  }
  if ( base_spell_power_multiplier > 0 )
  {
    // no spell power based debuffs at this time
  }

  if ( t -> debuffs.vulnerable -> up() ) target_multiplier *= t -> debuffs.vulnerable -> value();

  if ( target -> is_enemy())
  {
    target_t* t = target -> cast_target();

    if ( t -> resilience > 0 )
    {
      double percent_resil = t -> resilience / 94.3;
      double damage_reduce = percent_resil * 2 * 0.01;
      target_multiplier *= 1 - damage_reduce;
    }
  }

  if ( sim -> debug )
    log_t::output( sim, "action_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f penetration=%.0f",
                   name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power, target_penetration );
}

// action_t::result_is_hit ==================================================

bool action_t::result_is_hit( int r ) SC_CONST
{
  if ( r == RESULT_UNKNOWN ) r = result;

  return( r == RESULT_HIT        ||
          r == RESULT_CRIT       ||
          r == RESULT_GLANCE     ||
          r == RESULT_BLOCK      ||
          r == RESULT_CRIT_BLOCK ||
          r == RESULT_NONE       );
}

// action_t::result_is_miss =================================================

bool action_t::result_is_miss( int r ) SC_CONST
{
  if ( r == RESULT_UNKNOWN ) r = result;

  return( r == RESULT_MISS   ||
          r == RESULT_DODGE  ||
          r == RESULT_PARRY  ||
          r == RESULT_RESIST );
}

// action_t::armor ==========================================================

double action_t::armor() SC_CONST
{
  player_t* t = target;

  double adjusted_armor;

  adjusted_armor = target -> composite_armor();

  double armor_reduction = std::max( t -> debuffs.sunder_armor -> stack() * 0.04,
                           std::max( t -> debuffs.faerie_fire  -> stack() * t -> debuffs.faerie_fire -> value(),
                                     t -> debuffs.expose_armor -> value() ) );
  // TO-DO: Also need to add the Hunter Pets Raptor and Serpent

  armor_reduction += t -> debuffs.shattering_throw -> stack() * 0.20;

  adjusted_armor *= 1.0 - armor_reduction;

  return adjusted_armor;
}

// action_t::resistance =====================================================

double action_t::resistance() SC_CONST
{
  if ( ! may_resist ) return 0;
  
  player_t* t = target;
  double resist=0;

  double penetration = base_penetration + player_penetration + target_penetration;

  if ( school == SCHOOL_BLEED )
  {
    // Bleeds cannot be resisted
  }
  else if ( school == SCHOOL_PHYSICAL )
  {
    double temp_armor = armor();
    resist = temp_armor / ( temp_armor + player -> armor_coeff );

    if ( resist < 0.0 )
      resist = 0.0;
    else if ( resist > 0.75 )
      resist = 0.75;
  }
  else
  {
    double resist_rating = t -> spell_resistance[ school ];

    if ( school == SCHOOL_FROSTFIRE )
    {
      resist_rating = std::min( t -> spell_resistance[ SCHOOL_FROST ],
                                t -> spell_resistance[ SCHOOL_FIRE  ] );
    }
    else if ( school == SCHOOL_SPELLSTORM )
    {
      resist_rating = std::min( t -> spell_resistance[ SCHOOL_ARCANE ],
                                t -> spell_resistance[ SCHOOL_NATURE ] );
    }
    else if ( school == SCHOOL_SHADOWFROST )
    {
      resist_rating = std::min( t -> spell_resistance[ SCHOOL_SHADOW ],
                                t -> spell_resistance[ SCHOOL_FROST ] );
    }
    else if ( school == SCHOOL_SHADOWFLAME )
    {
      resist_rating = std::min( t -> spell_resistance[ SCHOOL_SHADOW ],
                                t -> spell_resistance[ SCHOOL_FIRE ] );
    }

    resist_rating -= penetration;
    if ( resist_rating < 0 ) resist_rating = 0;
    if ( resist_rating > 0 )
    {
      resist = resist_rating / ( resist_rating + player -> half_resistance_rating );
    }

#if 0
// TO-DO: No sign of partial resists on either Beta or PTR. ifdefing out for now in case they come back...
    if ( ! binary )
    {
      int delta_level = t -> level - player -> level;
      if ( delta_level > 0 )
      {
        resist += delta_level * 0.02;
      }
    }
#endif

    if ( resist > 1.0 ) resist = 1.0;
  }

  if ( sim -> debug ) log_t::output( sim, "%s queries resistance for %s: %.2f", player -> name(), name(), resist );

  return resist;
}

// action_t::total_crit_bonus ================================================

double action_t::total_crit_bonus() SC_CONST
{
  double crit_multiplier = (   base_crit_multiplier *
                             player_crit_multiplier *
                             target_crit_multiplier );

  double crit_bonus_multiplier = (   base_crit_bonus_multiplier *
                                   player_crit_bonus_multiplier *
                                   target_crit_bonus_multiplier );

  double crit_bonus = ( ( 1.0 + base_crit_bonus ) * crit_multiplier - 1.0 ) * crit_bonus_multiplier;

  if ( sim -> debug )
  {
    log_t::output( sim, "%s crit_bonus for %s: cb=%.3f b_cb=%.2f b_cm=%.2f p_cm=%.2f t_cm=%.2f b_cbm=%.2f p_cbm=%.2f t_cbm=%.2f",
                   player -> name(), name(), crit_bonus, base_crit_bonus,
                   base_crit_multiplier,       player_crit_multiplier,       target_crit_multiplier,
                   base_crit_bonus_multiplier, player_crit_bonus_multiplier, target_crit_bonus_multiplier );
  }

  return crit_bonus;
}

// action_t::total_power =====================================================

double action_t::total_power() SC_CONST
{
  double power=0;

  if ( base_spell_power_multiplier  > 0 ) power += total_spell_power();
  if ( base_attack_power_multiplier > 0 ) power += total_attack_power();

  return power;
}

// action_t::calculate_weapon_damage =========================================

double action_t::calculate_weapon_damage()
{
  if ( ! weapon || weapon_multiplier <= 0 ) return 0;

  double dmg = sim -> range( weapon -> min_dmg, weapon -> max_dmg ) + weapon -> bonus_dmg;

  double weapon_damage = normalize_weapon_damage ? dmg * 2.8 / weapon -> swing_time : dmg;
  double weapon_speed  = normalize_weapon_speed  ? weapon -> normalized_weapon_speed() : weapon -> swing_time;

  double power_damage = weapon_speed * weapon_power_mod * total_attack_power();

  double total_dmg = weapon_damage + power_damage;
    
  // OH penalty
  if ( weapon -> slot == SLOT_OFF_HAND )
    total_dmg *= 0.5;

  if ( sim -> debug )
  {
    log_t::output( sim, "%s weapon damage for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f pd=%.3f ap=%.3f",
		   player -> name(), name(), total_dmg, weapon_damage, weapon -> bonus_dmg, weapon_speed, power_damage, total_attack_power() );
  }

  return total_dmg;
}

// action_t::calculate_tick_damage ===========================================

double action_t::calculate_tick_damage()
{
  double dmg = 0;

  if ( base_td == 0 ) base_td = base_td_init;

  if ( base_td == 0 && tick_power_mod == 0 ) return 0;

  dmg  = floor( base_td + 0.5 ) + total_power() * tick_power_mod;
  dmg *= total_td_multiplier();

  double init_tick_dmg = dmg;

  if ( result == RESULT_CRIT )
  {
    dmg *= 1.0 + total_crit_bonus();
  }

  if ( ! binary )
  {
    dmg *= 1.0 - resistance();
  }

  if ( ! sim -> average_range ) dmg = floor( dmg + sim -> real() );

  if ( sim -> debug )
  {
    log_t::output( sim, "%s dmg for %s: td=%.0f i_td=%.0f b_td=%.0f mod=%.2f power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f",
                   player -> name(), name(), tick_dmg, init_tick_dmg, base_td, tick_power_mod,
                   total_power(), base_multiplier * base_td_multiplier, player_multiplier, target_multiplier );
  }

  return dmg;
}

// action_t::calculate_direct_damage =========================================

double action_t::calculate_direct_damage()
{
  double dmg = sim -> range( base_dd_min, base_dd_max );

  if ( round_base_dmg ) dmg = floor( dmg + 0.5 );

  if ( dmg == 0 && weapon_multiplier == 0 && direct_power_mod == 0 ) return 0;

  double base_direct_dmg = dmg;
  double weapon_dmg = 0;
  
  dmg += base_dd_adder + player_dd_adder + target_dd_adder;

  if ( weapon_multiplier > 0 )
  {
    // x% weapon damage + Y
    // e.g. Obliterate, Shred, Backstab
    dmg += calculate_weapon_damage();
    dmg *= weapon_multiplier;
    weapon_dmg = dmg;
  }
  dmg += direct_power_mod * total_power();
  dmg *= total_dd_multiplier();

  double init_direct_dmg = dmg;

  if ( result == RESULT_GLANCE )
  {
    double delta_skill = ( target -> level - player -> level ) * 5.0;

    if ( delta_skill < 0.0 )
      delta_skill = 0.0;

    double max_glance = 1.3 - 0.03 * delta_skill;

    if ( max_glance > 0.99 ) 
      max_glance = 0.99;
    else if ( max_glance < 0.2 )
      max_glance = 0.20;

    double min_glance = 1.4 - 0.05 * delta_skill;

    if ( min_glance > 0.91 )
      min_glance = 0.91;
    else if ( min_glance < 0.01 )
      min_glance = 0.01;

    if ( min_glance > max_glance )
    {
      double temp = min_glance;
      min_glance = max_glance;
      max_glance = temp;
    }

    dmg *= sim -> range( min_glance, max_glance ); // 0.75 against +3 targets.
  }
  else if ( result == RESULT_CRIT )
  {
    dmg *= 1.0 + total_crit_bonus();
  }

  if ( ! binary )
  {
    dmg *= 1.0 - resistance();
  }

  if ( result == RESULT_BLOCK )
  {
    dmg *= ( 1 - 0.3 );
    if ( dmg < 0 ) dmg = 0;
  }

  if ( result == RESULT_CRIT_BLOCK )
  {
    dmg *= ( 1 - 0.6 );
    if ( dmg < 0 ) dmg = 0;
  }

  if ( ! sim -> average_range ) dmg = floor( dmg + sim -> real() );

  if ( sim -> debug )
  {
    log_t::output( sim, "%s dmg for %s: dd=%.0f i_dd=%.0f w_dd=%.0f b_dd=%.0f mod=%.2f power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f w_mult=%.2f",
                   player -> name(), name(), dmg, init_direct_dmg, weapon_dmg, base_direct_dmg, direct_power_mod,
                   total_power(), base_multiplier * base_dd_multiplier, player_multiplier, target_multiplier, weapon_multiplier );
  }

  return dmg;
}

// action_t::consume_resource ===============================================

void action_t::consume_resource()
{
  if( resource == RESOURCE_NONE ) return;

  resource_consumed = cost();

  if ( sim -> debug )
    log_t::output( sim, "%s consumes %.1f %s for %s", player -> name(),
                   resource_consumed, util_t::resource_type_string( resource ), name() );

  player -> resource_loss( resource, resource_consumed, this );

  stats -> consume_resource( resource_consumed );
}

// action_t::execute ========================================================

void action_t::execute()
{
  if ( sim -> log && ! dual ) 
  {
    log_t::output( sim, "%s performs %s (%.0f)", player -> name(), name(), 
                   player -> resource_current[ player -> primary_resource() ] );
  }

  if ( harmful )
  {
    if ( player -> in_combat == false && sim -> debug )
      log_t::output( sim, "%s enters combat.", player -> name() );

    player -> in_combat = true;
  }

  player_buff();

  target_debuff( target, DMG_DIRECT );

  calculate_result();

  consume_resource();

  if ( result_is_hit() )
  {
    direct_dmg = calculate_direct_damage();
  }

  schedule_travel( target );

  update_ready();

  if ( ! dual ) stats -> add_execute( time_to_execute );

  if ( repeating && ! proc ) schedule_execute();
}

// action_t::tick ===========================================================

void action_t::tick()
{
  if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), dot -> current_tick, dot -> num_ticks );

  result = RESULT_HIT;

  player_tick();

  target_debuff( target, DMG_OVER_TIME );

  if ( tick_may_crit )
  {
    int delta_level = target -> level - player -> level;
    
    if ( rng[ RESULT_CRIT ] -> roll( crit_chance( delta_level ) ) )
    {
      result = RESULT_CRIT;
    }
  }

  tick_dmg = calculate_tick_damage();

  assess_damage( target, tick_dmg, DMG_OVER_TIME, result );

  if ( harmful && callbacks ) action_callback_t::trigger( player -> tick_callbacks[ result ], this );

  stats -> add_tick( time_to_tick );
}
// action_t::last_tick =======================================================

void action_t::last_tick()
{
  if ( sim -> debug ) log_t::output( sim, "%s fades from %s", name(), target -> name() );

  dot -> ticking = 0;
  time_to_tick = 0;

  if ( school == SCHOOL_BLEED ) target -> debuffs.bleeding -> decrement();
}

// action_t::travel ==========================================================

void action_t::travel( player_t* t, int travel_result, double travel_dmg=0 )
{
  assess_damage( t, travel_dmg, ( direct_tick ? DMG_OVER_TIME : DMG_DIRECT ), travel_result );

  if ( result_is_hit( travel_result ) )
  {
    if ( num_ticks > 0 )
    {
      if ( dot_behavior != DOT_REFRESH ) cancel();
      dot -> action = this;
      dot -> num_ticks = hasted_num_ticks();
      dot -> current_tick = 0;
      dot -> added_ticks = 0;
      dot -> added_seconds = 0;
      if ( dot -> ticking )
      {
	assert( dot -> tick_event );
	if ( ! channeled )
        {
	  // Recasting a dot while it's still ticking gives it an extra tick in total
	  dot -> num_ticks++;
	}
      }
      else
      {
	schedule_tick();
      }
      dot -> recalculate_ready();

      if ( sim -> debug )
	log_t::output( sim, "%s extends dot-ready to %.2f for %s (%s)", 
		       player -> name(), dot -> ready, name(), dot -> name() );
    }
  }
  else
  {
    if ( sim -> log )
    {
      log_t::output( sim, "Target %s avoids %s %s (%s)", target -> name(), player -> name(), name(), util_t::result_type_string( travel_result ) );
    }
  }
}

// action_t::assess_damage ==================================================

void action_t::assess_damage( player_t* t,
			      double dmg_amount,
			      int    dmg_type, 
			      int    dmg_result )
{
  double dmg_adjusted = t -> assess_damage( dmg_amount, school, dmg_type, dmg_result, this );
    
  if ( dmg_type == DMG_DIRECT )
  {
    if ( dmg_amount > 0 )
    {
      direct_dmg = dmg_adjusted;

      if ( sim -> log )
      {
        log_t::output( sim, "%s %s hits %s for %.0f %s damage (%s)",
                       player -> name(), name(),
                       target -> name(), dmg_adjusted,
                       util_t::school_type_string( school ),
                       util_t::result_type_string( result ) );
      }
      if ( callbacks ) action_callback_t::trigger( player -> direct_damage_callbacks[ school ], this );
    }
  }
  else // DMG_OVER_TIME
  {
    if ( dmg_amount > 0 )
    {
      tick_dmg = dmg_adjusted;

      if ( sim -> log )
      {
	log_t::output( sim, "%s %s ticks (%d of %d) %s for %.0f %s damage (%s)",
		       player -> name(), name(),
		       dot -> current_tick, dot -> num_ticks,
		       t -> name(), dmg_adjusted,
		       util_t::school_type_string( school ),
		       util_t::result_type_string( result ) );
      }
      if ( callbacks ) action_callback_t::trigger( player -> tick_damage_callbacks[ school ], this );
    }
  }

  stats -> add_result( dmg_adjusted, dmg_type, dmg_result );

  if ( dmg_amount > 0 && t -> is_enemy() )
  {
    target_t* q = t -> cast_target();

    if ( dmg_amount > 0 && aoe && q -> adds_nearby )
    {
      double dmg_aoe = dmg_amount * base_add_multiplier;
      for ( int i=0; i < q -> adds_nearby && i < ( aoe > 0 ? aoe : 9 ); i++ )
      {
        additional_damage( q, dmg_aoe, dmg_type, dmg_result );
      }
    }
  }
}

// action_t::additional_damage =============================================

void action_t::additional_damage( player_t* t,
                                  double dmg_amount,
                                  int    dmg_type,
                                  int    dmg_result )
{
  dmg_amount /= target_multiplier; // FIXME! Weak lip-service to the fact that the adds probably will not be properly debuffed.
  t -> assess_damage( dmg_amount, school, dmg_type, dmg_result, this );
  stats -> add_result( dmg_amount, dmg_type, dmg_result );
}

// action_t::schedule_execute ==============================================

void action_t::schedule_execute()
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s schedules execute for %s", player -> name(), name() );
  }

  time_to_execute = execute_time();

  execute_event = new ( sim ) action_execute_event_t( sim, this, time_to_execute );

  if ( ! background )
  {
    player -> executing = this;
    player -> gcd_ready = sim -> current_time + gcd();
    if( player -> action_queued )
    {
      player -> gcd_ready -= sim -> queue_gcd_reduction;
    }
  }
  if ( special && time_to_execute > 0 && ! proc )
  {
    // While an ability is casting, the auto_attack is paused
    // So we simply reschedule the auto_attack by the ability's casttime
    double time_to_next_hit;
    // Mainhand
    if ( player -> main_hand_attack )
    {
      time_to_next_hit  = player -> main_hand_attack -> execute_event -> occurs();
      time_to_next_hit -= sim -> current_time;
      time_to_next_hit += time_to_execute;
      player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
    // Offhand
    if ( player -> off_hand_attack )
    {
      time_to_next_hit  = player -> off_hand_attack -> execute_event -> occurs();
      time_to_next_hit -= sim -> current_time;
      time_to_next_hit += time_to_execute;
      player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
  }
}

// action_t::schedule_tick =================================================

void action_t::schedule_tick()
{
  if ( sim -> debug ) log_t::output( sim, "%s schedules tick for %s", player -> name(), name() );

  if ( dot -> current_tick == 0 )
  {
    if ( school == SCHOOL_BLEED ) target -> debuffs.bleeding -> increment();

    if ( tick_zero )
    {
      time_to_tick = 0;
      tick();
    }
  }

  time_to_tick = tick_time();

  dot -> tick_event = new ( sim ) dot_tick_event_t( sim, dot, time_to_tick );

  dot -> ticking = 1;

  if ( channeled ) player -> channeling = this;
}

// action_t::schedule_travel ===============================================

void action_t::schedule_travel( player_t* t )
{
  time_to_travel = travel_time();

  if ( time_to_travel == 0 )
  {
    travel( t, result, direct_dmg );
  }
  else
  {
    if ( sim -> log )
    {
      log_t::output( sim, "%s schedules travel (%.2f) for %s", player -> name(),time_to_travel, name() );
    }

    travel_event = new ( sim ) action_travel_event_t( sim, t, this, time_to_travel );
  }
}

// action_t::reschedule_execute ============================================

void action_t::reschedule_execute( double time )
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s reschedules execute for %s", player -> name(), name() );
  }

  double delta_time = sim -> current_time + time - execute_event -> occurs();

  time_to_execute += delta_time;

  if ( delta_time > 0 )
  {
    execute_event -> reschedule( time );
  }
  else // Impossible to reschedule events "early".  Need to be canceled and re-created.
  {
    event_t::cancel( execute_event );
    execute_event = new ( sim ) action_execute_event_t( sim, this, time );
  }
}

// action_t::refresh_duration ================================================

void action_t::refresh_duration()
{
  if ( sim -> log ) log_t::output( sim, "%s refreshes duration of %s", player -> name(), name() );

  // Make sure this DoT is still ticking......
  assert( dot -> tick_event );

  player_buff();
  target_debuff( target, DMG_OVER_TIME );

  dot -> action = this;
  dot -> current_tick = 0;
  dot -> added_ticks = 0;
  dot -> added_seconds = 0;
  dot -> num_ticks = hasted_num_ticks();
  dot -> recalculate_ready();
}

// action_t::extend_duration =================================================

void action_t::extend_duration( int extra_ticks )
{
  if ( sim -> log ) log_t::output( sim, "%s extends duration of %s, adding %d tick(s), totalling %d ticks", player -> name(), name(), extra_ticks, dot -> num_ticks + extra_ticks );

  // Make sure this DoT is still ticking......
  assert( dot -> tick_event );

  player_buff();
  target_debuff( target, DMG_OVER_TIME );

  dot -> action = this;
  dot -> added_ticks += extra_ticks;
  dot -> num_ticks += extra_ticks;
  dot -> recalculate_ready();
}

// action_t::extend_duration_seconds =========================================

void action_t::extend_duration_seconds( double extra_seconds )
{
  // Make sure this DoT is still ticking......
  assert( dot -> tick_event );
  
  // Treat extra_ticks as 'seconds added' instead of 'ticks added'
  // Duration left needs to be calculated with old haste for tick_time()
  // First we need the number of ticks remaining after the next one =>
  // ( num_ticks - current_tick ) - 1
  int old_num_ticks = dot -> num_ticks;
  int old_remaining_ticks = old_num_ticks - dot -> current_tick - 1;
  double old_haste_factor = 1.0 / player_haste;

  // Multiply with tick_time() for the duration left after the next tick
  double duration_left = old_remaining_ticks * tick_time();

  // Add the added seconds
  duration_left += extra_seconds;
  
  // Switch to new haste values and calculate resulting ticks
  // ONLY updates haste, modifiers/spellpower are left untouched.
  player_haste = haste();
  target_debuff( target, DMG_OVER_TIME );
  dot -> action = this;
  dot -> added_seconds += extra_seconds;

  int new_remaining_ticks = hasted_num_ticks( duration_left );
  dot -> num_ticks += ( new_remaining_ticks - old_remaining_ticks );

  if ( sim -> debug ) 
  {
    log_t::output( sim, "%s extends duration of %s by %.1f second(s). h: %.2f => %.2f, num_t: %d => %d, rem_t: %d => %d", 
                  player -> name(), name(), extra_seconds, 
                  old_haste_factor, (1.0 / player_haste), 
                  old_num_ticks, dot -> num_ticks,
                  old_remaining_ticks, new_remaining_ticks );
  }
  else if ( sim -> log ) 
  {
    log_t::output( sim, "%s extends duration of %s by %.1f second(s).", player -> name(), name(), extra_seconds );
  }

  dot -> recalculate_ready();
}

// action_t::update_ready ====================================================

void action_t::update_ready()
{
  if ( cooldown -> duration > 0 && ! dual )
  {
    if ( sim -> debug ) log_t::output( sim, "%s starts cooldown for %s (%s)", player -> name(), name(), cooldown -> name() );

    cooldown -> start();
  }
  if ( num_ticks )
  {
    if( result_is_miss() )
    {
      if ( sim -> debug ) 
        log_t::output( sim, "%s pushes out re-cast (%.2f) on miss for %s (%s)", 
                       player -> name(), sim -> reaction_time, name(), dot -> name() );

      dot -> miss_time = sim -> current_time;
    }
  }
}

// action_t::usable_moving ====================================================

bool action_t::usable_moving()
{
  bool usable = true;

  if ( execute_time() > 0 )
    return false;

  if ( channeled )
    return false;

  if ( range > 0 && range <= 5 )
    return false;

  return usable;
}

// action_t::ready ==========================================================

bool action_t::ready()
{
  player_t* t = target;

  if ( player -> skill < 1.0 )
    if ( ! sim -> roll( player -> skill ) )
      return false;

  if ( cooldown -> remains() > 0 )
    return false;

  if ( ! player -> resource_available( resource, cost() ) )
    return false;

  if ( min_current_time > 0 )
    if ( sim -> current_time < min_current_time )
      return false;

  if ( max_current_time > 0 )
    if ( sim -> current_time > max_current_time )
      return false;

  if ( max_haste > 0 )
    if ( ( ( 1.0 / haste() ) - 1.0 ) > max_haste )
      return false;

  if ( bloodlust_active > 0 )
    if ( ! player -> buffs.bloodlust -> check() )
      return false;

  if ( bloodlust_active < 0 )
    if ( player -> buffs.bloodlust -> check() )
      return false;

  if ( sync_action && ! sync_action -> ready() )
    return false;

  if ( target -> debuffs.invulnerable -> check() )
    if ( harmful )
      return false;

  if ( player -> buffs.moving -> check() )
    if ( ! usable_moving() )
      return false;

  if ( moving != -1 )
    if ( moving != ( player -> buffs.moving -> check() ? 1 : 0 ) )
      return false;

  if ( vulnerable )
    if ( ! t -> debuffs.vulnerable -> check() )
      return false;

  if ( invulnerable )
    if ( ! t -> debuffs.invulnerable -> check() )
      return false;

  if ( min_health_percentage > 0 )
    if ( t -> health_percentage() < min_health_percentage )
      return false;

  if ( max_health_percentage > 0 )
    if ( t -> health_percentage() > max_health_percentage )
      return false;

  if ( target -> is_enemy())
  {

    target_t* t = target -> cast_target();

    if ( min_time_to_die > 0 )
      if ( t -> time_to_die() < min_time_to_die )
        return false;

    if ( max_time_to_die > 0 )
      if ( t -> time_to_die() > max_time_to_die )
        return false;

  }

  if ( if_expr )
  {
    int result_type = if_expr -> evaluate();
    if ( result_type == TOK_NUM     ) return if_expr -> result_num != 0;
    if ( result_type == TOK_STR     ) return true;
    if ( result_type == TOK_UNKNOWN ) return false;
  }

  return true;
}

// action_t::reset ==========================================================

void action_t::reset()
{
  if( ! rng[ 0 ] )
  {
    std::string buffer;
    for ( int i=0; i < RESULT_MAX; i++ )
    {
      buffer  = name();
      buffer += "_";
      buffer += util_t::result_type_string( i );
      rng[ i ] = player -> get_rng( buffer, ( ( i == RESULT_CRIT ) ? RNG_DISTRIBUTED : RNG_CYCLIC ) );
    }
  }

  if ( ! sync_str.empty() && ! sync_action )
  {
    sync_action = player -> find_action( sync_str );

    if ( ! sync_action )
    {
      sim -> errorf( "Unable to find sync action '%s' for primary action '%s'\n", sync_str.c_str(), name() );
      sim -> cancel();
    }
  }

  cooldown -> reset();
  dot -> reset();
  result = RESULT_NONE;
  execute_event = 0;
  travel_event = 0;

  if ( ! dual ) stats -> reset( this );
}

// action_t::cancel =========================================================

void action_t::cancel()
{
  if ( dot -> ticking ) last_tick();

  if ( player -> executing  == this ) player -> executing  = 0;
  if ( player -> channeling == this ) player -> channeling = 0;

  event_t::cancel( execute_event );
  event_t::cancel( dot -> tick_event );

  dot -> reset();
}

// action_t::check_talent ===================================================

void action_t::check_talent( int talent_rank )
{
  if ( talent_rank != 0 ) return;

  if ( player -> is_pet() )
  {
    pet_t* p = player -> cast_pet();
    sim -> errorf( "Player %s has pet %s attempting to execute action %s without the required talent.\n", 
                   p -> owner -> name(), p -> name(), name() );
  }
  else
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required talent.\n", player -> name(), name() );
  }

  background = true; // prevent action from being executed
}

// action_t::check_spec =====================================================

void action_t::check_spec( int necessary_spec )
{
  if ( player -> primary_tree() != necessary_spec )
  {
    sim -> errorf( "Player %s attempting to execute action %s without %s spec.\n", 
                   player -> name(), name(), util_t::talent_tree_string( necessary_spec ) );

    background = true; // prevent action from being executed
  }
}

// action_t::check_min_level ===================================================

void action_t::check_min_level( int action_level )
{
  if ( action_level <= player -> level ) return;

  sim -> errorf( "Player %s attempting to execute action %s without the required level (%d < %d).\n", 
                 player -> name(), name(), player -> level, action_level );

  background = true; // prevent action from being executed
}

// action_t::create_expression ==============================================

action_expr_t* action_t::create_expression( const std::string& name_str )
{
  if ( name_str == "ticking" )
  {
    struct ticking_expr_t : public action_expr_t
    {
      ticking_expr_t( action_t* a ) : action_expr_t( a, "ticking", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> dot -> ticking; return TOK_NUM; }
    };
    return new ticking_expr_t( this );
  }
  if ( name_str == "ticks" )
  {
    struct ticks_expr_t : public action_expr_t
    {
      ticks_expr_t( action_t* a ) : action_expr_t( a, "ticks", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> dot -> current_tick; return TOK_NUM; }
    };
    return new ticks_expr_t( this );
  }
  if ( name_str == "ticks_remain" )
  {
    struct ticks_remain_expr_t : public action_expr_t
    {
      ticks_remain_expr_t( action_t* a ) : action_expr_t( a, "ticks_remain", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> dot -> ticks(); return TOK_NUM; }
    };
    return new ticks_remain_expr_t( this );
  }
  if ( name_str == "remains" )
  {
    struct remains_expr_t : public action_expr_t
    {
      remains_expr_t( action_t* a ) : action_expr_t( a, "remains", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> dot -> remains(); return TOK_NUM; }
    };
    return new remains_expr_t( this );
  }
  if ( name_str == "cast_time" )
  {
    struct cast_time_expr_t : public action_expr_t
    {
      cast_time_expr_t( action_t* a ) : action_expr_t( a, "cast_time", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> execute_time(); return TOK_NUM; }
    };
    return new cast_time_expr_t( this );
  }
  if ( name_str == "cooldown" )
  {
    struct cooldown_expr_t : public action_expr_t
    {
      cooldown_expr_t( action_t* a ) : action_expr_t( a, "cooldown", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> cooldown -> duration; return TOK_NUM; }
    };
    return new cooldown_expr_t( this );
  }
  if ( name_str == "tick_time" )
  {
    struct tick_time_expr_t : public action_expr_t
    {
      tick_time_expr_t( action_t* a ) : action_expr_t( a, "tick_time", TOK_NUM ) {}
          virtual int evaluate() { result_num = ( action -> dot -> ticking ) ? action -> dot -> action -> tick_time() : 0; return TOK_NUM; }
    };
    return new tick_time_expr_t( this );
  }
  if ( name_str == "gcd" )
  {
    struct cast_time_expr_t : public action_expr_t
    {
      cast_time_expr_t( action_t* a ) : action_expr_t( a, "gcd", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> gcd(); return TOK_NUM; }
    };
    return new cast_time_expr_t( this );
  }
  if ( name_str == "travel_time" )
  {
    struct travel_time_expr_t : public action_expr_t
    {
      travel_time_expr_t( action_t* a ) : action_expr_t( a, "travel_time", TOK_NUM ) {}
    virtual int evaluate() { result_num = action -> travel_time(); return TOK_NUM; }
    };
    return new travel_time_expr_t( this );
  }
  if ( name_str == "in_flight" )
  {
    struct in_flight_expr_t : public action_expr_t
    {
      in_flight_expr_t( action_t* a ) : action_expr_t( a, "in_flight", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> travel_event != NULL; return TOK_NUM; }
    };
    return new in_flight_expr_t( this );
  }
  if ( name_str == "miss_react" )
  {
    struct miss_react_expr_t : public action_expr_t
    {
      miss_react_expr_t( action_t* a ) : action_expr_t( a, "miss_react", TOK_NUM ) {}
      virtual int evaluate() { 
        if ( action -> dot -> miss_time == -1 
          || action -> sim -> current_time >= action -> dot -> miss_time + action -> sim -> reaction_time )
        {
          result_num = 1;
        }
        else
        {
          result_num = 0; 
        }
        return TOK_NUM; 
      }
    };
    return new miss_react_expr_t( this );
  }

  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, name_str, "." );

  if ( num_splits == 3 )
   {
     if ( splits[ 0 ] == "debuff" )
     {
       buff_t* buff = buff_t::find( target, splits[ 1 ] );
       if ( ! buff ) return 0;
       return buff -> create_expression( this, splits[ 2 ] );
     }
   }

  return player -> create_expression( this, name_str );
}

// action_t::ppm_proc_chance ================================================

double action_t::ppm_proc_chance( double PPM ) SC_CONST
{
  if ( weapon )
  {
    return weapon -> proc_chance_on_swing( PPM, time_to_execute );
  }
  else
  {
    double time = channeled ? time_to_tick : time_to_execute;

    if ( time == 0 ) time = player -> base_gcd;

    return( PPM * time / 60.0 );
  }
}

double action_t::tick_time() SC_CONST
{
  double t = base_tick_time;
  if ( channeled || hasted_ticks )
  {
    t *= player_haste;
  }
  return t;
}

int action_t::hasted_num_ticks( double d ) SC_CONST
{
  if ( ! hasted_ticks ) return num_ticks;

  assert( player_haste > 0.0 );

  // For the purposes of calculating the number of ticks, the tick time is rounded to the 3rd decimal place.
  // It's important that we're accurate here so that we model haste breakpoints correctly.

  if ( d < 0 )
    d = num_ticks * base_tick_time;

  double t = floor( ( base_tick_time * player_haste * 1000.0 ) + 0.5 ) / 1000.0;

  return (int) floor( ( d / t ) + 0.5 );
}

