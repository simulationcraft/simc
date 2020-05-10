// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_enums.hpp"
#include <string>

struct scaling_metric_data_t {
  std::string name;
  double value, stddev;
  scale_metric_e metric;
  scaling_metric_data_t( scale_metric_e m, const std::string& n, double v, double dev ) :
    name( n ), value( v ), stddev( dev ), metric( m ) {}
  scaling_metric_data_t( scale_metric_e m, const extended_sample_data_t& sd ) :
    name( sd.name_str ), value( sd.mean() ), stddev( sd.mean_std_dev ), metric( m ) {}
  scaling_metric_data_t( scale_metric_e m, const sc_timeline_t& tl, const std::string& name ) :
    name( name ), value( tl.mean() ), stddev( tl.mean_stddev() ), metric( m ) {}
};