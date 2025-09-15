// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>

#include "util/timespan.hpp"
#include "util/string_view.hpp"
#include "util/span.hpp"
#include "util/format.hpp"

struct sim_t;

namespace opts
{
// Status of the parsing operation for each option_t object
enum class parse_status : unsigned
{
  FAILURE = 0u, /// Parse failure, the user gave invalid input
  OK,           /// Parsing succeeded
  NOT_FOUND,    /// Option with specific name not found in the option database
  CONTINUE,     /// Parsing should continue (e.g. name, value pair not meant for this option_t object)
  DEPRECATED,   /// Option is deprecated, parsing failure (use non-deprecated option instead)
  WARNING       /// Parsing successful, but warn of an issue (such as string being overwritten)
};
} // Namespace opts ends

// Options ==================================================================

struct option_t
{
public:
  option_t( util::string_view name ) :
    _name( name )
{ }
  virtual ~option_t() = default;
  opts::parse_status parse( sim_t* sim, util::string_view name, util::string_view value ) const;
  util::string_view name() const
  { return _name; }
  
  friend void sc_format_to( const option_t&, fmt::format_context::iterator );
protected:
  virtual opts::parse_status do_parse( sim_t*, util::string_view name, util::string_view value ) const = 0;
  virtual void do_format_to( fmt::format_context::iterator ) const = 0;
private:
  std::string _name;
};


namespace opts {

using parse_status_fn_t = std::function<parse_status( parse_status, util::string_view, util::string_view )>;
using map_t             = std::unordered_map<std::string, std::string>;
using map_list_t        = std::unordered_map<std::string, std::vector<std::string>>;
using function_t        = std::function<bool( sim_t*, util::string_view, util::string_view )>;
using list_t            = std::vector<std::string>;

parse_status parse( sim_t*, util::span<const std::unique_ptr<option_t>>, util::string_view name, util::string_view value, const parse_status_fn_t& fn = nullptr );
void parse( sim_t*, util::string_view context, util::span<const std::unique_ptr<option_t>>, util::string_view options_str, const parse_status_fn_t& fn = nullptr );
}

inline void sc_format_to( const std::unique_ptr<option_t>& option, fmt::format_context::iterator out )
{ 
  sc_format_to(*option, out);
}

std::unique_ptr<option_t> opt_string( std::string_view n, std::string& v );
std::unique_ptr<option_t> opt_string_warn( std::string_view n, std::string& v );
std::unique_ptr<option_t> opt_append( std::string_view n, std::string& v );
std::unique_ptr<option_t> opt_bool( std::string_view n, int& v );
std::unique_ptr<option_t> opt_bool( std::string_view n, bool& v );
std::unique_ptr<option_t> opt_uint64( std::string_view n, uint64_t& v );
std::unique_ptr<option_t> opt_int( std::string_view n, int& v );
std::unique_ptr<option_t> opt_int( std::string_view n, int& v, int , int );
std::unique_ptr<option_t> opt_uint( std::string_view n, unsigned& v );
std::unique_ptr<option_t> opt_uint( std::string_view n, unsigned& v, unsigned , unsigned  );
std::unique_ptr<option_t> opt_float( std::string_view n, double& v );
std::unique_ptr<option_t> opt_float( std::string_view n, double& v, double , double  );
std::unique_ptr<option_t> opt_timespan( std::string_view n, timespan_t& v );
std::unique_ptr<option_t> opt_timespan( std::string_view n, timespan_t& v, timespan_t , timespan_t  );
std::unique_ptr<option_t> opt_list( std::string_view n, opts::list_t& v );
std::unique_ptr<option_t> opt_map( std::string_view n, opts::map_t& v );
std::unique_ptr<option_t> opt_map_list( std::string_view n, opts::map_list_t& v );
std::unique_ptr<option_t> opt_func( std::string_view n, const opts::function_t& f );
std::unique_ptr<option_t> opt_deprecated( std::string_view n, std::string_view new_option );
std::unique_ptr<option_t> opt_obsoleted( std::string_view n );

struct option_tuple_t
{
  std::string scope, name, value;
  option_tuple_t( util::string_view s, util::string_view n, util::string_view v ) : scope( s ), name( n ), value( v ) {}
};

struct option_db_t : public std::vector<option_tuple_t>
{
  std::vector<std::string> auto_path;
  std::unordered_map<std::string, std::string> var_map;

  option_db_t();
  void add( util::string_view scope, util::string_view name, util::string_view value )
  {
    emplace_back( scope, name, value );
  }
  bool parse_file( std::istream& input );
  void parse_token( util::string_view token );
  void parse_line( util::string_view line );
  void parse_text( util::string_view text );
  void parse_args( util::span<const std::string> args );
};
