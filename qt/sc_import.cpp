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

  if ( false )
  {
    QStringList tokens = url.split( QRegExp( "[?&=:/.]" ), QString::SkipEmptyParts );
    int count = tokens.count();
    for ( int i=0; i < count-1; i++ )
    {
      QString& t = tokens[ i ];
      if ( t == "http" )
      {
        region = tokens[ ++i ];
      }
      else if ( t == "r" ) // old armory
      {
        server = tokens[ ++i ];
      }
      else if ( t == "n" ) // old armory
      {
        character = tokens[ ++i ];
      }
      else if ( t == "character" && ( i<count-2 ) ) // new battle.net
      {
        server    = tokens[ ++i ];
        character = tokens[ ++i ];
      }
    }
  }

  if ( region.isEmpty() || server.isEmpty() || character.isEmpty() )
  {
    fprintf( sim->output_file, "Unable to determine Server and Character information!\n" );
  }
  else
  {
    // Windows 7 64bit somehow cannot handle straight toStdString() conversion, so
    // do it in a silly way as a workaround for now.
    std::string talents = mainWindow -> choice.armory_spec -> currentText().toUtf8().constData(),
                cpp_s   = server.toUtf8().constData(),
                cpp_c   = character.toUtf8().constData(),
                cpp_r   = region.toUtf8().constData();
    player = bcp_api::download_player( sim, cpp_r, cpp_s, cpp_c, talents );
  }
}

void ImportThread::importCharDev()
{
  int last_slash = url.lastIndexOf( '/' );
  int first_dash = url.indexOf( '-', last_slash );

  if ( last_slash > 0 && first_dash > 0 )
  {
    int len = first_dash - last_slash - 1;
    // Win7/x86_64 workaround
    std::string c = url.mid( last_slash + 1, len ).toUtf8().constData();
    player = chardev::download_player( sim, c, cache::players() );
  }
}

void ImportThread::importRawr()
{
  // Win7/x86_64 workaround
  std::string xml = mainWindow -> rawrText -> toPlainText().toUtf8().constData();
  player = rawr::load_player( sim, "rawr.xml", xml );
}

void ImportThread::run()
{
  cache::advance_era();
  sim -> parse_option( "item_db_source", item_db_sources.toUtf8().constData() );

  switch ( tab )
  {
  case TAB_BATTLE_NET: importBattleNet(); break;
  case TAB_CHAR_DEV:   importCharDev();   break;
  case TAB_RAWR:       importRawr();      break;
  default: assert( 0 ); break;
  }

  if ( player )
  {
    player -> role = util::parse_role_type( mainWindow-> choice.default_role->currentText().toUtf8().constData() );

    if ( sim->init() )
    {
      std::string buffer="";
      player -> create_profile( buffer );
      profile = QString::fromUtf8( buffer.c_str() );
    }
    else player = 0;
  }
}
