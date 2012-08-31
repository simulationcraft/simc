// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include <QDebug>
#include <QDir>
#include <QPixmapCache>
#include <QCoreApplication>
#include <QTextDocument>

#include <algorithm>

#include "simcpaperdoll.hpp"

// =============================================================================
// Temporary place to do implementation, will be removed later on
// =============================================================================
#define RAND_SUFFIX_GROUP_SIZE (51)

static struct random_suffix_group_t __rand_suffix_group_data[] = {
  {  390, {   5,   6,   7,   8,  14,  36,  37,  39,  40,  41,  42,  43,  45,  91, 114, 118, 120, 121, 122, 123, 124, 125, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 0 } },
  {  391, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  392, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  393, {   5,   6,   7,   8,  14,  36,  37,  39,  40,  41,  42,  43,  45,  91, 114, 118, 120, 121, 122, 123, 124, 125, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 0 } },
  {  394, {   6,   8,  36,  37,  39,  41,  42, 114, 129, 130, 131, 132, 138, 0 } },
  {  395, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  396, {   5,  40,  41,  91, 133, 134, 135, 136, 137, 0 } },
  {  397, {   5,  40,  41,  91, 133, 134, 135, 136, 137, 0 } },
  {  398, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  399, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  400, {   6,   8,  36,  37,  39,  42, 130, 131, 132, 138, 0 } },
  {  401, {   6,   8,  36,  37,  39,  41,  42, 114, 129, 130, 131, 132, 138, 0 } },
  {  402, {   5,  40,  91, 131, 132, 133, 134, 135, 136, 137, 0 } },
  {  403, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  404, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  405, {   5,  40,  91, 131, 132, 133, 134, 135, 136, 137, 0 } },
  {  406, {   6,   7,   8,  14,  36,  37,  39,  41,  42,  43,  45, 118, 120, 121, 122, 123, 124, 125, 127, 128, 131, 132, 139, 0 } },
  {  407, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  408, {   6,   7,   8,  14,  36,  37,  39,  41,  42,  43,  45, 118, 120, 121, 122, 123, 124, 125, 127, 128, 131, 132, 139, 0 } },
  {  409, {   6,   8,  36,  37,  39,  41,  42, 114, 129, 130, 131, 132, 138, 0 } },
  {  410, {   6,   8,  36,  37,  39,  41,  42, 114, 129, 130, 131, 132, 138, 0 } },
  {  411, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  412, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  413, {   5,   7,  14,  40,  41,  43,  91, 118, 120, 121, 122, 123, 133, 134, 135, 136, 137, 139, 0 } },
  {  414, {   6,   8,  36,  37,  39,  41,  42, 114, 129, 130, 131, 132, 138, 0 } },
  {  415, { 154, 155, 156, 157, 158, 159, 0 } },
  {  417, { 169, 170, 171, 172, 0 } },
  {  418, { 203, 204, 205, 206, 0 } },
  {  419, { 220, 221, 222, 223, 0 } },
  {  420, { 173, 174, 175, 176, 0 } },
  {  421, { 118, 120, 121, 122, 0 } },
  {  422, { 180, 181, 182, 0 } },
  {  423, { 224, 225, 226, 0 } },
  {  424, { 207, 208, 209, 0 } },
  {  425, { 177, 178, 179, 0 } },
  {  426, { 125, 127, 128, 0 } },
  {  427, { 183, 184, 185, 186, 187, 188, 0 } },
  {  429, { 114, 129, 130, 131, 132, 138, 0 } },
  {  431, { 189, 190, 191, 192, 193, 194, 0 } },
  {  432, { 191, 192, 193, 194, 0 } },
  {  433, { 229, 230, 231, 232, 0 } },
  {  434, { 212, 213, 214, 215, 0 } },
  {  435, { 185, 186, 187, 188, 0 } },
  {  437, { 195, 196, 197, 198, 0 } },
  {  438, { 216, 217, 218, 219, 0 } },
  {  439, { 233, 234, 235, 236, 0 } },
  {  440, { 199, 200, 201, 202, 0 } },
  {  441, { 133, 135, 136, 137, 0 } },
  {  442, { 114, 118, 120, 121, 122, 123, 124, 125, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 0 } },
  {  450, {   5,  40,  91, 131, 132, 133, 134, 135, 136, 137, 0 } },
  {  451, {   5,  40,  91, 131, 132, 133, 134, 135, 136, 137, 0 } },
  {    0, {   0 } }
};
// =============================================================================

#if 0
QString PaperdollPixmap::rpath;

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

const QString&
PaperdollPixmap::resourcePath( void )
{
  if ( PaperdollPixmap::rpath.isEmpty() )
  {
#ifdef Q_WS_MAC
    PaperdollPixmap::rpath = QString( "%1/../Resources/" ).arg( QCoreApplication::instance() -> applicationDirPath() );
#else
    PaperdollPixmap::rpath = QString( "%1/qt/icons/" ).arg( QCoreApplication::instance() -> applicationDirPath() );
#endif
  }

  return PaperdollPixmap::rpath;
}

PaperdollPixmap::PaperdollPixmap() : QPixmap() { }

PaperdollPixmap::PaperdollPixmap( const QString& icon, bool draw_border, QSize s ) :
  QPixmap( s )
{
  QImage icon_image( QString( resourcePath() + icon ), 0 );
  if ( icon_image.isNull() )
    icon_image = QImage( QString( QDir::homePath() + "/WoWAssets/Icons/" + icon ) );

  QImage border( resourcePath() + "border" );

  if ( icon_image.isNull() )
    icon_image = QImage( QString( resourcePath() + "ABILITY_SEAL" ), 0 );

  QSize d = ( draw_border ? border.size() : size() ) - icon_image.size();
  d /= 2;

  assert( ! draw_border || border.size() == size() );

  fill( QColor( 0, 0, 0, 0 ) );

  QPainter paint( this );
  paint.drawImage( d.width(), d.height(), icon_image );
  if ( draw_border ) paint.drawImage( 0, 0, border );
}
#endif

QPixmap
getPaperdollPixmap( const QString& name, bool draw_border, QSize s )
{
  QPixmap icon( s );

  if ( ! QPixmapCache::find( name, &icon ) )
  {
    icon.fill( QColor( 0, 0, 0, 0 ) );
    QPainter paint( &icon );

    QImage icon_image( "icon:" + name + ".png" );
    if ( icon_image.isNull() )
    {
      icon_image = QImage( QDir::homePath() + "/WoWAssets/Icons/" + name );
      if ( icon_image.isNull() )
        icon_image = QImage( "icon:ABILITY_SEAL.PNG" );
    }

    QSize d = s;

    if ( draw_border )
    {
      QImage border( "icon:border.png" );
      assert( border.size() == s );
      paint.drawImage( 0, 0, border );
      d = border.size();
    }

    d = ( d - icon_image.size() ) / 2;

    paint.drawImage( d.width(), d.height(), icon_image );

    QPixmapCache::insert( name, icon );
  }

  return icon;
}

PaperdollProfile::PaperdollProfile() :
  QObject(),
  m_class( PLAYER_NONE ), m_race( RACE_NONE ), m_currentSlot( SLOT_INVALID )
{
  m_professions[ 0 ] = m_professions[ 1 ] = PROFESSION_NONE;

  for ( int i = 0; i < SLOT_MAX; i++ )
  {
    m_slotItem[ i ] = 0;
    m_slotEnchant[ i ] = EnchantData();
  }
}

void
PaperdollProfile::setSelectedSlot( slot_e t )
{
  m_currentSlot = t;
  emit slotChanged( m_currentSlot );
  emit profileChanged();
}

void
PaperdollProfile::setSelectedItem( const QModelIndex& index )
{
  const item_data_t* item = reinterpret_cast< const item_data_t* >( index.data( Qt::UserRole ).value< void* >() );

  m_slotItem[ m_currentSlot ] = item;

  emit itemChanged( m_currentSlot, m_slotItem[ m_currentSlot ] );
  emit profileChanged();
}

void
PaperdollProfile::setSelectedEnchant( int idx )
{
  if ( QComboBox* sender = qobject_cast< QComboBox* >( QObject::sender() ) )
    m_slotEnchant[ m_currentSlot ] = sender -> model() -> index ( idx, 0 ).data( Qt::UserRole ).value< EnchantData >();

  emit enchantChanged( m_currentSlot, m_slotEnchant[ m_currentSlot ] );
  emit profileChanged();
}

