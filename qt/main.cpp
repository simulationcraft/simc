#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include <QLocale>
#include <QtWidgets/QApplication>
#include <locale>
#if defined SC_VS
#include <windows.h>
#include <stdio.h>
#if defined VS_WIN_NONXP_TARGET
#include <VersionHelpers.h>
#endif
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

#if defined SC_WINDOWS
bool checkWindowsVersion()
{
#if defined VS_WIN_NONXP_TARGET && defined SC_VS
  // The 32 bit version should be built with xp toolchain so that it will give an error box with an explanation, instead of a generic
  // appcrash that it would give with the normal toolchain.. which would just mean more issues being posted. 
  // After a few months we can probably remove this targetting for the GUI and only leave it in for CLI. - 01/08/2015
  if ( !IsWindows7SP1OrGreater() && IsWindows7OrGreater() ) // Winxp cannot access fancy-pants methods to determine if the OS has Win7 SP1... which is probably ok.
  {
    QMessageBox Msgbox;
    Msgbox.setText( "SimulationCraft GUI is known to have issues with Windows 7 when Service Pack 1 is not installed.\nThe program will continue to load, but if you run into any problems, please install Service Pack 1." );
    Msgbox.exec();
    return true;
  }
#endif
  if ( QSysInfo::WindowsVersion < QSysInfo::WV_VISTA )
  {
    QMessageBox Msgbox;
    Msgbox.setText( "SimulationCraft GUI is no longer supported on Windows XP as of January 2015. \nIf you wish to continue using Simulationcraft, you may do so by the command line interface -- simc.exe." );
    Msgbox.exec();
    return false; // Do not continue loading.
  }
  return true;
}
#endif

int main( int argc, char *argv[] )
{
  QLocale::setDefault( QLocale( "C" ) );
  std::locale::global( std::locale( "C" ) );
  setlocale( LC_ALL, "C" );

  dbc::init();
  module_t::init();

#if defined SC_WINDOWS
  if ( QSysInfo::WindowsVersion == QSysInfo::WV_WINDOWS7 )
    QApplication::setAttribute( Qt::AA_UseOpenGLES, true );
  // This is to fix an issue with older video cards on windows 7. It must be called before the application.
#endif

  QApplication a( argc, argv );

#if defined SC_WINDOWS
  if ( !checkWindowsVersion() ) // Check for compatability. Must be called after application.
  { return 0; }
#endif

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
  if ( lang == "auto" )
  {
    lang = QLocale::system().name().split('_').at(1).toLower();
    qDebug() << "QLocale system language: " << lang;
  }
  QTranslator myappTranslator;
  if ( !lang.isEmpty() && !lang.startsWith( "en" ) )
  {
    QString path_to_locale = SC_PATHS::getDataPath() + "/locale";

    QString qm_file = QString( "sc_" ) + lang;
    //qDebug() << "qm file: " << qm_file;
    myappTranslator.load( qm_file, path_to_locale );
    //qDebug() << "translator: " << myappTranslator.isEmpty();
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
