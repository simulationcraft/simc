// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include "data/report_data.inc"

// Helper functions
namespace {

/* Find the first action id associated with a given stats object
 */
int find_id( stats_t* s )
{
  int id = 0;

  for ( size_t i = 0; i < s -> action_list.size(); i++ )
  {
    if ( s -> action_list[ i ] -> id != 0 )
    {
      id = s -> action_list[ i ] -> id;
      break;
    }
  }
  return id;
}

}

// Experimental Raw Ability Output for Blizzard to do comparisons
namespace raw_ability_summary {

double aggregate_damage( const std::vector<stats_t::stats_results_t>& result )
{
  double total = 0;
  for ( size_t i = 0; i < result.size(); i++ )
  {
    total  += result[ i ].fight_actual_amount.mean();
  }
  return total;
}

void print_raw_action_damage( report::sc_html_stream& os, stats_t* s, player_t* p, int j, sim_t* sim )
{
  if ( s -> num_executes.mean() == 0 && s -> compound_amount == 0 && !sim -> debug )
    return;

  int id = find_id( s );

  char format[] =
    "<td class=\"left  small\">%s</td>\n"
    "<td class=\"left  small\">%s</td>\n"
    "<td class=\"left  small\">%s%s</td>\n"
    "<td class=\"right small\">%d</td>\n"
    "<td class=\"right small\">%.0f</td>\n"
    "<td class=\"right small\">%.0f</td>\n"
    "<td class=\"right small\">%.2f</td>\n"
    "<td class=\"right small\">%.0f</td>\n"
    "<td class=\"right small\">%.0f</td>\n"
    "<td class=\"right small\">%.1f</td>\n"
    "<td class=\"right small\">%.1f</td>\n"
    "<td class=\"right small\">%.1f%%</td>\n"
    "<td class=\"right small\">%.1f%%</td>\n"
    "<td class=\"right small\">%.1f%%</td>\n"
    "<td class=\"right small\">%.1f%%</td>\n"
    "<td class=\"right small\">%.2fsec</td>\n"
    "<td class=\"right small\">%.0f</td>\n"
    "<td class=\"right small\">%.2fsec</td>\n"
    "</tr>\n";

  double direct_total = aggregate_damage( s -> direct_results );
  double tick_total = aggregate_damage( s -> tick_results );
  if ( direct_total > 0.0 || tick_total <= 0.0 )
  {
    os << "<tr";
    if ( j & 1 )
      os << " class=\"odd\"";
    os << ">\n";

    os.printf(
      format,
      util::encode_html( p -> name() ).c_str(),
      util::encode_html( s -> player -> name() ).c_str(),
      s -> name_str.c_str(), "",
      id,
      direct_total,
      direct_total / s -> player -> collected_data.fight_length.mean(),
      s -> num_direct_results.mean() / ( s -> player -> collected_data.fight_length.mean() / 60.0 ),
      s -> direct_results[ RESULT_HIT  ].actual_amount.mean(),
      s -> direct_results[ RESULT_CRIT ].actual_amount.mean(),
      s -> num_executes.mean(),
      s -> num_direct_results.mean(),
      s -> direct_results[ RESULT_CRIT ].pct,
      s -> direct_results[ RESULT_MISS ].pct + s -> direct_results[ RESULT_DODGE  ].pct + s -> direct_results[ RESULT_PARRY  ].pct,
      s -> direct_results[ RESULT_GLANCE ].pct,
      s -> direct_results_detail[ FULLTYPE_HIT_BLOCK ].pct + s -> direct_results_detail[ FULLTYPE_HIT_CRITBLOCK ].pct +
      s -> direct_results_detail[ FULLTYPE_GLANCE_BLOCK ].pct + s -> direct_results_detail[ FULLTYPE_GLANCE_CRITBLOCK ].pct +
      s -> direct_results_detail[ FULLTYPE_CRIT_BLOCK ].pct + s -> direct_results_detail[ FULLTYPE_CRIT_CRITBLOCK ].pct,
      s -> total_intervals.mean(),
      s -> total_amount.mean(),
      s -> player -> collected_data.fight_length.mean() );
  }

  if ( tick_total > 0.0 )
  {
    os << "<tr";
    if ( j & 1 )
      os << " class=\"odd\"";
    os << ">\n";

    os.printf(
      format,
      util::encode_html( p -> name() ).c_str(),
      util::encode_html( s -> player -> name() ).c_str(),
      s -> name_str.c_str(), " ticks",
      -id,
      tick_total,
      tick_total / sim -> max_time.total_seconds(),
      s -> num_ticks.mean() / sim -> max_time.total_minutes(),
      s -> tick_results[ RESULT_HIT  ].actual_amount.mean(),
      s -> tick_results[ RESULT_CRIT ].actual_amount.mean(),
      s -> num_executes.mean(),
      s -> num_ticks.mean(),
      s -> tick_results[ RESULT_CRIT ].pct,
      s -> tick_results[ RESULT_MISS ].pct + s -> tick_results[ RESULT_DODGE  ].pct + s -> tick_results[ RESULT_PARRY  ].pct,
      s -> tick_results[ RESULT_GLANCE ].pct,
      s -> tick_results_detail[ FULLTYPE_HIT_BLOCK ].pct + s -> tick_results_detail[ FULLTYPE_HIT_CRITBLOCK ].pct +
      s -> tick_results_detail[ FULLTYPE_GLANCE_BLOCK ].pct + s -> tick_results_detail[ FULLTYPE_GLANCE_CRITBLOCK ].pct +
      s -> tick_results_detail[ FULLTYPE_CRIT_BLOCK ].pct + s -> tick_results_detail[ FULLTYPE_CRIT_CRITBLOCK ].pct,
      s -> total_intervals.mean(),
      s -> total_amount.mean(),
      s -> player -> collected_data.fight_length.mean() );
  }

  for ( size_t i = 0, num_children = s -> children.size(); i < num_children; i++ )
  {
    print_raw_action_damage( os, s -> children[ i ], p, j, sim );
  }
}

void print( report::sc_html_stream& os, sim_t* sim )
{
  os << "<div id=\"raw-abilities\" class=\"section\">\n\n";
  os << "<h2 class=\"toggle\">Raw Ability Summary</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  // Abilities Section
  os << "<table class=\"sc\">\n"
     << "<tr>\n"
     << "<th class=\"left small\">Character</th>\n"
     << "<th class=\"left small\">Unit</th>\n"
     << "<th class=\"small\"><a href=\"#help-ability\" class=\"help\">Ability</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-id\" class=\"help\">Id</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-total\" class=\"help\">Total</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-dps\" class=\"help\">DPS</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-ipm\" class=\"help\">Imp/Min</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-hit\" class=\"help\">Hit</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-crit\" class=\"help\">Crit</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-count\" class=\"help\">Count</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-impacts\" class=\"help\">Impacts</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-crit-pct\" class=\"help\">Crit%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-miss-pct\" class=\"help\">Avoid%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-glance-pct\" class=\"help\">G%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-block-pct\" class=\"help\">B%</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-interval\" class=\"help\">Interval</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-combined\" class=\"help\">Combined</a></th>\n"
     << "<th class=\"small\"><a href=\"#help-duration\" class=\"help\">Duration</a></th>\n"
     << "</tr>\n";

  int count = 0;
  for ( size_t player_i = 0; player_i < sim -> players_by_name.size(); player_i++ )
  {
    player_t* p = sim -> players_by_name[ player_i ];
    for ( size_t i = 0; i < p -> stats_list.size(); ++i )
    {
      stats_t* s = p -> stats_list[ i ];
      if ( s -> parent == NULL )
        print_raw_action_damage( os, s, p, count++, sim );
    }

    for ( size_t pet_i = 0; pet_i < p -> pet_list.size(); ++pet_i )
    {
      pet_t* pet = p -> pet_list[ pet_i ];
      for ( size_t i = 0; i < pet -> stats_list.size(); ++i )
      {
        stats_t* s = pet -> stats_list[ i ];
        if ( s -> parent == NULL )
          print_raw_action_damage( os, s, p, count++, sim );
      }
    }
  }

  // closure
  os << "</table>\n";
  os << "<div class=\"clear\"></div>\n"
     << "</div>\n"
     << "</div>\n\n";
}

} // raw_ability_summary


