// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include <math.h>
static const std::string amp = "&amp;";

// chart option overview: http://code.google.com/intl/de-DE/apis/chart/image/docs/chart_params.html

enum chart_e { HORIZONTAL_BAR, PIE };
enum fill_area_e { FILL_BACKGROUND };
enum fill_e { FILL_SOLID };

static std::string chart_type( chart_e t )
{
  std::ostringstream s;
  s << "cht=";

  switch ( t )
  {
  case HORIZONTAL_BAR:
    s << "bhg";
    break;
  case PIE:
    s << "p";
    break;
  default: break;
  }
  s << amp;
  return s.str();
}

static std::string chart_size( unsigned width, unsigned height )
{
  std::ostringstream s;
  s << "chs=";
  s << width;
  s << "x";
  s << height;
  s << amp;
  return s.str();
}

static std::string fill_chart( fill_area_e fa, fill_e ft, const std::string& color )
{
  std::ostringstream s;

  s << "chf=";

  switch ( fa )
  {
  case FILL_BACKGROUND:
    s << "bg";
    break;
  default: break;
  }
  s <<    ",";
  switch ( ft )
  {
  case FILL_SOLID:
    s << "s";
    break;
  default: break;
  }
  s << ",";

  s << color;

  s << amp;

  return s.str();
}

static std::string chart_title( const std::string& t )
{
  std::ostringstream s;

  s << "chtt=";
  s << t;
  s << amp;

  return s.str();
}

static std::string chart_title_formatting ( const std::string& color, unsigned font_size )
{
  std::ostringstream s;

  s << "chts=";
  s << color;
  s << ",";
  s << font_size;
  s << amp;

  return s.str();
}

namespace color {
// http://www.brobstsystems.com/colors1.htm
const std::string blue          = "2459FF";
const std::string cyan          = "69CCF0";
const std::string green         = "336600";
const std::string grey          = "C0C0C0";
const std::string nearly_white  = "FCFFFF";
const std::string pink          = "F58CBA";
const std::string purple        = "9482C9";
const std::string purple_dark   = "7668A1";
const std::string red           = "C41F3B";
const std::string taupe         = "C79C6E";
const std::string white         = "FFFFFF";
const std::string yellow        = "FFCC00";
const std::string olive         = "909000";
const std::string orange        = "FF6F00";
const std::string teal          = "009090";
const std::string darker_blue   = "59ADCC";
const std::string darker_silver = "8A8A8A";
const std::string darker_yellow = "C0B84F";
}

static std::string class_color( player_e type )
{
  switch ( type )
  {
  case PLAYER_NONE:  return color::grey;
  case DEATH_KNIGHT: return "C41F3B";
  case DRUID:        return "FF7D0A";
  case HUNTER:       return "ABD473";
  case MAGE:         return "69CCF0";
  case MONK:         return "00FF96";
  case PALADIN:      return "F58CBA";
  case PRIEST:       return "FFFFFF";
  case ROGUE:        return "FFF569";
  case SHAMAN:       return "0070DE";
  case WARLOCK:      return "9482C9";
  case WARRIOR:      return "C79C6E";
  case ENEMY:        return color::grey;
  case ENEMY_ADD:    return color::grey;
  case HEALING_ENEMY:    return color::grey;
  default: assert( 0 ); return std::string();
  }
}

// The above colors don't all work for text rendered on a light (white) background.  These colors work better by reducing the brightness HSV component of the above colors

static std::string class_text_color( player_e type )
{
  /* Commenting these out since we no longer render text on white backgrounds
   switch ( type )
   {
   case MAGE:         return color::darker_blue;
   case PRIEST:       return color::darker_silver;
   case ROGUE:        return color::darker_yellow;

   default:  */         return class_color( type );
  //}
}

static const std::string& get_chart_base_url()
{
  static const std::string base_urls[] =
  {
    "http://0.chart.apis.google.com/chart?",
    "http://1.chart.apis.google.com/chart?",
    "http://2.chart.apis.google.com/chart?",
    "http://3.chart.apis.google.com/chart?",
    "http://4.chart.apis.google.com/chart?",
    "http://5.chart.apis.google.com/chart?",
    "http://6.chart.apis.google.com/chart?",
    "http://7.chart.apis.google.com/chart?",
    "http://8.chart.apis.google.com/chart?",
    "http://9.chart.apis.google.com/chart?",
  };
  static int round_robin = -1;
  static mutex_t rr_mutex;

  auto_lock_t lock( rr_mutex );

  round_robin = ( round_robin + 1 ) % sizeof_array( base_urls );

  return base_urls[ round_robin ];
}

static player_e get_player_or_owner_type( player_t* p )
{
  player_e pt = PLAYER_NONE;

  if ( p -> is_pet() )
    pt = p -> cast_pet() -> owner -> type;
  else
    pt = p -> type;

  return pt;
}

// These colors are picked to sort of line up with classes, but match the "feel" of the spell class' color
static std::string school_color( school_e type )
{
  switch ( type )
  {
  case SCHOOL_ARCANE:       return class_color( MAGE );
  case SCHOOL_BLEED:        return "C55D54"; // Half way between DK "red" and Warrior "brown"
  case SCHOOL_CHAOS:        return color::orange;
  case SCHOOL_FIRE:         return class_color( DEATH_KNIGHT );
  case SCHOOL_FROST:        return class_color( SHAMAN );
  case SCHOOL_FROSTFIRE:    return class_color( SHAMAN );
  case SCHOOL_HOLY:         return class_color( PALADIN );
  case SCHOOL_NATURE:       return class_color( HUNTER );
  case SCHOOL_PHYSICAL:     return class_color( WARRIOR );
  case SCHOOL_SHADOW:       return class_color( WARLOCK );
  case SCHOOL_FROSTSTORM:   return "2C6080";
  case SCHOOL_SPELLSTORM:   return "8AD0B1"; // Half way between Hunter "green" and Mage "blue" (spellstorm = arcane/nature damage)
  case SCHOOL_SHADOWFROST:  return "000066"; // Shadowfrost???
  case SCHOOL_SHADOWFLAME:  return "435133";
  case SCHOOL_SHADOWSTRIKE: return "0099CC";
  case SCHOOL_SPELLSHADOW:  return color::purple_dark;
  case SCHOOL_ELEMENTAL:    return class_color( MONK );
  case SCHOOL_NONE:         return "FFFFFF";
  default: return std::string();
  }
}

static std::string stat_color( stat_e type )
{
  switch ( type )
  {
  case STAT_STRENGTH:                 return class_color( WARRIOR );
  case STAT_AGILITY:                  return class_color( HUNTER );
  case STAT_INTELLECT:                return class_color( MAGE );
  case STAT_SPIRIT:                   return class_color( PRIEST );
  case STAT_ATTACK_POWER:             return class_color( ROGUE );
  case STAT_SPELL_POWER:              return class_color( WARLOCK );
  case STAT_HIT_RATING:               return class_color( DEATH_KNIGHT );
  case STAT_CRIT_RATING:              return class_color( PALADIN );
  case STAT_HASTE_RATING:             return class_color( SHAMAN );
  case STAT_MASTERY_RATING:           return class_text_color( ROGUE );
  case STAT_EXPERTISE_RATING:         return school_color( SCHOOL_BLEED );
  default:                            return std::string();
  }
}

static std::string get_color( player_t* p )
{
  player_e type;
  if ( p -> is_pet() )
    type = p -> cast_pet() -> owner -> type;
  else
    type = p -> type;
  return class_color( type );
}

static std::string get_text_color( player_t* p )
{
  player_e type;
  if ( p -> is_pet() )
    type = p -> cast_pet() -> owner -> type;
  else
    type = p -> type;
  return class_text_color ( type );
}

static unsigned char simple_encoding( int number )
{
  if ( number < 0  ) number = 0;
  if ( number > 61 ) number = 61;

  static const char encoding[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  return encoding[ number ];
}

static std::string chart_bg_color( int print_styles )
{
  if ( print_styles == 1 )
  {
    return "666666";
  }
  else
  {
    return "dddddd";
  }
}

// ternary_coords ===========================================================

static std::vector<double> ternary_coords( std::vector<plot_data_t> xyz )
{
  std::vector<double> result;
  result.resize( 2 );
  result[0] = xyz[ 2 ].value/2.0 + xyz[ 1 ].value;
  result[1] = xyz[ 2 ].value/2.0 * sqrt( 3.0 );
  return result;
}

// color_temperature_gradient ===============================================

static std::string color_temperature_gradient( double n, double min, double range )
{
  std::string result = "";
  char buffer[ 10 ] = "";
  int red = ( int ) floor( 255.0 * ( n - min ) / range );
  int blue = 255 - red;
  snprintf( buffer, 10, "%.2X", red );
  result += buffer;
  result += "00";
  snprintf( buffer, 10, "%.2X", blue );
  result += buffer;

  return result;
}

namespace { // UNNAMED NAMESPACE

struct compare_downtime
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> waiting_time.mean > r -> waiting_time.mean;
  }
};

