// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Action
// ==========================================================================

// action_t::action_t =======================================================

void action_t::init_action_t_()
{
  sim                            = s_player->sim;
  name_str                       = s_token;
  player                         = s_player;
  target                         = s_player -> target;
  id                             = 0;
  result                         = RESULT_NONE;
  aoe                            = 0;
  dual                           = false;
  callbacks                      = true;
  binary                         = false;
  channeled                      = false;
  background                     = false;
  sequence                       = false;
  use_off_gcd                    = false;
  direct_tick                    = false;
  repeating                      = false;
  harmful                        = true;
  proc                           = false;
  item_proc                      = false;
  proc_ignores_slot              = false;
  may_trigger_dtr                = true;
  discharge_proc                 = false;
  auto_cast                      = false;
  initialized                    = false;
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
  no_buffs                       = false;
  no_debuffs                     = false;
  dot_behavior                   = DOT_CLIP;
  ability_lag                    = 0.0;
  ability_lag_stddev             = 0.0;
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
  crit_multiplier                = 1.0;
  crit_bonus_multiplier          = 1.0;
  base_dd_adder                  = 0.0;
  player_dd_adder                = 0.0;
  target_dd_adder                = 0.0;
  player_haste                   = 1.0;
  resource_consumed              = 0.0;
  direct_dmg                     = 0.0;
  tick_dmg                       = 0.0;
  snapshot_crit                  = 0.0;
  snapshot_haste                 = 1.0;
  snapshot_mastery               = 0.0;
  num_ticks                      = 0;
  weapon                         = NULL;
  weapon_multiplier              = 1.0;
  base_add_multiplier            = 1.0;
  normalize_weapon_speed         = false;
  rng_travel                     = NULL;
  stats                          = NULL;
  execute_event                  = NULL;
  travel_event                   = NULL;
  time_to_execute                = 0.0;
  time_to_travel                 = 0.0;
  travel_speed                   = 0.0;
  bloodlust_active               = 0;
  max_haste                      = 0.0;
  haste_gain_percentage          = 0.0;
  min_current_time               = 0.0;
  max_current_time               = 0.0;
  min_health_percentage          = 0.0;
  max_health_percentage          = 0.0;
  moving                         = -1;
  vulnerable                     = 0;
  invulnerable                   = 0;
  not_flying                     = 0;
  wait_on_ready                  = -1;
  interrupt                      = 0;
  round_base_dmg                 = true;
  class_flag1                    = false;
  if_expr_str.clear();
  if_expr                        = NULL;
  interrupt_if_expr_str.clear();
  interrupt_if_expr              = NULL;
  sync_str.clear();
  sync_action                    = NULL;
  next                           = NULL;
  marker                         = 0;
  last_reaction_time             = 0.0;
  dtr_action                     = 0;
  is_dtr_action                  = false;
  cached_targetdata = NULL;
  cached_targetdata_target = NULL;
  action_dot = NULL;
  targetdata_dot_offset = -1;

  init_dot( name_str );

  if ( sim -> debug ) log_t::output( sim, "Player %s creates action %s", player -> name(), name() );

  if ( unlikely( ! player -> initialized ) )
  {
    sim -> errorf( "Actions must not be created before player_t::init().  Culprit: %s %s\n", player -> name(), name() );
    sim -> cancel();
  }

  action_t** last = &( player -> action_list );
  while ( *last ) last = &( ( *last ) -> next );
  *last = this;

  range::fill( rng, 0 );

  cooldown = player -> get_cooldown( name_str );

  stats = player -> get_stats( name_str, this );

  id = spell_id();
  tree = util_t::talent_tree( s_tree, player -> type );

  parse_data();

  const spell_data_t* spell = player -> dbc.spell( id );

  if ( id && spell && s_type == T_CLASS && ! spell -> is_level( player -> level ) && spell -> level() <= MAX_LEVEL )
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required level (%d < %d).\n",
                   player -> name(), name(), player -> level, spell -> level() );

    background = true; // prevent action from being executed
  }
}

