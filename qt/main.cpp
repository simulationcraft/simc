#include <QtGui/QApplication>
#include "simcraftqt.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SimcraftWindow w;
    w.show();
    return a.exec();
}
