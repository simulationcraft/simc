#pragma once
#include <QGroupBox>
#include <QTreeWidget>

class QDir;

class SC_SampleProfilesTab : public QGroupBox
{
    Q_OBJECT
public:
    SC_SampleProfilesTab( QWidget* parent = nullptr );
    QTreeWidget* bisTree;
private:
    void fillTree( QDir );
};
