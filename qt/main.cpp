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

bool checkWindowsVersion()
{
#if defined SC_VS
  OSVERSIONINFO osvi;
  BOOL bIsWindowsXPorLater;
  ZeroMemory( &osvi, sizeof( OSVERSIONINFO ) );
  osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
  GetVersionEx(&osvi);
  bIsWindowsXPorLater = osvi.dwMajorVersion >= 6; // 6.0 and up is Vista
#ifndef VS_XP_TARGET 
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
  if ( !bIsWindowsXPorLater )
  {
    QMessageBox Msgbox;
    Msgbox.setText( "SimulationCraft GUI is no longer supported on Windows XP as of January 2015. \nIf you wish to continue using Simulationcraft, you may do so by the command line interface -- simc.exe." );
    Msgbox.exec();
    return false; // Do not continue loading.
  }
#endif
  return true;
}

int main( int argc, char *argv[] )
{
  QLocale::setDefault( QLocale( "C" ) );
  std::locale::global( std::locale( "C" ) );
  setlocale( LC_ALL, "C" );

  dbc::init();
  module_t::init();

#if defined ( SC_USE_WEBENGINE ) && ! defined ( VS_XP_TARGET )
  if ( !IsWindows8OrGreater() && IsWindows7OrGreater() )
    QApplication::setAttribute( Qt::AA_UseOpenGLES, true );
  // This is to fix an issue with older video cards on windows 7.
#endif
  QApplication a( argc, argv );

  if ( !checkWindowsVersion() ) // Check for compatability.
  { return 0; }

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

  QString lang( "" );
  QSettings settings;
  settings.beginGroup( "options" );
  if ( settings.contains( "gui_localization" ) )
  {
    lang = settings.value( "gui_localization" ).toString();
    qDebug() << "simc gui localization: " << lang;
  }
  settings.endGroup();
  if ( lang == "auto" )
  {
    lang = QLocale::system().name();
    //qDebug() << "using auto localization: " << lang;
  }
  QTranslator myappTranslator;
  if ( !lang.isEmpty() && !lang.startsWith( "en" ) )
  {
    QString path_to_locale( "locale" );

    QString qm_file = QString( "sc_" ) + lang;
    //qDebug() << "qm file: " << qm_file;
    myappTranslator.load( qm_file, path_to_locale );
    //qDebug() << "translator: " << myappTranslator.isEmpty();
  }
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
