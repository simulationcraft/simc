// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMCPAPERDOLL_H
#define SIMCPAPERDOLL_H

#include <QSortFilterProxyModel>
#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QCheckBox>
#include <QComboBox>
#include <QPainter>
#include <QGridLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListView>
#include <QLineEdit>
#include <QTabWidget>
#include <QGroupBox>
#include <QAbstractButton>
#include <QPaintEvent>
#include <QMetaType>
#include <QButtonGroup>
#include <QPushButton>

#include <vector>

#include "simulationcraft.h"

class Paperdoll;
class EnchantDataModel;
class EnchantFilterProxyModel;
class RandomSuffixDataModel;

class EnchantData
{
public:
  const item_data_t*  item;
  const spell_data_t* enchant;
  const item_enchantment_data_t* item_enchant;
  inline bool operator==( const EnchantData& other ) { return enchant == other.enchant; }
};
Q_DECLARE_METATYPE( EnchantData );

QPixmap getPaperdollPixmap( const QString& name, bool = false, QSize = QSize( 64, 64 ) );

#if 0
class PaperdollPixmap : public QPixmap
{
public:
  static QPixmap get( const QString&, bool = false, QSize = QSize( 64, 64 ) );
  static const QString& resourcePath( void );

  PaperdollPixmap();
  PaperdollPixmap( const QString&, bool = false, QSize = QSize( 64, 64 ) );
private:
  static QString rpath;
};
#endif

// Profile state + signaling, all state manipulation goes through this
// class to have some sanity in what signals bounce, and where
class PaperdollProfile : public QObject
{
  Q_OBJECT
public:
  PaperdollProfile();

  bool enchantUsableByProfile( const EnchantData& ) const;

  bool itemUsableByProfile( const item_data_t* ) const;
  bool itemUsableByClass( const item_data_t*, bool = true ) const;
  bool itemUsableByRace( const item_data_t* ) const;
  bool itemUsableByProfession( const item_data_t* ) const;
  bool itemFitsProfileSlot( const item_data_t* ) const;

  inline const item_data_t* slotItem( slot_type t ) const { return m_slotItem[ t ]; }
  inline unsigned           slotSuffix( slot_type t ) const { return m_slotSuffix[ t ]; }
  inline const EnchantData& slotEnchant( slot_type t ) const { return m_slotEnchant[ t ]; }
  inline slot_type          currentSlot( void ) const     { return m_currentSlot; }
  inline player_type        currentClass( void ) const { return m_class; }
  inline race_type          currentRace( void ) const { return m_race; }
  inline profession_type    currentProfession( unsigned p ) const { assert( p < 2 ); return m_professions[ p ]; }

public slots:
  void setSelectedSlot( slot_type );
  void setSelectedItem( const QModelIndex& );
  void setSelectedEnchant( int );
  void setSelectedSuffix( int );
  void setClass( int );
  void setRace( int );
  void setProfession( int, int );

  void validateSlot( slot_type t );
  bool clearSlot( slot_type t );

signals:
  void slotChanged( slot_type );
  void itemChanged( slot_type, const item_data_t* );
  void enchantChanged( slot_type, const EnchantData& );
  void suffixChanged( slot_type, unsigned );
  void classChanged( player_type );
  void raceChanged( race_type );
  void professionChanged( profession_type );

  void profileChanged();

private:
  player_type        m_class;
  race_type          m_race;
  profession_type    m_professions[ 2 ];
  slot_type          m_currentSlot;          // Currently selected paperdoll slot
  const item_data_t* m_slotItem[ SLOT_MAX ]; // Currently selected item in a slot
  unsigned           m_slotSuffix[ SLOT_MAX ];
  EnchantData        m_slotEnchant[ SLOT_MAX ]; // Currently selected enchants in a slot;
};

class ItemFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  static const int professionIds[ PROFESSION_MAX ];
  ItemFilterProxyModel( PaperdollProfile*, QObject* = 0 );

  inline int minIlevel( void ) { return m_minIlevel; }
  inline int maxIlevel( void ) { return m_maxIlevel; }

public slots:
  void setMinIlevel( int );
  void setMaxIlevel( int );
  void setSlot( slot_type );
  void setMatchArmor( int );
  void SearchTextChanged( const QString& );

  void filterChanged() { invalidate(); sort( 0 ); }

signals:
  void itemSelected( const QModelIndex& );