void action_t::init_dot( const std::string& name )
{
  std::unordered_map<std::string, std::pair<player_type, size_t> >::iterator doti = sim->targetdata_items[0].find( name );
  if( doti != sim->targetdata_items[0].end() && doti->second.first == player->type )
    targetdata_dot_offset = ( int )doti->second.second;
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
  player( s_player ), target( s_player -> target ), school( s ), resource( r ),
  tree( tr ), special( sp )
{
  init_action_t_();
}

action_t::action_t( int ty, const char* name, const char* sname, player_t* p, int t, bool sp ) :
  spell_id_t( p, name, sname ),
  sim( s_player->sim ), type( ty ), name_str( s_token ),
  player( s_player ), target( s_player -> target ), school( get_school_type() ), resource( power_type() ),
  tree( t ), special( sp )
{
  init_action_t_();
}

action_t::action_t( int ty, const active_spell_t& s, int t, bool sp ) :
  spell_id_t( s ),
  sim( s_player->sim ), type( ty ), name_str( s_token ),
  player( s_player ), target( s_player -> target ), school( get_school_type() ), resource( power_type() ),
  tree( t ), special( sp )
{
  init_action_t_();
}

action_t::action_t( int type, const char* name, const uint32_t id, player_t* p, int t, bool sp ) :
  spell_id_t( p, name, id ),
  sim( s_player->sim ), type( type ), name_str( s_token ),
  player( s_player ), target( s_player -> target ), school( get_school_type() ), resource( power_type() ),
  tree( t ), special( sp )
{
  init_action_t_();
}

action_t::~action_t()
{
  if ( ! is_dtr_action )
  {
    delete if_expr;
    delete interrupt_if_expr;
  }
}

// action_t::parse_data =====================================================

void action_t::parse_data()
{
  if ( id > 0 && ( spell = player -> dbc.spell( id ) ) )
  {
    base_execute_time    = spell -> cast_time( player -> level );
    cooldown -> duration = spell -> cooldown();
    range                = spell -> max_range();
    travel_speed         = spell -> missile_speed();
    trigger_gcd          = spell -> gcd();
    school               = spell_id_t::get_school_type( spell -> school_mask() );
    stats -> school      = school;
    resource             = spell -> power_type();
    rp_gain              = spell -> runic_power_gain();

    // For mana it returns the % of base mana, not the absolute cost
    if ( resource == RESOURCE_MANA )
      base_cost = floor ( spell -> cost() * player -> resource_base[ RESOURCE_MANA ] );
    else
      base_cost = spell -> cost();

    for ( int i=1; i <= MAX_EFFECTS; i++ )
    {
      parse_effect_data( id, i );
    }
  }
}

// action_t::parse_effect_data ==============================================
void action_t::parse_effect_data( int spell_id, int effect_nr )
{
  if ( ! spell_id )
  {
    sim -> errorf( "%s %s: parse_effect_data: no spell_id provided.\n", player -> name(), name() );
    return;
  }

  const spell_data_t* spell = player -> dbc.spell( spell_id );
  const spelleffect_data_t* effect = player -> dbc.effect( spell -> effect_id( effect_nr ) );
  
  assert( spell );

  if ( ! effect )
  {
    sim -> errorf( "%s %s: parse_effect_data: no effect to parse.\n", player -> name(), name() );
    return;
  }

  switch ( effect -> type() )
  {
    // Direct Damage
  case E_HEAL:
  case E_SCHOOL_DAMAGE:
  case E_HEALTH_LEECH:
    direct_power_mod = effect -> coeff();
    base_dd_min      = player -> dbc.effect_min( effect -> id(), player -> level );
    base_dd_max      = player -> dbc.effect_max( effect -> id(), player -> level );
    break;

  case E_NORMALIZED_WEAPON_DMG:
    normalize_weapon_speed = true;
  case E_WEAPON_DAMAGE:
    base_dd_min      = player -> dbc.effect_min( effect -> id(), player -> level );
    base_dd_max      = player -> dbc.effect_max( effect -> id(), player -> level );
    weapon = &( player -> main_hand_weapon );
    break;

  case E_WEAPON_PERCENT_DAMAGE:
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = player -> dbc.effect_min( effect -> id(), player -> level );
    break;

    // Dot
  case E_PERSISTENT_AREA_AURA:
  case E_APPLY_AURA:
    switch ( effect -> subtype() )
    {
    case A_PERIODIC_DAMAGE:
      if ( school == SCHOOL_PHYSICAL )
        school = stats -> school = SCHOOL_BLEED;
    case A_PERIODIC_LEECH:
    case A_PERIODIC_HEAL:
      tick_power_mod   = effect -> coeff();
      base_td_init     = player -> dbc.effect_average( effect -> id(), player -> level );
      base_td          = base_td_init;
    case A_PERIODIC_ENERGIZE:
    case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
    case A_PERIODIC_HEALTH_FUNNEL:
    case A_PERIODIC_MANA_LEECH:
    case A_PERIODIC_DAMAGE_PERCENT:
    case A_PERIODIC_DUMMY:
    case A_PERIODIC_TRIGGER_SPELL:
      base_tick_time   = effect -> period();
      num_ticks        = ( int ) ( spell -> duration() / base_tick_time );
      break;
    case A_SCHOOL_ABSORB:
      direct_power_mod = effect -> coeff();
      base_dd_min      = player -> dbc.effect_min( effect -> id(), player -> level );
      base_dd_max      = player -> dbc.effect_max( effect -> id(), player -> level );
      break;
    case A_ADD_FLAT_MODIFIER:
      switch ( effect -> misc_value1() )
      case E_APPLY_AURA:
      switch ( effect -> subtype() )
      {
      case P_CRIT:
        base_crit += 0.01 * effect -> base_value();
        break;
      case P_COOLDOWN:
        cooldown -> duration += 0.001 * effect -> base_value();
        break;
      default: break;
      }
      break;
    case A_ADD_PCT_MODIFIER:
      switch ( effect -> misc_value1() )
      {
      case P_RESOURCE_COST:
        base_cost *= 1 + 0.01 * effect -> base_value();
        break;
      }
      break;
    default: break;
    }
    break;
  default: break;
  }
}

// action_t::parse_options ==================================================

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
    { "interrupt_if",           OPT_STRING, &interrupt_if_expr_str },
    { "interrupt",              OPT_BOOL,   &interrupt             },
    { "invulnerable",           OPT_BOOL,   &invulnerable          },
    { "not_flying",             OPT_BOOL,   &not_flying            },
    { "moving",                 OPT_BOOL,   &moving                },
    { "sync",                   OPT_STRING, &sync_str              },
    { "time<",                  OPT_FLT,    &max_current_time      },
    { "time>",                  OPT_FLT,    &min_current_time      },
    { "travel_speed",           OPT_FLT,    &travel_speed          },
    { "vulnerable",             OPT_BOOL,   &vulnerable            },
    { "wait_on_ready",          OPT_BOOL,   &wait_on_ready         },
    { "target",                 OPT_STRING, &target_str            },
    { "label",                  OPT_STRING, &label_str             },
    { "use_off_gcd",            OPT_BOOL,   &use_off_gcd           },
    { NULL,                     0,          NULL                   }
  };

  std::vector<option_t> merged_options;
  option_t::merge( merged_options, options, base_options );

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
    else
    {
      sim -> errorf( "%s %s: Unable to locate target '%s'.\n", player -> name(), name(), options_str.c_str() );
    }
  }
}