void
PaperdollProfile::setSelectedSuffix( int idx )
{
  if ( QComboBox* sender = qobject_cast< QComboBox* >( QObject::sender() ) )
    m_slotSuffix[ m_currentSlot ] = sender -> model() -> index ( idx, 0 ).data( Qt::UserRole ).value< unsigned >();

  emit suffixChanged( m_currentSlot, m_slotSuffix[ m_currentSlot ] );
  emit profileChanged();
}

void
PaperdollProfile::setClass( int player_class )
{
  assert( player_class > PLAYER_NONE && player_class < PLAYER_PET );
  m_class = ( player_e ) player_class;

  for ( slot_e t = SLOT_INVALID; t < SLOT_MAX; t=(slot_e)((int)t+1) )
    validateSlot( t );

  emit classChanged( m_class );
  emit profileChanged();
}

void
PaperdollProfile::setRace( int player_race )
{
  assert( player_race >= RACE_NIGHT_ELF && player_race < RACE_MAX && player_race != RACE_PANDAREN );
  m_race = ( race_e ) player_race;

  for ( slot_e t = SLOT_INVALID; t < SLOT_MAX; t=(slot_e)((int)t+1) )
    validateSlot( t );

  emit raceChanged( m_race );
  emit profileChanged();
}

void
PaperdollProfile::setProfession( int profession, int type )
{
  assert( profession >= 0 && profession < 2 && type > PROFESSION_NONE && type < PROFESSION_MAX );

  m_professions[ profession ] = ( profession_e ) type;

  for ( slot_e t = SLOT_INVALID; t < SLOT_MAX; t=(slot_e)((int)t+1) )
    validateSlot( t );

  emit professionChanged( m_professions[ profession ] );
  emit profileChanged();
}

bool
PaperdollProfile::enchantUsableByProfile( const EnchantData& e ) const
{
  if ( e.enchant == 0 ) return true;

  if ( static_cast<unsigned>( m_slotItem[ m_currentSlot ] -> item_class ) != e.enchant -> _equipped_class )
    return false;

  if ( e.enchant -> _equipped_invtype_mask &&
      ! ( ( 1 << m_slotItem[ m_currentSlot ] -> inventory_type ) & e.enchant -> _equipped_invtype_mask ) )
    return false;

  if ( e.enchant -> _equipped_subclass_mask &&
      ! ( ( 1 << m_slotItem[ m_currentSlot ] -> item_subclass ) & e.enchant -> _equipped_subclass_mask ) )
    return false;

  if ( e.item_enchant -> req_skill > 0 )
  {
    unsigned profession_1 = ItemFilterProxyModel::professionIds[ m_professions[ 0 ] ],
             profession_2 = ItemFilterProxyModel::professionIds[ m_professions[ 1 ] ];

    if ( e.item_enchant -> req_skill == profession_1 || e.item_enchant -> req_skill == profession_2 )
      return true;

    return false;
  }

  return true;
}

bool
PaperdollProfile::itemUsableByClass( const item_data_t* item, bool match_armor ) const
{
  if ( ! item -> class_mask & util::class_id_mask( m_class ) ) return false;

  if ( item -> item_class == ITEM_CLASS_WEAPON )
  {
    // Dual wield isnt for everyone
    if ( item -> inventory_type == INVTYPE_WEAPONOFFHAND &&
         m_class != SHAMAN && m_class != DEATH_KNIGHT && m_class != ROGUE &&
         m_class != HUNTER && m_class != WARRIOR && m_class != MONK )
      return false;

    switch ( item -> item_subclass )
    {
      case ITEM_SUBCLASS_WEAPON_AXE:
        return m_class == DEATH_KNIGHT || m_class == HUNTER  ||
               m_class == PALADIN      || m_class == ROGUE   ||
               m_class == SHAMAN       || m_class == WARRIOR || m_class == MONK;
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
               m_class == WARRIOR      || m_class == MONK;
      case ITEM_SUBCLASS_WEAPON_MACE2:
        return m_class == DEATH_KNIGHT || m_class == DRUID   ||
               m_class == PALADIN      || m_class == SHAMAN  ||
               m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_POLEARM:
        return m_class == DEATH_KNIGHT || m_class == DRUID   ||
               m_class == HUNTER       || m_class == PALADIN ||
               m_class == WARRIOR      || m_class == MONK;
      case ITEM_SUBCLASS_WEAPON_SWORD:
        return m_class == DEATH_KNIGHT || m_class == HUNTER  ||
               m_class == MAGE         || m_class == PALADIN ||
               m_class == ROGUE        || m_class == WARLOCK ||
               m_class == WARRIOR      || m_class == MONK;
      case ITEM_SUBCLASS_WEAPON_SWORD2:
        return m_class == DEATH_KNIGHT || m_class == HUNTER  ||
               m_class == PALADIN      || m_class == WARRIOR;
      case ITEM_SUBCLASS_WEAPON_STAFF:
        return m_class == DRUID        || m_class == HUNTER  ||
               m_class == MAGE         || m_class == PRIEST  ||
               m_class == SHAMAN       || m_class == WARLOCK ||
               m_class == WARRIOR      || m_class == MONK;
      case ITEM_SUBCLASS_WEAPON_FIST:
        return m_class == DRUID        || m_class == HUNTER  ||
               m_class == ROGUE        || m_class == SHAMAN  ||
               m_class == WARRIOR      || m_class == MONK;
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
        if ( ! match_armor || item -> inventory_type == INVTYPE_CLOAK )
          return true;
        else
          return m_class == MAGE         || m_class == PRIEST  ||
                 m_class == WARLOCK;
      case ITEM_SUBCLASS_ARMOR_LEATHER:
        if ( ! match_armor )
          return m_class == DEATH_KNIGHT || m_class == DRUID   ||
                 m_class == HUNTER       || m_class == PALADIN ||
                 m_class == ROGUE        || m_class == SHAMAN  ||
                 m_class == WARRIOR      || m_class == MONK;
        else
          return m_class == DRUID        || m_class == ROGUE   || m_class == MONK;
      case ITEM_SUBCLASS_ARMOR_MAIL:
        if ( ! match_armor )
          return m_class == DEATH_KNIGHT || m_class == HUNTER  ||
                 m_class == PALADIN      || m_class == SHAMAN  ||
                 m_class == WARRIOR;
        else
          return m_class == HUNTER       || m_class == SHAMAN;
      case ITEM_SUBCLASS_ARMOR_PLATE:
        return m_class == DEATH_KNIGHT || m_class == PALADIN ||
               m_class == WARRIOR;
      case ITEM_SUBCLASS_ARMOR_SHIELD:
        return m_class == PALADIN        || m_class == SHAMAN  ||
               m_class == WARRIOR;
      case ITEM_SUBCLASS_ARMOR_LIBRAM:
        return m_class == PALADIN;
      case ITEM_SUBCLASS_ARMOR_IDOL:
        return m_class == DRUID;
      case ITEM_SUBCLASS_ARMOR_TOTEM:
        return m_class == SHAMAN;
      case ITEM_SUBCLASS_ARMOR_SIGIL:
        return m_class == DEATH_KNIGHT   || m_class == SHAMAN ||
               m_class == PALADIN        || m_class == DRUID;
    }
  }

  return false;
}

bool
PaperdollProfile::itemUsableByRace( const item_data_t* item ) const
{
  return item -> race_mask & util::race_mask( m_race );
}

bool
PaperdollProfile::itemUsableByProfession( const item_data_t* item ) const
{
  if ( item -> req_skill > 0 &&
       item -> req_skill != m_professions[ 0 ] &&
       item -> req_skill != m_professions[ 1 ] )
  {
    return false;
  }

  return true;
}

bool
PaperdollProfile::itemUsableByProfile( const item_data_t* item ) const
{
  if ( ! itemUsableByClass( item, false ) )
    return false;

  if ( ! itemUsableByProfession( item ) )
    return false;

  return true;
}

