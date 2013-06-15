#ifdef UNIT_TEST
#include "sample_data.hpp"
#include <iostream>

int main( int /*argc*/, char** /*argv*/ )
{
  simple_sample_data_t x;
  simple_sample_data_with_min_max_t y;
  extended_sample_data_t z("foo", false);

  for( int i = 0; i < 1000; ++i )
    z.add( rand() );

  z.analyze_all();

  std::ostringstream s;
  z.data_str( s );
  std::cout << s.str();
  return 0;
}
#endif // UNIT_TEST
