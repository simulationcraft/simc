// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include <QDebug>
#include <QDir>
#include <QPixmapCache>

#include <algorithm>

#include "simcpaperdoll.h"

QPixmap
PaperdollPixmap::get( const QString& icon_str, bool border, QSize size )
{
  QPixmap icon;
  
  if ( ! QPixmapCache::find( icon_str, &icon ) )
  {
    icon = PaperdollPixmap( icon_str, border, size );
    QPixmapCache::insert( icon_str, icon );
  }
  
  return icon;
}

PaperdollPixmap::PaperdollPixmap() : QPixmap() { }

PaperdollPixmap::PaperdollPixmap( const QString& icon, bool draw_border, QSize s ) :
  QPixmap( s )
{
  QImage icon_image( QString( QDir::homePath() + "/WoWAssets/fixed/" + icon ), 0 );
  QImage border( QString( QDir::homePath() + "/WoWAssets/border" ) );

  if ( icon_image.isNull() )
    icon_image = QImage( QString( QDir::homePath() + "/WoWAssets/fixed/ABILITY_SEAL" ), 0 );
  
  QSize d = ( draw_border ? border.size() : size() ) - icon_image.size();
  d /= 2;
  
  assert( ! draw_border || ( draw_border && border.size() == size() ) );
  
  fill( QColor( 0, 0, 0, 0 ) );

  QPainter paint( this );
  paint.drawImage( d.width(), d.height(), icon_image );
  if ( draw_border ) paint.drawImage( 0, 0, border );
}

PaperdollProfile::PaperdollProfile() : QObject(), m_currentSlot( SLOT_NONE )
{ 
  for ( int i = 0; i < SLOT_MAX; i++ )
    m_slotItem[ i ] = 0;
}

void
PaperdollProfile::setSelectedSlot( slot_type t )
{
  m_currentSlot = t;
  emit slotChanged( m_currentSlot );
}

void
PaperdollProfile::setSelectedItem( const QModelIndex& index )
{
  const item_data_t* item = reinterpret_cast< const item_data_t* >( index.data( Qt::UserRole ).value< void* >() );

  m_slotItem[ m_currentSlot ] = item;
  
  emit itemChanged( m_currentSlot, m_slotItem[ m_currentSlot ] );
}


ItemFilterProxyModel::ItemFilterProxyModel( PaperdollProfile* profile, QObject* parent ) :
  QSortFilterProxyModel( parent ), 
  m_profile( profile ), 
  m_matchArmor( true ), m_minIlevel( 359 ), m_maxIlevel( 410 ),
  m_class( 0 ), m_race( 0 ), m_searchText()
{
  setDynamicSortFilter( true );
}

void 
ItemFilterProxyModel::setMinIlevel( int newValue )
{
  m_minIlevel = newValue;
  invalidate();
  sort( 0 );
}

void 
ItemFilterProxyModel::setMaxIlevel( int newValue )
{
  m_maxIlevel = newValue;
  invalidate();
  sort( 0 );
}

void 
ItemFilterProxyModel::SetClass( quint32 newValue )
{
  m_class = newValue;
  invalidate();
  sort( 0 );
}

void 
ItemFilterProxyModel::SetRace( quint32 newValue )
{
  m_race = newValue;
  invalidate();
  sort( 0 );
}

void 
ItemFilterProxyModel::setSlot( slot_type slot )
{
  invalidate();
  sort( 0 );
  
  if ( m_profile -> slotItem( slot ) )
  {
    for ( int i = 0; i < rowCount(); i++ )
    {
      const QModelIndex& idx = index( i, 0 );
      const item_data_t* item = reinterpret_cast< const item_data_t* >( idx.data( Qt::UserRole ).value< void* >() );
      if ( item == m_profile -> slotItem( slot ) )
      {
        emit itemSelected( idx );
        break;
      }
    }
  }
}

void 
ItemFilterProxyModel::SearchTextChanged( const QString& newValue )
{
  m_searchText = newValue;
  invalidate();
  sort( 0 );
}

void
ItemFilterProxyModel::SetMatchArmor( int newValue )
{
  m_matchArmor = newValue;
  invalidate();
  sort( 0 );
}

bool
ItemFilterProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
  const item_data_t* ileft = reinterpret_cast< const item_data_t* >( left.data( Qt::UserRole ).value< void* >() ),
                   *iright = reinterpret_cast< const item_data_t* >( right.data( Qt::UserRole ).value< void* >() );

  if ( ileft -> level > iright -> level )
    return true;
  else if ( ileft -> level == iright -> level )
    return QString::compare( ileft -> name, iright -> name ) < 0;
  
  return false;
}

