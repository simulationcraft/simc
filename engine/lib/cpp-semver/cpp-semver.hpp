#ifndef CPP_SEMVER_HPP
#define CPP_SEMVER_HPP

#include "base/type.hpp"
#include "base/util.hpp"

#ifdef USE_PEGTL
#include "parser/peg.hpp" // PETGTL parser
#define PARSER peg::parser
#else
#include "parser/parser.hpp"
#define PARSER semver::parser
#endif

#include <string>
#include <vector>
#include <memory>

namespace semver
{
  namespace detail
  {
    /* parse version range in string into syntactical representation @c syntax::range_set */
    inline syntax::range_set parse(const std::string& input)
    {
      return PARSER(input);
    }

    /* parse syntactical representation @c syntax::simple and convert it to sementical representation @c semantic::interval */
    inline semantic::interval parse(const syntax::simple& input)
    {
      // default result * := [min, max]
      semantic::interval result;

      semantic::boundary b; // associate boundary from input x.y.z-pre
                            // replace * with 0
      b.major = (!input.major.is_wildcard ? input.major.value : 0);
      b.minor = (!input.minor.is_wildcard ? input.minor.value : 0);
      b.patch = (!input.patch.is_wildcard ? input.patch.value : 0);
      b.pre = input.pre;

      switch (input.cmp)
      {
      case syntax::comparator::lt:         // <x.y.z-pre := [min, x.y.z-pre)

        result.to_inclusive = false;
        result.to = b;

        if (input.major.is_wildcard ||
            input.minor.is_wildcard ||
            input.patch.is_wildcard)      // <x.*-pre   := [min, x.0.0)
          result.to.pre = "";             // <x.y.*-pre := [min, x,y.0)

        break;

      case syntax::comparator::lte:        // <=x.y.z.pre := [min, x.y.z-pre]

        result.to = b;

        if (input.major.is_wildcard ||
            input.minor.is_wildcard ||
            input.patch.is_wildcard)              // <=x.*-pre   := [min, 'x+1'.0.0)
        {                                         // <=x.y.*-pre := [min, x,'y+1'.0)
          result.to_inclusive = false;
          result.to.pre = "";
        }

        if (input.major.is_wildcard)              // <=* := [min, max]
        {
          result = semantic::interval();
        }
        else if (input.minor.is_wildcard)         // <=x.*-pre := [min, 'x+1'.0.0)
        {
          result.to.major += 1;
          result.to.patch = 0;
        }
        else if (input.patch.is_wildcard)        // <=x.y.*-pre := [min, x,'y+1'.0)
        {
          result.to.minor += 1;
        }

        break;

      case syntax::comparator::gt:         // >x.y.z-pre := (x.y.z-pre, max]
        result.from_inclusive = false;
        result.from = b;

        if (input.major.is_wildcard ||
            input.minor.is_wildcard ||
            input.patch.is_wildcard)       // >x.*-pre   := ['x+1', max]
        {                                  // >x.y.*-pre := [x.'y+1', max]
          result.from_inclusive = true;
          result.from.pre = "";
        }

        if (input.major.is_wildcard)
        {
          result.from = semantic::boundary::max(); // invalid set
        }
        else if (input.minor.is_wildcard)             // >x.*-pre := ['x+1'.0.0, max]
        {
          result.from.major += 1;
          result.from.patch = 0;
        }
        else if (input.patch.is_wildcard)             // >x.y.*-pre := [x,'y+1'.0, max]
        {
          result.from.minor += 1;
        }

        break;

      case syntax::comparator::gte:        // >=x.y.z-pre := [x.y.z-pre, max]

        result.from = b;

        if (input.major.is_wildcard ||
            input.minor.is_wildcard ||
            input.patch.is_wildcard)     // >=x.*-pre   := [x.0.0, max]
          result.from.pre = "";          // >=x.y.*-pre := [x.y.0, max]

        break;

      case syntax::comparator::caret:

        result.from = b;
        result.to = b;

        if (input.major.is_wildcard ||
            input.minor.is_wildcard ||
            input.patch.is_wildcard)
          result.from.pre = "";

        result.to_inclusive = false;
        result.to.pre = "";

        if (input.major.is_wildcard)              // ^* := [min, max]
        {
          result = semantic::interval();
        }
        else if (input.major.value != 0 ||  // ^x.y.z-pre := [x.y.z-pre, 'x+1'.0.0)
          input.minor.is_wildcard)     // ^x.y       := [x.y.0, 'x+1'.0.0)
        {                              // ^x         := [x.0.0, 'x+1'.0.0)
          result.to.major += 1;        // ^0         := [0.0.0, 1.0.0)
          result.to.minor = 0;
          result.to.patch = 0;
        }
        else if (input.minor.value != 0 || // ^0.y.z := [0.y.z, 0.'y+1'.z)
          input.patch.is_wildcard)        // ^0.y   := [0.y.0, 0.'y+1'.0)
        {
          result.to.minor += 1;
          result.to.patch = 0;
        }
        else if (input.patch.value != 0)
        {
          result.to.patch += 1; // ^0.0.z := [0.0.z, 0.0.'z+1')
        }

        break;

      case syntax::comparator::tilde:

        result.from = b;
        result.to = b;

        if (input.major.is_wildcard ||
            input.minor.is_wildcard ||
            input.patch.is_wildcard)
          result.from.pre = "";

        result.to_inclusive = false;
        result.to.pre = "";

        if (input.major.is_wildcard)              // ~* := [min, max]
        {
          result = semantic::interval();
        }
        else if (input.minor.is_wildcard)         // ~x-pre := [x.0.0, 'x+1'.0.0)
        {
          result.from.pre = "";

          result.to.major += 1;
          result.to.minor = 0;
          result.to.patch = 0;
        }
        else                           // ~x.y.z-pre := [x.y.z-pre, x.'y+1'.0)
        {
          result.to.minor += 1;
          result.to.patch = 0;
        }

        break;

      case syntax::comparator::eq:
      default:

        result.from = b;                   // x.y.z := [x.y.z, x.y.z]
        result.to = b;

        if (input.major.is_wildcard ||
            input.minor.is_wildcard ||
            input.patch.is_wildcard)       // =x.*-pre   := [x.0.0, 'x+1'.0.0)
        {                                  // =x.y.*-pre := [x.y.0, x.'y+1'.0)
          result.from.pre = "";

          result.to_inclusive = false;
          result.to.pre = "";
        }

        if (input.major.is_wildcard)                  // * := [min, max]
        {
          result = semantic::interval();
        }
        else if (input.minor.is_wildcard)             // x := [x.0.0, 'x+1'.0.0)
        {
          result.from.patch = 0;

          result.to.major += 1;
          result.to.patch = 0;
        }
        else if (input.patch.is_wildcard)              // x.y   := [x.y.0, x.'y+1'.0)
        {
          result.to.minor += 1;
        }
      }

      return result;
    };

