// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMCPAPERDOLL_H
#define SIMCPAPERDOLL_H
#ifndef SC_PAPERDOLL
#define SC_PAPERDOLL
#endif

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"

#include <QSortFilterProxyModel>
#include <QAbstractListModel>
#ifdef QT_VERSION_5
#include <QtWidgets/QtWidgets>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QButtonGroup>
#else
#include <QStyledItemDelegate>
#include <QCheckBox>
#include <QComboBox>
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
#include <QPushButton>
#include <QButtonGroup>
#endif
#include <QPainter>
#include <QPaintEvent>
#include <QMetaType>

#include <vector>

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

  inline const item_data_t* slotItem( slot_e t ) const { return m_slotItem[ t ]; }
  inline unsigned           slotSuffix( slot_e t ) const { return m_slotSuffix[ t ]; }
  inline const EnchantData& slotEnchant( slot_e t ) const { return m_slotEnchant[ t ]; }
  inline slot_e          currentSlot( void ) const     { return m_currentSlot; }
  inline player_e        currentClass( void ) const { return m_class; }
  inline race_e          currentRace( void ) const { return m_race; }
  inline specialization_e currentSpec( void ) const { return m_spec; }
  inline profession_e    currentProfession( unsigned p ) const { assert( p < 2 ); return m_professions[ p ]; }

public slots:
  void setSelectedSlot( slot_e );
  void setSelectedItem( const QModelIndex& );
  void setSelectedEnchant( int );
  void setSelectedSuffix( int );
  void setClass( int );
  void setRace( int );
  void setProfession( int, int );
  void setSpec( int );

  void validateSlot( slot_e t );
  bool clearSlot( slot_e t );

signals:
  void slotChanged( slot_e );
  void itemChanged( slot_e, const item_data_t* );
  void enchantChanged( slot_e, const EnchantData& );
  void suffixChanged( slot_e, unsigned );
  void classChanged( player_e );
  void raceChanged( race_e );
  void professionChanged( profession_e );
  void specChanged( specialization_e );

  void profileChanged();

private:
  player_e        m_class;
  race_e          m_race;
  profession_e    m_professions[ 2 ];
  slot_e          m_currentSlot;          // Currently selected paperdoll slot
  const item_data_t* m_slotItem[ SLOT_MAX ]; // Currently selected item in a slot
  unsigned           m_slotSuffix[ SLOT_MAX ];
  EnchantData        m_slotEnchant[ SLOT_MAX ]; // Currently selected enchants in a slot;
  specialization_e m_spec;
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
  void setSlot( slot_e );
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
  static QString getSlotIconName( slot_e );

  PaperdollSlotButton( slot_e, PaperdollProfile*, QWidget* = 0 );

  void  paintEvent( QPaintEvent* );
  QSize sizeHint() const { return QSize( 64, 64 ); }
  slot_e getSlot( void ) const { return m_slot; }
signals:
  void selectedSlot( slot_e );
public slots:
  void setSlotItem( slot_e, const item_data_t* );
protected:
  void mousePressEvent( QMouseEvent* );
  slot_e          m_slot;
};

class PaperdollClassButton : public PaperdollBasicButton
{
public:
  PaperdollClassButton( PaperdollProfile*, player_e, QWidget* = 0 );
protected:
  player_e       m_type;
};

class PaperdollRaceButton : public PaperdollBasicButton
{
public:
  PaperdollRaceButton( PaperdollProfile*, race_e, QWidget* = 0 );
protected:
  race_e         m_type;
};

class PaperdollProfessionButton : public PaperdollBasicButton
{
public:
  static QString professionString( profession_e );

  PaperdollProfessionButton( PaperdollProfile*, profession_e, QWidget* = 0 );
protected:
  profession_e   m_type;
};

class PaperdollSpecButton : public PaperdollBasicButton
{
public:
  PaperdollSpecButton( PaperdollProfile*, specialization_e, QWidget* = 0 );
  void set_spec( specialization_e s )
  {
      m_spec = s;
      setStatusTip( QString::fromStdString( dbc::specialization_string( s ) ) );
      m_icon = getPaperdollPixmap( QString( "spec_%1" ).arg( dbc::specialization_string( s ).c_str() ), true );
  }
protected:
  specialization_e m_spec;
};

class PaperdollClassButtonGroup : public QGroupBox
{
  Q_OBJECT
public:
  static const race_e raceCombinations[ 12 ][ 15 ];

  PaperdollClassButtonGroup( PaperdollProfile*, QWidget* = 0 );
public slots:
  void raceSelected( race_e );
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
  static const race_e   raceButtonOrder[ 2 ][ 7 ];
  static const player_e classCombinations[ 15 ][ 12 ];

  PaperdollRaceButtonGroup( PaperdollProfile*, QWidget* = 0 );
public slots:
  void classSelected( player_e );
private:
  PaperdollProfile*     m_profile;
  QButtonGroup*         m_raceButtonGroup;
  QVBoxLayout*          m_factionLayout;
  QHBoxLayout*          m_raceButtonGroupLayout[ 2 ];
  QLabel*               m_factionLabel[ 2 ];
  PaperdollRaceButton*  m_raceButtons[ 15 ];
};

class PaperdollSpecButtonGroup : public QGroupBox
{
  Q_OBJECT
public:
  PaperdollSpecButtonGroup( PaperdollProfile*, QWidget* = 0 );
public slots:
  void classSelected( player_e );
private:
  PaperdollProfile*     m_profile;
  QButtonGroup*         m_specButtonGroup;
  QHBoxLayout*          m_specButtonGroupLayout;
  std::vector<PaperdollSpecButton*>  m_specButtons;
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
  Paperdoll( SC_MainWindow*, PaperdollProfile*, QWidget* = 0 );
  QSize sizeHint() const;
  void setCurrentDPS( const std::string&, double, double );
protected:
private:
  QGridLayout*         m_layout;
  QVBoxLayout*         m_baseSelectorLayout;
  PaperdollClassButtonGroup* m_classGroup;
  PaperdollRaceButtonGroup*  m_raceGroup;
  PaperdollProfessionButtonGroup* m_professionGroup;
  PaperdollSpecButtonGroup* m_specGroup;
  PaperdollSlotButton* m_slotWidgets[ SLOT_MAX ];
  PaperdollProfile*    m_profile;
  SC_MainWindow* mainWindow;

  struct metric_t{
    QString name;
    double value, error;
  } current_metric;
  QLabel* current_dps;
};

#endif
