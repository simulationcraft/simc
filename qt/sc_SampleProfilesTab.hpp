#pragma once
#include "config.hpp"
#include <QtWidgets/QtWidgets>
#include <QtGui/QtGui>

class QDir;
class QTreeWidget;

class SC_SampleProfilesTab : public QGroupBox
{
    Q_OBJECT
public:
    SC_SampleProfilesTab( QWidget* parent = nullptr );
    QTreeWidget* tree;
private:
    void fillTree( QDir );
};
