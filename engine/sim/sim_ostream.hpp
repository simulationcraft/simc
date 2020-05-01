// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

/* Unformatted SimC output class.
 */
struct sc_raw_ostream_t {
  template <class T>
  sc_raw_ostream_t & operator<< (T const& rhs)
  { (*_stream) << rhs; return *this; }

  template<typename Format, typename... Args>
  sc_raw_ostream_t& printf(Format&& format, Args&& ... args)
  {
    fmt::fprintf(*get_stream(), std::forward<Format>(format), std::forward<Args>(args)... );
    return *this;
  }

  sc_raw_ostream_t( std::shared_ptr<std::ostream> os ) :
    _stream( os ) {}
  const sc_raw_ostream_t operator=( std::shared_ptr<std::ostream> os )
  { _stream = os; return *this; }
  std::ostream* get_stream()
  { return _stream.get(); }
private:
  std::shared_ptr<std::ostream> _stream;
};

/* Formatted SimC output class.
 */
struct sim_ostream_t
{
  struct no_close {};

  explicit sim_ostream_t( sim_t& s, std::shared_ptr<std::ostream> os ) :
      sim(s),
    _raw( os )
  {
  }
  sim_ostream_t( sim_t& s, std::ostream* os, no_close ) :
      sim(s),
    _raw( std::shared_ptr<std::ostream>( os, dont_close ) )
  {}
  const sim_ostream_t operator=( std::shared_ptr<std::ostream> os )
  { _raw = os; return *this; }

  sc_raw_ostream_t& raw()
  { return _raw; }

  std::ostream* get_stream()
  { return _raw.get_stream(); }
  template <class T>
  sim_ostream_t & operator<< (T const& rhs);

  template<typename Format, typename... Args>
  sim_ostream_t& printf(Format&& format, Args&& ... args);

  /**
   * Print using fmt libraries python-like formatting syntax.
   */
  template<typename Format, typename... Args>
  sim_ostream_t& print(Format&& format, Args&& ... args)
  {
    *this << fmt::format(std::forward<Format>(format), std::forward<Args>(args)... );
    return *this;
  }
private:
  static void dont_close( std::ostream* ) {}
  void print_simulation_time();
  sim_t& sim;
  sc_raw_ostream_t _raw;
};


template<typename Format, typename... Args>
sim_ostream_t& sim_ostream_t::printf(Format&& format, Args&& ... args)
{
  print_simulation_time();
  fmt::fprintf(*_raw.get_stream(), std::forward<Format>(format), std::forward<Args>(args)... );
  _raw << "\n";
  return *this;
}