protected:
  bool filterAcceptsRow( int, const QModelIndex& ) const;
  bool lessThan( const QModelIndex&, const QModelIndex& ) const;

private:
  bool filterByName( const item_data_t* ) const;
  bool itemUsedByClass( const item_data_t* ) const;
  int  primaryStat( void ) const;

  PaperdollProfile* m_profile;
  bool    m_matchArmor;
  int     m_minIlevel;
  int     m_maxIlevel;
  quint32 m_race;
  QString m_searchText;
};

class ItemDataListModel : public QAbstractListModel
{
  Q_OBJECT
public:
  ItemDataListModel( QObject* = 0 );

  int rowCount( const QModelIndex& ) const;
  QVariant data( const QModelIndex& = QModelIndex(), int = Qt::DisplayRole ) const;
public slots:
  void dataSourceChanged( int );
private:
};

class ItemDataDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  static QString itemQualityColor( const item_data_t* );
  static QString itemFlagStr( const item_data_t* );

  ItemDataDelegate( QObject* = 0 );

  QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex& ) const;
  void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex& ) const;
private:
  QString itemStatsString( const item_data_t*, bool = false ) const;
};

class ItemSelectionWidget : public QTabWidget
{
  Q_OBJECT
public:
  ItemSelectionWidget( PaperdollProfile*, QWidget* = 0 );

  QSize sizeHint() const;
private:
  PaperdollProfile*     m_profile;

  // Item search widget in first tab
  QWidget*              m_itemSearch;
  QVBoxLayout*          m_itemSearchLayout;

  QLineEdit*            m_itemSearchInput;
  QListView*            m_itemSearchView;

  ItemFilterProxyModel* m_itemSearchProxy;
  ItemDataListModel*    m_itemSearchModel;
  ItemDataDelegate*     m_itemSearchDelegate;

  // Item filtering rules in the second tab
  QWidget*              m_itemFilter;
  QFormLayout*          m_itemFilterLayout;

  QCheckBox*            m_itemFilterMatchArmor;

  QHBoxLayout*          m_itemFilterMinIlevelLayout;
  QSlider*              m_itemFilterMinIlevel;
  QLabel*               m_itemFilterMinIlevelLabel;

  QHBoxLayout*          m_itemFilterMaxIlevelLayout;
  QSlider*              m_itemFilterMaxIlevel;
  QLabel*               m_itemFilterMaxIlevelLabel;

  // Item enchant / reforge / gems / random suffix setup in the third tab
  QWidget*              m_itemSetup;
  QVBoxLayout*          m_itemSetupLayout;

  QGroupBox*            m_itemSetupRandomSuffix;
  QHBoxLayout*          m_itemSetupRandomSuffixLayout;

  QComboBox*            m_itemSetupRandomSuffixView;
  RandomSuffixDataModel* m_itemSetupRandomSuffixModel;

  QGroupBox*            m_itemSetupEnchantBox;
  QHBoxLayout*          m_itemSetupEnchantBoxLayout;

  QComboBox*            m_itemSetupEnchantView;
  EnchantDataModel*     m_itemSetupEnchantModel;
  EnchantFilterProxyModel* m_itemSetupEnchantProxy;

};

class RandomSuffixDataModel : public QAbstractListModel
{
  Q_OBJECT
public:
  RandomSuffixDataModel( PaperdollProfile*, QObject* = 0 );

  int rowCount( const QModelIndex& ) const;
  QVariant data( const QModelIndex& = QModelIndex(), int = Qt::DisplayRole ) const;
signals:
  void hasSuffixGroup( bool );
  void suffixSelected( int );
public slots:
  void stateChanged();
private:
  QString randomSuffixStatsStr( const random_suffix_data_t& ) const;

  PaperdollProfile* m_profile;
};

class EnchantDataModel : public QAbstractListModel
{
  Q_OBJECT
public:
  static QString enchantStatsStr( const item_enchantment_data_t* );

  EnchantDataModel( PaperdollProfile*, QObject* = 0 );

  int rowCount( const QModelIndex& ) const;
  QVariant data( const QModelIndex& = QModelIndex(), int = Qt::DisplayRole ) const;
protected:
private:
  PaperdollProfile* m_profile;

  // Construct enchant data so we dont have to access the item/spell structs all
  // the time
  std::vector< EnchantData > m_enchants;
};

class EnchantFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  EnchantFilterProxyModel( PaperdollProfile*, QWidget* = 0 );

public slots:
  void stateChanged();

