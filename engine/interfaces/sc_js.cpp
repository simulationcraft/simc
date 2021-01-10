// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_js.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "sim/sc_cooldown.hpp"
#include "util/util.hpp"
#include "util/timespan.hpp"
#include "util/sample_data.hpp"
#include "util/rng.hpp"
#include "util/timeline.hpp"
#include <cassert>

namespace js {

  sc_js_t::sc_js_t()
  {
    js_.SetObject();
  }

  sc_js_t::sc_js_t(const sc_js_t& o)
  {
    js_.CopyFrom(o.js_, js_.GetAllocator());
    fmt::print("\n\nSC_JS_T:: COPY CONSTRUCTOR USED!!!\n");
  }

  rapidjson::Value* sc_js_t::path_value(util::string_view path_str)
  {
    auto path = util::string_split(path_str, ".");
    rapidjson::Value* v = nullptr;
    if (path.empty())
      return v;

    assert(!util::is_number(path[0]));

    if (!js_.HasMember(path[0]))
    {
      rapidjson::Value nn(path[0], js_.GetAllocator());
      rapidjson::Value nv;
      js_.AddMember(nn, nv, js_.GetAllocator());
    }

    v = &(js_[path[0]]);

    for (size_t i = 1, end = path.size(); i < end; i++)
    {
      // Number is array indexing [0..size-1]
      if (util::is_number(path[i]))
      {
        if (v->GetType() != rapidjson::kArrayType)
          v->SetArray();

        int parsed_idx = util::to_int(path[i]);
        assert(parsed_idx >= 0);
        unsigned idx = parsed_idx;
        unsigned missing = 0;
        if (v->Size() <= idx)
          missing = (idx - v->Size()) + 1;

        // Pad with null objects, until we have enough
        for (unsigned midx = 0; midx < missing; midx++)
          v->PushBack(rapidjson::kNullType, js_.GetAllocator());

        v = &((*v)[rapidjson::SizeType(idx)]);
      }
      // Object traversal
      else
      {
        if (v->GetType() != rapidjson::kObjectType)
          v->SetObject();

        if (!v->HasMember(path[i]))
        {
          rapidjson::Value nn(path[i], js_.GetAllocator());
          rapidjson::Value nv;
          v->AddMember(nn, nv, js_.GetAllocator());
        }

        v = &((*v)[path[i]]);
      }
    }

    return v;
  }

  rapidjson::Value& sc_js_t::value(util::string_view path)
  {
    return *path_value(path);
  }

  sc_js_t& sc_js_t::set(util::string_view path, util::string_view value_)
  {
    if (rapidjson::Value* obj = path_value(path))
      *obj = rapidjson::Value(value_.data(), as<rapidjson::SizeType>( value_.size() ), js_.GetAllocator());
    return *this;
  }

  sc_js_t& sc_js_t::set(util::string_view path, const sc_js_t& value_)
  {
    if (rapidjson::Value* obj = path_value(path))
    {
      obj->CopyFrom(value_.js_, js_.GetAllocator());
    }
    return *this;
  }

  sc_js_t& sc_js_t::set(util::string_view path, size_t value_)
  {
    if (rapidjson::Value* obj = path_value(path))
    {
      rapidjson::Value v(static_cast<uint64_t>(value_));
      *obj = v;
    }
    return *this;
  }

  sc_js_t& sc_js_t::add(util::string_view path, util::string_view value_)
  {
    if (rapidjson::Value* obj = path_value(path))
    {
      if (obj->GetType() != rapidjson::kArrayType)
        obj->SetArray();

      rapidjson::Value v(value_.data(), as<rapidjson::SizeType>( value_.size() ), js_.GetAllocator());
      obj->PushBack(v, js_.GetAllocator());
    }
    return *this;
  }

  sc_js_t& sc_js_t::add(util::string_view path, const rapidjson::Value& value_)
  {
    if (rapidjson::Value* obj = path_value(path))
    {
      if (obj->GetType() != rapidjson::kArrayType)
        obj->SetArray();

      rapidjson::Value new_value(value_, js_.GetAllocator());

      obj->PushBack(new_value, js_.GetAllocator());
    }
    return *this;
  }

  sc_js_t& sc_js_t::add(util::string_view path, const sc_js_t& value_)
  {
    if (rapidjson::Value* obj = path_value(path))
    {
      if (obj->GetType() != rapidjson::kArrayType)
        obj->SetArray();

      rapidjson::Value new_value(value_.js_, js_.GetAllocator());

      obj->PushBack(new_value, js_.GetAllocator());
    }
    return *this;
  }

