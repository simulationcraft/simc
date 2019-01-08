#include "sc_SimulationThread.hpp"
#include "simulationcraftqt.hpp"
#include "sc_Workaround.hpp"

namespace {

/**
 * Print chained exceptions, separated by ' :'.
 */
void print_exception( std::string& output, const std::exception& e, int level =  0)
{
  std::string tmp = fmt::format("{}{}", level > 0 ? ": " : "", e.what());
  output += tmp;
  try {
      std::rethrow_if_nested(e);
  } catch(const std::exception& e) {
      print_exception(output, e, level+1);
  } catch(...) {}
}

}
SC_SimulateThread::SC_SimulateThread( SC_MainWindow* mw ) :
    mainWindow( mw ),
    sim( nullptr ),
    utf8_options(),
    tabName(),
    error_category(),
    error_str(),
    success( false )
{
  connect( this, SIGNAL( finished() ), this, SLOT( sim_finished() ) );
}

void SC_SimulateThread::run()
{
  // ********* Parsing **********
  try
  {
    sim_control_t description;
    error_str.clear();
    try
    {
      description.options.parse_text( utf8_options.constData() );
    }
    catch ( const std::exception& e )
    {
      std::throw_with_nested(std::invalid_argument("Incorrect option format"));
    }

    // ******** Setup ********
    try
    {
      sim -> setup( &description );
      workaround::apply_workarounds( sim.get() );
    }
    catch ( const std::exception& e )
    {
      std::throw_with_nested(std::runtime_error("Setup failure"));
    }

    if ( sim -> challenge_mode ) sim -> scale_to_itemlevel = 630;

    if ( sim -> spell_query != 0 )
    {
      success = false;
      return;
    }

    success = sim -> execute();
    if ( success )
    {
      sim -> scaling -> analyze();
      sim -> plot -> analyze();
<<<<<<< HEAD
      sim -> reforge_plot -> analyze();
      report::print_suite( sim.get() );
    }
    else
    {
      error_category = tr( "Simulation runtime error" );
      range::for_each( sim->error_list, [ this ]( const std::string& str ) {
        if ( !error_str.isEmpty() )
        {
          error_str += "\n";
        }

        error_str += QString::fromStdString( str );
      } );
=======
      sim -> reforge_plot -> start();
      report::print_suite( sim );
>>>>>>> 1c5f9bd6725cdfece4184bf1f8645dc1aab69b9c
    }
  }
  catch ( const std::exception& e )
  {
    success = false;
    error_category = tr("Simulation runtime error");
    std::string error_str_;
    print_exception(error_str_, e);
    error_str = QString::fromStdString(error_str_);
  }
}

void SC_SimulateThread::start( std::shared_ptr<sim_t> s, const QByteArray& o, QString t )
{
    sim = s;
    utf8_options = o;
    success = false;
    tabName = t;
    QThread::start();
}