    /* calculate AND-conjunction from a list of @c semantic::interval */
    inline std::unique_ptr<semantic::interval> and_conj(const std::vector< semantic::interval >& input)
    {
      const size_t size = input.size();

      if (size == 0)
        return nullptr;

      semantic::interval result = input.at(0);

      for (size_t i = 1; i < size; i++)
      {
        const semantic::interval& span = input.at(i);

        // prerelease tag case
        {
          const bool result_is_range = (result.from != result.to || result.from_inclusive != result.to_inclusive);
          const bool span_is_range = (span.from != span.to || span.from_inclusive != span.to_inclusive);

          if (result_is_range != span_is_range)
          {
            const semantic::interval& range = (result_is_range ? result : span);
            const semantic::interval& target = (result_is_range ? span : result);
            const bool range_from_has_pre = !range.from.pre.empty();
            const bool range_to_has_pre = !range.to.pre.empty();
            const bool target_has_pre = !target.from.pre.empty();

            if (target_has_pre)
            {
              // 1-rc not in '0 - 10'
              if (!range_from_has_pre && !range_to_has_pre)
                return nullptr;

              // 1-rc in '1-beta - 10'
              if (range_from_has_pre && !range_to_has_pre)
                if (range.from.major != target.from.major ||
                  range.from.minor != target.from.minor ||
                  range.from.patch != target.from.patch)
                  return nullptr;

              // 10-beta in '1 - 10-rc'
              if (!range_from_has_pre && range_to_has_pre)
                if (range.to.major != target.to.major ||
                  range.to.minor != target.to.minor ||
                  range.to.patch != target.to.patch)
                  return nullptr;

              // 1-rc or 10-beta in '1-beta - 10-rc'
              if (range_from_has_pre && range_to_has_pre)
                if (!((range.from.major == target.from.major &&
                  range.from.minor == target.from.minor &&
                  range.from.patch == target.from.patch) ||
                  (range.to.major == target.to.major &&
                    range.to.minor == target.to.minor &&
                    range.to.patch == target.to.patch)))
                  return nullptr;
            }
          }
        } // prerelease tag case

        if ((span.from > result.from) ||
          ((span.from == result.from) && !span.from_inclusive))
        {
          result.from = span.from;
          result.from_inclusive = span.from_inclusive;
        }

        if ((span.to < result.to) ||
          ((span.to == result.to) && !span.to_inclusive))
        {
          result.to = span.to;
          result.to_inclusive = span.to_inclusive;
        }
      }

      if ((result.from > result.to) ||
        ((result.from == result.to) && (result.from_inclusive != result.to_inclusive)))
        return nullptr;

      return std::unique_ptr<semantic::interval>(new semantic::interval(result));
    }

