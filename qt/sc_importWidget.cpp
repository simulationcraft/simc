#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

#include "sc_importWidget.hpp"

BattleNetImportWidget::BattleNetImportWidget( QWidget* parent ) :
    QWidget( parent ),
    m_layout( new QHBoxLayout( this ) ),
    m_realmLabel( new QLabel( tr( "Realm" ), this ) ),
    m_realmCombo( new QComboBox( this ) ),
    m_characterLabel( new QLabel( tr( "Character" ), this ) ),
    m_character( new QLineEdit( this ) ),
    m_specializationLabel( new QLabel( tr( "Specialization" ) ) ),
    m_specializationCombo( new QComboBox( this ) ),
    m_importButton( new QPushButton( tr( "Import" ), this ) )
{
    setLayout( m_layout );

    m_layout -> addWidget( m_realmLabel );
    m_layout -> addWidget( m_realmCombo );
    m_layout -> addWidget( m_characterLabel );
    m_layout -> addWidget( m_character );
    m_layout -> addWidget( m_specializationLabel );
    m_layout -> addWidget( m_specializationCombo );
    m_layout -> addWidget( m_importButton );

    // Don't enable specialization for now
    m_specializationLabel -> hide();
    m_specializationCombo -> hide();

    populateSpecialization();
    loadRealmData();

    connect( m_character, SIGNAL( editingFinished() ), this, SLOT( returnPressed() ) );
    connect( m_importButton, SIGNAL( clicked() ), this, SLOT( buttonClicked() ) );
    connect( m_realmCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( realmIndexChanged( int ) ) );
}

// Populate realmlists
void BattleNetImportWidget::loadRealmData()
{
    static const QVector<QString> files = {
        "://us-realmlist.json",
        "://eu-realmlist.json",
        "://tw-realmlist.json",
        "://kr-realmlist.json"
    };

    for ( const auto& file : files )
    {
        QFile json_file( file );
        if ( json_file.open( QIODevice::ReadOnly ) )
        {
            parseRealmListFile( json_file );
        }
        else
        {
            qDebug() << "Could not open" << json_file.fileName();
        }
    }

    populateComboBox( "US" ); // Populate with default data
}

void BattleNetImportWidget::parseRealmListFile( QFile& file )
{
    QJsonParseError error;
    auto jsdoc = QJsonDocument::fromJson( file.readAll(), &error );
    if ( error.error != QJsonParseError::NoError )
    {
        qDebug() << error.errorString();
        return;
    }

    if ( jsdoc.isNull() )
    {
        qDebug() << "No readable data in" << file.fileName();
        return;
    }

    auto root = jsdoc.object();
    if ( root.isEmpty() || ! root.contains( "realms" ) )
    {
        qDebug() << "No root element or realms in" << file.fileName();
        return;
    }

    auto realms = root[ "realms" ].toArray();
    if ( realms.empty() )
    {
        qDebug() << "No realms in realm object";
        return;
    }

    QVector<std::pair<QString, QString>> realmList;
    for ( QJsonValueRef entry : realms )
    {
        auto obj = entry.toObject();
        realmList.push_back( std::make_pair( obj["name"].toString(), obj["slug"].toString() ) );
    }

    QRegExp region( "([A-Za-z]+)-realmlist.json$");
    QString region_str;

    if ( region.indexIn( file.fileName() ) >= 0 )
    {
        region_str = region.cap( 1 ).toUpper();
    }

    if ( region_str.size() > 0 )
    {
        auto model = new QStandardItemModel( this );
        //model -> insertRows( 0, realmList.size() );
        for ( size_t i = 0, end = realmList.size(); i < end; ++i )
        {
            const auto& data = realmList[ i ];
            QStandardItem* item = new QStandardItem( data.first );
            item -> setData( data.second, Qt::UserRole );
            model -> appendRow( item );
        }
        m_realmModels[ region_str ] = model;

        qDebug() << "Loaded realm information for" << region_str << "n_model_entries" << model -> rowCount();
    }
}

void BattleNetImportWidget::populateComboBox( const QString& region )
{
    if ( m_realmModels.find( region ) == m_realmModels.end() )
    {
        return;
    }
    m_currentRegion = region;
    m_realmCombo -> setModel( m_realmModels[ region ] );

    QString s = m_settings.value( "importWidget/lastUsedRealm" ).toString();
    if ( s.size() > 0 )
    {
        auto index = m_realmCombo -> findData( s, Qt::DisplayRole );
        if ( index > -1 )
        {
            m_realmCombo -> setCurrentIndex( index );
            // Also set focus to the character input box
            m_character -> setFocus();
        }
    }
}

bool BattleNetImportWidget::validateInput() const
{
    if ( character().size() < 3 )
    {
        return false;
    }

    return true;
}

void BattleNetImportWidget::realmIndexChanged( int index )
{
    m_settings.setValue( "importWidget/lastUsedRealm", m_realmCombo -> itemText( index ) );
    m_settings.sync();
}

// Note, ensure import only begins when the user presses enter, and not when focus is lost on the
// character text input
void BattleNetImportWidget::returnPressed()
{
    if ( ! m_character -> hasFocus() )
    {
        return;
    }

    if ( ! validateInput() )
    {
        return;
    }

    emit importTriggered( region(), realm(), character(), specialization() );
}

void BattleNetImportWidget::buttonClicked()
{
    if ( ! validateInput() )
    {
        return;
    }

    emit importTriggered( region(), realm(), character(), specialization() );
}

void BattleNetImportWidget::armoryRegionChanged( const QString& region )
{
    // Remove last used realm if user changes regions
    m_settings.remove( "importWidget/lastUsedRealm" );
    populateComboBox( region.toUpper() );
}

void BattleNetImportWidget::populateSpecialization()
{
    m_specializationCombo -> addItem( tr( "Active" ) );
    m_specializationCombo -> addItem( tr( "First" ) );
    m_specializationCombo -> addItem( tr( "Second" ) );
    m_specializationCombo -> addItem( tr( "Third" ) );
    m_specializationCombo -> addItem( tr( "Fourth" ) );
}