bool 
ItemFilterProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
  // Grab the pointer from userdata, and filter through it
  const QModelIndex& idx  = sourceModel() -> index( sourceRow, 0, sourceParent );
  const item_data_t* item = reinterpret_cast< const item_data_t* >( idx.data( Qt::UserRole ).value< void* >() );
  
  if ( m_profile -> currentSlot() == SLOT_NONE )
    return false;
  
  bool state = ( filterByName( item ) && itemFitsSlot( item ) && itemFitsClass( item ) &&
                itemUsedByClass( item ) && 
                item -> level >= m_minIlevel && item -> level <= m_maxIlevel &&
                ( ! m_class || ( item -> class_mask & util_t::class_id_mask( m_class ) ) ) &&
                ( ! m_race  || ( item -> race_mask &  util_t::race_mask( m_race ) ) ) );
  
  return state;
}

bool
ItemFilterProxyModel::filterByName( const item_data_t* item ) const
{
  if ( m_searchText.length() > 2 )
  {
    if ( QString( item -> name ).startsWith( m_searchText, Qt::CaseInsensitive ) )
      return true;
    else
      return false;
  }
  
  return true;
}
          
bool
ItemFilterProxyModel::itemFitsSlot( const item_data_t* item, bool dual_wield ) const
{
  slot_type slot = m_profile -> currentSlot();
  switch ( item -> inventory_type )
  {
    case INVTYPE_HEAD:           return slot == SLOT_HEAD;
    case INVTYPE_NECK:           return slot == SLOT_NECK;
    case INVTYPE_SHOULDERS:      return slot == SLOT_SHOULDERS;
    case INVTYPE_BODY:           return slot == SLOT_SHIRT;
    case INVTYPE_CHEST:          return slot == SLOT_CHEST;
    case INVTYPE_WAIST:          return slot == SLOT_WAIST;
    case INVTYPE_LEGS:           return slot == SLOT_LEGS;
    case INVTYPE_FEET:           return slot == SLOT_FEET;
    case INVTYPE_WRISTS:         return slot == SLOT_WRISTS;
    case INVTYPE_HANDS:          return slot == SLOT_HANDS;
    case INVTYPE_FINGER:         return slot == SLOT_FINGER_1 || slot == SLOT_FINGER_2;
    case INVTYPE_TRINKET:        return slot == SLOT_TRINKET_1 || slot == SLOT_TRINKET_2;
    case INVTYPE_WEAPON:         return slot == SLOT_MAIN_HAND || ( dual_wield && slot == SLOT_OFF_HAND );
    case INVTYPE_SHIELD:         return slot == SLOT_OFF_HAND;
    case INVTYPE_RANGED:         return slot == SLOT_RANGED;
    case INVTYPE_CLOAK:          return slot == SLOT_BACK;
    case INVTYPE_2HWEAPON:       return slot == SLOT_MAIN_HAND;
    case INVTYPE_TABARD:         return slot == SLOT_TABARD;
    case INVTYPE_ROBE:           return slot == SLOT_CHEST;
    case INVTYPE_WEAPONMAINHAND: return slot == SLOT_MAIN_HAND;
    case INVTYPE_WEAPONOFFHAND:  return slot == SLOT_OFF_HAND;
    case INVTYPE_HOLDABLE:       return slot == SLOT_OFF_HAND;
    case INVTYPE_THROWN:         return slot == SLOT_RANGED;
    case INVTYPE_RELIC:          return slot == SLOT_RANGED;
  }
  
  return false;
}