struct filter_non_performing_players
{
  bool dps;
  filter_non_performing_players( bool dps_ ) : dps( dps_ ) {}
  bool operator()( player_t* p ) const
  { if ( dps ) { if ( p -> dps.mean<=0 ) return true;} else if ( p -> hps.mean<=0 ) return true; return false; }
};

struct compare_dpet
{
  bool operator()( const stats_t* l, const stats_t* r ) const
  {
    return l -> apet > r -> apet;
  }
};

struct filter_stats_dpet
{
  bool player_is_healer;
  filter_stats_dpet( player_t& p ) : player_is_healer( p.primary_role() == ROLE_HEAL ) {}
  bool operator()( const stats_t* st ) const
  {
    if ( st->quiet ) return true;
    if ( st->apet <= 0 ) return true;
    if ( st -> num_refreshes.mean > 4 * st -> num_executes.mean ) return true;
    if ( player_is_healer != ( st->type != STATS_DMG ) ) return true;

    return false;
  }
};

struct compare_amount
{
  bool operator()( const stats_t* l, const stats_t* r ) const
  {
    return l -> actual_amount.mean > r -> actual_amount.mean;
  }
};

struct compare_stats_time
{
  bool operator()( const stats_t* l, const stats_t* r ) const
  {
    return l -> total_time > r -> total_time;
  }
};

struct filter_waiting_stats
{
  bool operator()( const stats_t* st ) const
  {
    if ( st -> quiet ) return true;
    if ( st -> total_time <= timespan_t::zero() ) return true;
    if ( st -> background ) return true;

    return false;
  }
};

struct compare_gain
{
  bool operator()( const gain_t* l, const gain_t* r ) const
  {
    return l -> actual > r -> actual;
  }
};

} // UNNAMED NAMESPACE

// ==========================================================================
// Chart
// ==========================================================================

std::string chart::raid_downtime( std::vector<player_t*>& players_by_name, int print_styles )
{
  // This chart should serve as a well documented example on how to do a chart in a clean and elegant way.
  // chart option overview: http://code.google.com/intl/de-DE/apis/chart/image/docs/chart_params.html

  size_t num_players = players_by_name.size();

  if ( num_players == 0 )
    return std::string();

  std::vector<player_t*> waiting_list;

  for ( size_t i = 0; i < num_players; i++ )
  {
    player_t* p = players_by_name[ i ];
    if ( ( p -> waiting_time.mean / p -> fight_length.mean ) > 0.01 )
    {
      waiting_list.push_back( p );
    }
  }

  if ( waiting_list.empty() )
    return std::string();

  range::sort( waiting_list, compare_downtime() );

  std::ostringstream s;
  s.setf( std::ios_base::fixed ); // Set fixed flag for floating point numbers
  s << get_chart_base_url();
  s << chart_size( 500, ( waiting_list.size() * 30 + 30 ) ); // Set chart size
  s << chart_type( HORIZONTAL_BAR ); // Set chart type
  if ( !( print_styles == 1 ) )
  {
    s << fill_chart( FILL_BACKGROUND, FILL_SOLID, "333333" ); // fill chart background solid
  }

  // Fill in data
  s << "chd=t:";
  double max_waiting=0;
  for ( size_t i = 0; i < waiting_list.size(); i++ )
  {
    player_t* p = waiting_list[ i ];
    double waiting = 100.0 * p -> waiting_time.mean / p -> fight_length.mean;
    if ( waiting > max_waiting ) max_waiting = waiting;
    s << ( i?"|":"" );
    s << std::setprecision( 2 ) << waiting;
  }
  s << amp;

  // Custom chart data scaling
  s << "chds=0," << ( max_waiting * 1.9 );
  s << amp;

  // Fill in color series
  s << "chco=";
  for ( size_t i = 0; i < waiting_list.size(); i++ )
  {
    if ( i ) s << ",";
    s << class_color( get_player_or_owner_type( waiting_list[ i ] ) );
  }
  s << amp;

  // Text Data
  s << "chm=";
  for ( size_t i = 0; i < waiting_list.size(); i++ )
  {
    player_t* p = waiting_list[ i ];

    std::string formatted_name = p -> name_str;
    util::urlencode( util::str_to_utf8( formatted_name ) );

    double waiting_pct = ( 100.0 * p -> waiting_time.mean / p -> fight_length.mean );

    s << ( i?"|":"" )  << "t++" << std::setprecision( p -> sim -> report_precision / 2 ) << waiting_pct; // Insert waiting percent

    s << "%++" << formatted_name.c_str(); // Insert player name

    s << "," << class_text_color( get_player_or_owner_type( p ) ); // Insert player class text color

    s << "," << i; // Series Index

    s << ",0"; // <opt_which_points> 0 == draw markers for all points

    s << ",15"; // size
  }
  s << amp;

  s << chart_title( "Player Waiting Time" ); // Set chart title

  // Format chart title with color and font size
  s << chart_title_formatting( chart_bg_color( print_styles ), 18 );

  return s.str();
}

// chart::raid_dps ==========================================================

size_t chart::raid_aps( std::vector<std::string>& images,
                        sim_t* sim,
                        std::vector<player_t*>& players_by_aps,
                        bool dps )
{
  size_t num_players = players_by_aps.size();

  if ( num_players == 0 )
    return 0;

  double max_aps = 0;
  if ( dps )
    max_aps = players_by_aps[ 0 ] -> dps.mean;
  else
    max_aps = players_by_aps[ 0 ] -> hps.mean;

  std::string s = std::string();
  char buffer[ 1024 ];
  bool first = true;

  std::vector<player_t*> player_list ;
  size_t max_players = MAX_PLAYERS_PER_CHART;

  // Ommit Player with 0 DPS/HPS
  range::remove_copy_if( players_by_aps, back_inserter( player_list ), filter_non_performing_players( dps ) );

  num_players = player_list.size();

  if ( num_players == 0 )
    return 0;

  while ( true )
  {
    if ( num_players > max_players ) num_players = max_players;

    s = get_chart_base_url();
    s += chart_size( 525, num_players * 20 + ( first ? 20 : 0 ) ); // Set chart size
    s += "cht=bhg";
    s += "&amp;";
    if ( ! ( sim -> print_styles == 1 ) )
    {
      s += "chf=bg,s,333333";
      s += "&amp;";
    }
    s += "chbh=15";
    s += "&amp;";
    s += "chd=t:";

    for ( size_t i = 0; i < num_players; i++ )
    {
      player_t* p = player_list[ i ];
      snprintf( buffer, sizeof( buffer ), "%s%.0f", ( i?"|":"" ), dps ? p -> dps.mean : p -> hps.mean ); s += buffer;
    }
    s += "&amp;";
    snprintf( buffer, sizeof( buffer ), "chds=0,%.0f", max_aps * 2.5 ); s += buffer;
    s += "&amp;";
    s += "chco=";
    for ( size_t i = 0; i < num_players; i++ )
    {
      if ( i ) s += ",";
      s += get_color( player_list[ i ] );
    }
    s += "&amp;";
    s += "chm=";
    for ( size_t i = 0; i < num_players; i++ )
    {
      player_t* p = player_list[ i ];
      std::string formatted_name;
      http::format( formatted_name, p -> name_str );
      snprintf( buffer, sizeof( buffer ), "%st++%.0f++%s,%s,%d,0,15", ( i?"|":"" ), dps ? p -> dps.mean : p -> hps.mean, formatted_name.c_str(), get_text_color( p ).c_str(), ( int )i ); s += buffer;
    }
    s += "&amp;";
    if ( first )
    {
      s += chart_title( std::string( dps ? "DPS " : "HPS" ) + " Ranking" ); // Set chart title
    }

    s += "chts=" + chart_bg_color( sim -> print_styles ) + ",18";

    images.push_back( s );

    first = false;
    player_list.erase( player_list.begin(), player_list.begin() + num_players );
    num_players = ( int ) player_list.size();
    if ( num_players == 0 ) break;
  }

  return images.size();
}

// chart::raid_gear =========================================================

