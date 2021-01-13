// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "ImportTab.hpp"
#include "MainWindow.hpp"
#include "interfaces/bcp_api.hpp"
#include "lib/fmt/format.h"
#include "player/sc_player.hpp"
#include "sc_OptionsTab.hpp"
#include "simulationcraftqt.hpp"

#include <utility>

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

// ==========================================================================
// Import
// ==========================================================================

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
void SC_ImportThread::start( std::shared_ptr<sim_t> s, const QString& reg, const QString& rea, const QString& cha,
                             const QString& spe )
{
  mainWindow->soloChar->start( 50 );

  tab             = TAB_IMPORT_NEW;
  item_db_sources = mainWindow->optionsTab->get_db_order();
  api             = mainWindow->optionsTab->get_api_key();
  m_role          = mainWindow->optionsTab->get_player_role();
  error           = "";

  sim       = std::move(s);
  region    = reg;
  realm     = rea;
  character = cha;
  spec      = spe;

  QThread::start();
}

void SC_ImportThread::importWidget()
{
  // Windows 7 64bit somehow cannot handle straight toStdString() conversion, so
  // do it in a silly way as a workaround for now.
  std::string cpp_r  = region.toUtf8().constData();
  std::string cpp_s  = realm.toUtf8().constData();
  std::string cpp_c  = character.toUtf8().constData();
  std::string cpp_sp = spec.toUtf8().constData();
  player             = bcp_api::download_player( sim.get(), cpp_r, cpp_s, cpp_c, cpp_sp );
}

void SC_ImportThread::run()
{
  try
  {
    cache::advance_era();
    sim->parse_option( "item_db_source", item_db_sources.toUtf8().constData() );
    if ( api.size() == 32 )  // api keys are 32 characters long.
      sim->parse_option( "apikey", api.toUtf8().constData() );
    switch ( tab )
    {
      case TAB_IMPORT_NEW:
        importWidget();
        break;
      default:
        assert( 0 );
        break;
    }

    if ( player )
    {
      player->role = util::parse_role_type( m_role.toUtf8().constData() );

      sim->init();
      if ( sim->canceled )
      {
        player = nullptr;
        return;
      }
      std::string buffer = player->create_profile();
      profile            = QString::fromUtf8( buffer.c_str() );
    }
  }
  catch ( const std::exception& e )
  {
    std::string error_str_;
    print_exception( error_str_, e );
    error  = QString::fromStdString( error_str_ );
    player = nullptr;
  }
}
