// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

static const char* class_color( int type )
{
  switch( type )
  {
  case PLAYER_NONE:  return "FFFFFF";
  case DEATH_KNIGHT: return "C41F3B";
  case DRUID:        return "FF7D0A";
  case HUNTER:       return "9BB453";
  case MAGE:         return "69CCF0";
  case PALADIN:      return "F58CBA";
  case PRIEST:       return "333333";
  case ROGUE:        return "E09000";
  case SHAMAN:       return "2459FF";
  case WARLOCK:      return "9482CA";
  case WARRIOR:      return "C79C6E";
  default: assert(0);
  }
  return 0;
}

static const char* school_color( int type )
{
  switch( type )
  {
  case SCHOOL_ARCANE:    return class_color( DRUID );
  case SCHOOL_BLEED:     return class_color( WARRIOR );
  case SCHOOL_CHAOS:     return class_color( DEATH_KNIGHT );
  case SCHOOL_FIRE:      return class_color( DEATH_KNIGHT );
  case SCHOOL_FROST:     return class_color( MAGE );
  case SCHOOL_FROSTFIRE: return class_color( PALADIN );
  case SCHOOL_HOLY:      return class_color( ROGUE );
  case SCHOOL_NATURE:    return class_color( SHAMAN );
  case SCHOOL_PHYSICAL:  return class_color( WARRIOR );
  case SCHOOL_SHADOW:    return class_color( WARLOCK );
  default: assert(0);
  }
  return 0;
}

static const char* get_color( player_t* p )
{
  if( p -> is_pet() ) 
  {
    return class_color( p -> cast_pet() -> owner -> type );
  }
  return class_color( p -> type );
}

static unsigned char simple_encoding( int number )
{
  if( number < 0  ) number = 0;
  if( number > 61 ) number = 61;

  static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  return encoding[ number ];
}

#if 0
static const char* extended_encoding( int number )
{
  if( number < 0    ) number = 0;
  if( number > 4095 ) number = 4095;

  static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  int first  = number / 64;
  int second = number - ( first * 64 );

  std::string pair;

  pair = "";
  pair += encoding[ first  ];
  pair += encoding[ second ];

  return pair.c_str();
}
#endif

} // ANONYMOUS NAMESPACE =====================================================

// ==========================================================================
// Chart
// ==========================================================================

// chart_t::raid_dps =========================================================

int chart_t::raid_dps( std::vector<std::string>& images, 
                       sim_t* sim )
{
  int num_players = sim -> players_by_rank.size();
  assert( num_players != 0 );

  double max_dps = sim -> players_by_rank[ 0 ] -> dps;

  std::string s;
  char buffer[ 1024 ];

  std::vector<player_t*> player_list = sim -> players_by_rank;
  int max_players = 25;

  while( true )
  {
    if( num_players > max_players ) num_players = max_players;

    s = "http://chart.apis.google.com/chart?";
    snprintf( buffer, sizeof(buffer), "chs=450x%d", num_players * 20 + 30 ); s += buffer;
    s += "&";
    s += "cht=bhg";
    s += "&";
    s += "chbh=15";
    s += "&";
    s += "chd=t:";

    for( int i=0; i < num_players; i++ )
    {
      player_t* p = player_list[ i ];
      snprintf( buffer, sizeof(buffer), "%s%.0f", (i?"|":""), p -> dps ); s += buffer;
    }
    s += "&";
    snprintf( buffer, sizeof(buffer), "chds=0,%.0f", max_dps * 2.5 ); s += buffer;
    s += "&";
    s += "chco=";
    for( int i=0; i < num_players; i++ )
    {
      if( i ) s += ",";
      s += get_color( player_list[ i ] );
    }
    s += "&";
    s += "chm=";
    for( int i=0; i < num_players; i++ )
    {
      player_t* p = player_list[ i ];
      snprintf( buffer, sizeof(buffer), "%st++%.0f++%s,%s,%d,0,15", (i?"|":""), p -> dps, p -> name(), get_color( p ), i ); s += buffer;
    }
    s += "&";
    s += "chtt=DPS+Ranking";
    s += "&";
    s += "chts=000000,20";

    images.push_back( s );

    player_list.erase( player_list.begin(), player_list.begin() + num_players );
    num_players = player_list.size();
    if( num_players == 0 ) break;
  }

  return images.size();
}

// chart_t::raid_gear ========================================================