size_t chart::raid_gear( std::vector<std::string>& images,
                         sim_t* sim )
{
  size_t num_players = sim -> players_by_dps.size();

  if ( ! num_players )
    return 0;

  std::vector<double> data_points[ STAT_MAX ];

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    data_points[ i ].reserve( num_players );
    for ( size_t j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_dps[ j ];

      data_points[ i ].push_back( ( p -> gear.   get_stat( i ) +
                                    p -> enchant.get_stat( i ) ) * gear_stats_t::stat_mod( i ) );
    }
  }

  double max_total=0;
  for ( size_t i=0; i < num_players; i++ )
  {
    double total=0;
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( stat_color( j ).empty() )
        continue;

      total += data_points[ j ][ i ];
    }
    if ( total > max_total ) max_total = total;
  }

  std::string s;
  char buffer[ 1024 ];
  bool first;

  std::vector<player_t*> player_list = sim -> players_by_dps;
  static const size_t max_players = MAX_PLAYERS_PER_CHART;

  while ( true )
  {
    if ( num_players > max_players ) num_players = max_players;

    int height = num_players * 20 + 30;

    if ( num_players <= 12 ) height += 70;

    s = get_chart_base_url();
    s += chart_size( 525, height ); // Set chart size
    s += "cht=bhs";
    s += "&amp;";
    if ( ! ( sim -> print_styles == 1 ) )
    {
      s += "chf=bg,s,333333";
      s += "&amp;";
    }
    s += "chbh=15";
    s += "&amp;";
    s += "chd=t:";
    first = true;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( stat_color( i ).empty() )
        continue;

      if ( ! first ) s += "|";
      first = false;
      for ( size_t j=0; j < num_players; j++ )
      {
        snprintf( buffer, sizeof( buffer ), "%s%.0f", ( j?",":"" ), data_points[ i ][ j ] ); s += buffer;
      }
    }
    s += "&amp;";
    snprintf( buffer, sizeof( buffer ), "chds=0,%.0f", max_total ); s += buffer;
    s += "&amp;";
    s += "chco=";
    first = true;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( stat_color( i ).empty() )
        continue;

      if ( ! first ) s += ",";
      first = false;
      s += stat_color( i );
    }
    s += "&amp;";
    s += "chxt=y";
    s += "&amp;";
    s += "chxl=0:";
    for ( int i = num_players-1; i >= 0; i-- )
    {
      std::string formatted_name = player_list[ i ] -> name_str;
      util::urlencode( util::str_to_utf8( formatted_name ) );

      s += "|";
      s += formatted_name.c_str();
    }
    s += "&amp;";
    if ( sim -> print_styles == 1 )
    {
      s += "chxs=0,000000,14";
    }
    else
    {
      s += "chxs=0,dddddd,14";
    }
    s += "&amp;";
    s += "chdl=";
    first = true;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( stat_color( i ).empty() )
        continue;

      if ( ! first ) s += "|";
      first = false;
      s += util::stat_type_abbrev( i );
    }
    s += "&amp;";
    if ( num_players <= 12 )
    {
      s += "chdlp=t&amp;";
    }
    if ( ! ( sim -> print_styles == 1 ) )
    {
      s += "chdls=dddddd,12";
      s += "&amp;";
    }
    s += chart_title( "Gear Overview" ); // Set chart title

    s += "chts=" + chart_bg_color( sim -> print_styles ) + ",18";

    images.push_back( s );

    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      std::vector<double>& c = data_points[ i ];
      c.erase( c.begin(), c.begin() + num_players );
    }

    player_list.erase( player_list.begin(), player_list.begin() + num_players );
    num_players = ( int ) player_list.size();
    if ( num_players == 0 ) break;
  }

  return images.size();
}



// chart::raid_dpet =========================================================

size_t chart::raid_dpet( std::vector<std::string>& images,
                         sim_t* sim )
{
  size_t num_players = sim -> players_by_dps.size();

  if ( num_players == 0 )
    return 0;

  std::vector<stats_t*> stats_list;


  for ( size_t i = 0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_dps[ i ];

    // Copy all stats* from p -> stats_list to stats_list, which satisfy the filter
    range::remove_copy_if( p -> stats_list, back_inserter( stats_list ), filter_stats_dpet( *p ) );
  }

  size_t num_stats = stats_list.size();
  if ( num_stats == 0 ) return 0;

  range::sort( stats_list, compare_dpet() );

  double max_dpet = stats_list[ 0 ] -> apet;

  size_t max_actions_per_chart = 20;
  size_t max_charts = 4;

  std::string s;
  char buffer[ 1024 ];

  for ( size_t chart = 0; chart < max_charts; chart++ )
  {
    if ( num_stats > max_actions_per_chart ) num_stats = max_actions_per_chart;

    s = get_chart_base_url();
    s += chart_size( 500, num_stats * 15 + ( chart == 0 ? 20 : -10 ) ); // Set chart size
    s += "cht=bhg";
    s += "&amp;";
    if ( ! ( sim -> print_styles == 1 ) )
    {
      s += "chf=bg,s,333333";
      s += "&amp;";
    }
    s += "chbh=10";
    s += "&amp;";
    s += "chd=t:";
    for ( size_t i = 0; i < num_stats; i++ )
    {
      stats_t* st = stats_list[ i ];
      snprintf( buffer, sizeof( buffer ), "%s%.0f", ( i?"|":"" ), st -> apet ); s += buffer;
    }
    s += "&amp;";
    snprintf( buffer, sizeof( buffer ), "chds=0,%.0f", max_dpet * 2.5 ); s += buffer;
    s += "&amp;";
    s += "chco=";
    for ( size_t i = 0; i < num_stats; i++ )
    {
      if ( i ) s += ",";
      s += school_color ( stats_list[ i ] -> school );
    }
    s += "&amp;";
    s += "chm=";
    for ( size_t i = 0; i < num_stats; i++ )
    {
      stats_t* st = stats_list[ i ];
      std::string formatted_name = st -> player -> name_str;
      util::urlencode( util::str_to_utf8( formatted_name ) );

      snprintf( buffer, sizeof( buffer ), "%st++%.0f++%s+(%s),%s,%d,0,10", ( i?"|":"" ),
                st -> apet, st -> name_str.c_str(), formatted_name.c_str(), get_text_color( st -> player ).c_str(), ( int )i ); s += buffer;
    }
    s += "&amp;";
    if ( chart==0 )
    {
      s += chart_title( "Raid Damage Per Execute Time" ); // Set chart title
    }

    s += "chts=" + chart_bg_color( sim -> print_styles ) + ",18";

    images.push_back( s );

    stats_list.erase( stats_list.begin(), stats_list.begin() + num_stats );
    num_stats = stats_list.size();
    if ( num_stats == 0 ) break;
  }

  return images.size();
}

// chart::action_dpet =======================================================

std::string chart::action_dpet(  player_t* p )
{
  std::vector<stats_t*> stats_list;

  // Copy all stats* from p -> stats_list to stats_list, which satisfy the filter
  range::remove_copy_if( p -> stats_list, back_inserter( stats_list ), filter_stats_dpet( *p ) );

  int num_stats = ( int ) stats_list.size();
  if ( num_stats == 0 )
    return std::string();

  range::sort( stats_list, compare_dpet() );

  char buffer[ 1024 ];

  std::string s = get_chart_base_url();
  s += chart_size( 550, num_stats * 30 + 30 ); // Set chart size
  s += "cht=bhg";
  s += "&amp;";
  if ( ! ( p -> sim -> print_styles == 1 ) )
  {
    s += "chf=bg,s,333333";
    s += "&amp;";
  }
  s += "chd=t:";
  double max_apet=0;
  for ( int i=0; i < num_stats; i++ )
  {
    stats_t* st = stats_list[ i ];
    snprintf( buffer, sizeof( buffer ), "%s%.0f", ( i?"|":"" ), st -> apet ); s += buffer;
    if ( st -> apet > max_apet ) max_apet = st -> apet;
  }
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chds=0,%.0f", max_apet * 2 ); s += buffer;
  s += "&amp;";
  s += "chco=";
  for ( int i=0; i < num_stats; i++ )
  {
    if ( i ) s += ",";


    std::string school = school_color( stats_list[ i ] -> school );
    if ( school.empty() )
    {
      p -> sim -> errorf( "chart_t::action_dpet assertion error! School color unknown, stats %s from %s. School %s\n", stats_list[ i ] -> name_str.c_str(), p -> name(), util::school_type_string( stats_list[ i ] -> school ) );
      assert( 0 );
    }
    s += school;
  }
  s += "&amp;";
  s += "chm=";
  for ( int i=0; i < num_stats; i++ )
  {
    stats_t* st = stats_list[ i ];
    snprintf( buffer, sizeof( buffer ), "%st++%.0f++%s,%s,%d,0,15", ( i?"|":"" ), st -> apet, st -> name_str.c_str(), school_color( st -> school ).c_str(), i ); s += buffer;
  }
  s += "&amp;";

  std::string formatted_name = p -> name_str;
  util::urlencode( util::str_to_utf8( formatted_name ) );
  s += chart_title( formatted_name + " Damage Per Execute Time" ); // Set chart title
  s += "&amp;";

  s += "chts=" + chart_bg_color( p -> sim -> print_styles ) + ",18";

  return s;
}

// chart::action_dmg ========================================================

