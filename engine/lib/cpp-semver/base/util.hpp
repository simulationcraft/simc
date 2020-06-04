#ifndef CPP_SEMVER_UTIL_HPP
#define CPP_SEMVER_UTIL_HPP

#include <string>
#include <vector>

namespace semver
{
  const std::string any_space = " \n\r\t\v\f";
  const std::string any_number = "0123456789";
  const std::string any_alphabat = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  // [    x y    z  ] -> [ x y z ]
  inline std::string reduce_space(const std::string& str)
  {
    std::string new_string;
    bool pre_space = false;
    for (const char& c : str)
    {
      const bool has_space = (any_space.find(c) != std::string::npos);
      if (pre_space && has_space)
        continue;

      new_string.push_back(c);
      pre_space = has_space;
    }
    return new_string;
  }

  // [    x y    z  ] -> [x y    z]
  inline std::string trim_string(const std::string& str)
  {
    size_t start = str.find_first_not_of(any_space);
    if (start == std::string::npos)
      return {};

    size_t end = str.find_last_not_of(any_space);
    if (end == std::string::npos)
      return str.substr(start);

    return str.substr(start, end - start + 1);
  }

  inline std::vector<std::string> split(const std::string& input, const std::string& delimiter, bool trim = false)
  {
    std::vector<std::string> result;

    size_t search_pos = 0;
    while (1)
    {
      size_t found = input.find(delimiter, search_pos);
      if (found == std::string::npos)
      {
        result.emplace_back(input.substr(search_pos));
        break;
      }
      result.emplace_back(input.substr(search_pos, found - search_pos));
      search_pos = found + delimiter.length();
    }

    if (trim)
      for (std::string& s : result)
        s = trim_string(s);

    return result;
  }
}

#endif