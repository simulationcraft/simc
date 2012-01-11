// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================


// print_html_contents ======================================================

static void print_html_contents( FILE*  file, sim_t* sim )
{
  size_t c = 2;     // total number of TOC entries
  if ( sim -> scaling -> has_scale_factors() )
    ++c;

  const int num_players = ( int ) sim -> players_by_name.size();
  c += num_players;
  if ( sim -> report_targets )
    c += sim -> targets_by_name.size();

  if ( sim -> report_pets_separately )
  {
    for ( int i=0; i < num_players; i++ )
    {
      for ( pet_t* pet = sim -> players_by_name[ i ] -> pet_list; pet; pet = pet -> next_pet )
      {
        if ( pet -> summoned )
          ++c;
      }
    }
  }

  fputs( "\t\t<div id=\"table-of-contents\" class=\"section grouped-first grouped-last\">\n"
         "\t\t\t<h2 class=\"toggle\">Table of Contents</h2>\n"
         "\t\t\t<div class=\"toggle-content hide\">\n", file );

  // set number of columns
  int n;         // number of columns
  const char* toc_class; // css class
  if ( c < 6 )
  {
    n = 1;
    toc_class = "toc-wide";
  }
  else if ( sim -> report_pets_separately )
  {
    n = 2;
    toc_class = "toc-wide";
  }
  else if ( c < 9 )
  {
    n = 2;
    toc_class = "toc-narrow";
  }
  else
  {
    n = 3;
    toc_class = "toc-narrow";
  }

  int pi = 0;    // player counter
  int ab = 0;    // auras and debuffs link added yet?
  for ( int i=0; i < n; i++ )
  {
    int cs;    // column size
    if ( i == 0 )
    {
      cs = ( int ) ceil( 1.0 * c / n );
    }
    else if ( i == 1 )
    {
      if ( n == 2 )
      {
        cs = ( int ) ( c - ceil( 1.0 * c / n ) );
      }
      else
      {
        cs = ( int ) ceil( 1.0 * c / n );
      }
    }
    else
    {
      cs = ( int ) ( c - 2 * ceil( 1.0 * c / n ) );
    }

    fprintf( file,
             "\t\t\t\t<ul class=\"toc %s\">\n",
             toc_class );

    int ci = 1;    // in-column counter
    if ( i == 0 )
    {
      fputs( "\t\t\t\t\t<li><a href=\"#raid-summary\">Raid Summary</a></li>\n", file );
      ci++;
      if ( sim -> scaling -> has_scale_factors() )
      {
        fputs( "\t\t\t\t\t<li><a href=\"#raid-scale-factors\">Scale Factors</a></li>\n", file );
        ci++;
      }
    }
    while ( ci <= cs )
    {
      if ( pi < ( int ) sim -> players_by_name.size() )
      {
        player_t* p = sim -> players_by_name[ pi ];
        fprintf( file,
                 "\t\t\t\t\t<li><a href=\"#%s\">%s</a>",
                 p -> name(), p -> name() );
        ci++;
        if ( sim -> report_pets_separately )
        {
          fputs( "\n\t\t\t\t\t\t<ul>\n", file );
          for ( pet_t* pet = sim -> players_by_name[ pi ] -> pet_list; pet; pet = pet -> next_pet )
          {
            if ( pet -> summoned )
            {
              fprintf( file,
                       "\t\t\t\t\t\t\t<li><a href=\"#%s\">%s</a></li>\n",
                       pet -> name(), pet -> name() );
              ci++;
            }
          }
          fputs( "\t\t\t\t\t\t</ul>", file );
        }
        fputs( "</li>\n", file );
        pi++;
      }
      if ( pi == ( int ) sim -> players_by_name.size() )
      {
        if ( ab == 0 )
        {
          fputs( "\t\t\t\t\t\t<li><a href=\"#auras-buffs\">Auras/Buffs</a></li>\n", file );
          ab = 1;
        }
        ci++;
        fputs( "\t\t\t\t\t\t<li><a href=\"#sim-info\">Simulation Information</a></li>\n", file );
        ci++;
      }
      if ( sim -> report_targets && ab > 0 )
      {
        if ( ab == 1 )
        {
          pi = 0;
          ab = 2;
        }
        while ( ci <= cs )
        {
          if ( pi < ( int ) sim -> targets_by_name.size() )
          {
            player_t* p = sim -> targets_by_name[ pi ];
            fprintf( file,
                     "\t\t\t\t\t<li><a href=\"#%s\">%s</a></li>\n",
                     p -> name(), p -> name() );
          }
          ci++;
          pi++;
        }
      }
    }
    fputs( "\t\t\t\t</ul>\n", file );
  }

  fputs( "\t\t\t\t<div class=\"clear\"></div>\n"
         "\t\t\t</div>\n\n"
         "\t\t</div>\n\n", file );
}



// print_html_sim_summary ===================================================

static void print_html_sim_summary( FILE*  file, sim_t* sim )
{

  fprintf( file,
           "\t\t\t\t<div id=\"sim-info\" class=\"section\">\n" );

  fprintf( file,
           "\t\t\t\t\t<h2 class=\"toggle\">Simulation & Raid Information</h2>\n"
           "\t\t\t\t\t\t<div class=\"toggle-content hide\">\n" );

  fprintf( file,
           "\t\t\t\t\t\t<table class=\"sc mt\">\n" );

  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>Iterations:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%d</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           sim -> iterations );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>Threads:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%d</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           sim -> threads < 1 ? 1 : sim -> threads );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>Confidence:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%.2f%%</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           sim -> confidence * 100.0 );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>Fight Length:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%.0f - %.0f ( %.1f )</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           sim -> simulation_length.min,
           sim -> simulation_length.max,
           sim -> simulation_length.mean );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th><h2>Performance:</h2></th>\n"
           "\t\t\t\t\t\t\t\t<td></td>\n"
           "\t\t\t\t\t\t\t</tr>\n" );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>Total Events Processed:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%ld</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           ( long ) sim -> total_events_processed );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>Max Event Queue:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%ld</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           ( long ) sim -> max_events_remaining );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>Sim Seconds:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%.0f</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           sim -> iterations * sim -> simulation_length.mean );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>CPU Seconds:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%.4f</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           sim -> elapsed_cpu.total_seconds() );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>Speed Up:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%.0f</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           sim -> iterations * sim -> simulation_length.mean / sim -> elapsed_cpu.total_seconds() );

  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th><h2>Settings:</h2></th>\n"
           "\t\t\t\t\t\t\t\t<td></td>\n"
           "\t\t\t\t\t\t\t</tr>\n" );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>World Lag:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%.0f ms ( stddev = %.0f ms )</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           (double)sim -> world_lag.total_millis(), (double)sim -> world_lag_stddev.total_millis() );
  fprintf( file,
           "\t\t\t\t\t\t\t<tr class=\"left\">\n"
           "\t\t\t\t\t\t\t\t<th>Queue Lag:</th>\n"
           "\t\t\t\t\t\t\t\t<td>%.0f ms ( stddev = %.0f ms )</td>\n"
           "\t\t\t\t\t\t\t</tr>\n",
           (double)sim -> queue_lag.total_millis(), (double)sim -> queue_lag_stddev.total_millis() );
  if ( sim -> strict_gcd_queue )
  {
    fprintf( file,
             "\t\t\t\t\t\t\t<tr class=\"left\">\n"
             "\t\t\t\t\t\t\t\t<th>GCD Lag:</th>\n"
             "\t\t\t\t\t\t\t\t<td>%.0f ms ( stddev = %.0f ms )</td>\n"
             "\t\t\t\t\t\t\t</tr>\n",
             (double)sim -> gcd_lag.total_millis(), (double)sim -> gcd_lag_stddev.total_millis() );
    fprintf( file,
             "\t\t\t\t\t\t\t<tr class=\"left\">\n"
             "\t\t\t\t\t\t\t\t<th>Channel Lag:</th>\n"
             "\t\t\t\t\t\t\t\t<td>%.0f ms ( stddev = %.0f ms )</td>\n"
             "\t\t\t\t\t\t\t</tr>\n",
             (double)sim -> channel_lag.total_millis(), (double)sim -> channel_lag_stddev.total_millis() );
    fprintf( file,
             "\t\t\t\t\t\t\t<tr class=\"left\">\n"
             "\t\t\t\t\t\t\t\t<th>Queue GCD Reduction:</th>\n"
             "\t\t\t\t\t\t\t\t<td>%.0f ms</td>\n"
             "\t\t\t\t\t\t\t</tr>\n",
             (double)sim -> queue_gcd_reduction.total_millis() );
  }


  fprintf( file,
           "\t\t\t\t\t\t</table>\n" );

  // Left side charts: dps, gear, timeline, raid events

  fprintf( file,
           "\t\t\t\t<div class=\"charts charts-left\">\n" );
  // Timeline Distribution Chart
  if ( sim -> iterations > 1 && ! sim -> timeline_chart.empty() )
  {
    fprintf( file,
             "\t\t\t\t\t<a href=\"#help-timeline-distribution\" class=\"help\"><img src=\"%s\" alt=\"Timeline Distribution Chart\" /></a>\n",
             sim -> timeline_chart.c_str() );
  }
  // Gear Charts
  int count = ( int ) sim -> gear_charts.size();
  for ( int i=0; i < count; i++ )
  {
    fprintf( file,
             "\t\t\t\t\t<img src=\"%s\" alt=\"Gear Chart\" />\n",
             sim -> gear_charts[ i ].c_str() );
  }
  // Raid Downtime Chart
  if ( ! sim -> downtime_chart.empty() )
  {
    fprintf( file,
             "\t\t\t\t\t<img src=\"%s\" alt=\"Player Downtime Chart\" />\n",
             sim -> downtime_chart.c_str() );
  }

  fprintf( file,
           "\t\t\t\t</div>\n" );

  // Right side charts: dpet
  fprintf( file,
           "\t\t\t\t<div class=\"charts\">\n" );
  count = ( int ) sim -> dpet_charts.size();
  for ( int i=0; i < count; i++ )
  {
    fprintf( file,
             "\t\t\t\t\t<img src=\"%s\" alt=\"DPET Chart\" />\n",
             sim -> dpet_charts[ i ].c_str() );
  }



  fprintf( file,
           "\t\t\t\t</div>\n" );


  // closure
  fprintf( file,
           "\t\t\t\t<div class=\"clear\"></div>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n\n" );
}

// print_html_raid_summary ==================================================

static void print_html_raid_summary( FILE*  file, sim_t* sim )
{
  fprintf( file,
           "\t\t<div id=\"raid-summary\" class=\"section section-open\">\n\n" );
  fprintf( file,
           "\t\t\t<h2 class=\"toggle open\">Raid Summary</h2>\n" );
  fprintf( file,
           "\t\t\t<div class=\"toggle-content\">\n" );
  fprintf( file,
           "\t\t\t<ul class=\"params\">\n" );
  fprintf( file,
           "\t\t\t\t<li><b>Raid Damage:</b> %.0f</li>\n",
           sim -> total_dmg.mean );
  fprintf( file,
           "\t\t\t\t<li><b>Raid DPS:</b> %.0f</li>\n",
           sim -> raid_dps.mean );
  if ( sim -> total_heal.mean > 0 )
  {
    fprintf( file,
             "\t\t\t\t<li><b>Raid Heal:</b> %.0f</li>\n",
             sim -> total_heal.mean );
    fprintf( file,
             "\t\t\t\t<li><b>Raid HPS:</b> %.0f</li>\n",
             sim -> raid_hps.mean );
  }
  fprintf( file,
           "\t\t\t</ul><p>&nbsp;</p>\n" );

  // Left side charts: dps, raid events
  fprintf( file,
           "\t\t\t\t<div class=\"charts charts-left\">\n" );
  int count = ( int ) sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    fprintf( file, "\t\t\t\t\t<map id='DPSMAP%d' name='DPSMAP%d'></map>\n", i, i );
    fprintf( file,
             "\t\t\t\t\t<img id='DPSIMG%d' src=\"%s\" alt=\"DPS Chart\" />\n",
             i, sim -> dps_charts[ i ].c_str() );
  }

  if ( ! sim -> raid_events_str.empty() )
  {
    fprintf( file,
             "\t\t\t\t\t<table>\n"
             "\t\t\t\t\t\t<tr>\n"
             "\t\t\t\t\t\t\t<th></th>\n"
             "\t\t\t\t\t\t\t<th class=\"left\">Raid Event List</th>\n"
             "\t\t\t\t\t\t</tr>\n" );
    std::vector<std::string> raid_event_names;
    int num_raid_events = util_t::string_split( raid_event_names, sim -> raid_events_str, "/" );
    for ( int i=0; i < num_raid_events; i++ )
    {
      fprintf( file,
               "\t\t\t\t\t\t<tr" );
      if ( ( i & 1 ) )
      {
        fprintf( file, " class=\"odd\"" );
      }
      fprintf( file, ">\n" );
      fprintf( file,
               "\t\t\t\t\t\t\t<th class=\"right\">%d</th>\n"
               "\t\t\t\t\t\t\t<td class=\"left\">%s</td>\n"
               "\t\t\t\t\t\t</tr>\n",
               i,
               raid_event_names[ i ].c_str() );
    }
    fprintf( file,
             "\t\t\t\t\t</table>\n" );
  }
  fprintf( file,
           "\t\t\t\t</div>\n" );

  // Right side charts: hps
  fprintf( file,
           "\t\t\t\t<div class=\"charts\">\n" );

  count = ( int ) sim -> hps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    fprintf( file, "\t\t\t\t\t<map id='HPSMAP%d' name='HPSMAP%d'></map>\n", i, i );
    fprintf( file,
             "\t\t\t\t\t<img id='HPSIMG%d' src=\"%s\" alt=\"HPS Chart\" />\n",
             i, sim -> hps_charts[ i ].c_str() );
  }
  // RNG chart
  if ( sim -> report_rng )
  {
    fprintf( file,
             "\t\t\t\t\t<ul>\n" );
    for ( int i=0; i < ( int ) sim -> players_by_name.size(); i++ )
    {
      player_t* p = sim -> players_by_name[ i ];
      double range = ( p -> dps.percentile( 0.95 ) - p -> dps.percentile( 0.05 ) ) / 2.0;
      fprintf( file,
               "\t\t\t\t\t\t<li>%s: %.1f / %.1f%%</li>\n",
               p -> name(),
               range,
               p -> dps.mean ? ( range * 100 / p -> dps.mean ) : 0 );
    }
    fprintf( file,
             "\t\t\t\t\t</ul>\n" );
  }

  fprintf( file,
           "\t\t\t\t</div>\n" );

  // closure
  fprintf( file,
           "\t\t\t\t<div class=\"clear\"></div>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n\n" );

}