std::string chart::aps_portion(  player_t* p )
{
  std::vector<stats_t*> stats_list;

  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* st = p -> stats_list[ i ];
    if ( st -> quiet ) continue;
    if ( st -> actual_amount.mean <= 0 ) continue;
    if ( ( p -> primary_role() == ROLE_HEAL ) != ( st -> type != STATS_DMG ) ) continue;
    stats_list.push_back( st );
  }

  for ( size_t i = 0; i < p -> pet_list.size(); ++i )
  {
    pet_t* pet = p -> pet_list[ i ];
    for ( size_t j = 0; j < pet -> stats_list.size(); ++j )
    {
      stats_t* st = pet -> stats_list[ j ];
      if ( st -> quiet ) continue;
      if ( st -> actual_amount.mean <= 0 ) continue;
      if ( ( p -> primary_role() == ROLE_HEAL ) != ( st -> type != STATS_DMG ) ) continue;
      stats_list.push_back( st );
    }
  }

  int num_stats = ( int ) stats_list.size();
  if ( num_stats == 0 )
    return std::string();

  range::sort( stats_list, compare_amount() );

  char buffer[ 1024 ];

  std::string s = get_chart_base_url();
  s += chart_size( 550, 275 ); // Set chart size
  s += "cht=p";
  s += "&amp;";
  if ( ! ( p -> sim -> print_styles == 1 ) )
  {
    s += "chf=bg,s,333333";
    s += "&amp;";
  }
  s += "chd=t:";
  for ( int i=0; i < num_stats; i++ )
  {
    stats_t* st = stats_list[ i ];
    snprintf( buffer, sizeof( buffer ), "%s%.0f", ( i?",":"" ), 100.0 * st -> actual_amount.mean / ( ( p -> primary_role() == ROLE_HEAL ) ? p -> heal.mean : p -> dmg.mean ) ); s += buffer;
  }
  s += "&amp;";
  s += "chds=0,100";
  s += "&amp;";
  s += "chdls=ffffff";
  s += "&amp;";
  s += "chco=";
  for ( int i=0; i < num_stats; i++ )
  {
    if ( i ) s += ",";

    std::string school = school_color( stats_list[ i ] -> school );
    if ( school.empty() )
    {
      p -> sim -> errorf( "chart_t::action_dmg assertion error! School unknown, stats %s from %s.\n", stats_list[ i ] -> name_str.c_str(), p -> name() );
      assert( 0 );
    }
    s += school;

  }
  s += "&amp;";
  s += "chl=";
  for ( int i=0; i < num_stats; i++ )
  {
    if ( i ) s += "|";
    if ( stats_list[ i ] -> player -> type == PLAYER_PET || stats_list[ i ] -> player -> type == PLAYER_GUARDIAN )
    {
      s += stats_list[ i ] -> player -> name_str.c_str();
      s += ": ";
    }
    s += stats_list[ i ] -> name_str.c_str();
  }
  s += "&amp;";
  std::string formatted_name = p -> name();
  util::urlencode( util::str_to_utf8( formatted_name ) );
  s += chart_title( formatted_name + ( p -> primary_role() == ROLE_HEAL ? " Healing" : " Damage" ) + " Sources" ); // Set chart title

  s += "chts=" + chart_bg_color( p -> sim -> print_styles ) + ",18";

  return s;
}

// chart::spent_time ========================================================

std::string chart::time_spent( player_t* p )
{
  std::vector<stats_t*> filtered_waiting_stats;

  // Filter stats we do not want in the chart ( quiet, background, zero total_time ) and copy them to filtered_waiting_stats
  range::remove_copy_if( p -> stats_list, back_inserter( filtered_waiting_stats ), filter_waiting_stats() );

  size_t num_stats = filtered_waiting_stats.size();
  if ( num_stats == 0 && p -> waiting_time.mean == 0 )
    return std::string();

  range::sort( filtered_waiting_stats, compare_stats_time() );

  char buffer[ 1024 ];

  std::string s = get_chart_base_url();
  s += chart_size( 525, 275 ); // Set chart size
  s += "cht=p";
  s += "&amp;";
  if ( ! ( p -> sim -> print_styles == 1 ) )
  {
    s += "chf=bg,s,333333";
    s += "&amp;";
  }
  s += "chd=t:";
  for ( size_t i = 0; i < num_stats; i++ )
  {
    snprintf( buffer, sizeof( buffer ), "%s%.1f", ( i?",":"" ), 100.0 * filtered_waiting_stats[ i ] -> total_time.total_seconds() / p -> fight_length.mean ); s += buffer;
  }
  if ( p -> waiting_time.mean > 0 )
  {
    snprintf( buffer, sizeof( buffer ), "%s%.1f", ( num_stats > 0 ? ",":"" ), 100.0 * p -> waiting_time.mean / p -> fight_length.mean ); s += buffer;
  }
  s += "&amp;";
  s += "chds=0,100";
  s += "&amp;";
  s += "chdls=ffffff";
  s += "&amp;";
  s += "chco=";
  for ( size_t i = 0; i < num_stats; i++ )
  {
    if ( i ) s += ",";

    std::string school = school_color( filtered_waiting_stats[ i ] -> school );
    if ( school.empty() )
    {
      p -> sim -> errorf( "chart_t::time_spent assertion error! School unknown, stats %s from %s.\n", filtered_waiting_stats[ i ] -> name_str.c_str(), p -> name() );
      assert( 0 );
    }
    s += school;
  }
  if ( p -> waiting_time.mean > 0 )
  {
    if ( num_stats > 0 ) s += ",";
    s += "ffffff";
  }
  s += "&amp;";
  s += "chl=";
  for ( size_t i = 0; i < num_stats; i++ )
  {
    stats_t* st = filtered_waiting_stats[ i ];
    if ( i ) s += "|";
    s += st -> name_str.c_str();
    snprintf( buffer, sizeof( buffer ), " %.1fs", st -> total_time.total_seconds() ); s += buffer;
  }
  if ( p -> waiting_time.mean > 0 )
  {
    if ( num_stats > 0 )s += "|";
    s += "waiting";
    snprintf( buffer, sizeof( buffer ), " %.1fs", p -> waiting_time.mean ); s += buffer;
  }
  s += "&amp;";
  std::string formatted_name = p -> name();
  util::urlencode( util::str_to_utf8( formatted_name ) );
  s += chart_title( formatted_name + " Spent Time" ); // Set chart title

  s += "chts=" + chart_bg_color( p -> sim -> print_styles ) + ",18";

  return s;
}

// chart::gains =============================================================

std::string chart::gains( player_t* p, resource_e type )
{
  std::vector<gain_t*> gains_list;

  double total_gain = 0;

  for ( size_t i = 0; i < p -> gain_list.size(); ++i )
  {
    gain_t* g = p -> gain_list[ i ];
    if ( g -> actual[ type ] <= 0 ) continue;
    total_gain += g -> actual[ type ];
    gains_list.push_back( g );
  }

  int num_gains = ( int ) gains_list.size();
  if ( num_gains == 0 )
    return std::string();

  range::sort( gains_list, compare_gain() );

  std::ostringstream s;
  s.setf( std::ios_base::fixed ); // Set fixed flag for floating point numbers
  s << get_chart_base_url();
  s << chart_size( 550, 200 + num_gains * 10 );
  s << chart_type( PIE );
  if ( ! ( p -> sim -> print_styles == 1 ) )
    s << fill_chart( FILL_BACKGROUND, FILL_SOLID, "333333" );

  // Insert Chart Data
  s << "chd=t:";
  for ( int i=0; i < num_gains; i++ )
  {
    gain_t* g = gains_list[ i ];
    s << ( i?",":"" );
    s << std::setprecision( p -> sim -> report_precision / 2 ) << 100.0 * ( g -> actual[ type ] / total_gain );
  }
  s << "&amp;";

  // Chart scaling, may not be necessary if numbers are not shown
  s << "chds=0,100";
  s << "&amp;";

  // Series color
  s << "chco=";
  s << resource_color( type );
  s << "&amp;";

  // Labels
  s << "chl=";
  for ( int i=0; i < num_gains; i++ )
  {
    if ( i ) s << "|";
    s << gains_list[ i ] -> name();
  }
  s << "&amp;";

  std::string formatted_name = p -> name_str;
  util::urlencode( util::str_to_utf8( formatted_name ) );
  std::string r = util::resource_type_string( type );
  util::inverse_tokenize( r );
  s << chart_title( formatted_name + "+" + r + " Gains" );

  if ( p -> sim -> print_styles == 1 )
    s << chart_title_formatting( "666666", 18 );
  else
    s << chart_title_formatting( "dddddd", 18 );

  return s.str();
}

// chart::scale_factors =====================================================