// action_t::cost ===========================================================

double action_t::cost() const
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

  if ( is_dtr_action )
    c = 0;

  if ( sim -> debug ) log_t::output( sim, "action_t::cost: %s %.2f %.2f %s", name(), base_cost, c, util_t::resource_type_string( resource ) );

  return floor( c );
}

// action_t::gcd ============================================================

double action_t::gcd() const
{
  if ( ! harmful && ! player -> in_combat )
    return 0;

  return trigger_gcd;
}

// action_t::travel_time ====================================================

double action_t::travel_time()
{
  if ( travel_speed == 0 ) return 0;

  if ( player -> distance == 0 ) return 0;

  double t = player -> distance / travel_speed;

  double v = sim -> travel_variance;

  if ( v )
  {
    t = rng_travel -> gauss( t, v );
  }

  return t;
}

// action_t::player_buff ====================================================

void action_t::player_buff()
{
  player_multiplier              = 1.0;
  player_dd_multiplier           = 1.0;
  player_td_multiplier           = 1.0;
  player_hit                     = 0;
  player_crit                    = 0;
  player_penetration             = 0;
  player_dd_adder                = 0;
  player_spell_power             = 0;
  player_attack_power            = 0;
  player_spell_power_multiplier  = 1.0;
  player_attack_power_multiplier = 1.0;

  if ( ! no_buffs )
  {
    player_t* p = player;

    if ( school == SCHOOL_BLEED )
    {
      // Not applicable
    }

    if ( school == SCHOOL_PHYSICAL )
    {
      if ( p -> debuffs.demoralizing_roar    -> up() ||
           p -> debuffs.demoralizing_shout   -> up() ||
           p -> debuffs.demoralizing_screech -> up() ||
           p -> debuffs.scarlet_fever        -> up() ||
           p -> debuffs.vindication          -> up() )
      {
        player_multiplier *= 0.90;
      }
    }
    else if ( school != SCHOOL_PHYSICAL )
    {
      player_penetration = p -> composite_spell_penetration();
    }

    player_multiplier    = p -> composite_player_multiplier   ( school, this );
    player_dd_multiplier = p -> composite_player_dd_multiplier( school, this );
    player_td_multiplier = p -> composite_player_td_multiplier( school, this );

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
  }

  player_haste = total_haste();

  if ( sim -> debug )
    log_t::output( sim, "action_t::player_buff: %s hit=%.2f crit=%.2f penetration=%.0f spell_power=%.2f attack_power=%.2f ",
                   name(), player_hit, player_crit, player_penetration, player_spell_power, player_attack_power );
}

// action_t::target_debuff ==================================================