int chart_t::raid_gear( std::vector<std::string>& images, 
                        sim_t* sim )
{
  int num_players = sim -> players_by_rank.size();
  assert( num_players != 0 );

  std::vector<double> data_points[ STAT_MAX ];

  for( int i=0; i < STAT_MAX; i++ )
  {
    data_points[ i ].insert( data_points[ i ].begin(), num_players, 0.0 );

    for( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_rank[ j ];

      data_points[ i ][ j ] = p -> equip_stats.get_stat( i ) * gear_stats_t::stat_mod( i );
    }
  }

  double max_total=0;
  for( int i=0; i < num_players; i++ )
  {
    double total=0;
    for( int j=0; j < STAT_MAX; j++ )
    {
      total += data_points[ j ][ i ];
    }
    if( total > max_total ) max_total = total;
  }

  const char* colors[ STAT_MAX ];

  for( int i=0; i < STAT_MAX; i++ ) colors[ i ] = 0;

  colors[ STAT_STRENGTH                 ] = class_color( WARRIOR );
  colors[ STAT_AGILITY                  ] = class_color( HUNTER );
  colors[ STAT_INTELLECT                ] = class_color( MAGE );
  colors[ STAT_SPIRIT                   ] = class_color( DRUID );
  colors[ STAT_MP5                      ] = class_color( DRUID );
  colors[ STAT_ATTACK_POWER             ] = class_color( ROGUE );
  colors[ STAT_SPELL_POWER              ] = class_color( WARLOCK );
  colors[ STAT_HIT_RATING               ] = class_color( PRIEST );
  colors[ STAT_CRIT_RATING              ] = class_color( PALADIN );
  colors[ STAT_HASTE_RATING             ] = class_color( SHAMAN );
  colors[ STAT_EXPERTISE_RATING         ] = class_color( DEATH_KNIGHT );
  colors[ STAT_ARMOR_PENETRATION_RATING ] = "00FF00";
  colors[ STAT_SPELL_PENETRATION        ] = "00FF00";

  std::string s;
  char buffer[ 1024 ];
  bool first;

  std::vector<player_t*> player_list = sim -> players_by_rank;
  int max_players = 25;

  while( true )
  {
    if( num_players > max_players ) num_players = max_players;

    s = "http://chart.apis.google.com/chart?";
    snprintf( buffer, sizeof(buffer), "chs=450x%d", num_players * 20 + 30 ); s += buffer;
    s += "&";
    s += "cht=bhs";
    s += "&";
    s += "chbh=15";
    s += "&";
    s += "chd=t:";
    first = true;
    for( int i=0; i < STAT_MAX; i++ )
    {
      if( ! colors[ i ] ) continue;
      if( ! first ) s += "|";
      first = false;
      for( int j=0; j < num_players; j++ )
      {
        snprintf( buffer, sizeof(buffer), "%s%.0f", (j?",":""), data_points[ i ][ j ] ); s += buffer;
      }
    }
    s += "&";
    snprintf( buffer, sizeof(buffer), "chds=0,%.0f", max_total ); s += buffer;
    s += "&";
    s += "chco=";
    first = true;
    for( int i=0; i < STAT_MAX; i++ )
    {
      if( ! colors[ i ] ) continue;
      if( ! first ) s += ",";
      first = false;
      s += colors[ i ];
    }
    s += "&";
    s += "chxt=y";
    s += "&";
    s += "chxl=0:";
    for( int i = num_players-1; i >= 0; i-- )
    {
      s += "|";
      s += player_list[ i ] -> name();
    }
    s += "&";
    s += "chxs=0,000000,15";
    s += "&";
    s += "chdl=";
    first = true;
    for( int i=0; i < STAT_MAX; i++ )
    {
      if( ! colors[ i ] ) continue;
      if( ! first ) s += "|";
      first = false;
      s += util_t::stat_type_abbrev( i );
    }
    s += "&";
    s += "chtt=Gear+Overview";
    s += "&";
    s += "chts=000000,20";

    images.push_back( s );

    for( int i=0; i < STAT_MAX; i++ ) 
    {
      std::vector<double>& c = data_points[ i ];
      c.erase( c.begin(), c.begin() + num_players );
    }

    player_list.erase( player_list.begin(), player_list.begin() + num_players );
    num_players = player_list.size();
    if( num_players == 0 ) break;
  }

  return images.size();
}

// chart_t::raid_downtime =====================================================