std::string chart::scale_factors( player_t* p )
{
  std::vector<stat_e> scaling_stats;

  for ( std::vector<stat_e>::const_iterator it = p -> scaling_stats.begin(), end = p -> scaling_stats.end(); it != end; ++it )
  {
    if ( p -> scales_with[ *it ] && p -> scaling.get_stat( *it ) > 0 )
      scaling_stats.push_back( *it );
    if ( p -> scales_with[ *it ] && p -> scaling.get_stat( *it ) < 0  && p -> sim -> scaling -> scale_over == "dtps" )
      scaling_stats.push_back( *it );
  }

  size_t num_scaling_stats = scaling_stats.size();
  if ( num_scaling_stats == 0 )
    return std::string();

  double max_scale_factor = p -> scaling.get_stat( scaling_stats[ 0 ] );

  char buffer[ 1024 ];

  std::string s = get_chart_base_url();
  s += chart_size( 525, num_scaling_stats * 30 + 60 ); // Set chart size
  s += chart_type( HORIZONTAL_BAR );
  s += "chxs=0,ffffff|1,ffffff";
  s += "&amp;";

  if ( ! ( p -> sim -> print_styles == 1 ) )
    s += fill_chart( FILL_BACKGROUND, FILL_SOLID, "333333" );

  snprintf( buffer, sizeof( buffer ), "chd=t%i:" , 1 ); s += buffer;
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p -> scaling.get_stat( scaling_stats[ i ] );
    snprintf( buffer, sizeof( buffer ), "%s%.*f", ( i?",":"" ), p -> sim -> report_precision, factor ); s += buffer;
  }
  s += "|";
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p -> scaling.get_stat( scaling_stats[ i ] );
    double error = p -> scaling_error.get_stat( scaling_stats[ i ] );
    if ( error > 0 )
      factor -= error;
    else
      factor = 0;
    snprintf( buffer, sizeof( buffer ), "%s%.*f", ( i?",":"" ), p -> sim -> report_precision, factor ); s += buffer;
  }
  s += "|";
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p -> scaling.get_stat( scaling_stats[ i ] );
    double error = p -> scaling_error.get_stat( scaling_stats[ i ] );
    if ( error )
      factor += error;
    else
      factor = 0;
    snprintf( buffer, sizeof( buffer ), "%s%.*f", ( i?",":"" ), p -> sim -> report_precision, factor ); s += buffer;
  }
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chds=0,%.*f", p -> sim -> report_precision, max_scale_factor * 2 ); s += buffer;
  s += "&amp;";
  s += "chco=";
  s += class_color( p -> type );
  s += "&amp;";
  s += "chm=";
  snprintf( buffer, sizeof( buffer ), "E,FF0000,1:0,,1:20|" ); s += buffer;
  for ( size_t i = 0; i < num_scaling_stats; i++ )
  {
    double factor = p -> scaling.get_stat( scaling_stats[ i ] );
    const char* name = util::stat_type_abbrev( scaling_stats[ i ] );
    snprintf( buffer, sizeof( buffer ), "%st++++%.*f++%s,%s,0,%d,15,0.1", ( i?"|":"" ),
              p -> sim -> report_precision, factor, name, class_text_color( p -> type ).c_str(), ( int )i ); s += buffer;
  }

  s += "&amp;";
  std::string formatted_name = p -> scales_over().name_str;
  util::urlencode( util::str_to_utf8( formatted_name ) );
  s += chart_title( "Scale Factors|" + formatted_name ); // Set chart title

  s += "chts=" + chart_bg_color( p -> sim -> print_styles ) + ",18";

  return s;
}

// chart::scaling_dps =======================================================

std::string chart::scaling_dps( player_t* p )
{
  double max_dps = 0, min_dps = std::numeric_limits<double>::max();

  for ( size_t i = 0; i < p -> dps_plot_data.size(); ++i )
  {
    std::vector<plot_data_t>& pd = p -> dps_plot_data[ i ];
    size_t size = pd.size();
    for ( size_t j = 0; j < size; j++ )
    {
      if ( pd[ j ].value > max_dps ) max_dps = pd[ j ].value;
      if ( pd[ j ].value < min_dps ) min_dps = pd[ j ].value;
    }
  }
  if ( max_dps <= 0 )
    return std::string();

  double step = p -> sim -> plot -> dps_plot_step;
  int range = p -> sim -> plot -> dps_plot_points / 2;
  const int start = 0;  // start and end only used for dps_plot_positive
  const int end = 2 * range;
  size_t num_points = 1 + 2 * range;

  char buffer[ 1024 ];

  std::string s = get_chart_base_url();
  s += chart_size( 550, 300 ); // Set chart size
  s += "cht=lc";
  s += "&amp;";
  if ( ! ( p -> sim -> print_styles == 1 ) )
  {
    s += "chf=bg,s,333333";
    s += "&amp;";
  }
  s += "chd=t:";
  bool first=true;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( stat_color( i ).empty() ) continue;
    std::vector<plot_data_t>& pd = p -> dps_plot_data[ i ];
    size_t size = pd.size();
    if ( size != num_points ) continue;
    if ( ! first ) s += "|";
    for ( size_t j=0; j < size; j++ )
    {
      snprintf( buffer, sizeof( buffer ), "%s%.0f", ( j?",":"" ), pd[ j ].value ); s += buffer;
    }
    first = false;
  }
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chds=%.0f,%.0f", min_dps, max_dps ); s += buffer;
  s += "&amp;";
  s += "chxt=x,y";
  s += "&amp;";
  if ( ! p -> sim -> plot -> dps_plot_positive )
  {
    snprintf( buffer, sizeof( buffer ), "chxl=0:|%.0f|%.0f|0|%%2b%.0f|%%2b%.0f|1:|%.0f|%.0f|%.0f", ( -range*step ), ( -range*step )/2, ( +range*step )/2, ( +range*step ), min_dps, p -> dps.mean, max_dps ); s += buffer;
  }
  else
  {
    snprintf( buffer, sizeof( buffer ), "chxl=0:|0|%%2b%.0f|%%2b%.0f|%%2b%.0f|%%2b%.0f|1:|%.0f|%.0f|%.0f", ( start + ( 1.0/4 )*end )*step, ( start + ( 2.0/4 )*end )*step, ( start + ( 3.0/4 )*end )*step, ( start + end )*step, min_dps, p -> dps.mean, max_dps ); s += buffer;
  }
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chxp=0,0,24.5,50,74.5,100|1,1,%.0f,100", 100.0 * ( p -> dps.mean - min_dps ) / ( max_dps - min_dps ) ); s += buffer;
  s += "&amp;";
  s += "chdl=";
  first = true;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( stat_color( i ).empty() ) continue;
    size_t size = p -> dps_plot_data[ i ].size();
    if ( size != num_points ) continue;
    if ( ! first ) s += "|";
    s += util::stat_type_abbrev( i );
    first = false;
  }
  s += "&amp;";
  if ( ! ( p -> sim -> print_styles == 1 ) )
  {
    s += "chdls=dddddd,12";
    s += "&amp;";
  }
  s += "chco=";
  first = true;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( stat_color( i ).empty() ) continue;
    size_t size = p -> dps_plot_data[ i ].size();
    if ( size != num_points ) continue;
    if ( ! first ) s += ",";
    first = false;
    s += stat_color( i );
  }
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chg=%.4f,10,1,3", floor( 10000.0 * 100.0 / ( num_points - 1 ) ) / 10000.0 ); s += buffer;
  s += "&amp;";
  std::string formatted_name = p -> scales_over().name_str;
  util::urlencode( util::str_to_utf8( formatted_name ) );
  s += chart_title( "DPS Scaling|" + formatted_name ); // Set chart title

  s += "chts=" + chart_bg_color( p -> sim -> print_styles ) + ",18";

  return s;
}

// chart::reforge_dps =======================================================

