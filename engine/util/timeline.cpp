#include "timeline.hpp"

#include <ostream>

std::ostream& timeline_t::data_str( std::ostream& s ) const
{
  s << "Timeline: length: " << data().size();
  //s << " mean: " << mean() << " variance: " << variance << " mean_std_dev: " << mean_std_dev << "\n";
  if ( ! data().empty() )
    s << "data: ";
  for ( size_t i = 0, size = data().size(); i < size; ++i )
  { if ( i > 0 ) s << ", "; s << data()[ i ]; }
  s << "\n";
  return s;
}

#ifdef UNIT_TEST
#include <iostream>
int main( int /*argc*/, char** /*argv*/ )
{
  tl::timeline_t<double> x;

  std::cout << sizeof( double );

  return 0;
}
#endif // UNIT_TEST
