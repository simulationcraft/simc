#include "MainWindow.hpp"
#include "class_modules/class_module.hpp"
#include "dbc/dbc.hpp"
#include "player/unique_gear.hpp"
#include "interfaces/bcp_api.hpp"
#include "sc_SimulateTab.hpp"
#include "simulationcraftqt.hpp"
#if 0
#include <fstream>
#endif

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
      QFile file( args[ i ] );
      if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
      {
        QTextStream ts( &file );
        ts.setAutoDetectUnicode( true );
        w.simulateTab->add_Text( ts.readAll(), args[ i ] );
        file.close();
      }
    }
    w.mainTab->setCurrentTab( TAB_SIMULATE );
  }
}

int main( int argc, char* argv[] )
{
#if ( QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 ) )
  QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
  qputenv( "QT_AUTO_SCREEN_SCALE_FACTOR", "1" );
#endif

  QApplication a( argc, argv );

  QLocale::setDefault( QLocale( "C" ) );
  std::locale::global( std::locale( "C" ) );
  setlocale( LC_ALL, "C" );

  dbc::init();
  module_t::init();
  unique_gear::register_hotfixes();
  unique_gear::register_special_effects();
  unique_gear::sort_special_effects();

  bcp_api::token_load();

  hotfix::apply();

  QApplication::setStyle( QStyleFactory::create( "Fusion" ) );
  QCoreApplication::setApplicationName( "SimulationCraft" );
  QCoreApplication::setApplicationVersion( SC_VERSION );
  QCoreApplication::setOrganizationDomain( "org.simulationcraft" );
  QCoreApplication::setOrganizationName( "SimulationCraft" );
  QSettings::setDefaultFormat( QSettings::IniFormat );  // Avoid Registry entries on Windows

  QNetworkProxyFactory::setUseSystemConfiguration( true );

  // Localization
  QTranslator qtTranslator;
  qtTranslator.load( "qt_" + QLocale::system().name(), QLibraryInfo::location( QLibraryInfo::TranslationsPath ) );
  a.installTranslator( &qtTranslator );

  QString lang;
  QSettings settings;
  lang = settings.value( "options/gui_localization", "auto" ).toString();
  qDebug() << "[Localization]: Selected gui language: " << lang;
  if ( lang == "auto" )
  {
    QString langName = QLocale::system().name();
    QStringList spl  = langName.split( '_' );
    if ( spl.length() >= 2 )
    {
      lang = spl[ 1 ].toLower();
    }
    qDebug() << "[Localization]: QLocale system language: " << lang;
  }
  QTranslator myappTranslator;
  if ( !lang.isEmpty() && !lang.startsWith( "en" ) )
  {
    for ( const auto& path : SC_PATHS::getDataPaths() )
    {
      QString path_to_locale = path + "/locale";

      QString qm_file = QString( "sc_" ) + lang;
      qDebug() << "[Localization]: Trying to load local file from: " << path_to_locale << "/" << qm_file << ".qm";
      if ( myappTranslator.load( qm_file, path_to_locale ) )
      {
        break;
      }
      qDebug() << "[Localization]: Loaded translator isEmpty(): " << myappTranslator.isEmpty();
    }
  }
  else
  {
    qDebug() << "[Localization]: No specific translator chosen, using English.";
  }
  a.installTranslator( &myappTranslator );

  // Chromium theme color
  QString color;
  color = settings.value( "options/theme_color", "Light" ).toString();
  if ( color == "Dark" )
  {
    qputenv( "QTWEBENGINE_CHROMIUM_FLAGS", "--force-dark-mode --enable-features=WebUIDarkMode" );
  }

  QString iconlocation =
      QStandardPaths::locate( QStandardPaths::AppDataLocation, QString( "icon" ), QStandardPaths::LocateDirectory );
  QDir::addSearchPath( "icon", iconlocation );

  SC_MainWindow w;

  parse_additional_args( w, a.arguments() );

  auto ret = a.exec();

  bcp_api::token_save();

  return ret;
}
