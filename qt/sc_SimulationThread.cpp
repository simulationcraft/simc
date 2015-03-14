#include "sc_SimulationThread.hpp"
#include "simulationcraftqt.hpp"

SC_SimulateThread::SC_SimulateThread( SC_MainWindow* mw ) :
    mainWindow( mw ),
    sim( 0 )
{
  connect( this, SIGNAL( finished() ), this, SLOT( sim_finished() ) );
}

void SC_SimulateThread::run()
{
  sim_control_t description;
  try
  {
    description.options.parse_text( utf8_options.constData() );
  }
  catch ( const std::exception& e )
  {
    success = false;
    error_str = QString( tr("Option parsing error: ") ) + e.what();
    return;
  }

  try
  {
    sim -> setup( &description );
  }
  catch ( const std::exception& e )
  {
    success = false;
    error_str = QString( tr("Simulation setup error: ") ) + e.what();
    return;
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
    sim -> reforge_plot -> analyze();
    report::print_suite( sim );
  }
}
void SC_SimulateThread::start( sim_t* s, const QByteArray& o )
{
    sim = s; utf8_options = o; success = false; QThread::start();
}
