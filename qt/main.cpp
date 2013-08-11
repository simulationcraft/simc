#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include <QLocale>
#ifdef QT_VERSION_5
#include <QtGui/QGuiApplication>
#else
#include <QtGui/QApplication>
#endif
#include <locale>
#ifndef SIMC_NO_AUTOUPDATE
#include "sc_autoupdate.h"
#endif /* SIMC_NO_AUTOUPDATE */

int main( int argc, char *argv[] )
{
  QLocale::setDefault( QLocale( "C" ) );
  std::locale::global( std::locale( "C" ) );
  setlocale( LC_ALL, "C" );

  dbc::init();
  module_t::init();

  QApplication a( argc, argv );
  QCoreApplication::setApplicationName( "SimulationCraft" );
  QCoreApplication::setApplicationVersion( SC_VERSION );
  QCoreApplication::setOrganizationDomain( "http://code.google.com/p/simulationcraft/" );
  QCoreApplication::setOrganizationName( "SimulationCraft" );
  QSettings::setDefaultFormat( QSettings::IniFormat ); // Avoid Registry entries on Windows


  QNetworkProxyFactory::setUseSystemConfiguration( true );

#ifndef SIMC_NO_AUTOUPDATE
  AutoUpdater* updater = 0;

  // Localization
  QTranslator qtTranslator;
  qtTranslator.load( "qt_" + QLocale::system().name(),
                     QLibraryInfo::location( QLibraryInfo::TranslationsPath ) );
  a.installTranslator( &qtTranslator );

  QString path_to_locale = QString( "locale" );

  QTranslator myappTranslator;
  myappTranslator.load( QString( "sc_" ) + QLocale::system().name(), path_to_locale );
  a.installTranslator( &myappTranslator );

#if defined( Q_WS_MAC ) || defined( Q_OS_MAC )

  CocoaInitializer cocoaInitializer;
  updater = new SparkleAutoUpdater( "http://simc.rungie.com/simcqt/update.xml" );

#endif

  if ( updater )
  {
    updater -> checkForUpdates();
  }
#endif /* SIMC_NO_AUTOUPDATE */

  // Setup search paths for resources
  {
    QString appDirPath = a.applicationDirPath();
    QDir::addSearchPath( "icon", ":/icons/" );
    QDir::addSearchPath( "icon", QString( "%1/../Resources/" ).arg( appDirPath ) );
    QDir::addSearchPath( "icon", QString( "%1/qt/icons/" ).arg( appDirPath ) );
    QDir::addSearchPath( "icon", "./qt/icons/" );
  }

  SC_MainWindow w;

  {
    QStringList args = a.arguments();
    if ( args.size() > 1 )
    {
      for ( int i = 1; i < args.size(); ++i )
      {
        if ( i > 1 )
          w.simulateTab -> current_Text() -> append( "\n" );

        QFile file( args[ i ] );
        if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
          QTextStream ts( &file );
          ts.setCodec( "UTF-8" );
          ts.setAutoDetectUnicode( true );
          w.simulateTab -> current_Text() -> append( ts.readAll() );
          file.close();
        }
      }
      w.mainTab -> setCurrentTab( TAB_SIMULATE );
    }
  }

  return a.exec();
}
