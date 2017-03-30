// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// ==========================================================================
// Custom Time class
// ==========================================================================

#ifndef SC_TIMESPAN_HPP
#define SC_TIMESPAN_HPP

#include "config.hpp"

#include <cmath>
#include <numeric>
#include <limits>
#include <iostream>
#include <type_traits>

#include "util/generic.hpp"

// if timespan_t is in the global namespace, there's a name lookup issue with
// one of the Qt headers. Problem is avoided by defining in a sub-namespace
// and then lifting into the global namespace with a using declaration.
namespace timespan_adl_barrier
{

  /**
   * @brief Class for representing InGame time.
   *
   * World of Warcraft handles InGame time/events in milliseconds. This class
   * emulates that format for exactness, as well as representing C++ type-safety
   * when defining time values, without implicit conversion from or to any
   * integral or floating numbers.
   */
  class timespan_t
  {
  private:
    using time_t = int_least64_t;

    static time_t native_to_milli(time_t t)
    {
      return t;
    }
    static double native_to_second(time_t t)
    {
      return static_cast<double>(t) * (1.0 / 1000);
    }
    static double native_to_minute(time_t t)
    {
      return static_cast<double>(t) * (1.0 / (60 * 1000));
    }

    template<typename Rep>
    static time_t milli_to_native(Rep t)
    {
      return static_cast<time_t>(t);
    }
    template<typename Rep>
    static time_t second_to_native(Rep t)
    {
      return static_cast<time_t>(t * 1000);
    }
    template<typename Rep>
    static time_t minute_to_native(Rep t)
    {
      return static_cast<time_t>(t * (60 * 1000));
    }

    time_t time;

    template<typename Rep>
    explicit timespan_t(Rep t) :
        time(static_cast<time_t>(t))
    {
    }

  public:
    timespan_t() : time( 0 )
    { }

    double total_minutes() const
    {
      return native_to_minute(time);
    }
    double total_seconds() const
    {
      return native_to_second(time);
    }
    time_t total_millis() const
    {
      return native_to_milli(time);
    }

    template<typename Rep>
    static typename std::enable_if<std::is_arithmetic<Rep>::value, timespan_t>::type from_millis(
        Rep millis)
    {
      return timespan_t(milli_to_native(millis));
    }

    template<typename Rep>
    static typename std::enable_if<std::is_arithmetic<Rep>::value, timespan_t>::type from_seconds(
        Rep seconds)
    {
      return timespan_t(second_to_native(seconds));
    }

    template<typename Rep>
    static typename std::enable_if<std::is_arithmetic<Rep>::value, timespan_t>::type from_minutes(
        Rep minutes)
    {
      return timespan_t(minute_to_native(minutes));
    }

    bool operator==(timespan_t right) const
    {
      return time == right.time;
    }
    bool operator!=(timespan_t right) const
    {
      return time != right.time;
    }

    bool operator>(timespan_t right) const
    {
      return time > right.time;
    }
    bool operator>=(timespan_t right) const
    {
      return time >= right.time;
    }
    bool operator<(timespan_t right) const
    {
      return time < right.time;
    }
    bool operator<=(timespan_t right) const
    {
      return time <= right.time;
    }

    timespan_t& operator+=(timespan_t right)
    {
      assert(right < zero() || *this < zero() || *this <= max() - right);
      assert(right > zero() || *this >= min() - right);
      time += right.time;
      return *this;
    }
    timespan_t& operator-=(timespan_t right)
    {
      assert(right < zero() || *this >= right + min());
      assert(right > zero() || *this <= right + max());
      *this = *this - right;
      return *this;
    }
    timespan_t&
    operator %=(timespan_t right)
    {
      time %= right.time;
      return *this;
    }

    template<typename Rep>
    typename std::enable_if<std::is_arithmetic<Rep>::value, timespan_t&>::type operator*=(
        Rep right)
    {
      time = static_cast<time_t>(time * right);
      return *this;
    }

    template<typename Rep>
    typename std::enable_if<std::is_arithmetic<Rep>::value, timespan_t&>::type operator/=(
        Rep right)
    {
      time = static_cast<time_t>(time / right);
      return *this;
    }

    friend timespan_t operator+(timespan_t right)
    {
      return right;
    }
    friend timespan_t operator-(timespan_t right)
    {
      return timespan_t(-right.time);
    }

    friend timespan_t operator+(timespan_t left, timespan_t right)
    {
      assert(right < zero() || left < zero() || left <= max() - right);
      assert(right > zero() || left >= min() - right);
      return timespan_t(left.time + right.time);
    }

    friend timespan_t operator-(timespan_t left, timespan_t right)
    {
      assert(right < zero() || left >= right + min());
      assert(right > zero() || left <= right + max());
      return timespan_t(left.time - right.time);
    }

    template<typename Rep>
    friend typename std::enable_if<std::is_arithmetic<Rep>::value, timespan_t>::type operator*(
        timespan_t left, Rep right)
    {
      return timespan_t(left.time * right);
    }

    template<typename Rep>
    friend typename std::enable_if<std::is_arithmetic<Rep>::value, timespan_t>::type operator*(
        Rep left, timespan_t right)
    {
      return timespan_t(left * right.time);
    }

    template<typename Rep>
    friend typename std::enable_if<std::is_arithmetic<Rep>::value, timespan_t>::type operator/(
        timespan_t left, Rep right)
    {
      return timespan_t(left.time / right);
    }

    friend double operator/(timespan_t left, timespan_t right)
    {
      return static_cast<double>(left.time) / right.time;
    }

    friend timespan_t operator%(timespan_t left, timespan_t right)
    {
      left %= right;
      return left;
    }

    typedef time_t native_t;

    // Only to be used to convert without loss of precision for a computation
    // that will later be converted back via from_native().
    static native_t to_native(timespan_t t)
    {
      return t.time;
    }

    // Only to be used to convert the result of to_native().
    template<typename Rep>
    static timespan_t from_native(Rep t)
    {
      return timespan_t(t);
    }

    static timespan_t zero()
    {
      return timespan_t();
    }
    static timespan_t max()
    {
      return timespan_t(std::numeric_limits<time_t>::max());
    }
    static timespan_t min()
    {
      if (std::is_floating_point<time_t>::value)
        return timespan_t(-std::numeric_limits<time_t>::max());
      else
        return timespan_t(std::numeric_limits<time_t>::min());
    }
  };

  inline std::ostream& operator<<(std::ostream &os, const timespan_t& x)
  {
    os << x.total_seconds() << "seconds";
    return os;
  }
} // namespace timespan_adl_barrier

using timespan_adl_barrier::timespan_t;

#endif // SC_TIMESPAN_HPP