namespace { // UNNAMED NAMESPACE ==========================================

void print_simc_logo( std::ostream& os )
{
  for ( size_t i = 0; i < sizeof_array( __logo ); ++i )
  {
    os << __logo[ i ];
  }
}

struct html_style_t
{
  virtual void print_html_styles( std::ostream& os) = 0;
  virtual ~html_style_t() {}
};

struct mop_html_style_t : public html_style_t
{
  void print_html_styles( std::ostream& os) override
  {
    os << "<style type=\"text/css\" media=\"all\">\n"
           << "* {border: none;margin: 0;padding: 0; }\n"
           << "body {padding: 5px 25px 25px 25px;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 14px;background: url(data:image/jpeg;base64,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);color: #E2C7A3;text-align: center; }\n"
           << "a {color: #c1a144;text-decoration: none; }\n"
           << "a:hover,a:active {color: #e1b164; }\n"
           << "p {margin: 1em 0 1em 0; }\n"
           << "h1,h2,h3,h4,h5,h6 {width: auto;color: #27a052;margin-top: 1em;margin-bottom: 0.5em; }\n"
           << "h1,h2 {margin: 0;padding: 2px 2px 0 2px; }\n"
           << "h1 {margin: 57px 0 0 355px;font-size: 28px;color: #008467; }\n"
           << "h1 a {color: #c1a144; }\n"
           << "h2 {font-size: 18px; }\n"
           << "h3 {margin: 0 0 4px 0;font-size: 16px; }\n"
           << "h4 {font-size: 12px; }\n"
           << "h5 {font-size: 10px; }\n"
           << "ul,ol {padding-left: 20px; }\n"
           << "ul.float,ol.float {padding: 0;margin: 0; }\n"
           << "ul.float li,ol.float li {display: inline;float: left;padding-right: 6px;margin-right: 6px;list-style-type: none;border-right: 2px solid #333; }\n"
           << ".clear {clear: both; }\n"
           << ".hide, .charts span {display: none; }\n"
           << ".center {text-align: center; }\n"
           << ".float {float: left; }\n"
           << ".mt {margin-top: 20px; }\n"
           << ".mb {margin-bottom: 20px; }\n"
           << ".force-wrap {word-wrap: break-word; }\n"
           << ".mono {font-family: \"Lucida Console\", Monaco, monospace;font-size: 12px; }\n"
           << ".toggle,.toggle-details {cursor: pointer;background-image: url(data:image/png;base64,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);background-repeat: no-repeat; }\n"
           << "h2.toggle {padding-left: 18px;background-size: 16px auto;background-position: 0 4px; }\n"
           << "h2.toggle:hover {color: #47c072; }\n"
           << "h2.open {margin-bottom: 10px;background-position: 0 -18px; }\n"
           << "#home-toc h2.open {margin-top: 20px; }\n"
           << "h3.toggle {padding-left: 16px;background-size: 14px auto;background-position: 0 2px; }\n"
           << "h3.toggle:hover {text-shadow: 0 0 2px #47c072; }\n"
           << "h3.open {background-position: 0 -17px; }\n"
           << "h4.toggle {margin: 0 0 8px 0;padding-left: 12px; }\n"
           << "h4.open {background-position: 0 6px; }\n"
           << "a.toggle-details, span.toggle-details {margin: 0 0 8px 0;padding-left: 12px;background-size: 11px auto;background-position: 0 3px; }\n"
           << "a.open,span.open {background-position: 0 -13px; }\n"
           << "td.small a.toggle-details, span.toggle-details {background-size: 10px auto;background-position: 0 2px; }\n"
           << "td.small a.open, span.open {background-position: 0 -12px; }\n"
           << "#active-help, .help-box {display: none;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px; }\n"
           << "#active-help {position: absolute;width: auto;padding: 3px;background: transparent;z-index: 10; }\n"
           << "#active-help-dynamic {max-width: 400px;padding: 8px 8px 20px 8px;background: #333;font-size: 13px;text-align: left;border: 1px solid #222;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: 4px 4px 10px #000;-webkit-box-shadow: 4px 4px 10px #000;box-shadow: 4px 4px 10px #000; }\n"
           << "#active-help .close {display: block;height: 14px;width: 14px;position: absolute;right: 12px;bottom: 7px;background: #000 url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAYAAAAfSC3RAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAE8SURBVHjafNI/KEVhGMfxc4/j33BZjK4MbkmxnEFiQFcZlMEgZTAZDbIYLEaRUMpCuaU7yCCrJINsJFkUNolSBnKJ71O/V69zb576LOe8v/M+73ueVBzH38HfesQ5bhGiFR2o9xdFidAm1nCFop7VoAvTGHILQy9kCw+0W9F7/o4jHPs7uOAyZrCL0aC05rCgd/uu1Rus4g6VKKAa2wrNKziCPTyhx4InClkt4RNbardFoWG3E3WKCwteJ9pawSt28IEcDr33b7gPy9ysVRZf2rWpzPso0j/yax2T6EazzlynTgL9z2ykBe24xAYm0I8zqdJF2cUtog9tFsxgFs8YR68uwFVeLec1DDYEaXe+MZ1pIBFyZe3WarJKRq5CV59Wiy9IoQGDmPpvVq3/Tg34gz5mR2nUUPzWjwADAFypQitBus+8AAAAAElFTkSuQmCC) no-repeat; }\n"
           << "#active-help .close:hover {background-color: #1d1d1d; }\n"
           << ".help-box h3 {margin: 0 0 12px 0;font-size: 14px;color: #C68E17; }\n"
           << ".help-box p {margin: 0 0 10px 0; }\n"
           << ".help-box {background-color: #000;padding: 10px; }\n"
           << "a.help {color: #C68E17;cursor: help; }\n"
           << "a.help:hover {text-shadow: 0 0 1px #C68E17; }\n"
           << ".section {position: relative;width: 1200px;padding: 4px 8px;margin-left: auto;margin-right: auto;margin-bottom: -1px;background: #160f0b;text-align: left;-moz-box-shadow: 0px 0px 8px #160f0b;-webkit-box-shadow: 0px 0px 8px #160f0b;box-shadow: 0px 0px 8px #160f0b; }\n"
           << ".section-open {margin-top: 25px;margin-bottom: 35px;padding: 8px 8px 10px 8px;-moz-border-radius: 8px;-khtml-border-radius: 8px;-webkit-border-radius: 8px;border-radius: 8px; }\n"
           << ".grouped-first {-moz-border-radius-topright: 8px;-moz-border-radius-topleft: 8px;-khtml-border-top-right-radius: 8px;-khtml-border-top-left-radius: 8px;-webkit-border-top-right-radius: 8px;-webkit-border-top-left-radius: 8px;border-top-right-radius: 8px;border-top-left-radius: 8px;padding-top: 8px; }\n"
           << ".grouped-last {-moz-border-radius-bottomright: 8px;-moz-border-radius-bottomleft: 8px;-khtml-border-bottom-right-radius: 8px;-khtml-border-bottom-left-radius: 8px;-webkit-border-bottom-right-radius: 8px;-webkit-border-bottom-left-radius: 8px;border-bottom-right-radius: 8px;border-bottom-left-radius: 8px;padding-bottom: 8px; }\n"
           << ".section .toggle-content {padding: 0; }\n"
           << ".player-section .toggle-content {padding-left: 16px; }\n"
           << "#home-toc .toggle-content {margin-bottom: 20px; }\n"
           << ".subsection {background-color: #333;width: 1000px;padding: 8px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;font-size: 12px; }\n"
           << ".subsection-small {width: 500px; }\n"
           << ".subsection h4 {margin: 0 0 10px 0;color: #fff; }\n"
           << ".profile .subsection p {margin: 0; }\n"
           << "ul.params {padding: 0;margin: 4px 0 0 6px; }\n"
           << "ul.params li {float: left;padding: 2px 10px 2px 10px;margin-left: 10px;list-style-type: none;background: #2f2f2f;color: #ddd;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 11px;-moz-border-radius: 8px;-khtml-border-radius: 8px;-webkit-border-radius: 8px;border-radius: 8px; }\n"
           << "ul.params li.linked:hover {background: #393939; }\n"
           << "ul.params li a {color: #ddd; }\n"
           << "ul.params li a:hover {text-shadow: none; }\n"
           << ".player h2 {margin: 0; }\n"
           << ".player ul.params {position: relative;top: 2px; }\n"
           << "#masthead {height: auto;padding-bottom: 15px;border: 0;-moz-border-radius: 8px;-khtml-border-radius: 8px;-webkit-border-radius: 8px;border-radius: 8px;text-align: left;color: #FDD017;background: #331d0f url(data:image/jpeg;base64,";
        os << "/9j/4AAQSkZJRgABAQEASABIAAD/4gy4SUNDX1BST0ZJTEUAAQEAAAyoYXBwbAIQAABtbnRyUkdCIFh";
        os << "ZWiAH3AAHABIAEwAlACRhY3NwQVBQTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA9tYAAQAAAADTLW";
        os << "FwcGwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABFkZXNjAAABU";
        os << "AAAAGJkc2NtAAABtAAAAY5jcHJ0AAADRAAAACR3dHB0AAADaAAAABRyWFlaAAADfAAAABRnWFlaAAAD";
        os << "kAAAABRiWFlaAAADpAAAABRyVFJDAAADuAAACAxhYXJnAAALxAAAACB2Y2d0AAAL5AAAADBuZGluAAA";
        os << "MFAAAAD5jaGFkAAAMVAAAACxtbW9kAAAMgAAAAChiVFJDAAADuAAACAxnVFJDAAADuAAACAxhYWJnAA";
        os << "ALxAAAACBhYWdnAAALxAAAACBkZXNjAAAAAAAAAAhEaXNwbGF5AAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        os << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        os << "bWx1YwAAAAAAAAAeAAAADHNrU0sAAAAWAAABeGNhRVMAAAAWAAABeGhlSUwAAAAWAAABeHB0QlIAAAA";
        os << "WAAABeGl0SVQAAAAWAAABeGh1SFUAAAAWAAABeHVrVUEAAAAWAAABeGtvS1IAAAAWAAABeG5iTk8AAA";
        os << "AWAAABeGNzQ1oAAAAWAAABeHpoVFcAAAAWAAABeGRlREUAAAAWAAABeHJvUk8AAAAWAAABeHN2U0UAA";
        os << "AAWAAABeHpoQ04AAAAWAAABeGphSlAAAAAWAAABeGFyAAAAAAAWAAABeGVsR1IAAAAWAAABeHB0UFQA";
        os << "AAAWAAABeG5sTkwAAAAWAAABeGZyRlIAAAAWAAABeGVzRVMAAAAWAAABeHRoVEgAAAAWAAABeHRyVFI";
        os << "AAAAWAAABeGZpRkkAAAAWAAABeGhySFIAAAAWAAABeHBsUEwAAAAWAAABeHJ1UlUAAAAWAAABeGVuVV";
        os << "MAAAAWAAABeGRhREsAAAAWAAABeABBAGMAZQByACAASAAyADcANABIAEwAAHRleHQAAAAAQ29weXJpZ";
        os << "2h0IEFwcGxlLCBJbmMuLCAyMDEyAFhZWiAAAAAAAADz2AABAAAAARYIWFlaIAAAAAAAAGwPAAA4qQAA";
        os << "ApdYWVogAAAAAAAAYjYAALdyAAAR/1hZWiAAAAAAAAAokQAAD+UAAL6XY3VydgAAAAAAAAQAAAAABQA";
        os << "KAA8AFAAZAB4AIwAoAC0AMgA2ADsAQABFAEoATwBUAFkAXgBjAGgAbQByAHcAfACBAIYAiwCQAJUAmg";
        os << "CfAKMAqACtALIAtwC8AMEAxgDLANAA1QDbAOAA5QDrAPAA9gD7AQEBBwENARMBGQEfASUBKwEyATgBP";
        os << "gFFAUwBUgFZAWABZwFuAXUBfAGDAYsBkgGaAaEBqQGxAbkBwQHJAdEB2QHhAekB8gH6AgMCDAIUAh0C";
        os << "JgIvAjgCQQJLAlQCXQJnAnECegKEAo4CmAKiAqwCtgLBAssC1QLgAusC9QMAAwsDFgMhAy0DOANDA08";
        os << "DWgNmA3IDfgOKA5YDogOuA7oDxwPTA+AD7AP5BAYEEwQgBC0EOwRIBFUEYwRxBH4EjASaBKgEtgTEBN";
        os << "ME4QTwBP4FDQUcBSsFOgVJBVgFZwV3BYYFlgWmBbUFxQXVBeUF9gYGBhYGJwY3BkgGWQZqBnsGjAadB";
        os << "q8GwAbRBuMG9QcHBxkHKwc9B08HYQd0B4YHmQesB78H0gflB/gICwgfCDIIRghaCG4IggiWCKoIvgjS";
        os << "COcI+wkQCSUJOglPCWQJeQmPCaQJugnPCeUJ+woRCicKPQpUCmoKgQqYCq4KxQrcCvMLCwsiCzkLUQt";
        os << "pC4ALmAuwC8gL4Qv5DBIMKgxDDFwMdQyODKcMwAzZDPMNDQ0mDUANWg10DY4NqQ3DDd4N+A4TDi4OSQ";
        os << "5kDn8Omw62DtIO7g8JDyUPQQ9eD3oPlg+zD88P7BAJECYQQxBhEH4QmxC5ENcQ9RETETERTxFtEYwRq";
        os << "hHJEegSBxImEkUSZBKEEqMSwxLjEwMTIxNDE2MTgxOkE8UT5RQGFCcUSRRqFIsUrRTOFPAVEhU0FVYV";
        os << "eBWbFb0V4BYDFiYWSRZsFo8WshbWFvoXHRdBF2UXiReuF9IX9xgbGEAYZRiKGK8Y1Rj6GSAZRRlrGZE";
        os << "ZtxndGgQaKhpRGncanhrFGuwbFBs7G2MbihuyG9ocAhwqHFIcexyjHMwc9R0eHUcdcB2ZHcMd7B4WHk";
        os << "Aeah6UHr4e6R8THz4faR+UH78f6iAVIEEgbCCYIMQg8CEcIUghdSGhIc4h+yInIlUigiKvIt0jCiM4I";
        os << "2YjlCPCI/AkHyRNJHwkqyTaJQklOCVoJZclxyX3JicmVyaHJrcm6CcYJ0kneierJ9woDSg/KHEooijU";
        os << "KQYpOClrKZ0p0CoCKjUqaCqbKs8rAis2K2krnSvRLAUsOSxuLKIs1y0MLUEtdi2rLeEuFi5MLoIuty7";
        os << "uLyQvWi+RL8cv/jA1MGwwpDDbMRIxSjGCMbox8jIqMmMymzLUMw0zRjN/M7gz8TQrNGU0njTYNRM1TT";
        os << "WHNcI1/TY3NnI2rjbpNyQ3YDecN9c4FDhQOIw4yDkFOUI5fzm8Ofk6Njp0OrI67zstO2s7qjvoPCc8Z";
        os << "TykPOM9Ij1hPaE94D4gPmA+oD7gPyE/YT+iP+JAI0BkQKZA50EpQWpBrEHuQjBCckK1QvdDOkN9Q8BE";
        os << "A0RHRIpEzkUSRVVFmkXeRiJGZ0arRvBHNUd7R8BIBUhLSJFI10kdSWNJqUnwSjdKfUrESwxLU0uaS+J";
        os << "MKkxyTLpNAk1KTZNN3E4lTm5Ot08AT0lPk0/dUCdQcVC7UQZRUFGbUeZSMVJ8UsdTE1NfU6pT9lRCVI";
        os << "9U21UoVXVVwlYPVlxWqVb3V0RXklfgWC9YfVjLWRpZaVm4WgdaVlqmWvVbRVuVW+VcNVyGXNZdJ114X";
        os << "cleGl5sXr1fD19hX7NgBWBXYKpg/GFPYaJh9WJJYpxi8GNDY5dj62RAZJRk6WU9ZZJl52Y9ZpJm6Gc9";
        os << "Z5Nn6Wg/aJZo7GlDaZpp8WpIap9q92tPa6dr/2xXbK9tCG1gbbluEm5rbsRvHm94b9FwK3CGcOBxOnG";
        os << "VcfByS3KmcwFzXXO4dBR0cHTMdSh1hXXhdj52m3b4d1Z3s3gReG54zHkqeYl553pGeqV7BHtje8J8IX";
        os << "yBfOF9QX2hfgF+Yn7CfyN/hH/lgEeAqIEKgWuBzYIwgpKC9INXg7qEHYSAhOOFR4Wrhg6GcobXhzuHn";
        os << "4gEiGmIzokziZmJ/opkisqLMIuWi/yMY4zKjTGNmI3/jmaOzo82j56QBpBukNaRP5GokhGSepLjk02T";
        os << "tpQglIqU9JVflcmWNJaflwqXdZfgmEyYuJkkmZCZ/JpomtWbQpuvnByciZz3nWSd0p5Anq6fHZ+Ln/q";
        os << "gaaDYoUehtqImopajBqN2o+akVqTHpTilqaYapoum/adup+CoUqjEqTepqaocqo+rAqt1q+msXKzQrU";
        os << "StuK4trqGvFq+LsACwdbDqsWCx1rJLssKzOLOutCW0nLUTtYq2AbZ5tvC3aLfguFm40blKucK6O7q1u";
        os << "y67p7whvJu9Fb2Pvgq+hL7/v3q/9cBwwOzBZ8Hjwl/C28NYw9TEUcTOxUvFyMZGxsPHQce/yD3IvMk6";
        os << "ybnKOMq3yzbLtsw1zLXNNc21zjbOts83z7jQOdC60TzRvtI/0sHTRNPG1EnUy9VO1dHWVdbY11zX4Nh";
        os << "k2OjZbNnx2nba+9uA3AXcit0Q3ZbeHN6i3ynfr+A24L3hROHM4lPi2+Nj4+vkc+T85YTmDeaW5x/nqe";
        os << "gy6LzpRunQ6lvq5etw6/vshu0R7ZzuKO6070DvzPBY8OXxcvH/8ozzGfOn9DT0wvVQ9d72bfb794r4G";
        os << "fio+Tj5x/pX+uf7d/wH/Jj9Kf26/kv+3P9t//9wYXJhAAAAAAADAAAAAmZmAADypwAADVkAABPQAAAK";
        os << "DnZjZ3QAAAAAAAAAAQABAAAAAAAAAAEAAAABAAAAAAAAAAEAAAABAAAAAAAAAAEAAG5kaW4AAAAAAAA";
        os << "ANgAAo4AAAFbAAABPAAAAnoAAACgAAAAPAAAAUEAAAFRAAAIzMwACMzMAAjMzAAAAAAAAAABzZjMyAA";
        os << "AAAAABC7cAAAWW///zVwAABykAAP3X///7t////aYAAAPaAADA9m1tb2QAAAAAAAAEcgAAAmQUAFxfy";
        os << "qwIgAAAAAAAAAAAAAAAAAAAAAD/4QDKRXhpZgAATU0AKgAAAAgABwESAAMAAAABAAEAAAEaAAUAAAAB";
        os << "AAAAYgEbAAUAAAABAAAAagEoAAMAAAABAAIAAAExAAIAAAARAAAAcgEyAAIAAAAUAAAAhIdpAAQAAAA";
        os << "BAAAAmAAAAAAAAABIAAAAAQAAAEgAAAABUGl4ZWxtYXRvciAyLjAuNQAAMjAxMjowNzoyMiAxOTowNz";
        os << "o5NAAAA6ABAAMAAAABAAEAAKACAAQAAAABAAAEsKADAAQAAAABAAABKQAAAAD/4QF4aHR0cDovL25zL";
        os << "mFkb2JlLmNvbS94YXAvMS4wLwA8eDp4bXBtZXRhIHhtbG5zOng9ImFkb2JlOm5zOm1ldGEvIiB4Onht";
        os << "cHRrPSJYTVAgQ29yZSA0LjQuMCI+CiAgIDxyZGY6UkRGIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5";
        os << "vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyI+CiAgICAgIDxyZGY6RGVzY3JpcHRpb24gcmRmOm";
        os << "Fib3V0PSIiCiAgICAgICAgICAgIHhtbG5zOmRjPSJodHRwOi8vcHVybC5vcmcvZGMvZWxlbWVudHMvM";
        os << "S4xLyI+CiAgICAgICAgIDxkYzpzdWJqZWN0PgogICAgICAgICAgICA8cmRmOkJhZy8+CiAgICAgICAg";
        os << "IDwvZGM6c3ViamVjdD4KICAgICAgPC9yZGY6RGVzY3JpcHRpb24+CiAgIDwvcmRmOlJERj4KPC94Onh";
        os << "tcG1ldGE+CgD/2wBDAAYEBQYFBAYGBQYHBwYIChAKCgkJChQODwwQFxQYGBcUFhYaHSUfGhsjHBYWIC";
        os << "wgIyYnKSopGR8tMC0oMCUoKSj/2wBDAQcHBwoIChMKChMoGhYaKCgoKCgoKCgoKCgoKCgoKCgoKCgoK";
        os << "CgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCj/wAARCAEpBLADASIAAhEBAxEB/8QAHwAAAQUBAQEB";
        os << "AQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJ";
        os << "xFDKBkaEII0KxwRVS0fAkM2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZW";
        os << "ZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1";
        os << "NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL";
        os << "/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChY";
        os << "kNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiI";
        os << "mKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09";
        os << "fb3+Pn6/9oADAMBAAIRAxEAPwD52wBxTW4BPtTw25CTj8KikycVzI62BPI46HNGetCk7cUuMg0CHAYX";
        os << "pSHtxUmcgYph7UIGNbihhkfSlakNMkchxj6VJgbeaiWpFfAx2oZSG4zmkI+YU896YeCKBMa33j9aUcm";
        os << "hvvmlXrxQIRe9Ieppyd6b60wGnp9aaODSn0pBTJHkcn0peMgd6a3ByKUnmkMP60q9Rk03I4pV46mgB3";
        os << "IoPTPpSHoc80hOFNAxxGepprfdBFBPH1prcAUCAdcdqdnqB1qEuSfQU5XOOelOwidRlaixhDUseSKac";
        os << "7CD0pFEBqSM/u85qN+KcP8AVj61RKHE5JoTBpozTkJFIYZweKU880AZ5obgYFACDtStwM96MdKa55xT";
        os << "EOQ5FKetMBIPSlQ80AKvSk7kUL0Ipe5oATODUnHJ9abxxRuJJxQCE5JpNuQKUn0oJwopMaIyvIpw9qB";
        os << "1FIMknFMQ/HOaaeR70p4puefamAN1PvUbjPenk5qN+nFCEJj3pQeTSfw0meaYhR1zTScULyaGoAUc5p";
        os << "6HpUS9alHHahgiRv4aRz81Ix6U1up+lIY5etBHzGmp94fWnZ5NAg6EZ6VJtG0YqMHkU/dkCgYw85pg4";
        os << "apG/pTMfNQA4H0o9aavFOU5HvQIWkFBpKBscaKOnFJntQA7+EjNJ2xSbsLml7UwEUfOKGHzmhetK/Wg";
        os << "A7Yp38OM0g6Um7K5pABoHFJnNL14pgIe9Kab0pw5oAPSkb3oY4FNbmgQHlqeOMUzHzU8f0oGP2jac0z";
        os << "GScdKfuwKjJ5NIBQORSN1ozyKa/3jQIenLUqn71MX7w+lKp6n1oGhH71G3GKeeaibrTEOBzSnrmkWhu";
        os << "DQA8nkU3HvRnml/hoAVBjvUi9R7VGnTmpAcUhijpTsc5pmeR6U4ZNAwPNNC8mlOQRmlPU0gALwaXkUo";
        os << "OVOaQH1oQ2PwOvNMzkmnbiCM0EYzigTG9xQ3SjuKG6AUCFHWh+AaRzzSEknpQMVeRmkPehDzinetACD";
        os << "gZpM5PNOHIwaCMc0gQj4oBwaRzmkoGLIf3ec0wUrf6s/WmpzTESYyoqZhheKYAdgA6U6TgUhjM+vWmn";
        os << "rjtTWc446Ugcg+oosImX7pzSgY6GmLyDTgfl+lIYo6A+tKcnvTeqilHQY4oGI3U80f0pG56GjI5oEO7";
        os << "kd6bjkelAPJoXk80ANNKOlIaUdqYhw6inN/DSdxSv/AA0igPBNIv3h9aVvvc+lC/fFACgfMaXAGKQck";
        os << "08dqQ0LgAcVE5znHpUjPkY7VGw4NCGxqjH40o5PSkHFKnNBKAHr9acy5WkHen5wDmkykRZ6UmeSfWnY";
        os << "wB3prE7cUAOXkA+1PwDwaijyM1KW2oCMfjQxoaF+Xik2/dJ59qcrfLRzkHHagAC8DpTjjFNc4xSg5Ga";
        os << "BCA9aQjgUp64obpQIa/GaXjNNl4P4UIcNz0piHYx+VJ2qQrkcdcUxxhM96B2HKcqc1EzelPH3SO5qPG";
        os << "DQA1v9ZUikg/SkI/eU4DmmJCRNkvmlHWiJQN9Ljk0gIm+8aQU5+tNFMQsnQ0fxA0jHINNB5piuP6mlH";
        os << "Wmg80oNIY8n1prEbaGPFNJ4oAVjyoobp1pq9TQTxTAiyCfc1Ioynqaj24PSp4BlCO+aGSiWMggDOOKA";
        os << "BsOeQKWPCqwIPTsKZlRAeep6UiyCUYXn8KX+FabKc4ApZDtiUiqJHL96nqM1DG4ONwx9KsRlWYAEUmC";
        os << "FH6UhFSkdcdKiYUhsYzYANGM4PrSNzxT9oGOKYhtNUkZNTY7e1NCjHSgBqng04D9aaw24p69OKAEPBF";
        os << "PXC5460jZHJoLKPvECgERvgHjvRn5RTJJAMlRmljbdHnvmnYLiqMuMetKBzxwM4zSRsA+DSJjB570CH";
        os << "khSRnNMxj2NKTnAHakmwAPc0AIOhqNqeajPUU0AZxTQfmNDGmE8mmIkTHFI1IpxinYoAROoqTvUS9ak";
        os << "7igEOY/dobrTX6rStzSGIv3hS560g4ajvTAd6UtIThRTkHy0AITTf4qcw4pBwaAEHTmkXrThQo4JoEK";
        os << "aBQe1FIBG6Ug4oPNHrimAo56048ikUZ6048CgY1eCKVupoHUUHqaAAHFIeOlPHI5pjDHSgBp5pymm+l";
        os << "A45oEOIoWlFIOpoAa1BpzjgGg8UAN/ipynrQ3JpVGBQMSk9ac4+Wmg5U0AGelI33qO9Dct7UCFXg0Kf";
        os << "vUgoTq1Axe9Rv1NPzyajbrQJiilfvSU1uQaAHZ+YU7OaiB+YU9aBDlqTsKiHU1KPWkMXGfc0/IYgZxT";
        os << "IcEH604HGQe9IYEc88jOKGGHOfWkkxgc96JGBfAoAdn5TQmCaZI22PPfNJHIDjcPyoHcsNhscdKaOTS";
        os << "hlP3TQuT0pAIRTGPFPYcc01RnNADWJODTqcVGOlOx29qLgRY6n0pVbIJp20HPFMXjigB4px/SmqKlA6";
        os << "Z6UikQMMU09akkKqxBIqCRwM7Rn600SxxPytSRDKnH40RndGxJpsRxwaYFsj5BjgetEhABGc8Uz5TAO";
        os << "eh6U+XDBQAenWpKK7DCe9R5wfep5xhAO+agxmmiGTr0zmkU8sDTQePrQ3UUFD1I204e1Rg8U9TxSAO9";
        os << "NPWlJ5ppPNMBT94mlTpTCeTSqcAUCQppV6ikPNKnWgZIetJM2CmKdjkUkq52CkMaxJPWmL/rKkK801R";
        os << "+8FAMVW9alY4AxUGOakPKgdxQMM8UYzx7UqD93nvTguBz1pANwM0idqa5y3tSxcmmIco4NKT0oXpQOu";
        os << "KkYo6UhXg9KU8DNNQ7s5oGJt+8Rx7UFRt5p3OScdqRm+WgYMNoz2pVbdilk5GKijO18dqNxPcdICW4F";
        os << "OQYXBp5HPHpTQMGi4W1Gt96l96RvvUZ4oENl5GaTFPIyPamEYOKaEPjPapZMBDkelQxg5p8pOCKCuhA";
        os << "D8xFLjmmsMMKf3pkgfv1KAKhPLinZIpDQ9B8zUnrSqQSfWjoSPWgCOQcimfxVK/rTCKZLI+uaYKd3NI";
        os << "vWmSKOtOpop34UDQpxTDTj0qNiaSGwDcn1qSNc5zTY1x161IrbUJxmmxIWTaNqnqe/pShdhyvWoH5NS";
        os << "xzDo/OO9Kw+pL5mRgjbx1ppVBHxinoYynLLiqzbB0JI9KENjXG85A4FEnzbcdBTj93A4FIeQABTJI9p";
        os << "A4HFKoIYEg1My4WhGQn7wp3FYj8xlHBNOE55yAak2Kw7GomQKT/AHfX0o0G7i+aMDK8/Wn+cmRw1RmE";
        os << "DGDxSmI9jRoIkM6Z7/lQJkx0aovKOfvUohIHWjQLitKCCAp/OkErY6CnCHI60zYD06D9aNAGSSswOSa";
        os << "FyT0NP2j2FPVkx94UwRXdSDz0p8Z2Hn7pNSOuc1H0wDQA9MLMCeh705FUk5I61GjYbB5HpSxhCSCSBS";
        os << "BDy/zEKNwHeoyC5yevapSUA4YCoJJccL+dCBkisGyvcVE/FRgkcinbt681VhXGtTDTyOKaaBC8cU8c1";
        os << "GOSKlHWgBqj5jTu4pBwaXPSgaBuStLSHqKWgAH3qO9IPvU5euaADrx71Kn3RUK8salXoKBoQ0lB9qBy";
        os << "aQABS9qUdKQnFACUjnApRTWOTTEJThwKTHrSmgB605ulMTpT36UhoYOooPU0DrQ3WmA9fu01qcnSmP0";
        os << "pAIeabSr1ox6UxCpyKU01Tg0rdKAHU1h+VKDSmkMaaUUh4NKPemIc33TUXQ4qR+hqJuCKBsXvQ33qD1";
        os << "pD96gQtNXq1LSDgmgBe5prDmlz1pDyaAFNM45p561GepoEIKcKaOtOHSgCVOalZgu1e5qDfsXjmmkk8";
        os << "mlYZOAUOV6nrUgf5gCNoPeoI5c8N+dWMoQMsDSY0I6qDximOd0xI6CiQICACSKR2y2BwPSgGNkbf0+6";
        os << "KaikngU4c5AqVFxTAhbI7URysuME1MzLj7wpmwexoBimViOgpyygAAqfzpmwDGfun9KeYcDrS0AcZkx";
        os << "0agTpn+L8qYYSR1pvlHP3qLILknnJk8NTfNGDhefrSCI9zSCEHOScUaDHGduMAD8Kb5jMOSaFTJGPu+";
        os << "vrUuxVBzgUaArldsliQKNpxz0qZ2QH7wpVXK0XFYij+Xdn7p/ShBsOSODThwCD1oH3cHkelIZMFQx84";
        os << "p/m4GAAwA61XXYepIx2qy5jCcMoGKTKRHt8xiW60ke0llHUd/WkkmHITv3qFOD70WF1JJVxjFRluR61";
        os << "MzbowcYqORc9OtNCYA04VGpqQUmNAaaaU0hpiY096d0ApG60vcUCH96dGOaQCnp60i0O9KHHzLS5yRi";
        os << "kdgCPWkMVhUQ/1lOOW5NNHD0wYmOaQn5gKf3qMDLGgRbjwUGBzzUcrHkUsJwAKbIDk0iugzFOi4BNNA";
        os << "ycU9Rge1Nki0i/eNGeKF+9UjHOMrgdabGCG5FPNOA5GfTilcdhjNtJpoG7mmSHc+O1Sx8LinsC3Fzmo";
        os << "2XkmpBTQOlADk6EUYpF/iFGeaQDT945pvc0ueTTT1qiWO7UpAJGaTOBSk8igY8YApjmnZ4FMc9KBsjf";
        os << "iQVJ3zUcv3wakY7V5pkjT98GlNIjgkD2pWyOlIENJwSRT81GVOTxxT6YDuq0zpmnr92mmkBAepoFKw5";
        os << "/GjtVEiClB5oFL1oBCN0zSIOppSMnHalHXigBTxntUanOTSyHJwOnem9sUAKOW9qCuCcUo4FBGQeM0A";
        os << "MywGBSLn0qTbhTjnHWkV1z3pgO2kpz0p6qAvt60Fx5RwPzqIsS3r7Ug2FuH3LgdPX1p0CgLvxk1Gw4y";
        os << "acjlOvSmCepZwNmePrUT42vTt6fwg5+lM5bORgelShsVecBvSnc+nNRs+0jHWmPIxPJ/KnYVywF9qVu";
        os << "mKqK59Wp6yHHB/OnYSJuitj0pAoEaemKSOTdnPBoYlF46H9KQxRgLniomwU3YwaXevUg5+lRs5bOOBT";
        os << "SAI3xwen8qkdQVU9c1Fj0pd2MdqZIzBB47Ug3ZqQMM8j8qRnUHvQIZkkn60hBzUmATk9D2pCMdBincC";
        os << "M8UgODT2HFR0AP7U09DQDg0vY0AIOoqQnOKj71IKAEJ4pOhobpQO1ADgeafTFp460DGt96nJSHnP0oH";
        os << "agAT7xqRaZH9409T1oBCGhetDUi0hjqQ9aWm9OaBBSYoHNKaAF25x9KMc07OMUnWmMUdaVzxTaGPNIA";
        os << "XqKVuppB1pW6mmAqHikNCmigBMc8UFcZpc4peuaAI8UA/lQKQ8UCHDrTqbnJzS0AI3WlHSmt1py9BQA";
        os << "rVG/3hT3PSmyfeFAMRqReopTSCgB1MPWnGmGgBO9KDxSHvQtAhQcUw9TTzTO9AMB0FLR2ppOTigAPJp";
        os << "w5NMqRRigAC80ZIP404DPUZpcAHI7dqAGndnpS4JPPelV1J707cMjA/OkBJGoCt2xUcj54A4/nRuJz3";
        os << "puMdaBki4CbsZNTHBXPFVlcrjPIqTevUA5+lJjuSEAxt6Yp38K59KYpLjnp6etLJJtxjk4pDHryMUhX";
        os << "vioWkJHJ/Ko2c+pp2EWufxprDGdvpUKSMDwfzqRX3HnrSsNDkxsSpcDZnioeVxjkelO3rzuBz9KBojn";
        os << "UFdwGDTbd9owenr6UO5fp0pq9Min0F1JmVWHqPWmYIXjpSBiG649qlDjyuRSArtn0oyxGDT2dc96Xbl";
        os << "eePSmIaFyRSnhvagDGOMd6UnINIYjHABFSDnFQ9sU+M4OD+FAA46GlXpmlPXmkAwcdqAAnmkNO6U00g";
        os << "YGgdRQOlKBz+NMCXGcU/otMFPblKRQ3OOlMByQTTs00KfTigB4/KkH3zSqCetI7AEg+lAMXvmo05c1K";
        os << "p3LxUcX3zQBLGc+1POGWooz1p+eDSKEUAZpOxoB60Z4oEN9KePvDFMHWnZ6UCQ/HNDngCgnmkb+EUih";
        os << "qryDT84pCOtKaAIwTnmhTyKQHqO1JTsTckHU0metIDSUDuLSGkJpKYrjz0pCcUD7opp5YDtSAee1Nfn";
        os << "HtSk0hoGI3LD2pHbI5p5GMVG4xTEwAwwx6VKDkVGByMU/GB70AgNJSk8UmeKBC54ozSZpM9aBjDyaT+";
        os << "Gj1o/hpkjQwPFLnjg1GOG5paYiVRxzQx249aYGI5pNxZskYFIAJxQnqabnLe1OHSmBIuCMd6TBBIpg7";
        os << "/Wl3nuaQx+MA5qHad4AOKkLZz1NJnbksaEA4INmSaMKOnpUbHIyaUAqo9TzTEKRubApzgEgDoKD8i47";
        os << "mnxDJB9KQ0SBdoHr3qCRhkhT9aklfHHf1qD3PFCBiHnrT9qry3Wmryd1PHDZIyT0piFBH/PM49aCiso";
        os << "K/kacN394Z9KGycnGCBzSAixgHFKjZADH6UE/xUwjIyORTAkK7h70xRjOe9SRNxjGW9aSQcZ9aAGAbW";
        os << "waVtvGaPvAg9aY3zJjuKYhCoycGmhfnxSLw2al+9gjtTEB5Ge44pCCcc0hPJ7UhcnvSGK2AMd6hanHP";
        os << "emmmITrThyD60zoacKBC0/PFNPQGlFMYHgY70q9KRuvHpSjpSAdnBozSGgHigY7uaTtQDzQBmgQsfU0";
        os << "5eCaanU04UDBulNBpW6U0UASDkVGx5xTlPyUw9aAHg04cUxeTT6AQ4npmmEkHinGmMeaAHA5NK3WmA8";
        os << "4pT1NILjvSl9aaKDQMVetBOKQHkU0nmmIUcmpAetRA808d6BiHmmk08VGetAgU84p54FRjrT2Py0AIT";
        os << "Tl6Uw05elAA3JFJJyRSmmt1FAMB0o7ig0d6BBmk6mg9KQUDEbpQOnvSnpSL1oEHam96dSdAaAEPA96Z";
        os << "Smm9TTEOWp1xjHeoVpwzSGSgEd6XoCe54pgcjvSg80hjCvz4FOCjIyaeflyT3qJuWzTETrjJxSEbmwK";
        os << "avypjuad90ADrSGIw6YqQLtHvSRjjPpSyscYxg0hjWbghT9aTGQM00DAyelPB/ioAeEVVy35CkJGP9W";
        os << "cetOXIwcZJFB3f3hn0pAM2q3K0wcU88tkDBHWmNwd1MB8bDIDc+lTld2R3qr7irET54PWkxojQAEg9D";
        os << "TANrYPSppRgk+tMA3rg9RQAYXv6UhQbODTSCyn1FNU4GRTATad5Gc1PjIGOaZnccqaUPjHUUhC4JIBp";
        os << "WwBjv/KmbyTgGkPagYj+ooHNKeRTejcUxEqnP1pWHHvUW4q2QMinFi3NIBc8cnmkLDpTTSdW4oAl/ho";
        os << "HBo/ho64oGS5pc8UzPSlz1pFCmgUmeKUHigBS2BURGWOfSpMZHvTCOTmgAVsDinLwx96YozipcZzQCG";
        os << "ocZpwPWmilBoGKDmgdOaaOGIp2floEIKXNN+tAoAfnpTj1BqOnE0hgx5NBJzxTaCeg7UWFcaKXvTc4N";
        os << "OpiFBopF7UtAxDTTTqRqBCqeKVRk0idDT1/pSGhmPmIoHoacB85xQvU4oAG+8Ka46U5vvUN92gBq9ac";
        os << "1AGGyMUHgUANPQ0lKehpvWmJijoKRuppyjimuORQAw0dqD1pD93NMQ0gHvSomT14pmSTSk7RgdTTEOb";
        os << "HbpTGPOKYTjvTo/vZPJpANANPXOD608jk46U4L+NAWIgSCM08+9DfdpM5GO4oAbvOSAKaOT83Wgj5iK";
        os << "WNSxpgSRLkknoKeBk5PanYyAq9KSU4wopDI2yzZqZTtXPb+dRj7v1pHbkAdBQAhJZj3PWnbFHU5NKo2";
        os << "Jn+JqWNRzxlvU0ARkcgAECnA/eP4Ur8t1JpiHcSOtAiUYx935fWgtnafTikOcdvpTTgEDGDQMMHBGDi";
        os << "lCqRx1pyYxwxzTW9ejetAiMkoeeDTgdyj3/SmyksnuKbG2CB2NUAAlWp54II70EcUiHIKmgQx12vkdD";
        os << "Tfcdal6MVPTFRspBx2piGlzmjvSd6XocYoACSTTGB71KOnFNfoKAIjmlXpTu1IV4yKBCqeaeBUQOaep";
        os << "zx3FABwTzSjpSMDQDQMdSjpSdqcBQAtIOhp3YU3tQMVTyad2pq9aXtQAh9KO1OpG6UAN6CikNKvWgQ5";
        os << "RTqSloGIaaetOPeikAzNO+tGcmk9aYh3alpO1A6UhifSkpaM4NMQDrTl60nQ0DtQMWmsKd2pKAGUvUU";
        os << "N1pBzQIdjiihelL3oGHamt1FO7U1utAAelFJ2p3Y0ANPSk7UpHFN7UCA9KTjPFBNABoAUjimsefalc4";
        os << "47mmZoARulIBTwvGTS9qYhFBxxTwTnBoXpTjSGNPWgOc0nU9KTvQA/r160qLubnoKRVJPtUnVwo6UAK";
        os << "OSSabyzU5zgBRQBx70hjidqn2/WmglzxzmmyNkn0FOiJVPc0DJCqjryaTBwBg4pVHfq3rSvjH3jmkAB";
        os << "sbj68UHGPu/L600YJIxk9qUZx2+lIBCfun8KaBgkYJFDnBA6U9OD1xQAmxT0ODTQSrDsetSSKOOMN6i";
        os << "kYb0z/EvWgY5juXPb+VRL8rUqNyQehoI+UH0oAceDkd6ZIuDkdDT4jnKmlxgFWoArHg8dadvJOKSRSp";
        os << "pB94CmIlHtTCSScU7OBjvSr92kAxgcCmEGpyvpgUgHPPSgLDFPOKeuOh6VHIfmyODTc5pgTOmD14poA";
        os << "HekB3DB6ikyQaAJe1KKQdM0o60hj15NKelIo5NOYUhjacOgpnSnDoKAQ9aa33qVeRQRlsnFIY1B1py/";
        os << "eNCn5aVfvGgBh9KXHzAUrdRS4+cUwGsO9IelSN3+lRv296AYlKKQdaXvQIdSE0Chu9Awpp5NLSE5OKB";
        os << "CLgn5utO70qgd+tHegYAc0nanL1/Ck7UAJ3oPWiigQoGAaUEUAfJmmnrSGPHXijHyn60qj5aXHFAxjf";
        os << "eoPK0OPm4oP3cUCFximmn01hxQMYe9JinHpR2pk2Cmuead6UhGTQMbTH+7xTjxmmE8U0Sxo9KTa2eRz";
        os << "TkHNTL7jii4iqVPXqKVTlhVl0XaWXpVfG1ge2eaLgKp+cn0PSrGQVDZGe1VAdr1MkiY5z+VDQ0EowpO";
        os << "eKhVvmJqZz5ijjCj9aYBzjGM0IGLtGSxOacD6Co0Bb6ZqcAKMmgEOA2LnvUYG5snvTiS3Pagnb16d6B";
        os << "iSHaOPwpgGRmhzubinbgCFoESYwy/TikU/K315pfvIAPvLUeSWJU4buDSAeOVOOtNGAwxyOg9jR/Fkg";
        os << "g+1M3Abuo560wZKQPxpVAySenekXfjgjHrQSCo6nnOaAEIwnPXPFI33fxpy8lsAk+9NJ24JOW7AUCEI";
        os << "Bc59OaiIwoqUDAOfvGmEgnb2qkA5ORzTWGDxQDtf2pzcgHt2oENPzZPtSZ6A0gyHPpUmA1AEJUZz0ph";
        os << "OGzUjKQfahhyeM0xDlwVBpsgGPejOzn+E013Ujv+VIYwnGPegnAoJyRQBkE0xDQOaVN27IFOC9CelSL";
        os << "7DigSEPOcikwMc0489aQmgYgHanDikooAfTe1KDQvQ0DEXqadSJ1pwoBCZpCcilbpTRQAh6UL1pcfLS";
        os << "L1oESClNIKWgYh5paU9aTvQAhAzxSDqadgZ4pMYJpALRRS9KAG96UAZ5pAMkU7Azz1pgJQBzR3pQOTQ";
        os << "AgpKUUhoAY1A6UHrSj7tAhQeKKaacvIoGLTW604jimt1FAMD0paQ9BQTQAh5pD6UtIeaBCY44pynpgU";
        os << "DpQOOlAET53cim1Oec5HFMZepHSgBAcigc59qCMAGkB2k0ASpjGe9ObAUmmK6gd/wAqXO/2UUhjAcsa";
        os << "cFG7PWnKOQMYpFUk+1MQ7PJApR8uCfSnYCjJqNslx6UhjlG4805+BQvAPp3ppO5vagBAMqakAAcfTim";
        os << "ggHb2p55UYPzCkMVfu/jSgZTjrk5pud2SOG7g05sArwQfakA5gNwx07UgA/GkBAU9RznNDb8dR65oGN";
        os << "OC3PTofrTjwoz1qPcDt6nnrTs5bgEn3oBDmPyr9eKdjLt9OaiyQwLct2A7VKPlQg/eNAIhIwM06M7hz";
        os << "+NJuBJWmodrc0API2tnuKkI3rnvTQd2cdO1AJXkdKBkZJxyKbtGQwOKlYBlyKhcFfpnFAmNZvmyamiG";
        os << "VBpmOemach8tTxlTQxIlyApbIz3qBj849z0pzyIRxn8qiJJehIbYrHDGkCnr0FOxls9s1OqLtDNRcRX";
        os << "2nPyilqw3sOKhcZNCYWFT7vNPqMHing5xQND16071poGDTvWkUNPWlFHUUo6UCFA4pcDikUcU6kUNHC";
        os << "0q/epB0xSoOaBC4+UfWg9adjIpGHymgY0kGmkcCkHWnHlKYhBSd+KWgUAB6UEc0U5uo+lADO9I+Aflp";
        os << "x60MB260AIAOp60o60h5pV60APHf6Uyn9z9KYKQwNNFOb9aTHrTJHfw0hFKegpR1FIY5BhaC1OA4Aps";
        os << "g+YYpFdBknBFIMkU6TqB7UAfKaZPUO4oNKPvD6UpGBQMjbpR2pTSe1AhB0ox1pyjik6UAV275NNzxQ/";
        os << "LGkPOB71ZAoOAPU1KmAe9RhRuA6UpG0470hofkqAScjNIQMn0NEo+QEH8Kb0ApDZGQc59KRFJ5qQNl2";
        os << "xxUiLxxTFYbG3BVqCDnHanFc9qYwweM0AKMLSn5sZ4FIqHPSpFA6mgAA9evpTZjjCjr3qSQhV3d6rn5";
        os << "nyaENiDipFXoQM0bMHJ/KntJyFQc0CQFGxkcVESMneMH1FWTu/vDPpULANu3dR196ENjOQRhsrS4HOf";
        os << "WkHK47il3dwPYimSPCe520cAAe9NXy8Z3fhTt38RGOMAUgEGW3ZYhRSAgD5Bk+tL92Mj1pwwkYwMk0D";
        os << "GIjHlsc0roMdMc9acmTxvANKX6qw5pgVyM+5pynIC/lSleTt/KmHgqRTJHEfMaQfL9Kcp3Env3pcZPF";
        os << "AWI87gaQkg9aV15PHeo2HODmgAdgRgUzHHNPwBSkUxEeKeAMfSkY/MKUfcNAARu6dKkIHGKjjHU/pTw";
        os << "M9uaQ0IeQfamjpSt3ApB0piF7UvakHWnEUDE6E0g6U7ufpSAcUAKnJNOFJH1NKnU0AgbpTAKkboabQD";
        os << "G/w0ijJp4HHNA60AKKWgUUgFI6UnenZ5FIetAw4696RqWkbrQIUCgigdqD3oGIvWlOOvehetLQIb3pw";
        os << "HJpB1p2etMBlIadSGgCMjml/hp3egjigBpFOUcUlPXpQAhpj8EU9+opsnUUANPIpepFGOKB1H0oATtR";
        os << "2p2KYaAA9KUcAe9IelKvbNAhwA5zTANvWnkY+tMkHQ/pSGwIGKZjj6U8/dFIp+Y0xDMelSRtxg0Y4oI";
        os << "BoAcpJNOzgColHPGalReRx3pDBvm+lOVfmFOwAeaRjtIPftQAjHAK/nTANvsaUcliaeF5G78qAFRRjp";
        os << "nnrQ6MOVxUgfoqjmmvkcFhmkUMOCPnGD6045XbhiVNO4eNsjBFNzujAPakAvBBHvQye52/pSbv4gM9i";
        os << "Ka2zGdx+lAgwOCPWk5LHLfLRu7kewpDwuPWmAoI3DZyfWpAjYyeaao2hdvU9KmG7+8M+lJjRCV6kjFR";
        os << "HmrCyckOOaj2Z5FAMWE5yp69qeRx7+lQg7XyOKnjIYBu/ehjREMr05FIcNUjAHkcVGyHPSgQAHOO1Ej";
        os << "cBVpFGTzmpNoHQUAV3UjBpQDn69KmdeOaYWw696LhYeAMj0FLksDg4GaZ1BpYh8hJPfpSBCvgnv+NRE";
        os << "5B9RTwNx96RgNxA5poGNzxT17YNMHpSpwwpiLBGMUp6UvXmlYfLUlje1KvSkx2pQOKAFHFL3P0petIf";
        os << "vH6UhjTkCljPJpSPlFJH1INAhyt1pXGVojHzHNOYcEUFdCEClP3aO5pAODQSNNLSYpw4+tMBKeR0phq";
        os << "TuPpSAjPWg4PI60N1pOlMBR6UDrR05pqnJoBkvc/Sm04Hg59KZSGH1pO9LSgUCFboKD2NDZwKO4oGSD";
        os << "tQ/bNAobGKRRHJzIKXPBpG+8DQxpkiKfnAp7daYv3qeaAQw0nenAZJpQOTQFgAwlMY8VIfu1E3ShAyu";
        os << "eSaAOM0DqaeBwAKogTd6jNMDY7U/GDj0oA5oACSwxjims2NoFThQAc1UdssTQgYZO8kVNE2e/wCBquD";
        os << "ySRUsRXHIpsSLWwge9RnIyTQGXPUikZhzhjUotjiTjNJuA6c00bSMkk/jTlZVHAosIRgzDJpQAp4od8";
        os << "pxTAM9elMTElYtwOacg2sMnkilbhQFx9aVVJUBsZHQigB4wqgEHOaYcqWJPWnP5gxyDTNhJO48egoGy";
        os << "JcqCwHHpTvvHcnX0oYPu4HA7UFcnpg+1MkcCcj92M0Nx80h57Ck+YcbzikK4HQk0AByQXPSpT8yqR2q";
        os << "NN+TwMY9aUqU+6fwNAC46jByaJBubAPIFMy7HGQKft2xkL1PUmgZGjFWwRUjYYLTcdQ2PrTSMbcGmIT";
        os << "BDEinIwJGeDSBsHmlVgTyKBDj1JqNqU7QeCRTGIx940IGAUmkPAxnNGRjnNBK44pgMP3qcDwRSOcngU";
        os << "3PNAh6naemRTi2e2KMc8U8rmgaIyeOBTQMipigIpm3BwaAEA7VIPemkYIp3figBrdTQKCMsacBQA2P7";
        os << "xp601QNxqRR3oBCEelIB1px6UgpDE7U3vTh0prDvTEOFBoFFAx3pSUp6D6U3NAC96VxzSDnmnuOKQDB";
        os << "1FK3U0g6ilPU0ACDminJ0zTTxzQAlO9fpTM08dDTAaKQ0tIe9AhvendqQDvTj0oARhmlUetB604UDGv";
        os << "TH+8Klcd6iYDcKBMDSDqKeRTQORQAtMIzT+/NIBkmgCM8ClB45FOK5OBTggAoAYGx2zTWO4+gFShcUz";
        os << "HJzQA0ngCmj71GeaVDg8igQ4cjrj604gikBFJkY60hj1qTuPrUKkY+8aeNpPJJoBDnYA8cmmYJYE09m";
        os << "UHims2TxQBIuFDVG7FmwKUDO7Jp2OgXFADo12nB6kUmOg7ilC7owG6joRTMupxkGkUSj5VYnvUIyAHH";
        os << "SnYL/eP4Uj78jgYxQIVefmQ89xS5OT+7GaaFyOmDS/OeN5xQAnQ7n6+lNbLDcR+FKFweBk+9Kofccjg";
        os << "9qAJBlipB6U44KkAHOaj2EEbTx6U9PMOeQKRSGONzHB5ApsTFeDxUjLhSF6nqTTV5UhsfWgQpAY80KG";
        os << "UZHNMIxjB4p6PheaAQbgevBpcnGaGKsORTSFHIJH40hjhk4NP2Eg+tRKw4yxpxZc9SaBpkcrY759hUO";
        os << "TvBNSSlcDA61ETyDg1SIZKp+8DTlJUYxxUSNhgSKtlQQKTGiuW9qdu9BSkc0mMtg96AGleM0o608jgg";
        os << "0w9RQBYQ5FSEZSo06VKPu0mWiMjnFAp5GSKQ8MKQWFXrzTWPzkU4U1vvUAxewojOHNCmhfvGgCRO+KQ";
        os << "9DSrjHFIe9IoYO5oXoaO5oHSmSM6Gl9KCO9FMQU/uPpUdPY8DHpSAYepoPpSE4NL15pgI/wB2kSnHkY";
        os << "pqjBoBjzSd6cBkH6U2kAGlHvSUoPagBT0pe4pD0o7ikMetK/Q0gpW6GgojP3hmg9aGHzAGhqZIf8tKU";
        os << "01fvCnNQCBOrUozk00cE04Hk0DGP1pr/dNStjbmom6UCZCvU0q53UwfeNSA9DVEDP4uelSjHGKj4J46";
        os << "GgHmgELOflwSQPaq3HPJq9wwOeneqTrhiKEDEXHfNTpjHHSoAD0zUsS5BBamxIlOcDgYoI+XpS7ACMk";
        os << "01gvPJpFCj7pwO1NUDHNKu3GN2KcEBXjBoAjO3b1/WjjHWnuhC8U0H1oENbHHNOQjPU/nSuOAyjNPRh";
        os << "tDMAPSgERueB8x/Omg8H5j1qZyxC4SmhxkhhigGR56cmm5Oepp7MwbAAoLYPzEZpiIyT6t+VLk8/M1P";
        os << "3g84NLuyPlI/GgBmeepoY/L1NOQsWIwKczbuFGaBWIFPI5NS5GOp/Ok5Xll4qQkFNy84pjRE2N3X9aD";
        os << "jC808c5JGKaT0wM0CGnGev60igZpwUk80qIO+KAAgYpmBjinMF6Z70xgo6E0AxR34o9KaAMfepduO/N";
        os << "MAeo2xnqac+QeuaaRk0CHr0GCalFM6EY6U8kCkNCikPUUFgBTd2SKAHnqKKaTkil70DD+Kl700nDGnA";
        os << "0xCdzUidBUQPzGpF9KQ0I1JTm6U0UAFBoHSmk9qBCrSmgUUwFprdaeegx6UwikNjo+lPb7tMXinv0oA";
        os << "YOooPU0DqKD1NMB6fdpsnSnJ0pjc0gGr1p+aYBTx0OfSmCEBprdKWg0CAdKWmqe1KaQC0q0hpV6UDHP";
        os << "0NRHqKe3pUZPzCgGPzzSfxUE00HLCmIfSDqaTqfakBwTSGKOppSaZuwaeGBFAhGqNuh5NSAg0zrnPSm";
        os << "DI1xmnpj3qMDBpyZPegRJ60h+lJtz35oIGOtAx3GOacMVGoUjkmnqFzjNIEI2M0DGev6050HakKkHig";
        os << "A4w3NKpG7r+tAPXIpx4wQM/jQAmeOp/OomPJ5NWAQE3NxmozluVXigbEU/L1NBPPBNSK23hhimuWDAY";
        os << "FIQzceOWoyfVvyqTdgfMR+FJuA7GgYzcc9TTs+5pQ24/KRmhWYtggUABPA+Y9aVD1+Y04uMgKM05CwB";
        os << "ylIaI3Iz1P501cc81MzDaWUAjvTEHBZhigBOMdaBt29f1oJ9KciErzQAjAY4px+6MigoAvOBTW24xuz";
        os << "QMcANvSkGeeOKRQpxyadtBzhjQAx8Y56VAxHYmpJVwAA1REHpmmiWLxx1qzAfl4JI96rouWAq5woGOl";
        os << "JjiNOOc1Geox0oJ55o4B56CgGKx+akfqKcT1NRnqKALC9BT0pi9KlXG2kWgJORih+q0E4IpDyRSAWm/";
        os << "wDLSlFNb71AMcKQfeOKFoX7xFAEqH5RSMaF6UGkUM70DpSdzQOlMkD7U0UpPakoEB604U2nHgDHpQAx";
        os << "6VD8vFNYZNOHAxTBBQKTpSr1oAf3P0ptO7n6UwUhhR3oNJ1oEPY8Cj0pD0FL3FAx46ChqVT0NNkPzDB";
        os << "pFdBsnDig8g0SfeB9qAeDTJ6iL94HtTm4NIPvD6UpOeaAQ00A4NBpPegQ8HKVG3Q05T8uKTrQMrHgmg";
        os << "HjFI/DEUh4x9aozHhTjkgU0Bj3pwI3DNKeTkce1AwO5RzjHrTHXkEU+UjYB3pOoGKQMhx85xzUsS+vN";
        os << "IFxIe9Sowx602CJd3HAAphJOR0pN3vUbN83BpWKbJCDjkZoC4+6cGmhzmnrg8dDQIaznGGowG6U+Vdy";
        os << "8dRUGSrelNaiYsgKcr0pysGZeOgoLbuDTmjKsGT8qAH5LKDu+lMPzbhjp1qQ+yHNRE7d5JwTSGyIEkb";
        os << "R19adgKcAbmoUbVJpcEdOp61RI4eZxyPpmmsA3DDa1ACdNx3euaeQTwfvDoaQEeSAUPX1qXARVx1NNI";
        os << "DJu7inEbo1IPI5oGNz1OelDkIc9iOlKmMZKk5pShJLN+VAEChnbnpUrAKFNNLYPy9aYxyV71RIbjuIW";
        os << "nKvILcmlVdpPrS8A0hidMjpTGzSu3J+tRM3PJpoTY7PByM008jjijOe9LkYpgRt97BpwGcmkYfMKcp+";
        os << "Q0CEUEtx+dOYEck0kZHIPWnjjrg+1A0MYHGcg0g4+lObgnFNHSgBwPQ08VGKkJoYDW6mlFJ3/CgGgBU";
        os << "HzmnrTY+ppydTQCFJpAc0N3puaQx3am96A3HNA60CHCigUGmMd6fSkpccimnrSAXvmlc0lI3WgBR1FB";
        os << "6mgdqD3oAVDzSd80i9aWgApfX6Ug60uOTQA2g0UGmA3vTu1NPWgnjigQ4nGKUGmZpy9qABqY4+YU9+o";
        os << "psnUUAxDQvUUGk7j6UAONNJ6mnA0w9aAEJzSgHGcgUh6Uq8kZoAACehprAg8/nUh56YHtTJCOAOtAAR";
        os << "jBpq/e4pxPyCkUfMaBDhgDnmlJ44GKTNGcd6Bj1zT+uBUCtzwalRuR9aTBMVl5JXg03cdwDVJwTSMu4";
        os << "j1oGOUBgxqJgyNx0oU4J7VIGyfm60CFQhz9B0oz3z1pwQghl/KkfHUKRikULjerZ6ioskgKOvrUo+WN";
        os << "iTyeaaAFTd3NADVAXhRuanHzOeR9M0oUjgfePJNNITpuO71zSENwGOCNrU0kgbT1p+CevUdKRhuUGmB";
        os << "IPl2jHXpT8lVJ3fWowd2wg5I7VKPdDmpKRCxCs3HUU2MF+TUixlmLPTQ23gUxC4C9aVXOMLUWSzetWI";
        os << "l2rz1NDGhhXP3jk0AHHAxStgcdTTGc5pAPDEYHWnluOQDVdW+bk1Ju96Ghpkcq+nFQ/xgVZdhj0qIrm";
        os << "QdqaJYIvJJp/zMOOnrSdAadERsI70AiMgjvTypxwQaBwcnB9qQkbjigBCeMUDkim9c/WlTlgKYi0g4F";
        os << "PJwlMzilY/LioNAJyaBSZ70ooEPXrTG++T2p445pp+8fpQNgOAKI+ZDSk/KKSP7xPtQHUkTvSHoaIz8";
        os << "xyaVj1NIroRjvQp+U0dzSA8GmSJ3opOlKv60CFp3cfSmGn9x9KBjDQfWhutJ1piAYP1pR1oXB5o70gH";
        os << "jv8ASmU5etN7UABpopaOnSmIdjK5NITSj7vvTT1pDJUOVpGHShThaUnigYyT7wpASBSv97ig9KBCnqP";
        os << "pSGlApGPFAxD0o7UHkUnamIUdKTPWikJwaAIW5zxTccVKRnNMI45pkDAOBjtUqAZ60xME1MmfYUMaG/";
        os << "eAXHGaTgHHYU9ioQqDVctlto9aSATcQT70I5HFIAWfipEhBGSxFMQsa7ss3SlJwenFDAxqATkHoabk5";
        os << "5xgUDY7AY8fmKUnGM/gajjYr2yM1Nwy4oAUHOc9aZMOjDvTsEcGgjd7A9aBkPXmpVY8YOajYbW46U7b";
        os << "lgR0oEiUysB05qFhuJ3nA9Km+6obuegqI8MeNzGhAxMgkAZxS5689TinHORkgewqMrnOSevpQJkoZc4";
        os << "wMfSg84PocUKTt+78vSgqAo54z6UDEUhdynIBpAMD5DkelPXPzYYH2NMxnoNrUCFjkPcYpXf5TkjrTe";
        os << "GBOOR1phXDZ7UxiZxTlGMNTcbn9qe3AA7DpmmSIx+Y+tIDuPFNGS5qQYUCgEMwBnNNYZI44od8nHbNK";
        os << "xOTQBG64GR3pucjmpSN3A6DvTGjAHU0CG5zj2qQEYPvUR4IpQcKR+VMB+dnGO9SEDjGDUIIOAakHYcU";
        os << "AB4Bpo6U48daTt0oAOlLnikooAXqTSdqcBSL0NACx8E04U1OppwoGgbpTQaVjxTBQDF/hoVuaM/LSL1";
        os << "oESClpBS0hik9KTvSnrSd6AF4x70jdaOM8UhOTQA4GgmkooGC9aXjHvTQcEUvGeaBB3pQetJ3pR1oAS";
        os << "kNLSGmAxjzxS/w009ad/DQICacvSozT1PFA0KabJ1FONNfqKAY09Kd3FI3QUpFAhM8UnUUUUAIelOHI";
        os << "FJ+FKBnpQA4Ac5IFR53cU4+nFMJAyBQDHHGPpTM4z70E5UD86QZJ4oAM8cU5F3DJ7ULGCOpp4G3g9D3";
        os << "oAFGD04p2M4pFJyM0iNg47ZpDHk4PNOB+YetBwwNRnIcUAx7jOWpuc08cgjsetMxtf2oAmR/lGCOtJJ";
        os << "IewzUYXJz2p/CgHHJ6UigIyPnOPalYhtoGcCkxjqNzfyp7Z+XLAewpCEHGT74oZlzjHH0oCgqeeM+lD";
        os << "E7fu/L0oAaT056HFNyASDnFAXGME9fSnjOeCD7GmCGKNpGw59qmErY6c1EBlhgbWFS/eUt3HUUmNDGY";
        os << "85OKi6c0/bhiT0pqjc3PSgGPhHVj2p5OMY60gG0eoHSjBPFAxoOc4/E0mAp/rT+FWoXYntxmgQ8HJ6c";
        os << "UjrtwV6Gkyc8YwRTlBkUgHgdTQCIncnil3Eke1PeEAZDE1Ech8GmJk/BOOxpfugrjjNRBsNtPbpU6lS";
        os << "gUmpY0xjhQetRkcHPep3B6YBqF8CmgYmOKcvGOKAOKeBjFMCQnpSnpTQcmlqShe1C9KTtSjgUAOHNHc";
        os << "/SkU8U4j1pDGEkgUsecmgdM0qH5jmgQqjrSucLRnikY5U0DGg5oxhcimjrTj9ygSGGnA0nXrRTEKaee";
        os << "30pnanN1/CkMaetBwPrR3obA5pgMQfN8x5p3em4yadQIUHnNHakXtS0AJ3oPWikagBw6cUoFNXoaeKT";
        os << "GgHWjt+NJnDGhTgkUDBvvUH7vNIT81K33aBC5pppR1wBQfagBhoBoPQ0n1piHU1xzTlPFNfqKBiUx/u";
        os << "0pPNIfu4pksYPUHFJvYnmk6GlZcjIoEIX7ChRhqYeafHknmmA5B87AdSanCgLjvURODgU8HOKQ0NmIK";
        os << "Y61AoOcZqZvun060gGPm9aBMMqTjuKUAjpUROXJp0T4PPSnYLlnO9femKcHntwadnbgr0pso5DCpRQk";
        os << "oyMjtSKeMUufl+lI64II6GmIlzll9hTV+6x96QHcgI+8tOjOQSDk9waQCAbVJ/ipABv4+v1obAbuKYg";
        os << "wfpTBk3uDz6UAAkg8g8/SmHp05oYcg569QKAFY5TcetI33QR2pV247k0j9OfwFAhCQHOT1HNRscqOaW";
        os << "QFUJPU01FyRnoOtUgHx9PrSO2en4UpPFIg4JNAg+7n1xSYJOaU8uSegFRu+T7UCAkA470w5Jxmk7048";
        os << "nIpgSIMKAKSTGPekHTrTX6CkA1skj2pCOKXtSFsLjvTAaDzinRs27ApAMU9BjJoEPOe/akyMU1iaTGa";
        os << "BjwaXrzTe1PWgB1N7Uvak7UAC9TTqavWndqQIDSEYFO70jdKBjD0oTk0HtSr1piHilPSkpaBiE4paQ9";
        os << "6KQAcBuKQdTSY+aj1oEPopvalHSgYh6ilGCeab6UuOaBDqAc0lA7UDFpDS0lMCNutA6UrdaQUCHAZFK";
        os << "KF+7S96BiU1uopx6UjdaAE7U6m9qd2NADelNNONN7UCDPHWlGe3emYxSqxoAa7NuwaaTzinuM80wjNA";
        os << "hwHFC5BPvSBsrjuKXqKBkseMe9K4ypBpidDTj060ARjIbGaeCCcd6bznJpO9AE2CDml+9j1xTFbB9jT";
        os << "wNrgjoRSAVGwefxok5H0pHHAYUoPFAxqn5SM1ICC4weg4qJ1wTjoelOjBZQR1FDAlX7pJ70KcJnvQvT";
        os << "j8RQ23HcGpGOIAIA7c/WjPcnn0pijknPToKUdOnNAxCBv5+v0pxG5Qf4qicZP19KeuCe5xQCFb7qn3p";
        os << "2cM3uOaSQ4AJ4PYCmk7UJP3mpANY8Yp0QwMnvTUXJOegpSfl+tMBWOTx34FPzsT3pkQ5LGnZ3ZJ6UDI";
        os << "yCetNyoIHc02R8nimj74NOxNwYHOKnhIC46UwjPzelKv3R6UAtCUqCuO9QOPnAPXNSk4zTAcnBFJDZG";
        os << "wy1AfsRSyZB4pg4piJN7A8UH1JzSAYGTR1NAEifdp/GaYPu4pQeaQ0PQc06mp1NOY8UihCaBSH2pR0F";
        os << "Ahwp2RSDpzQeuCKQwH3aF+9Qv3aFPzUALn5aD1pGPIFKTlxQMQikPTmnN/SmP0FAmA60d6RaWmIXtQT";
        os << "zRSN3oAO9Ncc/KeadTcYNACrS96ADz6UUAKKKUUlIYhpppxpKYhV6Uqn1oH3ab0f60hjs/MaQE0EUE0";
        os << "wHN96muelKTnFRueaSBj16041GDgin5yM0AI3Q0lKRxQRxTExB0FI3WnYppGc0AMNJj5aUjrR0WmIYc";
        os << "DrTkYA9Kj6tzSj0oEOIGeOlMYHOR6U7G4YoAYNhqAIgxp6k4JowA3NOHTigBvLYB4FPNIO9GCeeaAGF";
        os << "GyTSL15qbBGeaTaG4xg07gLE3JU9KcDzg1EeB707JKgjqOKQwOVbFSgblx+RpjfMMipIjg49aGCIeVY";
        os << "np2p4KnkcNTpU/i/OoMZJBo3AezHdycilxncDTF/u08ZLcYBH60xEmDjqNtN2Y24780DH9w5pWJHXqR";
        os << "x7UhjQSAcHij5R82cmmnONtMzjgfnTsIV8v1pyjao/WliBxuH4CiQ8YoAi5Y049lFA+UEnrTWOFz3NM";
        os << "QkjZbA6Ypp/X0pFyW96lAC44yTTEQlTnmlHWnkc9aaVI55oATkGmO1OJJprUAMJNKvTNIeTThTEOXk+";
        os << "1P7U3oBSikMTjPNA5FKwAPFKBxQAdqUdKQ0oHFAIdSDoaB1oB60AC9TT+1NT7xpwpDQ00E8UrdKb1pg";
        os << "FApyj5DTD1oEPU5p1MXrT6BoQ009aeRzTGHNIBKWlA556UpHNABR2oFKaAG0lOA5pCOeOlMBB1pwpqj";
        os << "mngc0AFNY4p1MbrQAhopB1p7D5KBCdBS02nL0oGL2pjdRTzTX+8KAEPQUtIxoPWgQh6UlKRxSUAxDxS";
        os << "cZ4pxHFIoGaAFNMYYNONJ1BoAjb1oBNONMHBoESK1O5J5pgp4JoAO9IFOeKcFJ7GnAc8GgY0frTo2w2";
        os << "D0xTiA2eMEVE3DAd6AJx3U0zlTSg5Xd3FOPzAEdaQxWG5T+lNTKdKeh4xSyg43H8RSGB2n5s4NBJwMn";
        os << "iogc8H86eAcbaLAP2Z3Z7c04g46jZSKSen3gOfekOP7hz+lIY3GNoFIrHdwcClOQ3OCT+lMb+7TESHa";
        os << "OTy1N5Zgevam4AIAqaFD978qWw0BG1cfmaiGWbFTTHJx6VGvyjJoQCk84FNlbkKOlJkhST1PFNHI96E";
        os << "gGN14pQpyDUmAvGMmlwTjmncVhBTORnuKfgjnnFIe31pANYnANNLGpD05puAW4pgCg5yaeoBPPSkIYt";
        os << "haXBUYpAK7AnpTRg9KQ+lHRuKYEmOKBR1WgDpSGPXrSnoaQDGKdikMSlHQUAcUAcUAhwprdaXOBmmE5";
        os << "JoGPQ9aVfvGokPNSg4zSYIaSaUn5hSA0AUwFY+lI3Sk6v9Kcfu0gGCnCkpRTELQRzRSkUhje9IaWgg8";
        os << "elMQ4DtQBzT6Qdqm5VgA5NNxTl6saMc0XCwyg/pS96aetMQ49KQjNL2pSORQAhHSmtxipMcCmyDpQMY";
        os << "Thh701lwOaWUncKkcblFMRED8wx6VIBgU1EAIPtTmPpQCA9KO1N3HNLj1oEL2oxTl+7TTSGQnij+HFD";
        os << "Hk/WjPFUSNVMHOaXAxkUo5NLQFgUjHvSOM49aDwfY0vQ0ARkZpE6Yp7jac9qbjuKAJFAxk/hSE5JpBy";
        os << "PajOAcY6YoGP3bgc/nUG/5x1qTcCp29+vtTVjGfvGhCHhlKcjk+tGFH3fSnGP90cYNRYIbHQ+lAC52t";
        os << "7U9jtIweDUbHK88GlVS/0piLO7cAfzqvIgUnH/AOqptmOjHNM5BO4DB70kUyEnByfzqQlW6nDUOmT8t";
        os << "RupB5BFPclkwDf3xiglVHBy1QAE/wARp6oSOMmiwBnqR19aEUEc/wD66kjjxnd19KHywwo4Hei4CZ28";
        os << "96jB3ZzTvLz1Y5qMqVznpQgHZDt7UrAHGaZnHTrSkE470xDSVVjgc00P8+e1SBOewprIM9aBDuAPc80";
        os << "FvrSZAIDdR3pGP0oGIwGMioWqVjxUdMQ0DFPAwD60mM5pe1Agp3am96eORQMaeRmlXpSN0paAHY5paR";
        os << "SaeKBoTuaTtSnqfpQOwoEEfU05eSaSP7xpyDk0DQjDimgU9qRRzSAUcCo2HOalxTSM8UwGgcU8c00Ut";
        os << "IEPI6VGetSHJxSdKBiAc0rdaWkbrQAelHrQOtB60AC9aQjmlUc0tAEY61IB1pOtKMjNADTxTCOKd9aQ";
        os << "80CGqOc1IeRSAY4p2KYEZFOXpQw5pVoARuCKbJ1FPccimyfeFAMb2pe4oPegdvpQAlFONMbNACN0pF4";
        os << "GaD3oXpQIXtTKkNR96YARkAjrTSM0/sKaRg0hAtTKBjJqGpFPFA0SBvrRwR7imqaXIyQvU96Qxhf589";
        os << "qcCrMMjmhUGetOKc8YNMQ5QBnFJkI3tSAEZxxSE569aQxxO3GKkzu571CFLYxwKk8vHRjmgYjqAOP/A";
        os << "NVGehPX1qRMqMMODRJHnG3r6UgAFWBydrUEP/fGKjZCByCKYwI7miwEoKr0OWqPPOR+dCKSeATUiJg/";
        os << "NQCEjQMRn/8AXVjdtBP5VFySNoGB3p+zPVjmkykRqQxOegpmdze1DKU+lIpwOOaZI/Cn73pQWUIMDke";
        os << "lMwS2Op9KlEf7oZwKQ9yDf85OKn3bQMfnUbRjPWnbgFG7t096GAoOCOvNDAYyPxpucgZx0xQeB7UDGv";
        os << "0xQBijHc05BuOe1MQqDGc9aViMe9L3po5PsKQCYGMmkKZOc0+kNAB/DQOaO1CnkfWgCbFHagU5vu1JQ";
        os << "3tQOlJj0pAxzTAcVyOKjJ+Y59KlU+vSmugLE+1AMYq5HFOHLH2p6DYpqOI/MaAHLzmnAdaIx196djg0";
        os << "hjAMUo6c0oHJpMcUCE/lS00dadQAYpxHIpSOaG6qaVx2GMOTQQelOPenUXCwmOKYTgmnyHAzUSfM/tQ";
        os << "hslToTSk0vA6dMU0cmkMafvGm96c33qTFUQwpSemaQ9MU0nJzQMm4Ipj9eKSNuadJ0J7UD6EL8yCpPa";
        os << "omPzLipO4pkCH74FLTT9+ndaQ0MIznFPxTkXBPrSkZJNA7CDhTTOtOfpimZxTERH7xo7UdzSLyaZIo4";
        os << "pRyaQU7nNIaEbG3FIh6048io2yORQhMlYZBzUajjBpyMDTwu5DjrT2AiBwcUMQSe9DcVJHCW5PAoAhy";
        os << "2BgUKT7VcSKLbyO3XNV2CcBWoTCwbyE56U8bSvqKYcheenrSEbQCMY9KAEuEKD2qSA7k2g/MKHIZeeQ";
        os << "aSNEB7/nRfQOpPgbOmKif7r07IWoXcMSP4e9JDbJVPQnincn6+lQmVTilMo9KdhEoPbilPTiq/mjP3a";
        os << "eJgR0osK48nhiPSkBHlrjpikWVQOaj3AcZ47UDJdoZemT61EwwmCfmoBH1+lOWNSMnOfrTDchRCT7dz";
        os << "UzEKqgcUMQvTgVGfmwT0piGFiTxTQzZ7VKi7n46U6MJuO5u9Akivzkk+tGeeOKtmOPHQ/nVeSIjleRR";
        os << "cLEZ5po68U8dMClCbQd1MQ3HFJ2NOOMUw0MBR1FPIximDqKlHJoBDTnFIKXqaXGMUDAdafTTwRS0AI3";
        os << "WlSk6tzSr1xQAJ941ItRrwxqROgoBCGhRzQ1A60hi0nenCkIzQA00macOlNPBoEPDYxSZ5pp569qU0A";
        os << "PHWlccU1Oae/IoGMXqKG6mhetDdaYDkHFIetOTgUx+KQCZ5pS2c00Ug46d6Yhc0Ug5NOPSkAd6WgDFK";
        os << "aBjWHNAoPWhaAFamP8AeFPfoajblhTEwekXrTm64pvRuKAHUw9adSDkmgBhpR0pcZzR0NAABnNRnqal";
        os << "6Goj1NAmKOgpe1NFOHSgBh6804cU8puHy009MGgA3ZPNLzkY9afHETy3AqcRx46H86Vx2KpZs9qcGIP";
        os << "NSyBNw2tSOu1+aLhYepDKwPNROhB9u1KPlyR0qRSG69DQMaoymAfmqXaFXpj3pjRqBkZz9aaSPp9aQy";
        os << "Un92wPTFOz8qk+lQ7geM8d6e0qkDH0pCJF6c0hPbiozMAOlN80Z+7RYLk3I/wprHqRzTBKPSkEqjNFh";
        os << "j0+6lS4Gzpn1+tVkcKQP4e1TEhqGNDJzhME/MajgQsPanyIhPfP1pVIVeOlHQXUU7QvoKZvJTjpSAbg";
        os << "ScY9KUcrx09aAI2J9qMnHIqRQnRmqy8UW3gduuaGwSKikAjtQTk4qSSEryORUa80ADdMCpAMAYpWXag";
        os << "z1pjsBRuAj9qVR8tMXJ5NSDpQwQh60hp3OaaaBsO1A+8KG60dxQST9MU48qKjzmnoeMUi0JimgYxmpc";
        os << "YINI65IpDsNpB98il6U0ffoAf7VGnEhp/c1Gp+Zs0xEye9P4ApsXIHpSSNzSKFGDnFN6g00HBzTweMU";
        os << "CG96cPvCjHFC/eoBDwaRxwKDxTuD16YqSiNTk+9PxmoX+V/apYzkZpsSGsdwwKVF201VO2nZOQPagBH";
        os << "YhuPSlQ5Gaa4zg05RgYo6BrcRvvUdjQfvZoPSgQ2TgUlEvX8KRRlvaqESRjPNSyDKH8KYTgcUxz8nvS";
        os << "K6EYHzZpc804DgmmZyaZIp+/Uwx1FQk/vKcG54pDQ9T8zUmetJGwO40Z5NADZDyKZ/FSv1pBTJGjjNM";
        os << "FSMMA01etMkB1p1IBzTsUikB65phqRhxTCOKAYwDBJqWJsA5pq9TSEcUxEsiqSrHAA6+9KX3EBeTVbd";
        os << "nvU9vwpI65pWGmSeW2Mv932prKnl8AU+PLKTuwMVHtXyT2INIZCxKZXPB6UP8m0dj0psowc9frTpRmJ";
        os << "cVZIzecYGKVWJIGaIk/vHAqxGiK4OM/WjQEiAgsOMmlELnPFW+BwOlRsaVwsQ+ScAkgU7yOR8/6UMcd";
        os << "6k3fd5FF2AwwD++fyoEAx9/8ASpc00Nx1ouwsRNEQCdwNNEbEfdqRjnp0p69KLhYqOpXOQQacrMO9Wn";
        os << "y3BpDGp68H2p3FYqOxJwaco3kDsOtOki67TmliG2PB65pgKo3SBc4ApyBOhApsa5kz0oQKQc5JzSAXY";
        os << "QTs+7703dtJDdaccjByTTZsHb6g0ACIF3Occ9BUcnNPJ4xmoj94U0JjG6001I4phHJpiF9KeOKYOcU7";
        os << "PFAAp+Y0vcU1etSdxQMa33lp1D9VpSKQxBy1Hegff46Ud6Yhehz71Kv3RUZGVFOQjbSGgNJihjgZpBy";
        os << "aAFU9qWmilU5BFAgFI4yKU9RQaAGU7qKSjkZxQBIlObpUan1px6UxiDqKD1NA6ig9TQA9elNegdKax9";
        os << "KAE6Cm0vJxmigQqDApTQKB1NAC0jHtQxwAKQ0gFpRTTwaVTkZoGPboai6nPvT3Py+9NAwppgxO9IfvU";
        os << "d6D97mgQUi9WpwFInVqBidzSE/MKd3NRt1oExx5NMx1p9MPegQ0U5aQDkU9RQA+PipHQNhxjjqKiHU1";
        os << "KDxjNJjQbtxAHWn7CSN/3famw4Ab1JpwycnOKQwcJ0AGKaw2yFc5BocLgYyDmiRcSZoAY42EjselMRi";
        os << "DgVLKN0eB1zSRxdNxxTAazMe9NRS2MAmrYjUdOT70qfLwKVwsVzGwH3aesRIBLAVI3SmqcDnpRcBDAM";
        os << "ff/SgW4z98/lTy3HWnZ5pXY7EPkcn5/wBKb5JwTkGpt33uRUanPei7AYYX44pApA5yKsKal4PB6UXBI";
        os << "osxBxmjccYNWJERnJAx9KglTrtORTuDQqfPkdh1oU78L2HWiIYibNJEMnPT6UAWFVPL5Ap3ltjKfd96";
        os << "ZtXyR3JNSSZUKd2RipKQ0PtJDcH0psaqCzDBB6e1JccqCeuag3Ed6LCuTSnIGKiIyQacBxSt1FMQgp/";
        os << "vTQOKeo4pDQ0009afimkc0wGGnnnFIw5pyjIFBIv8VPjPJphpU60iiX0pWPzLTc8iiQgbDSKHtjvUQ/";
        os << "1lKW5poP7ygGGeaQj5s0ZwaeR8oNMRPGMIPxqKQYyaSM/J70/ORz1pFdCKli5GKa4w3tSxdfwpkj+1C";
        os << "/epR0yaQfezUjFc4GaRGLNSsMjFIgxk0dB63Fdd1Ip2jBpcnJHtTWU7aAF3cdc0m77vY0oXahBFRScE";
        os << "UDZMDwKU4FQLnbntT92Ac0gHAZzSE9KdjA4ppzxTQmI/elGM0jUhpkjs5pO1C09VyKBoFHynNQsuOnS";
        os << "pz3ph6igGQsf3lSLyfrSkDeeKVTg8UxDYhy+aXvQvek70gGP1pB7Up9aQUxCydDSfxAU5uTigjnNACd";
        os << "+aUdfajHTmlXmgBT05prD5aceQaQjK80AMIwQRSt0pxHA9qRjnFAFfGDmpVOE4NMKkE5H405VJHt61R";
        os << "KLEQAUcnkfnQMGMjotEZwtNJyh9Kksgl+ZR7UvO1aR+acPuD61RILjOKepx1plOQE0holFNbFIGwMUj";
        os << "dMikAjDIAo6YFLn7uaawOaYh3frTBk5ApQCaVBzTARRxTxTV6Gl7mkArHningbqYMcUAEMeOtMBrjni";
        os << "j+FaUj0oOdoxQwGqcOPc0D07ZzQByKQcE4FADiATxnHamnnrTifTimY546UCEHQ0xhUhHFRvwKaAQjN";
        os << "NxljTv4aTHNMQIOlIaE4NDUACYyKk71EpwalHNAIHH3aVuvNKw6U1up+lIYi/eFLikT7w+tOxyaAD0p";
        os << "elAHIFO24HIoAawzTejU9v6Uz+KgAB9aRaUc0qgAe9AhTRQaQUADetIOaceaTbzxQAo96XoKQqSvvS9";
        os << "qYxF6ihuCaF60N1oAXqKQ+1L2pApC+9ADTSr60beeaUcUCCgUhpRSAY3WlJpWAI96Q8UwDq1OUYpp+9";
        os << "T1/pQMTrSetSbcjgUwjkikA3FI33qdjkU1/vGmA5etIg+9QvUfSnKOtIBveo2xk1IeKiY5NNCYo6Urj";
        os << "rSLQ/JoACPmFOxikxzS/w0AKtSHpUacipMcUhijjpTgADzk+tMxzzTwfXmkAH07ZzQxy59jTW5IyKUj";
        os << "k0DF/hNCDnmlGdpzQB60ICQjbTFJBOaCCWHHSg45oADTWFL3FI3QUANORgGn/jSOOaQgg0AKecihQQC";
        os << "KRRzTs/eoAcKcajXpk0pbIxSGhGOelMbGcU5wRTTQDE52tRF8qn3pT9w/WmpxTEWTgRgdVolAKnk8Di";
        os << "mg4UccU6U5WpKIGOU5NRYyakZSB7etNCkkYH41RBKvSkAySTTlOAaUDg+9SUIo+WnDpxSAYXilHAFAD";
        os << "T19qTvxTm4pMdeaAG/xEUsfQUoHOaF4OKAGn3pU60jUo9aYiQ9aSYZKYo705u1Ioa3B+lRqf3lTMcnm";
        os << "kUDeOKAGKuevSpnHyjFNHU08dqQ0R9qXOPypzLgUxu9AMU4J5pE7ZpBSrQJCg8GlIxikHenYz1pMpCj";
        os << "FITwaZnIGKY2cZ7UASbvvdzS7vl64qOPnNSFdyAAc0DQpweQTg0x+Rj2oX7tIeppAxOen4UEdetDdV+";
        os << "tA70CJA2V60054pE+5Snt9aaExGGTxQxoPemv/DVCY9Bn8qmB+X0FRx9B9KVvu0ikGcZpufmFJQe1BL";
        os << "EP3j9aUcNmhvvGjvTAF/ipD1NKvU0h70g6DT0po5NL2agfe/CmSOPDGl7g00dfxpf4qBh2FC9RQPvGn";
        os << "CgEHWjjBHrSjtSHt9aQwOB1prYCjFK3b60H+GmIjH6U/nBI/KmjofrT160APXheahJyhFO7H6Uz+CmA";
        os << "w09BmOmHpU0f3R9KGJDSME+lCcd6c3Q01eooGB65FBGBigdfxNK33vwoENB6U5uV96RfvClbrTAEGBS";
        os << "nrQtI33j9KAEXpR3J9qUdKT1oAOpHNScdM0wU7u1AIaOppM8CnNUb9FoY0BPIpw71D3FSR0hIfxnFN7";
        os << "E0dz9aH6iqARutRucdakP8VRt0oENyKUDk0g608daBEY9KaeTT/wCOmimAKMZqRB0qPvUi9qGIe38Oa";
        os << "Rx81OftSN96pGNXqKUn5jQP60nemAvUjtUmRtHOai9KcPu0DEPemDlqc3f6UxfvUASKMUetC9aU9TQI";
        os << "KbS0npQMcaKRulKtAC8Bc0nbNO/5Z03+EUAxFPz0MfnpB94UH7xpgO7ZpeCuaT+E07/lnSAbRQ1C0AJ";
        os << "S031paBB6UMM0vcUN1oGRnhqeO1Mb71OXt9KAJcjaecVH0J70p+7SetAAD8wobqaB1oPX8aBAg+alX+";
        os << "LFC/epU70ARuOtRsM4qRu9R96aAAMU40007+OgBccikyKeetMPWkA5DmpF61GvQ1IP4aBi9gadxnFNX";
        os << "qaO4+tA0KaaDyRRJUfc0gJs8GlPUUxejVItCGx3HTNR9Ceaf3Wm/wD16BMO4+lDdKT0pT0oAUdaRxkU";
        os << "L94fSlagBF+770hPWlXrSN940AAGRigdcmlX734Uh60gB+e9AGT7UN1NOXoKBjX4j/GoxU0n3T9KhXp";
        os << "TQmSA4QCpm5XioP4Kk7D6Uhic4BP5U09fant1ph6D60AOXBU5604YPSkH8VCdT9aQC8YA9KOlA7/WlP";
        os << "egYxupo7GnGmn7y0ALjkmk6sKP4qQ9fxoENPFOHSkP3vwoHRaYh46ilb+GkHalbqKRSA8tmkH3h9aU9";
        os << "TQv3hQAvVjTs5xTB1NFIETE/L1zULjH5VIPu0kn3T9KCiNTQvB5pE70o7UEijPPNOJwvWm+v1pH+5SG";
        os << "hMdKOen1FKe1IvVvrQMcnAx7U8YHJPFMHUfShvu0DR//2Q==";
        os << ") -440px -10px; }\n"
           << "#logo {display: block;position: absolute;top: 7px;left: 15px;width: 375px;height: 153px;background: ";
        print_simc_logo( os );
        os << " 0px 0px no-repeat; }\n"
           << "#masthead h2 {margin-left: 355px;color: 268f45; }\n"
           << "#masthead ul.params {margin: 20px 0 0 345px; }\n"
           << "#masthead p {color: #fff;margin: 20px 20px 0 20px; }\n"
           << "#notice {font-size: 12px;-moz-box-shadow: 0px 0px 8px #E41B17;-webkit-box-shadow: 0px 0px 8px #E41B17;box-shadow: 0px 0px 8px #E41B17; }\n"
           << ".alert {width: 800px;padding: 10px;margin: 10px 0 10px 0;background-color: #333;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: inset 0px 0px 6px #C11B17;-webkit-box-shadow: inset 0px 0px 6px #C11B17;box-shadow: inset 0px 0px 6px #C11B17; }\n"
           << ".alert p {margin-bottom: 0px; }\n"
           << ".section .toggle-content {padding-left: 18px; }\n"
           << ".player > .toggle-content {padding-left: 0; }\n"
           << ".toc {float: left;padding: 0; }\n"
           << ".toc-wide {width: 560px; }\n"
           << ".toc-narrow {width: 375px; }\n"
           << ".toc li {margin-bottom: 10px;list-style-type: none; }\n"
           << ".toc li ul {padding-left: 10px; }\n"
           << ".toc li ul li {margin: 0;list-style-type: none;font-size: 13px; }\n"
           << "#raid-summary .toggle-content {padding-bottom: 0px; }\n"
           << "#raid-summary ul.params,#raid-summary ul.params li:first-child {margin-left: 0; }\n"
           << ".charts {float: left;width: 541px;margin-top: 10px; }\n"
           << ".charts-left {margin-right: 40px; }\n"
           << ".charts img {background-color: #333;padding: 5px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
           << ".talents div.float {width: auto;margin-right: 50px; }\n"
           << "table.sc {background-color: #333;padding: 4px 2px 2px 2px;margin: 10px 0 20px 0;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
           << "table.sc tr {color: #fff;background-color: #1a1a1a; }\n"
           << "table.sc tr.head {background-color: #aaa;color: #fff; }\n"
           << "table.sc tr.odd {background-color: #222; }\n"
           << "table.sc th {padding: 2px 4px 4px 4px;text-align: center;background-color: #333;color: #fff; }\n"
           << "table.sc td {padding: 2px;text-align: center;font-size: 13px; }\n"
           << "table.sc th.left, table.sc td.left, table.sc tr.left th, table.sc tr.left td {text-align: left; vertical-align: top; }\n"
           << "table.sc th.right, table.sc td.right, table.sc tr.right th, table.sc tr.right td {text-align: right;padding-right: 4px; vertical-align: top; }\n"
           << "table.sc th.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
           << "table.sc td.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
           << "table.sc tr.details td {padding: 0 0 15px 15px;text-align: left;background-color: #333;font-size: 11px; }\n"
           << "table.sc tr.details td ul {padding: 0;margin: 4px 0 8px 0; }\n"
           << "table.sc tr.details td ul li {clear: both;padding: 2px;list-style-type: none; }\n"
           << "table.sc tr.details td ul li span.label {display: block;padding: 2px;float: left;width: 145px;margin-right: 4px;background: #222; }\n"
           << "table.sc tr.details td ul li span.tooltip {display: block;float: left;width: 190px; }\n"
           << "table.sc tr.details td ul li span.tooltip-wider {display: block;float: left;width: 350px; }\n"
           << "table.sc tr.details td div.float {width: 350px; }\n"
           << "table.sc tr.details td div.float h5 {margin-top: 4px; }\n"
           << "table.sc tr.details td div.float ul {margin: 0 0 12px 0; }\n"
           << "table.sc td.filler {background-color: #333; }\n"
           << "table.sc .dynamic-buffs tr.details td ul li span.label {width: 120px; }\n"
           << "tr.details td table.details {padding: 0px;margin: 5px 0 10px 0; }\n"
           << "tr.details td table.details tr th {background-color: #222; }\n"
           << "tr.details td table.details tr td {background-color: #2d2d2d; }\n"
           << "tr.details td table.details tr.odd td {background-color: #292929; }\n"
           << "tr.details td table.details tr td {padding: 1px 3px 1px 3px; }\n"
           << "tr.details td table.details tr td.right {text-align: right; }\n"
           << ".player-thumbnail {float: right;margin: 8px;border-radius: 12px;-moz-border-radius: 12px;-khtml-border-radius: 12px;-webkit-border-radius: 12px; }\n"
           << "</style>\n";
  }
};

struct white_html_style_t : public html_style_t
{
  void print_html_styles( std::ostream& os) override
  {
    os << "<style type=\"text/css\" media=\"all\">\n"
           << "* {border: none;margin: 0;padding: 0; }\n"
           << "body {padding: 5px 25px 25px 25px;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 14px;background-color: #f9f9f9;color: #333;text-align: center; }\n"
           << "p {margin: 1em 0 1em 0; }\n"
           << "h1, h2, h3, h4, h5, h6 {width: auto;color: #777;margin-top: 1em;margin-bottom: 0.5em; }\n"
           << "h1, h2 {margin: 0;padding: 2px 2px 0 2px; }\n"
           << "h1 {font-size: 24px; }\n"
           << "h2 {font-size: 18px; }\n"
           << "h3 {margin: 0 0 4px 0;font-size: 16px; }\n"
           << "h4 {font-size: 12px; }\n"
           << "h5 {font-size: 10px; }\n"
           << "a {color: #666688;text-decoration: none; }\n"
           << "a:hover, a:active {color: #333; }\n"
           << "ul, ol {padding-left: 20px; }\n"
           << "ul.float, ol.float {padding: 0;margin: 0; }\n"
           << "ul.float li, ol.float li {display: inline;float: left;padding-right: 6px;margin-right: 6px;list-style-type: none;border-right: 2px solid #eee; }\n"
           << ".clear {clear: both; }\n"
           << ".hide, .charts span {display: none; }\n"
           << ".center {text-align: center; }\n"
           << ".float {float: left; }\n"
           << ".mt {margin-top: 20px; }\n"
           << ".mb {margin-bottom: 20px; }\n"
           << ".force-wrap {word-wrap: break-word; }\n"
           << ".mono {font-family: \"Lucida Console\", Monaco, monospace;font-size: 12px; }\n"
           << ".toggle {cursor: pointer; }\n"
           << "h2.toggle {padding-left: 16px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAeCAIAAACT/LgdAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD1SURBVHja7JQ9CoQwFIT9LURQG3vBwyh4XsUjWFtb2IqNCmIhkp1dd9dsfIkeYKdKHl+G5CUTvaqqrutM09Tk2rYtiiIrjuOmaeZ5VqBBEADVGWPTNJVlOQwDyYVhmKap4zgGJp7nJUmCpQoOY2Mv+b6PkkDz3IGevQUOeu6VdxrHsSgK27azLOM5AoVwPqCu6wp1ApXJ0G7rjx5oXdd4YrfQtm3xFJdluUYRBFypghb32ve9jCaOJaPpDpC0tFmg8zzn46nq6/rSd2opAo38IHMXrmeOdgWHACKVFx3Y/c7cjys+JkSP9HuLfYR/Dg1icj0EGACcXZ/44V8+SgAAAABJRU5ErkJggg==) 0 -10px no-repeat; }\n"
           << "h2.open {margin-bottom: 10px;background-position: 0 9px; }\n"
           << "h3.toggle {padding-left: 16px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAaCAIAAAAMmCo2AAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAEfSURBVHjazJPLjkRAGIXbdSM8ACISvWeDNRYeGuteuL2EdMSGWLrOmdExaCO9nLOq+vPV+S9VRTwej6IoGIYhCOK21zzPfd/f73da07TiRxRFbTkQ4zjKsqyqKoFN27ZhGD6fT5ZlV2IYBkVRXNflOI5ESBAEz/NEUYT5lnAcBwQi307L6aZpoiiqqgprSZJwbCF2EFTXdRAENE37vr8SR2jhAPE8vw0eoVORtw/0j6Fpmi7afEFlWeZ5jhu9grqui+M4SZIrCO8Eg86y7JT7LXx5TODSNL3qDhw6eOeOIyBJEuUj6ZY7mRNmAUvQa4Q+EEiHJizLMgzj3AkeMLBte0vsoCULPHRd//NaUK9pmu/EywDCv0M7+CTzmb4EGADS4Lwj+N6gZgAAAABJRU5ErkJggg==) 0 -11px no-repeat; }\n"
           << "h3.open {background-position: 0 7px; }\n"
           << "h4.toggle {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAVCAIAAADw0OikAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD8SURBVHjavJHLjkRAGIUbRaxd3oAQ8QouifDSFmysPICNIBZ2EhuJuM6ZMdFR3T3LOYtKqk79/3/qKybLsrZteZ5/3DXPs67rxLbtvu+bprluHMexrqumaZZlMdhM05SmaVVVhBBst20zDMN1XRR822erJEnKsmQYxjRNz/M4jsM5ORsKguD7/r7vqHAc5/Sg3+orDsuyGHGd3OxXsY8/9R92XdfjOH60i6IAODzsvQ0sgApw1I0nAZACVGAAPlEU6WigDaLoEcfxleNN8mEY8Id0c2hZFlmWgyDASlefXhiGqqrS0eApihJFkSRJt0nHj/I877rueNGXAAMAKcaTc/aCM/4AAAAASUVORK5CYII=) 0 -8px no-repeat; }\n"
           << "h4.open {background-position: 0 6px; }\n"
           << "a.toggle-details, span.toggle-details {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAkAAAAXCAYAAADZTWX7AAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAADiSURBVHjaYvz//z/DrFmzGBkYGLqBeG5aWtp1BjTACFIEAkCFZ4AUNxC7ARU+RlbEhMT+BMQaQLwOqEESlyIYMIEqlMenCAQsgLiakKILQNwF47AgSfyH0leA2B/o+EfYTOID4gdA7IusAK4IGk7ngNgPqOABut3I1uUDFfzA5kB4YOIDTAxEgOGtiAUY2vlA2hCIf2KRZwXie6AQPwzEFUAsgUURSGMQEzAqQHFmB8R30BS8BWJXoPw2sJuAjNug2Afi+1AFH4A4DCh+GMXhQIEboHQExKeAOAbI3weTAwgwAIZTQ9CyDvuYAAAAAElFTkSuQmCC) 0 4px no-repeat; }\n"
           << "a.open, span.open {background-position: 0 -11px; }\n"
           << "td.small a.toggle-details, span.toggle-details {background-position: 0 2px; }\n"
           << "td.small a.open, span.open {background-position: 0 -13px; }\n"
           << "#active-help, .help-box {display: none; }\n"
           << "#active-help {position: absolute;width: 350px;padding: 3px;background: #fff;z-index: 10; }\n"
           << "#active-help-dynamic {padding: 6px 6px 18px 6px;background: #eeeef5;outline: 1px solid #ddd;font-size: 13px; }\n"
           << "#active-help .close {position: absolute;right: 10px;bottom: 4px; }\n"
           << ".help-box h3 {margin: 0 0 5px 0;font-size: 16px; }\n"
           << ".help-box {border: 1px solid #ccc;background-color: #fff;padding: 10px; }\n"
           << "a.help {cursor: help; }\n"
           << ".section {position: relative;width: 1200px;padding: 8px;margin-left: auto;margin-right: auto;margin-bottom: -1px;border: 1px solid #ccc;background-color: #fff;-moz-box-shadow: 4px 4px 4px #bbb;-webkit-box-shadow: 4px 4px 4px #bbb;box-shadow: 4px 4px 4px #bbb;text-align: left; }\n"
           << ".section-open {margin-top: 25px;margin-bottom: 35px;-moz-border-radius: 15px;-khtml-border-radius: 15px;-webkit-border-radius: 15px;border-radius: 15px; }\n"
           << ".grouped-first {-moz-border-radius-topright: 15px;-moz-border-radius-topleft: 15px;-khtml-border-top-right-radius: 15px;-khtml-border-top-left-radius: 15px;-webkit-border-top-right-radius: 15px;-webkit-border-top-left-radius: 15px;border-top-right-radius: 15px;border-top-left-radius: 15px; }\n"
           << ".grouped-last {-moz-border-radius-bottomright: 15px;-moz-border-radius-bottomleft: 15px;-khtml-border-bottom-right-radius: 15px;-khtml-border-bottom-left-radius: 15px;-webkit-border-bottom-right-radius: 15px;-webkit-border-bottom-left-radius: 15px;border-bottom-right-radius: 15px;border-bottom-left-radius: 15px; }\n"
           << ".section .toggle-content {padding: 0; }\n"
           << ".player-section .toggle-content {padding-left: 16px;margin-bottom: 20px; }\n"
           << ".subsection {background-color: #ccc;width: 1000px;padding: 8px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;font-size: 12px; }\n"
           << ".subsection-small {width: 500px; }\n"
           << ".subsection h4 {margin: 0 0 10px 0; }\n"
           << ".profile .subsection p {margin: 0; }\n"
           << "#raid-summary .toggle-content {padding-bottom: 0px; }\n"
           << "ul.params {padding: 0;margin: 4px 0 0 6px; }\n"
           << "ul.params li {float: left;padding: 2px 10px 2px 10px;margin-left: 10px;list-style-type: none;background: #eeeef5;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 11px; }\n"
           << "#masthead ul.params {margin-left: 4px; }\n"
           << "#masthead ul.params li {margin-left: 0px;margin-right: 10px; }\n"
           << ".player h2 {margin: 0; }\n"
           << ".player ul.params {position: relative;top: 2px; }\n"
           << "#masthead h2 {margin: 10px 0 5px 0; }\n"
           << "#notice {border: 1px solid #ddbbbb;background: #ffdddd;font-size: 12px; }\n"
           << "#notice h2 {margin-bottom: 10px; }\n"
           << ".alert {width: 800px;padding: 10px;margin: 10px 0 10px 0;background-color: #ddd;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
           << ".alert p {margin-bottom: 0px; }\n"
           << ".section .toggle-content {padding-left: 18px; }\n"
           << ".player > .toggle-content {padding-left: 0; }\n"
           << ".toc {float: left;padding: 0; }\n"
           << ".toc-wide {width: 560px; }\n"
           << ".toc-narrow {width: 375px; }\n"
           << ".toc li {margin-bottom: 10px;list-style-type: none; }\n"
           << ".toc li ul {padding-left: 10px; }\n"
           << ".toc li ul li {margin: 0;list-style-type: none;font-size: 13px; }\n"
           << ".charts {float: left;width: 541px;margin-top: 10px; }\n"
           << ".charts-left {margin-right: 40px; }\n"
           << ".charts img {padding: 8px;margin: 0 auto;margin-bottom: 20px;border: 1px solid #ccc;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: inset 1px 1px 4px #ccc;-webkit-box-shadow: inset 1px 1px 4px #ccc;box-shadow: inset 1px 1px 4px #ccc; }\n"
           << ".talents div.float {width: auto;margin-right: 50px; }\n"
           << "table.sc {border: 0;background-color: #eee; }\n"
           << "table.sc tr {background-color: #fff; }\n"
           << "table.sc tr.head {background-color: #aaa;color: #fff; }\n"
           << "table.sc tr.odd {background-color: #f3f3f3; }\n"
           << "table.sc th {padding: 2px 4px 4px 4px;text-align: center;background-color: #aaa;color: #fcfcfc; }\n"
           << "table.sc th.small {padding: 2px 2px;font-size: 12px; }\n"
           << "table.sc th a {color: #fff;text-decoration: underline; }\n"
           << "table.sc th a:hover, table.sc th a:active {color: #f1f1ff; }\n"
           << "table.sc td {padding: 2px;text-align: center;font-size: 13px; }\n"
           << "table.sc th.left, table.sc td.left, table.sc tr.left th, table.sc tr.left td {text-align: left; vertical-align: top; }\n"
           << "table.sc th.right, table.sc td.right, table.sc tr.right th, table.sc tr.right td {text-align: right;padding-right: 4px; vertical-align: top; }\n"
           << "table.sc th.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
           << "table.sc td.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
           << "table.sc tr.details td {padding: 0 0 15px 15px; 0px;margin: 5px 0 10px 0;text-align: left;background-color: #eee;font-size: 11px; }\n"
           << "table.sc tr.details td ul {padding: 0;margin: 4px 0 8px 0; }\n"
           << "table.sc tr.details td ul li {clear: both;padding: 2px;list-style-type: none; }\n"
           << "table.sc tr.details td ul li span.label {display: block;padding: 2px;float: left;width: 145px;margin-right: 4px;background: #f3f3f3; }\n"
           << "table.sc tr.details td ul li span.tooltip {display: block;float: left;width: 190px; }\n"
           << "table.sc tr.details td ul li span.tooltip-wider {display: block;float: left;width: 350px; }\n"
           << "table.sc tr.details td div.float {width: 350px; }\n"
           << "table.sc tr.details td div.float h5 {margin-top: 4px; }\n"
           << "table.sc tr.details td div.float ul {margin: 0 0 12px 0; }\n"
           << "table.sc td.filler {background-color: #ccc; }\n"
           << "table.sc .dynamic-buffs tr.details td ul li span.label {width: 120px; }\n"
           << "tr.details td table.details {padding: 0px;margin: 5px 0 10px 0; background-color: #eee;}\n"
           << "tr.details td table.details tr.odd td {background-color: #f3f3f3; }\n"
           << "tr.details td table.details tr td {padding: 0 0 15px 15px; }\n"
           << "tr.details td table.details tr td.right {text-align: right; }\n"
           << ".player-thumbnail {float: right;margin: 8px;border-radius: 12px;-moz-border-radius: 12px;-webkit-border-radius: 12px;-khtml-border-radius: 12px; }\n"
           << "</style>\n";
  }
};

struct classic_html_style_t : public html_style_t
{
  void print_html_styles( std::ostream& os ) override
  {
    os << "<style type=\"text/css\" media=\"all\">\n"
           << "* {border: none;margin: 0;padding: 0; }\n"
           << "body {padding: 5px 25px 25px 25px;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 14px;background: #261307;color: #FFF;text-align: center; }\n"
           << "p {margin: 1em 0 1em 0; }\n"
           << "h1, h2, h3, h4, h5, h6 {width: auto;color: #FDD017;margin-top: 1em;margin-bottom: 0.5em; }\n"
           << "h1, h2 {margin: 0;padding: 2px 2px 0 2px; }\n"
           << "h1 {font-size: 28px;text-shadow: 0 0 3px #FDD017; }\n"
           << "h2 {font-size: 18px; }\n"
           << "h3 {margin: 0 0 4px 0;font-size: 16px; }\n"
           << "h4 {font-size: 12px; }\n"
           << "h5 {font-size: 10px; }\n"
           << "a {color: #FDD017;text-decoration: none; }\n"
           << "a:hover, a:active {text-shadow: 0 0 1px #FDD017; }\n"
           << "ul, ol {padding-left: 20px; }\n"
           << "ul.float, ol.float {padding: 0;margin: 0; }\n"
           << "ul.float li, ol.float li {display: inline;float: left;padding-right: 6px;margin-right: 6px;list-style-type: none;border-right: 2px solid #333; }\n"
           << ".clear {clear: both; }\n"
           << ".hide, .charts span {display: none; }\n"
           << ".center {text-align: center; }\n"
           << ".float {float: left; }\n"
           << ".mt {margin-top: 20px; }\n"
           << ".mb {margin-bottom: 20px; }\n"
           << ".force-wrap {word-wrap: break-word; }\n"
           << ".mono {font-family: \"Lucida Console\", Monaco, monospace;font-size: 12px; }\n"
           << ".toggle {cursor: pointer; }\n"
           << "h2.toggle {padding-left: 18px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAeCAIAAACT/LgdAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAFaSURBVHjaYoz24a9N51aVZ2PADT5//VPS+5WRk51RVZ55STu/tjILVnV//jLEVn1cv/cHMzsb45OX/+48/muizSoiyISm7vvP/yn1n1bs+AE0kYGbkxEiaqDOcn+HyN8L4nD09aRYhCcHRBakDK4UCKwNWM+sEIao+34aoQ6LUiCwMWR9sEMETR12pUBgqs0a5MKOJohdKVYAVMbEQDQYVUq6UhlxZmACIBwNQNJCj/XVQVFjLVbCsfXrN4MwP9O6fn4jTVai3Ap0xtp+fhMcZqN7S06CeU0fPzBxERUCshLM6ycKmOmwEhVYkiJMa/oE0HyJM1zffvj38u0/wkq3H/kZU/nxycu/yIJY8v65678LOj8DszsBt+4+/iuo8COmOnSlh87+Ku///PjFXwIRe2qZkKggE56IZebnZfn56x8nO9P5m/+u3vkNLHBYWdARExMjNxczQIABACK8cxwggQ+oAAAAAElFTkSuQmCC) 0 -10px no-repeat; }\n"
           << "h2.toggle:hover {text-shadow: 0 0 2px #FDD017; }\n"
           << "h2.open {margin-bottom: 10px;background-position: 0 9px; }\n"
           << "#home-toc h2.open {margin-top: 20px; }\n"
           << "h3.toggle {padding-left: 16px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAaCAYAAACD+r1hAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD/SURBVHjaYvx7QdyTgYGhE4iVgfg3A3bACsRvgDic8f///wz/Lkq4ADkrgVgIh4bvIMVM+i82M4F4QMYeIBUAxE+wKP4IxCEgxWC1MFGgwGEglQnEj5EUfwbiaKDcNpgA2EnIAOg8VyC1Cog5gDgMZjJODVBNID9xABVvQZdjweHJO9CQwQBYbcAHmBhIBMNBAwta+MtgSx7A+MBpgw6pTloKxBGkaOAB4vlAHEyshu/QRLcQlyZ0DYxQmhuIFwNxICnBygnEy4DYg5R4AOW2D8RqACXxMCA+QYyG20CcAcSHCGUgTmhxEgPEp4gJpetQZ5wiNh7KgXg/vlAACDAAkUxCv8kbXs4AAAAASUVORK5CYII=) 0 -11px no-repeat; }\n"
           << "h3.toggle:hover {text-shadow: 0 0 2px #CDB007; }\n"
           << "h3.open {background-position: 0 7px; }\n"
           << "h4.toggle {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAVCAIAAADw0OikAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAD8SURBVHjavJHLjkRAGIUbRaxd3oAQ8QouifDSFmysPICNIBZ2EhuJuM6ZMdFR3T3LOYtKqk79/3/qKybLsrZteZ5/3DXPs67rxLbtvu+bprluHMexrqumaZZlMdhM05SmaVVVhBBst20zDMN1XRR822erJEnKsmQYxjRNz/M4jsM5ORsKguD7/r7vqHAc5/Sg3+orDsuyGHGd3OxXsY8/9R92XdfjOH60i6IAODzsvQ0sgApw1I0nAZACVGAAPlEU6WigDaLoEcfxleNN8mEY8Id0c2hZFlmWgyDASlefXhiGqqrS0eApihJFkSRJt0nHj/I877rueNGXAAMAKcaTc/aCM/4AAAAASUVORK5CYII=) 0 -8px no-repeat; }\n"
           << "h4.open {background-position: 0 6px; }\n"
           << "a.toggle-details, span.toggle-details {margin: 0 0 8px 0;padding-left: 12px;background: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAWCAYAAAD5Jg1dAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAADpSURBVHjaYvx7QdyLgYGhH4ilgfgPAypgAuIvQBzD+P//f4Z/FyXCgJzZQMyHpvAvEMcx6b9YBlYIAkDFAUBqKRBzQRX9AuJEkCIwD6QQhoHOCADiX0D8F4hjkeXgJsIA0OQYIMUGNGkesjgLAyY4AsTM6IIYJuICTAxEggFUyIIULIpA6jkQ/0AxSf8FhoneQKxJjNVxQLwFiGUJKfwOxFJAvBmakgh6Rh+INwCxBDG+NoEq1iEmeK4A8Rt8iQIEpgJxPjThYpjIhKSoFFkRukJQQK8D4gpoCDDgSo+Tgfg0NDNhAIAAAwD5YVPrQE/ZlwAAAABJRU5ErkJggg==) 0 -9px no-repeat; }\n"
           << "a.open, span.open {background-position: 0 6px; }\n"
           << "td.small a.toggle-details, span.toggle-details {background-position: 0 -10px; }\n"
           << "td.small a.open, span.open {background-position: 0 5px; }\n"
           << "#active-help, .help-box {display: none;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px; }\n"
           << "#active-help {position: absolute;width: auto;padding: 3px;background: transparent;z-index: 10; }\n"
           << "#active-help-dynamic {max-width: 400px;padding: 8px 8px 20px 8px;background: #333;font-size: 13px;text-align: left;border: 1px solid #222;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: 4px 4px 10px #000;-webkit-box-shadow: 4px 4px 10px #000;box-shadow: 4px 4px 10px #000; }\n"
           << "#active-help .close {display: block;height: 14px;width: 14px;position: absolute;right: 12px;bottom: 7px;background: #000 url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAYAAAAfSC3RAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAE8SURBVHjafNI/KEVhGMfxc4/j33BZjK4MbkmxnEFiQFcZlMEgZTAZDbIYLEaRUMpCuaU7yCCrJINsJFkUNolSBnKJ71O/V69zb576LOe8v/M+73ueVBzH38HfesQ5bhGiFR2o9xdFidAm1nCFop7VoAvTGHILQy9kCw+0W9F7/o4jHPs7uOAyZrCL0aC05rCgd/uu1Rus4g6VKKAa2wrNKziCPTyhx4InClkt4RNbardFoWG3E3WKCwteJ9pawSt28IEcDr33b7gPy9ysVRZf2rWpzPso0j/yax2T6EazzlynTgL9z2ykBe24xAYm0I8zqdJF2cUtog9tFsxgFs8YR68uwFVeLec1DDYEaXe+MZ1pIBFyZe3WarJKRq5CV59Wiy9IoQGDmPpvVq3/Tg34gz5mR2nUUPzWjwADAFypQitBus+8AAAAAElFTkSuQmCC) no-repeat; }\n"
           << "#active-help .close:hover {background-color: #1d1d1d; }\n"
           << ".help-box h3 {margin: 0 0 12px 0;font-size: 14px;color: #C68E17; }\n"
           << ".help-box p {margin: 0 0 10px 0; }\n"
           << ".help-box {background-color: #000;padding: 10px; }\n"
           << "a.help {color: #C68E17;cursor: help; }\n"
           << "a.help:hover {text-shadow: 0 0 1px #C68E17; }\n"
           << ".section {position: relative;width: 1200px;padding: 8px;margin-left: auto;margin-right: auto;margin-bottom: -1px;border: 0;-moz-box-shadow: 0px 0px 8px #FDD017;-webkit-box-shadow: 0px 0px 8px #FDD017;box-shadow: 0px 0px 8px #FDD017;color: #fff;background-color: #000;text-align: left; }\n"
           << ".section-open {margin-top: 25px;margin-bottom: 35px;-moz-border-radius: 15px;-khtml-border-radius: 15px;-webkit-border-radius: 15px;border-radius: 15px; }\n"
           << ".grouped-first {-moz-border-radius-topright: 15px;-moz-border-radius-topleft: 15px;-khtml-border-top-right-radius: 15px;-khtml-border-top-left-radius: 15px;-webkit-border-top-right-radius: 15px;-webkit-border-top-left-radius: 15px;border-top-right-radius: 15px;border-top-left-radius: 15px; }\n"
           << ".grouped-last {-moz-border-radius-bottomright: 15px;-moz-border-radius-bottomleft: 15px;-khtml-border-bottom-right-radius: 15px;-khtml-border-bottom-left-radius: 15px;-webkit-border-bottom-right-radius: 15px;-webkit-border-bottom-left-radius: 15px;border-bottom-right-radius: 15px;border-bottom-left-radius: 15px; }\n"
           << ".section .toggle-content {padding: 0; }\n"
           << ".player-section .toggle-content {padding-left: 16px; }\n"
           << "#home-toc .toggle-content {margin-bottom: 20px; }\n"
           << ".subsection {background-color: #333;width: 1000px;padding: 8px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;font-size: 12px; }\n"
           << ".subsection-small {width: 500px; }\n"
           << ".subsection h4 {margin: 0 0 10px 0;color: #fff; }\n"
           << ".profile .subsection p {margin: 0; }\n"
           << "#raid-summary .toggle-content {padding-bottom: 0px; }\n"
           << "ul.params {padding: 0;margin: 4px 0 0 6px; }\n"
           << "ul.params li {float: left;padding: 2px 10px 2px 10px;margin-left: 10px;list-style-type: none;background: #2f2f2f;color: #ddd;font-family: \"Lucida Grande\", Arial, sans-serif;font-size: 11px;-moz-border-radius: 8px;-khtml-border-radius: 8px;-webkit-border-radius: 8px;border-radius: 8px; }\n"
           << "ul.params li.linked:hover {background: #393939; }\n"
           << "ul.params li a {color: #ddd; }\n"
           << "ul.params li a:hover {text-shadow: none; }\n"
           << ".player h2 {margin: 0; }\n"
           << ".player ul.params {position: relative;top: 2px; }\n"
           << "#masthead {height: auto;padding-bottom: 30px;border: 0;-moz-border-radius: 15px;-khtml-border-radius: 15px;-webkit-border-radius: 15px;border-radius: 15px;-moz-box-shadow: 0px 0px 8px #FDD017;-webkit-box-shadow: 0px 0px 8px #FDD017;box-shadow: 0px 0px 8px #FDD017;text-align: left;color: #FDD017;background: #000 ";
        print_simc_logo( os );
        os <<" 7px 13px no-repeat; }\n"
           << "#masthead h1 {margin: 57px 0 0 355px; }\n"
           << "#home #masthead h1 {margin-top: 98px; }\n"
           << "#masthead h2 {margin-left: 355px; }\n"
           << "#masthead ul.params {margin: 20px 0 0 345px; }\n"
           << "#masthead p {color: #fff;margin: 20px 20px 0 20px; }\n"
           << "#notice {font-size: 12px;-moz-box-shadow: 0px 0px 8px #E41B17;-webkit-box-shadow: 0px 0px 8px #E41B17;box-shadow: 0px 0px 8px #E41B17; }\n"
           << "#notice h2 {margin-bottom: 10px; }\n"
           << ".alert {width: 800px;padding: 10px;margin: 10px 0 10px 0;background-color: #333;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px;-moz-box-shadow: inset 0px 0px 6px #C11B17;-webkit-box-shadow: inset 0px 0px 6px #C11B17;box-shadow: inset 0px 0px 6px #C11B17; }\n"
           << ".alert p {margin-bottom: 0px; }\n"
           << ".section .toggle-content {padding-left: 18px; }\n"
           << ".player > .toggle-content {padding-left: 0; }\n"
           << ".toc {float: left;padding: 0; }\n"
           << ".toc-wide {width: 560px; }\n"
           << ".toc-narrow {width: 375px; }\n"
           << ".toc li {margin-bottom: 10px;list-style-type: none; }\n"
           << ".toc li ul {padding-left: 10px; }\n"
           << ".toc li ul li {margin: 0;list-style-type: none;font-size: 13px; }\n"
           << ".charts {float: left;width: 541px;margin-top: 10px; }\n"
           << ".charts-left {margin-right: 40px; }\n"
           << ".charts img {background-color: #333;padding: 5px;margin-bottom: 20px;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
           << ".talents div.float {width: auto;margin-right: 50px; }\n"
           << "table.sc {background-color: #333;padding: 4px 2px 2px 2px;margin: 10px 0 20px 0;-moz-border-radius: 6px;-khtml-border-radius: 6px;-webkit-border-radius: 6px;border-radius: 6px; }\n"
           << "table.sc tr {color: #fff;background-color: #1a1a1a; }\n"
           << "table.sc tr.head {background-color: #aaa;color: #fff; }\n"
           << "table.sc tr.odd {background-color: #222; }\n"
           << "table.sc th {padding: 2px 4px 4px 4px;text-align: center;background-color: #333;color: #fff; }\n"
           << "table.sc td {padding: 2px;text-align: center;font-size: 13px; }\n"
           << "table.sc th.left, table.sc td.left, table.sc tr.left th, table.sc tr.left td {text-align: left; }\n"
           << "table.sc th.right, table.sc td.right, table.sc tr.right th, table.sc tr.right td {text-align: right;padding-right: 4px; }\n"
           << "table.sc th.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
           << "table.sc td.small {padding: 2px 2px 3px 2px;font-size: 11px; }\n"
           << "table.sc tr.details td {padding: 0 0 15px 15px;text-align: left;background-color: #333;font-size: 11px; }\n"
           << "table.sc tr.details td ul {padding: 0;margin: 4px 0 8px 0; }\n"
           << "table.sc tr.details td ul li {clear: both;padding: 2px;list-style-type: none; }\n"
           << "table.sc tr.details td ul li span.label {display: block;padding: 2px;float: left;width: 145px;margin-right: 4px;background: #222; }\n"
           << "table.sc tr.details td ul li span.tooltip {display: block;float: left;width: 190px; }\n"
           << "table.sc tr.details td ul li span.tooltip-wider {display: block;float: left;width: 350px; }\n"
           << "table.sc tr.details td div.float {width: 350px; }\n"
           << "table.sc tr.details td div.float h5 {margin-top: 4px; }\n"
           << "table.sc tr.details td div.float ul {margin: 0 0 12px 0; }\n"
           << "table.sc td.filler {background-color: #333; }\n"
           << "table.sc .dynamic-buffs tr.details td ul li span.label {width: 120px; }\n"
           << "tr.details td table.details {padding: 0px;margin: 5px 0 10px 0; }\n"
           << "tr.details td table.details tr th {background-color: #222; }\n"
           << "tr.details td table.details tr td {background-color: #2d2d2d; }\n"
           << "tr.details td table.details tr.odd td {background-color: #292929; }\n"
           << "tr.details td table.details tr td {padding: 1px 3px 1px 3px; }\n"
           << "tr.details td table.details tr td.right {text-align: right; }\n"
           << ".player-thumbnail {float: right;margin: 8px;border-radius: 12px;-moz-border-radius: 12px;-webkit-border-radius: 12px;-khtml-border-radius: 12px; }\n"
           << "</style>\n";
  }
};

typedef mop_html_style_t default_html_style_t;

// print_html_contents ======================================================

void print_html_contents( report::sc_html_stream& os, const sim_t* sim )
{
  size_t c = 2;     // total number of TOC entries
  if ( sim -> scaling -> has_scale_factors() )
    ++c;

  size_t num_players = sim -> players_by_name.size();
  c += num_players;
  if ( sim -> report_targets )
    c += sim -> targets_by_name.size();

  if ( sim -> report_pets_separately )
  {
    for ( size_t i = 0; i < num_players; i++ )
    {
      for ( size_t j = 0; j < sim -> players_by_name[ i ] -> pet_list.size(); ++j )
      {
        pet_t* pet = sim -> players_by_name[ i ] -> pet_list[ j ];

        if ( pet -> summoned && !pet -> quiet )
          ++c;
      }
    }
  }

  os << "<div id=\"table-of-contents\" class=\"section grouped-first grouped-last\">\n"
     << "<h2 class=\"toggle\">Table of Contents</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

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
  for ( int i = 0; i < n; i++ )
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

    os << "<ul class=\"toc " << toc_class << "\">\n";

    int ci = 1;    // in-column counter
    if ( i == 0 )
    {
      os << "<li><a href=\"#raid-summary\">Raid Summary</a></li>\n";
      ci++;
      if ( sim -> scaling -> has_scale_factors() )
      {
        os << "<li><a href=\"#raid-scale-factors\">Scale Factors</a></li>\n";
        ci++;
      }
    }
    while ( ci <= cs )
    {
      if ( pi < static_cast<int>( sim -> players_by_name.size() ) )
      {
        player_t* p = sim -> players_by_name[ pi ];
        std::string html_name = p -> name();
        util::encode_html( html_name );
        os << "<li><a href=\"#player" << p -> index << "\">" << html_name << "</a>";
        ci++;
        if ( sim -> report_pets_separately )
        {
          os << "\n<ul>\n";
          for ( size_t k = 0; k < sim -> players_by_name[ pi ] -> pet_list.size(); ++k )
          {
            pet_t* pet = sim -> players_by_name[ pi ] -> pet_list[ k ];
            if ( pet -> summoned )
            {
              html_name = pet -> name();
              util::encode_html( html_name );
              os << "<li><a href=\"#player" << pet -> index << "\">" << html_name << "</a></li>\n";
              ci++;
            }
          }
          os << "</ul>";
        }
        os << "</li>\n";
        pi++;
      }
      if ( pi == static_cast<int>( sim -> players_by_name.size() ) )
      {
        if ( ab == 0 )
        {
          os << "<li><a href=\"#auras-buffs\">Auras/Buffs</a></li>\n";
          ab = 1;
        }
        ci++;
        os << "<li><a href=\"#sim-info\">Simulation Information</a></li>\n";
        ci++;
        if ( sim -> report_raw_abilities )
        {
          os << "<li><a href=\"#raw-abilities\">Raw Ability Summary</a></li>\n";
          ci++;
        }
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
          if ( pi < static_cast<int>( sim -> targets_by_name.size() ) )
          {
            player_t* p = sim -> targets_by_name[ pi ];
            os << "<li><a href=\"#player" << p -> index << "\">"
               << util::encode_html( p -> name() ) << "</a></li>\n";
          }
          ci++;
          pi++;
        }
      }
    }
    os << "</ul>\n";
  }

  os << "<div class=\"clear\"></div>\n"
     << "</div>\n\n"
     << "</div>\n\n";
}

