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

/// Control proc information in various Simulationcraft reports
enum proc_report_e : unsigned
{
  REPORT_PROC_HTML = 0x1U,    /// Include proc information in HTML report
  REPORT_PROC_TEXT = 0x2U,    /// Include proc information in textual report
  REPORT_PROC_JSON = 0x4U,    /// Include proc information in JSON report

  /// Include proc information in all report types (default)
  REPORT_PROC_ALL = REPORT_PROC_HTML | REPORT_PROC_TEXT | REPORT_PROC_JSON,
  REPORT_PROC_QUIET = 0x0     /// Suppress all reporting
};

struct proc_t : private noncopyable
{
private:
  sim_t& sim;
  size_t iteration_count; // track number of procs during the current iteration
  timespan_t last_proc; // track time of the last proc
public:
  const std::string name_str;
  unsigned proc_report_flags; // Don't output proc in reporting

  // These are initialized in SIMPLE mode. Only change mode for infrequent procs to keep memory usage reasonable.
  extended_sample_data_t interval_sum;
  extended_sample_data_t count;

  proc_t( sim_t& s, util::string_view n, unsigned flags = proc_report_e::REPORT_PROC_ALL );

  proc_t* set_proc_report_flags( proc_report_e flag )
  {
    if ( flag == proc_report_e::REPORT_PROC_QUIET )
    {
      proc_report_flags = flag;
    }
    else
    {
      proc_report_flags |= flag;
    }

    return this;
  }

  proc_t* unset_proc_report_flags( proc_report_e flag )
  {
    if ( flag == proc_report_e::REPORT_PROC_QUIET )
    {
      proc_report_flags = REPORT_PROC_ALL;
    }
    else
    {
      proc_report_flags &= ~flag;
    }

    return this;
  }

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
