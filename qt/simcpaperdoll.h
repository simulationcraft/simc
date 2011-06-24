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

#include <vector>

#include "simulationcraft.h"

class Paperdoll;
class EnchantDataModel;

class PaperdollPixmap : public QPixmap
{
public:
  static QPixmap get( const QString&, bool = false, QSize = QSize( 64, 64 ) );

  PaperdollPixmap();
  PaperdollPixmap( const QString&, bool = false, QSize = QSize( 64, 64 ) );
private:
  
};

// Profile state + signaling, all state manipulation goes through this 
// class to have some sanity in what signals bounce, and where
class PaperdollProfile : public QObject
{
  Q_OBJECT
public:
  PaperdollProfile();

  inline const item_data_t* slotItem( slot_type t ) const { return m_slotItem[ t ]; }
  inline slot_type          currentSlot( void ) const     { return m_currentSlot; }

public slots:
  void setSelectedSlot( slot_type );
  void setSelectedItem( const QModelIndex& );

signals:
  void slotChanged( slot_type );
  void itemChanged( slot_type, const item_data_t* );

private:
  slot_type          m_currentSlot;          // Currently selected paperdoll slot
  const item_data_t* m_slotItem[ SLOT_MAX ]; // Currently selected item in a slot
};

class ItemFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  ItemFilterProxyModel( PaperdollProfile*, QObject* = 0 );

  inline int minIlevel( void ) { return m_minIlevel; }
  inline int maxIlevel( void ) { return m_maxIlevel; }

public slots:
  void setMinIlevel( int );
  void setMaxIlevel( int );
  void SetClass( quint32 );
  void SetRace( quint32 );
  void setSlot( slot_type );
  void SetMatchArmor( int );
  void SearchTextChanged( const QString& );

signals:
  void itemSelected( const QModelIndex& );
  
protected:  
  bool filterAcceptsRow( int, const QModelIndex& ) const;
  bool lessThan( const QModelIndex&, const QModelIndex& ) const;
  
private:
  bool itemFitsSlot( const item_data_t*, bool = false ) const;
  bool itemFitsClass( const item_data_t* ) const;
  bool filterByName( const item_data_t* ) const;
  bool itemUsedByClass( const item_data_t* ) const;
  int  primaryStat( void ) const;

  PaperdollProfile* m_profile;
  bool    m_matchArmor;
  int     m_minIlevel;
  int     m_maxIlevel;
  quint32 m_class;
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
  ItemDataDelegate( QObject* = 0 );
  
  QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex& ) const;
  void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex& ) const;
private:
  QString itemStatsString( const item_data_t* ) const;
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
  
  QGroupBox*            m_itemSetupEnchantBox;
  QHBoxLayout*          m_itemSetupEnchantBoxLayout;
  
  QComboBox*            m_itemSetupEnchantView;
  EnchantDataModel*     m_itemSetupEnchantModel;
};

struct EnchantData {
  const item_data_t*  item_enchant;
  const spell_data_t* enchant;
  inline bool operator==(const EnchantData& other) { return enchant == other.enchant; }
};
Q_DECLARE_METATYPE( EnchantData );

class EnchantDataModel : public QAbstractListModel
{
  Q_OBJECT
public:
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
/*
class EnchantFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  EnchantFilterProxyModel( PaperdollProfile*, QWidget* = 0 );

public slots:
  void setSlot( slot_type );

protected:  
  bool filterAcceptsRow( int, const QModelIndex& ) const;
  bool lessThan( const QModelIndex&, const QModelIndex& ) const;
  
private:
  PaperdollProfile* m_profile;
};
*/
class PaperdollSlotButton : public QAbstractButton
{
  Q_OBJECT
public:
  static QString getSlotIconName( slot_type );

  PaperdollSlotButton( slot_type, PaperdollProfile*, QWidget* = 0 );
  
  QSize sizeHint() const;
  void  paintEvent( QPaintEvent* );
  slot_type getSlot( void ) const { return m_slot; }
signals:
  void selectedSlot( slot_type );
public slots:
  void setSlotItem( slot_type, const item_data_t* );
protected:
  void mousePressEvent( QMouseEvent* );
private:
  slot_type          m_slot;
  QPixmap            m_slotIcon;
  PaperdollProfile*  m_profile;
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
  PaperdollSlotButton* m_slotWidgets[ SLOT_MAX ];
  PaperdollProfile*    m_profile;
};

#endif