// print_html_sim_summary ===================================================

void print_html_sim_summary( report::sc_html_stream& os, const sim_t* sim, const sim_report_information_t& ri )
{
  os << "<div id=\"sim-info\" class=\"section\">\n";

  os << "<h2 class=\"toggle\">Simulation & Raid Information</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  os << "<table class=\"sc mt\">\n";

  os << "<tr class=\"left\">\n"
     << "<th>Iterations:</th>\n"
     << "<td>" << sim -> iterations << "</td>\n"
     << "</tr>\n";

  os << "<tr class=\"left\">\n"
     << "<th>Threads:</th>\n"
     << "<td>" << ( ( sim -> threads < 1 ) ? 1 : sim -> threads ) << "</td>\n"
     << "</tr>\n";

  os << "<tr class=\"left\">\n"
     << "<th>Confidence:</th>\n"
     << "<td>" << sim -> confidence * 100.0 << "%</td>\n"
     << "</tr>\n";

  os.printf( "<tr class=\"left\">\n"
             "<th>Fight Length%s:</th>\n"
             "<td>%.0f - %.0f ( %.1f )</td>\n"
             "</tr>\n",
             (sim -> fixed_time ? " (fixed time)" : ""),
             sim -> simulation_length.min(),
             sim -> simulation_length.max(),
             sim -> simulation_length.mean() );

  os << "<tr class=\"left\">\n"
     << "<td><h2>Performance:</h2></td>\n"
     << "<td></td>\n"
     << "</tr>\n";

  os.printf(
    "<tr class=\"left\">\n"
    "<th>Total Events Processed:</th>\n"
    "<td>%ld</td>\n"
    "</tr>\n",
    ( long ) sim -> total_events_processed );

  os.printf(
    "<tr class=\"left\">\n"
    "<th>Max Event Queue:</th>\n"
    "<td>%ld</td>\n"
    "</tr>\n",
    ( long ) sim -> max_events_remaining );

  os.printf(
    "<tr class=\"left\">\n"
    "<th>Sim Seconds:</th>\n"
    "<td>%.0f</td>\n"
    "</tr>\n",
    sim -> iterations * sim -> simulation_length.mean() );
  os.printf(
    "<tr class=\"left\">\n"
    "<th>CPU Seconds:</th>\n"
    "<td>%.4f</td>\n"
    "</tr>\n",
    sim -> elapsed_cpu );
  os.printf(
    "<tr class=\"left\">\n"
    "<th>Physical Seconds:</th>\n"
    "<td>%.4f</td>\n"
    "</tr>\n",
    sim -> elapsed_time );
  os.printf(
    "<tr class=\"left\">\n"
    "<th>Speed Up:</th>\n"
    "<td>%.0f</td>\n"
    "</tr>\n",
    sim -> iterations * sim -> simulation_length.mean() / sim -> elapsed_cpu );

  os << "<tr class=\"left\">\n"
     << "<td><h2>Settings:</h2></td>\n"
     << "<td></td>\n"
     << "</tr>\n";

  os.printf(
    "<tr class=\"left\">\n"
    "<th>World Lag:</th>\n"
    "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
    "</tr>\n",
    ( double )sim -> world_lag.total_millis(), ( double )sim -> world_lag_stddev.total_millis() );
  os.printf(
    "<tr class=\"left\">\n"
    "<th>Queue Lag:</th>\n"
    "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
    "</tr>\n",
    ( double )sim -> queue_lag.total_millis(), ( double )sim -> queue_lag_stddev.total_millis() );

  if ( sim -> strict_gcd_queue )
  {
    os.printf(
      "<tr class=\"left\">\n"
      "<th>GCD Lag:</th>\n"
      "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
      "</tr>\n",
      ( double )sim -> gcd_lag.total_millis(), ( double )sim -> gcd_lag_stddev.total_millis() );
    os.printf(
      "<tr class=\"left\">\n"
      "<th>Channel Lag:</th>\n"
      "<td>%.0f ms ( stddev = %.0f ms )</td>\n"
      "</tr>\n",
      ( double )sim -> channel_lag.total_millis(), ( double )sim -> channel_lag_stddev.total_millis() );
    os.printf(
      "<tr class=\"left\">\n"
      "<th>Queue GCD Reduction:</th>\n"
      "<td>%.0f ms</td>\n"
      "</tr>\n",
      ( double )sim -> queue_gcd_reduction.total_millis() );
  }

  int sd_counter = 0;
  report::print_html_sample_data( os, sim, sim -> simulation_length, "Simulation Length", sd_counter, 2 );

  os << "</table>\n";

  // Left side charts: dps, gear, timeline, raid events

  os << "<div class=\"charts charts-left\">\n";
  // Timeline Distribution Chart
  if ( sim -> iterations > 1 && ! ri.timeline_chart.empty() )
  {
    os.printf(
      "<a href=\"#help-timeline-distribution\" class=\"help\"><img src=\"%s\" alt=\"Timeline Distribution Chart\" /></a>\n",
      ri.timeline_chart.c_str() );
  }

  // Gear Charts
  for ( size_t i = 0; i < ri.gear_charts.size(); i++ )
  {
    os << "<img src=\"" << ri.gear_charts[ i ] << "\" alt=\"Gear Chart\" />\n";
  }

  // Raid Downtime Chart
  if ( !  ri.downtime_chart.empty() )
  {
    os.printf(
      "<img src=\"%s\" alt=\"Raid Downtime Chart\" />\n",
      ri.downtime_chart.c_str() );
  }

  os << "</div>\n";

  // Right side charts: dpet
  os << "<div class=\"charts\">\n";

  for ( size_t i = 0; i < ri.dpet_charts.size(); i++ )
  {
    os.printf(
      "<img src=\"%s\" alt=\"DPET Chart\" />\n",
      ri.dpet_charts[ i ].c_str() );
  }

  os << "</div>\n";


  // closure
  os << "<div class=\"clear\"></div>\n"
     << "</div>\n"
     << "</div>\n\n";
}

