
#include "cache.hpp"

#include <ostream>

std::ostream& cache::operator<<(std::ostream &os, const cache_era& x)
{
  switch ( x )
  {
  case cache_era::INVALID:
    os << "invalid era";
    break;
  case cache_era::IN_THE_BEGINNING:
    os << "in the beginning";
    break;
  default:
    break;
  }
  return os;
}