bool
PaperdollProfile::itemFitsProfileSlot( const item_data_t* item ) const
{
  switch ( item -> inventory_type )
  {
    case INVTYPE_HEAD:           return m_currentSlot == SLOT_HEAD;
    case INVTYPE_NECK:           return m_currentSlot == SLOT_NECK;
    case INVTYPE_SHOULDERS:      return m_currentSlot == SLOT_SHOULDERS;
    case INVTYPE_BODY:           return m_currentSlot == SLOT_SHIRT;
    case INVTYPE_CHEST:          return m_currentSlot == SLOT_CHEST;
    case INVTYPE_WAIST:          return m_currentSlot == SLOT_WAIST;
    case INVTYPE_LEGS:           return m_currentSlot == SLOT_LEGS;
    case INVTYPE_FEET:           return m_currentSlot == SLOT_FEET;
    case INVTYPE_WRISTS:         return m_currentSlot == SLOT_WRISTS;
    case INVTYPE_HANDS:          return m_currentSlot == SLOT_HANDS;
    case INVTYPE_FINGER:         return m_currentSlot == SLOT_FINGER_1  || m_currentSlot == SLOT_FINGER_2;
    case INVTYPE_TRINKET:        return m_currentSlot == SLOT_TRINKET_1 || m_currentSlot == SLOT_TRINKET_2;
    case INVTYPE_WEAPON:
    {
     return m_currentSlot == SLOT_MAIN_HAND ||
        ( ( m_class == SHAMAN || m_class == DEATH_KNIGHT || m_class == ROGUE || m_class == HUNTER || m_class == WARRIOR || m_class == MONK ) && m_currentSlot == SLOT_OFF_HAND );
    }
    case INVTYPE_SHIELD:         return m_currentSlot == SLOT_OFF_HAND;
    case INVTYPE_RANGED:         return m_currentSlot == SLOT_RANGED;
    case INVTYPE_CLOAK:          return m_currentSlot == SLOT_BACK;
    case INVTYPE_2HWEAPON:       return m_currentSlot == SLOT_MAIN_HAND;
    case INVTYPE_TABARD:         return m_currentSlot == SLOT_TABARD;
    case INVTYPE_ROBE:           return m_currentSlot == SLOT_CHEST;
    case INVTYPE_WEAPONMAINHAND: return m_currentSlot == SLOT_MAIN_HAND;
    case INVTYPE_WEAPONOFFHAND:  return m_currentSlot == SLOT_OFF_HAND;
    case INVTYPE_HOLDABLE:       return m_currentSlot == SLOT_OFF_HAND;
    case INVTYPE_THROWN:         return m_currentSlot == SLOT_RANGED;
    case INVTYPE_RELIC:          return m_currentSlot == SLOT_RANGED;
  }

  return false;
}

void
PaperdollProfile::validateSlot( slot_e t )
{
  if ( t == SLOT_INVALID ) return;
  if ( ! m_slotItem[ t ] ) return;

  // Make sure item is still usable, if not, clear the slot
  if ( ! itemUsableByProfile( m_slotItem[ t ] ) )
  {
    if ( clearSlot( t ) ) emit itemChanged( t, m_slotItem[ t ] );
  }

  // Validate enchant
  if ( m_slotEnchant[ t ].enchant != 0 && ! enchantUsableByProfile( m_slotEnchant[ t ] ) )
  {
    m_slotEnchant[ t ] = EnchantData();
    emit enchantChanged( t, m_slotEnchant[ t ] );
  }
}

bool
PaperdollProfile::clearSlot( slot_e t )
{
  if ( t == SLOT_INVALID ) return false;
  bool has_item = m_slotItem[ t ] != 0;

  m_slotItem   [ t ] = 0;
  m_slotSuffix [ t ] = 0;
  m_slotEnchant[ t ] = EnchantData();

  return has_item;
}

ItemFilterProxyModel::ItemFilterProxyModel( PaperdollProfile* profile, QObject* parent ) :
  QSortFilterProxyModel( parent ),
  m_profile( profile ),
  m_matchArmor( true ), m_minIlevel( 378 ), m_maxIlevel( 580 ),
  m_searchText()
{

  QObject::connect( profile, SIGNAL( profileChanged() ),
                    this,    SLOT( filterChanged() ) );

  QObject::connect( profile, SIGNAL( profileChanged() ),
                    this,    SLOT( filterChanged() ) );

  QObject::connect( profile, SIGNAL( profileChanged() ),
                   this,    SLOT( filterChanged() ) );
}

void
ItemFilterProxyModel::setMinIlevel( int newValue )
{
  m_minIlevel = newValue;
  filterChanged();
}

void
ItemFilterProxyModel::setMaxIlevel( int newValue )
{
  m_maxIlevel = newValue;
  filterChanged();
}

void
ItemFilterProxyModel::setSlot( slot_e slot )
{
  filterChanged();

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
  filterChanged();
}

void
ItemFilterProxyModel::setMatchArmor( int newValue )
{
  m_matchArmor = newValue;
  filterChanged();
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

  if ( m_profile -> currentSlot() == SLOT_INVALID )
    return false;

  bool state = filterByName( item ) &&
               m_profile -> itemFitsProfileSlot( item ) &&
               item -> level >= m_minIlevel && item -> level <= m_maxIlevel &&
               m_profile -> itemUsableByClass( item, m_matchArmor ) &&
               m_profile -> itemUsableByRace( item ) &&
               m_profile -> itemUsableByProfession( item ) &&
               itemUsedByClass( item );

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

int
ItemFilterProxyModel::primaryStat( void ) const
{
  player_e player_class = m_profile -> currentClass();

  switch ( player_class )
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
    default:
      return -0xFF;
  }
}

bool
ItemFilterProxyModel::itemUsedByClass( const item_data_t* item ) const
{
  player_e player_class = m_profile -> currentClass();
  bool primary_stat_found = ( player_class == DRUID || player_class == SHAMAN || player_class == PALADIN || player_class == MONK );

  if ( item -> inventory_type == INVTYPE_TRINKET ) return true;
  if ( item -> id_suffix_group > 0 ) return true;

  for ( int i = 0; i < 10; i++ )
  {
    if ( item -> stat_type_e[ i ] == primaryStat() )
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
ItemDataListModel::rowCount( const QModelIndex& /* parent */ ) const
{
  return dbc_t::n_items( false );
}

QVariant
ItemDataListModel::data( const QModelIndex& index, int role ) const
{
  const item_data_t* items = dbc_t::items( false );

  if ( index.row() > static_cast<int>( dbc_t::n_items( false ) ) - 1 )
    return QVariant( QVariant::Invalid );

  if ( role == Qt::DecorationRole )
    return QVariant::fromValue( getPaperdollPixmap( items[ index.row() ].icon, true ).scaled( 48, 48 ) );
  else if ( role == Qt::UserRole )
    return QVariant::fromValue<void*>( const_cast<item_data_t*>( &( items[ index.row() ] ) ) );

  return QVariant( QVariant::Invalid );
}

void
ItemDataListModel::dataSourceChanged( int /* newChoice */ )
{
  reset();
}

ItemDataDelegate::ItemDataDelegate( QObject* parent ) :
  QStyledItemDelegate( parent )
{
}

QSize
ItemDataDelegate::sizeHint( const QStyleOptionViewItem& /* option */, const QModelIndex& /* index */ ) const
{
  return QSize( 350, 50 );
}

void
ItemDataDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  const item_data_t* item = reinterpret_cast< const item_data_t* >( index.data( Qt::UserRole ).value< void* >() );
  QPixmap icon = index.data( Qt::DecorationRole ).value< QPixmap >();
  QRect draw_area = option.rect.adjusted( 1, 1, -1, -1 );

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
      socket_icon = getPaperdollPixmap( QString( "socket_%1" ).arg( item -> socket_color[ i ] ), false, QSize( 16, 16 ) );
      if ( ! socket_icon.isNull() )
        painter -> drawPixmap( draw_area.x() + icon.width() + 1,
                               draw_area.y() + ( i * ( socket_icon.height() + 1 ) ),
                               socket_icon.width(),
                               socket_icon.height(),
                               socket_icon );
    }

    has_sockets = true;
  }

  QRect stats_area = draw_area.translated( icon.width() + 1 + ( has_sockets ? socket_icon.width() + 1 : 0 ), 0 );
  stats_area.setSize( QSize( draw_area.width() - ( icon.width() + 1 + ( has_sockets ? socket_icon.width() : 0 ) + 2 ),
                             draw_area.height() - 12 ) );

  QRect flags_area = draw_area.translated( icon.width() + 1 + ( has_sockets ? socket_icon.width() + 1 : 0 ),
                                           draw_area.height() - 12 );
  flags_area.setSize( QSize( stats_area.width(), 12 ) );

  painter -> setRenderHint(QPainter::Antialiasing, true);
  QTextDocument text;

  painter -> save();
  painter -> translate( stats_area.x(), stats_area.y() );
  text.setDocumentMargin( 0 );
  text.setPageSize( stats_area.size() );
  text.setHtml( itemStatsString( item ) );
  text.drawContents( painter );
  painter -> restore();

  painter -> save();
  painter -> translate( flags_area.x(), flags_area.y() );
  text.setDocumentMargin( 0 );
  text.setPageSize( flags_area.size() );
  text.setHtml( ItemDataDelegate::itemFlagStr( item ) );
  text.drawContents( painter );
  painter -> restore();

  painter -> restore();
}

