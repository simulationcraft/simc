
#include "sample_data.hpp"

#include <ostream>

std::ostream& extended_sample_data_t::data_str( std::ostream& s ) const
  {
    s << "Sample_Data \"" << name_str << "\": count: " << count();
    s << " mean: " << mean() << " variance: " << variance
      << " mean variance: " << mean_variance
      << " mean_std_dev: " << mean_std_dev << "\n";

    if ( !data().empty() )
      s << "data: ";
    for ( size_t i = 0, size = data().size(); i < size; ++i )
    {
      if ( i > 0 )
        s << ", ";
      s << data()[ i ];
    }
    s << "\n";
    return s;
  }

#ifdef UNIT_TEST
#include <iostream>

int main( int /*argc*/, char** /*argv*/ )
{
  simple_sample_data_t x;
  simple_sample_data_with_min_max_t y;
  extended_sample_data_t z( "foo", false );

  for( int i = 0; i < 1000; ++i )
    z.add( rand() );

  z.analyze_all();

  std::ostringstream s;
  z.data_str( s );
  std::cout << s.str();
  return 0;
}
#endif // UNIT_TEST
