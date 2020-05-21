// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include <string>
#include <iosfwd>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>

#include "sc_timespan.hpp"

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
  option_t( const std::string& name ) :
    _name( name )
{ }
  virtual ~option_t() { }
  opts::parse_status parse( sim_t* sim , const std::string& n, const std::string& value ) const;
  std::string name() const
  { return _name; }
  std::ostream& print( std::ostream& stream ) const
  { return do_print( stream ); }
protected:
  virtual opts::parse_status do_parse( sim_t*, const std::string& name, const std::string& value ) const = 0;
  virtual std::ostream& do_print( std::ostream& stream ) const = 0;
private:
  std::string _name;
};


namespace opts {

typedef std::function<parse_status(parse_status, const std::string&, const std::string&)> parse_status_fn_t;
typedef std::unordered_map<std::string, std::string> map_t;
typedef std::unordered_map<std::string, std::vector<std::string>> map_list_t;
typedef std::function<bool(sim_t*,const std::string&, const std::string&)> function_t;
typedef std::function<opts::parse_status(sim_t*,const std::string&, const std::string&)> function_unfilterd_t;
typedef std::vector<std::string> list_t;

parse_status parse( sim_t*, const std::vector<std::unique_ptr<option_t>>&, const std::string& name, const std::string& value, const parse_status_fn_t& fn = nullptr );
void parse( sim_t*, const std::string& context, const std::vector<std::unique_ptr<option_t>>&, const std::string& options_str, const parse_status_fn_t& fn = nullptr );
void parse( sim_t*, const std::string& context, const std::vector<std::unique_ptr<option_t>>&, const std::vector<std::string>& strings, const parse_status_fn_t& fn = nullptr );
}
inline std::ostream& operator<<( std::ostream& stream, const std::unique_ptr<option_t>& opt )
{ return opt -> print( stream ); }

std::unique_ptr<option_t> opt_string( const std::string& n, std::string& v );
std::unique_ptr<option_t> opt_append( const std::string& n, std::string& v );
std::unique_ptr<option_t> opt_bool( const std::string& n, int& v );
std::unique_ptr<option_t> opt_bool( const std::string& n, bool& v );
std::unique_ptr<option_t> opt_uint64( const std::string& n, uint64_t& v );
std::unique_ptr<option_t> opt_int( const std::string& n, int& v );
std::unique_ptr<option_t> opt_int( const std::string& n, int& v, int , int );
std::unique_ptr<option_t> opt_uint( const std::string& n, unsigned& v );
std::unique_ptr<option_t> opt_uint( const std::string& n, unsigned& v, unsigned , unsigned  );
std::unique_ptr<option_t> opt_float( const std::string& n, double& v );
std::unique_ptr<option_t> opt_float( const std::string& n, double& v, double , double  );
std::unique_ptr<option_t> opt_timespan( const std::string& n, timespan_t& v );
std::unique_ptr<option_t> opt_timespan( const std::string& n, timespan_t& v, timespan_t , timespan_t  );
std::unique_ptr<option_t> opt_list( const std::string& n, opts::list_t& v );
std::unique_ptr<option_t> opt_map( const std::string& n, opts::map_t& v );
std::unique_ptr<option_t> opt_map_list( const std::string& n, opts::map_list_t& v );
std::unique_ptr<option_t> opt_func( const std::string& n, const opts::function_t& f );
std::unique_ptr<option_t> opt_func_unfiltered( const std::string& n, const opts::function_unfilterd_t& f );
std::unique_ptr<option_t> opt_deprecated( const std::string& n, const std::string& new_option );
std::unique_ptr<option_t> opt_obsoleted( const std::string& n );

struct option_tuple_t
{
  std::string scope, name, value;
  option_tuple_t( const std::string& s, const std::string& n, const std::string& v ) : scope( s ), name( n ), value( v ) {}
};

struct option_db_t : public std::vector<option_tuple_t>
{
  std::vector<std::string> auto_path;
  std::unordered_map<std::string, std::string> var_map;

  option_db_t();
  void add( const std::string& scope, const std::string& name, const std::string& value )
  {
    push_back( option_tuple_t( scope, name, value ) );
  }
  bool parse_file( std::istream& file );
  void parse_token( const std::string& token );
  void parse_line( const std::string& line );
  void parse_text( const std::string& text );
  void parse_args( const std::vector<std::string>& args );
};
