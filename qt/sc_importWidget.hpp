// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SC_IMPORTWIDGET_HPP
#define SC_IMPORTWIDGET_HPP

#include <QtWidgets/QtWidgets>
#include <QStandardItemModel>
#include <QMap>
#include <QVector>
#include <QFile>
#include <QRegExpValidator>
#include <QDebug>
#include <QSettings>

class BattleNetImportWidget : public QWidget
{
    Q_OBJECT

    typedef QMap<QString, QStandardItemModel*> RealmDataModel;

    // Simple horizontal layout for the widget
    QHBoxLayout*      m_layout;

    // Components
    QLabel*           m_regionLabel;
    QComboBox*        m_regionCombo;
    QLabel*           m_realmLabel;
    QComboBox*        m_realmCombo;
    QLabel*           m_characterLabel;
    QLineEdit*        m_character;
    QLabel*           m_specializationLabel;
    QComboBox*        m_specializationCombo;
    QPushButton*      m_importButton;

    // Data
    RealmDataModel    m_realmModels;
    QSettings         m_settings;

public:
    BattleNetImportWidget( QWidget* parent = nullptr );

    QLineEdit* characterWidget() const
    { return m_character; }

    // Accessors that sanitize the input to a form simc understands
    QString region() const
    { return m_regionCombo -> currentText().toLower(); }

    QString realm() const
    { return m_realmCombo -> currentData( Qt::UserRole ).toString(); }

    QString character() const
    { return m_character -> displayText(); }

    QString specialization() const
    { return m_specializationCombo -> currentData( Qt::DisplayRole ).toString().toLower(); }

    bool validateInput() const;
signals:
    void importTriggeredOut( const QString&, const QString&, const QString&, const QString& );
    void armoryRegionChangedOut( const QString& );

public slots:
    void armoryRegionChangedIn( const QString& );

private slots:
    void returnPressed();
    void buttonClicked();
    void realmIndexChanged( int );

private:
    void parseRealmListFile( QFile& file );
    void loadRealmData();
    void populateSpecialization();
    void populateRegion();

    void selectRegion( const QString& );
};

#endif /* SC_IMPORTWIDGET_HPP */