  sc_js_t& sc_js_t::add(util::string_view path, double x, double low, double high)
  {
    if (rapidjson::Value* obj = path_value(path))
    {
      if (obj->GetType() != rapidjson::kArrayType)
        obj->SetArray();

      rapidjson::Value v(rapidjson::kArrayType);
      v.PushBack(x, js_.GetAllocator()).PushBack(low, js_.GetAllocator()).PushBack(high, js_.GetAllocator());

      obj->PushBack(v, js_.GetAllocator());
    }
    return *this;
  }

  sc_js_t& sc_js_t::add(util::string_view path, double x, double y)
  {
    if (rapidjson::Value* obj = path_value(path))
    {
      if (obj->GetType() != rapidjson::kArrayType)
        obj->SetArray();

      rapidjson::Value v(rapidjson::kArrayType);
      v.PushBack(x, js_.GetAllocator()).PushBack(y, js_.GetAllocator());

      obj->PushBack(v, js_.GetAllocator());
    }
    return *this;
  }

  sc_js_t& sc_js_t::set(rapidjson::Value& obj, util::string_view name_, util::string_view value_)
  {
    assert(obj.GetType() == rapidjson::kObjectType);

    rapidjson::Value value_obj(value_.data(), as<rapidjson::SizeType>( value_.size() ), js_.GetAllocator());

    do_set(obj, name_, value_obj);
    return *this;
  }

  std::string sc_js_t::to_json() const
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer(b);

    js_.Accept(writer);

    return b.GetString();
  }

  JsonOutput& js::JsonOutput::operator=(const cooldown_t& v)
  {
    assert(v_.IsObject());

    v_.AddMember(rapidjson::StringRef("name"), rapidjson::StringRef(v.name()), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("duration"), v.duration.total_seconds(), d_.GetAllocator());
    if (v.charges > 1)
    {
      v_.AddMember(rapidjson::StringRef("charges"), v.charges, d_.GetAllocator());
    }
    return *this;
  }

  JsonOutput& JsonOutput::operator=(const rng::rng_t& v)
  {
    assert(v_.IsObject());
    v_.AddMember(rapidjson::StringRef("name"), rapidjson::StringRef(v.name()), d_.GetAllocator());

    return *this;
  }

  JsonOutput& JsonOutput::operator=(timespan_t v)
  {
    v_ = v.total_seconds(); return *this;
  }

  JsonOutput& JsonOutput::operator=(const extended_sample_data_t& v)
  {
    assert(v_.IsObject());

    v_.AddMember(rapidjson::StringRef("sum"), v.sum(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("count"), as<unsigned>(v.count()), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("mean"), v.mean(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("min"), v.min(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("max"), v.max(), d_.GetAllocator());
    if (!v.simple)
    {
      v_.AddMember(rapidjson::StringRef("median"), v.percentile(0.5), d_.GetAllocator());
      v_.AddMember(rapidjson::StringRef("variance"), v.variance, d_.GetAllocator());
      v_.AddMember(rapidjson::StringRef("std_dev"), v.std_dev, d_.GetAllocator());
      v_.AddMember(rapidjson::StringRef("mean_variance"), v.mean_variance, d_.GetAllocator());
      v_.AddMember(rapidjson::StringRef("mean_std_dev"), v.mean_std_dev, d_.GetAllocator());
    }

    return *this;
  }

  JsonOutput& JsonOutput::operator=(const simple_sample_data_t& v)
  {
    assert(v_.IsObject());

    v_.AddMember(rapidjson::StringRef("sum"), v.sum(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("count"), as<unsigned>(v.count()), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("mean"), v.mean(), d_.GetAllocator());

    return *this;
  }

  JsonOutput& JsonOutput::operator=(const simple_sample_data_with_min_max_t& v)
  {
    assert(v_.IsObject());

    v_.AddMember(rapidjson::StringRef("sum"), v.sum(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("count"), as<unsigned>(v.count()), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("mean"), v.mean(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("min"), v.min(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("max"), v.max(), d_.GetAllocator());

    return *this;
  }

  JsonOutput& JsonOutput::operator=( const std::vector<std::string>& v )
  {
    v_.SetArray();

    range::for_each( v, [ this ]( const std::string& value ) {
      v_.PushBack( rapidjson::StringRef( value.c_str() ), d_.GetAllocator() );
    } );

    return *this;
  }
  
  JsonOutput& JsonOutput::operator=(const sc_timeline_t& v)
  {
    assert(v_.IsObject());

    v_.AddMember(rapidjson::StringRef("mean"), v.mean(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("mean_std_dev"), v.mean_stddev(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("min"), v.min(), d_.GetAllocator());
    v_.AddMember(rapidjson::StringRef("max"), v.max(), d_.GetAllocator());

    rapidjson::Value data_arr(rapidjson::kArrayType);
    range::for_each(v.data(), [&data_arr, this](double dp) {
      data_arr.PushBack(dp, d_.GetAllocator());
      });

    v_.AddMember(rapidjson::StringRef("data"), data_arr, d_.GetAllocator());
    return *this;
  }
} // js