const char* chart_t::raid_downtime( std::string& s,
                                    sim_t* sim )
{
  int num_players = sim -> players_by_name.size();
  assert( num_players != 0 );
  
  std::vector<player_t*> waiting_list;

  for( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];
    if( ( p -> total_waiting / p -> total_seconds ) > 0.01 ) 
    {
      waiting_list.push_back( p );
    }
  }

  int num_waiting = waiting_list.size();
  if( num_waiting == 0 ) return 0;

  char buffer[ 1024 ];

  s = "http://chart.apis.google.com/chart?";
  snprintf( buffer, sizeof(buffer), "chs=500x%d", num_waiting * 30 + 30 ); s += buffer;
  s += "&";
  s += "cht=bhg";
  s += "&";
  s += "chd=t:";
  double max_waiting=0;
  for( int i=0; i < num_waiting; i++ )
  {
    player_t* p = waiting_list[ i ];
    double waiting = 100.0 * p -> total_waiting / p -> total_seconds;
    if( waiting > max_waiting ) max_waiting = waiting;
    snprintf( buffer, sizeof(buffer), "%s%.0f", (i?"|":""), waiting ); s += buffer;
  }
  s += "&";
  snprintf( buffer, sizeof(buffer), "chds=0,%.0f", max_waiting * 2 ); s += buffer;
  s += "&";
  s += "chco=";
  for( int i=0; i < num_waiting; i++ )
  {
    if( i ) s += ",";
    s += get_color( waiting_list[ i ] );
  }
  s += "&";
  s += "chm=";
  for( int i=0; i < num_waiting; i++ )
  {
    player_t* p = waiting_list[ i ];
    snprintf( buffer, sizeof(buffer), "%st++%.0f%%++%s,%s,%d,0,15", (i?"|":""), 100.0 * p -> total_waiting / p -> total_seconds, p -> name(), get_color( p ), i ); s += buffer;
  }
  s += "&";
  s += "chtt=Raid+Down-Time";
  s += "&";
  s += "chts=000000,20";

  return s.c_str();
}

// chart_t::raid_uptimes ========================================================

const char* chart_t::raid_uptimes( std::string& s,
                                   sim_t* sim )
{
  std::vector<uptime_t*> uptime_list;

  for( uptime_t* u = sim -> target -> uptime_list; u; u = u -> next )
  {
    if( u -> up <= 0 ) continue;
    uptime_list.push_back( u );
  }

  int num_uptimes = uptime_list.size();
  if( num_uptimes == 0 ) return 0;

  char buffer[ 1024 ];

  s = "http://chart.apis.google.com/chart?";
  snprintf( buffer, sizeof(buffer), "chs=500x%d", num_uptimes * 30 + 30 ); s += buffer;
  s += "&";
  s += "cht=bhs";
  s += "&";
  s += "chd=t:";
  for( int i=0; i < num_uptimes; i++ )
  {
    uptime_t* u = uptime_list[ i ];
    snprintf( buffer, sizeof(buffer), "%s%.0f", (i?",":""), u -> percentage() ); s += buffer;
  }
  s += "&";
  s += "chds=0,200";
  s += "&";
  s += "chm=";
  for( int i=0; i < num_uptimes; i++ )
  {
    uptime_t* u = uptime_list[ i ];
    snprintf( buffer, sizeof(buffer), "%st++%.0f%%++%s,000000,0,%d,15", (i?"|":""), u -> percentage(), u -> name(), i ); s += buffer;
  }
  s += "&";
  s += "chtt=Global+Up-Times";
  s += "&";
  s += "chts=000000,20";
  
  return s.c_str();
}

// chart_t::raid_dpet ========================================================

struct compare_dpet {
  bool operator()( stats_t* l, stats_t* r ) const
  {
    return l -> dpet > r -> dpet;
  }
};

int chart_t::raid_dpet( std::vector<std::string>& images, 
                        sim_t* sim )
{
  int num_players = sim -> players_by_rank.size();
  assert( num_players != 0 );

  std::vector<stats_t*> stats_list;

  for( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_rank[ i ];

    for( stats_t* st = p -> stats_list; st; st = st -> next )
    {
      if( st -> total_dmg <= 0 ) continue;
      if( ! st -> channeled && st -> total_execute_time <= 0 ) continue;
      if( st -> num_executes < 5 ) continue;

      stats_list.push_back( st );
    }
  }

  int num_stats = stats_list.size();
  if( num_stats == 0 ) return 0;

  std::sort( stats_list.begin(), stats_list.end(), compare_dpet() );

  double max_dpet = stats_list[ 0 ] -> dpet;

  int max_actions_per_chart = 25;
  int max_charts = 4;

  std::string s;
  char buffer[ 1024 ];

  for( int chart=0; chart < max_charts; chart++ )
  {
    if( num_stats > max_actions_per_chart ) num_stats = max_actions_per_chart;

    s = "http://chart.apis.google.com/chart?";
    snprintf( buffer, sizeof(buffer), "chs=500x%d", num_stats * 15 + 30 ); s += buffer;
    s += "&";
    s += "cht=bhg";
    s += "&";
    s += "chbh=10";
    s += "&";
    s += "chd=t:";
    for( int i=0; i < num_stats; i++ )
    {
      stats_t* st = stats_list[ i ];
      snprintf( buffer, sizeof(buffer), "%s%.0f", (i?"|":""), st -> dpet ); s += buffer;
    }
    s += "&";
    snprintf( buffer, sizeof(buffer), "chds=0,%.0f", max_dpet * 2.5 ); s += buffer;
    s += "&";
    s += "chco=";
    for( int i=0; i < num_stats; i++ )
    {
      if( i ) s += ",";
      s += get_color( stats_list[ i ] -> player );
    }
    s += "&";
    s += "chm=";
    for( int i=0; i < num_stats; i++ )
    {
      stats_t* st = stats_list[ i ];
      snprintf( buffer, sizeof(buffer), "%st++%.0f++%s+(%s),%s,%d,0,10", (i?"|":""), 
               st -> dpet, st -> name_str.c_str(), st -> player -> name(), get_color( st -> player ), i ); s += buffer;
    }
    s += "&";
    s += "chtt=Raid+Damage+Per+Execute+Time";
    s += "&";
    s += "chts=000000,20";

    images.push_back( s );

    stats_list.erase( stats_list.begin(), stats_list.begin() + num_stats );
    num_stats = stats_list.size();
    if( num_stats == 0 ) break;
  }

  return images.size();
}

