#include "player/nn_policy.hpp"
#include "player/nn_trajectory.hpp"
#include "sim/sim.hpp"
#include "report/reports.hpp"
#include "util/util.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <type_traits>

namespace
{
bool host_is_little_endian()
{
  const uint32_t probe = 1;
  unsigned char first;
  std::memcpy( &first, &probe, 1 );
  return first == 1;
}

// Writes the array in little-endian byte order regardless of the host. On little-endian
// hosts this is a single write; on big-endian hosts the elements are byte-swapped through
// a fixed-size buffer. Assumes floats share the byte order of integers (true on every
// current platform with IEEE 754 floats).
template <typename T>
void write_array( std::ofstream& out, const std::vector<T>& v, std::size_t expected, const char* name )
{
  static_assert( std::is_trivially_copyable<T>::value, "write_array needs trivially copyable elements" );
  if ( v.size() != expected )
    throw std::runtime_error( fmt::format( "write_nn_trajectory: array '{}' has wrong size", name ) );
  if ( v.empty() )
    return;

  if ( sizeof( T ) == 1 || host_is_little_endian() )
  {
    out.write( reinterpret_cast<const char*>( v.data() ), static_cast<std::streamsize>( v.size() * sizeof( T ) ) );
    return;
  }

  constexpr std::size_t CHUNK = 65536;  // elements per write
  std::vector<char> buf( std::min( CHUNK, v.size() ) * sizeof( T ) );
  for ( std::size_t start = 0; start < v.size(); start += CHUNK )
  {
    const std::size_t count = std::min( CHUNK, v.size() - start );
    const char* src = reinterpret_cast<const char*>( v.data() + start );
    for ( std::size_t i = 0; i < count; ++i )
      for ( std::size_t b = 0; b < sizeof( T ); ++b )
        buf[ i * sizeof( T ) + b ] = src[ i * sizeof( T ) + ( sizeof( T ) - 1 - b ) ];
    out.write( buf.data(), static_cast<std::streamsize>( count * sizeof( T ) ) );
  }
}

std::string format_double( double v )
{
  char buf[ 64 ];
  std::snprintf( buf, sizeof buf, "%.17g", v );
  return buf;
}

// Writes a merged trajectory to disk for the Python trainer.
//
// File layout: a text header (one item per line) followed by raw arrays, back to
// back, in this order. The arrays are always little-endian, whatever the host's byte
// order (big-endian hosts byte-swap on write):
//
//   nn_trajectory 1
//   steps <S>
//   episodes <E>
//   raw_limit <value>          clamp applied to raw expression values
//   norm_limit <value>         clamp applied to normalized features
//   inputs <N>
//   <expression> <offset> <scale>     (N lines, copied from the policy file)
//   actions <M>
//   <action line>                     (M lines, copied from the policy file)
//   data
//   features  float32[S * N]   row-major, raw values
//   mask      uint8  [S * M]   row-major
//   action    uint32 [S]
//   log_prob  float32[S]
//   time      float64[S]
//   reward    float64[S]
//   episode   uint32 [S]
//   terminal  uint8  [S]
//
// The file is written to "<path>.tmp" and renamed into place, so a reader never
// sees a half-written file.
void write_nn_trajectory( const std::string& path, const nn_trajectory_t& traj, const nn_policy_data_t& policy )
{
  static_assert( sizeof( float ) == 4 && sizeof( double ) == 8, "unexpected floating point sizes" );
  if ( traj.n_inputs != policy.input_size() || traj.n_actions != policy.output_size() )
    throw std::runtime_error( "write_nn_trajectory: trajectory layout does not match policy" );

  const std::size_t S = traj.size();
  const std::string tmp_path = path + ".tmp";
  {
    std::ofstream out( tmp_path, std::ios::binary | std::ios::trunc );
    if ( !out ) throw std::runtime_error( fmt::format( "write_nn_trajectory: cannot open '{}'", tmp_path ) );

    std::string header;
    header += "nn_trajectory 1\n";
    header += "steps " + std::to_string( S ) + "\n";
    header += "episodes " + std::to_string( traj.episodes() ) + "\n";
    header += "raw_limit " + format_double( RAW_LIMIT ) + "\n";
    header += "norm_limit " + format_double( NORM_LIMIT ) + "\n";
    header += "inputs " + std::to_string( policy.inputs.size() ) + "\n";
    for ( const nn_input_t& in : policy.inputs )
      header += in.expression + " " + format_double( in.offset ) + " " + format_double( in.scale ) + "\n";
    header += "actions " + std::to_string( policy.actions.size() ) + "\n";
    for ( const std::string& a : policy.actions )
      header += a + "\n";
    header += "data\n";
    out.write( header.data(), static_cast<std::streamsize>( header.size() ) );

    write_array( out, traj.features, S * traj.n_inputs, "features" );
    write_array( out, traj.mask, S * traj.n_actions, "mask" );
    write_array( out, traj.action, S, "action" );
    write_array( out, traj.log_prob, S, "log_prob" );
    write_array( out, traj.time, S, "time" );
    write_array( out, traj.reward, S, "reward" );
    write_array( out, traj.episode, S, "episode" );
    write_array( out, traj.terminal, S, "terminal" );

    out.flush();
    if ( !out ) throw std::runtime_error( fmt::format( "write_nn_trajectory: write to '{}' failed", tmp_path ) );
  }

  std::remove( path.c_str() );
  if ( std::rename( tmp_path.c_str(), path.c_str() ) != 0 )
    throw std::runtime_error( fmt::format( "write_nn_trajectory: cannot rename '{}' to '{}'", tmp_path, path ) );
}
}  // namespace

namespace report
{
void print_trajectories( sim_t& s )
{
  for ( auto p : s.player_no_pet_list )
  {
    if ( p->parsed_policy && p->trajectory )
    {
      assert( !p->trajectory_out.empty() );
      write_nn_trajectory( p->trajectory_out, *p->trajectory, *p->parsed_policy );
    }
  }
}
}  // namespace report