QString
ItemDataDelegate::itemQualityColor( const item_data_t* item )
{
  assert( item );

  switch ( item -> quality )
  {
    case 0:
      return QString( "#%1%2%3" ).arg( 0x9d, 0, 16 ).arg( 0x9d, 0, 16 ).arg( 0x9d, 0, 16 );
    case 1:
      return QString( "#%1%2%3" ).arg( 0xff, 0, 16 ).arg( 0xff, 0, 16 ).arg( 0xff, 0, 16 );
    case 2:
      return QString( "#%1%2%3" ).arg( 0x1e, 0, 16 ).arg( 0xff, 0, 16 ).arg( 0x00, 0, 16 );
    case 3:
      return QString( "#%1%2%3" ).arg( 0x00, 0, 16 ).arg( 0x70, 0, 16 ).arg( 0xff, 0, 16 );
    case 4:
      return QString( "#%1%2%3" ).arg( 0xb0, 0, 16 ).arg( 0x48, 0, 16 ).arg( 0xf8, 0, 16 );
    case 5:
      return QString( "#%1%2%3" ).arg( 0xff, 0, 16 ).arg( 0x80, 0, 16 ).arg( 0x80, 0, 16 );
  }

  return QString("white");
}

QString
ItemDataDelegate::itemFlagStr( const item_data_t* item )
{
  QStringList str;

  str += QString( "%1" ).arg( item -> level );
  if ( item -> flags_1 & ITEM_FLAG_HEROIC )
    str += QString( "<span style='color:#1eff00;font-style:italic;'>%1</span>" ).arg( "Heroic" );

  if ( item -> id_set > 0 && util::set_item_type_string( item -> id_set ) != 0 )
    str += QString( "<span style='color:yellow;'>%1 Set</span>" ).arg( util::set_item_type_string( item -> id_set ) );

  return QString( "<div align='right' style='font-size:9pt;color:white;'>%1</div>" ).arg( str.join(" ") );
}

QString
ItemDataDelegate::itemStatsString( const item_data_t* item, bool /* html */ ) const
{
  assert( item != 0 );
  QStringList stats;
  QStringList weapon_stats;
  QString stats_str;
  uint32_t armor;
  dbc_t dbc( false );

  stats_str += QString( "<span style='font-size:11pt;font-weight:bold;color:%1;'>%2</span><br />" )
                      .arg( itemQualityColor( item ) )
                      .arg( item -> name );

  if ( item -> item_class == ITEM_CLASS_WEAPON )
  {
    weapon_stats += QString( "<strong>%1%2" )
      .arg( util::weapon_class_string( ( inventory_type ) item -> inventory_type ) ? QString( util::weapon_class_string( ( inventory_type ) item -> inventory_type ) ) + QString( " " ) : "" )
      .arg( util::weapon_subclass_string( ( item_subclass_weapon ) item -> item_subclass ) );
    weapon_stats += QString( "%1 - %2 (%3)" )
      .arg( item_database::weapon_dmg_min( item, dbc ) )
      .arg( item_database::weapon_dmg_max( item, dbc ) )
      .arg( dbc.weapon_dps( item -> id ), 0, 'f', 1 );
    weapon_stats += QString( "%1 Speed</strong>" ).arg( item -> delay / 1000.0, 0, 'f', 2 );
  }

  if ( ( armor = item_database::armor_value( item, dbc ) ) > 0 )
    stats += QString( "%1 Armor" ).arg( armor );

  for ( int i = 0; i < 10; i++ )
  {
    if ( item -> stat_type_e[ i ] < 0 || item -> stat_val[ i ] == 0 )
      continue;

    stat_e t = util::translate_item_mod( ( item_mod_type ) item -> stat_type_e[ i ] );
    if ( t == STAT_NONE ) continue;

    stats << QString( "%1 %2" )
      .arg( item -> stat_val[ i ] )
      .arg( util::stat_type_abbrev( t ) );
  }

  if ( item -> id_suffix_group > 0 )
    stats += "<span style='font-size:10pt;color:#1eff00'>&lt;Random Enchantment&gt;</span>";

  if ( weapon_stats.size() > 0 )
  {
    stats_str += weapon_stats.join( ", " );
    stats_str += "<br />";
  }

  if ( stats.size() > 0 )
    stats_str += stats.join( ", " );

  return QString("<div style='font-size:10pt;color:white;'>%1</div>").arg(stats_str);
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

  m_itemSetupRandomSuffix     = new QGroupBox( m_itemSetup );
  m_itemSetupRandomSuffixLayout = new QHBoxLayout();

  m_itemSetupRandomSuffixModel = new RandomSuffixDataModel( profile );
  m_itemSetupRandomSuffixView = new QComboBox( m_itemSetup );

  m_itemSetupEnchantBox       = new QGroupBox( m_itemSetup );
  m_itemSetupEnchantBoxLayout = new QHBoxLayout();
  m_itemSetupEnchantView      = new QComboBox( m_itemSetup );
  m_itemSetupEnchantModel     = new EnchantDataModel( profile );
  m_itemSetupEnchantProxy     = new EnchantFilterProxyModel( profile );

  // Search widget setup

  m_itemSearchView -> setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  m_itemSearchView -> setModel( m_itemSearchProxy );
  m_itemSearchView -> setItemDelegate( m_itemSearchDelegate );

  m_itemSearchProxy -> setSourceModel( m_itemSearchModel );

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
  m_itemFilterMinIlevel -> setRange( 378, 580 );
  m_itemFilterMinIlevel -> setSingleStep( 1 );
  m_itemFilterMinIlevel -> setPageStep( 10 );
  m_itemFilterMinIlevel -> setTracking( false );
  m_itemFilterMinIlevel -> setValue ( m_itemSearchProxy -> minIlevel() );

  m_itemFilterMinIlevelLabel -> setNum( m_itemSearchProxy -> minIlevel() );

  m_itemFilterMinIlevelLayout -> addWidget( m_itemFilterMinIlevel, 0, Qt::AlignLeft | Qt::AlignCenter );
  m_itemFilterMinIlevelLayout -> addWidget( m_itemFilterMinIlevelLabel, 0, Qt::AlignLeft | Qt::AlignCenter );

  m_itemFilterMaxIlevel -> setTickPosition( QSlider::TicksBelow );
  m_itemFilterMaxIlevel -> setRange( 378, 580 );
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
                    m_itemSearchProxy,          SLOT( setMatchArmor( int ) ) );

  m_itemFilter -> setLayout( m_itemFilterLayout );

  // Item setup widget

  m_itemSetup -> setLayout( m_itemSetupLayout );

  m_itemSetupRandomSuffix -> setTitle( "Random Suffix" );
  m_itemSetupRandomSuffix -> setLayout( m_itemSetupRandomSuffixLayout );
  m_itemSetupRandomSuffix -> setVisible( false );

  m_itemSetupRandomSuffixLayout -> addWidget( m_itemSetupRandomSuffixView );
  m_itemSetupRandomSuffixLayout -> setAlignment( Qt::AlignCenter | Qt::AlignTop );

  m_itemSetupRandomSuffixView -> setModel( m_itemSetupRandomSuffixModel );

  m_itemSetupEnchantBox -> setTitle( "Enchant" );
  m_itemSetupEnchantBox -> setLayout( m_itemSetupEnchantBoxLayout );
  m_itemSetupEnchantBox -> setVisible( false );

  m_itemSetupEnchantBoxLayout -> addWidget( m_itemSetupEnchantView );
  m_itemSetupEnchantBoxLayout -> setAlignment( Qt::AlignCenter | Qt::AlignTop );

  m_itemSetupEnchantView -> setModel( m_itemSetupEnchantProxy );
  m_itemSetupEnchantView -> setEditable( false );

  m_itemSetupEnchantProxy -> setSourceModel( m_itemSetupEnchantModel );

  m_itemSetupLayout -> addWidget( m_itemSetupRandomSuffix );
  m_itemSetupLayout -> addWidget( m_itemSetupEnchantBox );

  QObject::connect( profile,                 SIGNAL( slotChanged( slot_e ) ),
                    m_itemSetupEnchantView,  SLOT( update() ) );

  QObject::connect( profile,                 SIGNAL( itemChanged( slot_e, const item_data_t* ) ),
                    m_itemSetupEnchantView,  SLOT( update() ) );

  QObject::connect( profile,                 SIGNAL( professionChanged( profession_e ) ),
                    m_itemSetupEnchantView,  SLOT( update() ) );

  QObject::connect( m_itemSetupEnchantProxy, SIGNAL( enchantSelected( int ) ),
                    m_itemSetupEnchantView,  SLOT( setCurrentIndex( int ) ) );

  QObject::connect( m_itemSetupEnchantProxy, SIGNAL( hasEnchant( bool ) ),
                    m_itemSetupEnchantBox,   SLOT( setVisible( bool ) ) );

  QObject::connect( m_itemSetupRandomSuffixModel, SIGNAL( hasSuffixGroup( bool ) ),
                    m_itemSetupRandomSuffix, SLOT( setVisible( bool ) ) );

  QObject::connect( m_itemSetupRandomSuffixModel, SIGNAL( suffixSelected( int ) ),
                    m_itemSetupRandomSuffixView, SLOT( setCurrentIndex( int ) ) );

  // Add to main tab

  addTab( m_itemSearch, "Search" );
  addTab( m_itemFilter, "Filter" );
  addTab( m_itemSetup, "Setup" );

  setMaximumWidth( 400 );

  QObject::connect( m_itemSearchView,  SIGNAL( clicked( const QModelIndex& ) ),
                    profile,           SLOT( setSelectedItem( const QModelIndex& ) ) );

  QObject::connect( profile,           SIGNAL( slotChanged( slot_e ) ),
                    m_itemSearchProxy, SLOT( setSlot( slot_e ) ) );

  QObject::connect( m_itemSetupEnchantView, SIGNAL( currentIndexChanged( int ) ),
                   profile,           SLOT( setSelectedEnchant( int ) ) );

  QObject::connect( m_itemSetupRandomSuffixView, SIGNAL( currentIndexChanged( int ) ),
                   profile,          SLOT( setSelectedSuffix( int ) ) );
}

