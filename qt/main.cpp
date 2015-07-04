#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include <QLocale>
#include <QtWidgets/QApplication>
#include "sc_SimulateTab.hpp"
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
        w.simulateTab -> current_Text() -> appendPlainText( "\n" );

      QFile file( args[ i ] );
      if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
      {
        QTextStream ts( &file );
        ts.setCodec( "UTF-8" );
        ts.setAutoDetectUnicode( true );
        w.simulateTab -> current_Text() -> appendPlainText( ts.readAll() );
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
  unique_gear::register_hotfixes();
  unique_gear::register_special_effects();

  hotfix::apply();

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

  QString lang;
  QSettings settings;
  lang = settings.value( "options/gui_localization", "auto" ).toString();
  qDebug() << "[Localization]: Seleced gui language: " << lang;
  if ( lang == "auto" )
  {
    lang = QLocale::system().name().split('_').at(1).toLower();
    qDebug() << "[Localization]: QLocale system language: " << lang;
  }
  QTranslator myappTranslator;
  if ( !lang.isEmpty() && !lang.startsWith( "en" ) )
  {
    QString path_to_locale = SC_PATHS::getDataPath() + "/locale";

    QString qm_file = QString( "sc_" ) + lang;
    qDebug() << "[Localization]: Trying to load local file from: " << path_to_locale << "/" << qm_file << ".qm";
    myappTranslator.load( qm_file, path_to_locale );
    qDebug() << "[Localization]: Loaded translator isEmpty(): " << myappTranslator.isEmpty();
  }
  else
  {
    qDebug() << "[Localization]: No specific translator chosen, using English.";
  }
  a.installTranslator( &myappTranslator );

  QString iconlocation = QStandardPaths::locate( QStandardPaths::DataLocation,
                                                 QString( "icon" ),
                                                 QStandardPaths::LocateDirectory );
  QDir::addSearchPath( "icon", iconlocation );

  SC_MainWindow w;

  parse_additional_args( w, a.arguments() );

  return a.exec();
}
