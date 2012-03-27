// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_REPORT_HPP
#define SC_REPORT_HPP

namespace {
#if SC_BETA
// beta warning messages
static const std::string beta_warnings[] =
{
  "Beta! Beta! Beta! Beta! Beta! Beta!",
  "Not All classes are yet supported.",
  "Some class models still need tweaking.",
  "Some class action lists need tweaking.",
  "Some class BiS gear setups need tweaking.",
  "Some trinkets not yet implemented.",
  "Constructive feedback regarding our output will shorten the Beta phase dramatically.",
  "Beta! Beta! Beta! Beta! Beta! Beta!",
  ""
};
#endif

void print_html_rng_information( FILE* file, rng_t* rng )
{
  fprintf( file,
           "\t\t\t\t\t\t\t<table>\n"
           "\t\t\t\t\t\t\t\t<tr>\n"
           "\t\t\t\t\t\t\t\t\t<th class=\"left small\"><a href=\"#\" class=\"toggle-details\" rel=\"sample=%s\">RNG %s</a></th>\n"
           "\t\t\t\t\t\t\t\t\t<th></th>\n"
           "\t\t\t\t\t\t\t\t</tr>\n", rng->name_str.c_str(), rng->name_str.c_str() );
  fprintf( file,
           "\t\t\t\t\t\t\t\t<tr class=\"details hide\">\n"
           "\t\t\t\t\t\t\t\t<td colspan=\"2\" class=\"filler\">\n" );
  fprintf( file,
           "\t\t\t\t\t\t\t<table class=\"details\">\n" );
  fprintf( file,
           "\t\t\t\t\t\t\t\t<tr>\n"
           "\t\t\t\t\t\t\t\t\t<td class=\"left\">Actual Roll / Expected Roll / Diff%%</td>\n"
           "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.8f / %.8f / %.8f%%</td>\n"
           "\t\t\t\t\t\t\t\t</tr>\n",
           rng->actual_roll, rng->expected_roll,
           rng->expected_roll ? fabs( ( rng->actual_roll-rng->expected_roll ) / rng->expected_roll ) * 100.0 : 0 );

  fprintf( file,
           "\t\t\t\t\t\t\t\t</table>\n" );
  fprintf( file,
           "\t\t\t\t\t\t\t\t</td>\n"
           "\t\t\t\t\t\t\t</tr>\n" );
  fprintf( file,
           "\t\t\t\t\t\t\t\t</table>\n" );

}
void print_html_sample_data( FILE* file, player_t* p, sample_data_t& data, const std::string& name )
{
  // Print Statistics of a Sample Data Container

  fprintf( file,
           "\t\t\t\t\t\t\t<table>\n"
           "\t\t\t\t\t\t\t\t<tr>\n"
           "\t\t\t\t\t\t\t\t\t<th class=\"left small\"><a href=\"#\" class=\"toggle-details\" rel=\"sample=%s\">%s</a></th>\n"
           "\t\t\t\t\t\t\t\t\t<th></th>\n"
           "\t\t\t\t\t\t\t\t</tr>\n", name.c_str(), name.c_str() );

  if ( data.basics_analyzed() )
  {
    fprintf( file,
             "\t\t\t\t\t\t\t\t<tr class=\"details hide\">\n"
             "\t\t\t\t\t\t\t\t<td colspan=\"2\" class=\"filler\">\n" );

    fprintf( file,
             "\t\t\t\t\t\t\t<table class=\"details\">\n" );

    fprintf( file,
             "\t\t\t\t\t\t\t\t<tr>\n"
             "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Sample Data</b></td>\n"
             "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
             "\t\t\t\t\t\t\t\t</tr>\n" );

    fprintf( file,
             "\t\t\t\t\t\t\t\t<tr>\n"
             "\t\t\t\t\t\t\t\t\t<td class=\"left\">Count</td>\n"
             "\t\t\t\t\t\t\t\t\t<td class=\"right\">%d</td>\n"
             "\t\t\t\t\t\t\t\t</tr>\n",
             data.size() );

    fprintf( file,
             "\t\t\t\t\t\t\t\t<tr>\n"
             "\t\t\t\t\t\t\t\t\t<td class=\"left\">Mean</td>\n"
             "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
             "\t\t\t\t\t\t\t\t</tr>\n",
             data.mean );

    if ( !data.simple || data.min_max )
    {

      fprintf( file,
               "\t\t\t\t\t\t\t\t<tr>\n"
               "\t\t\t\t\t\t\t\t\t<td class=\"left\">Minimum</td>\n"
               "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
               "\t\t\t\t\t\t\t\t</tr>\n",
               data.min );

      fprintf( file,
               "\t\t\t\t\t\t\t\t<tr>\n"
               "\t\t\t\t\t\t\t\t\t<td class=\"left\">Maximum</td>\n"
               "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
               "\t\t\t\t\t\t\t\t</tr>\n",
               data.max );

      fprintf( file,
               "\t\t\t\t\t\t\t\t<tr>\n"
               "\t\t\t\t\t\t\t\t\t<td class=\"left\">Spread ( max - min )</td>\n"
               "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
               "\t\t\t\t\t\t\t\t</tr>\n",
               data.max - data.min );

      fprintf( file,
               "\t\t\t\t\t\t\t\t<tr>\n"
               "\t\t\t\t\t\t\t\t\t<td class=\"left\">Range [ ( max - min ) / 2 * 100%% ]</td>\n"
               "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f%%</td>\n"
               "\t\t\t\t\t\t\t\t</tr>\n",
               data.mean ? ( ( data.max - data.min ) / 2 ) * 100 / data.mean : 0 );
      if ( data.variance_analyzed() )
      {
        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">Standard Deviation</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.4f</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 data.std_dev );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">5th Percentile</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 data.percentile( 0.05 ) );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">95th Percentile</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 data.percentile( 0.95 ) );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">( 95th Percentile - 5th Percentile )</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.2f</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 data.percentile( 0.95 ) - data.percentile( 0.05 ) );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Mean Distribution</b></td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n" );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">Standard Deviation</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%.4f</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 data.mean_std_dev );

        double mean_error = data.mean_std_dev * p -> sim -> confidence_estimator;
        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">%.2f%% Confidence Intervall</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f - %.2f )</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 p -> sim -> confidence * 100.0,
                 data.mean - mean_error,
                 data.mean + mean_error );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">Normalized %.2f%% Confidence Intervall</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">( %.2f%% - %.2f%% )</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 p -> sim -> confidence * 100.0,
                 data.mean ? 100 - mean_error * 100 / data.mean : 0,
                 data.mean ? 100 + mean_error * 100 / data.mean : 0 );



        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\"><b>Approx. Iterations needed for ( always use n>=50 )</b></td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\"></td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n" );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">1%% Error</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 ( int ) ( data.mean ? ( ( mean_error * mean_error * ( ( float ) data.size() ) / ( 0.01 * data.mean * 0.01 * data.mean ) ) ) : 0 ) );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1%% Error</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 ( int ) ( data.mean ? ( ( mean_error * mean_error * ( ( float ) data.size() ) / ( 0.001 * data.mean * 0.001 * p -> dps.mean ) ) ) : 0 ) );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.1 Scale Factor Error with Delta=300</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 ( int ) ( 2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 30 * 30 ) ) );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.05 Scale Factor Error with Delta=300</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 ( int ) ( 2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 15 * 15 ) ) );

        fprintf( file,
                 "\t\t\t\t\t\t\t\t<tr>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"left\">0.01 Scale Factor Error with Delta=300</td>\n"
                 "\t\t\t\t\t\t\t\t\t<td class=\"right\">%i</td>\n"
                 "\t\t\t\t\t\t\t\t</tr>\n",
                 ( int ) (  2.0 * mean_error * mean_error * ( ( float ) data.size() ) / ( 3 * 3 ) ) );
      }
    }

  }
  fprintf( file,
           "\t\t\t\t\t\t\t\t</table>\n" );
  fprintf( file,
           "\t\t\t\t\t\t\t\t</td>\n"
           "\t\t\t\t\t\t\t</tr>\n" );
  fprintf( file,
           "\t\t\t\t\t\t\t\t</table>\n" );

}

};
#endif