// print_html_raid_imagemaps ==================================================

static void print_html_raid_imagemap( FILE* file, sim_t* sim, int num, bool dps )
{
  std::vector<player_t*> player_list = ( dps ) ? sim -> players_by_dps : sim -> players_by_hps;
  int start = num * MAX_PLAYERS_PER_CHART;
  unsigned int end = start + MAX_PLAYERS_PER_CHART;

  for ( unsigned int i=0; i < player_list.size(); i++ )
  {
    player_t* p = player_list[ i ];
    if ( dps ? p -> dps.mean <= 0 : p -> hps.mean <=0 )
    {
      player_list.resize( i );
      break;
    }
  }

  if ( end > player_list.size() ) end = static_cast<unsigned>( player_list.size() );

  fprintf( file, "\t\t\tn = [" );
  for ( int i=end-1; i >= start; i-- )
  {
    fprintf( file, "\"%s\"", player_list[i] -> name_str.c_str() );
    if ( i != start ) fprintf( file, ", " );
  }
  fprintf( file, "];\n" );

  char imgid[32];
  util_t::snprintf( imgid, sizeof( imgid ), "%sIMG%d", ( dps ) ? "DPS" : "HPS", num );
  char mapid[32];
  util_t::snprintf( mapid, sizeof( mapid ), "%sMAP%d", ( dps ) ? "DPS" : "HPS", num );

  fprintf( file, "\t\t\tu = document.getElementById('%s').src;\n"
                 "\t\t\tgetMap(u, n, function(mapStr) {\n"
                 "\t\t\t\tdocument.getElementById('%s').innerHTML += mapStr;\n"
                 "\t\t\t\t$j('#%s').attr('usemap','#%s');\n"
                 "\t\t\t\t$j('#%s area').click(function(e) {\n"
                 "\t\t\t\t\tanchor = $j(this).attr('href');\n"
                 "\t\t\t\t\ttarget = $j(anchor).children('h2:first');\n"
                 "\t\t\t\t\topen_anchor(target);\n"
                 "\t\t\t\t});\n"
                 "\t\t\t});\n\n",
                 imgid, mapid, imgid, mapid, mapid );
}

static void print_html_raid_imagemaps( FILE*  file, sim_t* sim )
{

  fprintf( file, "\t\t<script type=\"text/javascript\">\n"
                 "\t\t\tvar $j = jQuery.noConflict();\n"
                 "\t\t\tfunction getMap(url, names, mapWrite) {\n"
                 "\t\t\t\t$j.getJSON(url + '&chof=json&callback=?', function(jsonObj) {\n"
                 "\t\t\t\t\tvar area = false;\n"
                 "\t\t\t\t\tvar chart = jsonObj.chartshape;\n"
                 "\t\t\t\t\tvar mapStr = '';\n"
                 "\t\t\t\t\tfor (var i = 0; i < chart.length; i++) {\n"
                 "\t\t\t\t\t\tarea = chart[i];\n"
                 "\t\t\t\t\t\tarea.coords[2] = 523;\n"
                 "\t\t\t\t\t\tmapStr += \"\\n  <area name='\" + area.name + \"' shape='\" + area.type + \"' coords='\" + area.coords.join(\",\") + \"' href='#\" + names[i] + \"'  title='\" + names[i] + \"'>\";\n"
                 "\t\t\t\t\t}\n"
                 "\t\t\t\t\tmapWrite(mapStr);\n"
                 "\t\t\t\t});\n"
                 "\t\t\t}\n\n" );

  int count = ( int ) sim -> dps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    print_html_raid_imagemap( file, sim, i, true );
  }

  count = ( int ) sim -> hps_charts.size();
  for ( int i=0; i < count; i++ )
  {
    print_html_raid_imagemap( file, sim, i, false );
  }

  fprintf( file, "\t\t</script>\n" );

}

// print_html_scale_factors =================================================

static void print_html_scale_factors( FILE*  file, sim_t* sim )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;

  if ( sim -> report_precision < 0 )
    sim -> report_precision = 2;

  fprintf( file,
           "\t\t<div id=\"raid-scale-factors\" class=\"section grouped-first\">\n\n"
           "\t\t\t<h2 class=\"toggle\">DPS Scale Factors (dps increase per unit stat)</h2>\n"
           "\t\t\t<div class=\"toggle-content hide\">\n" );

  fprintf( file,
           "\t\t\t\t<table class=\"sc\">\n" );

  std::string buffer;
  int num_players = ( int ) sim -> players_by_name.size();
  int prev_type = PLAYER_NONE;

  for ( int i=0; i < num_players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    if ( p -> type != prev_type )
    {
      prev_type = p -> type;

      fprintf( file,
               "\t\t\t\t\t<tr>\n"
               "\t\t\t\t\t\t<th class=\"left small\">Profile</th>\n" );
      for ( int j=0; j < STAT_MAX; j++ )
      {
        if ( sim -> scaling -> stats.get_stat( j ) != 0 )
        {
          fprintf( file,
                   "\t\t\t\t\t\t<th class=\"small\">%s</th>\n",
                   util_t::stat_type_abbrev( j ) );
        }
      }
      fprintf( file,
               "\t\t\t\t\t\t<th class=\"small\">wowhead</th>\n"
               "\t\t\t\t\t\t<th class=\"small\">lootrank</th>\n"
               "\t\t\t\t\t</tr>\n" );
    }

    fprintf( file,
             "\t\t\t\t\t<tr" );
    if ( ( i & 1 ) )
    {
      fprintf( file, " class=\"odd\"" );
    }
    fprintf( file, ">\n" );
    fprintf( file,
             "\t\t\t\t\t\t<td class=\"left small\">%s</td>\n",
             p -> name() );
    for ( int j=0; j < STAT_MAX; j++ )
    {
      if ( sim -> scaling -> stats.get_stat( j ) != 0 )
      {
        if ( p -> scaling.get_stat( j ) == 0 )
        {
          fprintf( file, "\t\t\t\t\t\t<td class=\"small\">-</td>\n" );
        }
        else
        {
          fprintf( file,
                   "\t\t\t\t\t\t<td class=\"small\">%.*f</td>\n",
                   sim -> report_precision,
                   p -> scaling.get_stat( j ) );
        }
      }
    }
    fprintf( file,
             "\t\t\t\t\t\t<td class=\"small\"><a href=\"%s\"> wowhead </a></td>\n"
             "\t\t\t\t\t\t<td class=\"small\"><a href=\"%s\"> lootrank</a></td>\n"
             "\t\t\t\t\t</tr>\n",
             p -> gear_weights_wowhead_link.c_str(),
             p -> gear_weights_lootrank_link.c_str() );
  }
  fprintf( file,
           "\t\t\t\t</table>\n" );
  if ( sim -> iterations < 10000 )
    fprintf( file,
             "\t\t\t\t<div class=\"alert\">\n"
             "\t\t\t\t\t<h3>Warning</h3>\n"
             "\t\t\t\t\t<p>Scale Factors generated using less than 10,000 iterations will vary from run to run.</p>\n"
             "\t\t\t\t</div>\n" );
  fprintf( file,
           "\t\t\t</div>\n"
           "\t\t</div>\n\n" );
}

// print_html_help_boxes ====================================================

static void print_html_help_boxes( FILE*  file, sim_t* sim )
{
  fprintf( file,
           "\t\t<!-- Help Boxes -->\n" );

  fprintf( file,
           "\t\t<div id=\"help-apm\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>APM</h3>\n"
           "\t\t\t\t<p>Average number of actions executed per minute.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-constant-buffs\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Constant Buffs</h3>\n"
           "\t\t\t\t<p>Buffs received prior to combat and present the entire fight.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-count\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Count</h3>\n"
           "\t\t\t\t<p>Average number of times an action is executed per iteration.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-crit\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Crit</h3>\n"
           "\t\t\t\t<p>Average crit damage.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-crit-pct\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Crit%%</h3>\n"
           "\t\t\t\t<p>Percentage of executes that resulted in critical strikes.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-dodge-pct\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Dodge%%</h3>\n"
           "\t\t\t\t<p>Percentage of executes that resulted in dodges.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-dpe\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>DPE</h3>\n"
           "\t\t\t\t<p>Average damage per execution of an individual action.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-dpet\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>DPET</h3>\n"
           "\t\t\t\t<p>Average damage per execute time of an individual action; the amount of damage generated, divided by the time taken to execute the action, including time spent in the GCD.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-dpr\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>DPR</h3>\n"
           "\t\t\t\t<p>Average damage per resource point spent.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-dps\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>DPS</h3>\n"
           "\t\t\t\t<p>Average damage per active player duration.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-dpse\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Effective DPS</h3>\n"
           "\t\t\t\t<p>Average damage per fight duration.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-dps-pct\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>DPS%%</h3>\n"
           "\t\t\t\t<p>Percentage of total DPS contributed by a particular action.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-dynamic-buffs\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Dynamic Buffs</h3>\n"
           "\t\t\t\t<p>Temporary buffs received during combat, perhaps multiple times.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-error\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Error</h3>\n"
           "\t\t\t\t<p>Estimator for the %.2f%% confidence intervall.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n",
           sim -> confidence * 100.0 );

  fprintf( file,
           "\t\t<div id=\"help-glance-pct\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>G%%</h3>\n"
           "\t\t\t\t<p>Percentage of executes that resulted in glancing blows.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-block-pct\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>G%%</h3>\n"
           "\t\t\t\t<p>Percentage of executes that resulted in blocking blows.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-hit\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Hit</h3>\n"
           "\t\t\t\t<p>Average non-crit damage.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-interval\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Interval</h3>\n"
           "\t\t\t\t<p>Average time between executions of a particular action.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-max\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Max</h3>\n"
           "\t\t\t\t<p>Maximum crit damage over all iterations.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-miss-pct\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>M%%</h3>\n"
           "\t\t\t\t<p>Percentage of executes that resulted in misses, dodges or parries.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-origin\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Origin</h3>\n"
           "\t\t\t\t<p>The player profile from which the simulation script was generated. The profile must be copied into the same directory as this HTML file in order for the link to work.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-parry-pct\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Parry%%</h3>\n"
           "\t\t\t\t<p>Percentage of executes that resulted in parries.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-range\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Range</h3>\n"
           "\t\t\t\t<p>( dps.percentile( 0.95 ) - dps.percentile( 0.05 ) / 2</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-rps-in\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>RPS In</h3>\n"
           "\t\t\t\t<p>Average resource points generated per second.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-rps-out\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>RPS Out</h3>\n"
           "\t\t\t\t<p>Average resource points consumed per second.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-scale-factors\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Scale Factors</h3>\n"
           "\t\t\t\t<p>DPS gain per unit stat increase except for <b>Hit/Expertise</b> which represent <b>DPS loss</b> per unit stat <b>decrease</b>.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-ticks\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Ticks</h3>\n"
           "\t\t\t\t<p>Average number of periodic ticks per iteration. Spells that do not have a damage-over-time component will have zero ticks.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-ticks-crit\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>T-Crit</h3>\n"
           "\t\t\t\t<p>Average crit tick damage.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-ticks-crit-pct\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>T-Crit%%</h3>\n"
           "\t\t\t\t<p>Percentage of ticks that resulted in critical strikes.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-ticks-hit\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>T-Hit</h3>\n"
           "\t\t\t\t<p>Average non-crit tick damage.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-ticks-miss-pct\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>T-M%%</h3>\n"
           "\t\t\t\t<p>Percentage of ticks that resulted in misses, dodges or parries.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-ticks-uptime\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>UpTime%%</h3>\n"
           "\t\t\t\t<p>Percentage of total time that DoT is ticking on target.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-timeline-distribution\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Timeline Distribution</h3>\n"
           "\t\t\t\t<p>The simulated encounter's duration can vary based on the health of the target and variation in the raid DPS. This chart shows how often the duration of the encounter varied by how much time.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<div id=\"help-fight-length\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Fight Length</h3>\n"
           "\t\t\t\t<p>Fight Length: %.0f<br />\n"
           "\t\t\t\tVary Combat Length: %.2f</p>\n"
           "\t\t\t\t<p>Fight Length is the specified average fight duration. If vary_combat_length is set, the fight length will vary by +/- that portion of the value. See <a href=\"http://code.google.com/p/simulationcraft/wiki/Options#Combat_Length\" class=\"ext\">Combat Length</a> in the wiki for further details.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n",
           sim -> max_time.total_seconds(),
           sim -> vary_combat_length );

  fprintf( file,
           "\t\t<div id=\"help-waiting\">\n"
           "\t\t\t<div class=\"help-box\">\n"
           "\t\t\t\t<h3>Waiting</h3>\n"
           "\t\t\t\t<p>This is the percentage of time in which no action can be taken other than autoattacks. This can be caused by resource starvation, lockouts, and timers.</p>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n" );

  fprintf( file,
           "\t\t<!-- End Help Boxes -->\n" );
}

// print_html_styles ====================================================

