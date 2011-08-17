#include <QLocale>
#include <QtGui/QApplication>
#include "simulationcraftqt.h"
#ifndef SIMC_NO_AUTOUPDATE
#include "sc_autoupdate.h"
#endif /* SIMC_NO_AUTOUPDATE */

int main(int argc, char *argv[])
{
  QLocale::setDefault( QLocale( "C" ) );
  thread_t::init();
  dbc_t::init();

  QApplication a(argc, argv);
#ifndef SIMC_NO_AUTOUPDATE
  AutoUpdater* updater = 0;

#ifdef Q_WS_MAC

  CocoaInitializer cocoaInitializer;
  updater = new SparkleAutoUpdater("http://simc.rungie.com/simcqt/update.xml");

#endif

  if( updater)
  {
    updater->checkForUpdates();
  }
#endif /* SIMC_NO_AUTOUPDATE */

#ifdef Q_WS_MAC
  QDir::home().mkpath("Library/Application Support/simcqt");
  QDir::setCurrent(QDir::home().absoluteFilePath("Library/Application Support/simcqt"));
#endif

  SimulationCraftWindow w;

  if( w.historyWidth  != 0 &&
      w.historyHeight != 0 )
  {
    w.resize( w.historyWidth, w.historyHeight );
  }

  if( w.historyMaximized )
  {
    w.showMaximized();
  }
  else
  {
    w.showNormal();
  }

  w.cmdLine->setFocus();

  if( argc > 1 )
  {
    for( int i=1; i < argc; i++ )
    {
      QFile file( argv[ i ] );

      if( file.open( QIODevice::ReadOnly ) )
      {
        w.simulateText->appendPlainText( file.readAll() );
        file.close();
      }
    }
    w.mainTab->setCurrentIndex( TAB_SIMULATE );
  }

  return a.exec();
}
