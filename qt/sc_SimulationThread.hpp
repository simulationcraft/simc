#pragma once
#include "config.hpp"

#include "simulationcraftqt.hpp"

#include <QThread>

class SC_MainWindow;
struct sim_t;

class SC_SimulateThread : public QThread
{
  Q_OBJECT
  SC_MainWindow* mainWindow;
  std::shared_ptr<sim_t> sim;
  QByteArray utf8_options;
  QString tabName;
  QString error_category;
  QString error_str;

public:
  bool success;
  SC_SimulateThread( SC_MainWindow* );
  void start( std::shared_ptr<sim_t> s, const QByteArray& o, QString tab_name );
  void run() override;
  QByteArray getOptions() const
  {
    return utf8_options;
  }
  QString getErrorCategory() const
  {
    return error_category;
  }
  QString getError() const
  {
    return error_str;
  }
  QString getTabName() const
  {
    return tabName;
  }

private slots:
  void sim_finished()
  {
    emit simulationFinished( sim );
  }

signals:
  void simulationFinished( std::shared_ptr<sim_t> s );
};