QSize
ItemSelectionWidget::sizeHint() const
{
  return QSize( 400, 250 );
}

QString
RandomSuffixDataModel::randomSuffixStatsStr( const random_suffix_data_t& suffix ) const
{
  const item_data_t* item = m_profile -> slotItem( m_profile -> currentSlot() );
  int f = item_database::random_suffix_type( item );
  dbc_t dbc( false );

  const random_prop_data_t& ilevel_data   = dbc.random_property( item -> level );
  const random_suffix_data_t& suffix_data = dbc.random_suffix( suffix.id );
  assert( f != -1 && ilevel_data.ilevel > 0 && suffix_data.id > 0 );
  unsigned enchant_id;
  double stat_amount;
  QStringList stat_list;

  for ( int i = 0; i < 5; i++ )
  {
    if ( ! ( enchant_id = suffix_data.enchant_id[ i ] ) )
      continue;

    const item_enchantment_data_t& enchant_data = dbc.item_enchantment( enchant_id );

    if ( ! enchant_data.id )
      continue;

    // Calculate amount of points
    if ( item -> quality == 4 ) // Epic
      stat_amount = ilevel_data.p_epic[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
    else if ( item -> quality == 3 ) // Rare
      stat_amount = ilevel_data.p_rare[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
    else if ( item -> quality == 2 ) // Uncommon
      stat_amount = ilevel_data.p_uncommon[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
    else // Impossible choices, so bogus data, skip
      continue;

    // Loop through enchantment stats, adding valid ones to the stats of the item.
    // Typically (and for cata random suffixes), there seems to be only one stat per enchantment
    for ( int j = 0; j < 3; j++ )
    {
      if ( enchant_data.ench_type[ j ] != ITEM_ENCHANTMENT_STAT ) continue;

      stat_e stat = util::translate_item_mod( (item_mod_type ) enchant_data.ench_prop[ j ] );

      if ( stat == STAT_NONE ) continue;

      std::string stat_str = util::stat_type_abbrev( stat );
      char statbuf[32];
      snprintf( statbuf, sizeof( statbuf ), "+%d%s", static_cast< int >( stat_amount ), stat_str.c_str() );
      stat_list.push_back( statbuf );
    }
  }

  return stat_list.join( ", " );
}

RandomSuffixDataModel::RandomSuffixDataModel( PaperdollProfile* profile, QObject* parent ) :
  QAbstractListModel( parent ), m_profile( profile )
{
  QObject::connect( profile, SIGNAL( slotChanged( slot_e ) ),
                   this, SLOT( stateChanged() ) );
  QObject::connect( profile, SIGNAL( itemChanged( slot_e, const item_data_t* ) ),
                   this, SLOT( stateChanged() ) );
}

void
RandomSuffixDataModel::stateChanged()
{
  beginResetModel();
  endResetModel();

  unsigned currentSuffix = m_profile -> slotSuffix( m_profile -> currentSlot() );

  if ( currentSuffix == 0 )
  {
    emit hasSuffixGroup( rowCount( QModelIndex() ) );
    return;
  }

  for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
  {
    const QModelIndex& idx = index( i, 0 );
    unsigned s = idx.data( Qt::UserRole ).value< unsigned >();
    if ( currentSuffix == s )
    {
      emit suffixSelected( idx.row() );
      break;
    }
  }

  emit hasSuffixGroup( rowCount( QModelIndex() ) );
}

int
RandomSuffixDataModel::rowCount( const QModelIndex& ) const
{
  if ( m_profile -> currentSlot() == SLOT_INVALID )
    return 0;

  const item_data_t* item = m_profile -> slotItem( m_profile -> currentSlot() );

  if ( ! item )
    return 0;

  if ( item -> id_suffix_group == 0 )
    return 0;

  for ( int i = 0; i < RAND_SUFFIX_GROUP_SIZE; i++ )
  {
    if ( static_cast<int>( __rand_suffix_group_data[ i ].id ) == item -> id_suffix_group )
    {
      int count = 0;
      while ( __rand_suffix_group_data[ i ].suffix_id[ count ] )
        count++;
      return count;
    }
  }

  return 0;
}

QVariant
RandomSuffixDataModel::data( const QModelIndex& index, int role ) const
{
  const item_data_t* item = m_profile -> slotItem( m_profile -> currentSlot() );
  dbc_t dbc( false );

  for ( int i = 0; i < RAND_SUFFIX_GROUP_SIZE; i++ )
  {
    if ( static_cast<int>( __rand_suffix_group_data[ i ].id ) == item -> id_suffix_group )
    {
      const random_suffix_data_t& rs = dbc.random_suffix( __rand_suffix_group_data[ i ].suffix_id[ index.row() ] );

      if ( role == Qt::DisplayRole )
        return QVariant( QString( "%1 (%2)" ).arg( rs.suffix ).arg( randomSuffixStatsStr( rs ) ) );
      else if ( role == Qt::UserRole )
        return QVariant( rs.id );
    }
  }

  return QVariant( QVariant::Invalid );
}

QString
EnchantDataModel::enchantStatsStr( const item_enchantment_data_t* enchant )
{
  assert( enchant );

  QStringList stats;

  for ( int i = 0; i < 3; i++ )
  {
    if ( enchant -> ench_type[ i ] != ITEM_ENCHANTMENT_STAT )
      continue;

    stat_e t = util::translate_item_mod( ( item_mod_type ) enchant -> ench_prop[ i ] );
    if ( t == STAT_NONE )
      continue;

    stats << QString( "+%1 %2" )
      .arg( enchant -> ench_amount[ i ] )
      .arg( util::stat_type_abbrev( t ) );

  }

  return stats.join( ", " );
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
        if ( spell -> effectN( 1 ).type() == E_ENCHANT_ITEM )
        {
          const item_enchantment_data_t* item_enchant = &( dbc.item_enchantment( spell -> effectN( 1 ).misc_value1() ) );

          EnchantData ed;
          ed.item = item;
          ed.enchant = spell;
          ed.item_enchant = item_enchant;

          if ( range::find( m_enchants, ed ) == m_enchants.end() )
            m_enchants.push_back( ed );

          break;
        }
      }
    }

    item++;
  }

  for ( spell_data_t* s = spell_data_t::list( dbc.ptr ); s -> id(); s++ )
  {
    if ( s -> effectN( 1 ).type() == E_ENCHANT_ITEM )
    {
      const item_enchantment_data_t* item_enchant = &( dbc.item_enchantment( s -> effectN( 1 ).misc_value1() ) );

      EnchantData ed;
      ed.item = 0;
      ed.enchant = s;
      ed.item_enchant = item_enchant;

      if ( range::find( m_enchants, ed ) == m_enchants.end() )
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
    QString enchant_stats_str = enchantStatsStr( m_enchants[ index.row() ].item_enchant );
    if ( enchant_stats_str.size() )
      enchant_stats_str = " (" + enchant_stats_str + ")";

    if ( m_enchants[ index.row() ].item )
      return QVariant( QString( "%1%2" ).arg( m_enchants[ index.row() ].item -> name ).arg( enchant_stats_str ) );
    else
      return QVariant( QString( "%1%2" ).arg( m_enchants[ index.row() ].enchant -> name_cstr() ).arg( enchant_stats_str ) );
  }
  else if ( role == Qt::UserRole )
    return QVariant::fromValue( m_enchants[ index.row() ] );
  else if ( role == Qt::DecorationRole )
  {
    const EnchantData& data =  m_enchants[ index.row() ];
    if ( data.item )
      return QVariant( getPaperdollPixmap( data.item -> icon, true ).scaled( 16, 16 ) );
    else
      return QVariant( getPaperdollPixmap( data.enchant -> _icon, true ).scaled( 16, 16 ) );

    return QVariant( QVariant::Invalid );
  }
  else if ( role == Qt::ToolTipRole )
    return QVariant( enchantStatsStr( m_enchants[ index.row() ].item_enchant ) );

  return QVariant( QVariant::Invalid );
}

EnchantFilterProxyModel::EnchantFilterProxyModel( PaperdollProfile* profile, QWidget* parent ) :
  QSortFilterProxyModel( parent ), m_profile( profile )
{
  QObject::connect( profile, SIGNAL( slotChanged( slot_e ) ),
                    this,    SLOT( stateChanged() ) );

  QObject::connect( profile, SIGNAL( itemChanged( slot_e, const item_data_t* ) ),
                    this,    SLOT( stateChanged() ) );

  QObject::connect( profile, SIGNAL( professionChanged( profession_e ) ),
                    this,    SLOT( stateChanged() ) );
}

void
EnchantFilterProxyModel::stateChanged()
{
  invalidate();
  sort( 0 );

  EnchantData currentEnchant = m_profile -> slotEnchant( m_profile -> currentSlot() );

  if ( ! currentEnchant.enchant )
  {
    emit hasEnchant( rowCount( QModelIndex() ) );
    return;
  }

  for ( int i = 0; i < rowCount(); i++ )
  {
    const QModelIndex& idx = index( i, 0 );
    EnchantData e = idx.data( Qt::UserRole ).value< EnchantData >();
    if ( currentEnchant == e )
    {
      emit enchantSelected( idx.row() );
      break;
    }
  }

  emit hasEnchant( rowCount( QModelIndex() ) );
}

bool
EnchantFilterProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
  const EnchantData e_left  = left.data( Qt::UserRole ).value< EnchantData >(),
                    e_right = right.data( Qt::UserRole ).value< EnchantData >();

  return QString::compare( e_left.item ? e_left.item -> name : e_left.enchant -> name_cstr(),
                           e_right.item ? e_right.item -> name : e_right.enchant -> name_cstr() ) < 0;
}