void action_t::target_debuff( player_t* t, int /* dmg_type */ )
{
  target_multiplier            = 1.0;
  target_hit                   = 0;
  target_crit                  = 0;
  target_attack_power          = 0;
  target_spell_power           = 0;
  target_penetration           = 0;
  target_dd_adder              = 0;

  if ( ! no_debuffs )
  {
    if ( school == SCHOOL_PHYSICAL ||
         school == SCHOOL_BLEED    )
    {
      if ( t -> debuffs.savage_combat -> up() )
      {
        target_multiplier *= 1.04;
      }
      else if ( t -> debuffs.blood_frenzy_physical -> value() || t -> debuffs.brittle_bones -> value() || t -> debuffs.ravage -> value() )
      {
        target_multiplier *= 1.0 + std::max(
                                   std::max( t -> debuffs.blood_frenzy_physical -> value() * 0.01,
                                             t -> debuffs.brittle_bones         -> value() ),
                                             t -> debuffs.ravage                -> value() * 0.01 );

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
      if ( t -> debuffs.mangle -> up() || t -> debuffs.hemorrhage -> up() || t -> debuffs.tendon_rip -> up() )
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

  }

  if ( sim -> debug )
    log_t::output( sim, "action_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f penetration=%.0f",
                   name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power, target_penetration );
}

// action_t::snapshot

void action_t::snapshot()
{
  snapshot_crit    = total_crit();
  snapshot_haste   = haste();
  snapshot_mastery = player -> composite_mastery();
}

// action_t::result_is_hit ==================================================

bool action_t::result_is_hit( int r ) const
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

bool action_t::result_is_miss( int r ) const
{
  if ( r == RESULT_UNKNOWN ) r = result;

  return( r == RESULT_MISS   ||
          r == RESULT_DODGE  ||
          r == RESULT_PARRY  ||
          r == RESULT_RESIST );
}

// action_t::armor ==========================================================

double action_t::armor() const
{
  return target -> composite_armor();
}

// action_t::resistance =====================================================

double action_t::resistance() const
{
  if ( ! may_resist )
    return 0;

  player_t* t = target;
  double resist=0;

  double penetration = base_penetration + player_penetration + target_penetration;

  if ( school == SCHOOL_BLEED )
  {
    // Bleeds cannot be resisted
  }
  else
  {
    double resist_rating = t -> composite_spell_resistance( school );

    if ( school == SCHOOL_FROSTFIRE )
    {
      resist_rating = std::min( t -> composite_spell_resistance( SCHOOL_FROST ),
                                t -> composite_spell_resistance( SCHOOL_FIRE  ) );
    }
    else if ( school == SCHOOL_SPELLSTORM )
    {
      resist_rating = std::min( t -> composite_spell_resistance( SCHOOL_ARCANE ),
                                t -> composite_spell_resistance( SCHOOL_NATURE ) );
    }
    else if ( school == SCHOOL_SHADOWFROST )
    {
      resist_rating = std::min( t -> composite_spell_resistance( SCHOOL_SHADOW ),
                                t -> composite_spell_resistance( SCHOOL_FROST  ) );
    }
    else if ( school == SCHOOL_SHADOWFLAME )
    {
      resist_rating = std::min( t -> composite_spell_resistance( SCHOOL_SHADOW ),
                                t -> composite_spell_resistance( SCHOOL_FIRE   ) );
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

// action_t::total_crit_bonus ===============================================

double action_t::total_crit_bonus() const
{
  double bonus = ( ( 1.0 + crit_bonus ) * crit_multiplier - 1.0 ) * crit_bonus_multiplier;

  if ( sim -> debug )
  {
    log_t::output( sim, "%s crit_bonus for %s: cb=%.3f b_cb=%.2f b_cm=%.2f b_cbm=%.2f",
                   player -> name(), name(), bonus, crit_bonus, crit_multiplier, crit_bonus_multiplier );
  }

  return bonus;
}

// action_t::total_power ====================================================

double action_t::total_power() const
{
  double power=0;

  if ( base_spell_power_multiplier  > 0 ) power += total_spell_power();
  if ( base_attack_power_multiplier > 0 ) power += total_attack_power();

  return power;
}

// action_t::calculate_weapon_damage ========================================

double action_t::calculate_weapon_damage()
{
  if ( ! weapon || weapon_multiplier <= 0 ) return 0;

  double dmg = sim -> range( weapon -> min_dmg, weapon -> max_dmg ) + weapon -> bonus_dmg;

  double weapon_speed  = normalize_weapon_speed  ? weapon -> normalized_weapon_speed() : weapon -> swing_time;

  double power_damage = weapon_speed * weapon_power_mod * total_attack_power();

  double total_dmg = dmg + power_damage;

  // OH penalty
  if ( weapon -> slot == SLOT_OFF_HAND )
    total_dmg *= 0.5;

  if ( sim -> debug )
  {
    log_t::output( sim, "%s weapon damage for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f pd=%.3f ap=%.3f",
                   player -> name(), name(), total_dmg, dmg, weapon -> bonus_dmg, weapon_speed, power_damage, total_attack_power() );
  }

  return total_dmg;
}

// action_t::calculate_tick_damage ==========================================

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

  if ( ! sim -> average_range )
    dmg = floor( dmg + sim -> real() );

  if ( sim -> debug )
  {
    log_t::output( sim, "%s dmg for %s: td=%.0f i_td=%.0f b_td=%.0f mod=%.2f power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f",
                   player -> name(), name(), dmg, init_tick_dmg, base_td, tick_power_mod,
                   total_power(), base_multiplier * base_td_multiplier, player_multiplier, target_multiplier );
  }

  return dmg;
}

// action_t::calculate_direct_damage ========================================

double action_t::calculate_direct_damage( int chain_target )
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

  // AoE with decay per target
  if ( chain_target > 0 && base_add_multiplier != 1.0 )
    dmg *= pow( base_add_multiplier, chain_target );
  
  // AoE with static reduced damage per target
  if ( chain_target > 1 && base_aoe_multiplier != 1.0 )
    dmg *= base_aoe_multiplier;

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
  if ( resource == RESOURCE_NONE ) return;

  resource_consumed = cost();

  player -> resource_loss( resource, resource_consumed, this );

  if ( sim -> log )
    log_t::output( sim, "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                   resource_consumed, util_t::resource_type_string( resource ),
                   name(), player -> resource_current[ resource] );

  stats -> consume_resource( resource_consumed );
}

// action_t::available_targets ==============================================

size_t action_t::available_targets( std::vector< player_t* >& tl ) const
{
  // TODO: This does not work for heals at all, as it presumes enemies in the 
  // actor list.

  tl.push_back( target );

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    if ( ! sim -> actor_list[ i ] -> sleeping &&
         sim -> actor_list[ i ] -> is_enemy() && 
         sim -> actor_list[ i ] != target )
      tl.push_back( sim -> actor_list[ i ] );
  }
  
  return tl.size();
}

// action_t::target_list ====================================================

std::vector< player_t* > action_t::target_list() const
{
  // A very simple target list for aoe spells, pick any and all targets, up to 
  // aoe amount, or if aoe == -1, pick all (enemy) targets

  std::vector< player_t* > t;
  
  size_t total_targets = available_targets( t );
  
  if ( aoe == -1 || total_targets <= static_cast< size_t >( aoe + 1 ) )
    return t;
  // Drop out targets from the end
  else 
  {
    t.resize( aoe + 1 );
    
    return t;
  }
}

// action_t::execute ========================================================

void action_t::execute()
{
  if ( unlikely( ! initialized ) )
  {
    sim -> errorf( "action_t::execute: action %s from player %s is not initialized.\n", name(), player -> name() );
    assert( 0 );
  }

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
  
  if ( aoe == -1 || aoe > 0 )
  {
    std::vector< player_t* > tl = target_list();
    
    for ( size_t t = 0; t < tl.size(); t++ )
    {
      target_debuff( tl[ t ], DMG_DIRECT );

      calculate_result();

      if ( result_is_hit() )
        direct_dmg = calculate_direct_damage( t + 1 );

      schedule_travel( tl[ t ] );
    }
  }
  else
  {
    target_debuff( target, DMG_DIRECT );
    
    calculate_result();
    
    if ( result_is_hit() )
      direct_dmg = calculate_direct_damage();
    
    schedule_travel( target );
  }

  consume_resource();

  update_ready();

  if ( ! dual ) stats -> add_execute( time_to_execute );

  if ( repeating && ! proc ) schedule_execute();
}

// action_t::tick ===========================================================

void action_t::tick( dot_t* d )
{
  if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );

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

  stats -> add_tick( d -> time_to_tick );
}

// action_t::last_tick ======================================================

void action_t::last_tick( dot_t* d )
{
  if ( sim -> debug ) log_t::output( sim, "%s fades from %s", d -> name(), target -> name() );

  d -> ticking = 0;

  if ( school == SCHOOL_BLEED ) target -> debuffs.bleeding -> decrement();
}

// action_t::impact =========================================================

void action_t::impact( player_t* t, int impact_result, double travel_dmg=0 )
{
  assess_damage( t, travel_dmg, DMG_DIRECT, impact_result );
  
  // Set target so aoe dots work
  player_t* orig_target = target;
  target = t;

  if ( result_is_hit( impact_result ) )
  {
    if ( num_ticks > 0 )
    {
      dot_t* dot = this -> dot();
      if ( dot_behavior != DOT_REFRESH ) dot -> cancel();
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

          // tick_zero dots tick again when reapplied
          if ( tick_zero )
          {
            tick( dot );
          }
        }
      }
      else
      {
        if ( school == SCHOOL_BLEED ) target -> debuffs.bleeding -> increment();

        dot -> schedule_tick();
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
      log_t::output( sim, "Target %s avoids %s %s (%s)", target -> name(), player -> name(), name(), util_t::result_type_string( impact_result ) );
    }
  }

  // Reset target
  target = orig_target;
}

