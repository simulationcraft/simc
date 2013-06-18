#include "timeline.hpp"
#include <iostream>
#ifdef UNIT_TEST

int main( int /*argc*/, char** /*argv*/ )
{
  tl::timeline_t<double> x;

  std::cout << sizeof( double );

  return 0;
}
#endif // UNIT_TEST
