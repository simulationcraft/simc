// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_JS_HPP
#define SC_JS_HPP

#include "util/rapidjson/document.h"
#include "util/rapidjson/stringbuffer.h"
#include "util/rapidjson/prettywriter.h"
#include <utility>
#include <string>
#include <vector>

namespace js
{

struct sc_js_t
{
  rapidjson::Document js_;

  sc_js_t();
  sc_js_t( const sc_js_t& );

  virtual std::string to_json() const;

  sc_js_t& set( rapidjson::Value& obj, const std::string& name_, const rapidjson::Value& value_ );

  // Direct data access to a rapidjson value.
  rapidjson::Value& value( const std::string& path );

  // Set the value of JSON object indicated by path to value_
  template <typename T>
  sc_js_t& set( const std::string& path, const T& value_ );
  // Set the value of JSON object indicated by path to an array of values_
  template <typename T>
  sc_js_t& set( const std::string& path, const std::vector<T>& values_ );
  // Set the JSON object property name_ to value_
  template<typename T>
  sc_js_t& set( rapidjson::Value& obj, const std::string& name_, const T& value_ );
  // Set the JSON object property name_ to a JSON array value_
  template<typename T>
  sc_js_t& set( rapidjson::Value& obj, const std::string& name_, const std::vector<T>& value_ );

  sc_js_t& set( const std::string& path, const sc_js_t& value_ );
  sc_js_t& set( const std::string& path, const size_t& value_ );

  // Specializations for const char* (we need copies), and std::string
  sc_js_t& set( const std::string& path, const char* value );
  sc_js_t& set( const std::string& path, const std::string& value );
  sc_js_t& set( rapidjson::Value& obj, const std::string& name, const char* value_ );
  sc_js_t& set( rapidjson::Value& obj, const std::string& name, const std::string& value_ );

  // Add elements to a JSON array indicated by path by appending value_
  template <typename T>
  sc_js_t& add( const std::string& path, const T& value_ );
  // Add elements to a JSON array indicated by path by appending data
  template <typename T>
  sc_js_t& add( const std::string& path, const std::vector<T>& data );

  // Specializations for adding elements to a JSON array for various types
  sc_js_t& add( const std::string& path, const rapidjson::Value& obj );
  sc_js_t& add( const std::string& path, const sc_js_t& obj );
  sc_js_t& add( const std::string& path, const std::string& value_ );
  sc_js_t& add( const std::string& path, const char* value_ );
  sc_js_t& add( const std::string& path, double x, double low, double high );
  sc_js_t& add( const std::string& path, double x, double y );

protected:
  // Find the Value object given by a path, and construct any missing objects along the way
  rapidjson::Value* path_value( const std::string& path );

  // Note, value_ will transfer ownership to this object after the call.
  rapidjson::Value& do_set( rapidjson::Value& obj, const char* name_, rapidjson::Value& value_ )
  {
    assert( obj.GetType() == rapidjson::kObjectType );

    rapidjson::Value name_value( name_, js_.GetAllocator() );

    return obj.AddMember( name_value, value_, js_.GetAllocator() );
  }

  template <typename T>
  rapidjson::Value& do_insert( rapidjson::Value& obj, const std::vector<T>& values )
  {
    assert( obj.GetType() == rapidjson::kArrayType );

    for ( size_t i = 0, end = values.size(); i < end; i++ )
      obj.PushBack( values[ i ], js_.GetAllocator() );

    return obj;
  }
};

template <typename T>
sc_js_t& sc_js_t::set( const std::string& path, const T& value_ )
{
  if ( rapidjson::Value* obj = path_value( path ) )
    *obj = value_;
  return *this;
}

template<typename T>
sc_js_t& sc_js_t::set( const std::string& path, const std::vector<T>& values )
{
  if ( rapidjson::Value* obj = path_value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    obj -> Clear();

    do_insert( *obj, values );
  }
  return *this;
}

template <typename T>
sc_js_t& sc_js_t::add( const std::string& path, const T& value_ )
{
  if ( rapidjson::Value* obj = path_value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    obj -> PushBack( value_, js_.GetAllocator() );
  }

  return *this;
}

template<typename T>
sc_js_t& sc_js_t::add( const std::string& path, const std::vector<T>& values )
{
  if ( rapidjson::Value* obj = path_value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    do_insert( *obj, values );
  }
  return *this;
}

template<typename T>
sc_js_t& sc_js_t::set( rapidjson::Value& obj, const std::string& name_, const T& value_ )
{
  assert( obj.GetType() == rapidjson::kObjectType );

  rapidjson::Value value_obj( value_ );

  do_set( obj, name_.c_str(), value_obj );
  return *this;
}

template<typename T>
sc_js_t& sc_js_t::set( rapidjson::Value& obj, const std::string& name_, const std::vector<T>& value_ )
{
  assert( obj.GetType() == rapidjson::kObjectType );

  rapidjson::Value value_obj( rapidjson::kArrayType );

  do_set( obj, name_.c_str(), do_insert( value_obj, value_ ) );
  return *this;
}

inline sc_js_t& sc_js_t::set( rapidjson::Value& obj, const std::string& name_, const rapidjson::Value& value_ )
{
  assert( obj.GetType() == rapidjson::kObjectType );

  rapidjson::Value new_value( value_, js_.GetAllocator() );

  do_set( obj, name_.c_str(), new_value );
  return *this;
}

} /* namespace js */

#endif /* SC_JS_HPP */