// action_t::assess_damage ==================================================

void action_t::assess_damage( player_t* t,
                              double dmg_amount,
                              int    dmg_type,
                              int    dmg_result )
{
  double dmg_adjusted = t -> assess_damage( dmg_amount, school, dmg_type, dmg_result, this );
  double actual_amount = t -> infinite_resource[ RESOURCE_HEALTH ] ? dmg_adjusted : std::min( dmg_adjusted, t -> resource_current[ RESOURCE_HEALTH ] );

  if ( dmg_type == DMG_DIRECT )
  {
    if ( sim -> log )
    {
      log_t::output( sim, "%s %s hits %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     t -> name(), dmg_adjusted,
                     util_t::school_type_string( school ),
                     util_t::result_type_string( dmg_result ) );
    }

    direct_dmg = dmg_adjusted;

    if ( callbacks ) action_callback_t::trigger( player -> direct_damage_callbacks[ school ], this );
  }
  else // DMG_OVER_TIME
  {
    if ( sim -> log )
    {
      dot_t* dot = this -> dot();
      log_t::output( sim, "%s %s ticks (%d of %d) %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     dot -> current_tick, dot -> num_ticks,
                     t -> name(), dmg_adjusted,
                     util_t::school_type_string( school ),
                     util_t::result_type_string( dmg_result ) );
    }

    tick_dmg = dmg_adjusted;

    if ( callbacks ) action_callback_t::trigger( player -> tick_damage_callbacks[ school ], this );
  }

  stats -> add_result( actual_amount, dmg_adjusted, ( direct_tick ? DMG_OVER_TIME : dmg_type ), dmg_result );
}

