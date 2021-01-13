// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_JS_HPP
#define SC_JS_HPP


#include "config.hpp"
#include "rapidjson/document.h"
#include "util/timespan.hpp"
#include "util/string_view.hpp"
#include "util/generic.hpp"

#include <utility>
#include <string>
#include <vector>
#include <algorithm>


struct cooldown_t;
class extended_sample_data_t;
class simple_sample_data_t;
class simple_sample_data_with_min_max_t;
struct sc_timeline_t;
namespace rng {
  struct rng_t;
}

namespace js
{

struct sc_js_t
{
  rapidjson::Document js_;

  sc_js_t();
  sc_js_t( const sc_js_t& );
  virtual ~sc_js_t() {}

  virtual std::string to_json() const;

  sc_js_t& set( rapidjson::Value& obj, util::string_view name_, const rapidjson::Value& value_ );

  // Direct data access to a rapidjson value.
  rapidjson::Value& value( util::string_view path );

  // Set the value of JSON object indicated by path to value_
  template <typename T, typename = std::enable_if_t<!std::is_convertible<T, util::string_view>::value>>
  sc_js_t& set( util::string_view path, const T& value_ );
  // Set the value of JSON object indicated by path to an array of values_
  template <typename T>
  sc_js_t& set( util::string_view path, const std::vector<T>& values_ );

  sc_js_t& set( util::string_view path, const sc_js_t& value_ );
  sc_js_t& set( util::string_view path, size_t value_ );
  sc_js_t& set( util::string_view path, util::string_view value );

  // Set the JSON object property name_ to value_
  template <typename T, typename = std::enable_if_t<!std::is_convertible<T, util::string_view>::value>>
  sc_js_t& set( rapidjson::Value& obj, util::string_view name_, const T& value_ );
  // Set the JSON object property name_ to a JSON array value_
  template <typename T>
  sc_js_t& set( rapidjson::Value& obj, util::string_view name_, const std::vector<T>& value_ );

  sc_js_t& set( rapidjson::Value& obj, util::string_view name, util::string_view value );

  // Add elements to a JSON array indicated by path by appending value_
  template <typename T, typename = std::enable_if_t<!std::is_convertible<T, util::string_view>::value>>
  sc_js_t& add( util::string_view path, const T& value_ );
  // Add elements to a JSON array indicated by path by appending data
  template <typename T>
  sc_js_t& add( util::string_view path, const std::vector<T>& data );

  // Specializations for adding elements to a JSON array for various types
  sc_js_t& add( util::string_view path, const rapidjson::Value& value_ );
  sc_js_t& add( util::string_view path, const sc_js_t& value_ );
  sc_js_t& add( util::string_view path, util::string_view value_ );
  sc_js_t& add( util::string_view path, double x, double low, double high );
  sc_js_t& add( util::string_view path, double x, double y );

protected:
  // Find the Value object given by a path, and construct any missing objects along the way
  rapidjson::Value* path_value( util::string_view path );

  // Note, value_ will transfer ownership to this object after the call.
  rapidjson::Value& do_set( rapidjson::Value& obj, util::string_view name_, rapidjson::Value& value_ )
  {
    assert( obj.GetType() == rapidjson::kObjectType );

    rapidjson::Value name_value( name_.data(), as<rapidjson::SizeType>( name_.size() ), js_.GetAllocator() );

    return obj.AddMember( name_value, value_, js_.GetAllocator() );
  }

  template <typename T>
  rapidjson::Value& do_insert( rapidjson::Value& obj, const std::vector<T>& values )
  {
    assert( obj.GetType() == rapidjson::kArrayType );

    for ( const T& value : values )
      obj.PushBack( value, js_.GetAllocator() );

    return obj;
  }
};

template <typename T, typename /* enable_if junk */>
sc_js_t& sc_js_t::set( util::string_view path, const T& value_ )
{
  if ( rapidjson::Value* obj = path_value( path ) )
    *obj = value_;
  return *this;
}

template<typename T>
sc_js_t& sc_js_t::set( util::string_view path, const std::vector<T>& values )
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

template <typename T, typename /* enable_if junk */>
sc_js_t& sc_js_t::add( util::string_view path, const T& value_ )
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
sc_js_t& sc_js_t::add( util::string_view path, const std::vector<T>& values )
{
  if ( rapidjson::Value* obj = path_value( path ) )
  {
    if ( obj -> GetType() != rapidjson::kArrayType )
      obj -> SetArray();

    do_insert( *obj, values );
  }
  return *this;
}

template <typename T, typename /* enable_if junk */>
sc_js_t& sc_js_t::set( rapidjson::Value& obj, util::string_view name_, const T& value_ )
{
  assert( obj.GetType() == rapidjson::kObjectType );

  rapidjson::Value value_obj( value_ );

  do_set( obj, name_, value_obj );
  return *this;
}

template<typename T>
sc_js_t& sc_js_t::set( rapidjson::Value& obj, util::string_view name_, const std::vector<T>& value_ )
{
  assert( obj.GetType() == rapidjson::kObjectType );

  rapidjson::Value value_obj( rapidjson::kArrayType );

  do_set( obj, name_, do_insert( value_obj, value_ ) );
  return *this;
}

inline sc_js_t& sc_js_t::set( rapidjson::Value& obj, util::string_view name_, const rapidjson::Value& value_ )
{
  assert( obj.GetType() == rapidjson::kObjectType );

  rapidjson::Value new_value( value_, js_.GetAllocator() );

  do_set( obj, name_, new_value );
  return *this;
}

struct JsonOutput
{
private:
  rapidjson::Document& d_;
  rapidjson::Value& v_;

public:
  JsonOutput( rapidjson::Document& d, rapidjson::Value& v ) : d_( d ), v_( v )
  { }

