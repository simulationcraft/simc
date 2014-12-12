// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "str.hpp"
#include "json.hpp"

// ==========================================================================
// JSON_NUM
// ==========================================================================

void json_num_t::print( FILE* f, int spacing )
{
  if( ! f ) f = stdout;
  if( precision >= 0 )
    fprintf( f, "%.*f", precision, num );
  else if( num == floor( num ) )
    fprintf( f, "%.0f", num );
  else
    fprintf( f, "%f", num );
}

void json_num_t::format( std::string& buffer, int spacing )
{
  if( precision >= 0 )
    str::format( buffer, "%.*f", precision, num );
  else if( num == floor( num ) )
    str::format( buffer, "%.0f", num );
  else
    str::format( buffer, "%f", num );
}

// ==========================================================================
// JSON_STR
// ==========================================================================

void json_str_t::print( FILE* f, int spacing )
{
  if( ! f ) f = stdout;
  fprintf( f, "\"%s\"", str.c_str() );
}

void json_str_t::format( std::string& buffer, int spacing )
{
  buffer += "\"";
  buffer += str;
  buffer += "\"";
}

// ==========================================================================
// JSON_BOOL
// ==========================================================================

void json_bool_t::print( FILE* f, int spacing )
{
  if( ! f ) f = stdout;
  fprintf( f, "%s", value ? "true" : "false" );
}

void json_bool_t::format( std::string& buffer, int spacing )
{
  if( value ) 
    buffer += "true"; 
  else 
    buffer += "false";
}

// ==========================================================================
// JSON_ARRAY
// ==========================================================================

void json_array_t::print( FILE* f, int spacing )
{
  if( ! f ) f = stdout;
  if( nodes.empty() )
  {
    fprintf( f, "[]" );
    return;
  }
  fprintf( f, "[\n" );
  spacing += 2;
  for( int i=0; i < nodes.size(); i++ )
  {
    if( i ) fprintf( f, ",\n" );
    fprintf( f, "%*s", spacing, "" );
    nodes[ i ] -> print( f, spacing );
  }
  spacing -= 2;
  fprintf( f, "\n%*s]", spacing, "" );
}

void json_array_t::format( std::string& buffer, int spacing )
{
  if( nodes.empty() )
  {
    buffer += "[]";
    return;
  }
  buffer += "[\n";
  spacing += 2;
  for( int i=0; i < nodes.size(); i++ )
  {
    if( i ) buffer += ",\n";
    str::format( buffer, "%*s", spacing, "" );
    nodes[ i ] -> format( buffer, spacing );
  }
  spacing -= 2;
  str::format( buffer, "\n%*s]", spacing, "" );
}

json_num_t* json_array_t::add_num( double num, int precision )
{
  json_num_t* n = new json_num_t( num, precision );
  nodes.push_back( n );
  return n;
}

json_str_t* json_array_t::add_str( const std::string& str )
{
  json_str_t* n = new json_str_t( str );
  nodes.push_back( n );
  return n;
}

json_bool_t* json_array_t::add_bool( bool value )
{
  json_bool_t* n = new json_bool_t( value );
  nodes.push_back( n );
  return n;
}

json_array_t* json_array_t::add_array()
{
  json_array_t* n = new json_array_t();
  nodes.push_back( n );
  return n;
}

json_obj_t* json_array_t::add_obj()
{
  json_obj_t* n = new json_obj_t();
  nodes.push_back( n );
  return n;
}

// ==========================================================================
// JSON_OBJ
// ==========================================================================

void json_obj_t::print( FILE* f, int spacing )
{
  if( ! f ) f = stdout;
  if( nodes.empty() )
  {
    fprintf( f, "null" );
    return;
  }
  fprintf( f, "{\n" );
  spacing += 2;
  bool first=true;
  for( std::map<std::string,json_node_t*>::iterator iter=nodes.begin(), end=nodes.end(); iter!=end; ++iter )
  {
    if( ! first ) fprintf( f, ",\n" );
    first = false;
    fprintf( f, "%*s\"%s\" : ", spacing, "", iter -> first.c_str() );
    iter -> second -> print( f, spacing );
  }
  spacing -= 2;
  fprintf( f, "\n%*s}", spacing, "" );
}

void json_obj_t::format( std::string& buffer, int spacing )
{
  if( nodes.empty() )
  {
    buffer += "null";
    return;
  }
  buffer += "{\n";
  spacing += 2;
  bool first=true;
  for( std::map<std::string,json_node_t*>::iterator iter=nodes.begin(), end=nodes.end(); iter!=end; ++iter )
  {
    if( ! first ) buffer += ",\n";
    first = false;
    str::format( buffer, "%*s\"%s\" : ", spacing, "", iter -> first.c_str() );
    iter -> second -> format( buffer, spacing );
  }
  spacing -= 2;
  str::format( buffer, "\n%*s}", spacing, "" );
}

json_num_t* json_obj_t::add_num( const std::string& name, double num, int precision )
{
  json_num_t* n = new json_num_t( num, precision );
  nodes[ name ] = n;
  return n;
}

json_str_t* json_obj_t::add_str( const std::string& name, const std::string& str )
{
  json_str_t* n = new json_str_t( str );
  nodes[ name ] = n;
  return n;
}

json_bool_t* json_obj_t::add_bool( const std::string& name, bool value )
{
  json_bool_t* n = new json_bool_t( value );
  nodes[ name ] = n;
  return n;
}

json_array_t* json_obj_t::add_array( const std::string& name )
{
  json_array_t* n = new json_array_t();
  nodes[ name ] = n;
  return n;
}

json_obj_t* json_obj_t::add_obj( const std::string& name )
{
  json_obj_t* n = new json_obj_t();
  nodes[ name ] = n;
  return n;
}

#ifdef UNIT_TEST
int main( int argc, char** argv )
{
  json_obj_t* obj = new json_obj_t();
  obj -> add_num( "a", 2.3, 2 );
  obj -> add_num( "b", 2.0 );
  json_array_t* array = obj -> add_array( "c" );
  array -> add_str( "one" );
  array -> add_str( "two" );
  obj -> add_bool( "d", true );
  obj -> print();
  printf( "\n" );
  std::string buffer;
  obj -> format( buffer );
  printf( "%s\n", buffer.c_str() );
}
#endif