bool
ItemFilterProxyModel::itemFitsClass( const item_data_t* item ) const
{
  if ( item -> item_class == ITEM_CLASS_WEAPON )
  {
    switch ( item -> item_subclass )
    {
      case ITEM_SUBCLASS_WEAPON_AXE:
        return m_class == DEATH_KNIGHT || m_class == HUNTER  ||
               m_class == PALADIN      || m_class == ROGUE   ||
               m_class == SHAMAN       || m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_AXE2:
        return m_class == DEATH_KNIGHT || m_class == HUNTER  ||
               m_class == PALADIN      || m_class == SHAMAN  || 
               m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_BOW:
      case ITEM_SUBCLASS_WEAPON_GUN:
      case ITEM_SUBCLASS_WEAPON_THROWN:
      case ITEM_SUBCLASS_WEAPON_CROSSBOW:
        return m_class == HUNTER       || m_class == ROGUE   ||
               m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_MACE:
        return m_class == DEATH_KNIGHT || m_class == DRUID   || 
               m_class == PALADIN      || m_class == PRIEST  || 
               m_class == ROGUE        || m_class == SHAMAN  || 
               m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_MACE2:
        return m_class == DEATH_KNIGHT || m_class == DRUID   || 
               m_class == PALADIN      || m_class == SHAMAN  || 
               m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_POLEARM:
        return m_class == DEATH_KNIGHT || m_class == DRUID   || 
               m_class == HUNTER       || m_class == PALADIN || 
               m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_SWORD:
        return m_class == DEATH_KNIGHT || m_class == HUNTER  ||
               m_class == MAGE         || m_class == PALADIN || 
               m_class == ROGUE        || m_class == WARLOCK || 
               m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_SWORD2:
        return m_class == DEATH_KNIGHT || m_class == HUNTER  ||
               m_class == PALADIN      || m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_STAFF:
        return m_class == DRUID        || m_class == HUNTER  ||
               m_class == MAGE         || m_class == PRIEST  ||
               m_class == SHAMAN       || m_class == WARLOCK ||
               m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_FIST:
        return m_class == DRUID        || m_class == HUNTER  ||
               m_class == ROGUE        || m_class == SHAMAN  || 
               m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_DAGGER:
        return m_class == DRUID        || m_class == HUNTER  ||
               m_class == MAGE         || m_class == PRIEST  || 
               m_class == ROGUE        || m_class == SHAMAN  ||
               m_class == WARLOCK      || m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_WAND:
        return m_class == MAGE         || m_class == PRIEST  ||
               m_class == WARLOCK;
        
    }
  }
  else if ( item -> item_class == ITEM_CLASS_ARMOR )
  {
    switch ( item -> item_subclass )
    {
      case ITEM_SUBCLASS_ARMOR_MISC:
        return true;
      case ITEM_SUBCLASS_ARMOR_CLOTH: // Cloaks are all cloth
        if ( ! m_matchArmor || item -> inventory_type == INVTYPE_CLOAK )
          return true;
        else
          return m_class == MAGE         || m_class == PRIEST  ||
                 m_class == WARLOCK;
      case ITEM_SUBCLASS_ARMOR_LEATHER:
        if ( ! m_matchArmor )
          return m_class == DEATH_KNIGHT || m_class == DRUID   ||
                 m_class == HUNTER       || m_class == PALADIN ||
                 m_class == ROGUE        || m_class == SHAMAN  ||
                 m_class == WARRIOR;
        else
          return m_class == DRUID        || m_class == ROGUE;
      case ITEM_SUBCLASS_ARMOR_MAIL:
        if ( ! m_matchArmor )
          return m_class == DEATH_KNIGHT || m_class == HUNTER  || 
                 m_class == PALADIN      || m_class == SHAMAN  ||
                 m_class == WARRIOR;
        else
          return m_class == HUNTER       || m_class == SHAMAN;
      case ITEM_SUBCLASS_ARMOR_PLATE:
          return m_class == DEATH_KNIGHT || m_class == PALADIN ||
                 m_class == WARRIOR;
      case ITEM_SUBCLASS_ARMOR_SHIELD:
        return m_class == PALADIN      || m_class == SHAMAN  ||
               m_class == WARRIOR;
      case ITEM_SUBCLASS_ARMOR_LIBRAM:
        return m_class == PALADIN;
      case ITEM_SUBCLASS_ARMOR_IDOL:
        return m_class == DRUID;
      case ITEM_SUBCLASS_ARMOR_TOTEM:
        return m_class == SHAMAN;
      case ITEM_SUBCLASS_ARMOR_SIGIL:
        return m_class == DEATH_KNIGHT || m_class == SHAMAN || m_class == PALADIN || m_class == DRUID;
    }
  }

  return false;
}

int
ItemFilterProxyModel::primaryStat( void ) const
{
  switch ( m_class )
  {
    case WARRIOR:
    case DEATH_KNIGHT:
      return ITEM_MOD_STRENGTH;
    case PRIEST:
    case MAGE:
    case WARLOCK:
      return ITEM_MOD_INTELLECT;
    case ROGUE:
    case HUNTER:
      return ITEM_MOD_AGILITY;
  }
  
  return -0xFF;
}

bool 
ItemFilterProxyModel::itemUsedByClass( const item_data_t* item ) const
{
  bool primary_stat_found = ( m_class == DRUID || m_class == SHAMAN || m_class == PALADIN );

  if ( item -> inventory_type == INVTYPE_TRINKET ) return true;
  
  for ( int i = 0; i < 10; i++ )
  {
    if ( item -> stat_type[ i ] == primaryStat() )
    {
      primary_stat_found = true;
    }
  }
  
  return primary_stat_found;
}

ItemDataListModel::ItemDataListModel( QObject* parent ) :
  QAbstractListModel( parent )
{
}

int
ItemDataListModel::rowCount( const QModelIndex& parent ) const
{
  return dbc_t::n_items( false );
}

QVariant 
ItemDataListModel::data( const QModelIndex& index, int role ) const
{
  const item_data_t* items = dbc_t::items( false );
  
  if ( index.row() > dbc_t::n_items( false ) - 1 )
    return QVariant( QVariant::Invalid );

  // Make an icon here ..
  if ( role == Qt::DecorationRole )
  {
    return QVariant::fromValue< QPixmap >( PaperdollPixmap::get( items[ index.row() ].icon, true ).scaled( 48, 48 ) );
  }
  else if ( role == Qt::UserRole )
    return QVariant::fromValue< void* >( (void*) &( items[ index.row() ] ) );
  
  return QVariant( QVariant::Invalid );
}

void
ItemDataListModel::dataSourceChanged( int newChoice )
{
  reset();
}

ItemDataDelegate::ItemDataDelegate( QObject* parent ) :
  QStyledItemDelegate( parent )
{
}

QSize
ItemDataDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  return QSize( 350, 50 );
}

