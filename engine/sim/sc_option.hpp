// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include <string>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <cstring>
#include <sstream>

#include "sc_timespan.hpp"

struct sim_t;

// Options ==================================================================

struct option_t
{
public:
  option_t( const std::string& name ) :
    _name( name )
{ }
  virtual ~option_t() { }
  bool parse_option( sim_t* sim , const std::string& n, const std::string& value ) const
  {
    try {
      return parse( sim, n, value );
    }
    catch ( const std::exception& e) {
      std::stringstream s;
      s << "Could not parse option '" << n << "' with value '" << value << "': " << e.what();
      throw std::invalid_argument(s.str());
    }
  }
  std::string name() const
  { return _name; }
  std::ostream& print_option( std::ostream& stream ) const
  { return print( stream ); }
protected:
  virtual bool parse( sim_t*, const std::string& name, const std::string& value ) const = 0;
  virtual std::ostream& print( std::ostream& stream ) const = 0;
private:
  std::string _name;
};


namespace opts {

typedef std::unordered_map<std::string, std::string> map_t;
typedef std::unordered_map<std::string, std::vector<std::string>> map_list_t;
typedef std::function<bool(sim_t*,const std::string&, const std::string&)> function_t;
typedef std::vector<std::string> list_t;
bool parse( sim_t*, const std::vector<std::unique_ptr<option_t>>&, const std::string& name, const std::string& value );
void parse( sim_t*, const std::string& context, const std::vector<std::unique_ptr<option_t>>&, const std::string& options_str );
void parse( sim_t*, const std::string& context, const std::vector<std::unique_ptr<option_t>>&, const std::vector<std::string>& strings );
}
inline std::ostream& operator<<( std::ostream& stream, const std::unique_ptr<option_t>& opt )
{ return opt -> print_option( stream ); }

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
std::unique_ptr<option_t> opt_deprecated( const std::string& n, const std::string& new_option );

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
  bool parse_file( FILE* file );
  void parse_token( const std::string& token );
  void parse_line( const std::string& line );
  void parse_text( const std::string& text );
  void parse_args( const std::vector<std::string>& args );
};
