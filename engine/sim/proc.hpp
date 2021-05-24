// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/sample_data.hpp"
#include "util/string_view.hpp"
#include "util/timespan.hpp"

#include <string>

struct sim_t;

// Proc =====================================================================

struct proc_t : private noncopyable
{
private:
  sim_t& sim;
  size_t iteration_count; // track number of procs during the current iteration
  timespan_t last_proc; // track time of the last proc
public:
  const std::string name_str;
  // These are initialized in SIMPLE mode. Only change mode for infrequent procs to keep memory usage reasonable.
  extended_sample_data_t interval_sum;
  extended_sample_data_t count;

  proc_t(sim_t& s, util::string_view n);

  void occur();

  void reset();

  void merge(const proc_t& other);

  void analyze();

  void datacollection_begin();

  void datacollection_end();

  const std::string& name() const;

  proc_t* collect_count(bool collect = true);

  proc_t* collect_interval(bool collect = true);
};