void 
ItemDataDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  const item_data_t* item = reinterpret_cast< const item_data_t* >( index.data( Qt::UserRole ).value< void* >() );
  QPixmap icon = index.data( Qt::DecorationRole ).value< QPixmap >();
  QFont name_font = painter -> font();
  QFont ilevel_font = painter -> font();
  QFont heroic_font = painter -> font();
  QFont stats_font = painter -> font();
  QRect draw_area = option.rect.adjusted( 1, 1, -1, -1 );
  QFontMetrics qfm = QFontMetrics( name_font );

  name_font.setBold( true );
  
  ilevel_font.setPointSize( 9 );

  heroic_font.setPointSize( 9 );
  heroic_font.setItalic( true );
  
  QFontMetrics qfm2 = QFontMetrics( ilevel_font );
  
  stats_font.setPointSize( 10 );

  painter -> save();
  
  if (option.state & QStyle::State_Selected)
    painter -> fillRect( option.rect, option.palette.highlight() );
  else
  {
    if ( index.row() % 2 == 0 )
      painter -> fillRect( option.rect, QColor( Qt::black ) );
    else
      painter -> fillRect( option.rect, QColor( 0x3A, 0x3A, 0x3A ) );
  }

  painter -> drawPixmap( draw_area.x(), draw_area.y(), icon.width(), icon.height(), icon );
  bool has_sockets = false;
  
  QPixmap socket_icon;
  
  for ( int i = 0; i < 3; i++ )
  {
    if ( item -> socket_color[ i ] > 0 )
    {
      socket_icon = PaperdollPixmap::get( QString( "socket_%1" ).arg( item -> socket_color[ i ] ), false, QSize( 16, 16 ) );
      if ( ! socket_icon.isNull() )
        painter -> drawPixmap( draw_area.x() + icon.width() + 1,
                               draw_area.y() + ( i * ( socket_icon.height() + 1 ) ),
                               socket_icon.width(),
                               socket_icon.height(),
                               socket_icon );
    }
    
    has_sockets = true;
  }
  
  QRect name_area = draw_area;
  name_area.setX( name_area.x() + icon.width() + ( has_sockets ? socket_icon.width() : 0 ) + 3 );
  
  QRect stats_area = name_area;
  stats_area.setY( name_area.y() + qfm.height() );
  
  QRect ilevel_area = QRect( draw_area.x() + draw_area.width() - 20, 
                             draw_area.y(),
                             20,
                             qfm2.height() ) ;
  
  QRect heroic_area = QRect( ilevel_area.x() - 50,
                             ilevel_area.y(),
                             50,
                             qfm2.height() );
  
  QRect set_area = QRect( draw_area.x() + draw_area.width() - 50,
                          draw_area.y() + draw_area.height() - qfm2.height(),
                          50,
                          qfm2.height() );
  
  painter -> setFont( name_font );
  painter -> setRenderHint(QPainter::Antialiasing, true);
  painter -> setPen( Qt::SolidLine );
  switch ( item -> quality )
  {
    case 0:
      painter -> setPen( QColor( 0x9d, 0x9d, 0x9d ) );
      break;
    case 1:
      painter -> setPen( QColor( 0xff, 0xff, 0xff ) );
      break;
    case 2:
      painter -> setPen( QColor( 0x1e, 0xff, 0x00 ) );
      break;
    case 3: 
      painter -> setPen( QColor( 0x00, 0x70, 0xff ) );
      break;
    case 4:
      painter -> setPen( QColor( 0xb0, 0x48, 0xf8 ) );
      break;
    case 5:
      painter -> setPen( QColor( 0xff, 0x80, 0x80 ) );
      break;
  }
  painter -> drawText( name_area, item -> name );
  painter -> setPen( Qt::white );
  painter -> setFont( ilevel_font );
  painter -> drawText( ilevel_area, Qt::AlignRight, QString( "%1" ).arg( item -> level ) );
  painter -> setFont( heroic_font );
  painter -> setPen( QColor( 0x1e, 0xff, 0x00 ) );
  painter -> drawText( heroic_area, Qt::AlignRight, QString( "%1" ).arg( item -> flags_1 & ITEM_FLAG_HEROIC ? "Heroic" : "" ) );
  if ( item -> id_set > 0 && util_t::set_item_type_string( item -> id_set ) != 0 )
  {
    painter -> setPen( Qt::yellow );
    painter -> setFont( ilevel_font );
    painter -> drawText( set_area, Qt::AlignRight, QString( "%1 Set" ).arg( util_t::set_item_type_string( item -> id_set ) ) );
  }
  painter -> setFont( stats_font );
  painter -> setPen( Qt::white );
  painter -> drawText( stats_area, Qt::AlignLeft | Qt::TextWordWrap, itemStatsString( item ) );
  
  painter -> restore();
}