std::string chart::reforge_dps( player_t* p )
{
  double dps_range = 0.0, min_dps = std::numeric_limits<double>::max(), max_dps = 0.0;

  if ( ! p )
    return std::string();

  std::vector< std::vector<plot_data_t> >& pd = p -> reforge_plot_data;
  if ( pd.empty() )
    return std::string();

  size_t num_stats = pd[ 0 ].size() - 1;
  if ( num_stats != 3 && num_stats != 2 )
  {
    p -> sim -> errorf( "You must choose 2 or 3 stats to generate a reforge plot.\n" );
    return std::string();
  }

  for ( size_t i = 0; i < pd.size(); i++ )
  {
    assert( num_stats < pd[ i ].size() );
    if ( pd[ i ][ num_stats ].value < min_dps )
      min_dps = pd[ i ][ num_stats ].value;
    if ( pd[ i ][ num_stats ].value > max_dps )
      max_dps = pd[ i ][ num_stats ].value;
  }

  dps_range = max_dps - min_dps;

  std::ostringstream s;
  s.setf( std::ios_base::fixed ); // Set fixed flag for floating point numbers
  if ( num_stats == 2 )
  {
    int range = p -> sim -> reforge_plot -> reforge_plot_amount;
    int num_points = ( int ) pd.size();
    std::vector<stat_e> stat_indices = p -> sim -> reforge_plot -> reforge_plot_stat_indices;
    plot_data_t& baseline = pd[ num_points / 2 ][ 2 ];
    double min_delta = baseline.value - ( min_dps - baseline.error / 2 );
    double max_delta = ( max_dps + baseline.error / 2 ) - baseline.value;
    double max_ydelta = std::max( min_delta, max_delta );
    int ysteps = 5;
    double ystep_amount = max_ydelta / ysteps;

    s << get_chart_base_url();
    s << chart_size( 525, 300 ); // Set chart size
    s << "cht=lxy";
    s << "&amp;";
    if ( ! ( p -> sim -> print_styles == 1 ) )
    {
      s << "chf=bg,s,333333";
      s << "&amp;";
    }

    // X series
    s << "chd=t2:";
    for ( int i=0; i < num_points; i++ )
    {
      s << pd[ i ][ 0 ].value;
      if ( i < num_points - 1 )
        s << ",";
    }

    // Y series
    s << "|";
    for ( int i=0; i < num_points; i++ )
    {
      s << pd[ i ][ 2 ].value;
      if ( i < num_points - 1 )
        s << ",";
    }

    // Min Y series
    s << "|-1|";
    for ( int i=0; i < num_points; i++ )
    {
      double v = pd[ i ][ 2 ].value - pd[ i ][ 2 ].error / 2;
      s << v;
      if ( i < num_points - 1 )
        s << ",";
    }

    // Max Y series
    s << "|-1|";
    for ( int i=0; i < num_points; i++ )
    {
      double v = pd[ i ][ 2 ].value + pd[ i ][ 2 ].error / 2;
      s << v;
      if ( i < num_points - 1 )
        s << ",";
    }

    s << "&amp;";

    // Axis dimensions
    s << "chds=" << -range << "," << +range << "," << floor( baseline.value - max_ydelta ) << "," << ceil( baseline.value + max_ydelta );
    s << "&amp;chxt=x,y,x&amp;";

    // X Axis labels
    s << "chxl=0:|" << range << "|" << range / 2 << "|0|" << range / 2 << "|" << range << "|";

    // Y Axis labels
    s << "1:|";
    for ( int i = ysteps; i >= 1; i -= 1 )
    {
      s << ( int ) util::round( baseline.value - i * ystep_amount ) << " (" << - ( int ) util::round( i * ystep_amount ) << ")|";
    }
    s << baseline.value << "|";
    for ( int i = 1; i <= ysteps; i += 1 )
    {
      s << ( int ) util::round( baseline.value + i * ystep_amount ) << " (%2b" << ( int ) util::round( i * ystep_amount ) << ")|";
    }

    // X2 Axis labels
    s << "2:|" << util::stat_type_abbrev( stat_indices[ 0 ] );
    s << " to " << util::stat_type_abbrev( stat_indices[ 1 ] );
    s << "||" <<  util::stat_type_abbrev( stat_indices[ 1 ] );
    s << " to " << util::stat_type_abbrev( stat_indices[ 0 ] );
    s << "&amp;";

    // Chart legend
    if ( ! ( p -> sim -> print_styles == 1 ) )
    {
      s << "chdls=dddddd,12";
      s << "&amp;";
    }

    // Chart color
    s << "chco=";
    s << stat_color( stat_indices[ 0 ] );
    s << "&amp;";

    // Grid lines
    s << "chg=5,";
    s << util::to_string( 100 / ( ysteps * 2 ) );
    s << ",1,3";
    s << "&amp;";

    // Chart Title
    std::string formatted_name = p -> scales_over().name_str;
    util::urlencode( util::str_to_utf8( formatted_name ) );
    s << chart_title( "Reforge Scaling|" + formatted_name ); // Set chart title

    s << "chts=" + chart_bg_color( p -> sim -> print_styles ) + ",18";
    s << "&amp;";

    // Chart markers (Errorbars and Center-line)
    s << "chm=E,FF2222,1,-1,1:5|h,888888,1,0.5,1,-1.0";
  }
  else if ( num_stats == 3 )
  {
    if ( max_dps == 0 ) return 0;

    std::vector<std::vector<double> > triangle_points;
    std::vector< std::string > colors;
    for ( int i=0; i < ( int ) pd.size(); i++ )
    {
      std::vector<plot_data_t> scaled_dps = pd[ i ];
      int ref_plot_amount = p -> sim -> reforge_plot -> reforge_plot_amount;
      for ( int j=0; j < 3; j++ )
        scaled_dps[ j ].value = ( scaled_dps[ j ].value + ref_plot_amount ) / ( 3. * ref_plot_amount );
      triangle_points.push_back( ternary_coords( scaled_dps ) );
      colors.push_back( color_temperature_gradient( pd[ i ][ 3 ].value, min_dps, dps_range ) );
    }

    s << "<form action='";
    s << get_chart_base_url();
    s << "' method='POST'>";
    s << "<input type='hidden' name='chs' value='525x425' />";
    s << "\n";
    s << "<input type='hidden' name='cht' value='s' />";
    s << "\n";
    if ( ! ( p -> sim -> print_styles == 1 ) )
    {
      s << "<input type='hidden' name='chf' value='bg,s,333333' />";
      s << "\n";
    }

    s << "<input type='hidden' name='chd' value='t:";
    for ( size_t j = 0; j < 2; j++ )
    {
      for ( size_t i = 0; i < triangle_points.size(); i++ )
      {
        s << triangle_points[ i ][ j ];
        if ( i < triangle_points.size() - 1 )
          s << ",";
      }
      if ( j == 0 )
        s << "|";
    }
    s << "' />";
    s << "\n";
    s << "<input type='hidden' name='chco' value='";
    for ( int i=0; i < ( int ) colors.size(); i++ )
    {
      s << colors[ i ];
      if ( i < ( int ) colors.size() - 1 )
        s<< "|";
    }
    s << "' />\n";
    s << "<input type='hidden' name='chds' value='-0.1,1.1,-0.1,0.95' />";
    s << "\n";

    if ( ! ( p -> sim -> print_styles == 1 ) )
    {
      s << "<input type='hidden' name='chdls' value='dddddd,12' />";
      s << "\n";
    }
    s << "\n";
    s << "<input type='hidden' name='chg' value='5,10,1,3'";
    s << "\n";
    std::string formatted_name = p -> name_str;
    util::urlencode( util::str_to_utf8( formatted_name ) );
    s << "<input type='hidden' name='chtt' value='";
    s << formatted_name;
    s << "+Reforge+Scaling' />";
    s << "\n";
    if ( p -> sim -> print_styles == 1 )
    {
      s << "<input type='hidden' name='chts' value='666666,18' />";
    }
    else
    {
      s << "<input type='hidden' name='chts' value='dddddd,18' />";
    }
    s << "\n";
    s << "<input type='hidden' name='chem' value='";
    std::vector<stat_e> stat_indices = p -> sim -> reforge_plot -> reforge_plot_stat_indices;
    s << "y;s=text_outline;d=FF9473,18,l,000000,_,";
    s << util::stat_type_string( stat_indices[ 0 ] );
    s << ";py=1.0;po=0.0,0.01;";
    s << "|y;s=text_outline;d=FF9473,18,r,000000,_,";
    s << util::stat_type_string( stat_indices[ 1 ] );
    s << ";py=1.0;po=1.0,0.01;";
    s << "|y;s=text_outline;d=FF9473,18,h,000000,_,";
    s << util::stat_type_string( stat_indices[ 2 ] );
    s << ";py=1.0;po=0.5,0.9' />";
    s << "\n";
    s << "<input type='submit' value='Get Reforge Plot Chart'>";
    s << "\n";
    s << "</form>";
    s << "\n";
  }

  return s.str();
}

// chart::timeline ==========================================================

std::string chart::timeline(  player_t* p,
                              const std::vector<double>& timeline_data,
                              const std::string& timeline_name,
                              double avg,
                              std::string color )
{
  if ( timeline_data.empty() )
    return std::string();

  static const size_t max_points = 600;
  static const double timeline_range  = 60.0;

  size_t max_buckets = timeline_data.size();
  int increment = ( ( max_buckets > max_points ) ?
                    ( ( int ) floor( ( double ) max_buckets / max_points ) + 1 ) :
                    1 );

  double timeline_max = ( max_buckets ?
                          *range::max_element( timeline_data ) :
                          0 );

  double timeline_adjust = timeline_range / timeline_max;

  char buffer[ 2048 ];

  std::string s = get_chart_base_url();
  s += chart_size( 525, 200 ); // Set chart size
  s += "cht=lc";
  s += "&amp;";
  s += "chxs=0,ffffff|1,ffffff";
  s += "&amp;";
  if ( p -> sim -> print_styles == 1 )
  {
    s += "chf=c,ls,0,EEEEEE,0.2,FFFFFF,0.2";
  }
  else
  {
    s += "chf=bg,s,333333";
  }
  s += "&amp;";
  s += "chg=20,20";
  s += "&amp;";
  s += "chd=s:";
  for ( size_t i = 0; i < max_buckets; i += increment )
  {
    s += simple_encoding( ( int ) ( timeline_data[ i ] * timeline_adjust ) );
  }
  s += "&amp;";
  if ( ! ( p -> sim -> print_styles == 1 ) )
  {
    snprintf( buffer, sizeof( buffer ), "chco=%s", color.c_str() ); s += buffer;
    s += "&amp;";
  }
  snprintf( buffer, sizeof( buffer ), "chds=0,%.0f", timeline_range ); s += buffer;
  s += "&amp;";
  if ( avg > 0 )
  {
    snprintf( buffer, sizeof( buffer ), "chm=h,FF0000,0,%.4f,0.4", avg / timeline_max ); s += buffer;
    s += "&amp;";
  }
  s += "chxt=x,y";
  s += "&amp;";
  std::ostringstream f;
  f << "chxl=0:|0|sec=" << util::to_string( max_buckets ) << "|1:|0";
  if ( avg )
    f << "|avg=" << util::to_string( avg, 0 );
  else f << "|";
  if ( timeline_max )
    f << "|max=" << util::to_string( timeline_max, 0 );
  s += f.str();
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chxp=1,1,%.0f,100", 100.0 * avg / timeline_max ); s += buffer;
  s += "&amp;";

  s += chart_title( timeline_name + " Timeline" ); // Set chart title

  s += "chts=" + chart_bg_color( p -> sim -> print_styles ) + ",18";

  return s;
}

