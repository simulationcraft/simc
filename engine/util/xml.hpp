// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "../config.hpp"

#if USE_TR1_NAMESPACE
  #include <tr1/memory>
#else
  #include <memory>
#endif

#include <string>
#include <vector>
#include <sstream>
#include <stack>

#include "cache.hpp"
#include "io.hpp"

struct sim_t;

// XML ======================================================================

// XML Reader ==================================================================

struct xml_parm_t
{
  std::string name_str;
  std::string value_str;
  xml_parm_t( const std::string& n, const std::string& v ) : name_str( n ), value_str( v ) {}
  const char* name() { return name_str.c_str(); }
};

struct xml_node_t
{
  std::string name_str;
  std::vector<std::shared_ptr<xml_node_t> > children;
  std::vector<xml_parm_t> parameters;
  xml_node_t() {}
  xml_node_t( const std::string& n ) : name_str( n ) {}
  const char* name() { return name_str.c_str(); }
  xml_node_t* get_child( const std::string& name );
  xml_node_t* get_node ( const std::string& path );
  xml_node_t* get_node ( const std::string& path, const std::string& parm_name, const std::string& parm_value );
  std::vector<xml_node_t*>  get_children( const std::string& name = std::string() );
  std::vector<xml_node_t*>  get_nodes   ( const std::string& path );
  std::vector<xml_node_t*>  get_nodes   ( const std::string& path, const std::string& parm_name, const std::string& parm_value );
  bool get_value( std::string& value, const std::string& path = std::string() );
  bool get_value( int&         value, const std::string& path = std::string() );
  bool get_value( double&      value, const std::string& path = std::string() );
  xml_parm_t* get_parm( const std::string& parm_name );

  std::shared_ptr<xml_node_t> create_node     ( sim_t* sim, const std::string& input, std::string::size_type& index );
  int         create_children ( sim_t* sim, const std::string& input, std::string::size_type& index );
  void        create_parameter( const std::string& input, std::string::size_type& index );

  xml_node_t* search_tree( const std::string& node_name );
  xml_node_t* search_tree( const std::string& node_name, const std::string& parm_name, const std::string& parm_value );
  xml_node_t* split_path ( std::string& key, const std::string& path );

  void print( FILE* f = stdout, int spacing = 0 );
  void print_xml( FILE* f = stdout, int spacing = 0 );
  static std::shared_ptr<xml_node_t> get( sim_t* sim, const std::string& url, cache::behavior_e b,
                          const std::string& confirmation = std::string() );
  static std::shared_ptr<xml_node_t> create( sim_t* sim, const std::string& input );

  xml_node_t* add_child( const std::string& name );
  template <typename T>
  void add_parm( const std::string& name, const T& value )
  {
    std::ostringstream s;
    s << value;
    parameters.push_back( xml_parm_t( name, s.str() ) );
  }
};

// XML Writer ================================================================

class xml_writer_t
{
private:
  io::cfile file;
  enum state
  {
    NONE, TAG, TEXT
  };
  std::stack<std::string> current_tags;
  std::string tabulation;
  state current_state;
  std::string indentation;

  struct replacement
  {
    const char* from;
    const char* to;
  };

public:
  xml_writer_t( const std::string & filename );

  bool ready() const;

  int printf( const char *format, ... ) const PRINTF_ATTRIBUTE( 2, 3 );
  void init_document( const std::string & stylesheet_file );
  void begin_tag( const std::string & tag );
  void end_tag( const std::string & tag );
  void print_attribute( const std::string & name, const std::string & value );
  void print_attribute_unescaped( const std::string & name, const std::string & value );
  void print_tag( const std::string & name, const std::string & inner_value );
  void print_text( const std::string & input );
  static std::string sanitize( std::string v );
};