    /* parse syntactical representation @c syntax::range_set and convert it to sementical representation @c semantic::interval_set */
    inline semantic::interval_set parse(const syntax::range_set& input)
    {
      semantic::interval_set interval_set;
      for (const syntax::range& range : input.or_set)
      {
        std::vector<semantic::interval> and_set;
        for (const syntax::simple& simple : range.and_set)
          and_set.emplace_back(parse(simple));
        const std::unique_ptr<semantic::interval> conj = and_conj(and_set);
        if (conj)
          interval_set.or_set.emplace_back(std::move(*conj));
      }
      return interval_set;
    }

    /* check if two @c semantic::interval_set intersect with each other */
    inline bool intersects(const semantic::interval_set& s1, const semantic::interval_set& s2)
    {
      if (s1.or_set.empty() || s2.or_set.empty())
        return false;

      for (const semantic::interval& intval_1 : s1.or_set)
        for (const semantic::interval& intval_2 : s2.or_set)
          if (and_conj({ intval_1, intval_2 }))
            return true;

      return false;
    }

    /* check if two @c syntax::range_set intersect with each other */
    inline bool intersects(const syntax::range_set& rs1, const syntax::range_set& rs2)
    {
      return intersects(parse(rs1), parse(rs2));
    }

    /** parse or cast as a syntax::simple if possible */
    inline syntax::simple as_simple(const std::string& s)
    {
      syntax::range_set parsed = parse(s);

      if (!parsed.or_set.empty() && !parsed.or_set.at(0).and_set.empty())
        return std::move(parsed.or_set.at(0).and_set.at(0));

      return{};
    }
  }


  // *************** API **************************************** //


  /** Return true if the comparator or range intersect */
  inline bool intersects(const std::string& range)
  {
    return detail::intersects(detail::parse(range), detail::parse("*"));
  }

  /** Return true if the two supplied ranges or comparators intersect. */
  inline bool intersects(const std::string& range1, const std::string& range2)
  {
    return detail::intersects(detail::parse(range1), detail::parse(range2));
  }

  /**  Return true if the version satisfies the range */
  inline bool satisfies(const std::string& version, const std::string& range)
  {
    return detail::intersects(detail::parse(version), detail::parse(range));
  }

  /** v1 == v2 */
  inline bool eq(const std::string& v1, const std::string& v2)
  {
    semantic::interval_set interval_set_1 = detail::parse(detail::parse(v1));
    semantic::interval_set interval_set_2 = detail::parse(detail::parse(v2));

    if (interval_set_1.or_set.empty() && interval_set_2.or_set.empty())
      return true;
    else if (interval_set_1.or_set.empty() != interval_set_2.or_set.empty())
      return false;

    for (const semantic::interval& interval_1 : interval_set_1.or_set)
      for (const semantic::interval& interval_2 : interval_set_2.or_set)
        if (interval_1 == interval_2)
          return true;

    return false;
  }

