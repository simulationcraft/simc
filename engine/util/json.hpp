// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef JSON_HPP
#define JSON_HPP

enum json_e { 
  JSON_UNKNOWN=0,
  JSON_NUM,
  JSON_STR,
  JSON_BOOL,
  JSON_ARRAY,
  JSON_OBJ
};

struct json_num_t;
struct json_str_t;
struct json_bool_t;
struct json_array_t;
struct json_obj_t;

struct json_node_t 
{
  json_e type;
  json_node_t( json_e t=JSON_UNKNOWN ) : type(t) {}
  virtual ~json_node_t() {}
  virtual void print( FILE* f=0, int spacing=0 ) = 0;
  virtual void format( std::string& buffer, int spacing=0 ) = 0;
};

struct json_num_t : public json_node_t
{
  double num;
  int precision;
  json_num_t( double n, int p=-1 ) : json_node_t(JSON_NUM), num(n), precision(p) {}
  void print( FILE* f=0, int spacing=0 );
  void format( std::string& buffer, int spacing=0 );
};

struct json_str_t : public json_node_t
{
  std::string str;
  json_str_t( const std::string& s ) : json_node_t(JSON_STR), str(s) {}
  void print( FILE* f=0, int spacing=0 );
  void format( std::string& buffer, int spacing=0 );
};

struct json_bool_t : public json_node_t
{
  bool value;
  json_bool_t( bool v ) : json_node_t(JSON_BOOL), value(v) {}
  void print( FILE* f=0, int spacing=0 );
  void format( std::string& buffer, int spacing=0 );
};

struct json_array_t : public json_node_t
{
  std::vector<json_node_t*> nodes;
  json_array_t() : json_node_t(JSON_ARRAY) {}
  void print( FILE* f=0, int spacing=0 );
  void format( std::string& buffer, int spacing=0 );
  json_num_t*   add_num  ( double num, int precision=-1 );
  json_str_t*   add_str  ( const std::string& str );
  json_bool_t*  add_bool ( bool value );
  json_array_t* add_array();
  json_obj_t*   add_obj  ();
};

struct json_obj_t : public json_node_t
{
  std::map<std::string,json_node_t*> nodes;
  json_obj_t() : json_node_t(JSON_OBJ) {}
  void print( FILE* f=0, int spacing=0 );
  void format( std::string& buffer, int spacing=0 );
  json_num_t*   add_num  ( const std::string& name, double num, int precision=-1 );
  json_str_t*   add_str  ( const std::string& name, const std::string& str );
  json_bool_t*  add_bool ( const std::string& name, bool value );
  json_array_t* add_array( const std::string& name );
  json_obj_t*   add_obj  ( const std::string& name );
};

#endif
