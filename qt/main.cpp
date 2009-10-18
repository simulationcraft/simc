#include <QtGui/QApplication>
#include "simucraftqt.h"
#include "sc_autoupdate.h"

int main(int argc, char *argv[])
{
  thread_t::init();
  QApplication a(argc, argv);
  SimucraftWindow w;

  AutoUpdater* updater = 0;

#ifdef Q_WS_MAC

  CocoaInitializer cocoaInitializer;
  updater = new SparkleAutoUpdater("http://simucraft.rungie.com/simucraftqt/update.xml");
  QDir::home().mkpath("Library/Application Support/simucraftqt");
  QDir::setCurrent(QDir::home().absoluteFilePath("Library/Application Support/simucraftqt"));

#endif
  
  if(updater)
  {
     updater->checkForUpdates();
  }
  
  w.showMaximized();
  w.cmdLine->setFocus();
  return a.exec();
}