QString
ItemDataDelegate::itemStatsString( const item_data_t* item ) const
{
  assert( item != 0 );
  QStringList stats;
  QStringList weapon_stats;
  QString stats_str;
  uint32_t armor;
  dbc_t dbc( false );

  if ( item -> item_class == ITEM_CLASS_WEAPON )
  {
    weapon_stats += QString( "%1%2" )
      .arg( util_t::weapon_class_string( item -> inventory_type ) ? QString( util_t::weapon_class_string( item -> inventory_type ) ) + QString( " " ) : "" )
      .arg( util_t::weapon_subclass_string( item -> item_subclass ) );
    weapon_stats += QString( "%1 - %2 (%3)" )
      .arg( item_database_t::weapon_dmg_min( item, dbc ) )
      .arg( item_database_t::weapon_dmg_max( item, dbc ) )
      .arg( dbc.weapon_dps( item -> id ), 0, 'f', 1 );
    weapon_stats += QString( "%1 Speed" ).arg( item -> delay / 1000.0, 0, 'f', 2 );
  }
  
  if ( ( armor = item_database_t::armor_value( item, dbc ) ) > 0 )
    stats += QString( "%1 Armor" ).arg( armor );
  
  for ( int i = 0; i < 10; i++ )
  {
    if ( item -> stat_type[ i ] < 0 || item -> stat_val[ i ] == 0 )
      continue;
    
    stat_type t = util_t::translate_item_mod( item -> stat_type[ i ] );
    if ( t == STAT_NONE ) continue;
    
    stats << QString( "%1 %2" )
      .arg( item -> stat_val[ i ] )
      .arg( util_t::stat_type_abbrev( t ) );
  }
  
  if ( weapon_stats.size() > 0 )
  {
    stats_str += weapon_stats.join( ", " );
    stats_str += "\n";
  }
  
  if ( stats.size() > 0 )
    stats_str += stats.join( ", " );
  
  return stats_str;
}

