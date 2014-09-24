#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include <QLocale>
#include <QtWidgets/QApplication>
#include <locale>

/* Parse additional arguments
 * 1. Argument is parsed as a file name, complete content goes into simulate tab.
 * Following arguments are appended as new lines.
 */
void parse_additional_args( SC_MainWindow& w, QStringList args )
{
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

  // Localization
  QTranslator qtTranslator;
  qtTranslator.load( "qt_" + QLocale::system().name(),
                     QLibraryInfo::location( QLibraryInfo::TranslationsPath ) );
  a.installTranslator( &qtTranslator );

  QString path_to_locale = QString( "locale" );

  QTranslator myappTranslator;
  myappTranslator.load( QString( "sc_" ) + QLocale::system().name(), path_to_locale );
  a.installTranslator( &myappTranslator );

  // Setup search paths for resources
  {
    QString appDirPath = a.applicationDirPath();
    QDir::addSearchPath( "icon", ":/icons/" );
    QDir::addSearchPath( "icon", QString( "%1/../Resources/" ).arg( appDirPath ) );
    QDir::addSearchPath( "icon", QString( "%1/qt/icons/" ).arg( appDirPath ) );
    QDir::addSearchPath( "icon", "./qt/icons/" );
  }

  SC_MainWindow w;

  parse_additional_args( w, a.arguments() );

  return a.exec();
}
