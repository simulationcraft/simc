
#include "cache.hpp"
namespace
{
const char* cache_era_str( cache::cache_era x )
{
  switch ( x )
  {
    case cache::cache_era::INVALID:
      return "invalid era";
    case cache::cache_era::IN_THE_BEGINNING:
      return "in the beginning";
    default:
      break;
  }
  return "unknown";
}

}  // namespace
void format_to( cache::cache_era era, fmt::format_context::iterator out )
{
  fmt::format_to( out, "{}", cache_era_str( era ) );
}