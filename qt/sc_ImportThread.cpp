// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include "sc_OptionsTab.hpp"

#ifdef SC_PAPERDOLL
#include "sc_PaperDoll.hpp"
#endif
#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

// ==========================================================================
// Import
// ==========================================================================

void SC_ImportThread::start( sim_t* s, const QString& reg, const QString& rea, const QString& cha, const QString& spe )
{
  mainWindow -> soloChar -> start( 50 );

  tab             = TAB_IMPORT_NEW;
  item_db_sources = mainWindow -> optionsTab -> get_db_order();
  api             = mainWindow -> optionsTab -> get_api_key();
  m_role          = mainWindow -> optionsTab -> get_player_role();

  sim             = s;
  region          = reg;
  realm           = rea;
  character       = cha;
  spec            = spe;

  QThread::start();
}

void SC_ImportThread::importWidget()
{
  // Windows 7 64bit somehow cannot handle straight toStdString() conversion, so
  // do it in a silly way as a workaround for now.
  std::string cpp_r   = region.toUtf8().constData();
  std::string cpp_s   = realm.toUtf8().constData();
  std::string cpp_c   = character.toUtf8().constData();
  std::string cpp_sp  = spec.toUtf8().constData();
  player = bcp_api::download_player( sim, cpp_r, cpp_s, cpp_c, cpp_sp );
}

void SC_ImportThread::run()
{
  cache::advance_era();
  sim -> parse_option( "item_db_source", item_db_sources.toUtf8().constData() );
  if ( api.size() == 32 ) // api keys are 32 characters long.
    sim -> parse_option( "apikey", api.toUtf8().constData() );
  switch ( tab )
  {
    case TAB_IMPORT_NEW: importWidget(); break;
    default: assert( 0 ); break;
  }

  if ( player )
  {
    player -> role = util::parse_role_type( m_role.toUtf8().constData() );

    if ( sim->init() )
    {
      std::string buffer = player -> create_profile();
      profile = QString::fromUtf8( buffer.c_str() );
    }
    else player = 0;
  }
}