static void print_html_styles( FILE*  file, sim_t* sim )
{
  // Styles
  // If file is being hosted on simulationcraft.org, link to the local
  // stylesheet; otherwise, embed the styles.
  if ( sim -> hosted_html )
  {
    fprintf( file,
             "\t\t<style type=\"text/css\" media=\"screen\">\n"
             "\t\t\t@import url('http://www.simulationcraft.org/css/styles.css');\n"
             "\t\t</style>\n"
             "\t\t<style type=\"text/css\" media=\"print\">\n"
             "\t\t  @import url('http://www.simulationcraft.org/css/styles-print.css');\n"
             "\t\t</style>\n" );
  }
  else if ( sim -> print_styles )
  {
    fprintf( file,
             "\t\t<style type=\"text/css\" media=\"all\">\n"
             "\t\t\t* {border: none;margin: 0;padding: 0; }\n"
             "\t\t\tbody {padding: 5px 25px 25px 25px;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 14px;background-color: #f9f9f9;color: #333;text-align: center; }\n"
             "\t\t\tp {margin: 1em 0 1em 0; }\n"
             "\t\t\th1, h2, h3, h4, h5, h6 {width: auto;color: #777;margin-top: 1em;margin-bottom: 0.5em; }\n"
             "\t\t\th1, h2 {margin: 0;padding: 2px 2px 0 2px; }\n"
             "\t\t\th1 {font-size: 24px; }\n"
             "\t\t\th2 {font-size: 18px; }\n"
             "\t\t\th3 {margin: 0 0 4px 0;font-size: 16px; }\n"
             "\t\t\th4 {font-size: 12px; }\n"
             "\t\t\th5 {font-size: 10px; }\n"
             "\t\t\ta {color: #666688;text-decoration: none; }\n"
             "\t\t\ta:hover, a:active {color: #333; }\n"
             "\t\t\tul, ol {padding-left: 20px; }\n"
             "\t\t\tul.float, ol.float {padding: 0;margin: 0; }\n"
             "\t\t\tul.float li, ol.float li {display: inline;float: left;padding-right: 6px;margin-right: 6px;list-style-type: none;border-right: 2px solid #eee; }\n"
             "\t\t\t.clear {clear: both; }\n"
             "\t\t\t.hide, .charts span {display: none; }\n"
             "\t\t\t.center {text-align: center; }\n"
             "\t\t\t.float {float: left; }\n"
             "\t\t\t.mt {margin-top: 20px; }\n"
             "\t\t\t.mb {margin-bottom: 20px; }\n"
             "\t\t\t.force-wrap {word-wrap: break-word; }\n"
             "\t\t\t.mono {font-family: \"Lucida Console\", Monaco, monospace;font-size: 12px; }\n"
             "\t\t\t.toggle {cursor: pointer; }\n"
             "\t\t\th2.toggle {padding-left: 16px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAeCAIAAACT/LgdAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD1SURBVHja7JQ9CoQwFIT9LURQG3vBwyh4XsUjWFtb2IqNCmIhkp1dd9dsfIkeYKdKHl+G5CUTvaqqrutM09Tk2rYtiiIrjuOmaeZ5VqBBEADVGWPTNJVlOQwDyYVhmKap4zgGJp7nJUmCpQoOY2Mv+b6PkkDz3IGevQUOeu6VdxrHsSgK27azLOM5AoVwPqCu6wp1ApXJ0G7rjx5oXdd4YrfQtm3xFJdluUYRBFypghb32ve9jCaOJaPpDpC0tFmg8zzn46nq6/rSd2opAo38IHMXrmeOdgWHACKVFx3Y/c7cjys+JkSP9HuLfYR/Dg1icj0EGACcXZ/44V8+SgAAAABJRU5ErkJggg==) 0 -10px no-repeat; }\n"
             "\t\t\th2.open {margin-bottom: 10px;background-position: 0 9px; }\n"
             "\t\t\th3.toggle {padding-left: 16px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAaCAIAAAAMmCo2AAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAEfSURBVHjazJPLjkRAGIXbdSM8ACISvWeDNRYeGuteuL2EdMSGWLrOmdExaCO9nLOq+vPV+S9VRTwej6IoGIYhCOK21zzPfd/f73da07TiRxRFbTkQ4zjKsqyqKoFN27ZhGD6fT5ZlV2IYBkVRXNflOI5ESBAEz/NEUYT5lnAcBwQi307L6aZpoiiqqgprSZJwbCF2EFTXdRAENE37vr8SR2jhAPE8vw0eoVORtw/0j6Fpmi7afEFlWeZ5jhu9grqui+M4SZIrCO8Eg86y7JT7LXx5TODSNL3qDhw6eOeOIyBJEuUj6ZY7mRNmAUvQa4Q+EEiHJizLMgzj3AkeMLBte0vsoCULPHRd//NaUK9pmu/EywDCv0M7+CTzmb4EGADS4Lwj+N6gZgAAAABJRU5ErkJggg==) 0 -11px no-repeat; }\n"
             "\t\t\th3.open {background-position: 0 7px; }\n"
             "\t\t\th4.toggle {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAVCAIAAADw0OikAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD8SURBVHjavJHLjkRAGIUbRaxd3oAQ8QouifDSFmysPICNIBZ2EhuJuM6ZMdFR3T3LOYtKqk79/3/qKybLsrZteZ5/3DXPs67rxLbtvu+bprluHMexrqumaZZlMdhM05SmaVVVhBBst20zDMN1XRR822erJEnKsmQYxjRNz/M4jsM5ORsKguD7/r7vqHAc5/Sg3+orDsuyGHGd3OxXsY8/9R92XdfjOH60i6IAODzsvQ0sgApw1I0nAZACVGAAPlEU6WigDaLoEcfxleNN8mEY8Id0c2hZFlmWgyDASlefXhiGqqrS0eApihJFkSRJt0nHj/I877rueNGXAAMAKcaTc/aCM/4AAAAASUVORK5CYII=) 0 -8px no-repeat; }\n"
             "\t\t\th4.open {background-position: 0 6px; }\n"
             "\t\t\ta.toggle-details {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAkAAAAXCAYAAADZTWX7AAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAADiSURBVHjaYvz//z/DrFmzGBkYGLqBeG5aWtp1BjTACFIEAkCFZ4AUNxC7ARU+RlbEhMT+BMQaQLwOqEESlyIYMIEqlMenCAQsgLiakKILQNwF47AgSfyH0leA2B/o+EfYTOID4gdA7IusAK4IGk7ngNgPqOABut3I1uUDFfzA5kB4YOIDTAxEgOGtiAUY2vlA2hCIf2KRZwXie6AQPwzEFUAsgUURSGMQEzAqQHFmB8R30BS8BWJXoPw2sJuAjNug2Afi+1AFH4A4DCh+GMXhQIEboHQExKeAOAbI3weTAwgwAIZTQ9CyDvuYAAAAAElFTkSuQmCC) 0 4px no-repeat; }\n"
             "\t\t\ta.open {background-position: 0 -11px; }\n"
             "\t\t\ttd.small a.toggle-details {background-position: 0 2px; }\n"
             "\t\t\ttd.small a.open {background-position: 0 -13px; }\n"
             "\t\t\t#active-help, .help-box {display: none; }\n"
             "\t\t\t#active-help {position: absolute;width: 350px;padding: 3px;background: #fff;z-index: 10; }\n"
             "\t\t\t#active-help-dynamic {padding: 6px 6px 18px 6px;background: #eeeef5;outline: 1px solid #ddd;font-size: 13px; }\n"
             "\t\t\t#active-help .close {position: absolute;right: 10px;bottom: 4px; }\n"
             "\t\t\t.help-box h3 {margin: 0 0 5px 0;font-size: 16px; }\n"
             "\t\t\t.help-box {border: 1px solid #ccc;background-color: #fff;padding: 10px; }\n"
             "\t\t\ta.help {cursor: help; }\n"
             "\t\t\t.section {position: relative;width: 1150px;padding: 8px;margin-left: auto;margin-right: auto;margin-bottom: -1px;border: 1px solid #ccc;background-color: #fff;-moz-box-shadow: 4px 4px 4px #bbb;-webkit-box-shadow: 4px 4px 4px #bbb;box-shadow: 4px 4px 4px #bbb;text-align: left; }\n"
             "\t\t\t.section-open {margin-top: 25px;margin-bottom: 35px;-moz-border-radius: 15px;-khtml-border-radius: 15px;-webkit-border-radius: 15px;border-radius: 15px; }\n"
             "\t\t\t.grouped-first {-moz-border-radius-topright: 15px;-moz-border-radius-topleft: 15px;-khtml-border-top-right-radius: 15px;-khtml-border-top-left-radius: 15px;-webkit-border-top-right-radius: 15px;-webkit-border-top-left-radius: 15px;border-top-right-radius: 15px;border-top-left-radius: 15px; }\n"
             "\t\t\t.grouped-last {-moz-border-radius-bottomright: 15px;-moz-border-radius-bottomleft: 15px;-khtml-border-bottom-right-radius: 15px;-khtml-border-bottom-left-radius: 15px;-webkit-border-bottom-right-radius: 15px;-webkit-border-bottom-left-radius: 15px;border-bottom-right-radius: 15px;border-bottom-left-radius: 15px; }\n"
             "\t\t\t.section .toggle-content {padding: 0; }\n"
             "\t\t\t.player-section .toggle-content {padding-left: 16px;margin-bottom: 20px; }\n"
             "\t\t\t.subsection {background-color: #ccc;width: 1000px;padding: 8px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;font-size: 12px; }\n"
             "\t\t\t.subsection-small {width: 500px; }\n"
             "\t\t\t.subsection h4 {margin: 0 0 10px 0; }\n"
             "\t\t\t.profile .subsection p {margin: 0; }\n"
             "\t\t\t#raid-summary .toggle-content {padding-bottom: 0px; }\n"
             "\t\t\tul.params {padding: 0;margin: 4px 0 0 6px; }\n"
             "\t\t\tul.params li {float: left;padding: 2px 10px 2px 10px;margin-left: 10px;list-style-type: none;background: #eeeef5;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 11px; }\n"
             "\t\t\t#masthead ul.params {margin-left: 4px; }\n"
             "\t\t\t#masthead ul.params li {margin-left: 0px;margin-right: 10px; }\n"
             "\t\t\t.player h2 {margin: 0; }\n"
             "\t\t\t.player ul.params {position: relative;top: 2px; }\n"
             "\t\t\t#masthead h2 {margin: 10px 0 5px 0; }\n"
             "\t\t\t#notice {border: 1px solid #ddbbbb;background: #ffdddd;font-size: 12px; }\n"
             "\t\t\t#notice h2 {margin-bottom: 10px; }\n"
             "\t\t\t.alert {width: 800px;padding: 10px;margin: 10px 0 10px 0;background-color: #ddd;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
             "\t\t\t.alert p {margin-bottom: 0px; }\n"
             "\t\t\t.section .toggle-content {padding-left: 18px; }\n"
             "\t\t\t.player > .toggle-content {padding-left: 0; }\n"
             "\t\t\t.toc {float: left;padding: 0; }\n"
             "\t\t\t.toc-wide {width: 560px; }\n"
             "\t\t\t.toc-narrow {width: 375px; }\n"
             "\t\t\t.toc li {margin-bottom: 10px;list-style-type: none; }\n"
             "\t\t\t.toc li ul {padding-left: 10px; }\n"
             "\t\t\t.toc li ul li {margin: 0;list-style-type: none;font-size: 13px; }\n"
             "\t\t\t.charts {float: left;width: 541px;margin-top: 10px; }\n"
             "\t\t\t.charts-left {margin-right: 40px; }\n"
             "\t\t\t.charts img {padding: 8px;margin: 0 auto;margin-bottom: 20px;border: 1px solid #ccc;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: inset 1px 1px 4px #ccc;-webkit-box-shadow: inset 1px 1px 4px #ccc;box-shadow: inset 1px 1px 4px #ccc; }\n"
             "\t\t\t.talents div.float {width: auto;margin-right: 50px; }\n"
             "\t\t\ttable.sc {border: 0;background-color: #eee; }\n"
             "\t\t\ttable.sc tr {background-color: #fff; }\n"
             "\t\t\ttable.sc tr.head {background-color: #aaa;color: #fff; }\n"
             "\t\t\ttable.sc tr.odd {background-color: #f3f3f3; }\n"
             "\t\t\ttable.sc th {padding: 2px 4px 4px 4px;text-align: center;background-color: #aaa;color: #fcfcfc; }\n"
             "\t\t\ttable.sc th.small {padding: 2px 2px;font-size: 12px; }\n"
             "\t\t\ttable.sc th a {color: #fff;text-decoration: underline; }\n"
             "\t\t\ttable.sc th a:hover, table.sc th a:active {color: #f1f1ff; }\n"
             "\t\t\ttable.sc td {padding: 2px;text-align: center;font-size: 13px; }\n"
             "\t\t\ttable.sc th.left, table.sc td.left, table.sc tr.left th, table.sc tr.left td {text-align: left; }\n"
             "\t\t\ttable.sc th.right, table.sc td.right, table.sc tr.right th, table.sc tr.right td {text-align: right;padding-right: 4px; }\n"
             "\t\t\ttable.sc th.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
             "\t\t\ttable.sc td.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
             "\t\t\ttable.sc tr.details td {padding: 0 0 15px 15px;text-align: left;background-color: #fff;font-size: 11px; }\n"
             "\t\t\ttable.sc tr.details td ul {padding: 0;margin: 4px 0 8px 0; }\n"
             "\t\t\ttable.sc tr.details td ul li {clear: both;padding: 2px;list-style-type: none; }\n"
             "\t\t\ttable.sc tr.details td ul li span.label {display: block;padding: 2px;float: left;width: 145px;margin-right: 4px;background: #f3f3f3; }\n"
             "\t\t\ttable.sc tr.details td ul li span.tooltip {display: block;float: left;width: 190px; }\n"
             "\t\t\ttable.sc tr.details td ul li span.tooltip-wider {display: block;float: left;width: 350px; }\n"
             "\t\t\ttable.sc tr.details td div.float {width: 350px; }\n"
             "\t\t\ttable.sc tr.details td div.float h5 {margin-top: 4px; }\n"
             "\t\t\ttable.sc tr.details td div.float ul {margin: 0 0 12px 0; }\n"
             "\t\t\ttable.sc td.filler {background-color: #ccc; }\n"
             "\t\t\ttable.sc .dynamic-buffs tr.details td ul li span.label {width: 120px; }\n"
             "\t\t</style>\n" );
  }
  else
  {
    fprintf( file,
             "\t\t<style type=\"text/css\" media=\"all\">\n"
             "\t\t\t* {border: none;margin: 0;padding: 0; }\n"
             "\t\t\tbody {padding: 5px 25px 25px 25px;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 14px;background: #261307;color: #FFF;text-align: center; }\n"
             "\t\t\tp {margin: 1em 0 1em 0; }\n"
             "\t\t\th1, h2, h3, h4, h5, h6 {width: auto;color: #FDD017;margin-top: 1em;margin-bottom: 0.5em; }\n"
             "\t\t\th1, h2 {margin: 0;padding: 2px 2px 0 2px; }\n"
             "\t\t\th1 {font-size: 28px;text-shadow: 0 0 3px #FDD017; }\n"
             "\t\t\th2 {font-size: 18px; }\n"
             "\t\t\th3 {margin: 0 0 4px 0;font-size: 16px; }\n"
             "\t\t\th4 {font-size: 12px; }\n"
             "\t\t\th5 {font-size: 10px; }\n"
             "\t\t\ta {color: #FDD017;text-decoration: none; }\n"
             "\t\t\ta:hover, a:active {text-shadow: 0 0 1px #FDD017; }\n"
             "\t\t\tul, ol {padding-left: 20px; }\n"
             "\t\t\tul.float, ol.float {padding: 0;margin: 0; }\n"
             "\t\t\tul.float li, ol.float li {display: inline;float: left;padding-right: 6px;margin-right: 6px;list-style-type: none;border-right: 2px solid #333; }\n"
             "\t\t\t.clear {clear: both; }\n"
             "\t\t\t.hide, .charts span {display: none; }\n"
             "\t\t\t.center {text-align: center; }\n"
             "\t\t\t.float {float: left; }\n"
             "\t\t\t.mt {margin-top: 20px; }\n"
             "\t\t\t.mb {margin-bottom: 20px; }\n"
             "\t\t\t.force-wrap {word-wrap: break-word; }\n"
             "\t\t\t.mono {font-family: \"Lucida Console\", Monaco, monospace;font-size: 12px; }\n"
             "\t\t\t.toggle {cursor: pointer; }\n"
             "\t\t\th2.toggle {padding-left: 18px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAeCAIAAACT/LgdAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAFaSURBVHjaYoz24a9N51aVZ2PADT5//VPS+5WRk51RVZ55STu/tjILVnV//jLEVn1cv/cHMzsb45OX/+48/muizSoiyISm7vvP/yn1n1bs+AE0kYGbkxEiaqDOcn+HyN8L4nD09aRYhCcHRBakDK4UCKwNWM+sEIao+34aoQ6LUiCwMWR9sEMETR12pUBgqs0a5MKOJohdKVYAVMbEQDQYVUq6UhlxZmACIBwNQNJCj/XVQVFjLVbCsfXrN4MwP9O6fn4jTVai3Ap0xtp+fhMcZqN7S06CeU0fPzBxERUCshLM6ycKmOmwEhVYkiJMa/oE0HyJM1zffvj38u0/wkq3H/kZU/nxycu/yIJY8v65678LOj8DszsBt+4+/iuo8COmOnSlh87+Ku///PjFXwIRe2qZkKggE56IZebnZfn56x8nO9P5m/+u3vkNLHBYWdARExMjNxczQIABACK8cxwggQ+oAAAAAElFTkSuQmCC) 0 -10px no-repeat; }\n"
             "\t\t\th2.toggle:hover {text-shadow: 0 0 2px #FDD017; }\n"
             "\t\t\th2.open {margin-bottom: 10px;background-position: 0 9px; }\n"
             "\t\t\t#home-toc h2.open {margin-top: 20px; }\n"
             "\t\t\th3.toggle {padding-left: 16px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAaCAYAAACD+r1hAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD/SURBVHjaYvx7QdyTgYGhE4iVgfg3A3bACsRvgDic8f///wz/Lkq4ADkrgVgIh4bvIMVM+i82M4F4QMYeIBUAxE+wKP4IxCEgxWC1MFGgwGEglQnEj5EUfwbiaKDcNpgA2EnIAOg8VyC1Cog5gDgMZjJODVBNID9xABVvQZdjweHJO9CQwQBYbcAHmBhIBMNBAwta+MtgSx7A+MBpgw6pTloKxBGkaOAB4vlAHEyshu/QRLcQlyZ0DYxQmhuIFwNxICnBygnEy4DYg5R4AOW2D8RqACXxMCA+QYyG20CcAcSHCGUgTmhxEgPEp4gJpetQZ5wiNh7KgXg/vlAACDAAkUxCv8kbXs4AAAAASUVORK5CYII=) 0 -11px no-repeat; }\n"
             "\t\t\th3.toggle:hover {text-shadow: 0 0 2px #CDB007; }\n"
             "\t\t\th3.open {background-position: 0 7px; }\n"
             "\t\t\th4.toggle {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAVCAIAAADw0OikAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD8SURBVHjavJHLjkRAGIUbRaxd3oAQ8QouifDSFmysPICNIBZ2EhuJuM6ZMdFR3T3LOYtKqk79/3/qKybLsrZteZ5/3DXPs67rxLbtvu+bprluHMexrqumaZZlMdhM05SmaVVVhBBst20zDMN1XRR822erJEnKsmQYxjRNz/M4jsM5ORsKguD7/r7vqHAc5/Sg3+orDsuyGHGd3OxXsY8/9R92XdfjOH60i6IAODzsvQ0sgApw1I0nAZACVGAAPlEU6WigDaLoEcfxleNN8mEY8Id0c2hZFlmWgyDASlefXhiGqqrS0eApihJFkSRJt0nHj/I877rueNGXAAMAKcaTc/aCM/4AAAAASUVORK5CYII=) 0 -8px no-repeat; }\n"
             "\t\t\th4.open {background-position: 0 6px; }\n"
             "\t\t\ta.toggle-details {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAWCAYAAAD5Jg1dAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAADpSURBVHjaYvx7QdyLgYGhH4ilgfgPAypgAuIvQBzD+P//f4Z/FyXCgJzZQMyHpvAvEMcx6b9YBlYIAkDFAUBqKRBzQRX9AuJEkCIwD6QQhoHOCADiX0D8F4hjkeXgJsIA0OQYIMUGNGkesjgLAyY4AsTM6IIYJuICTAxEggFUyIIULIpA6jkQ/0AxSf8FhoneQKxJjNVxQLwFiGUJKfwOxFJAvBmakgh6Rh+INwCxBDG+NoEq1iEmeK4A8Rt8iQIEpgJxPjThYpjIhKSoFFkRukJQQK8D4gpoCDDgSo+Tgfg0NDNhAIAAAwD5YVPrQE/ZlwAAAABJRU5ErkJggg==) 0 -9px no-repeat; }\n"
             "\t\t\ta.open {background-position: 0 6px; }\n"
             "\t\t\ttd.small a.toggle-details {background-position: 0 -10px; }\n"
             "\t\t\ttd.small a.open {background-position: 0 5px; }\n"
             "\t\t\t#active-help, .help-box {display: none;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px; }\n"
             "\t\t\t#active-help {position: absolute;width: auto;padding: 3px;background: transparent;z-index: 10; }\n"
             "\t\t\t#active-help-dynamic {max-width: 400px;padding: 8px 8px 20px 8px;background: #333;font-size: 13px;text-align: left;border: 1px solid #222;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: 4px 4px 10px #000;-webkit-box-shadow: 4px 4px 10px #000;box-shadow: 4px 4px 10px #000; }\n"
             "\t\t\t#active-help .close {display: block;height: 14px;width: 14px;position: absolute;right: 12px;bottom: 7px;background: #000 url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAYAAAAfSC3RAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAE8SURBVHjafNI/KEVhGMfxc4/j33BZjK4MbkmxnEFiQFcZlMEgZTAZDbIYLEaRUMpCuaU7yCCrJINsJFkUNolSBnKJ71O/V69zb576LOe8v/M+73ueVBzH38HfesQ5bhGiFR2o9xdFidAm1nCFop7VoAvTGHILQy9kCw+0W9F7/o4jHPs7uOAyZrCL0aC05rCgd/uu1Rus4g6VKKAa2wrNKziCPTyhx4InClkt4RNbardFoWG3E3WKCwteJ9pawSt28IEcDr33b7gPy9ysVRZf2rWpzPso0j/yax2T6EazzlynTgL9z2ykBe24xAYm0I8zqdJF2cUtog9tFsxgFs8YR68uwFVeLec1DDYEaXe+MZ1pIBFyZe3WarJKRq5CV59Wiy9IoQGDmPpvVq3/Tg34gz5mR2nUUPzWjwADAFypQitBus+8AAAAAElFTkSuQmCC) no-repeat; }\n"
             "\t\t\t#active-help .close:hover {background-color: #1d1d1d; }\n"
             "\t\t\t.help-box h3 {margin: 0 0 12px 0;font-size: 14px;color: #C68E17; }\n"
             "\t\t\t.help-box p {margin: 0 0 10px 0; }\n"
             "\t\t\t.help-box {background-color: #000;padding: 10px; }\n"
             "\t\t\ta.help {color: #C68E17;cursor: help; }\n"
             "\t\t\ta.help:hover {text-shadow: 0 0 1px #C68E17; }\n"
             "\t\t\t.section {position: relative;width: 1150px;padding: 8px;margin-left: auto;margin-right: auto;margin-bottom: -1px;border: 0;-moz-box-shadow: 0px 0px 8px #FDD017;-webkit-box-shadow: 0px 0px 8px #FDD017;box-shadow: 0px 0px 8px #FDD017;color: #fff;background-color: #000;text-align: left; }\n"
             "\t\t\t.section-open {margin-top: 25px;margin-bottom: 35px;-moz-border-radius: 15px;-khtml-border-radius: 15px;-webkit-border-radius: 15px;border-radius: 15px; }\n"
             "\t\t\t.grouped-first {-moz-border-radius-topright: 15px;-moz-border-radius-topleft: 15px;-khtml-border-top-right-radius: 15px;-khtml-border-top-left-radius: 15px;-webkit-border-top-right-radius: 15px;-webkit-border-top-left-radius: 15px;border-top-right-radius: 15px;border-top-left-radius: 15px; }\n"
             "\t\t\t.grouped-last {-moz-border-radius-bottomright: 15px;-moz-border-radius-bottomleft: 15px;-khtml-border-bottom-right-radius: 15px;-khtml-border-bottom-left-radius: 15px;-webkit-border-bottom-right-radius: 15px;-webkit-border-bottom-left-radius: 15px;border-bottom-right-radius: 15px;border-bottom-left-radius: 15px; }\n"
             "\t\t\t.section .toggle-content {padding: 0; }\n"
             "\t\t\t.player-section .toggle-content {padding-left: 16px; }\n"
             "\t\t\t#home-toc .toggle-content {margin-bottom: 20px; }\n"
             "\t\t\t.subsection {background-color: #333;width: 1000px;padding: 8px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;font-size: 12px; }\n"
             "\t\t\t.subsection-small {width: 500px; }\n"
             "\t\t\t.subsection h4 {margin: 0 0 10px 0;color: #fff; }\n"
             "\t\t\t.profile .subsection p {margin: 0; }\n"
             "\t\t\t#raid-summary .toggle-content {padding-bottom: 0px; }\n"
             "\t\t\tul.params {padding: 0;margin: 4px 0 0 6px; }\n"
             "\t\t\tul.params li {float: left;padding: 2px 10px 2px 10px;margin-left: 10px;list-style-type: none;background: #2f2f2f;color: #ddd;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 11px;-moz-border-radius: 8px;-khtml-border-radius: 8px;-webkit-border-radius: 8px;border-bottom-radius: 8px; }\n"
             "\t\t\tul.params li.linked:hover {background: #393939; }\n"
             "\t\t\tul.params li a {color: #ddd; }\n"
             "\t\t\tul.params li a:hover {text-shadow: none; }\n"
             "\t\t\t.player h2 {margin: 0; }\n"
             "\t\t\t.player ul.params {position: relative;top: 2px; }\n"
             "\t\t\t#masthead {height: auto;padding-bottom: 15px;border: 0;-moz-border-radius: 15px;-khtml-border-radius: 15px;-webkit-border-radius: 15px;border-radius: 15px;-moz-box-shadow: 0px 0px 8px #FDD017;-webkit-box-shadow: 0px 0px 8px #FDD017;box-shadow: 0px 0px 8px #FDD017;text-align: left;color: #FDD017;background: #000 url(data:image/jpeg;base64,/9j/4AAQSkZJRgABAgAAZABkAAD/7AARRHVja3kAAQAEAAAAHgAA/+4ADkFkb2JlAGTAAAAAAf/bAIQAEAsLCwwLEAwMEBcPDQ8XGxQQEBQbHxcXFxcXHx4XGhoaGhceHiMlJyUjHi8vMzMvL0BAQEBAQEBAQEBAQEBAQAERDw8RExEVEhIVFBEUERQaFBYWFBomGhocGhomMCMeHh4eIzArLicnJy4rNTUwMDU1QEA/QEBAQEBAQEBAQEBA/8AAEQgAlgFMAwEiAAIRAQMRAf/EAKUAAAEFAQEAAAAAAAAAAAAAAAUAAQIDBAYHAQADAQEAAAAAAAAAAAAAAAABAgMABBAAAgEDAgMFBQQIBAYCAwAAAQIDABEEIRIxEwVBUWFxIoEyQhQGkaHRFbHBUmKCIzNT4XKSJPGiQ7NUFrIl0nM1EQABAwMDAgUCBAUFAQAAAAABABECIRIDMUFRYaFxgSJiE5EEscEyQvDRUqIj4XKCkjNj/9oADAMBAAIRAxEAPwDgCTc6njS3HvNJuJ86assn3HvNLce801Kssn3HvNLce801Kssn3HvNLce801Kssn3HvNOqyvfaGa3G1zUaP9D6tMMRul85oQ+440y+kxzHVVJHFX4a8KTLKUY3Rjc2tWpymiASxLICd4NjcGl6vGut6dNl5UZTIzpsdiNCwV0ewvZWtuVvA8eyqJcuOWJjDl5DWt/NO5kuvvEL4twudKkM8jIxtFNWJP5J/iDO65n1+NK7DiTXRfOzrGN88kgY6OVYAE6dja1nylxsmImV90sjArMFN/2bcbG9OMh3j9EDj4KC7j3mluPeaJ5GDgmBHxnk5hYBt4AQjhcN33q+DB6RYCcyhiDt4eq3EnUDSicgZ2l9EPjPRBbnvNON5IAuSdABxNFn6b08SJEjzbmvYlRduFgOAt40U6fD0/pQ5zFZcpPfdhoo7k7qWWYAUBJOgRjiJOwA1KAr02ZUaTIPLWP+og1kXuuuluNPi4sTEs5HL23LyHaB5C4uaNSRdU6isjYGMXgcm6gAH1cSN1QwekZadUTEyMc3hUSzgAM3KtxC8dS1MLyPUbeg2WIiCGDqtk6RjoqwbJm+IsLg+01BU6bNtV4owx97YdpF+FHGgxTJIqooCelQdOGpOvnah2biYdyzICw7q3xe6To3+2KGTdIhKloJSpBsA40J9mtDp8fIx32SqVPYeIPkaKZSvjy/7e7REgrGfVa9jYHvpKzuSdSbgkX1U9hBHb4ilulHX1BEwjJ2oQg9n7j99IiQcQfvouc3NilvJmS8s313MfYdpFWPlZAFpM6bb2xsHsQdBrv76PySpQV4J/kls6oHc99K576IzpGJC+7ez+/uTawNr21JJqRTDkC7kIkOhvck6aWta2lNeGdihZ1Qy7d5perhrRmF+QEjx3ERW4IRG3sx9V2cG/DhVkErJlF2yJnkbWYoGSVl7Buux8qU5NWG38bLfH1QL1eNSVJn91WbyBNddiTpKkjxdTnjMICvGymR2c8NisNS3YKog6rmYrzZc80rJCAIMd2uZZm93cY9NoA3G3lUz9xOoEATHZyC5/4pviH9XZcwyyp74Zb8L3FRue81fnZ+Z1CdsjMlaWRu1joB3KOAHlWeuiLsLmB3bRST7j3mluPeaalRWT7j3mluPeaalWWT7j3mluPeaalWWT7j3mnudnHtqNP8HtrLJNxPnTU7cT501ZZKlSpVlkqVSVGc2UVfL0/IihErqbHs8O+gSBQnVEAmrLNSpUqKCQBJsNSeAoli4gjgeWVtkhA5aC2+97rtHfcewVOLBiihjyVfmO6syfCt0946gcCbCozjkxOrm7odvG1yBY2t2a8KmZCQYHf8E4iQXISk6g6xxRuNxjA2xiwjBsQH2rxY8dx1qh+oZji3MIW5OwaLcm5svCs2pNzqTxJqQFEQiNgi55UjPO4szEjjY0hLJ4aeA/CkFqQWtRMH5KjzJPD7B+FXRLkSDQAqP3R+FMkRY0b6d05mj32so4mknMBMIy5Kqw8d4Ieaw3TS6JwGxO+sL5iNNtU3hTQX+L94/qoh1iYxROq3V5Dyl/yrxIoABY68KGEXPM+SGaVrQHmurwOuyY4A+FRfaKj07qDY3Us7LMpmXKj95z6rhtF/hFCMnDysRYmyRy2nCssYN22toC1vd8jRL6gwMbpmVFHilgkqFtjHdtIO3Q+NUvhdGO83by1UvUxPH5qjO6q2Q+gBHj2eRrK2XMEB46aE6kfbWV2UH9Nbuj9JyeqyEBuXjRaSzHW1/hUdrGjOUYxMpFgNUA5LCpKx/NSpJdCVZjY95v2VHIzpmyOagVBH6EWw0UaWrp87of0xiKI8vJkhmK3QmQM9gOOwLaua6p0qXAaOQOJ8ScbsbJT3ZF/Uw7RUseTHkLgHgXRZ/BUlfAWvu9Cr1WPLhDINGvvF7lSKxusyMQbXGnAU/TptkxiY+iTTjb1Dh+FEhCJt/p1SzeNjpWJMJEajUJx6og6HQoQZJO23+kfhTcx/D7B+FapoNptas7LanBBQMTyVHmyXvfUdthUly8lTdZCDe9xprTFaiVo04S+rkrUvUpif515G3b+ZciQNprv96+nfWuF3zcsF3sxAEL+ld729JcjTeeAJ4nQ0JIq3GKl+XISIyD228aWUAxYMW4WBL1WvrEEWmRGqRSAiOeBdNr2vuUfsm3D4Tp3ULonNGcuRZch7Lb+Y62JAWys20kefjWXPxFw8lscScwobMbEajz7+NHGWAgS5b+KpZipOyzUqVWQwvM4RQTfjYXqiRV0qvyMKfHNpFt+HfVFAEGoqiQRqlSpUqKCVP8Htpqf4PbWWSbifOmp24nzpqyyVTijeWRY095u/h5moUQ6UYm50G5IsiZbRTymyDW7Ke7cO2lnK2JPCMQ5ZXwy5nToY5A+1gfdVYyuw66st218a09SkklieaXLLbhHy4QFAUyXa3YdoH7IqpI8fmZphI+SjR0jBBszMt91+4MBa9Y5p0SWBpLyOgJlCnZb4UVWAvooHGoAXSBA9QqfTWtWViWiz08fJXwso4chlJJbekZIv3eg2FXcxWOvyrd5CR3t2f9O1C/mCAFQWXaAwJNiRpenGTIpJUBd3G1x+unON60S3Ii0xM8c2TKpghI2QwhbsCwJFgFUXtrWXPTbHEysGSQliR724gOd3lvtWYTyqGCnaHFmt29lWo6y4pgYBZEbmRN+1cbWT9YrCFpB24CzvRZwKmopKuoq1V14U5KaMUlQ1fFDuIFSiTwrdioOYvpFRnNl0RxqWH00yaqLmjuRC/T+j8x9DI6r+k/qq3pzQXA27e/urR9Vwk9AV4+CTRk+R3L+upYyZkk7BJMsQG3XB9TmLyxqTcKtx7T/hRL6Z6QuZP89lC2JjG634PINfsXiaE9RUrMpOt14+RrZhfUnVMHGTEhMZiS/L3oGK3Nzrp299XlGZwiOJgTRz3UCR8hM1ozYOodW6q2TjYkzY5kBQlGttuLuS1h6uNEvqvFjlyHy5cuKFYYgsMBO6WV7lrbB7o140Hi671rqGfjwT5b7JJUVkT0LYsL3CWrN1zLGZ1nKnRbKz7VHbZQEH6KUY5/JAloiESKV1bcoGUbTqbisbPxNd9EYOidEjYruWFFdwNN8klidfbXAzwTQHbMhjYgGzCxF+F/Ouxz5D1j6W52L6njEbTRjiGhFnW3l6hS/dxuOIH9ByC78k2EteR+q2i5DNy5czKlypjeSVizeHcB4AaV0vQYF6h9MZmJNqqSO0JPwMEEgI9tctHFJNIsUSmSRzZEXUknurr8rb9PfTXyZYfOZQZSB+1J/UI8EX0+dN9waQhCkjONrbAalLj1lI6CJfzXGAlWVxxBBFdN0tRLmbSdwljfS/cNw/RXMnurrvp7Hkfqce7gkT6fwW/XT5v2+abDpLyWLLgUX87ChjxkMa6zMwRtZwBe+g7a5zIXbLYga1HHPZdBiKLFtA41EqL1pNr2IqthckWqwKUxWcgVEXDrt0NxY1aw7qUHLEyvL/AE4zuI/a26hR5070UZBb5o8dFbG5gSZzuDrZo1NmUxk8ddoPDSpmeVwGnOPJMNCzqj3HexKE3ob8zNzXlWyNISSF4am9qZMiWPRbAcNL/jU/j5r4rOPBEjLdbD5UkcBsjIAv/wDrvVMAR8xIxMIuYxAdNgAZvdv7o21iGS622qFI4EXB/TSeeMxFFQhjs13Ei633G3jejYzgboEhGcrPzFjgxPmDKJVDPIFBYqdNvqHZb/G1Cs7FyFd5nsy3FiNl9p0ViI7jsohjqmTJMsb6PCRjMR6k3OhYWUAaFmGlSw4oWxxO20vJviyYGO15XYgR7Bbh23HDWkiRDQbsWGr1TSF2p8K8IDSqyeIxSsh4Dge8dlV10qCVP8Htpqf4PbWWSbifOmp24nzrX0yJJJ2MqhoY0ZpL+VgPaaBLAnhEByBylHiRxxrLOS7uN8cEfEr+07fCK2SfmAVWjZI4+WrkRqAVVvcS5G49lVSOYYpZWcmScbV2D0gg2ZSfLhW7Lnx2xsXgFnXmlAOJiURIh77EGoSJcUuckdOVYRFQCxCzkMMJ1kkZ5pR/MDX3Ki+pzbuuFUeRoXI5ka9gANFA7hUpJXdyze+bhiOFu6w00qAqkY2v1qlkQWA2SAqQWnAvVgU0SUYxVew1IJVoWrFQX1FKZKggoQwl3VQLkmrOUUcqRqDrRXpj4aTIHi82vVuZPgyysI8ex4br2ualLIqCNWZDYk4UQxEBcX7KqVEvov31qgsrDT76hObqwBZFcFACC19vf50TzkOb0zJwQt2aO6H99PUv6KD404W3np4UXhmMcis9y3ED8aTHO0qGSK8/y4HlhV9vu6+Nu2qDiyIFZho3C/H7K6Lr+GuHlmVUvi5d3i7lY6vGf0jwoC6ywZCszl4nIAb9m3wmu7HI6eY6qM4xPqrx4LOrSYuRHOh2vEyup7fSb0+LkpD1CLKmBdElWSQDibG5tenynQuWPE9g4VkZ730qrPqommiLfUXU+n9RljmwllVjuM3Ntqxtt22J7Ky9L6rm9MmM2I+0nR421Rx+8tYaXCgIRtsPqGnqqs5d9D0XUp9YxQgvD0yGLIbjIh2g3/yqG++gPUOo5XUck5OU25yLKBoqqOCqOwVlubU1CGHHAmUY1O+p7omciGJotPT4PmMuNCLqDubyXWu++nscKcjJZduxBGPF39Z/5QK5no+A0UV2QtNMQuwH1a+6ntNdnHGMTFTEU7nW7zMOBdveP6hXNmyC4naNB4rohC2AG8qnwWXLkRNxHbf/AIVzGfEHe66d1dFlsJF46LwBoPMqk2tcVzQnV10RjRBW00YWI7arJW9FmhiPFPvrM+PGG0WuqOQFCUJIey3JA4VHl1teNQx9Pb31Aqp7KoJKdnKybKYpWsRgnhTGOjcl+NYmSoEVrZKqZKcSU5QWjpsw+YVpNFVSkpH9p/QzH/Le/sq6Rc/mlIJmcKNyAm6E/HsLXFrHd5UNIIvbt0Na4JVMDiQlFjIMZU2YC+qbuPCllGtw8GZAVppvqm6ik5MfMCu7IHEkY2+kj3So00rBR3NbCxcw445jkLYEa7dwLKAO3iL1VNHzYJ0YI00SAoVW3u2Mgt99aExaKULd0JY3Jrpqg9P8Htpqf4PbVVJI8TW3Cli5E8FtryqArkgC49VjeqUhxmsXn2k8VCMSK2YD4sErjnxqput5Ime4P7tJMuDQ0roU8aF6IgXmjXkB4dshiSOJiDEouLvs7PVp3nUmhOTIoifHYkywylVc6Eod24W/za0QOFgvjJyskZDFr/LAiE6qfUC+gItwNB5jIZW5pJcGzFjc6aamp4wHP10aqeZP5KAqS1GpqKqUoViirVF6igtWiGJ5pVihQvI/uoNSanIq0QkEqxVFEl6HkIoGRNBAx12vJ6vuFRyulz4kQmbbJCSF5kbbhc8K5/mhIsJAurALLGLMvnVwT1Gq4ygYaHj31cGW50++hJUirUXvrRCl3AFZ1kQa2++jHScvAVwrx2ktox1FRMSTq3VUMgIkgOsMRUPZzYX1PGjUc0B9QlDXOnG4WheXkYjzNyo9ovxp4JYrWP2UrsUk4XAGo6Ivlx4ebinCyG3wPbYw0ZCPddfEVxnUen5PTZjjZQ3RyaxzD3JF7CO4+HZXRDLuAmqgHQ9tXSywyRcnLjE8Dj1K2vDtB7DVoZmpLT8FzyxEafTlef5EMq6++o4EfrrPXYz/AE4rkv0yYOvH5eY2YeCvwPtoTl9LkhJ+bxJIm7ypsf4l0rsjmBH9XgoSxf8AHoUEpURGDiMTaQiw7SOP2Vrxem4rOtojMSbbRuY/YtE5YjlAYZHhBoYJp3EcKF2PYBRvp/R+Qwlns84ttj4gE/pNHsXoeXtsIxjQ6WaT0cP3B6jRJYMXCYsh5s1tZ2tfusg+H9NQy5yx/aO6rDFGJ/rl2UOnYSYX+5nAOS39KLjyweN/3j91UT5j3ZQbAm5FaEzcdGDzDaLkDtO6sWfLjxeubbjh7lRIx3kf5EDGuKcjJm8BHdWiACTL6rPJKWrO41BNWRNHkq7YrpOIwS4RrMoHaVcKard09OnZ30REiirExOlVW3dUBFvYKOJOlWcyIdh+2teBlYazrvi1/aveniC/CMiw0dCsiHbKynQg61QY7Ub6lkYbzMEi1vq16GuYuxT9tWiTolFakLOq6ioFa0qY9wsD9tUMUHYftpwiWVLKKoYXNXsy91VsacKMmWd1poCiTxmTWIMDIvYVBuRU2IqlqoKqEgi2DPIP93vRVklaP1D1x2jNmjPHRePkKsm/kSRZOQySEwsrFGDPISdo1v6vTwP21iwMcZGwzyGGKN1C5BcBIhcFjy/eb2UTgm6dgCfl50UkkwAMkkDyMmt/Sy6VGYaRYOdGA28QtE0Y0GtVzb23aC3hTfB7a2S4+BqyZW4k8eW4H31Ryod23nDZx37W+y1q6LgztL/qVG0vt9QqjxNGuhPHHzMiWFZYkUiZ5V3qAbAADtPhegp4nzrXjZbcg4bGyOysDfhr6h7aGSN0W518EcZAlVEooceMczmR2QMqyNEZVeNx6LR6+rQjXhQeZWSZ1ZdjAm6kbbfw9lHYRC+aZE3Y4l3SKSLqkinchW3H1UN6jjSLMTy3WTUTbzdmkA3SP5VLHL1EE6jdUnGjrEKsWqwampqxSxVyUT6Dn4+D1MyZR2RyRmNZDwUkg6+dClaroo2nWUKNxiXey2v6L2Y+y9SyQE4yjLSQaisDoxqCuj6j9OrlyPm4ciTc4lyrG+p/ZfUWoO0OZ0sSwzRvHDOBdD7u5TcMp4VmxsnKw234c7RHiVBup81OldHjZ7dX6DmHMQK8Aa7gWUso3Kw7j2VzH5cQiJmOXHdGL6SHFE1CdLZahDoo1hxYepuyvjO+0KL7iRe4sR4VLJhkSCLOFvl8q5jte47QG0rE8x/9egj7BkTf/Ff/AMqKrJzY4uiuf6uFFJjX7J1Be38QppXBjr65P/sBbssMhcPuB9Vlw0ky5mihI9CGR2a+1VHfYGpQSXxxmRsDHzDEOIO4Dd2jhar+kk4RixCLZObHLPkA8VjVGESfrrBit/8AQqe/Mb/t0rOTw4A6irnsmGUkhW/NL8s+UTZEk5bDXdutu/RWuQvCsLllInQSJtv7p4XuBQjd/wDTzeOSf+3RDqDEY/TbduIlGWMOByZD6IwyyJqaK8ZYsTft1NWrlyPCkp/pOWVH7Dt96g93lCxxmxmbYrdgt7x/hFEenTR5oyelozaKJMJWXbt5Y27RqfeHGknjAD8a9BynOUXAM/JWnH6nhoAWkVS5MabiRc8L+lTpfvonkdbOFP8ALmRVlK7ljYkki3Z6dtzbSuOzCq45CixQg68bg0Q63MsmZITxCJY/wKf10RjaUWJAN3Zm/FLIEkgmJIAPRHJOsKscOXLHG8eSN0fpBNvavjU5/qFMWdIXYQMben3dt+G7aLLQDCmDv0CGQ+gbyQeFw7BfvFZesvumy1k94SP9t6MRIzsJO5/uIH4KYAMSaBgunzeqmXNTBBKzyE7N1wL23dl6xT9STE5iZasGuFYj4SKy5xJyOkbjaf5Ry57f6bbT+mqg79d6Syr/AP08NQJF7ZoxwPnSiINs5fpkwkeCSQ/ghczxHki/ToRJn5M8h3LjMI4hxCsV3OdQNbECheRkCaZ5jq7nie7sHsor0eVWyupY17OswYjts6Kv6RXOM+x2ja4ZCVI8QbUmOD5Mj/tEAPAh1SBDDq/4qSsI+oQSQsIpXOy5vtbf6drW771plCRZQwpJkSZWEfq37dx/e2WrAzhsjFA/vR//ACFHOpJgHIny1hfKmxZv58ZfasbaFXKgXK1WdDEEEvEs3L01QciUrSBohmRvhleGQWeMlWHiKijSckZIB5W/l7+zeBe1U5U8mVOzj1T5D6AcNzmt3TZceeabo3MJiljC491sBLF6t4N/iNzTEWxcjSsvDdGWQggKrHBzZp0R1VoUMr77gbF4nQHhVbGEo7R5EcrIN3LXfuI8NyCrOjAx5+esqncmLKHTgbg6i9Ynlx5EjMcRhdC267b73tb1WFMB6yKsLeOEvyS2I1OyIYmCcqdoIMiF5oxudAW0HA67LaVjlRAheOeOYBtrBN11PZcMq6aVb0SbldUZ10vjzf8AKu79VDtwGo0uNaMYyvIJoBEjz1W+Q86FlJmqBeoM/jUC1WEVOU1JmqsmmLVKKJpnCroCQCx4LuNrmm0UzJ1vwI0eBo5SkYmNlLw7i4BG7ZKNVI+ytmDy4sfJgiSOXKnQtHEVu8cQuQA5JG4g7rWqCwvB02aX1RxRaJGx3Bp7ct5EPdbTzqiXYsMLYynlJvZpSR6t4W/u91rVH9bsaGXk4R0Y9EMyBaQ2FkJO0dwvVfw+2rciRW2qpvtuSfE1V8Htro2Ud0m4nzpqc8T501FBacfPngPvErwI++iIyossF8mRo5pU2cxQDp2q3nQWtWAqySGN5OUrD3hrrxGlJOIIdqjfdUhIux0OysyMdmkIALzsWdioshW1/T9hrLqLXFr6iiy47q6wxy75pGCoouAd2n/GrposWPNULi/NR422FYmLJv2C7khRfU3NJ8jFmej/AMOnOM6hBgsnL5u1uXfbvt6b916vwM+bAy0y4AGdQQVbgytoQaOdU62mfir03DgWDHlFxCAAFK66nw76CJ02dkVlZNxvcF1WwFrG5PxUIyvgfliIXUtJenVYgghi+9OUUPWeiynmTdKHNOpCPZCfLT9FU5/XZMuAYkMKYmIOMSfFbhc6VgHT84gkREgcSCp/XS+QzeyI93FeP20gxYQQXcx0umZN9Smvl/ASM4OHHjFSdkryNrYEOFFv+Wr8vqT5ORBlJGsE2OEVChJFo/d0NU/l+fcDktc6DUfjUJsLMgF5YWUfb+iqNAnUE13/AKtUr+K2J1eZepSdSeNZZpNwsxIVQw22FvDSqoM8wwyYphWTFkfmrGWIZHAtdXHhVHyebqORJcC5G03AqPIyCu7ltt7yLfprCENm0A140WuOtVfNlCSFceOMRQqxcoCSWZtCzM3gK2P1pZY4o5cGFxAgjjJeS4UdmhFCzDkAXMbAWvw7KhdhxBFY4oluj7ndYTZFj1RWcn5SNV5XJjVGYCNTqxX95u0ms8GY+LmRZkQBeEkgE2BuLWNqyIJn0RGY68ATwq9endRkAKY7kHhw/GhZAAgsAQxc7eaa+lHO6nn5nzskkuwRNKbsqkldx4kX76WRmNkSNI1gWAFhw9Khf1VH8p6ra/yz2Pl+NNkdPzMYKXS+6wO3Xa7DdsPiBxrD46ASiW0q6N51Y6MlJNvgxkW6PjKQG/eLs4I+2r5epHIcS5cEc0wtue7KJCvAuq6E/ZQ7dUo1klYLGpZmIUAd7cB7aYwG/WumuqS4bLd+ZzNnfPTqJ5LFQpJRQCuywC9gB0qrGzJcPLTLxvSyfCTcMp4q3CnPS88hTHGZQwButtD2qQeBBqP5X1Mmwx3J9n40v+JiHg1tpD0bhkXPB1daE6xkQ9VfqkagNKSZoQfSynitEOoQw9SVup9LPMLerKxv+qjdrbe0d9vOhA6T1VuGM5+z8anF0rrUcglhgkjkXg6kKR99JKOJxKM4RlEW6hjHgoiUhsa10UEn2TRylRII2DhSbAlTcXtWk9WnXqZ6nEipJJfnRXJjkBFiDfsppundbyDvlxGaX4pFCgt/mCmxPjVH5V1X/wAZ/u/Gi+I6yhpafVsVnJq0udFbHnxR5LTjFX1AhU3ttj3CxKdvbp3VRHkGHJiyIxrC4dQT3dhNP+V9T/8AHb7vxpvyzqf/AI7fd+NH/FX1RqGPqWc8S+i3HrpOXPmHEQSZMZils7AFToSPE0OMg7OHYKkem9RAuYWsfEfjUT07P/snXxH40YjHHQx2H6n0QuPB+iUGS0E3OUAko8dj3SKUJ++qi9WDp+ceEJPkR+NOnTctrkhUA47nQH2AtrTPDW6P1SvLg1VC73YIgLOxsqgXJPgKi25WKsCrDQg6EUb6Qg6XmPmF1flaRMpufVput2Xrb17Nw+oBAmCrTaczKBKEbvdHo0v50hzEZBEQugR+sHQ+BTfGTF3rwuZSJ2u20lEsX8FJ/XRKH5WBS87MMc7ykC6PIj2Oxz7BUpIy+Pj5MZMan/bZHG2+L3Dccbp+ioZfTGix/mJ5Td7cmMgAtu/aHZprRMoyYE21t68ICJAJZ/wRHNzOXFBBKI9qFJHxY/gRVLbW7AAbCgrSGOGZ22rLJZRGBYKtuwd9JpOUojuqqLbtlyzkfE16xu5diTRx4xEMhkm/io0/we2mp/g9tWUUm4nzpqduJ86asslS4UqVZZdHgdSE0CF9M2FlOPLYHc3ujdf7G+2rnzJ48uTKyo2SVSpaVV3bDt2n08GUjh3VzsaMln7eNqO9O61mbTFE45m2wEgBI/eB4Nbxrky4gHMYxk9KlmHD8LrxkyYE2y26/wCqzdSgSbJ3wq21zuUkbSRfTTTjTSekRxvG4YXS9wL24/ATp50smd8x+Rj3bseY8XPh4Vq/KeqZCpzSxaMAI3bYcNe21a4RjETIi3JqjKMSTa5PI0dY96OWXYSFO0tcW1F9bR+FJolc2MPpXi4IAF/HYK2/lGdG215WVntpc61D8hn32cnyJNb5ce0whYeCfos4ii3WdDbgCCoXv4hKuSSFZAREbcA11sfP0Vq/9acoHju3YeOlM/03Mlzrb20vzYTS/ojZIbd1nZ4G9MgkJv6XVwAAddSFqIGOx2ncVFgoZ72vx121cnRNx23IbsBq5fprJ3bdjX7rGj8mMfuWsPCyyy45uiJYaBrtcHyO3Sq4lw1DLKnMZjowNlF+709lFU+l5iPUjjzBrFl9CeA6hh9taObGTaJF0DjOrBU82JVWGNCm0+8GFjfjrtqz5yJTtZbbdC4IC/8AKlTxehHIW6htw7Na0v8ASkhj5iBivbxoSy4QbTKqIjPZZvzGDmKvLuCNHDWGo8Fq+CeDIk2ltqICDKpDqquLNf0i3duHC9Zh0QBtpJDDhWnG+nMq++LcXHZYkEUkzhakraUTC8agELJkdFwWmkPzARdt12kBS51su7sF7U64WNj4dkkDySAcyJbbtDc7ib7QpW4Pjaugw8HMx2maVS/OjKncuoIGgrBN0maKNecpkVlOwDQJc8D5d1JH7i42/I9reaxxAVA1Qp54NvM2l5GtwkN1vrrZR99TfKcooJK6e4rAMAe+yVdH0aWW4VSRxNqT9CmN3KttXiTVb8TsZV6oNLangqTlkKJCpYcAu4XHmAgq9MmGVQTDZmGtmAI9gSsy9JdjYXrXD9PZJ9QBtwvWnLCBWQHmyI+RRORjuASh10sCFPtslMcpFjEmwtfQJuFx5qIxTP8AT04Nhe586s/9byUUFgw8TeluwAfrH1R/yLK08MuvJIY8bMAQfIJUGkgdQWQ2OlgQp+5KvXoGQ72UGta/S2RcKd1+NtdKY5MMf39yltmdWQh1xirMYmD+6EDLfXjwjFRMO8BuUWP7IKg27yAgo0/0wY/W7MF4acSfbVCfT+SW9O7zF6Iz4iKTS/EdwhpQCzcvVhYKCN3/AG6hzEZVk2NYm1gyi2vb6KOR9CzEjZ97LfQDtNVr0DMYAHcUQ3Atpet8+LeQ+qJxHr2QfKileRZTGyqNApsNV7NAo+6iT5EcGKmNseRm9bwqvvge4d41Hj+qmzsHqKTLPMCdo2pHawC9wFSj6lmYEL7WXlyL7r/9IjtvxrSN0Y22zbYSbv0RjFidR/LlWY804wIY+oqIMTG1WJh6pHuSCR2Kvd2mgHVM+XNyS73VV0RO4fie2p5GbNmSbncuRwY6W/yqOFZ54tNw9tVxYxGTkASL6aB1PLJ4tFyI7ndZ6VKlXQuZKn+D201P8HtrLJNxPnTU7cT501ZZKpwlBIC/u/oqFKssKIgVsbHh2VWU9QI0PhUMaQEcpzY/Af1VcwIPtqTEFl0AiQdGei48ZYaa/Yab6iyszA6uuPiZk8cckcbOOYfSW491qo6fnJCQGovnflXV4UXKtHOtgmSli4H7LftCuS6WPPdIGUJRagdjynnC6AEdQj+V0KGaPar5MYUf10yJATpxYsxU1QUjCRMHEqIAhmvoxQbd27xPbQPDi6dhQKMvIk6g66LE7t8uoB0HLvr7dPCrZvqAMwNxt4AdgHdauWeLJItGeTKxNZC0eW6fHEipEYU81u+o+pxYXSWx8bmLn5LAQmPepsh3M4YWvpppQQ9QzOkZPTs2Xqf5njSj/dxrLzAjcHS1+xWuPGiSdaxUy/m0JaYKI0c/9OMfCg7LnVj2+VZ8w9CzpUnyIFEqPvYp6RKO1ZAtvtGtdWGQxwjjljkQ1ZUNSpTxzkTIEa6I68MBIdCGRwGRhwKtqD9lWpgYM8zZE4d22KljIyRxhPjsrLr40D/M8PHgSDFXlxLqibiwUHsG4kjyq2PrcAUo4WRDo6uAynzBrijiz45XQM7Xba63zV5ATg0mu7Ohv0rkz5+fkYeXlTvHFEzQqk8i6q4ufS2uho8cQQwPAZpspXk5kZlO94wQBs3seF6HP1DpHN+YTFiSfslQbGFtNChFRPXEZ7k28K6PuTmyyfHfGNouhIbgvRTxYxGsmcGhBRHqefj9M6RMAJFyp15WPtV1JkbudRxXjoa5o53V+n4uH1QdVOU3MHzGHzSxjt6lWRb63A100owOsYhyYclv5kkAtAre7Gze8472Pf2CqeoS9I6lIJc6AGVSGMiehmF/dfbxv9tUwSGKMYmE6+qZoalJPHORMnHARp0xsqKLPx/VFOokQ+fEew6UN+qMubH6PHLjSSY8qyALJExS4PFW2nXvqrHz8DBhaDDUpExLBC7Oqk/shuFS/NcTJgbEzEWbGfUo3f3g8QfEVDFCWPNeL5Y4kkDdjs3RUkDLHaWuWn6Zg/MOiJkZcs+RNK8is/PlBUKbADa1h31txenYmCJ8WKd5nkIlZZm3yRgDbYnsHdeufxOndLxJJDFn5Aw5bFsRH5ZZh2O68R7L1tfq2JjRCDERYogb7F7Se1idSfE033AyZDMQyZJxyt6LbRHzP5JMcCGJiI27vr5Ik0G7DykQshMTlXQlXDKpZSCuvEUB+inzOpS5hyZ5JhFGoVXZmALk62vbgKJRdXxpsd8aUsglFmeNtj7e1Q1ja9VYeD9PYhLQc6MkWfZkOu4X+PYVpcYtw5MeS4SmfSYxcx8+qacJmYlEUHXVEmxYYZ44G0kkDOidu2O1z5a0+b089Rh2Y2RLiZKgiOSN2C3tazpwt4jWsMP5Vh5EmTjbjNKvLLyyNKVW9yFL3Iv51fD1rHgYsW43Gh1HjUYQnGYOMzIYXGcdeaKkokxN4D7AFC/pfqeb87L0DNUS5MJkEUzHcFZL7hIeLL3fZ5FYOkYXT2LBnysliS0shO1Sf7cVyq+HE1kxX+nsbJ+chjdMoksZ+a5clr7t1zre+tXz9XxXa6EC3DWrfcAy/wDGBxif/p6bTI/yU8WMg/5K2/pqr+pRZXyq42DlR4nUJz/KDau6gEsFtcr/AJrVz+Dny9Gf5P6hxJyX3FMkTSAlb67QH2tbw1oq2T03KykzH/lZ0QtHlxkBx2eoG6uLaajhUs+TCzo4l6vkJkwY7b4seBTEHa1t0pJY/wAKm1N9u2P0MbCHk49d/QjUeKXLCZk7eDGjLfCIfy6E487ZUBu0U0h3uVYkgFjr6eFZeryZ0fTFkwZzjZHOiiR7gIea2z1Ag1mfruKsS4+OiQwp7saAKoHgBVWd1PAz4IoJpZIkiYSBYiq3kX3WYlSTakhjnHOMhuMXJLipHDBUMScVoZ/FVdI+oMiHJl6R9Rs0eUH/AJc8lgqae61uAPENWT6u6pJHLiz9LnnihnRiZFd0SUq1t6KTw8ba0SyMjoPUnhnzk52TAwPM0UyAfBLtFitLqE/RepTrLnoZhGu2KMNsRB3LstXRjnjGT5TjnCcotMaxpwpHFlMbXBANKojNHFFgwcvczNGkm+Ri7XdQSzMxJvXF9ViDStY7rm/maPZHUsZMZcbHLctABGHcuQANBdtbVzuXkF2IU2FtR31vtoyGSczQSNH1ZUkAMYideixqoQ7RY99qmStju93tpC1vGs2RLuOxeA4+ddjXFSJsj+SpYqWJXh2XpqVKqrnSp/g9tNT/AAe2sskQLnXtpWHePvpUqyyVh3j76Vh3j76VKsslbxrduOwCRCJB2qRY/fSpUk9lTHvqluHYrX9n41MSSAemJj3XYClSpKdO6rXr2UTJl3/pD2n/ABpNJlfHEp9o/GlSrU9ndav/ANOybmTdkIHk1LmZP9oW8/8AGlSrU9vdCvv7JCTI7YQf4qXMn/tHh2NSpVqe3utX39kt8v8AaP8AqqO+W+kZ/wBVKlRp7e61fd2Th57/ANM/6qmXyu2M/aKVKgW9vdYP7+yiXm/tt9opB5/7R/1UqVant7rV93ZTDSnijjyINMWkB0RmPiQP10qVanRGvXsnWTI7IT/qqwyZltIWt23YUqVAt7e6Iu9/9qYy5nbCfY1QMmVr/KO3tu2tKlWDezugX9/9qYPJb+m/2inV3v8A03HtBpUqNOndYP7uykzzAHZE5J4biBb7DUObk9sP2tSpUA3t7rF3/f2UTJkX0hA/i/xpGWQnSG3f6qVKjT290K+/slzMn4Yht7ibn9IpB8q/qjv3WNqVKjT291q+/sn3v2xsPaDUWY39xr+YpUqFOndGrfu7KMjOUIjQr3kkGslvGlSp4abKWTUa+aVh3j76Vh3j76VKnU0rDvH309hs49tKlWWX/9k=) 7px 13px no-repeat; }\n"
             "\t\t\t#masthead h1 {margin: 57px 0 0 355px; }\n"
             "\t\t\t#home #masthead h1 {margin-top: 98px; }\n"
             "\t\t\t#masthead h2 {margin-left: 355px; }\n"
             "\t\t\t#masthead ul.params {margin: 20px 0 0 345px; }\n"
             "\t\t\t#masthead p {color: #fff;margin: 20px 20px 0 20px; }\n"
             "\t\t\t#notice {font-size: 12px;-moz-box-shadow: 0px 0px 8px #E41B17;-webkit-box-shadow: 0px 0px 8px #E41B17;box-shadow: 0px 0px 8px #E41B17; }\n"
             "\t\t\t#notice h2 {margin-bottom: 10px; }\n"
             "\t\t\t.alert {width: 800px;padding: 10px;margin: 10px 0 10px 0;background-color: #333;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: inset 0px 0px 6px #C11B17;-webkit-box-shadow: inset 0px 0px 6px #C11B17;box-shadow: inset 0px 0px 6px #C11B17; }\n"
             "\t\t\t.alert p {margin-bottom: 0px; }\n"
             "\t\t\t.section .toggle-content {padding-left: 18px; }\n"
             "\t\t\t.player > .toggle-content {padding-left: 0; }\n"
             "\t\t\t.toc {float: left;padding: 0; }\n"
             "\t\t\t.toc-wide {width: 560px; }\n"
             "\t\t\t.toc-narrow {width: 375px; }\n"
             "\t\t\t.toc li {margin-bottom: 10px;list-style-type: none; }\n"
             "\t\t\t.toc li ul {padding-left: 10px; }\n"
             "\t\t\t.toc li ul li {margin: 0;list-style-type: none;font-size: 13px; }\n"
             "\t\t\t.charts {float: left;width: 541px;margin-top: 10px; }\n"
             "\t\t\t.charts-left {margin-right: 40px; }\n"
             "\t\t\t.charts img {background-color: #333;padding: 5px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
             "\t\t\t.talents div.float {width: auto;margin-right: 50px; }\n"
             "\t\t\ttable.sc {background-color: #333;padding: 4px 2px 2px 2px;margin: 10px 0 20px 0;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
             "\t\t\ttable.sc tr {color: #fff;background-color: #1a1a1a; }\n"
             "\t\t\ttable.sc tr.head {background-color: #aaa;color: #fff; }\n"
             "\t\t\ttable.sc tr.odd {background-color: #222; }\n"
             "\t\t\ttable.sc th {padding: 2px 4px 4px 4px;text-align: center;background-color: #333;color: #fff; }\n"
             "\t\t\ttable.sc td {padding: 2px;text-align: center;font-size: 13px; }\n"
             "\t\t\ttable.sc th.left, table.sc td.left, table.sc tr.left th, table.sc tr.left td {text-align: left; }\n"
             "\t\t\ttable.sc th.right, table.sc td.right, table.sc tr.right th, table.sc tr.right td {text-align: right;padding-right: 4px; }\n"
             "\t\t\ttable.sc th.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
             "\t\t\ttable.sc td.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
             "\t\t\ttable.sc tr.details td {padding: 0 0 15px 15px;text-align: left;background-color: #333;font-size: 11px; }\n"
             "\t\t\ttable.sc tr.details td ul {padding: 0;margin: 4px 0 8px 0; }\n"
             "\t\t\ttable.sc tr.details td ul li {clear: both;padding: 2px;list-style-type: none; }\n"
             "\t\t\ttable.sc tr.details td ul li span.label {display: block;padding: 2px;float: left;width: 145px;margin-right: 4px;background: #222; }\n"
             "\t\t\ttable.sc tr.details td ul li span.tooltip {display: block;float: left;width: 190px; }\n"
             "\t\t\ttable.sc tr.details td ul li span.tooltip-wider {display: block;float: left;width: 350px; }\n"
             "\t\t\ttable.sc tr.details td div.float {width: 350px; }\n"
             "\t\t\ttable.sc tr.details td div.float h5 {margin-top: 4px; }\n"
             "\t\t\ttable.sc tr.details td div.float ul {margin: 0 0 12px 0; }\n"
             "\t\t\ttable.sc td.filler {background-color: #333; }\n"
             "\t\t\ttable.sc .dynamic-buffs tr.details td ul li span.label {width: 120px; }\n"
             "\t\t\ttr.details td table.details {padding: 0px;margin: 5px 0 10px 0; }\n"
             "\t\t\ttr.details td table.details tr th {background-color: #222; }\n"
             "\t\t\ttr.details td table.details tr td {background-color: #2d2d2d; }\n"
             "\t\t\ttr.details td table.details tr.odd td {background-color: #292929; }\n"
             "\t\t\ttr.details td table.details tr td {padding: 1px 3px 1px 3px; }\n"
             "\t\t\ttr.details td table.details tr td.right {text-align: right; }\n"
             "\t\t\t.player-thumbnail {float: right;margin: 8px;border-radius: 12px;-moz-border-radius: 12px;-webkit-border-radius: 12px;-khtml-border-radius: 12px; }\n"
             "\t\t</style>\n" );
  }
}