  /** v1 != v2 */
  inline bool neq(const std::string& v1, const std::string& v2)
  {
    return !eq(v1, v2);
  }

  /** v1 > v2 */
  inline bool gt(const std::string& v1, const std::string& v2)
  {
    semantic::interval_set interval_set_1 = detail::parse(detail::parse(v1));
    semantic::interval_set interval_set_2 = detail::parse(detail::parse(v2));

    if (!interval_set_1.or_set.empty() && interval_set_2.or_set.empty())
      return true;
    else if (interval_set_1.or_set.empty() || interval_set_2.or_set.empty())
      return false;

    for (const semantic::interval& interval_1 : interval_set_1.or_set)
      for (const semantic::interval& interval_2 : interval_set_2.or_set)
        if (interval_1 > interval_2)
          return true;

    return false;
  }

  /** v1 >= v2 */
  inline bool gte(const std::string& v1, const std::string& v2)
  {
    if (eq(v1, v2) || gt(v1, v2))
      return true;
    return false;
  }

  /** v1 < v2 */
  inline bool lt(const std::string& v1, const std::string& v2)
  {
    if (!eq(v1, v2) && !gt(v1, v2))
      return true;
    return false;
  }

  /** v1 <= v2 */
  inline bool lte(const std::string& v1, const std::string& v2)
  {
    if (eq(v1, v2) || lt(v1, v2))
      return true;
    return false;
  }

  /** Return true if version is greater than all the versions possible in the range. */
  inline bool gtr(const std::string& version, const std::string& range)
  {
    semantic::interval_set interval_set_v = detail::parse(detail::parse(version));
    semantic::interval_set interval_set_r = detail::parse(detail::parse(range));

    if (!interval_set_v.or_set.empty() && interval_set_r.or_set.empty())
      return true;
    else if (interval_set_v.or_set.empty() || interval_set_r.or_set.empty())
      return false;

    for (const semantic::interval& interval_v : interval_set_v.or_set)
      for (const semantic::interval& interval_r : interval_set_r.or_set)
        if (interval_v <= interval_r)
          return false;

    return true;
  }

  /** Return true if version is less than all the versions possible in the range. */
  inline bool ltr(const std::string& version, const std::string& range)
  {
    semantic::interval_set interval_set_v = detail::parse(detail::parse(version));
    semantic::interval_set interval_set_r = detail::parse(detail::parse(range));

    if (interval_set_v.or_set.empty() && !interval_set_r.or_set.empty())
      return true;
    else if (interval_set_v.or_set.empty() || interval_set_r.or_set.empty())
      return false;

    for (const semantic::interval& interval_v : interval_set_v.or_set)
      for (const semantic::interval& interval_r : interval_set_r.or_set)
        if (interval_v >= interval_r)
          return false;

    return true;
  }

  /** Return true if the version or range is valid */
  inline bool valid(const std::string& s)
  {
    try
    {
      semantic::interval_set interval_set_s = detail::parse(detail::parse(s));
      return !interval_set_s.or_set.empty();
    }
    catch (semver_error const&)
    {
      return false;
    }
  }

  /** Return the major version number. */
  inline int major(const std::string& version)
  {
    const auto& major = detail::as_simple(version).major;
    return !major.is_wildcard ? major.value : 0;
  }

  /** Return the minor version number. */
  inline int minor(const std::string& version)
  {
    const auto& minor = detail::as_simple(version).minor;
    return !minor.is_wildcard ? minor.value : 0;
  }

  /** Return the patch version number. */
  inline int patch(const std::string& version)
  {
    const auto& patch = detail::as_simple(version).patch;
    return !patch.is_wildcard ? patch.value : 0;
  }

  /** Returns an array of prerelease components. */
  inline std::vector<std::string> prerelease(const std::string& version)
  {
    const auto& pre = detail::as_simple(version).pre;
    if (pre.empty())
      return {};

    return split(pre, ".");
  }

}

#endif