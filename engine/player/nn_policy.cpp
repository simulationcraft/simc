#include "player/nn_policy.hpp"
#include "util/util.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace
{
constexpr int SUPPORTED_VERSION = 1;

bool is_space( char c )
{ return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

std::string trim( const std::string& s )
{
  std::size_t b = 0, e = s.size();
  while ( b < e && is_space( s[ b ] ) ) ++b;
  while ( e > b && is_space( s[ e - 1 ] ) ) --e;
  return s.substr( b, e - b );
}

class line_reader_t
{
  std::istream& in;
  std::string source;
  std::size_t line_no = 0;

public:
  line_reader_t( std::istream& in, std::string source ) : in( in ), source( std::move( source ) ) {}

  // Next non-empty, non-comment line (trimmed). Returns false at end of file.
  bool next( std::string& line )
  {
    std::string raw;
    while ( std::getline( in, raw ) )
    {
      ++line_no;
      line = trim( raw );
      if ( line.empty() || line[ 0 ] == '#' ) continue;
      return true;
    }
    if ( in.bad() ) fail( "read error" );
    return false;
  }

  std::string expect( const std::string& what )
  {
    std::string line;
    if ( !next( line ) ) fail( fmt::format( "unexpected end of file, expected {}", what ) );
    return line;
  }

  [[noreturn]] void fail( const std::string& msg ) const
  { throw std::runtime_error( fmt::format( "{}:{}: {}", source, line_no, msg ) ); }

  // Appends exactly `expected` numbers from `line` to `out`.
  void parse_numbers( const std::string& line, std::size_t expected, std::vector<double>& out ) const
  {
    const char* p = line.c_str();
    std::size_t count = 0;
    while ( true )
    {
      while ( is_space( *p ) ) ++p;
      if ( *p == '\0' ) break;
      char* end = nullptr;
      double v = std::strtod( p, &end );
      if ( end == p || ( *end != '\0' && !is_space( *end ) ) )
        fail( "invalid number" );
      if ( !std::isfinite( v ) ) fail( "non-finite number" );
      out.push_back( v );
      ++count;
      p = end;
    }
    if ( count != expected )
      fail( fmt::format( "expected {} numbers, got {}", expected, count ) );
  }

  // Parses "<keyword> <count>".
  std::size_t expect_section( const std::string& keyword )
  {
    auto tokens = util::string_split( expect( fmt::format( "'{}'", keyword ) ), " " );
    if ( tokens.size() != 2 || tokens[ 0 ] != keyword ) fail( fmt::format( "expected '{} <count>'", keyword ) );
    std::size_t n = util::to_unsigned( tokens[ 1 ] );
    if ( n == 0 ) fail( fmt::format( "'{}' count must be positive", keyword ) );
    return n;
  }
};

nn_activation_e parse_activation( const std::string& tok, const line_reader_t& r )
{
  if ( tok == "linear" ) return nn_activation_e::LINEAR;
  if ( tok == "relu" ) return nn_activation_e::RELU;
  if ( tok == "tanh" ) return nn_activation_e::TANH;
  r.fail( fmt::format( "unknown activation '{}' (expected linear, relu or tanh)", tok ) );
}
}  // namespace

nn_policy_data_t read_nn_policy( const std::string& path )
{
  std::ifstream in( path );
  if ( !in ) throw std::runtime_error( fmt::format( "cannot open policy file '{}'", path ) );

  line_reader_t r( in, path );
  nn_policy_data_t data;

  // Version
  {
    auto tokens = util::string_split( r.expect( "'version'" ), " " );
    if ( tokens.size() != 2 || tokens[ 0 ] != "version" ) r.fail( "expected 'version <n>'" );
    data.version = util::to_int( tokens[ 1 ] );
    if ( data.version != SUPPORTED_VERSION )
      r.fail( fmt::format( "unsupported version {} (expected {})", tokens[ 1 ], SUPPORTED_VERSION ) );
  }

  // Inputs
  std::size_t n_inputs = r.expect_section( "inputs" );
  data.inputs.reserve( n_inputs );
  for ( std::size_t i = 0; i < n_inputs; ++i )
  {
    auto tokens = util::string_split( r.expect( "input line" ), " " );
    if ( tokens.size() != 3 ) r.fail( "expected '<expression> <offset> <scale>'" );
    nn_input_t input;
    input.expression = tokens[ 0 ];
    input.offset     = util::to_double( tokens[ 1 ] );
    input.scale      = util::to_double( tokens[ 2 ] );
    if ( input.scale == 0.0 ) r.fail( "scale must be nonzero" );
    data.inputs.push_back( std::move( input ) );
  }

  // Actions
  std::size_t n_actions = r.expect_section( "actions" );
  data.actions.reserve( n_actions );
  std::unordered_set<std::string> seen;
  for ( std::size_t i = 0; i < n_actions; ++i )
  {
    std::string line = r.expect( "action line" );
    if ( !seen.insert( line ).second ) r.fail( fmt::format( "duplicate action '{}'", line ) );
    data.actions.push_back( std::move( line ) );
  }

  // Layers, until end of file
  std::string line;
  std::size_t expected_in = n_inputs;
  while ( r.next( line ) )
  {
    auto tokens = util::string_split( line, " " );
    if ( tokens.size() != 4 || tokens[ 0 ] != "layer" ) r.fail( "expected 'layer <n_in> <n_out> <activation>'" );

    nn_layer_t layer;
    layer.n_in       = util::to_unsigned( tokens[ 1 ] );
    layer.n_out      = util::to_unsigned( tokens[ 2 ] );
    layer.activation = parse_activation( tokens[ 3 ], r );
    if ( layer.n_out == 0 ) r.fail( "layer output size must be positive" );
    if ( layer.n_in != expected_in )
      r.fail( fmt::format( "layer input size {} does not match previous size {}", tokens[ 1 ], expected_in ) );

    layer.weights.reserve( layer.n_in * layer.n_out );
    for ( std::size_t row = 0; row < layer.n_out; ++row )
      r.parse_numbers( r.expect( "weight row" ), layer.n_in, layer.weights );

    layer.bias.reserve( layer.n_out );
    r.parse_numbers( r.expect( "bias line" ), layer.n_out, layer.bias );

    expected_in = layer.n_out;
    data.layers.push_back( std::move( layer ) );
  }

  if ( data.layers.empty() ) r.fail( "no layers defined" );
  if ( expected_in != n_actions )
    r.fail( fmt::format( "last layer output size {} does not match action count {}", expected_in, n_actions ) );

  return data;
}