// print_html_masthead ====================================================

static void print_html_masthead( FILE*  file, sim_t* sim )
{
  // Begin masthead section
  fprintf( file,
           "\t\t<div id=\"masthead\" class=\"section section-open\">\n\n" );

  fprintf( file,
           "\t\t\t<h1><a href=\"http://code.google.com/p/simulationcraft/\">SimulationCraft %s-%s</a></h1>\n"
           "\t\t\t<h2>for World of Warcraft %s %s (build level %s)</h2>\n\n",
           SC_MAJOR_VERSION, SC_MINOR_VERSION, dbc_t::wow_version( sim -> dbc.ptr ), ( sim -> dbc.ptr ? "PTR" : "Live" ), dbc_t::build_level( sim -> dbc.ptr ) );

  time_t rawtime;
  time ( &rawtime );

  fprintf( file,
           "\t\t\t<ul class=\"params\">\n" );
  fprintf( file,
           "\t\t\t\t<li><b>Timestamp:</b> %s</li>\n",
           ctime( &rawtime ) );
  fprintf( file,
           "\t\t\t\t<li><b>Iterations:</b> %d</li>\n",
           sim -> iterations );

  if ( sim -> vary_combat_length > 0.0 )
  {
    timespan_t min_length = sim -> max_time * ( 1 - sim -> vary_combat_length );
    timespan_t max_length = sim -> max_time * ( 1 + sim -> vary_combat_length );
    fprintf( file,
             "\t\t\t\t<li class=\"linked\"><a href=\"#help-fight-length\" class=\"help\"><b>Fight Length:</b> %.0f - %.0f</a></li>\n",
             min_length.total_seconds(),
             max_length.total_seconds() );
  }
  else
  {
    fprintf( file,
             "\t\t\t\t<li><b>Fight Length:</b> %.0f</li>\n",
             sim -> max_time.total_seconds() );
  }
  fprintf( file,
           "\t\t\t\t<li><b>Fight Style:</b> %s</li>\n",
           sim -> fight_style.c_str() );
  fprintf( file,
           "\t\t\t</ul>\n" );
  fprintf( file,
           "\t\t\t<div class=\"clear\"></div>\n\n"
           "\t\t</div>\n\n" );
  // End masthead section
}

} // ANONYMOUS NAMESPACE ====================================================

