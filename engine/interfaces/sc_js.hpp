// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_JS_HPP
#define SC_JS_HPP

#ifndef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1
#endif

#include "util/rapidjson/document.h"
#include "util/rapidjson/stringbuffer.h"
#include "util/rapidjson/prettywriter.h"
#include <utility>
#include <string>
#include <vector>

#include "sc_timespan.hpp"
#include "util/sample_data.hpp"
#include "util/rng.hpp"
#include "util/timeline.hpp"

struct cooldown_t;

namespace js
{

struct sc_js_t
{
  rapidjson::Document js_;

  sc_js_t();
  sc_js_t( const sc_js_t& );
  virtual ~sc_js_t() {}

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

struct JsonOutput
{
private:
  rapidjson::Document& d_;
  rapidjson::Value& v_;

public:
  JsonOutput( rapidjson::Document& d, rapidjson::Value& v ) : d_( d ), v_( v )
  { }

  // Create a new object member (n) to the current object (v_), and return a reference to it
  JsonOutput operator[]( const char* n )
  {
    if ( ! v_.HasMember( n ) )
    {
      auto obj = rapidjson::Value();
      obj.SetObject();

      rapidjson::Value key { n, d_.GetAllocator() };

      v_.AddMember( key, obj, d_.GetAllocator() );
    }

    return JsonOutput( d_, v_[ n ] );
  }

  JsonOutput operator[]( const std::string& n )
  { return operator[]( n.c_str() ); }

  rapidjson::Value& val()
  { return v_; }

  rapidjson::Document& doc()
  { return d_; }

  // Makes v_ an array (instead of an Object which it is by default)
  JsonOutput& make_array()
  { v_.SetArray(); return *this; }

  // Assign any primitive (supported value by RapidJSON) to the current value (v_)
  template <typename T>
  JsonOutput& operator=( T v )
  { v_ = v; return *this; }

  // Assign a string to the current value (v_), and have RapidJSON do the copy.
  JsonOutput& operator=( const char* v )
  { v_.SetString( v, d_.GetAllocator() ); return *this; }

  JsonOutput operator=( const std::string& v )
  { v_.SetString( v.c_str(), d_.GetAllocator() ); return *this; }

  // Assign an external RapidJSON Value to the current value (v_)
  JsonOutput operator=( rapidjson::Value& v )
  { v_ = v; return *this; }

  // Assign a simc-internal timespan object to the current value (v_). Timespan object is converted
  // to a double.
  JsonOutput& operator=( const timespan_t& v )
  { v_ = v.total_seconds(); return *this; }

  // Assign a simc-internal sample data container to the current value (v_). The container is
  // converted to an object with fixed fields.
  JsonOutput& operator=( const extended_sample_data_t& v )
  {
    assert( v_.IsObject() );

    v_.AddMember( rapidjson::StringRef( "sum" ), v.sum(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "count" ), as<unsigned>( v.count() ), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "mean" ), v.mean(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "min" ), v.min(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "max" ), v.max(), d_.GetAllocator() );
    if ( ! v.simple )
    {
      v_.AddMember( rapidjson::StringRef( "median" ), v.percentile( 0.5 ), d_.GetAllocator() );
      v_.AddMember( rapidjson::StringRef( "variance" ), v.variance, d_.GetAllocator() );
      v_.AddMember( rapidjson::StringRef( "std_dev" ), v.std_dev, d_.GetAllocator() );
      v_.AddMember( rapidjson::StringRef( "mean_variance" ), v.mean_variance, d_.GetAllocator() );
      v_.AddMember( rapidjson::StringRef( "mean_std_dev" ), v.mean_std_dev, d_.GetAllocator() );
    }

    return *this;
  }

  // Assign a simc-internal sample data container to the current value (v_). The container is
  // converted to an object with fixed fields.
  JsonOutput& operator=( const simple_sample_data_t& v )
  {
    assert( v_.IsObject() );

    v_.AddMember( rapidjson::StringRef( "sum" ), v.sum(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "count" ), as<unsigned>( v.count() ), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "mean" ), v.mean(), d_.GetAllocator() );

    return *this;
  }

  // Assign a simc-internal sample data container to the current value (v_). The container is
  // converted to an object with fixed fields.
  JsonOutput& operator=( const simple_sample_data_with_min_max_t& v )
  {
    assert( v_.IsObject() );

    v_.AddMember( rapidjson::StringRef( "sum" ), v.sum(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "count" ), as<unsigned>( v.count() ), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "mean" ), v.mean(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "min" ), v.min(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "max" ), v.max(), d_.GetAllocator() );

    return *this;
  }

  JsonOutput& operator=( const std::vector<std::string>& v )
  {
    v_.SetArray();

    range::for_each( v, [ this ]( const std::string& value ) {
      v_.PushBack( rapidjson::StringRef( value.c_str() ), d_.GetAllocator() );
    } );

    return *this;
  }

  template <typename T>
  JsonOutput& operator=( const std::vector<T>& v )
  {
    v_.SetArray();

    range::for_each( v, [ this ]( const T& value ) {
      v_.PushBack( value, d_.GetAllocator() );
    } );

    return *this;
  }

  // Assign a simc-internal timeline data container to the current value (v_). The container is
  // converted to an object with fixed fields.
  JsonOutput& operator=( const sc_timeline_t& v )
  {
    assert( v_.IsObject() );

    v_.AddMember( rapidjson::StringRef( "mean" ), v.mean(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "mean_std_dev" ), v.mean_stddev(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "min" ), v.min(), d_.GetAllocator() );
    v_.AddMember( rapidjson::StringRef( "max" ), v.max(), d_.GetAllocator() );

    rapidjson::Value data_arr( rapidjson::kArrayType );
    range::for_each( v.data(), [ &data_arr, this ]( double dp ) {
      data_arr.PushBack( dp, d_.GetAllocator() );
    } );

    v_.AddMember( rapidjson::StringRef( "data" ), data_arr, d_.GetAllocator() );
    return *this;
  }

  JsonOutput& operator=( const cooldown_t& v );

  JsonOutput& operator=( const rng::rng_t& v )
  {
    assert( v_.IsObject() );
    v_.AddMember( rapidjson::StringRef( "name" ), rapidjson::StringRef( v.name() ), d_.GetAllocator() );

    return *this;
  }

  template <typename T>
  JsonOutput& add( T v )
  { assert( v_.IsArray() ); v_.PushBack( v, d_.GetAllocator() ); return *this; }

  JsonOutput& add( const char* v )
  { assert( v_.IsArray() ); v_.PushBack( rapidjson::StringRef( v ), d_.GetAllocator() ); return *this; }

  JsonOutput add( rapidjson::Value& v )
  { assert( v_.IsArray() ); v_.PushBack( v, d_.GetAllocator() ); return JsonOutput( d_, v_[ v_.Size() - 1 ] ); }

  JsonOutput add()
  { auto v = rapidjson::Value(); v.SetObject(); return add( v ); }
};

} /* namespace js */

#endif /* SC_JS_HPP */
