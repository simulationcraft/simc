#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "simulationcraft.hpp"
#include "sc_UpdateCheck.hpp"

static const QString UPDATE_CHECK_URL { "https://www.simulationcraft.org/version.json" };

UpdateCheckWidget::UpdateCheckWidget( QWidget* parent ) :
  QMessageBox( parent ),
  m_manager( new QNetworkAccessManager( this ) )
{
  connect( m_manager, SIGNAL( finished( QNetworkReply* ) ),
           this,      SLOT( replyFinished( QNetworkReply* ) ) );
}

void UpdateCheckWidget::start()
{
  m_manager->get( QNetworkRequest( QUrl( UPDATE_CHECK_URL ) ) );
}

void UpdateCheckWidget::replyFinished( QNetworkReply* reply )
{
  reply->deleteLater();

  if ( reply->error() != QNetworkReply::NoError )
  {
    qDebug() << "HTTP server returned error, QNetworkError " << reply->error();
    return;
  }

  auto data = reply->readAll();
  QJsonParseError error;
  auto document = QJsonDocument::fromJson( data, &error );
  if ( document.isNull() )
  {
    qDebug() << "HTTP server did not return JSON (" << error.errorString() << ")";
    qDebug() << "Reply contents: " << data;
    return;
  }

  // Version information comes in an array of objects, with the first element of the array being the
  // latest release
  if ( !document.isArray() )
  {
    qDebug() << "Version document is malformed (does not start with an array)";
    return;
  }

  auto first = document.array().at( 0 );
  // No entries in the array, or entry is not an object
  if ( first.isUndefined() || !first.isObject() )
  {
    qDebug() << "Version document contains no data, or does not contain a version object";
    return;
  }

  auto latest = first.toObject();

  // Ensure entry has "version", "release", and "url" keys
  if ( latest[ "version" ].isUndefined() || !latest[ "version" ].isString() )
  {
    qDebug() << "Malformed information, no Simulationcraft release version found";
    return;
  }

  if ( latest[ "release" ].isUndefined() || !latest[ "release" ].isString() )
  {
    qDebug() << "Malformed information, no Simulationcraft release minor version found";
    return;
  }

  if ( latest[ "url" ].isUndefined() || !latest[ "url" ].isString() )
  {
    qDebug() << "Malformed information, no release URL found";
    return;
  }

  // Do lexicographical comparison to SC_MAJOR_VERSION and SC_MINOR_VERSION, should work OK in
  // practically all situations, even if converting to base10 int would be better, potentially
  auto version = latest[ "version" ].toString();
  auto release = latest[ "release" ].toString();
  auto url = latest[ "url" ].toString();

  if ( SC_MAJOR_VERSION < version || SC_MINOR_VERSION < release )
  {
    setText( tr( "A new version of Simulationcraft has been released" ) );
    QString info = "<p>" + tr( "Your current version is" ) + " <strong>" + SC_VERSION + "</strong></p>";
    info        += "<p>" + tr( "The new version is" ) + " <strong>" + version + "-" + release + "</strong></p>";
    info        += "<p>" + tr( "You can download the new release from " ) + "<a href=\"" + url + "\">" + url + "</a>.</p>";
    if ( !latest[ "notes" ].isUndefined() && latest[ "notes" ].isString() )
    {
      info += "<h3>" + tr( "Release notes" ) + "</h3>";
      info += "<p>" + latest[ "notes" ].toString() + "</p>";
    }
    setInformativeText( info );
    exec();
  }
}
