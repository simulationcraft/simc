#include <QtGui/QApplication>
#include "simulationcraftqt.h"
#include "sc_autoupdate.h"

int main(int argc, char *argv[])
{
  thread_t::init();
  QApplication a(argc, argv);
  SimulationCraftWindow w;

  AutoUpdater* updater = 0;

#ifdef Q_WS_MAC

  CocoaInitializer cocoaInitializer;
  updater = new SparkleAutoUpdater("http://simc.rungie.com/simcqt/update.xml");
  QDir::home().mkpath("Library/Application Support/simcqt");
  QDir::setCurrent(QDir::home().absoluteFilePath("Library/Application Support/simcqt"));

#endif
  
  if(updater)
  {
     updater->checkForUpdates();
  }
  
  w.showMaximized();
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