// print_html_raid_summary ==================================================

void print_html_raid_summary( report::sc_html_stream& os, const sim_t* sim, const sim_report_information_t& ri )
{
  os << "<div id=\"raid-summary\" class=\"section section-open\">\n\n";

  os << "<h2 class=\"toggle open\">Raid Summary</h2>\n";

  os << "<div class=\"toggle-content\">\n";

  os << "<ul class=\"params\">\n";

  os.printf(
    "<li><b>Raid Damage:</b> %.0f</li>\n",
    sim -> total_dmg.mean() );
  os.printf(
    "<li><b>Raid DPS:</b> %.0f</li>\n",
    sim -> raid_dps.mean() );
  if ( sim -> total_heal.mean() > 0 )
  {
    os.printf(
      "<li><b>Raid Heal+Absorb:</b> %.0f</li>\n",
      sim -> total_heal.mean() + sim -> total_absorb.mean() );
    os.printf(
      "<li><b>Raid HPS+APS:</b> %.0f</li>\n",
      sim -> raid_hps.mean() + sim -> raid_aps.mean() );
  }
  os << "</ul><p>&nbsp;</p>\n";

  // Left side charts: dps, raid events
  os << "<div class=\"charts charts-left\">\n";

  for ( size_t i = 0; i < ri.dps_charts.size(); i++ )
  {
    os.printf(
      "<map id='DPSMAP%d' name='DPSMAP%d'></map>\n", ( int )i, ( int )i );
    os.printf(
      "<img id='DPSIMG%d' src=\"%s\" alt=\"DPS Chart\" />\n",
      ( int )i, ri.dps_charts[ i ].c_str() );
  }

  if ( ! sim -> raid_events_str.empty() )
  {
    os << "<table>\n"
       << "<tr>\n"
       << "<th></th>\n"
       << "<th class=\"left\">Raid Event List</th>\n"
       << "</tr>\n";

    std::vector<std::string> raid_event_names = util::string_split( sim -> raid_events_str, "/" );
    for ( size_t i = 0; i < raid_event_names.size(); i++ )
    {
      os << "<tr";
      if ( ( i & 1 ) )
      {
        os << " class=\"odd\"";
      }
      os << ">\n";

      os.printf(
        "<th class=\"right\">%d</th>\n"
        "<td class=\"left\">%s</td>\n"
        "</tr>\n",
        ( int )i,
        raid_event_names[ i ].c_str() );
    }
    os << "</table>\n";
  }
  os << "</div>\n";

  // Right side charts: hps
  os << "<div class=\"charts\">\n";

  for ( size_t i = 0; i < ri.hps_charts.size(); i++ )
  {
    os.printf(  "<map id='HPSMAP%d' name='HPSMAP%d'></map>\n", ( int )i, ( int )i );
    os.printf(
      "<img id='HPSIMG%d' src=\"%s\" alt=\"HPS Chart\" />\n",
      ( int )i, ri.hps_charts[ i ].c_str() );
  }

  // RNG chart
  if ( sim -> report_rng )
  {
    os << "<ul>\n";
    for ( size_t i = 0; i < sim -> players_by_name.size(); i++ )
    {
      player_t* p = sim -> players_by_name[ i ];
      double range = ( p -> collected_data.dps.percentile( 0.95 ) - p -> collected_data.dps.percentile( 0.05 ) ) / 2.0;
      os.printf(
        "<li>%s: %.1f / %.1f%%</li>\n",
        util::encode_html( p -> name() ).c_str(),
        range,
        p -> collected_data.dps.mean() ? ( range * 100 / p -> collected_data.dps.mean() ) : 0 );
    }
    os << "</ul>\n";
  }

  os << "</div>\n";

  // closure
  os << "<div class=\"clear\"></div>\n"
     << "</div>\n"
     << "</div>\n\n";

}