// action_t::additional_damage ==============================================

void action_t::additional_damage( player_t* t,
                                  double dmg_amount,
                                  int    dmg_type,
                                  int    dmg_result )
{
  dmg_amount /= target_multiplier; // FIXME! Weak lip-service to the fact that the adds probably will not be properly debuffed.
  double dmg_adjusted = t -> assess_damage( dmg_amount, school, dmg_type, dmg_result, this );
  double actual_amount = std::min( dmg_adjusted, t -> resource_current[ resource ] );
  stats -> add_result( actual_amount, dmg_amount, dmg_type, dmg_result );
}

// action_t::schedule_execute ===============================================

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
    if ( player -> action_queued && sim -> strict_gcd_queue )
    {
      player -> gcd_ready -= sim -> queue_gcd_reduction;
    }

    if ( special && time_to_execute > 0 && ! proc )
    {
      // While an ability is casting, the auto_attack is paused
      // So we simply reschedule the auto_attack by the ability's casttime
      double time_to_next_hit;
      // Mainhand
      if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
      {
        time_to_next_hit  = player -> main_hand_attack -> execute_event -> occurs();
        time_to_next_hit -= sim -> current_time;
        time_to_next_hit += time_to_execute;
        player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
      // Offhand
      if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
      {
        time_to_next_hit  = player -> off_hand_attack -> execute_event -> occurs();
        time_to_next_hit -= sim -> current_time;
        time_to_next_hit += time_to_execute;
        player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
    }
  }
}

// action_t::schedule_travel ================================================

