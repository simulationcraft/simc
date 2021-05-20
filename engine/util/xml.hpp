// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef XML_HPP
#define XML_HPP

#include "config.hpp"

#include <memory>
#include <string>
#include <vector>
#include <stack>

#include "fmt/format.h"

#include "cache.hpp"
#include "io.hpp"


namespace rapidxml {
  template<class Ch>
  class xml_node;
}

// XML ======================================================================

// RapidXML Wrapper

struct sc_xml_t
{
  std::vector<char> buf;
  std::unique_ptr<rapidxml::xml_node<char>> root_owner;
  rapidxml::xml_node<char>* root;

  sc_xml_t();

  sc_xml_t(rapidxml::xml_node<char>* n);

  sc_xml_t(std::unique_ptr<rapidxml::xml_node<char>> n, std::vector<char> b);

  // We have only one releaseable source of xml_node* in the system, which will
  // be handled by the xml_cache release process. Thus, we only make a copy of
  // the parsed XML
  sc_xml_t(const sc_xml_t& other);

  sc_xml_t& operator=(const sc_xml_t& other);

  bool valid() const
  { return root != nullptr; }

  std::string name() const;

  sc_xml_t get_child( const std::string& name ) const;
  sc_xml_t get_node ( const std::string& path ) const;
  sc_xml_t get_node ( const std::string& path, const std::string& parm_name, const std::string& parm_value ) const;

  std::vector<sc_xml_t>  get_children( const std::string& name = std::string() );
  std::vector<sc_xml_t>  get_nodes   ( const std::string& path );
  std::vector<sc_xml_t>  get_nodes   ( const std::string& path, const std::string& parm_name, const std::string& parm_value );

  bool get_value( std::string& value, const std::string& path = std::string() );
  bool get_value( int&         value, const std::string& path = std::string() );
  bool get_value( double&      value, const std::string& path = std::string() );

  void print( FILE* f = stdout, int spacing = 0 );
  void print_xml( FILE* f = stdout, int spacing = 0 );
  static sc_xml_t get( const std::string& url, cache::behavior_e cache_behavior,
                       const std::string& confirmation = std::string() );
  static sc_xml_t create( const std::string& input, const std::string& cache_key );

  virtual ~sc_xml_t();

private:
  sc_xml_t search_tree( util::string_view node_name ) const;
  sc_xml_t search_tree( const std::string& node_name, const std::string& parm_name, const std::string& parm_value ) const;
  sc_xml_t split_path ( std::string& key, const std::string& path ) const;
};


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
  std::vector<std::unique_ptr<xml_node_t> > children;
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

  std::unique_ptr<xml_node_t> create_node     ( const std::string& input, std::string::size_type& index );
  void         create_children ( const std::string& input, std::string::size_type& index );
  void        create_parameter( const std::string& input, std::string::size_type& index );

  xml_node_t* search_tree( util::string_view node_name );
  xml_node_t* search_tree( const std::string& node_name, const std::string& parm_name, const std::string& parm_value );
  xml_node_t* split_path ( std::string& key, const std::string& path );

  void print( FILE* f = stdout, int spacing = 0 );
  void print_xml( FILE* f = stdout, int spacing = 0 );
  static std::shared_ptr<xml_node_t> get( const std::string& url, cache::behavior_e cache_behavior,
                                          const std::string& confirmation = std::string() );
  static std::unique_ptr<xml_node_t> create( const std::string& input );

  xml_node_t* add_child( const std::string& name );
  template <typename T>
  void add_parm( const std::string& name, const T& value )
  {
    parameters.push_back( xml_parm_t( name, fmt::to_string( value ) ) );
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

  template <typename... Args>
  int printf( util::string_view format, Args&& ... args )
  {
    return fmt::fprintf( file, format, std::forward<Args>(args)... );
  }
  void init_document( const std::string & stylesheet_file );
  void begin_tag( const std::string & tag );
  void end_tag( const std::string & tag );
  void print_attribute( const std::string & name, const std::string & value );
  void print_attribute_unescaped( const std::string & name, const std::string & value );
  void print_tag( const std::string & name, const std::string & inner_value );
  void print_text( const std::string & input );
  static std::string sanitize( std::string v );
};

#endif /* XML_HPP */