// print_html_raid_imagemaps ================================================

void print_html_raid_imagemap( report::sc_html_stream& os, sim_t* sim, size_t num, bool dps )
{
  std::vector<player_t*> player_list = ( dps ) ? sim -> players_by_dps : sim -> players_by_hps;
  size_t start = num * MAX_PLAYERS_PER_CHART;
  size_t end = start + MAX_PLAYERS_PER_CHART;

  for ( size_t i = 0; i < player_list.size(); i++ )
  {
    player_t* p = player_list[ i ];
    if ( ( dps ? p -> collected_data.dps.mean() : p -> collected_data.hps.mean() ) <= 0 )
    {
      player_list.resize( i );
      break;
    }
  }

  if ( end > player_list.size() ) end = player_list.size();

  os << "n = [";
  for ( int i = ( int )end - 1; i >= ( int )start; i-- )
  {
    os << "\"player" << player_list[i] -> index << "\"";
    if ( i != ( int )start ) os << ", ";
  }
  os << "];\n";

  char imgid[32];
  util::snprintf( imgid, sizeof( imgid ), "%sIMG%u", ( dps ) ? "DPS" : "HPS", as<unsigned>( num ) );
  char mapid[32];
  util::snprintf( mapid, sizeof( mapid ), "%sMAP%u", ( dps ) ? "DPS" : "HPS", as<unsigned>( num ) );

  os.printf(
    "u = document.getElementById('%s').src;\n"
    "getMap(u, n, function(mapStr) {\n"
    "document.getElementById('%s').innerHTML += mapStr;\n"
    "$j('#%s').attr('usemap','#%s');\n"
    "$j('#%s area').click(function(e) {\n"
    "anchor = $j(this).attr('href');\n"
    "target = $j(anchor).children('h2:first');\n"
    "open_anchor(target);\n"
    "});\n"
    "});\n\n",
    imgid, mapid, imgid, mapid, mapid );
}