bool
EnchantFilterProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
  const QModelIndex& idx  = sourceModel() -> index( sourceRow, 0, sourceParent );
  const EnchantData&   ed = idx.data( Qt::UserRole ).value< EnchantData >();

  // Slot based filtering then.
  slot_e          slot = m_profile -> currentSlot();
  const item_data_t* item = m_profile -> slotItem( slot );

  if ( slot == SLOT_INVALID || ! item )
    return false;

  return m_profile -> enchantUsableByProfile( ed );
}

PaperdollBasicButton::PaperdollBasicButton( PaperdollProfile* profile, QWidget* parent ) :
  QAbstractButton( parent ), m_profile( profile )
{
  setCheckable( true );
  setAutoExclusive( true );
}

void
PaperdollBasicButton::paintEvent( QPaintEvent* event )
{
  QPainter paint;

  paint.begin( this );

  if ( isEnabled() )
  {
    paint.drawPixmap( event -> rect(), m_icon.scaled( sizeHint() ) );
  }
  // Disabled, grayscale
  else
  {
    QImage image = m_icon.toImage();
    QRgb col;
    int gray;
    int width = m_icon.width();
    int height = m_icon.height();
    for ( int i = 0; i < width; i++ )
    {
      for ( int j = 0; j < height; j++ )
      {
        col = image.pixel( i, j );
        gray = qGray( col );
        image.setPixel( i, j, qRgba( gray, gray, gray, qAlpha( col ) ) );
      }
    }
    paint.drawImage( event -> rect(), image );
  }

  // Selection border
  if ( isChecked() )
    paint.drawPixmap( event -> rect(), getPaperdollPixmap( "selection_border", false ).scaled( sizeHint() ) );

  paint.end();
}

QString
PaperdollSlotButton::getSlotIconName( slot_e slot_id )
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

PaperdollSlotButton::PaperdollSlotButton( slot_e slot, PaperdollProfile* profile, QWidget* parent ) :
  PaperdollBasicButton( profile, parent ), m_slot( slot )
{
  m_icon = getPaperdollPixmap( PaperdollSlotButton::getSlotIconName( m_slot ), true );

  QObject::connect( this,    SIGNAL( selectedSlot( slot_e ) ),
                    profile, SLOT( setSelectedSlot( slot_e ) ) );
  QObject::connect( profile, SIGNAL( itemChanged( slot_e, const item_data_t* ) ),
                    this,    SLOT( setSlotItem( slot_e, const item_data_t* ) ) );
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
    paint.drawPixmap( event -> rect(), m_icon );
  // Item in it, paint the item icon instead
  else
    paint.drawPixmap( event -> rect(), getPaperdollPixmap( m_profile -> slotItem( m_slot ) -> icon, true ) );

  // Selection border
  if ( isChecked() )
    paint.drawPixmap( event -> rect(), getPaperdollPixmap( "selection_border" ) );

  paint.end();
}

void
PaperdollSlotButton::setSlotItem( slot_e t, const item_data_t* )
{
  if ( t == m_slot )
    update();
}

PaperdollClassButton::PaperdollClassButton( PaperdollProfile* profile, player_e t, QWidget* parent ) :
  PaperdollBasicButton( profile, parent ), m_type( t )
{
  m_icon = getPaperdollPixmap( QString( "class_%1" ).arg( util::player_type_string( t ) ), true );
}

PaperdollRaceButton::PaperdollRaceButton( PaperdollProfile* profile, race_e t, QWidget* parent ) :
  PaperdollBasicButton( profile, parent ), m_type( t )
{
  m_icon = getPaperdollPixmap( QString( "race_%1" ).arg( util::race_type_string( t ) ), true );
}

