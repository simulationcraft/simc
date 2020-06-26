// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/string_view.hpp"

#include "fmt/core.h"
#include "fmt/printf.h"

#include <iosfwd>

struct sim_t;

/* Unformatted SimC output class.
 */
struct sc_raw_ostream_t {
  template <typename T>
  sc_raw_ostream_t& operator<< (T const& rhs)
  {
    print( "{}", rhs );
    return *this;
  }

  sc_raw_ostream_t& operator<< (const char* rhs);

  template <typename... Args>
  sc_raw_ostream_t& print( util::string_view format, Args&& ... args )
  {
    vprint( format, fmt::make_format_args( std::forward<Args>(args)... ) );
    return *this;
  }

  sc_raw_ostream_t( std::shared_ptr<std::ostream> os ) :
    _stream( os ) {}
  const sc_raw_ostream_t operator=( std::shared_ptr<std::ostream> os )
  { _stream = os; return *this; }
  std::ostream* get_stream()
  { return _stream.get(); }

  void vprint( util::string_view format, fmt::format_args args );

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

  template <typename T>
  sim_ostream_t& operator<< (T const& rhs)
  {
    print( "{}", rhs );
    return *this;
  }

  sim_ostream_t& operator<< (const char* rhs);

  template <typename... Args>
  sim_ostream_t& printf( util::string_view format, Args&& ... args )
  {
    vprintf( format, fmt::make_printf_args( std::forward<Args>(args)... ) );
    return *this;
  }

  /**
   * Print using fmt libraries python-like formatting syntax.
   */
  template <typename... Args>
  sim_ostream_t& print( util::string_view format, Args&& ... args)
  {
    vprint( format, fmt::make_format_args( std::forward<Args>(args)... ) );
    return *this;
  }
private:
  static void dont_close( std::ostream* ) {}
  void print_simulation_time();
  void vprintf( util::string_view format, fmt::printf_args args );
  void vprint( util::string_view format, fmt::format_args args );

  sim_t& sim;
  sc_raw_ostream_t _raw;
};