void print_html_raid_imagemaps( report::sc_html_stream& os, sim_t* sim, sim_report_information_t& ri )
{

  os << "<script type=\"text/javascript\">\n"
     << "var $j = jQuery.noConflict();\n"
     << "function getMap(url, names, mapWrite) {\n"
     << "$j.getJSON(url + '&chof=json&callback=?', function(jsonObj) {\n"
     << "var area = false;\n"
     << "var chart = jsonObj.chartshape;\n"
     << "var mapStr = '';\n"
     << "for (var i = 0; i < chart.length; i++) {\n"
     << "area = chart[i];\n"
     << "area.coords[2] = 523;\n"
     << "mapStr += \"\\n  <area name='\" + area.name + \"' shape='\" + area.type + \"' coords='\" + area.coords.join(\",\") + \"' href='#\" + names[i] + \"'  title='\" + names[i] + \"'>\";\n"
     << "}\n"
     << "mapWrite(mapStr);\n"
     << "});\n"
     << "}\n\n";

  for ( size_t i = 0; i < ri.dps_charts.size(); i++ )
  {
    print_html_raid_imagemap( os, sim, i, true );
  }

  for ( size_t i = 0; i < ri.hps_charts.size(); i++ )
  {
    print_html_raid_imagemap( os, sim, i, false );
  }

  os << "</script>\n";

}