// chart_t::action_dpet ======================================================

const char* chart_t::action_dpet( std::string& s, 
                                  player_t* p )
{
  std::vector<stats_t*> stats_list;

  for( stats_t* st = p -> stats_list; st; st = st -> next )
  {
    if( st -> total_dmg <= 0 ) continue;
    if( ! st -> channeled && st -> total_execute_time <= 0 ) continue;
    if( st -> dpet > ( 5 * p -> dps ) ) continue;

    stats_list.push_back( st );
  }

  for( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    for( stats_t* st = pet -> stats_list; st; st = st -> next )
    {
      if( st -> total_dmg <= 0 ) continue;
      if( ! st -> channeled && st -> total_execute_time <= 0 ) continue;
      if( st -> dpet > ( 10 * p -> dps ) ) continue;
      
      stats_list.push_back( st );
    }
  }

  int num_stats = stats_list.size();
  if( num_stats == 0 ) return 0;

  std::sort( stats_list.begin(), stats_list.end(), compare_dpet() );

  char buffer[ 1024 ];

  s = "http://chart.apis.google.com/chart?";
  snprintf( buffer, sizeof(buffer), "chs=500x%d", num_stats * 30 + 65 ); s += buffer;
  s += "&";
  s += "cht=bhg";
  s += "&";
  s += "chd=t:";
  double max_dpet=0;
  for( int i=0; i < num_stats; i++ )
  {
    stats_t* st = stats_list[ i ];
    snprintf( buffer, sizeof(buffer), "%s%.0f", (i?"|":""), st -> dpet ); s += buffer;
    if( st -> dpet > max_dpet ) max_dpet = st -> dpet;
  }
  s += "&";
  snprintf( buffer, sizeof(buffer), "chds=0,%.0f", max_dpet * 2 ); s += buffer;
  s += "&";
  s += "chco=";
  for( int i=0; i < num_stats; i++ )
  {
    if( i ) s += ",";
    s += school_color( stats_list[ i ] -> school );
  }
  s += "&";
  s += "chm=";
  for( int i=0; i < num_stats; i++ )
  {
    stats_t* st = stats_list[ i ];
    snprintf( buffer, sizeof(buffer), "%st++%.0f++%s,%s,%d,0,15", (i?"|":""), st -> dpet, st -> name_str.c_str(), school_color( st -> school ), i ); s += buffer;
  }
  s += "&";
  snprintf( buffer, sizeof(buffer), "chtt=%s|Damage+Per+Execute+Time", p -> name() ); s += buffer;
  s += "&";
  s += "chts=000000,20";

  return s.c_str();
}

// chart_t::action_dmg =======================================================

struct compare_dmg {
  bool operator()( stats_t* l, stats_t* r ) const
  {
    return l -> total_dmg > r -> total_dmg;
  }
};

