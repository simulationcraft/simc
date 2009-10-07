#include <QtGui/QApplication>
#include "simcraftqt.h"

int main(int argc, char *argv[])
{
  thread_t::init();
  QApplication a(argc, argv);
  SimcraftWindow w;
  w.showMaximized();
  return a.exec();
}