// print_html_scale_factors =================================================

void print_html_scale_factors( report::sc_html_stream& os, sim_t* sim )
{
  if ( ! sim -> scaling -> has_scale_factors() ) return;

  os << "<div id=\"raid-scale-factors\" class=\"section grouped-first\">\n\n"
     << "<h2 class=\"toggle\">DPS Scale Factors (dps increase per unit stat)</h2>\n"
     << "<div class=\"toggle-content hide\">\n";

  os << "<table class=\"sc\">\n";

  // this next part is used to determine which columns to suppress
  std::vector<double> stat_effect_is_nonzero;

    //initialize accumulator to zero
  for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
  {
    stat_effect_is_nonzero.push_back( 0.0 );
  }

  // cycle through players
  for ( size_t i = 0, players = sim -> players_by_name.size(); i < players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    // add the absolute value of their stat weights to accumulator element
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      stat_effect_is_nonzero[ j ] += std::abs( p -> scaling.get_stat( j ) );
    }
  }
  // end column suppression section

  player_e prev_type = PLAYER_NONE;

  for ( size_t i = 0, players = sim -> players_by_name.size(); i < players; i++ )
  {
    player_t* p = sim -> players_by_name[ i ];

    if ( p -> type != prev_type )
    {
      prev_type = p -> type;

      os << "<tr>\n"
         << "<th class=\"left small\">Profile</th>\n";
      for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
      {
        if ( sim -> scaling -> stats.get_stat( j ) != 0 && stat_effect_is_nonzero[ j ] > 0 )
        {
          os << "<th class=\"small\">" << util::stat_type_abbrev( j ) << "</th>\n";
        }
      }
      os << "<th colspan=\"2\" class=\"small\">wowhead</th>\n"
         << "<th class=\"small\">lootrank</th>\n"
         << "</tr>\n";
    }

    os << "<tr";
    if ( ( i & 1 ) )
    {
      os << " class=\"odd\"";
    }
    os << ">\n";
    os.printf(
      "<td class=\"left small\">%s</td>\n",
      p -> name() );
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( sim -> scaling -> stats.get_stat( j ) != 0 && stat_effect_is_nonzero[ j ] > 0 )
      {
        if ( p -> scaling.get_stat( j ) == 0 )
        {
          os << "<td class=\"small\">-</td>\n";
        }
        else
        {
          os.printf(
            "<td class=\"small\">%.*f</td>\n",
            sim -> report_precision,
            p -> scaling.get_stat( j ) );
        }
      }
    }
    os.printf(
      "<td class=\"small\"><a href=\"%s\"> wowhead </a></td>\n"
      "<td class=\"small\"><a href=\"%s\"> wowhead (caps merged)</a></td>\n"
      "<td class=\"small\"><a href=\"%s\"> lootrank</a></td>\n"
      "</tr>\n",
      p -> report_information.gear_weights_wowhead_std_link.c_str(),
      p -> report_information.gear_weights_wowhead_alt_link.c_str(),
      p -> report_information.gear_weights_lootrank_link.c_str() );
  }
  os << "</table>\n";

  if ( sim -> iterations < 10000 )
    os << "<div class=\"alert\">\n"
       << "<h3>Warning</h3>\n"
       << "<p>Scale Factors generated using less than 10,000 iterations will vary from run to run.</p>\n"
       << "</div>\n";

  os << "</div>\n"
     << "</div>\n\n";
}