ItemSelectionWidget::ItemSelectionWidget( PaperdollProfile* profile, QWidget* parent ) :
  QTabWidget( parent ), m_profile( profile )
{
  m_itemSearch                = new QWidget( this );
  m_itemSearchLayout          = new QVBoxLayout();
  
  m_itemSearchInput           = new QLineEdit( m_itemSearch );
  m_itemSearchView            = new QListView( m_itemSearch );
  
  m_itemSearchModel           = new ItemDataListModel();
  m_itemSearchProxy           = new ItemFilterProxyModel( profile );
  m_itemSearchDelegate        = new ItemDataDelegate();
  
  m_itemFilter                = new QWidget( this );
  m_itemFilterLayout          = new QFormLayout();
  
  m_itemFilterMatchArmor      = new QCheckBox( m_itemFilter );
  
  m_itemFilterMinIlevelLayout = new QHBoxLayout();
  m_itemFilterMinIlevel       = new QSlider( Qt::Horizontal, m_itemFilter );
  m_itemFilterMinIlevelLabel  = new QLabel();
  
  m_itemFilterMaxIlevelLayout = new QHBoxLayout();
  m_itemFilterMaxIlevel       = new QSlider( Qt::Horizontal, m_itemFilter );
  m_itemFilterMaxIlevelLabel  = new QLabel();

  m_itemSetup                 = new QWidget( this );
  m_itemSetupLayout           = new QVBoxLayout();
  
  m_itemSetupEnchantBox       = new QGroupBox( m_itemSetup );
  m_itemSetupEnchantBoxLayout = new QHBoxLayout();
  m_itemSetupEnchantView      = new QComboBox( m_itemSetup );
  m_itemSetupEnchantModel     = new EnchantDataModel( profile );
  
  // Search widget setup
  
  m_itemSearchView -> setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  m_itemSearchView -> setModel( m_itemSearchProxy );
  m_itemSearchView -> setItemDelegate( m_itemSearchDelegate );

  m_itemSearchProxy -> setSourceModel( m_itemSearchModel );
  m_itemSearchProxy -> SetClass( SHAMAN );
  
  QObject::connect( m_itemSearchInput, SIGNAL( textChanged( const QString& ) ),
                    m_itemSearchProxy, SLOT( SearchTextChanged( const QString& ) ) );
  
  QObject::connect( m_itemSearchProxy, SIGNAL( itemSelected( const QModelIndex& ) ),
                    m_itemSearchView,  SLOT( setCurrentIndex( const QModelIndex& ) ) );

  m_itemSearch -> setLayout( m_itemSearchLayout );
  
  m_itemSearchLayout -> addWidget( m_itemSearchInput );
  m_itemSearchLayout -> addWidget( m_itemSearchView );
  
  // Filter widget setup
  
  m_itemFilterMatchArmor -> setCheckState( Qt::Checked );
  
  m_itemFilterMinIlevel -> setTickPosition( QSlider::TicksBelow );
  m_itemFilterMinIlevel -> setRange( 272, 410 );
  m_itemFilterMinIlevel -> setSingleStep( 1 );
  m_itemFilterMinIlevel -> setPageStep( 10 );
  m_itemFilterMinIlevel -> setTracking( false );
  m_itemFilterMinIlevel -> setValue ( m_itemSearchProxy -> minIlevel() );
  
  m_itemFilterMinIlevelLabel -> setNum( m_itemSearchProxy -> minIlevel() );
  
  m_itemFilterMinIlevelLayout -> addWidget( m_itemFilterMinIlevel, 0, Qt::AlignLeft | Qt::AlignCenter );
  m_itemFilterMinIlevelLayout -> addWidget( m_itemFilterMinIlevelLabel, 0, Qt::AlignLeft | Qt::AlignCenter );
  
  m_itemFilterMaxIlevel -> setTickPosition( QSlider::TicksBelow );
  m_itemFilterMaxIlevel -> setRange( 272, 410 );
  m_itemFilterMaxIlevel -> setSingleStep( 1 );
  m_itemFilterMaxIlevel -> setPageStep( 10 );
  m_itemFilterMaxIlevel -> setTracking( false );
  m_itemFilterMaxIlevel -> setValue( m_itemSearchProxy -> maxIlevel() );
  
  m_itemFilterMaxIlevelLabel -> setNum( m_itemSearchProxy -> maxIlevel() );
  
  m_itemFilterMaxIlevelLayout -> addWidget( m_itemFilterMaxIlevel, 0, Qt::AlignLeft | Qt::AlignCenter );
  m_itemFilterMaxIlevelLayout -> addWidget( m_itemFilterMaxIlevelLabel, 0, Qt::AlignLeft | Qt::AlignCenter );

  m_itemFilterLayout -> setLabelAlignment( Qt::AlignLeft | Qt::AlignCenter );
  m_itemFilterLayout -> addRow( "Match Armor:", m_itemFilterMatchArmor );
  m_itemFilterLayout -> addRow( "Minimum item level: ", m_itemFilterMinIlevelLayout );
  m_itemFilterLayout -> addRow( "Maximum item level: ", m_itemFilterMaxIlevelLayout );
  
  QObject::connect( m_itemFilterMinIlevel,      SIGNAL( sliderMoved( int ) ),
                    m_itemFilterMinIlevelLabel, SLOT( setNum( int ) ) );
  QObject::connect( m_itemFilterMaxIlevel,      SIGNAL( sliderMoved( int ) ),
                    m_itemFilterMaxIlevelLabel, SLOT( setNum( int ) ) );
  QObject::connect( m_itemFilterMinIlevel,      SIGNAL( valueChanged( int ) ),
                    m_itemSearchProxy,          SLOT( setMinIlevel( int ) ) );
  QObject::connect( m_itemFilterMaxIlevel,      SIGNAL( valueChanged( int ) ),
                    m_itemSearchProxy,          SLOT( setMaxIlevel( int ) ) );
  QObject::connect( m_itemFilterMatchArmor,     SIGNAL( stateChanged( int ) ),
                    m_itemSearchProxy,          SLOT( SetMatchArmor( int ) ) );

  m_itemFilter -> setLayout( m_itemFilterLayout );
  
  // Item setup widget
  
  m_itemSetup -> setLayout( m_itemSetupLayout );
  
  m_itemSetupEnchantBox -> setTitle( "Enchant" );
  m_itemSetupEnchantBox -> setLayout( m_itemSetupEnchantBoxLayout );
  
  m_itemSetupEnchantBoxLayout -> addWidget( m_itemSetupEnchantView );
  m_itemSetupEnchantBoxLayout -> setAlignment( Qt::AlignCenter | Qt::AlignTop );
  
  m_itemSetupEnchantView -> setModel( m_itemSetupEnchantModel );
  
  m_itemSetupLayout -> addWidget( m_itemSetupEnchantBox );

  // Add to main tab
  
  addTab( m_itemSearch, "Search" );
  addTab( m_itemFilter, "Filter" );
  addTab( m_itemSetup, "Setup" );
  
  setMaximumWidth( 400 );
  
  QObject::connect( m_itemSearchView,  SIGNAL( clicked( const QModelIndex& ) ),
                    profile,           SLOT( setSelectedItem( const QModelIndex& ) ) );
  
  QObject::connect( profile,           SIGNAL( slotChanged( slot_type ) ),
                    m_itemSearchProxy, SLOT( setSlot( slot_type ) ) );  
}

QSize
ItemSelectionWidget::sizeHint() const
{
  return QSize( 400, 250 );
}

