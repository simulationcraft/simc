// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/sample_data.hpp"
#include "sc_timespan.hpp"
#include <string>

struct sim_t;

struct uptime_common_t
{
protected:
  timespan_t last_start;
  timespan_t iteration_uptime_sum;

public:
  uptime_common_t() :
    last_start(timespan_t::min()),
    iteration_uptime_sum(timespan_t::zero())
  {}

  virtual ~uptime_common_t() {}

  virtual void update(bool is_up, timespan_t current_time);

  virtual void merge(const uptime_common_t&) {}

  virtual void datacollection_end(timespan_t) {}

  void datacollection_begin()
  {
    iteration_uptime_sum = timespan_t::zero();
  }

  void reset()
  {
    last_start = timespan_t::min();
  }
};

struct uptime_simple_t : public uptime_common_t
{
  simple_sample_data_t uptime_sum;

  uptime_simple_t() : uptime_common_t(),
    uptime_sum()
  {}

  void merge(const uptime_common_t& other) override
  {
    uptime_sum.merge(dynamic_cast<const uptime_simple_t&>(other).uptime_sum);
  }

  void datacollection_end(timespan_t t) override
  {
    uptime_sum.add(t != timespan_t::zero() ? iteration_uptime_sum / t : 0.0);
  }
};


struct uptime_t : public uptime_common_t
{
public:
  std::string name_str;
  sim_t& sim;

  extended_sample_data_t uptime_sum;
  extended_sample_data_t uptime_instance;

  uptime_t(sim_t& s, const std::string& n) : uptime_common_t(),
    name_str(n),
    sim(s),
    uptime_sum("Uptime", true),
    uptime_instance("Duration", true)
  {}

  void update(bool is_up, timespan_t current_time) override;

  void analyze()
  {
    uptime_sum.analyze();
    uptime_instance.analyze();
  }

  void merge(const uptime_common_t& other) override
  {
    uptime_sum.merge(dynamic_cast<const uptime_t&>(other).uptime_sum);
    uptime_instance.merge(dynamic_cast<const uptime_t&>(other).uptime_instance);
  }

  void datacollection_end(timespan_t t) override
  {
    uptime_sum.add(t != timespan_t::zero() ? iteration_uptime_sum / t : 0.0);
  }

  const char* name() const
  {
    return name_str.c_str();
  }

  uptime_t* collect_uptime(bool collect = true);

  uptime_t* collect_duration(bool collect = true);
};