// chart::timeline_dps_error ================================================

std::string chart::timeline_dps_error( player_t* p )
{
  static const size_t min_data_number = 50;
  size_t max_buckets = p -> dps_convergence_error.size();
  if ( max_buckets <= min_data_number )
    return std::string();

  size_t max_points  = 600;
  size_t increment   = 1;

  if ( max_buckets > max_points )
  {
    increment = ( ( int ) floor( max_buckets / ( double ) max_points ) ) + 1;
  }

  double dps_max_error= *std::max_element( p -> dps_convergence_error.begin() + min_data_number, p -> dps_convergence_error.end() );
  double dps_range  = 60.0;
  double dps_adjust = dps_range / dps_max_error;

  char buffer[ 1024 ];

  std::string s = get_chart_base_url();
  s += chart_size( 525, 185 ); // Set chart size
  s += "cht=lc";
  s += "&amp;";
  s += "chg=20,20";
  s += "&amp;";
  s += "chxs=0,ffffff|1,ffffff";
  s += "&amp;";
  s += "chco=FF0000,0000FF";
  s += "&amp;";
  if ( p -> sim -> print_styles == 1 )
  {
    s += "chf=c,ls,0,EEEEEE,0.2,FFFFFF,0.2";
  }
  else
  {
    s += "chf=bg,s,333333";
  }
  s += "&amp;";
  s += "chd=s:";
  for ( size_t i = 0; i < max_buckets; i += increment )
  {
    if ( i < min_data_number )
      s += simple_encoding( 0 );
    else
      s += simple_encoding( ( int ) ( p -> dps_convergence_error[ i ] * dps_adjust ) );
  }
  s += "&amp;";
  s += "chxt=x,y";
  s += "&amp;";
  s += "chm=";
  for ( unsigned i = 1; i <= 5; i++ )
  {
    unsigned j = ( int ) ( ( max_buckets / 5 ) * i );
    if ( !j ) continue;
    if ( j >= max_buckets ) j = max_buckets - 1;
    if ( i > 1 ) s += "|";
    snprintf( buffer, sizeof( buffer ), "t%.1f,FFFFFF,0,%d,10", p -> dps_convergence_error[ j ], int( j / increment ) ); s += buffer;

  }
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chxl=0:|0|iterations=%d|1:|0|max dps error=%.0f", int( max_buckets + 1 ), dps_max_error ); s += buffer;
  s += "&amp;";
  s += "chdl=DPS Error";
  s += "&amp;";
  s += chart_title( "Standard Error Confidence ( n >= 50 )" ); // Set chart title

  s += "chts=" + chart_bg_color( p -> sim -> print_styles ) + ",18";

  return s;
}


// chart::distribution_dps ==================================================

std::string chart::distribution( int print_style,
                                 std::vector<int>& dist_data,
                                 const std::string& distribution_name,
                                 double avg, double min, double max )
{
  int max_buckets = ( int ) dist_data.size();

  if ( ! max_buckets )
    return std::string();

  int count_max = *range::max_element( dist_data );

  char buffer[ 1024 ];

  std::string s = get_chart_base_url();
  s += chart_size( 525, 185 ); // Set chart size
  s += "cht=bvs";
  s += "&amp;";
  if ( print_style == 1 )
  {
    s += "chf=c,ls,0,EEEEEE,0.2,FFFFFF,0.2";
  }
  else
  {
    s += "chf=bg,s,333333";
  }
  s += "&amp;";
  s += "chg=100,100";
  s += "&amp;";
  s += "chxs=0,ffffff|1,ffffff";
  s += "&amp;";
  s += "chd=t:";
  for ( int i=0; i < max_buckets; i++ )
  {
    snprintf( buffer, sizeof( buffer ), "%s%d", ( i?",":"" ), dist_data[ i ] ); s += buffer;
  }
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chds=0,%d", count_max ); s += buffer;
  s += "&amp;";
  s += "chbh=5";
  s += "&amp;";
  s += "chxt=x";
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chxl=0:|min=%.0f|avg=%.0f|max=%.0f", min, avg, max ); s += buffer;
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chxp=0,1,%.0f,100", 100.0 * ( avg - min ) / ( max - min ) ); s += buffer;
  s += "&amp;";
  s += chart_title( distribution_name + " Distribution" ); // Set chart title

  s += "chts=" + chart_bg_color( print_style ) + ",18";

  return s;
}

#if LOOTRANK_ENABLED == 1
// chart::gear_weights_lootrank =============================================

std::string chart::gear_weights_lootrank( player_t* p )
{
  char buffer[ 1024 ];

  std::string s = std::string();
  s = "http://www.guildox.com/wr.asp?";

  switch ( p -> type )
  {
  case DEATH_KNIGHT: s += "&Cla=2048"; break;
  case DRUID:        s += "&Cla=1024"; break;
  case HUNTER:       s += "&Cla=4";    break;
  case MAGE:         s += "&Cla=128";  break;
  case PALADIN:      s += "&Cla=2";    break;
  case PRIEST:       s += "&Cla=16";   break;
  case ROGUE:        s += "&Cla=8";    break;
  case SHAMAN:       s += "&Cla=64";   break;
  case WARLOCK:      s += "&Cla=256";  break;
  case WARRIOR:      s += "&Cla=1";    break;
  default: p -> sim -> errorf( util::player_type_string( p -> type ) ); assert( 0 ); break;
  }

  switch ( p -> race )
  {
  case RACE_NIGHT_ELF:
  case RACE_HUMAN:
  case RACE_GNOME:
  case RACE_DWARF:
  case RACE_WORGEN:
  case RACE_DRAENEI: s += "&F=A"; break;

  case RACE_ORC:
  case RACE_TROLL:
  case RACE_UNDEAD:
  case RACE_BLOOD_ELF:
  case RACE_GOBLIN:
  case RACE_TAUREN: s += "&F=H"; break;
  default: break;
  }

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    double value = p -> scaling.get_stat( i );
    if ( value == 0 ) continue;

    const char* name;
    switch ( i )
    {
    case STAT_STRENGTH:                 name = "Str";  break;
    case STAT_AGILITY:                  name = "Agi";  break;
    case STAT_STAMINA:                  name = "Sta";  break;
    case STAT_INTELLECT:                name = "Int";  break;
    case STAT_SPIRIT:                   name = "Spi";  break;
    case STAT_SPELL_POWER:              name = "spd";  break;
    case STAT_ATTACK_POWER:             name = "map";  break;
    case STAT_EXPERTISE_RATING:         name = "Exp";  break;
    case STAT_HIT_RATING:               name = "mhit"; break;
    case STAT_CRIT_RATING:              name = "mcr";  break;
    case STAT_HASTE_RATING:             name = "mh";   break;
    case STAT_MASTERY_RATING:           name = "Mr";   break;
    case STAT_ARMOR:                    name = "Arm";  break;
    case STAT_WEAPON_DPS:
      if ( HUNTER == p -> type ) name = "rdps"; else name = "dps";  break;
    case STAT_WEAPON_OFFHAND_DPS:       name = "odps"; break;
    case STAT_WEAPON_SPEED:
      if ( HUNTER == p -> type ) name = "rsp"; else name = "msp"; break;
    case STAT_WEAPON_OFFHAND_SPEED:     name = "osp"; break;
    default: name = 0; break;
    }

    if ( name )
    {
      snprintf( buffer, sizeof( buffer ), "&%s=%.*f", name, p -> sim -> report_precision, value );
      s += buffer;
    }
  }

  s += "&Ver=6";
  util::urlencode( s );

  return s;
}
#endif

// chart::gear_weights_wowhead ==============================================