struct help_box_t
{
  std::string abbreviation, text;
};

/* Here simple help boxes with just a Title/Abbreviation and a Text can be added
 * the help-id will be a tokenized form of the abbreviation, spaces replaced with '-',
 * everything lowerspace and '%' replaced by '-pct'
 */
static const help_box_t help_boxes[] =
{
    { "APM", "Average number of actions executed per minute."},
    { "APS", "Average absorption per active player duration."},
    { "Constant Buffs", "Buffs received prior to combat and present the entire fight."},
    { "Count", "Average number of times an action is executed per iteration."},
    { "Crit", "Average crit damage."},
    { "Crit%", "Percentage of executes that resulted in critical strikes."},
    { "DPE", "Average damage per execution of an individual action."},
    { "DPET", "Average damage per execute time of an individual action; the amount of damage generated, divided by the time taken to execute the action, including time spent in the GCD."},
    { "DPR", "Average damage per resource point spent."},
    { "DPS", "Average damage per active player duration."},
    { "DPSE", "Average damage per fight duration."},
    { "DTPS", "Average damage taken per second per active player duration."},
    { "HPS", "Average healing (and absorption) per active player duration."},
    { "HPSE", "Average healing (and absorption) per fight duration."},
    { "HPE", "Average healing (or absorb) per execution of an individual action."},
    { "HPET", "Average healing (or absorb) per execute time of an individual action; the amount of healing generated, divided by the time taken to execute the action, including time spent in the GCD."},
    { "HPR", "Average healing (or absorb) per resource point spent."},
    { "Impacts", "Average number of impacts against a target (for attacks that hit multiple times per execute) per iteration."},
    { "Dodge%", "Percentage of executes that resulted in dodges."},
    { "DPS%", "Percentage of total DPS contributed by a particular action."},
    { "HPS%", "Percentage of total HPS (including absorb) contributed by a particular action."},
    { "Theck-Meloree Index", "Measure of damage smoothness, calculated over entire fight length. Related to max spike damage, 1k TMI is roughly equivalent to 1% of your health. TMI ignores external healing and absorbs. Lower is better."},
    { "TMI bin size", "Time bin size used to calculate TMI and MSD, in seconds."},
    { "Max Spike Damage Frequency", "This is roughly how many spikes as large as MSD Mean you take per iteration. Calculated from TMI and MSD values."},
    { "Dynamic Buffs", "Temporary buffs received during combat, perhaps multiple times."},
    { "Glance%", "Percentage of executes that resulted in glancing blows."},
    { "Block%", "Percentage of executes that resulted in blocking blows."},
    { "Id", "Associated spell-id for this ability."},
    { "Ability", "Name of the ability."},
    { "Total", "Total damage for this ability during the fight."},
    { "Hit", "Average non-crit damage."},
    { "Interval", "Average time between executions of a particular action."},
    { "Avg", "Average direct damage per execution."},
    { "Miss%", "Percentage of executes that resulted in misses, dodges or parries."},
    { "Origin", "The player profile from which the simulation script was generated. The profile must be copied into the same directory as this HTML file in order for the link to work."},
    { "Parry%", "Percentage of executes that resulted in parries."},
    { "RPS In", "Average primary resource points generated per second."},
    { "RPS Out", "Average primary resource points consumed per second."},
    { "Scale Factors", "Gain per unit stat increase except for <b>Hit/Expertise</b> which represent <b>Loss</b> per unit stat <b>decrease</b>."},
    { "Gear Amount", "Amount from raw gear, before class or buff modifiers. Amount from hybrid primary stats (i.e. Agility/Intellect) shown in parentheses."},
    { "Stats Raid Buffed", "Amount after all static buffs have been accounted for. Dynamic buffs (i.e. trinkets, potions) not included."},
    { "Stats Unbuffed", "Amount after class modifiers and effects, but before buff modifiers."},
    { "Ticks", "Average number of periodic ticks per iteration. Spells that do not have a damage-over-time component will have zero ticks."},
    { "Ticks Crit", "Average crit tick damage."},
    { "Ticks Crit%", "Percentage of ticks that resulted in critical strikes."},
    { "Ticks Hit", "Average non-crit tick damage."},
    { "Ticks Miss%", "Percentage of ticks that resulted in misses, dodges or parries."},
    { "Ticks Uptime%", "Percentage of total time that DoT is ticking on target."},
    { "Ticks Avg", "Average damage per tick."},
    { "Timeline Distribution", "The simulated encounter's duration can vary based on the health of the target and variation in the raid DPS. This chart shows how often the duration of the encounter varied by how much time."},
    { "Waiting", "This is the percentage of time in which no action can be taken other than autoattacks. This can be caused by resource starvation, lockouts, and timers."},
    { "Scale Factor Ranking", "This row ranks the scale factors from highest to lowest, checking whether one scale factor is higher/lower than another with statistical significance."},

};

void print_html_help_boxes( report::sc_html_stream& os, sim_t* sim )
{
  os << "<!-- Help Boxes -->\n";

  for ( size_t i = 0; i < sizeof_array( help_boxes ); ++i )
  {
    const help_box_t& hb = help_boxes[ i ];
    std::string tokenized_id = hb.abbreviation;
    util::replace_all( tokenized_id, " ", "-" );
    util::replace_all( tokenized_id, "%", "-pct" );
    util::tolower( tokenized_id );
    os << "<div id=\"help-" << tokenized_id << "\">\n"
       << "<div class=\"help-box\">\n"
       << "<h3>" << hb.abbreviation << "</h3>\n"
       << "<p>" << hb.text << "</p>\n"
       << "</div>\n"
       << "</div>\n";
  }

  // From here on go special help boxes with dynamic text / etc.

  os << "<div id=\"help-tmirange\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>TMI Range</h3>\n"
     << "<p>This is the range of TMI values containing " << sim -> confidence * 100 << "% of the data, roughly centered on the mean.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-tmiwin\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>TMI/MSD Window</h3>\n"
     << "<p>Window length used to calculate TMI and MSD, in seconds.</p>\n"
     << "</div>\n"
     << "</div>\n";

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    os << "<div id=\"help-msd" << sim -> actor_list[ i ] -> actor_index << "\">\n"
      << "<div class=\"help-box\">\n"
      << "<h3>Max Spike Damage</h3>\n"
      << "<p>Maximum amount of net damage taken in any " << sim -> actor_list[ i ] -> tmi_window << "-second period, expressed as a percentage of max health. Calculated independently for each iteration. "
      << "'MSD Min/Mean/Max' are the lowest/average/highest MSDs out of all iterations.</p>\n"
      << "</div>\n"
      << "</div>\n";
  }

  os << "<div id=\"help-error\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Error</h3>\n"
     << "<p>Estimator for the " << sim -> confidence * 100.0 << "% confidence interval.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-range\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Range</h3>\n"
     << "<p>This is the range of values containing " << sim -> confidence * 100 << "% of the data, roughly centered on the mean.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<div id=\"help-fight-length\">\n"
     << "<div class=\"help-box\">\n"
     << "<h3>Fight Length</h3>\n"
     << "<p>Fight Length: " << sim -> max_time.total_seconds() << "<br />\n"
     << "Vary Combat Length: " << sim -> vary_combat_length << "</p>\n"
     << "<p>Fight Length is the specified average fight duration. If vary_combat_length is set, the fight length will vary by +/- that portion of the value. See <a href=\"http://code.google.com/p/simulationcraft/wiki/Options#Combat_Length\" class=\"ext\">Combat Length</a> in the wiki for further details.</p>\n"
     << "</div>\n"
     << "</div>\n";

  os << "<!-- End Help Boxes -->\n";
}

// print_html_styles ========================================================

void print_html_styles( report::sc_html_stream& os, sim_t* sim )
{
  // First select the appropriate style
  std::shared_ptr<html_style_t> style;
  if ( sim -> print_styles == 1 )
  {
    style = std::shared_ptr<html_style_t>( new mop_html_style_t() );
  }
  else if ( sim -> print_styles == 2 )
  {
    style = std::shared_ptr<html_style_t>( new classic_html_style_t() );
  }
  else
  {
    style = std::shared_ptr<html_style_t>( new default_html_style_t() );
  }

  // Print
  style -> print_html_styles( os );
}

// print_html_masthead ======================================================

void print_html_masthead( report::sc_html_stream& os, sim_t* sim )
{
  // Begin masthead section
  os << "<div id=\"masthead\" class=\"section section-open\">\n\n";

  os.printf(
    "<span id=\"logo\"></span>\n"
    "<h1><a href=\"%s\">SimulationCraft %s</a></h1>\n"
    "<h2>for World of Warcraft %s %s (build level %d)</h2>\n\n",
    "http://code.google.com/p/simulationcraft/",
    SC_VERSION, sim -> dbc.wow_version(), ( sim -> dbc.ptr ?
#if SC_BETA
        "BETA"
#else
        "PTR"
#endif
        : "Live" ), sim -> dbc.build_level() );

  time_t rawtime;
  time( &rawtime );

  os << "<ul class=\"params\">\n";
  os.printf(
    "<li><b>Timestamp:</b> %s</li>\n",
    ctime( &rawtime ) );
  os.printf(
    "<li><b>Iterations:</b> %d</li>\n",
    sim -> iterations );

  if ( sim -> vary_combat_length > 0.0 )
  {
    timespan_t min_length = sim -> max_time * ( 1 - sim -> vary_combat_length );
    timespan_t max_length = sim -> max_time * ( 1 + sim -> vary_combat_length );
    os.printf(
      "<li class=\"linked\"><a href=\"#help-fight-length\" class=\"help\"><b>Fight Length:</b> %.0f - %.0f</a></li>\n",
      min_length.total_seconds(),
      max_length.total_seconds() );
  }
  else
  {
    os.printf(
      "<li><b>Fight Length:</b> %.0f</li>\n",
      sim -> max_time.total_seconds() );
  }
  os.printf(
    "<li><b>Fight Style:</b> %s</li>\n",
    sim -> fight_style.c_str() );
  os << "</ul>\n"
     << "<div class=\"clear\"></div>\n\n"
     << "</div>\n\n";
  // End masthead section
}

void print_html_errors( report::sc_html_stream& os, sim_t* sim )
{
  if ( ! sim -> error_list.empty() )
  {
    os << "<pre class=\"section section-open\" style=\"color: black; background-color: white; font-weight: bold;\">\n";

    for ( size_t i = 0; i < sim -> error_list.size(); i++ )
      os <<  sim -> error_list[ i ] << "\n";

    os << "</pre>\n\n";
  }
}

void print_html_beta_warning( report::sc_html_stream& os )
{
#if SC_BETA
  os << "<div id=\"notice\" class=\"section section-open\">\n"
     << "<h2>Beta Release</h2>\n"
     << "<ul>\n";

  for ( size_t i = 0; i < sizeof_array( report::beta_warnings ); ++i )
    os << "<li>" << report::beta_warnings[ i ] << "</li>\n";

  os << "</ul>\n"
     << "</div>\n\n";
#else
  (void)os;
#endif
}

void print_html_image_load_scripts( std::ostream& os )
{
  for ( size_t i = 0; i < sizeof_array( __image_load_script ); ++i )
  {
    os << __image_load_script[ i ];
  }
}

void print_html_head( report::sc_html_stream& os, sim_t* sim )
{
  os << "<title>Simulationcraft Results</title>\n";
  os << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
     << "<script type=\"text/javascript\" src=\"http://static.wowhead.com/widgets/power.js\"></script>\n"
     << "<script>var wowhead_tooltips = { \"colorlinks\": true, \"iconizelinks\": true, \"renamelinks\": true }</script>\n";

  print_html_styles( os, sim );
}

/* Main function building the html document and calling subfunctions
 */
void print_html_( report::sc_html_stream& os, sim_t* sim )
{
  // Set floating point formatting
  os.precision( sim -> report_precision );
  os << std::fixed;

  os << "<!DOCTYPE html>\n\n";
  os << "<html>\n";

  os << "<head>\n";
  print_html_head( os, sim );
  os << "</head>\n\n";

  os << "<body>\n";

  print_html_errors( os, sim );

  // Prints div wrappers for help popups
  os << "<div id=\"active-help\">\n"
     << "<div id=\"active-help-dynamic\">\n"
     << "<div class=\"help-box\"></div>\n"
     << "<a href=\"#\" class=\"close\"><span class=\"hide\">close</span></a>\n"
     << "</div>\n"
     << "</div>\n\n";

  print_html_masthead( os, sim );

  print_html_beta_warning( os );

  size_t num_players = sim -> players_by_name.size();

  if ( num_players > 1 || sim -> report_raid_summary )
  {
    print_html_contents( os, sim );
  }

  if ( num_players > 1 || sim -> report_raid_summary || ! sim -> raid_events_str.empty() )
  {
    print_html_raid_summary( os, sim, sim -> report_information );
    print_html_scale_factors( os, sim );
  }

  int k = 0; // Counter for both players and enemies, without pets.

  // Report Players
  for ( size_t i = 0; i < num_players; ++i )
  {
    report::print_html_player( os, sim -> players_by_name[ i ], k );

    // Pets
    if ( sim -> report_pets_separately )
    {
      std::vector<pet_t*>& pl = sim -> players_by_name[ i ] -> pet_list;
      for ( size_t j = 0; j < pl.size(); ++j )
      {
        pet_t* pet = pl[ j ];
        if ( pet -> summoned && !pet -> quiet )
          report::print_html_player( os, pet, 1 );
      }
    }
  }

  print_html_sim_summary( os, sim, sim -> report_information );

  if ( sim -> report_raw_abilities )
    raw_ability_summary::print( os, sim );

  // Report Targets
  if ( sim -> report_targets )
  {
    for ( size_t i = 0; i < sim -> targets_by_name.size(); ++i )
    {
      report::print_html_player( os, sim -> targets_by_name[ i ], k );
      ++k;

      // Pets
      if ( sim -> report_pets_separately )
      {
        std::vector<pet_t*>& pl = sim -> targets_by_name[ i ] -> pet_list;
        for ( size_t j = 0; j < pl.size(); ++j )
        {
          pet_t* pet = pl[ j ];
          //if ( pet -> summoned )
          report::print_html_player( os, pet, 1 );
        }
      }
    }
  }

  print_html_help_boxes( os, sim );

  // jQuery
  // The /1/ url auto-updates to the latest minified version
  os << "<script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1/jquery.min.js\"></script>\n";

  if ( sim -> hosted_html )
  {
    // Google Analytics
    os << "<script type=\"text/javascript\" src=\"http://www.simulationcraft.org/js/ga.js\"></script>\n";
  }

  print_html_image_load_scripts( os );

  if ( num_players > 1 )
    print_html_raid_imagemaps( os, sim, sim -> report_information );

  os << "</body>\n\n"
     << "</html>\n";
}

} // UNNAMED NAMESPACE ====================================================

namespace report {

void print_html( sim_t* sim )
{
  if ( sim -> simulation_length.mean() == 0 ) return;
  if ( sim -> html_file_str.empty() ) return;


  // Setup file stream and open file
  report::sc_html_stream s;
  s.open( sim -> html_file_str );
  if ( ! s )
  {

    sim -> errorf( "Failed to open html output file '%s'.", sim -> html_file_str.c_str() );
    return;
  }

  report::generate_sim_report_information( sim, sim -> report_information );

  // Print html report
  print_html_( s, sim );
}

} // report
