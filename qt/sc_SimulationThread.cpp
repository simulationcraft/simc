#include "sc_SimulationThread.hpp"
#include "simulationcraftqt.hpp"
#include "sc_Workaround.hpp"

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
  sim_control_t description;
  try
  {
    description.options.parse_text( utf8_options.constData() );
  }
  catch ( const std::exception& e )
  {
    success = false;
    error_category = tr("Option parsing error");
    error_str = e.what();
    return;
  }

  // ******** Setup ********
  try
  {
    sim -> setup( &description );
    workaround::apply_workarounds( sim );
  }
  catch ( const std::exception& e )
  {
    success = false;
    error_category = tr("Simulation setup error");
    error_str = e.what();
    return;
  }

  // ********* Simulation *********
  try
  {
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
      sim -> reforge_plot -> analyze();
      report::print_suite( sim );
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
    success = false;
    error_category = tr("Simulation runtime error");
    error_str = e.what();
  }
}

void SC_SimulateThread::start( sim_t* s, const QByteArray& o, QString t )
{
    sim = s;
    utf8_options = o;
    success = false;
    tabName = t;
    QThread::start();
}