std::string chart::gear_weights_wowhead( player_t* p )
{
  char buffer[ 1024 ];
  bool first=true;

  // FIXME: switch back to www.wowhead.com once MoP ( including monks ) goes live
  std::string s = "http://mop.wowhead.com/?items&amp;filter=";

  switch ( p -> type )
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
  case MONK:         s += "ub=10;"; break;
  default: assert( 0 ); break;
  }

  // Restrict wowhead to rare gems. When epic gems become available:"gm=4;gb=1;"
  s += "gm=3;gb=1;";

  // Automatically reforge items, and min ilvl of 346 (sensible for
  // current raid tier).
  s += "rf=1;minle=346;";

  std::string    id_string = "";
  std::string value_string = "";

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    double value = p -> scaling.get_stat( i );
    if ( value == 0 ) continue;

    int id=0;
    switch ( i )
    {
    case STAT_STRENGTH:                 id = 20;  break;
    case STAT_AGILITY:                  id = 21;  break;
    case STAT_STAMINA:                  id = 22;  break;
    case STAT_INTELLECT:                id = 23;  break;
    case STAT_SPIRIT:                   id = 24;  break;
    case STAT_SPELL_POWER:              id = 123; break;
    case STAT_ATTACK_POWER:             id = 77;  break;
    case STAT_EXPERTISE_RATING:         id = 117; break;
    case STAT_HIT_RATING:               id = 119; break;
    case STAT_CRIT_RATING:              id = 96;  break;
    case STAT_HASTE_RATING:             id = 103; break;
    case STAT_ARMOR:                    id = 41;  break;
    case STAT_MASTERY_RATING:           id = 170; break;
    case STAT_WEAPON_DPS:
      if ( HUNTER == p -> type ) id = 138; else id = 32;  break;
    default: break;
    }

    if ( id )
    {
      if ( ! first )
      {
        id_string += ":";
        value_string += ":";
      }
      first = false;

      snprintf( buffer, sizeof( buffer ), "%d", id );
      id_string += buffer;

      snprintf( buffer, sizeof( buffer ), "%.*f", p -> sim -> report_precision, value );
      value_string += buffer;
    }
  }

  s += "wt="  +    id_string + ";";
  s += "wtv=" + value_string + ";";

  return s;
}

// chart::gear_weights_wowreforge ===========================================

std::string chart::gear_weights_wowreforge( player_t* p )
{
  char buffer[ 1024 ];

  std::string s = std::string();

  std::string region_str, server_str, name_str;

  // Use valid names if we are provided those
  if ( ! p -> region_str.empty() && ! p -> server_str.empty() && ! p -> name_str.empty() )
  {
    s = "http://wowreforge.com/" + p -> region_str + "/" + p -> server_str + "/" + p -> name_str + "?Spec=Main&amp;template=";
  }
  else
  {
    if ( util::parse_origin( region_str, server_str, name_str, p -> origin_str ) )
    {
      s = "http://wowreforge.com/" + region_str + "/" + server_str + "/" + name_str + "?Spec=Main&amp;template=";
    }
    else
    {
      s = "http://wowreforge.com/?template=";
    }
  }

  s += "for:";
  s += util::player_type_string( p -> type );
  s += "-";
  s += util::specialization_string( p -> specialization() );

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    double value = p -> scaling.get_stat( i );
    if ( value == 0 ) continue;

    snprintf( buffer, sizeof( buffer ), ",%s:%.*f", util::stat_type_abbrev( i ), p -> sim -> report_precision, value );
    s += buffer;
  }

  util::urlencode( s );

  return s;
}

// chart::gear_weights_pawn =================================================

std::string chart::gear_weights_pawn( player_t* p,
                                      bool hit_expertise )
{
  std::vector<stat_e> stats;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    stats.push_back( i );

  std::string s = std::string();
  char buffer[ 1024 ];
  bool first = true;

  s.clear();
  snprintf( buffer, sizeof( buffer ), "( Pawn: v1: \"%s\": ", p -> name() );
  s += buffer;

  double maxR = 0;
  double maxB = 0;
  double maxY = 0;

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    stat_e stat = stats[ i ];

    double value = p -> scaling.get_stat( stat );
    if ( value == 0 ) continue;

    if ( ! hit_expertise )
      if ( stat == STAT_HIT_RATING || stat == STAT_EXPERTISE_RATING )
        value = 0;

    const char* name=0;
    switch ( stat )
    {
    case STAT_STRENGTH:                 name = "Strength";         if ( value*20 > maxR ) maxR = value*20; break;
    case STAT_AGILITY:                  name = "Agility";          if ( value*20 > maxR ) maxR = value*20; break;
    case STAT_STAMINA:                  name = "Stamina";          if ( value*20 > maxB ) maxB = value*20; break;
    case STAT_INTELLECT:                name = "Intellect";        if ( value*20 > maxY ) maxY = value*20; break;
    case STAT_SPIRIT:                   name = "Spirit";           if ( value*20 > maxB ) maxB = value*20; break;
    case STAT_SPELL_POWER:              name = "SpellDamage";      if ( value*20 > maxR ) maxR = value*20; break;
    case STAT_ATTACK_POWER:             name = "Ap";               if ( value*20 > maxR ) maxR = value*20; break;
    case STAT_EXPERTISE_RATING:         name = "ExpertiseRating";  if ( value*20 > maxR ) maxR = value*20; break;
    case STAT_HIT_RATING:               name = "HitRating";        if ( value*20 > maxY ) maxY = value*20; break;
    case STAT_CRIT_RATING:              name = "CritRating";       if ( value*20 > maxY ) maxY = value*20; break;
    case STAT_HASTE_RATING:             name = "HasteRating";      if ( value*20 > maxY ) maxY = value*20; break;
    case STAT_MASTERY_RATING:           name = "MasteryRating";    if ( value*20 > maxY ) maxY = value*20; break;
    case STAT_ARMOR:                    name = "Armor";            break;
    case STAT_WEAPON_DPS:
      if ( HUNTER == p -> type ) name = "RangedDps"; else name = "MeleeDps";  break;
    default: break;
    }

    if ( name )
    {
      if ( ! first ) s += ",";
      first = false;
      snprintf( buffer, sizeof( buffer ), " %s=%.*f", name, p -> sim -> report_precision, value );
      s += buffer;
    }
  }

  s += " )";

  return s;
}

// chart::dps_error =========================================================

std::string chart::dps_error( player_t* p )
{
  char buffer[ 1024 ];

  double std_dev = p -> dps.mean_std_dev;

  std::string s = get_chart_base_url();
  s += chart_size( 525, 185 ); // Set chart size
  s += "cht=lc";
  s += "&amp;";
  s += "chg=20,20";
  s += "&amp;";
  s += "chco=FF0000";
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chxr=0,%.0f,%.0f|2,0,%.4f", p -> dps.mean - std_dev * 4, p -> dps.mean + std_dev * 4, 1 / ( std_dev * sqrt ( 0.5 * M_PI ) ) ); s += buffer;

  s += "&amp;";
  if ( p -> sim -> print_styles == 1 )
  {
    s += "chf=c,ls,0,EEEEEE,0.2,FFFFFF,0.2";
  }
  else
  {
    s += "chf=bg,s,333333";
  }
  s += "&amp;";
  s += "chxt=x,x,y,y";
  s += "&amp;";
  s += "chxl=1:|DPS|3:|p";
  s += "&amp;";
  s += chart_title( util::to_string( p -> sim -> confidence * 100.0, 2 ) + "%" + " Confidence Interval" ); // Set chart title

  if ( p -> sim -> print_styles == 1 )
  {
    s += "chxs=0,000000|1,000000|2,000000|3,000000";
  }
  else
  {
    s += "chxs=0,ffffff|1,ffffff|2,ffffff|3,ffffff";
  }
  s += "&amp;";

  s += "chts=" + chart_bg_color( p -> sim -> print_styles ) + ",18";
  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chfd=0,x,%.2f,%.2f,%.8f,100*exp(-(x-%.4f)^2/(2*%.2f^2))", p -> dps.mean - std_dev * 4, p -> dps.mean + std_dev * 4, std_dev / 100.0, p -> dps.mean, std_dev ); s += buffer;

  s += "&amp;";
  s += "chd=t:-1";

  s += "&amp;";
  snprintf( buffer, sizeof( buffer ), "chm=B,C6D9FD,0,%.0f:%.0f,0", std::max( 4 * std_dev - p -> dps_error, 0.0 ) * 100.0 / std_dev, floor( std::min( 4 * std_dev + p -> dps_error, 8* std_dev ) * 100.0 / std_dev ) ); s += buffer;


  return s;
}

// chart::resource_color ================================================

std::string chart::resource_color( int type )
{
  switch ( type )
  {
  case RESOURCE_HEALTH:
  case RESOURCE_RUNE_UNHOLY:   return class_color( HUNTER );

  case RESOURCE_RUNE_FROST:
  case RESOURCE_MANA:          return class_color( SHAMAN );

  case RESOURCE_ENERGY:
  case RESOURCE_FOCUS:         return class_text_color( ROGUE );

  case RESOURCE_RAGE:
  case RESOURCE_RUNIC_POWER:
  case RESOURCE_RUNE:
  case RESOURCE_RUNE_BLOOD:    return class_color( DEATH_KNIGHT );

  case RESOURCE_HOLY_POWER:    return class_color( PALADIN );

  case RESOURCE_SOUL_SHARD:
  case RESOURCE_BURNING_EMBER:
  case RESOURCE_DEMONIC_FURY:  return class_color( WARLOCK );

  case RESOURCE_CHI:           return class_color( MONK );

  case RESOURCE_NONE:
  default:                   return "000000";
  }
}