void action_t::schedule_travel( player_t* t )
{
  time_to_travel = travel_time();

  snapshot();

  if ( time_to_travel == 0 )
  {
    impact( t, result, direct_dmg );
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

// action_t::reschedule_execute =============================================

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

// action_t::update_ready ===================================================

void action_t::update_ready()
{
  double delay = 0;
  if ( cooldown -> duration > 0 && ! dual )
  {

    if ( ! background && ! proc )
    {
      double lag, dev;

      lag = player -> world_lag_override ? player -> world_lag : sim -> world_lag;
      dev = player -> world_lag_stddev_override ? player -> world_lag_stddev : sim -> world_lag_stddev;
      delay = player -> rngs.lag_world -> gauss( lag, dev );
      if ( sim -> debug ) log_t::output( sim, "%s delaying the cooldown finish of %s by %f", player -> name(), name(), delay );
    }

    cooldown -> start( -1, delay );

    if ( sim -> debug ) log_t::output( sim, "%s starts cooldown for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready );
  }
  if ( num_ticks )
  {
    if ( result_is_miss() )
    {
      dot_t* dot = this -> dot();
      last_reaction_time = player -> total_reaction_time();
      if ( sim -> debug )
        log_t::output( sim, "%s pushes out re-cast (%.2f) on miss for %s (%s)",
                       player -> name(), last_reaction_time, name(), dot -> name() );

      dot -> miss_time = sim -> current_time;
    }
  }
}

// action_t::usable_moving ==================================================

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

  if ( unlikely( is_dtr_action ) )
    assert( 0 );

  if ( player -> skill < 1.0 )
    if ( ! sim -> roll( player -> skill ) )
      return false;

  if ( cooldown -> remains() > 0 )
    return false;

  if ( ! player -> resource_available( resource, cost() ) )
    return false;

  if ( if_expr && ! if_expr -> success() )
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

  if ( unlikely( t -> sleeping ) )
    return false;

  if ( target -> debuffs.invulnerable -> check() )
    if ( harmful )
      return false;

  if ( player -> is_moving() )
    if ( ! usable_moving() )
      return false;

  if ( moving != -1 )
    if ( moving != ( player -> is_moving() ? 1 : 0 ) )
      return false;

  if ( vulnerable )
    if ( ! t -> debuffs.vulnerable -> check() )
      return false;

  if ( invulnerable )
    if ( ! t -> debuffs.invulnerable -> check() )
      return false;

  if ( not_flying )
    if ( t -> debuffs.flying -> check() )
      return false;

  if ( min_health_percentage > 0 )
    if ( t -> health_percentage() < min_health_percentage )
      return false;

  if ( max_health_percentage > 0 )
    if ( t -> health_percentage() > max_health_percentage )
      return false;

  return true;
}

// action_t::init ===========================================================

void action_t::init()
{
  if ( initialized ) return;

  std::string buffer;
  for ( int i=0; i < RESULT_MAX; i++ )
  {
    buffer  = name();
    buffer += "_";
    buffer += util_t::result_type_string( i );
    rng[ i ] = player -> get_rng( buffer, ( ( i == RESULT_CRIT ) ? RNG_DISTRIBUTED : RNG_CYCLIC ) );
  }

  if ( ! sync_str.empty() )
  {
    sync_action = player -> find_action( sync_str );

    if ( ! sync_action )
    {
      sim -> errorf( "Unable to find sync action '%s' for primary action '%s'\n", sync_str.c_str(), name() );
      sim -> cancel();
    }
  }

  if ( ! if_expr_str.empty() && ! is_dtr_action )
  {
    if_expr = action_expr_t::parse( this, if_expr_str );
  }

  if ( ! interrupt_if_expr_str.empty() )
  {
    interrupt_if_expr = action_expr_t::parse( this, interrupt_if_expr_str );
  }

  if ( sim -> travel_variance && travel_speed && player -> distance )
    rng_travel = player -> get_rng( name_str + "_travel", RNG_DISTRIBUTED );

  if ( is_dtr_action )
  {
    cooldown = player -> get_cooldown( name_str + "_DTR" );
    cooldown -> duration = 0;

    stats = player -> get_stats( name_str + "_DTR", this );
    background = true;
  }

  initialized = true;
}

// action_t::reset ==========================================================

void action_t::reset()
{
  cooldown -> reset();
  if( action_dot )
    action_dot -> reset();
  result = RESULT_NONE;
  execute_event = 0;
  travel_event = 0;
}

// action_t::cancel =========================================================

void action_t::cancel()
{
  if ( sim -> debug ) log_t::output( sim, "action %s of %s is canceled", name(), player -> name() );

  if ( channeled )
  {
    dot_t* dot = this -> dot();
    if( dot -> ticking )
    {
      last_tick( dot );
      event_t::cancel( dot -> tick_event );
      dot -> reset();
    }
  }

  if ( player -> executing  == this ) player -> executing  = 0;
  if ( player -> channeling == this ) player -> channeling = 0;

  event_t::cancel( execute_event );

  player -> debuffs.casting -> expire();
}

// action_t::interrupt ======================================================

void action_t::interrupt_action()
{
  if ( sim -> debug ) log_t::output( sim, "action %s of %s is interrupted", name(), player -> name() );

  if ( cooldown -> duration > 0 && ! dual )
  {
    if ( sim -> debug ) log_t::output( sim, "%s starts cooldown for %s (%s)", player -> name(), name(), cooldown -> name() );

    cooldown -> start();
  }

  if ( player -> executing  == this ) player -> executing  = 0;
  if ( player -> channeling == this )
  {
    dot_t* dot = this->dot();
    if ( dot -> ticking ) last_tick( dot );
    player -> channeling = 0;
    event_t::cancel( dot -> tick_event );
    dot -> reset();
  }

  event_t::cancel( execute_event );

  player -> debuffs.casting -> expire();
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

// action_t::check_race =====================================================

void action_t::check_race( int race )
{
  if ( player -> race != race )
  {
    sim -> errorf( "Player %s attempting to execute action %s while not being a %s.\n", player -> name(), name(), util_t::race_type_string( race ) );

    background = true; // prevent action from being executed
  }
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

// action_t::create_expression ==============================================

action_expr_t* action_t::create_expression( const std::string& name_str )
{
  if ( name_str == "ticking" )
  {
    struct ticking_expr_t : public action_expr_t
    {
      ticking_expr_t( action_t* a ) : action_expr_t( a, "ticking", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> dot() -> ticking; return TOK_NUM; }
    };
    return new ticking_expr_t( this );
  }
  if ( name_str == "ticks" )
  {
    struct ticks_expr_t : public action_expr_t
    {
      ticks_expr_t( action_t* a ) : action_expr_t( a, "ticks", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> dot() -> current_tick; return TOK_NUM; }
    };
    return new ticks_expr_t( this );
  }
  if ( name_str == "ticks_remain" )
  {
    struct ticks_remain_expr_t : public action_expr_t
    {
      ticks_remain_expr_t( action_t* a ) : action_expr_t( a, "ticks_remain", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> dot() -> ticks(); return TOK_NUM; }
    };
    return new ticks_remain_expr_t( this );
  }
  if ( name_str == "remains" )
  {
    struct remains_expr_t : public action_expr_t
    {
      remains_expr_t( action_t* a ) : action_expr_t( a, "remains", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> dot() -> remains(); return TOK_NUM; }
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
      virtual int evaluate() { result_num = ( action -> dot() -> ticking ) ? action -> dot() -> action -> tick_time() : 0; return TOK_NUM; }
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
      virtual int evaluate()
      {
        dot_t* dot = action -> dot();
        if ( dot -> miss_time == -1 ||
             action -> sim -> current_time >= ( dot -> miss_time + action -> last_reaction_time ) )
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
  if ( name_str == "cast_delay" )
  {
    struct cast_delay_expr_t : public action_expr_t
    {
      cast_delay_expr_t( action_t* a ) : action_expr_t( a, "cast_delay", TOK_NUM ) {}
      virtual int evaluate()
      {
        if ( action -> sim -> debug )
        {
          log_t::output( action -> sim, "%s %s cast_delay(): can_react_at=%f cur_time=%f",
                         action -> player -> name_str.c_str(),
                         action -> name_str.c_str(),
                         action -> player -> cast_delay_occurred + action -> player -> cast_delay_reaction,
                         action -> sim -> current_time );
        }

        if ( ! action -> player -> cast_delay_occurred ||
             action -> player -> cast_delay_occurred + action -> player -> cast_delay_reaction < action -> sim -> current_time )
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
    return new cast_delay_expr_t( this );
  }

  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, name_str, "." );

  if ( num_splits == 2 )
  {
    if ( splits[ 0 ] == "prev" )
    {
      struct prev_expr_t : public action_expr_t
      {
        std::string prev_action;
        prev_expr_t( action_t* a, const std::string& prev_action ) : action_expr_t( a, "prev", TOK_NUM ), prev_action( prev_action ) {}
        virtual int evaluate()
        {
          result_num = ( action -> player -> last_foreground_action ) ? action -> player -> last_foreground_action -> name_str == prev_action : 0;
          return TOK_NUM;
        }
      };

      return new prev_expr_t( this, splits[ 1 ] );
    }
  }

  if ( num_splits == 3 && ( splits[0] == "buff" || splits[0] == "debuff" || splits[0] == "aura" ) )
  {
    buff_t* buff = sim -> get_targetdata_aura( player, target, splits[1] );
    if( buff )
      return buff -> create_expression( this, splits[ 2 ] );
  }

  if ( num_splits >= 2 && ( splits[ 0 ] == "debuff" || splits[ 0 ] == "dot" ) )
  {
    return target -> create_expression( this, name_str );
  }

  if ( num_splits >= 2 && splits[ 0 ] == "aura" )
  {
    return sim -> create_expression( this, name_str );
  }

  if ( num_splits == 2 && splits[ 0 ] == "target" )
  {  
    return target -> create_expression( this, splits[ 1 ] );
  }

  if ( num_splits > 2 && splits[ 0 ] == "target" )
  {
    // Find target
    player_t* expr_target = sim -> find_player( splits[ 1 ] );
    if ( ! expr_target )
    {
      sim -> errorf( "Unable to find target for %s", name_str.c_str() );
      sim -> cancel();
    }
    
    std::string rest = splits[ 2 ];
    for( int i = 3; i < num_splits; ++i )
      rest += '.' + splits[ i ];

    return expr_target -> create_expression( this, rest );
  }

  // necessary for self.target.*, self.dot.*
  if ( num_splits >= 2 && splits[ 0 ] == "self" )
  {
    std::string rest = splits[1];
    for( int i = 2; i < num_splits; ++i )
      rest += '.' + splits[i];
    return player -> create_expression( this, rest );
  }

  // necessary for sim.target.*
  if ( num_splits >= 2 && splits[ 0 ] == "sim" )
  {
    std::string rest = splits[1];
    for( int i = 2; i < num_splits; ++i )
      rest += '.' + splits[i];
    return sim -> create_expression( this, rest );
  }

  return player -> create_expression( this, name_str );
}

// action_t::ppm_proc_chance ================================================

double action_t::ppm_proc_chance( double PPM ) const
{
  if ( weapon )
  {
    return weapon -> proc_chance_on_swing( PPM );
  }
  else
  {
    double time = channeled ? dot() -> time_to_tick : time_to_execute;

    if ( time == 0 ) time = player -> base_gcd;

    return ( PPM * time / 60.0 );
  }
}

// action_t::tick_time ======================================================

double action_t::tick_time() const
{
  double t = base_tick_time;
  if ( channeled || hasted_ticks )
  {
    t *= player_haste;
  }
  return t;
}

// action_t::hasted_num_ticks ===============================================

int action_t::hasted_num_ticks( double d ) const
{
  if ( ! hasted_ticks ) return num_ticks;

  assert( player_haste > 0.0 );

  // For the purposes of calculating the number of ticks, the tick time is rounded to the 3rd decimal place.
  // It's important that we're accurate here so that we model haste breakpoints correctly.

  if ( d < 0 )
    d = num_ticks * base_tick_time;

  double t = floor( ( base_tick_time * player_haste * 1000.0 ) + 0.5 ) / 1000.0;

  double n = d / t;

  // banker's rounding
  if ( n - 0.5 == ( double ) ( int ) n && ( ( int ) n ) % 2 == 0 )
    return ( int ) ceil ( n - 0.5 );

  return ( int ) floor( n + 0.5 );
}