PaperdollProfessionButton::PaperdollProfessionButton( PaperdollProfile* profile, profession_e t, QWidget* parent ) :
PaperdollBasicButton( profile, parent ), m_type( t )
{
  m_icon = getPaperdollPixmap( QString( "prof_%1" ).arg( util::profession_type_string( t ) ), true );
}

PaperdollClassButtonGroup::PaperdollClassButtonGroup( PaperdollProfile* profile, QWidget* parent ) :
  QGroupBox( "Select character class", parent ), m_profile( profile )
{
  PaperdollClassButton* tmp;

  m_classButtonGroupLayout = new QHBoxLayout();
  m_classButtonGroupLayout -> setAlignment( Qt::AlignHCenter | Qt::AlignTop );
  m_classButtonGroupLayout -> setSpacing( 2 );
  m_classButtonGroupLayout -> setContentsMargins( 1, 1, 1, 1 );

  m_classButtonGroup = new QButtonGroup();

  for ( player_e i = DEATH_KNIGHT; i < PLAYER_PET; i=(player_e)((int)i+1) )
  {
    tmp = m_classButtons[ i ] = new PaperdollClassButton( profile, i, this );

    m_classButtonGroup -> addButton( tmp, i );
    m_classButtonGroupLayout -> addWidget( tmp );
  }

  QObject::connect( m_classButtonGroup, SIGNAL( buttonClicked( int ) ),
                    profile,            SLOT( setClass( int ) ) );

  QObject::connect( profile,            SIGNAL( raceChanged( race_e ) ),
                    this,               SLOT( raceSelected( race_e ) ) );

  setLayout( m_classButtonGroupLayout );
  setCheckable( false );
  setFlat( true );
  setAlignment( Qt::AlignHCenter );
}

void
PaperdollClassButtonGroup::raceSelected( race_e t )
{
  assert( t > RACE_ELEMENTAL && t < RACE_MAX && t != RACE_PANDAREN );

  for ( player_e i = DEATH_KNIGHT; i < PLAYER_PET; i=(player_e)((int)i+1) )
    m_classButtons[ i ] -> setEnabled( false );

  for ( int i = DEATH_KNIGHT; i < PLAYER_PET; i++ )
  {
    for ( int j = 0; j < RACE_MAX - RACE_NIGHT_ELF; j++ )
    {
      if ( raceCombinations[ i ][ j ] == t )
      {
        m_classButtons[ i ] -> setEnabled( true );
        break;
      }
    }
  }
}

PaperdollRaceButtonGroup::PaperdollRaceButtonGroup( PaperdollProfile* profile, QWidget* parent ) :
  QGroupBox( "Select character race", parent ), m_profile( profile )
{
  PaperdollRaceButton* tmp;
  const char* faction_str[2] = {
    "alliance",
    "horde"
  };

  m_factionLayout = new QVBoxLayout();
  m_factionLayout -> setSpacing( 2 );
  m_factionLayout -> setContentsMargins( 1, 1, 1, 1 );

  m_raceButtonGroup = new QButtonGroup();
  for ( int faction = 0; faction < 2; faction++ )
  {
    m_raceButtonGroupLayout[ faction ] = new QHBoxLayout();
    m_raceButtonGroupLayout[ faction ] -> setAlignment( Qt::AlignHCenter | Qt::AlignTop );
    m_raceButtonGroupLayout[ faction ] -> setSpacing( 2 );
    m_raceButtonGroupLayout[ faction ] -> setContentsMargins( 0, 0, 0, 0 );
    m_factionLayout -> addLayout( m_raceButtonGroupLayout[ faction ] );

    m_factionLabel[ faction ] = new QLabel( this );
    m_factionLabel[ faction ] -> setPixmap( getPaperdollPixmap( QString( "%1" ).arg( faction_str[ faction ] ), true ).scaled( 32, 32 ) );
    m_raceButtonGroupLayout[ faction ] -> addWidget( m_factionLabel[ faction ] );

    for ( int race = 0; race < 7; race++ )
    {
      tmp = m_raceButtons[ raceButtonOrder[ faction ][ race ] - RACE_NIGHT_ELF ] = new PaperdollRaceButton( profile, raceButtonOrder[ faction ][ race ], this );
      m_raceButtonGroup -> addButton( tmp, raceButtonOrder[ faction ][ race ] );
      m_raceButtonGroupLayout[ faction ] -> addWidget( tmp );
    }
  }

  setLayout( m_factionLayout );
  setCheckable( false );
  setFlat( true );
  setAlignment( Qt::AlignHCenter );

  QObject::connect( m_raceButtonGroup, SIGNAL( buttonClicked( int ) ),
                    profile,           SLOT( setRace( int ) ) );

  QObject::connect( profile,            SIGNAL( classChanged( player_e ) ),
                    this,               SLOT( classSelected( player_e ) ) );
}

void
PaperdollRaceButtonGroup::classSelected( player_e t )
{
  assert( t < PLAYER_PET && t > PLAYER_NONE );

  for ( race_e race = RACE_NIGHT_ELF; race < RACE_MAX; ++race )
  {
    if ( race == RACE_PANDAREN ) continue;
    m_raceButtons[ race - RACE_NIGHT_ELF ] -> setEnabled( false );
  }

  for ( int i = 0; i < RACE_MAX - RACE_NIGHT_ELF; i++ )
  {
    for ( int j = 0; j < PLAYER_PET; j++ )
    {
      if ( classCombinations[ i ][ j ] == t )
      {
        m_raceButtons[ i ] -> setEnabled( true );
        break;
      }
    }
  }
}

PaperdollProfessionButtonGroup::PaperdollProfessionButtonGroup( PaperdollProfile* profile, QWidget* parent ) :
  QGroupBox( "Select character professions", parent )
{
  PaperdollProfessionButton* tmp;

  m_professionsLayout = new QVBoxLayout();
  m_professionsLayout -> setSpacing( 2 );
  m_professionsLayout -> setContentsMargins( 1, 1, 1, 1 );

  for ( int profession = 0; profession < 2; profession++ )
  {
    m_professionGroup[ profession ] = new QButtonGroup();
    m_professionLayout[ profession ] = new QHBoxLayout();
    m_professionLayout[ profession ] -> setAlignment( Qt::AlignHCenter | Qt::AlignTop );
    m_professionLayout[ profession ] -> setSpacing( 2 );
    m_professionLayout[ profession ] -> setContentsMargins( 0, 0, 0, 0 );

    m_professionsLayout -> addLayout( m_professionLayout[ profession ] );

    for ( profession_e p = PROF_ALCHEMY; p < PROFESSION_MAX; p=(profession_e)((int)p+1) )
    {
      tmp = m_professionButtons[ profession ][ p - PROF_ALCHEMY ] = new PaperdollProfessionButton( profile, p, this );
      m_professionGroup[ profession ] -> addButton( tmp, p );
      m_professionLayout[ profession ] -> addWidget( tmp );
    }

    QObject::connect( m_professionGroup[ profession ], SIGNAL( buttonClicked( int ) ),
                      this,                            SLOT( setProfession( int ) ) );
  }

  QObject::connect( this,    SIGNAL( professionSelected( int, int ) ),
                    profile, SLOT( setProfession( int, int ) ) );

  setLayout( m_professionsLayout );
  setCheckable( false );
  setFlat( true );
  setAlignment( Qt::AlignHCenter );
}

void
PaperdollProfessionButtonGroup::setProfession( int newValue )
{
  QButtonGroup* sender = qobject_cast< QButtonGroup* >( QObject::sender() );

  for ( int i = 0; i < 2; i ++ )
  {
    if ( m_professionGroup[ i ] == sender )
      continue;

    // Enable all first
    for ( int j = 0; j < PROFESSION_MAX - 1; j++ )
      m_professionButtons[ i ][ j ] -> setEnabled( true );

    m_professionButtons[ i ][ newValue - PROF_ALCHEMY ] -> setEnabled( false );

    emit professionSelected( i, newValue );
    break;
  }
}