// report_t::print_html =====================================================
void report_t::print_html( sim_t* sim )
{
  int num_players = ( int ) sim -> players_by_name.size();

  if ( num_players == 0 ) return;
  if ( sim -> simulation_length.mean == 0 ) return;
  if ( sim -> html_file_str.empty() ) return;

  FILE* file = fopen( sim -> html_file_str.c_str(), "w" );
  if ( ! file )
  {
    sim -> errorf( "Unable to open html file '%s'\n", sim -> html_file_str.c_str() );
    return;
  }

  fprintf( file,
           "<!DOCTYPE html>\n\n" );
  fprintf( file,
           "<html>\n\n" );

  fprintf( file,
           "\t<head>\n\n" );
  fprintf( file,
           "\t\t<title>Simulationcraft Results</title>\n\n" );
  fprintf( file,
           "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n\n" );

  print_html_styles( file, sim );

  fprintf( file,
           "\t</head>\n\n" );

  fprintf( file,
           "\t<body>\n\n" );

  if ( ! sim -> error_list.empty() )
  {
    fprintf( file,
             "\t\t<pre>\n" );
    size_t num_errors = sim -> error_list.size();
    for ( size_t i=0; i < num_errors; i++ )
      fprintf( file,
               "      %s\n", sim -> error_list[ i ].c_str() );
    fprintf( file,
             "\t\t</pre>\n\n" );
  }

  // Prints div wrappers for help popups
  fprintf( file,
           "\t\t<div id=\"active-help\">\n"
           "\t\t\t<div id=\"active-help-dynamic\">\n"
           "\t\t\t\t<div class=\"help-box\"></div>\n"
           "\t\t\t\t<a href=\"#\" class=\"close\"><span class=\"hide\">close</span></a>\n"
           "\t\t\t</div>\n"
           "\t\t</div>\n\n" );

  print_html_masthead( file, sim );

#if SC_BETA
  fprintf( file,
           "\t\t<div id=\"notice\" class=\"section section-open\">\n" );
  fprintf( file,
           "\t\t\t<h2>Beta Release</h2>\n" );
  int ii = 0;
  if ( beta_warnings[ 0 ] )
    fprintf( file,
             "\t\t\t<ul>\n" );
  while ( beta_warnings[ ii ] )
  {
    fprintf( file,
             "\t\t\t\t<li>%s</li>\n",
             beta_warnings[ ii ] );
    ii++;
  }
  if ( beta_warnings[ 0 ] )
    fprintf( file,
             "\t\t\t</ul>\n" );
  fprintf( file,
           "\t\t</div>\n\n" );
#endif

  if ( num_players > 1 )
  {
    print_html_contents( file, sim );
  }

  if ( num_players > 1 )
  {
    print_html_raid_summary( file, sim );
    print_html_scale_factors( file, sim );
  }

  // Players
  for ( int i=0; i < num_players; i++ )
  {
    report_t::print_html_player( file, sim -> players_by_name[ i ], i );

    // Pets
    if ( sim -> report_pets_separately )
    {
      for ( pet_t* pet = sim -> players_by_name[ i ] -> pet_list; pet; pet = pet -> next_pet )
      {
        if ( pet -> summoned )
          report_t::print_html_player( file, pet, 1 );
      }
    }
  }

  // Sim Summary
  print_html_sim_summary( file, sim );

  // Report Targets
  if ( sim -> report_targets )
  {
    for ( int i=0; i < ( int ) sim -> targets_by_name.size(); i++ )
    {
      report_t::print_html_player( file, sim -> targets_by_name[ i ], i );

      // Pets
      if ( sim -> report_pets_separately )
      {
        for ( pet_t* pet = sim -> targets_by_name[ i ] -> pet_list; pet; pet = pet -> next_pet )
        {
          //if ( pet -> summoned )
          report_t::print_html_player( file, pet, 1 );
        }
      }
    }
  }

  // Help Boxes
  print_html_help_boxes( file, sim );

  // jQuery
  fprintf ( file,
            "\t\t<script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.7.1/jquery.min.js\"></script>\n" );

  // Toggles, image load-on-demand, etc. Load from simulationcraft.org if
  // hosted_html=1, otherwise embed
  if ( sim -> hosted_html )
  {
    fprintf( file,
             "\t\t<script type=\"text/javascript\" src=\"http://www.simulationcraft.org/js/ga.js\"></script>\n"
             "\t\t<script type=\"text/javascript\" src=\"http://www.simulationcraft.org/js/rep.js\"></script>\n"
             "\t\t<script type=\"text/javascript\" src=\"http://static.wowhead.com/widgets/power.js\"></script>\n" );
  }
  else
  {
    fprintf( file,
             "\t\t<script type=\"text/javascript\">\n"
             "\t\t\tfunction load_images(containers) {\n"
             "\t\t\t\tvar $j = jQuery.noConflict();\n"
             "\t\t\t\tcontainers.each(function() {\n"
             "\t\t\t\t\t$j(this).children('span').each(function() {\n"
             "\t\t\t\t\t\tvar j = jQuery.noConflict();\n"
             "\t\t\t\t\t\timg_class = $j(this).attr('class');\n"
             "\t\t\t\t\t\timg_alt = $j(this).attr('title');\n"
             "\t\t\t\t\t\timg_src = $j(this).html().replace(/&amp;/g, '&');\n"
             "\t\t\t\t\t\tvar img = new Image();\n"
             "\t\t\t\t\t\t$j(img).attr('class', img_class);\n"
             "\t\t\t\t\t\t$j(img).attr('src', img_src);\n"
             "\t\t\t\t\t\t$j(img).attr('alt', img_alt);\n"
             "\t\t\t\t\t\t$j(this).replaceWith(img);\n"
             "\t\t\t\t\t\t$j(this).load();\n"
             "\t\t\t\t\t});\n"
             "\t\t\t\t});\n"
             "\t\t\t}\n"
             "\t\t\tfunction open_anchor(anchor) {\n"
             "\t\t\t\tvar img_id = '';\n"
             "\t\t\t\tvar src = '';\n"
             "\t\t\t\tvar target = '';\n"
             "\t\t\t\tanchor.addClass('open');\n"
             "\t\t\t\tvar section = anchor.parent('.section');\n"
             "\t\t\t\tsection.addClass('section-open');\n"
             "\t\t\t\tsection.removeClass('grouped-first');\n"
             "\t\t\t\tsection.removeClass('grouped-last');\n"
             "\t\t\t\tif (!(section.next().hasClass('section-open'))) {\n"
             "\t\t\t\t\tsection.next().addClass('grouped-first');\n"
             "\t\t\t\t}\n"
             "\t\t\t\tif (!(section.prev().hasClass('section-open'))) {\n"
             "\t\t\t\t\tsection.prev().addClass('grouped-last');\n"
             "\t\t\t\t}\n"
             "\t\t\t\tanchor.next('.toggle-content').show(150);\n"
             "\t\t\t\tchart_containers = anchor.next('.toggle-content').find('.charts');\n"
             "\t\t\t\tload_images(chart_containers);\n"
             "\t\t\t\tsetTimeout(\"var ypos=0;var e=document.getElementById('\" + section.attr('id') + \"');while( e != null ) {ypos += e.offsetTop;e = e.offsetParent;}window.scrollTo(0,ypos);\", 500);\n"
             "\t\t\t}\n"
             "\t\t\tjQuery.noConflict();\n"
             "\t\t\tjQuery(document).ready(function($) {\n"
             "\t\t\t\tvar chart_containers = false;\n"
             "\t\t\t\tvar anchor_check = document.location.href.split('#');\n"
             "\t\t\t\tif (anchor_check.length > 1) {\n"
             "\t\t\t\t\tvar anchor = anchor_check[anchor_check.length - 1];\n"
             "\t\t\t\t}\n"
             "\t\t\t\t$('a.ext').mouseover(function() {\n"
             "\t\t\t\t\t$(this).attr('target', '_blank');\n"
             "\t\t\t\t});\n"
             "\t\t\t\t$('.toggle').click(function(e) {\n"
             "\t\t\t\t\tvar img_id = '';\n"
             "\t\t\t\t\tvar src = '';\n"
             "\t\t\t\t\tvar target = '';\n"
             "\t\t\t\t\te.preventDefault();\n"
             "\t\t\t\t\t$(this).toggleClass('open');\n"
             "\t\t\t\t\tvar section = $(this).parent('.section');\n"
             "\t\t\t\t\tif (section.attr('id') != 'masthead') {\n"
             "\t\t\t\t\t\tsection.toggleClass('section-open');\n"
             "\t\t\t\t\t}\n"
             "\t\t\t\t\tif (section.attr('id') != 'masthead' && section.hasClass('section-open')) {\n"
             "\t\t\t\t\t\tsection.removeClass('grouped-first');\n"
             "\t\t\t\t\t\tsection.removeClass('grouped-last');\n"
             "\t\t\t\t\t\tif (!(section.next().hasClass('section-open'))) {\n"
             "\t\t\t\t\t\t\tsection.next().addClass('grouped-first');\n"
             "\t\t\t\t\t\t}\n"
             "\t\t\t\t\t\tif (!(section.prev().hasClass('section-open'))) {\n"
             "\t\t\t\t\t\t\tsection.prev().addClass('grouped-last');\n"
             "\t\t\t\t\t\t}\n"
             "\t\t\t\t\t} else if (section.attr('id') != 'masthead') {\n"
             "\t\t\t\t\t\tif (section.hasClass('final') || section.next().hasClass('section-open')) {\n"
             "\t\t\t\t\t\t\tsection.addClass('grouped-last');\n"
             "\t\t\t\t\t\t} else {\n"
             "\t\t\t\t\t\t\tsection.next().removeClass('grouped-first');\n"
             "\t\t\t\t\t\t}\n"
             "\t\t\t\t\t\tif (section.prev().hasClass('section-open')) {\n"
             "\t\t\t\t\t\t\tsection.addClass('grouped-first');\n"
             "\t\t\t\t\t\t} else {\n"
             "\t\t\t\t\t\t\tsection.prev().removeClass('grouped-last');\n"
             "\t\t\t\t\t\t}\n"
             "\t\t\t\t\t}\n"
             "\t\t\t\t\t$(this).next('.toggle-content').toggle(150);\n"
             "\t\t\t\t\t$(this).prev('.toggle-thumbnail').toggleClass('hide');\n"
             "\t\t\t\t\tchart_containers = $(this).next('.toggle-content').find('.charts');\n"
             "\t\t\t\t\tload_images(chart_containers);\n"
             "\t\t\t\t});\n"
             "\t\t\t\t$('.toggle-details').click(function(e) {\n"
             "\t\t\t\t\te.preventDefault();\n"
             "\t\t\t\t\t$(this).toggleClass('open');\n"
             "\t\t\t\t\t$(this).parents().next('.details').toggleClass('hide');\n"
             "\t\t\t\t});\n"
             "\t\t\t\t$('.toggle-db-details').click(function(e) {\n"
             "\t\t\t\t\te.preventDefault();\n"
             "\t\t\t\t\t$(this).toggleClass('open');\n"
             "\t\t\t\t\t$(this).parent().next('.toggle-content').toggle(150);\n"
             "\t\t\t\t});\n"
             "\t\t\t\t$('.help').click(function(e) {\n"
             "\t\t\t\t\te.preventDefault();\n"
             "\t\t\t\t\tvar target = $(this).attr('href') + ' .help-box';\n"
             "\t\t\t\t\tvar content = $(target).html();\n"
             "\t\t\t\t\t$('#active-help-dynamic .help-box').html(content);\n"
             "\t\t\t\t\t$('#active-help .help-box').show();\n"
             "\t\t\t\t\tvar t = e.pageY - 20;\n"
             "\t\t\t\t\tvar l = e.pageX - 20;\n"
             "\t\t\t\t\t$('#active-help').css({top:t,left:l});\n"
             "\t\t\t\t\t$('#active-help').show(250);\n"
             "\t\t\t\t});\n"
             "\t\t\t\t$('#active-help a.close').click(function(e) {\n"
             "\t\t\t\t\te.preventDefault();\n"
             "\t\t\t\t\t$('#active-help').toggle(250);\n"
             "\t\t\t\t});\n"
             "\t\t\t\tif (anchor) {\n"
             "\t\t\t\t\tanchor = '#' + anchor;\n"
             "\t\t\t\t\ttarget = $(anchor).children('h2:first');\n"
             "\t\t\t\t\topen_anchor(target);\n"
             "\t\t\t\t}\n"
             "\t\t\t\t$('ul.toc li a').click(function(e) {\n"
             "\t\t\t\t\tanchor = $(this).attr('href');\n"
             "\t\t\t\t\ttarget = $(anchor).children('h2:first');\n"
             "\t\t\t\t\topen_anchor(target);\n"
             "\t\t\t\t});\n"
             "\t\t\t});\n"
             "\t\t</script>\n\n" );
  }

  if ( num_players > 1 ) print_html_raid_imagemaps( file, sim );

  fprintf( file,
           "\t</body>\n\n"
           "</html>\n" );

  fclose( file );
}
