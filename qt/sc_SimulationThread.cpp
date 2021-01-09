#include "sc_SimulationThread.hpp"

#include "lib/fmt/format.h"
#include "report/reports.hpp"
#include "sc_Workaround.hpp"
#include "sim/plot.hpp"
#include "sim/reforge_plot.hpp"
#include "sim/sc_sim.hpp"
#include "sim/scale_factor_control.hpp"
#include "sim/sim_control.hpp"
#include "simulationcraftqt.hpp"

#include <utility>

namespace
{
/**
 * Print chained exceptions, separated by ' :'.
 */
void print_exception( std::string& output, const std::exception& e, int level = 0 )
{
  std::string tmp = fmt::format( "{}{}", level > 0 ? ": " : "", e.what() );
  output += tmp;
  try
  {
    std::rethrow_if_nested( e );
  }
  catch ( const std::exception& e )
  {
    print_exception( output, e, level + 1 );
  }
  catch ( ... )
  {
  }
}

}  // namespace
SC_SimulateThread::SC_SimulateThread( SC_MainWindow* mw )
  : mainWindow( mw ), sim( nullptr ), utf8_options(), tabName(), error_category(), error_str(), success( false )
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
      description.options.parse_text( util::string_view( utf8_options.constData(), utf8_options.size() ) );
    }
    catch ( const std::exception& e )
    {
      std::throw_with_nested( std::invalid_argument( "Incorrect option format" ) );
    }

    // ******** Setup ********
    try
    {
      sim->setup( &description );
      workaround::apply_workarounds( sim.get() );
    }
    catch ( const std::exception& e )
    {
      std::throw_with_nested( std::runtime_error( "Setup failure" ) );
    }

    if ( sim->challenge_mode )
      sim->scale_to_itemlevel = 630;

    if ( sim->spell_query != nullptr )
    {
      success = false;
      return;
    }

    success = sim->execute();
    if ( success )
    {
      sim->scaling->analyze();
      sim->plot->analyze();
      sim->reforge_plot->analyze();
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
    }
  }
  catch ( const std::exception& e )
  {
    success        = false;
    error_category = tr( "Simulation runtime error" );
    std::string error_str_;
    print_exception( error_str_, e );
    range::for_each( sim->error_list, [ this ]( const std::string& str ) {
      if ( !error_str.isEmpty() )
      {
        error_str += "\n";
      }

      error_str += QString::fromStdString( str );
    } );
    error_str += QString::fromStdString( error_str_ );
  }
}

void SC_SimulateThread::start( std::shared_ptr<sim_t> s, const QByteArray& o, QString t )
{
  sim          = std::move(s);
  utf8_options = o;
  success      = false;
  tabName      = std::move(t);
  QThread::start();
}