const char* chart_t::action_dmg( std::string& s, 
                                 player_t* p )
{
  std::vector<stats_t*> stats_list;

  for( stats_t* st = p -> stats_list; st; st = st -> next )
  {
    if( st -> total_dmg <= 0 ) continue;
    stats_list.push_back( st );
  }

  for( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
  {
    for( stats_t* st = pet -> stats_list; st; st = st -> next )
    {
      if( st -> total_dmg <= 0 ) continue;
      stats_list.push_back( st );
    }
  }

  int num_stats = stats_list.size();
  if( num_stats == 0 ) return 0;

  std::sort( stats_list.begin(), stats_list.end(), compare_dmg() );

  char buffer[ 1024 ];

  s = "http://chart.apis.google.com/chart?";
  snprintf( buffer, sizeof(buffer), "chs=500x%d", 200 + num_stats * 10 ); s += buffer;
  s += "&";
  s += "cht=p";
  s += "&";
  s += "chd=t:";
  for( int i=0; i < num_stats; i++ )
  {
    stats_t* st = stats_list[ i ];
    snprintf( buffer, sizeof(buffer), "%s%.0f", (i?",":""), 100.0 * st -> total_dmg / p -> total_dmg ); s += buffer;
  }
  s += "&";
  s += "chds=0,100";
  s += "&";
  s += "chco=";
  for( int i=0; i < num_stats; i++ )
  {
    if( i ) s += ",";
    s += school_color( stats_list[ i ] -> school );
  }
  s += "&";
  s += "chl=";
  for( int i=0; i < num_stats; i++ )
  {
    if( i ) s += "|";
    s += stats_list[ i ] -> name_str.c_str();
  }
  s += "&";
  snprintf( buffer, sizeof(buffer), "chtt=%s+Damage+Sources", p -> name() ); s += buffer;
  s += "&";
  s += "chts=000000,20";

  return s.c_str();
}

// chart_t::gains ============================================================

struct compare_gain {
  bool operator()( gain_t* l, gain_t* r ) const
  {
    return l -> actual > r -> actual;
  }
};

const char* chart_t::gains( std::string& s, 
                            player_t* p )
{
  std::vector<gain_t*> gains_list;

  double total_gain=0;
  for( gain_t* g = p -> gain_list; g; g = g -> next )
  {
    if( g -> actual <= 0 ) continue;
    total_gain += g -> actual;
    gains_list.push_back( g );
  }

  int num_gains = gains_list.size();
  if( num_gains == 0 ) return 0;

  std::sort( gains_list.begin(), gains_list.end(), compare_gain() );

  char buffer[ 1024 ];

  s = "http://chart.apis.google.com/chart?";
  snprintf( buffer, sizeof(buffer), "chs=550x%d", 200 + num_gains * 10 ); s += buffer;
  s += "&";
  s += "cht=p";
  s += "&";
  s += "chd=t:";
  for( int i=0; i < num_gains; i++ )
  {
    gain_t* g = gains_list[ i ];
    snprintf( buffer, sizeof(buffer), "%s%.0f", (i?",":""), 100.0 * g -> actual / total_gain ); s += buffer;
  }
  s += "&";
  s += "chds=0,100";
  s += "&";
  s += "chco=";  
  s += get_color( p );
  s += "&";
  s += "chl=";
  for( int i=0; i < num_gains; i++ )
  {
    if( i ) s += "|";
    s += gains_list[ i ] -> name();
  }
  s += "&";
  snprintf( buffer, sizeof(buffer), "chtt=%s+Resource+Gains", p -> name() ); s += buffer;
  s += "&";
  s += "chts=000000,20";

  return s.c_str();
}

// chart_t::uptimes_and_procs ==================================================

const char* chart_t::uptimes_and_procs( std::string& s, 
                                        player_t* p )
{
  std::vector<uptime_t*> uptime_list;
  std::vector<proc_t*>     proc_list;

  for( uptime_t* u = p -> uptime_list; u; u = u -> next )
  {
    if( u -> up <= 0 ) continue;
    if( floor( u -> percentage() ) <= 0 ) continue;
    uptime_list.push_back( u );
  }

  double max_proc_count=0;
  for( proc_t* proc = p -> proc_list; proc; proc = proc -> next )
  {
    if( floor( proc -> count ) <= 0 ) continue;
    if( proc -> count > max_proc_count ) max_proc_count = proc -> count;
    proc_list.push_back( proc );
  }

  int num_uptimes = uptime_list.size();
  int num_procs   =   proc_list.size();

  if( num_uptimes == 0 && num_procs == 0 ) return 0;

  char buffer[ 1024 ];

  s = "http://chart.apis.google.com/chart?";
  snprintf( buffer, sizeof(buffer), "chs=500x%d", ( num_uptimes + num_procs ) * 30 + 65 ); s += buffer;
  s += "&";
  s += "cht=bhg";
  s += "&";
  s += "chd=t:";
  for( int i=0; i < num_uptimes; i++ )
  {
    uptime_t* u = uptime_list[ i ];
    snprintf( buffer, sizeof(buffer), "%s%.0f", (i?"|":""), u -> percentage() ); s += buffer;
  }
  for( int i=0; i < num_procs; i++ )
  {
    proc_t* proc = proc_list[ i ];
    snprintf( buffer, sizeof(buffer), "%s%.0f", ((num_uptimes+i)?"|":""), 100 * proc -> count / max_proc_count ); s += buffer;
  }
  s += "&";
  s += "chds=0,200";
  s += "&";
  s += "chco=";
  for( int i=0; i < num_uptimes; i++ )
  {
    if( i ) s += ",";
    s += "00FF00";
  }
  for( int i=0; i < num_procs; i++ )
  {
    if( num_uptimes+i) s += ",";
    s += "FF0000";
  }
  s += "&";
  s += "chm=";
  for( int i=0; i < num_uptimes; i++ )
  {
    uptime_t* u = uptime_list[ i ];
    snprintf( buffer, sizeof(buffer), "%st++%.0f%%++%s,000000,%d,0,15", (i?"|":""), u -> percentage(), u -> name(), i ); s += buffer;
  }
  for( int i=0; i < num_procs; i++ )
  {
    proc_t* proc = proc_list[ i ];
    snprintf( buffer, sizeof(buffer), "%st++%.0f+(%.2fsec)++%s,000000,%d,0,15", ((num_uptimes+i)?"|":""), 
             proc -> count, proc -> frequency, proc -> name(), num_uptimes+i ); s += buffer;
  }
  s += "&";
  snprintf( buffer, sizeof(buffer), "chtt=%s|Up-Times+and+Procs+(%.0fsec)", p -> name(), p -> sim -> total_seconds ); s += buffer;
  s += "&";
  s += "chts=000000,20";

  return s.c_str();
}

// chart_t::timeline_dps =====================================================

const char* chart_t::timeline_dps( std::string& s, 
                                   player_t* p )
{
  int max_buckets = p -> timeline_dps.size();
  int max_points  = 600;
  int increment   = 1;

  if( max_buckets <= max_points )
  {
    max_points = max_buckets;
  }
  else
  {
    increment = ( (int) floor( max_buckets / (double) max_points ) ) + 1;
  }

  double dps_max=0;
  for( int i=0; i < max_buckets; i++ ) 
  {
    if( p -> timeline_dps[ i ] > dps_max ) 
    {
      dps_max = p -> timeline_dps[ i ];
    }
  }
  double dps_range  = 60.0;
  double dps_adjust = dps_range / dps_max;

  char buffer[ 1024 ];

  s = "http://chart.apis.google.com/chart?";
  s += "chs=425x130";
  s += "&";
  s += "cht=lc";
  s += "&";
  s += "chd=s:";
  for( int i=0; i < max_buckets; i += increment ) 
  {
    s += simple_encoding( (int) ( p -> timeline_dps[ i ] * dps_adjust ) );
  }
  s += "&";
  snprintf( buffer, sizeof(buffer), "chds=0,%.0f", dps_range ); s += buffer;
  s += "&";
  s += "chxt=x,y";
  s += "&";
  snprintf( buffer, sizeof(buffer), "chxl=0:|0|sec=%d|1:|0|avg=%.0f|max=%.0f", max_buckets, p -> dps, dps_max ); s += buffer;
  s += "&";
  snprintf( buffer, sizeof(buffer), "chxp=1,1,%.0f,100", 100.0 * p -> dps / dps_max ); s += buffer;
  s += "&";
  snprintf( buffer, sizeof(buffer), "chtt=%s+DPS+Timeline", p -> name() ); s += buffer;
  s += "&";
  s += "chts=000000,20";

  return s.c_str();
}

// chart_t::timeline_resource ================================================

const char* chart_t::timeline_resource( std::string& s, 
                                        player_t* p )
{
  if( p -> primary_resource() == RESOURCE_NONE ) return 0;

  int max_buckets = p -> timeline_resource.size();
  int max_points  = 600;
  int increment   = 1;

  if( max_buckets <= 0 ) return 0;

  if( max_buckets <= max_points )
  {
    max_points = max_buckets;
  }
  else
  {
    increment = ( (int) floor( max_buckets / (double) max_points ) ) + 1;
  }

  double resource_max=0;
  for( int i=0; i < max_buckets; i++ ) 
  {
    if( p -> timeline_resource[ i ] > resource_max ) 
    {
      resource_max = p -> timeline_resource[ i ];
    }
  }
  double resource_range  = 60.0;
  double resource_adjust = resource_range / resource_max;

  char buffer[ 1024 ];

  s = "http://chart.apis.google.com/chart?";
  s += "chs=425x150";
  s += "&";
  s += "cht=lc";
  s += "&";
  s += "chd=s:";
  for( int i=0; i < max_buckets; i += increment ) 
  {
    s += simple_encoding( (int) ( p -> timeline_resource[ i ] * resource_adjust ) );
  }
  s += "&";
  snprintf( buffer, sizeof(buffer), "chds=0,%.0f", resource_range ); s += buffer;
  s += "&";
  s += "chxt=x,y";
  s += "&";
  snprintf( buffer, sizeof(buffer), "chxl=0:|0|sec=%d|1:|0|max=%.0f", max_buckets, resource_max ); s += buffer;
  s += "&";
  snprintf( buffer, sizeof(buffer), "chtt=%s|Resource+(%s)+Timeline", p -> name(), util_t::resource_type_string( p -> primary_resource() ) ); s += buffer;
  s += "&";
  s += "chts=000000,20";

  return s.c_str();
}

// chart_t::distribution_dps ==================================================

const char* chart_t::distribution_dps( std::string& s, 
                                       player_t* p )
{
  int max_buckets = p -> distribution_dps.size();

  int count_max=0;
  for( int i=0; i < max_buckets; i++ ) 
  {
    if( p -> distribution_dps[ i ] > count_max ) 
    {
      count_max = p -> distribution_dps[ i ];
    }
  }

  char buffer[ 1024 ];

  s = "http://chart.apis.google.com/chart?";
  s += "chs=525x130";
  s += "&";
  s += "cht=bvs";
  s += "&";
  s += "chd=t:";
  for( int i=0; i < max_buckets; i++ ) 
  {
    snprintf( buffer, sizeof(buffer), "%s%d", (i?",":""), p -> distribution_dps[ i ] ); s += buffer;
  }
  s += "&";
  snprintf( buffer, sizeof(buffer), "chds=0,%d", count_max ); s += buffer;
  s += "&";
  s += "chbh=5";
  s += "&";
  s += "chxt=x";
  s += "&";
  snprintf( buffer, sizeof(buffer), "chxl=0:|min=%.0f|avg=%.0f|max=%.0f", p -> dps_min, p -> dps, p -> dps_max ); s += buffer;
  s += "&";
  snprintf( buffer, sizeof(buffer), "chxp=0,1,%.0f,100", 100.0 * ( p -> dps - p -> dps_min ) / ( p -> dps_max - p -> dps_min ) ); s += buffer;
  s += "&";
  snprintf( buffer, sizeof(buffer), "chtt=%s+DPS+Distribution", p -> name() ); s += buffer;
  s += "&";
  s += "chts=000000,20";

  return s.c_str();
}

// chart_t::gear_weights_lootrank =============================================

const char* chart_t::gear_weights_lootrank( std::string& s,
                                            player_t*    p )
{
  char buffer[ 1024 ];

  s = "http://www.lootrank.com/wow/wr.asp?";

  // Restrict lootrank to rare gems until epic gems become available
  // Gem: Not Set = Epic, 2=Common, 3=Rare
  s += "Gem=3&";

  switch( p -> type )
  {
  case DEATH_KNIGHT: s += "Cla=2048"; break;
  case DRUID:        s += "Cla=1024"; break;
  case HUNTER:       s += "Cla=4";    break; 
  case MAGE:         s += "Cla=128";  break;
  case PALADIN:      s += "Cla=2";    break;
  case PRIEST:       s += "Cla=16";   break;
  case ROGUE:        s += "Cla=8";    break;
  case SHAMAN:       s += "Cla=64";   break;
  case WARLOCK:      s += "Cla=256";  break;
  case WARRIOR:      s += "Cla=1";    break;
  default: assert(0);
  }

  for( int i=0; i < STAT_MAX; i++ )
  {
    double value = p -> scaling.get_stat( i );
    if( value == 0 ) continue;

    const char* name=0;
    switch( i )
    {
    case STAT_STRENGTH:                 name = "Str";  break;
    case STAT_AGILITY:                  name = "Agi";  break;
    case STAT_STAMINA:                  name = "Sta";  break;
    case STAT_INTELLECT:                name = "Int";  break;
    case STAT_SPIRIT:                   name = "Spi";  break;
    case STAT_SPELL_POWER:              name = "spd";  break;
    case STAT_ATTACK_POWER:             name = "map";  break;
    case STAT_EXPERTISE_RATING:         name = "Exp";  break;
    case STAT_ARMOR_PENETRATION_RATING: name = "arp";  break;
    case STAT_HIT_RATING:               name = "mhit"; break;
    case STAT_CRIT_RATING:              name = "mcr";  break;
    case STAT_HASTE_RATING:             name = "mh";   break;
    }

    if( name )
    {
      snprintf( buffer, sizeof(buffer), "&%s=%.2f", name, value );
      s += buffer;
    }
  }

  s += "&Ver=6&usr=&ser=&grp=www";

  return s.c_str();
}

// chart_t::gear_weights_wowhead ==============================================

const char* chart_t::gear_weights_wowhead( std::string& s,
                                           player_t*    p )
{
  char buffer[ 1024 ];
  bool first=true;

  s = "http://www.wowhead.com/?items&filter=";

  switch( p -> type )
  {
  case DEATH_KNIGHT: s += "ub=6;";  break;
  case DRUID:        s += "ub=11;"; break;
  case HUNTER:       s += "ub=3;";  break; 
  case MAGE:         s += "ub=8;";  break;
  case PALADIN:      s += "ub=2;";  break;
  case PRIEST:       s += "ub=5;";  break;
  case ROGUE:        s += "ub=4;";  break;
  case SHAMAN:       s += "ub=7;";  break;
  case WARLOCK:      s += "ub=9;";  break;
  case WARRIOR:      s += "ub=1;";  break;
  default: assert(0);
  }

  // Restrict wowhead to rare gems. When epic gems become available:"gm=4;gb=1;"
  s += "gm=3;gb=1;";

  std::string    id_string = "";
  std::string value_string = "";

  for( int i=0; i < STAT_MAX; i++ )
  {
    double value = p -> scaling.get_stat( i );
    if( value == 0 ) continue;

    int id=0;
    switch( i )
    {
    case STAT_STRENGTH:                 id = 20;  break;
    case STAT_AGILITY:                  id = 21;  break;
    case STAT_STAMINA:                  id = 22;  break;
    case STAT_INTELLECT:                id = 23;  break;
    case STAT_SPIRIT:                   id = 24;  break;
    case STAT_SPELL_POWER:              id = 123; break;
    case STAT_ATTACK_POWER:             id = 77;  break;
    case STAT_EXPERTISE_RATING:         id = 117; break;
    case STAT_ARMOR_PENETRATION_RATING: id = 114; break;
    case STAT_HIT_RATING:               id = 119; break;
    case STAT_CRIT_RATING:              id = 96;  break;
    case STAT_HASTE_RATING:             id = 103; break;
    }

    if( id )
    {
      if( ! first )
      {
	   id_string += ":";
	value_string += ":";
      }
      first = false;

      snprintf( buffer, sizeof(buffer), "%d", id );
      id_string += buffer;

      snprintf( buffer, sizeof(buffer), "%.2f", value );
      value_string += buffer;
    }
  }

  s += "wt="  +    id_string + ";";
  s += "wtv=" + value_string + ";";

  return s.c_str();
}

// chart_t::gear_weights_pawn =============================================

struct compare_stat_scale_factors {
  player_t* player;
  compare_stat_scale_factors( player_t* p ) : player(p) {}
  bool operator()( const int& l, const int& r ) const
  {
    return( player -> scaling.get_stat( l ) > 
            player -> scaling.get_stat( r ) );
  }
};

const char* chart_t::gear_weights_pawn( std::string& s,
                                        player_t*    p )
{
  std::vector<int> stats;
  for( int i=0; i < STAT_MAX; i++ ) stats.push_back( i );
  std::sort( stats.begin(), stats.end(), compare_stat_scale_factors( p ) );

  char buffer[ 1024 ];
  bool first = true;

  s = "";
  snprintf( buffer, sizeof(buffer), "( Pawn: v1: \"%s\": ", p -> name() );
  s += buffer;

  for( int i=0; i < STAT_MAX; i++ )
  {
    int stat = stats[ i ];

    double value = p -> scaling.get_stat( stat );
    if( value == 0 ) continue;

    const char* name=0;
    switch( stat )
    {
    case STAT_STRENGTH:                 name = "Strength";         break;
    case STAT_AGILITY:                  name = "Agility";          break;
    case STAT_STAMINA:                  name = "Stamina";          break;
    case STAT_INTELLECT:                name = "Intellect";        break;
    case STAT_SPIRIT:                   name = "Spirit";           break;
    case STAT_SPELL_POWER:              name = "SpellDamage";      break;
    case STAT_ATTACK_POWER:             name = "AP";               break;
    case STAT_EXPERTISE_RATING:         name = "ExpertiseRating";  break;
    case STAT_ARMOR_PENETRATION_RATING: name = "ArmorPenetration"; break;
    case STAT_HIT_RATING:               name = "HitRating";        break;
    case STAT_CRIT_RATING:              name = "CritRating";       break;
    case STAT_HASTE_RATING:             name = "HasteRating";      break;
    }

    if( name )
    {
      if( ! first ) s += ",";
      first = false;
      snprintf( buffer, sizeof(buffer), " %s=%.2f", name, value );
      s += buffer;
    }
  }
   
  s += " )";

  return s.c_str();
}