Paperdoll::Paperdoll( PaperdollProfile* profile, QWidget* parent ) :
  QWidget( parent ), m_profile( profile )
{
  m_layout = new QGridLayout();
  m_layout -> setSpacing( 2 );
  m_layout -> setContentsMargins( 3, 3, 3, 3 );
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

  m_baseSelectorLayout = new QVBoxLayout();

  m_classGroup = new PaperdollClassButtonGroup( profile, this );
  m_raceGroup = new PaperdollRaceButtonGroup( profile, this );
  m_professionGroup = new PaperdollProfessionButtonGroup( profile, this );

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

  m_layout -> addLayout( m_baseSelectorLayout, 0, 1, 3, 3 );

  m_baseSelectorLayout -> addWidget( m_classGroup, Qt::AlignLeft | Qt::AlignTop );
  m_baseSelectorLayout -> addWidget( m_raceGroup, Qt::AlignLeft | Qt::AlignTop );
  m_baseSelectorLayout -> addWidget( m_professionGroup, Qt::AlignLeft | Qt::AlignTop );

  setLayout( m_layout );
}

QSize
Paperdoll::sizeHint() const
{
  return QSize( 400, 616 );
}

const int ItemFilterProxyModel::professionIds[ PROFESSION_MAX ] = {
  0,
  171,  // Alchemy
  186,  // Mining
  182,  // Herbalism
  165,  // Leatherworking
  202,  // Engineering
  164,  // Blacksmithing
  773,  // Inscription
  393,  // Skinning
  197,  // Tailoring,
  755,  // Jewelcrafting
  333,  // Enchanting
};

const race_e PaperdollRaceButtonGroup::raceButtonOrder[ 2 ][ 7 ] = {
  { RACE_NIGHT_ELF, RACE_HUMAN, RACE_GNOME, RACE_DWARF, RACE_DRAENEI, RACE_WORGEN, RACE_PANDAREN_ALLIANCE },
  { RACE_ORC, RACE_TROLL, RACE_UNDEAD, RACE_BLOOD_ELF, RACE_TAUREN, RACE_GOBLIN, RACE_PANDAREN_HORDE }
};

const player_e PaperdollRaceButtonGroup::classCombinations[ 15 ][ 12 ] = {
  // Night Elf
  { DEATH_KNIGHT, DRUID, HUNTER, MAGE,          PRIEST, ROGUE,                  WARRIOR, MONK },
  // Human
  { DEATH_KNIGHT,        HUNTER, MAGE, PALADIN, PRIEST, ROGUE,         WARLOCK, WARRIOR, MONK },
  // Gnome
  { DEATH_KNIGHT,                MAGE,          PRIEST, ROGUE,         WARLOCK, WARRIOR, MONK },
  // Dwarf
  { DEATH_KNIGHT,        HUNTER,       PALADIN, PRIEST, ROGUE, SHAMAN,          WARRIOR, MONK },
  // Draenei
  { DEATH_KNIGHT,        HUNTER, MAGE, PALADIN, PRIEST,        SHAMAN,          WARRIOR, MONK },
  // Worgen
  { DEATH_KNIGHT, DRUID, HUNTER, MAGE,          PRIEST, ROGUE,         WARLOCK, WARRIOR },
  // Orc
  { DEATH_KNIGHT,        HUNTER, MAGE,                  ROGUE, SHAMAN, WARLOCK, WARRIOR, MONK },
  // Troll
  { DEATH_KNIGHT, DRUID, HUNTER, MAGE,          PRIEST, ROGUE, SHAMAN, WARLOCK, WARRIOR, MONK },
  // Undead
  { DEATH_KNIGHT,        HUNTER, MAGE,          PRIEST, ROGUE,         WARLOCK, WARRIOR, MONK },
  // Blood Elf
  { DEATH_KNIGHT,        HUNTER, MAGE, PALADIN, PRIEST, ROGUE,         WARLOCK, WARRIOR, MONK },
  // Tauren
  { DEATH_KNIGHT, DRUID, HUNTER,       PALADIN, PRIEST,        SHAMAN,          WARRIOR, MONK },
  // Goblin
  { DEATH_KNIGHT,        HUNTER, MAGE,          PRIEST, ROGUE, SHAMAN, WARLOCK, WARRIOR },
  // Pandaren
  {                                                                                           },
  // Pandaren Alliance
  {                      HUNTER, MAGE,          PRIEST, ROGUE, SHAMAN,          WARRIOR, MONK },
  // Pandaren Horde
  {                      HUNTER, MAGE,          PRIEST, ROGUE, SHAMAN,          WARRIOR, MONK },
};

// Note note this is in simulationcraft internal order, not the order of the game client
const race_e PaperdollClassButtonGroup::raceCombinations[ 12 ][ 15 ] = {
  { RACE_NONE },
  // Death Knight
  { RACE_HUMAN, RACE_ORC, RACE_DWARF, RACE_NIGHT_ELF, RACE_UNDEAD, RACE_TAUREN, RACE_GNOME, RACE_TROLL, RACE_GOBLIN, RACE_BLOOD_ELF, RACE_DRAENEI, RACE_WORGEN },
  // Druid
  {                                   RACE_NIGHT_ELF,              RACE_TAUREN,             RACE_TROLL,                                            RACE_WORGEN },
  // Hunter
  { RACE_HUMAN, RACE_ORC, RACE_DWARF, RACE_NIGHT_ELF, RACE_UNDEAD, RACE_TAUREN,             RACE_TROLL, RACE_GOBLIN, RACE_BLOOD_ELF, RACE_DRAENEI, RACE_WORGEN, RACE_PANDAREN, RACE_PANDAREN_ALLIANCE, RACE_PANDAREN_HORDE },
  // Mage
  { RACE_HUMAN, RACE_ORC, RACE_DWARF, RACE_NIGHT_ELF, RACE_UNDEAD,              RACE_GNOME, RACE_TROLL, RACE_GOBLIN, RACE_BLOOD_ELF, RACE_DRAENEI, RACE_WORGEN, RACE_PANDAREN, RACE_PANDAREN_ALLIANCE, RACE_PANDAREN_HORDE },
  // Monk
  { RACE_HUMAN, RACE_ORC, RACE_DWARF, RACE_NIGHT_ELF, RACE_UNDEAD, RACE_TAUREN, RACE_GNOME, RACE_TROLL,              RACE_BLOOD_ELF, RACE_DRAENEI,              RACE_PANDAREN, RACE_PANDAREN_ALLIANCE, RACE_PANDAREN_HORDE },
  // Paladin
  { RACE_HUMAN,           RACE_DWARF,                              RACE_TAUREN,                                      RACE_BLOOD_ELF, RACE_DRAENEI,             },
  // Priest
  { RACE_HUMAN,           RACE_DWARF, RACE_NIGHT_ELF, RACE_UNDEAD, RACE_TAUREN, RACE_GNOME, RACE_TROLL, RACE_GOBLIN, RACE_BLOOD_ELF, RACE_DRAENEI, RACE_WORGEN, RACE_PANDAREN, RACE_PANDAREN_ALLIANCE, RACE_PANDAREN_HORDE },
  // Rogue
  { RACE_HUMAN, RACE_ORC, RACE_DWARF, RACE_NIGHT_ELF, RACE_UNDEAD,              RACE_GNOME, RACE_TROLL, RACE_GOBLIN, RACE_BLOOD_ELF,               RACE_WORGEN, RACE_PANDAREN, RACE_PANDAREN_ALLIANCE, RACE_PANDAREN_HORDE },
  // Shaman
  {             RACE_ORC, RACE_DWARF,                              RACE_TAUREN,             RACE_TROLL, RACE_GOBLIN,                 RACE_DRAENEI,              RACE_PANDAREN, RACE_PANDAREN_ALLIANCE, RACE_PANDAREN_HORDE },
  // Warlock
  { RACE_HUMAN, RACE_ORC, RACE_DWARF,                 RACE_UNDEAD,              RACE_GNOME, RACE_TROLL, RACE_GOBLIN, RACE_BLOOD_ELF,               RACE_WORGEN },
  // Warrior
  { RACE_HUMAN, RACE_ORC, RACE_DWARF, RACE_NIGHT_ELF, RACE_UNDEAD, RACE_TAUREN, RACE_GNOME, RACE_TROLL, RACE_GOBLIN, RACE_BLOOD_ELF, RACE_DRAENEI, RACE_WORGEN, RACE_PANDAREN, RACE_PANDAREN_ALLIANCE, RACE_PANDAREN_HORDE },
};