// All enchants are cached and split by slot as we do not want to go through the 
// whole item and spell databases every time the the model gets stale.
EnchantDataModel::EnchantDataModel( PaperdollProfile* profile, QObject* parent ) :
  QAbstractListModel( parent ), m_profile( profile )
{
  dbc_t dbc( false );

  const item_data_t* item = dbc_t::items( dbc.ptr );

  while ( item -> id != 0 )
  {
    for ( int i = 0; i < 5; i++ )
    {
      if ( item -> id_spell[ i ] > 0 )
      {
        const spell_data_t* spell = dbc.spell( item -> id_spell[ i ] );
        
        // Item based permanent enchant, presume enchant in effect 1 always for now
        if ( spell -> effect1().type() == E_ENCHANT_ITEM )
        {
          EnchantData ed;
          ed.item_enchant = item;
          ed.enchant = spell;
          
          if ( std::find( m_enchants.begin(), m_enchants.end(), ed ) == m_enchants.end() )
            m_enchants.push_back( ed );

          break;
        }
      }
    }
    
    item++;
  }

  for ( spell_data_t* s = spell_data_t::list( dbc.ptr ); s -> id(); s++ )
  {
    if ( s -> effect1().type() == E_ENCHANT_ITEM )
    {
      EnchantData ed;
      ed.item_enchant = 0;
      ed.enchant = s;
      
      if ( std::find( m_enchants.begin(), m_enchants.end(), ed ) == m_enchants.end() )
        m_enchants.push_back( ed );
    }
  }
}

int
EnchantDataModel::rowCount( const QModelIndex& ) const
{
  return m_enchants.size();
}

QVariant
EnchantDataModel::data( const QModelIndex& index, int role ) const
{
  if ( ! index.isValid() ) return QVariant( QVariant::Invalid );
  
  if ( role == Qt::DisplayRole )
  {
    if ( m_enchants[ index.row() ].item_enchant )
      return QVariant( m_enchants[ index.row() ].item_enchant -> name );
    else
      return QVariant( m_enchants[ index.row() ].enchant -> name_cstr() );
  }
  else if ( role == Qt::UserRole )
    return QVariant::fromValue< EnchantData >( m_enchants[ index.row() ] );
  else if ( role == Qt::DecorationRole )
  {
    const EnchantData& data =  m_enchants[ index.row() ];
    if ( data.item_enchant )
      return QVariant( PaperdollPixmap::get( data.item_enchant -> icon, true ).scaled( 16, 16 ) );
    
    return QVariant( QVariant::Invalid );
  }
  
  return QVariant( QVariant::Invalid );
}

QString
PaperdollSlotButton::getSlotIconName( slot_type slot_id )
{
  QString slot;
  
  switch ( slot_id )
  {
    case SLOT_HEAD:      slot = "-Head"; break;
    case SLOT_NECK:      slot = "-Neck"; break;
    case SLOT_SHOULDERS: slot = "-Shoulder"; break;
    case SLOT_SHIRT:     slot = "-Shirt"; break;
    case SLOT_CHEST:     slot = "-Chest"; break;
    case SLOT_WAIST:     slot = "-Waist"; break;
    case SLOT_LEGS:      slot = "-Legs"; break;
    case SLOT_FEET:      slot = "-Feet"; break;
    case SLOT_WRISTS:    slot = "-Wrists"; break;
    case SLOT_HANDS:     slot = "-Hands"; break;
    case SLOT_FINGER_1:  slot = "-Finger"; break;
    case SLOT_FINGER_2:  slot = "-Finger"; break;
    case SLOT_TRINKET_1: slot = "-Trinket"; break;
    case SLOT_TRINKET_2: slot = "-Trinket"; break;
    case SLOT_TABARD:    slot = "-Tabard"; break;
    case SLOT_BACK:      slot = "-Chest"; break;
    case SLOT_MAIN_HAND: slot = "-MainHand"; break;
    case SLOT_OFF_HAND:  slot = "-SecondaryHand"; break;
    case SLOT_RANGED:    slot = "-Ranged"; break;
    default: slot = "Background"; break;
  }

  return QString( "UI-PaperDoll-Slot%1" ).arg( slot );
}

PaperdollSlotButton::PaperdollSlotButton( slot_type slot, PaperdollProfile* profile, QWidget* parent ) :
  QAbstractButton( parent ), m_slot( slot ), m_profile( profile )
{
  m_slotIcon = PaperdollPixmap::get( PaperdollSlotButton::getSlotIconName( m_slot ), true );
  
  setCheckable( true );
  setAutoExclusive( true );
  
  QObject::connect( this,    SIGNAL( selectedSlot( slot_type ) ),
                    profile, SLOT( setSelectedSlot( slot_type ) ) );
  QObject::connect( profile, SIGNAL( itemChanged( slot_type, const item_data_t* ) ),
                    this,    SLOT( setSlotItem( slot_type, const item_data_t* ) ) );
}

QSize
PaperdollSlotButton::sizeHint() const
{
  return QSize( 64, 64 );
}

void
PaperdollSlotButton::mousePressEvent( QMouseEvent* event )
{
  QAbstractButton::mousePressEvent( event );
  emit selectedSlot( m_slot );
}

void
PaperdollSlotButton::paintEvent( QPaintEvent* event )
{
  QPainter paint;

  paint.begin( this );

  // Empty slot
  if ( ! m_profile -> slotItem( m_slot ) )
    paint.drawPixmap( event -> rect(), m_slotIcon );
  // Item in it, paint the item icon instead
  else
    paint.drawPixmap( event -> rect(), PaperdollPixmap::get( m_profile -> slotItem( m_slot ) -> icon, true ) );

  // Selection border
  if ( isChecked() )
    paint.drawPixmap( event -> rect(), PaperdollPixmap::get( "UI-Quickslot-Depress" ) );
  
  paint.end();
}