  // Create a new object member (n) to the current object (v_), and return a reference to it
  JsonOutput operator[]( util::string_view n )
  {
    rapidjson::Value name( n.data(), as<rapidjson::SizeType>( n.size() ) );
    auto member = v_.FindMember( name );
    if ( member == v_.MemberEnd() )
    {
      rapidjson::Value key( n.data(), as<rapidjson::SizeType>( n.size() ), d_.GetAllocator() );
      rapidjson::Value obj( rapidjson::kObjectType );
      v_.AddMember( key, obj, d_.GetAllocator() );
      member = std::prev( v_.MemberEnd() );
      assert( member -> value == v_[ name ] );
    }
    return JsonOutput( d_, member -> value );
  }

  rapidjson::Value& val()
  { return v_; }

  rapidjson::Document& doc()
  { return d_; }

  // Makes v_ an array (instead of an Object which it is by default)
  JsonOutput& make_array()
  { v_.SetArray(); return *this; }

  // Assign any primitive (supported value by RapidJSON) to the current value (v_)
  template <typename T, typename = std::enable_if_t<!std::is_convertible<T, util::string_view>::value>>
  JsonOutput& operator=( const T& v )
  { v_ = v; return *this; }

  // Assign a string to the current value (v_), and have RapidJSON do the copy.
  JsonOutput& operator=( util::string_view v )
  { v_.SetString( v.data(), as<rapidjson::SizeType>( v.size() ), d_.GetAllocator() ); return *this; }

  // Assign an external RapidJSON Value to the current value (v_)
  JsonOutput operator=( rapidjson::Value& v )
  { v_ = v; return *this; }

  // Assign a simc-internal timespan object to the current value (v_). Timespan object is converted
  // to a double.
  JsonOutput& operator=(timespan_t v);

  // Assign a simc-internal sample data container to the current value (v_). The container is
  // converted to an object with fixed fields.
  JsonOutput& operator=(const extended_sample_data_t& v);

  // Assign a simc-internal sample data container to the current value (v_). The container is
  // converted to an object with fixed fields.
  JsonOutput& operator=(const simple_sample_data_t& v);

  // Assign a simc-internal sample data container to the current value (v_). The container is
  // converted to an object with fixed fields.
  JsonOutput& operator=(const simple_sample_data_with_min_max_t& v);

  JsonOutput& operator=( const std::vector<std::string>& v );

  template <typename T>
  JsonOutput& operator=( const std::vector<T>& v )
  {
    v_.SetArray();

    std::for_each( v.begin(), v.end(), [ this ]( const T& value ) {
      v_.PushBack( value, d_.GetAllocator() );
    } );

    return *this;
  }

  // Assign a simc-internal timeline data container to the current value (v_). The container is
  // converted to an object with fixed fields.
  JsonOutput& operator=(const sc_timeline_t& v);

  JsonOutput& operator=( const cooldown_t& v );

  JsonOutput& operator=(const rng::rng_t& v);

  template <typename T>
  JsonOutput& add( T v )
  { assert( v_.IsArray() ); v_.PushBack( v, d_.GetAllocator() ); return *this; }

  JsonOutput add( rapidjson::Value& v )
  { assert( v_.IsArray() ); v_.PushBack( v, d_.GetAllocator() ); return JsonOutput( d_, v_[ v_.Size() - 1 ] ); }

  JsonOutput add()
  { auto v = rapidjson::Value( rapidjson::kObjectType ); return add( v ); }
};

} /* namespace js */

#endif /* SC_JS_HPP */
