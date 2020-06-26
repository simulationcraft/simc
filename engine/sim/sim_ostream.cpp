#include "sim_ostream.hpp"

#include "sim/sc_sim.hpp"

sc_raw_ostream_t& sc_raw_ostream_t::operator<< (const char* rhs)
{
  *_stream << rhs;
  return *this;
}

void sc_raw_ostream_t::vprint( util::string_view format, fmt::format_args args )
{
  fmt::vprint( *_stream, to_string_view( format ), args );
}

sim_ostream_t& sim_ostream_t::operator<< (const char* rhs)
{
  print_simulation_time();
  _raw << rhs;
  *_raw.get_stream() << '\n';
  return *this;
}

void sim_ostream_t::print_simulation_time()
{
  _raw.print( "{:.3f}", sim.current_time().total_seconds() );
}

void sim_ostream_t::vprintf( util::string_view format, fmt::printf_args args )
{
  print_simulation_time();
  fmt::vfprintf( *_raw.get_stream(), to_string_view( format ), args );
  *_raw.get_stream() << '\n';
}

void sim_ostream_t::vprint( util::string_view format, fmt::format_args args )
{
  print_simulation_time();
  _raw.vprint( format, args );
  *_raw.get_stream() << '\n';
}
