#include <QtGui/QApplication>
#include "simcraftqt.h"
#include "sc_autoupdate.h"

int main(int argc, char *argv[])
{
  thread_t::init();
  QApplication a(argc, argv);
  SimcraftWindow w;

  AutoUpdater* updater = 0;

#ifdef Q_WS_MAC

  CocoaInitializer cocoaInitializer;
  updater = new SparkleAutoUpdater("http://simcraft.rungie.com/simcraftqt/update.xml");
  QDir::home().mkpath("Library/Application Support/simcraftqt");
  QDir::setCurrent(QDir::home().absoluteFilePath("Library/Application Support/simcraftqt"));

#endif
  
  if(updater)
  {
     updater->checkForUpdates();
  }
  
  w.showMaximized();
  w.cmdLine->setFocus();
  return a.exec();
}