signals:
  void enchantSelected( int );
  void hasEnchant( bool );

protected:
  bool filterAcceptsRow( int, const QModelIndex& ) const;
  bool lessThan( const QModelIndex&, const QModelIndex& ) const;

private:
  PaperdollProfile* m_profile;
};

class PaperdollBasicButton : public QAbstractButton
{
  Q_OBJECT
public:
  PaperdollBasicButton( PaperdollProfile*, QWidget* = 0 );

  virtual QSize sizeHint() const { return QSize( 32, 32 ); }
  virtual void paintEvent( QPaintEvent* );
protected:
  QPixmap           m_icon;
  PaperdollProfile* m_profile;
};

class PaperdollSlotButton : public PaperdollBasicButton
{
  Q_OBJECT
public:
  static QString getSlotIconName( slot_type );

  PaperdollSlotButton( slot_type, PaperdollProfile*, QWidget* = 0 );

  void  paintEvent( QPaintEvent* );
  QSize sizeHint() const { return QSize( 64, 64 ); }
  slot_type getSlot( void ) const { return m_slot; }
signals:
  void selectedSlot( slot_type );
public slots:
  void setSlotItem( slot_type, const item_data_t* );
protected:
  void mousePressEvent( QMouseEvent* );
  slot_type          m_slot;
};

class PaperdollClassButton : public PaperdollBasicButton
{
public:
  PaperdollClassButton( PaperdollProfile*, player_type, QWidget* = 0 );
protected:
  player_type       m_type;
};

class PaperdollRaceButton : public PaperdollBasicButton
{
public:
  PaperdollRaceButton( PaperdollProfile*, race_type, QWidget* = 0 );
protected:
  race_type         m_type;
};

class PaperdollProfessionButton : public PaperdollBasicButton
{
public:
  static QString professionString( profession_type );

  PaperdollProfessionButton( PaperdollProfile*, profession_type, QWidget* = 0 );
protected:
  profession_type   m_type;
};

class PaperdollClassButtonGroup : public QGroupBox
{
  Q_OBJECT
public:
  static const race_type raceCombinations[ 11 ][ 12 ];

  PaperdollClassButtonGroup( PaperdollProfile*, QWidget* = 0 );
public slots:
  void raceSelected( race_type );
private:
  PaperdollProfile*     m_profile;
  QButtonGroup*         m_classButtonGroup;
  QHBoxLayout*          m_classButtonGroupLayout;
  PaperdollClassButton* m_classButtons[ PLAYER_PET ];
};

class PaperdollRaceButtonGroup : public QGroupBox
{
  Q_OBJECT
public:
  static const race_type   raceButtonOrder[ 2 ][ 6 ];
  static const player_type classCombinations[ 12 ][ 11 ];

  PaperdollRaceButtonGroup( PaperdollProfile*, QWidget* = 0 );
public slots:
  void classSelected( player_type );
private:
  PaperdollProfile*     m_profile;
  QButtonGroup*         m_raceButtonGroup;
  QVBoxLayout*          m_factionLayout;
  QHBoxLayout*          m_raceButtonGroupLayout[ 2 ];
  QLabel*               m_factionLabel[ 2 ];
  PaperdollRaceButton*  m_raceButtons[ 12 ];
};

class PaperdollProfessionButtonGroup : public QGroupBox
{
  Q_OBJECT
public:
  PaperdollProfessionButtonGroup( PaperdollProfile*, QWidget* = 0 );
signals:
  void professionSelected( int, int );
private slots:
  void setProfession( int );
private:
  PaperdollProfile*          m_profile;
  QButtonGroup*              m_professionGroup[ 2 ];
  QHBoxLayout*               m_professionLayout[ 2 ];
  QVBoxLayout*               m_professionsLayout;
  PaperdollProfessionButton* m_professionButtons[ 2 ][ 11 ];
};

class Paperdoll : public QWidget
{
  Q_OBJECT
public:
  Paperdoll( PaperdollProfile*, QWidget* = 0 );
  QSize sizeHint() const;
protected:
private:
  QGridLayout*         m_layout;
  QVBoxLayout*         m_baseSelectorLayout;
  PaperdollClassButtonGroup* m_classGroup;
  PaperdollRaceButtonGroup*  m_raceGroup;
  PaperdollProfessionButtonGroup* m_professionGroup;
  PaperdollSlotButton* m_slotWidgets[ SLOT_MAX ];
  PaperdollProfile*    m_profile;
};

#endif
