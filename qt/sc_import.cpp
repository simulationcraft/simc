// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#ifdef SC_PAPERDOLL
#include "simcpaperdoll.hpp"
#endif
#include <QtWebKit>
#ifdef Q_WS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif


// ==========================================================================
// Import
// ==========================================================================

void ImportThread::importBattleNet()
{
  QString region, server, character;
  QUrl qurl = url;

  {
    QStringList parts = qurl.host().split( '.' );

    if ( parts.size() )
    {
      if ( parts[ parts.size() - 1 ].length() == 2 )
        region = parts[ parts.size() - 1 ];
      else
      {
        for ( QStringList::size_type i = 0; i < parts.size(); ++i )
        {
          if ( parts[ i ].length() == 2 )
          {
            region = parts[ i ];
            break;
          }
        }
      }
    }
  }

  {
    QStringList parts = qurl.path().split( '/' );
    for ( QStringList::size_type i = 0, n = parts.size(); i + 2 < n; ++i )
    {
      if ( parts[ i ] == "character" )
      {
        server = parts[ i + 1 ];
        character = parts[ i + 2 ];
        break;
      }
    }
  }

  if ( region.isEmpty() || server.isEmpty() || character.isEmpty() )
  {
    sim -> errorf( "Unable to determine Server and Character information!\n" );
  }
  else
  {
    // Windows 7 64bit somehow cannot handle straight toStdString() conversion, so
    // do it in a silly way as a workaround for now.
    std::string talents = active_spec.toStdString(),
                cpp_r   = region.toUtf8().constData(),
                cpp_s   = server.toUtf8().constData(),
                cpp_c   = character.toUtf8().constData();
    player = bcp_api::download_player( sim, cpp_r, cpp_s, cpp_c, talents );
  }
}


void ImportThread::run()
{
  cache::advance_era();
  sim -> parse_option( "item_db_source", item_db_sources.toUtf8().constData() );

  switch ( tab )
  {
    case TAB_BATTLE_NET: importBattleNet(); break;
    default: assert( 0 ); break;
  }

  if ( player )
  {
    player -> role = util::parse_role_type( m_role.toUtf8().constData() );

    if ( sim->init() )
    {
      std::string buffer = "";
      player -> create_profile( buffer );
      profile = QString::fromUtf8( buffer.c_str() );
    }
    else player = 0;
  }
}
