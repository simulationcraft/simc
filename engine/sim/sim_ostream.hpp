// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/string_view.hpp"

#include "fmt/core.h"
#include "fmt/printf.h"
#include "fmt/ostream.h"

struct sim_t;

/* Unformatted SimC output class.
 */
struct sc_raw_ostream_t {
  template <class T>
  sc_raw_ostream_t & operator<< (T const& rhs)
  { (*_stream) << rhs; return *this; }

  template <typename... Args>
  sc_raw_ostream_t& print( util::string_view format, Args&& ... args )
  {
    fmt::print( *get_stream(), format, std::forward<Args>(args)... );
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
  sim_ostream_t & operator<< (T const& rhs)
  {
    print_simulation_time();
    _raw << rhs << "\n";

    return *this;
  }

  template <typename... Args>
  sim_ostream_t& printf( util::string_view format, Args&& ... args )
  {
    print_simulation_time();
    fmt::fprintf(*_raw.get_stream(), format, std::forward<Args>(args)... );
    _raw << "\n";
    return *this;
  }

  /**
   * Print using fmt libraries python-like formatting syntax.
   */
  template <typename... Args>
  sim_ostream_t& print( util::string_view format, Args&& ... args)
  {
    print_simulation_time();
    fmt::print( *_raw.get_stream(), format, std::forward<Args>(args)... );
    _raw << '\n';
    return *this;
  }
private:
  static void dont_close( std::ostream* ) {}
  void print_simulation_time();
  sim_t& sim;
  sc_raw_ostream_t _raw;
};

