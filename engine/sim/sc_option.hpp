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
  DEPRECATED    /// Option is deprecated, parsing failure (use non-deprecated option instead)
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
  
  friend void format_to( const option_t&, fmt::format_context::iterator );
protected:
  virtual opts::parse_status do_parse( sim_t*, util::string_view name, util::string_view value ) const = 0;
  virtual void do_format_to( fmt::format_context::iterator ) const = 0;
private:
  std::string _name;
};


namespace opts {

typedef std::function<parse_status(parse_status, util::string_view name, util::string_view value)> parse_status_fn_t;
typedef std::unordered_map<std::string, std::string> map_t;
typedef std::unordered_map<std::string, std::vector<std::string>> map_list_t;
typedef std::function<bool(sim_t*, util::string_view, util::string_view)> function_t;
typedef std::vector<std::string> list_t;

parse_status parse( sim_t*, util::span<const std::unique_ptr<option_t>>, util::string_view name, util::string_view value, const parse_status_fn_t& fn = nullptr );
void parse( sim_t*, util::string_view context, util::span<const std::unique_ptr<option_t>>, util::string_view options_str, const parse_status_fn_t& fn = nullptr );
}

inline void format_to( const std::unique_ptr<option_t>& option, fmt::format_context::iterator out )
{ 
  format_to(*option, out);
}

std::unique_ptr<option_t> opt_string( util::string_view n, std::string& v );
std::unique_ptr<option_t> opt_append( util::string_view n, std::string& v );
std::unique_ptr<option_t> opt_bool( util::string_view n, int& v );
std::unique_ptr<option_t> opt_bool( util::string_view n, bool& v );
std::unique_ptr<option_t> opt_uint64( util::string_view n, uint64_t& v );
std::unique_ptr<option_t> opt_int( util::string_view n, int& v );
std::unique_ptr<option_t> opt_int( util::string_view n, int& v, int , int );
std::unique_ptr<option_t> opt_uint( util::string_view n, unsigned& v );
std::unique_ptr<option_t> opt_uint( util::string_view n, unsigned& v, unsigned , unsigned  );
std::unique_ptr<option_t> opt_float( util::string_view n, double& v );
std::unique_ptr<option_t> opt_float( util::string_view n, double& v, double , double  );
std::unique_ptr<option_t> opt_timespan( util::string_view n, timespan_t& v );
std::unique_ptr<option_t> opt_timespan( util::string_view n, timespan_t& v, timespan_t , timespan_t  );
std::unique_ptr<option_t> opt_list( util::string_view n, opts::list_t& v );
std::unique_ptr<option_t> opt_map( util::string_view n, opts::map_t& v );
std::unique_ptr<option_t> opt_map_list( util::string_view n, opts::map_list_t& v );
std::unique_ptr<option_t> opt_func( util::string_view n, const opts::function_t& f );
std::unique_ptr<option_t> opt_deprecated( util::string_view n, util::string_view new_option );
std::unique_ptr<option_t> opt_obsoleted( util::string_view n );

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
