// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/generic.hpp"
#include "util/sample_data.hpp"

/* Luxurious sample data container with automatic merge/analyze,
 * intended to be used in class modules for custom reporting.
 * Iteration based sampling
 */
struct sample_data_helper_t : public extended_sample_data_t, private noncopyable
{
  sample_data_helper_t(util::string_view n, bool simple) :
    extended_sample_data_t(n, simple),
    buffer_value(0.0)
  {
  }

  void add(double x)
  {
    buffer_value += x;
  }

  void datacollection_begin()
  {
    reset_buffer();
  }
  void datacollection_end()
  {
    write_buffer_as_sample();
  }
private:
  double buffer_value;
  void write_buffer_as_sample()
  {
    extended_sample_data_t::add(buffer_value);
    reset_buffer();
  }
  void reset_buffer()
  {
    buffer_value = 0.0;
  }
};