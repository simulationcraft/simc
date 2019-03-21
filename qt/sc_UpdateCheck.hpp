// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_UPDATECHECK_HPP
#define SC_UPDATECHECK_HPP

#include <QMessageBox>
#include <QNetworkAccessManager>

class UpdateCheckWidget : public QMessageBox
{
  Q_OBJECT

  QNetworkAccessManager* m_manager;

public:
  UpdateCheckWidget( QWidget* parent );

  /// Execute the update check
  void start();

private slots:
  void replyFinished( QNetworkReply* reply );
};

#endif /* SC_UPDATECHECK_HPP */
