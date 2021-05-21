// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/sample_data.hpp"
#include "util/timespan.hpp"
#include <string>

struct sim_t;

struct uptime_base_t
{
protected:
  timespan_t last_start;
  timespan_t iteration_uptime_sum;

  uptime_base_t() :
    last_start(timespan_t::min()),
    iteration_uptime_sum(timespan_t::zero())
  {}
  ~uptime_base_t() = default;

  void update(bool is_up, timespan_t current_time);

  void datacollection_begin()
  {
    iteration_uptime_sum = timespan_t::zero();
  }

  void reset()
  {
    last_start = timespan_t::min();
  }
};

struct uptime_simple_t : public uptime_base_t
{
  simple_sample_data_t uptime_sum;

  uptime_simple_t() = default;

  using uptime_base_t::update;
  using uptime_base_t::datacollection_begin;
  using uptime_base_t::reset;

  void merge(const uptime_simple_t& other)
  {
    uptime_sum.merge(other.uptime_sum);
  }

  void datacollection_end(timespan_t t)
  {
    uptime_sum.add(t != timespan_t::zero() ? iteration_uptime_sum / t : 0.0);
  }
};


struct uptime_t : public uptime_base_t
{
  std::string name_str;

  extended_sample_data_t uptime_sum;
  extended_sample_data_t uptime_instance;

  uptime_t(util::string_view n) :
    uptime_base_t(),
    name_str(n),
    uptime_sum("Uptime", true),
    uptime_instance("Duration", true)
  {}

  using uptime_base_t::datacollection_begin;
  using uptime_base_t::reset;

  void update(bool is_up, timespan_t current_time);

  void analyze()
  {
    uptime_sum.analyze();
    uptime_instance.analyze();
  }

  void merge(const uptime_t& other)
  {
    uptime_sum.merge(other.uptime_sum);
    uptime_instance.merge(other.uptime_instance);
  }

  void datacollection_end(timespan_t t)
  {
    uptime_sum.add(t != timespan_t::zero() ? iteration_uptime_sum / t : 0.0);
  }

  const std::string& name() const
  {
    return name_str;
  }

  uptime_t* collect_uptime(sim_t& sim, bool collect = true);

  uptime_t* collect_duration(sim_t& sim, bool collect = true);
};