void
PaperdollSlotButton::setSlotItem( slot_type t, const item_data_t* )
{
  if ( t == m_slot )
    update();
}

Paperdoll::Paperdoll( PaperdollProfile* profile, QWidget* parent ) :
  QWidget( parent ), m_profile( profile )
{
  m_layout = new QGridLayout();
  m_layout -> setColumnMinimumWidth( 1, 96 );
  m_layout -> setColumnMinimumWidth( 2, 96 );
  m_layout -> setColumnMinimumWidth( 3, 96 );
  
  m_slotWidgets[ SLOT_HEAD ]      = new PaperdollSlotButton( SLOT_HEAD,      profile, this );
  m_slotWidgets[ SLOT_NECK ]      = new PaperdollSlotButton( SLOT_NECK,      profile, this );
  m_slotWidgets[ SLOT_SHOULDERS ] = new PaperdollSlotButton( SLOT_SHOULDERS, profile, this );
  m_slotWidgets[ SLOT_BACK ]      = new PaperdollSlotButton( SLOT_BACK,      profile, this );
  m_slotWidgets[ SLOT_CHEST ]     = new PaperdollSlotButton( SLOT_CHEST,     profile, this );
  m_slotWidgets[ SLOT_TABARD ]    = new PaperdollSlotButton( SLOT_TABARD,    profile, this );
  m_slotWidgets[ SLOT_SHIRT ]     = new PaperdollSlotButton( SLOT_SHIRT,     profile, this );
  m_slotWidgets[ SLOT_WRISTS ]    = new PaperdollSlotButton( SLOT_WRISTS,    profile, this );

  m_slotWidgets[ SLOT_HANDS ]     = new PaperdollSlotButton( SLOT_HANDS,     profile, this );
  m_slotWidgets[ SLOT_WAIST ]     = new PaperdollSlotButton( SLOT_WAIST,     profile, this );
  m_slotWidgets[ SLOT_LEGS ]      = new PaperdollSlotButton( SLOT_LEGS,      profile, this );
  m_slotWidgets[ SLOT_FEET ]      = new PaperdollSlotButton( SLOT_FEET,      profile, this );
  m_slotWidgets[ SLOT_FINGER_1 ]  = new PaperdollSlotButton( SLOT_FINGER_1,  profile, this );
  m_slotWidgets[ SLOT_FINGER_2 ]  = new PaperdollSlotButton( SLOT_FINGER_2,  profile, this );
  m_slotWidgets[ SLOT_TRINKET_1 ] = new PaperdollSlotButton( SLOT_TRINKET_1, profile, this );
  m_slotWidgets[ SLOT_TRINKET_2 ] = new PaperdollSlotButton( SLOT_TRINKET_2, profile, this );

  m_slotWidgets[ SLOT_MAIN_HAND ] = new PaperdollSlotButton( SLOT_MAIN_HAND, profile, this );
  m_slotWidgets[ SLOT_OFF_HAND ]  = new PaperdollSlotButton( SLOT_OFF_HAND,  profile, this );
  m_slotWidgets[ SLOT_RANGED ]    = new PaperdollSlotButton( SLOT_RANGED,    profile, this );
  
  m_layout -> addWidget( m_slotWidgets[ SLOT_HEAD ],      0, 0, Qt::AlignLeft | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_NECK ],      1, 0, Qt::AlignLeft | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_SHOULDERS ], 2, 0, Qt::AlignLeft | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_BACK ],      3, 0, Qt::AlignLeft | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_CHEST ],     4, 0, Qt::AlignLeft | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_TABARD ],    5, 0, Qt::AlignLeft | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_SHIRT ],     6, 0, Qt::AlignLeft | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_WRISTS ],    7, 0, Qt::AlignLeft | Qt::AlignTop );

  m_layout -> addWidget( m_slotWidgets[ SLOT_MAIN_HAND ], 8, 1, Qt::AlignCenter | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_OFF_HAND ],  8, 2, Qt::AlignCenter | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_RANGED ],    8, 3, Qt::AlignCenter | Qt::AlignTop );
  
  m_layout -> addWidget( m_slotWidgets[ SLOT_HANDS ],     0, 4, Qt::AlignRight | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_WAIST ],     1, 4, Qt::AlignRight | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_LEGS ],      2, 4, Qt::AlignRight | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_FEET ],      3, 4, Qt::AlignRight | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_FINGER_1 ],  4, 4, Qt::AlignRight | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_FINGER_2 ],  5, 4, Qt::AlignRight | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_TRINKET_1 ], 6, 4, Qt::AlignRight | Qt::AlignTop );
  m_layout -> addWidget( m_slotWidgets[ SLOT_TRINKET_2 ], 7, 4, Qt::AlignRight | Qt::AlignTop );

  setLayout( m_layout );
}

QSize
Paperdoll::sizeHint() const
{
  return QSize( 400, 616 );
}
