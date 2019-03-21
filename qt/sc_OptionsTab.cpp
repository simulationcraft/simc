// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include "sc_OptionsTab.hpp"
#include "sc_SimulateTab.hpp"
#include "util/sc_mainwindowcommandline.hpp"
#include <QtCore/QDateTime>

namespace { // unnamed namespace

struct OptionEntry
{
  const char* label;
  const char* option;
  const char* tooltip;
};

const OptionEntry itemSourceOptions[] =
{
  { "Local Item Database", "local",   "Use Simulationcraft item database" },
  { "Blizzard API",        "bcpapi",  "Remote Blizzard Community Platform API source" },
  { "Wowhead.com",         "wowhead", "Remote Wowhead.com item data source" },
#if SC_USE_PTR
  { "Wowhead.com (PTR)",   "ptrhead", "Remote Wowhead.com PTR item data source" },
#endif
#if SC_BETA
  { "Wowhead.com (Beta)",  SC_BETA_STR "head", "Remote Wowhead.com Beta item data source" },
#endif
};

QComboBox* createChoiceFromRange( int lowerInclusive, int upperInclusive ) {
  QComboBox* choice = new QComboBox();
  for ( int i = lowerInclusive; i <= upperInclusive; i++ ) {
    QString choiceText = QString::number(i);
    choice -> addItem( choiceText );
  }
  return choice;
}

QComboBox* createChoice( int count, ... )
{
  QComboBox* choice = new QComboBox();
  va_list vap;
  va_start( vap, count );
  for ( int i = 0; i < count; i++ )
    choice -> addItem( va_arg( vap, char* ) );
  va_end( vap );
  return choice;
}

void appendCheckBox( const QString& label, const QString& option, const QString& tooltip, QLayout* layout, QButtonGroup* group )
{
  QCheckBox* checkBox = new QCheckBox( label );
  checkBox->setObjectName( option );  // store key of option as object name
  checkBox -> setToolTip( tooltip );
  if (group)
    group -> addButton( checkBox );
  layout -> addWidget( checkBox );
}

QString RemoveBadFileChar( QString& filename )
{
  // Purge not allowed Characters.
  static const char notAllowedChars[] = ",^@={}[]~!?:&*\"|#%<>$\"'();`'/\\";
  if ( filename == "" )
    return filename;
  for ( size_t i = 0; i < sizeof( notAllowedChars ); i++ )
  {
    filename.replace( notAllowedChars[i] , "" );
  }
  filename.replace( "..", "" );
  filename.replace( ".html", "" );


#if defined SC_WINDOWS
  // Prevent Windows Devices
  static const char* windowsDevices = "CON|AUX|PRN|COM1|COM2|COM3|LPT1|LPT2|LPT3|NUL";
  static QRegExp regexWindowsDevice(QLatin1String(windowsDevices), Qt::CaseInsensitive);
  bool matchesWinDevice = regexWindowsDevice.exactMatch( filename );
  if ( matchesWinDevice )
    filename = "";
#endif

  // Limit filename length
  if ( filename.length() > 50 ) // The filename limit is much higher, but let's protect people from naming the file an absurdly long name and possibly hitting the 255 limit
    filename.truncate( 50 );

  return filename;
}

/* Decode all options/setting from a string ( loaded from the history ).
 * Decode / Encode order needs to be equal!
 *
 * If no default_value is specified, index 0 is used as default.
 */
void load_setting( QSettings& s, const QString& name, QComboBox* choice, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  int index = choice -> findText( v );
  if ( index != -1 )
    choice -> setCurrentIndex( index );
  else if ( !default_value.isEmpty() )
  {
    bool ok;
    v.toInt( &ok, 10 );
    if ( ok )
      choice -> setCurrentText( v );
    else
    {
      int default_index = choice -> findText( default_value );
      if ( default_index != -1 )
        choice -> setCurrentIndex( default_index );
    }
  }
  else
  {
    choice -> setCurrentIndex( 0 );
  }
}

void load_setting( QSettings& s, const QString& name, QLineEdit* textbox, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  if ( !v.isEmpty() )
    textbox -> setText( v );
  else if ( !default_value.isEmpty() )
    textbox -> setText( default_value );
}

void load_button_group( QSettings& s, const QString& groupname, QButtonGroup* bg )
{
  s.beginGroup( groupname );
  QList<QString> button_names;
  QList<QAbstractButton*> buttons = bg -> buttons();
  for( int i = 0; i < buttons.size(); ++i)
  {
    button_names.push_back( buttons[ i ] -> text() );
  }
  QStringList keys = s.childKeys();
  for( int i = 0; i < keys.size(); ++i )
  {
    int index = button_names.indexOf( keys[ i ] );
    if ( index != -1 )
      buttons[ index ] -> setChecked( s.value( keys[ i ] ).toBool() );
  }

  s.endGroup();
}

void load_buff_debuff_group( QSettings& s, const QString& groupname, QButtonGroup* bg )
{
  s.beginGroup( groupname );
  QStringList keys = s.childKeys();
  s.endGroup();
  if ( keys.isEmpty() )
  {
    QList<QAbstractButton*> buttons = bg->buttons();
    for ( int i = 0; i < buttons.size(); ++i )
    {
      // All buttons with index > 0 will be checked, 0 won't.
      buttons[i] -> setChecked( (i > 0) );
    }
  }
  else
  {
    load_button_group( s, groupname, bg );
  }
}

void store_button_group( QSettings& s, const QString& name, QButtonGroup* bg )
{
  s.beginGroup( name );
  QList<QAbstractButton*> buttons = bg -> buttons();
  for ( int i = 0; i < buttons.size(); ++i )
  {
    s.setValue( buttons[ i ] -> text(), buttons[ i ] -> isChecked());
  }
  s.endGroup();
}

void load_scaling_groups( QSettings& s, const QString& groupname, QButtonGroup* bg )
{
  s.beginGroup( groupname );
  QStringList keys = s.childKeys();
  s.endGroup();
  if ( keys.isEmpty() )
  {
    QList<QAbstractButton*> buttons = bg->buttons();
    for ( int i = 0; i < buttons.size(); ++i )
    {
      // All buttons unchecked.
      buttons[i] -> setChecked( false );
    }
  }
  else
  {
    load_button_group( s, groupname, bg );
  }
}

} // end unnamed namespace

SC_OptionsTab::SC_OptionsTab( SC_MainWindow* parent ) :
  QTabWidget( parent ), mainWindow( parent )
{
  createGlobalsTab();
  createBuffsDebuffsTab();
  createScalingTab();
  createPlotsTab();
  createReforgePlotsTab();

  QAbstractButton* allBuffs   = buffsButtonGroup -> buttons().at( 0 );
  QAbstractButton* allDebuffs = debuffsButtonGroup -> buttons().at( 0 );
  QAbstractButton* allScaling = scalingButtonGroup -> buttons().at( 1 );

  connect( allBuffs,   SIGNAL( toggled( bool ) ), this, SLOT( allBuffsChanged( bool ) )   );
  connect( allDebuffs, SIGNAL( toggled( bool ) ), this, SLOT( allDebuffsChanged( bool ) ) );
  connect( allScaling, SIGNAL( toggled( bool ) ), this, SLOT( allScalingChanged( bool ) ) );

  connect( choice.armory_region, SIGNAL( currentIndexChanged( const QString& ) ), this, SIGNAL( armory_region_changed( const QString& ) ) );

  connect( choice.armory_region,      SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.armory_spec,        SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.center_scale_delta, SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.challenge_mode,     SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.debug,              SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.default_role,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.gui_localization,   SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.update_check,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.boss_type,          SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.tank_dummy,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.tmi_boss,           SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.tmi_window,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.show_etmi,          SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.deterministic_rng,  SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.fight_length,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.fight_style,        SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.fight_variance,     SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.iterations,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.num_target,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.plots_points,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.plots_step,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.plots_target_error, SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.plots_iterations,   SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.pvp_crit,           SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.reforgeplot_amount, SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.reforgeplot_step,   SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.report_pets,        SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.scale_over,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.statistics_level,   SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.target_error,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.target_level,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.target_race,        SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.threads,            SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.process_priority,    SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.auto_save,          SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.version,            SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.world_lag,          SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( api_client_id,             SIGNAL( textChanged( const QString& ) ), this, SLOT( _optionsChanged() ) );
  connect( api_client_secret,         SIGNAL( textChanged( const QString& ) ), this, SLOT( _optionsChanged() ) );

  connect( buffsButtonGroup,          SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );
  connect( debuffsButtonGroup,        SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );
  connect( scalingButtonGroup,        SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );
  connect( plotsButtonGroup,          SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );
  connect( reforgeplotsButtonGroup,   SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );

  connect( itemDbOrder,               SIGNAL( itemSelectionChanged() ), this, SLOT( _optionsChanged() ) );

  createToolTips();
}

void SC_OptionsTab::_armoryRegionChanged( const QString& region )
{
  auto region_index = choice.armory_region -> findData( region.toUpper(), Qt::DisplayRole );
  if ( region_index != -1 )
  {
    choice.armory_region -> setCurrentIndex( region_index );
  }
}

void SC_OptionsTab::createGlobalsTab()
{
  // Create left side global options
  QFormLayout* globalsLayout_left = new QFormLayout();
  globalsLayout_left -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  globalsLayout_left -> addRow( tr( "Armory Region" ),  choice.armory_region = createChoice( 5, "US", "EU", "TW", "CN", "KR" ) );
  globalsLayout_left -> addRow( tr(   "Armory Spec" ),    choice.armory_spec = createChoice( 2, "Active", "Inactive" ) );

#if SC_BETA
  globalsLayout_left -> addRow(       tr(  "Version" ),        choice.version = createChoice( 3, "Live", "Beta", "Both" ) );
#else
#if SC_USE_PTR
  globalsLayout_left -> addRow(        tr( "Version" ),        choice.version = createChoice( 3, "Live", "PTR", "Both" ) );
#else
  globalsLayout_left -> addRow(        tr( "Version" ),        choice.version = createChoice( 1, "Live" ) );
#endif
#endif
  globalsLayout_left->addRow( tr( "Target Error" ),     choice.target_error     = createChoice( 9, "N/A", "Auto", "1%", "0.5%", "0.3%", "0.1%", "0.05%", "0.03%", "0.01%" ) );
  globalsLayout_left->addRow( tr( "Iterations" ),       choice.iterations       = addValidatorToComboBox( 1, INT_MAX, createChoice( 9, "1", "100", "1000", "10000", "25000", "50000", "100000", "250000", "500000" ) ) );
  globalsLayout_left->addRow( tr( "Length (sec)" ),     choice.fight_length     = addValidatorToComboBox( 1, 10000, createChoice( 10, "100", "150", "200", "250", "300", "350", "400", "450", "500", "600" ) ) );
  globalsLayout_left->addRow( tr( "Vary Length %" ),    choice.fight_variance   = addValidatorToComboBox( 0, 100, createChoice( 6, "0", "10", "20", "30", "40", "50" ) ) );
  globalsLayout_left->addRow( tr( "Fight Style" ),      choice.fight_style      = createChoice( 9, "Patchwerk", "HecticAddCleave", "HelterSkelter", "Ultraxion", "LightMovement", "HeavyMovement", "Beastlord", "CastingPatchwerk", "DungeonSlice" ) );
  globalsLayout_left->addRow( tr( "Challenge Mode" ),   choice.challenge_mode   = createChoice( 2, "Disabled", "Enabled" ) );
  globalsLayout_left->addRow( tr( "Default Role" ),     choice.default_role     = createChoice( 4, "Auto", "DPS", "Heal", "Tank" ) );
  globalsLayout_left->addRow( tr( "GUI Localization" ), choice.gui_localization = createChoice( 5, "auto", "en", "de", "zh", "it" ) );
  globalsLayout_left->addRow( tr( "Update Check" ),     choice.update_check = createChoice( 2, "Yes", "No" ) );

  QPushButton* resetb = new QPushButton( tr("Reset all Settings" ), this );
  QFont override_font = QFont();
  override_font.setPixelSize( 16 );
  resetb -> setFont( override_font );
  resetb -> setToolTip( tr( "Can also be used to fix corrupt settings that are crashing the simulator." ) );

  connect( resetb, SIGNAL(clicked()), this, SLOT(_resetallSettings()) );
  globalsLayout_left -> addWidget( resetb );
   
  QGroupBox* globalsGroupBox_left = new QGroupBox( tr( "Basic Options" ) );
  globalsGroupBox_left -> setLayout( globalsLayout_left );

  // Create middle column of global options
  QFormLayout* globalsLayout_middle = new QFormLayout();
  globalsLayout_middle -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  globalsLayout_middle -> addRow( tr( "Num Enemies" ), choice.num_target = createChoice( 20, "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20" ) );
  globalsLayout_middle -> addRow( tr( "Target Level" ), choice.target_level = createChoice( 4, "Raid Boss", "5-Man Heroic", "5-Man Normal", "Max Player Level" ) );
  globalsLayout_middle -> addRow( tr( "PVP Crit Damage Reduction" ), choice.pvp_crit = createChoice( 2, "Disable", "Enable" ) );
  globalsLayout_middle -> addRow( tr( "Target Race" ),   choice.target_race = createChoice( 7, "Humanoid", "Beast", "Demon", "Dragonkin", "Elemental", "Giant", "Undead" ) );
  globalsLayout_middle -> addRow( tr( "Target Type" ),       choice.boss_type = createChoice( 4, "Custom", "Fluffy Pillow", "Tank Dummy", "TMI Standard Boss" ) );
  globalsLayout_middle -> addRow( tr( "Tank Dummy" ),       choice.tank_dummy = createChoice( 5, "None", "Weak", "Dungeon", "Raid", "Mythic" ) );
  globalsLayout_middle -> addRow( tr( "TMI Standard Boss" ),  choice.tmi_boss = createChoice( 5, "None", "T19M", "T19H", "T19N", "T19L" ) );
  globalsLayout_middle -> addRow( tr( "TMI Window (sec)" ), choice.tmi_window = createChoice( 10, "0", "2", "3", "4", "5", "6", "7", "8", "9", "10" ) );
  globalsLayout_middle -> addRow( tr( "Show ETMI" ),         choice.show_etmi = createChoice( 2, "Only when in group", "Always" ) );

  QGroupBox* globalsGroupBox_middle = new QGroupBox( tr( "Target and Tanking Options" ) );
  globalsGroupBox_middle -> setLayout( globalsLayout_middle );

  // Create right side of global options
  QFormLayout* globalsLayout_right = new QFormLayout();
  globalsLayout_right -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  globalsLayout_right -> addRow( tr( "Threads" ), choice.threads = addValidatorToComboBox( 1, QThread::idealThreadCount(), createChoiceFromRange( 1, QThread::idealThreadCount() ) ) );
  globalsLayout_right -> addRow( tr( "Process Priority" ), choice.process_priority = createChoice( 5, "High", "Above_normal", "Normal", "Below_normal", "Low" ) );
  globalsLayout_right -> addRow( tr( "World Lag" ), choice.world_lag = createChoice( 5, "Super Low - 25 ms", "Low - 50 ms", "Medium - 100 ms", "High - 150 ms", "Australia - 200 ms" ) );
  globalsLayout_right -> addRow( tr( "Generate Debug" ), choice.debug = createChoice( 3, "None", "Log Only", "Gory Details" ) );
  globalsLayout_right -> addRow( tr( "Report Pets Separately" ),  choice.report_pets = createChoice( 2, "Yes", "No" ) );
  globalsLayout_right -> addRow( tr( "Statistics Level" ),   choice.statistics_level = createChoice( 4, "0", "1", "2", "3" ) );
  globalsLayout_right -> addRow( tr( "Deterministic RNG" ), choice.deterministic_rng = createChoice( 2, "Yes", "No" ) );
  globalsLayout_right -> addRow( tr( "Auto-Save Reports" ), choice.auto_save = createChoice( 3, "No", "Use current date/time", "Ask for filename on each simulation" ) );

  QPushButton* savelocation = new QPushButton( tr( "Change default location for reports." ), this );
  QFont override_font2 = QFont();
  override_font2.setPixelSize( 14 );
  resetb -> setFont( override_font2 );

  connect( savelocation, SIGNAL( clicked() ), this, SLOT( _savefilelocation() ) );
  globalsLayout_right -> addWidget( savelocation );

  createItemDataSourceSelector( globalsLayout_right );

  globalsLayout_right -> addRow( tr( "Armory API Client Id" ), api_client_id = new QLineEdit() );
  globalsLayout_right -> addRow( tr( "Armory API Client Secret" ), api_client_secret = new QLineEdit() );

  api_client_id -> setMaxLength( 32 ); // Api key is 32 characters long.
  api_client_id -> setMinimumWidth( 200 );
  api_client_id -> setEchoMode( QLineEdit::PasswordEchoOnEdit ); // Only show the key while typing it in.

  api_client_secret -> setMaxLength( 32 ); // Api key is 32 characters long.
  api_client_secret -> setMinimumWidth( 200 );
  api_client_secret -> setEchoMode( QLineEdit::PasswordEchoOnEdit ); // Only show the key while typing it in.

  QGroupBox* globalsGroupBox_right = new QGroupBox( tr( "Advanced Options" ) );
  globalsGroupBox_right -> setLayout( globalsLayout_right );

  QHBoxLayout* globalsLayout = new QHBoxLayout();
  globalsLayout -> addWidget( globalsGroupBox_left, 1 );
  globalsLayout -> addWidget( globalsGroupBox_middle, 1 );
  globalsLayout -> addWidget( globalsGroupBox_right, 1 );

  QGroupBox* globalsGroupBox = new QGroupBox();
  globalsGroupBox -> setLayout( globalsLayout );

  QScrollArea* globalsGroupBoxScrollArea = new QScrollArea;
  globalsGroupBoxScrollArea -> setWidget( globalsGroupBox );
  globalsGroupBoxScrollArea -> setWidgetResizable( true );
  addTab( globalsGroupBoxScrollArea, tr( "Globals" ) );
}

void SC_OptionsTab::createBuffsDebuffsTab()
{
  // Buffs
  QVBoxLayout* buffsLayout = new QVBoxLayout();               // Buff Layout
  buffsButtonGroup = new QButtonGroup( this );
  buffsButtonGroup -> setExclusive( false );

  appendCheckBox( tr( "Toggle All Buffs" ),             "",                                 tr( "Toggle all buffs on/off" ),                          buffsLayout, buffsButtonGroup );
  appendCheckBox( tr( "Bloodlust" ),                    "override.bloodlust",               tr( "Ancient Hysteria\nBloodlust\nHeroism\nTime Warp" ),  buffsLayout, buffsButtonGroup );
  appendCheckBox( tr( "Arcane Intellect" ),             "override.arcane_intellect",        tr( "Arcane Intellect" ),                                 buffsLayout, buffsButtonGroup );
  appendCheckBox( tr( "Power Word: Fortitude" ),        "override.power_word_fortitude",    tr( "Power Word: Fortitude" ),                                 buffsLayout, buffsButtonGroup );
  appendCheckBox( tr( "Battle Shout" ),                 "override.battle_shout",            tr( "Battle Shout" ),                                        buffsLayout, buffsButtonGroup );
  buffsLayout -> addStretch( 1 );

  QGroupBox* buffsGroupBox = new QGroupBox( tr( "Buffs" ) );  // Buff Widget
  buffsGroupBox -> setLayout( buffsLayout );

  // Debuffs
  QVBoxLayout* debuffsLayout = new QVBoxLayout(); // Debuff Layout
  debuffsButtonGroup = new QButtonGroup( this );
  debuffsButtonGroup -> setExclusive( false );

  appendCheckBox( tr( "Toggle All Debuffs" ),           "",                                 tr( "Toggle all debuffs on/off" ),            debuffsLayout, debuffsButtonGroup );
  appendCheckBox( tr( "Bleeding" ),                     "override.bleeding",                tr( "Rip\nRupture" ),                         debuffsLayout, debuffsButtonGroup );
  appendCheckBox( tr( "Mortal Wounds" ),                "override.mortal_wounds",           tr( "Healing Debuff" ),                       debuffsLayout, debuffsButtonGroup );
  appendCheckBox( tr( "Chaos Brand" ),                  "override.chaos_brand",             tr( "Chaos Brand\nMagic damage debuff" ),     debuffsLayout, debuffsButtonGroup );
  appendCheckBox( tr( "Mystic Touch" ),                 "override.mystic_touch",            tr( "Mystic Touch\nPhysical damage debuff" ), debuffsLayout, debuffsButtonGroup );
  debuffsLayout -> addStretch( 1 );

  QGroupBox* debuffsGroupBox = new QGroupBox( tr( "Debuffs" ) ); // Debuff Widget
  debuffsGroupBox -> setLayout( debuffsLayout );

  // Combined Buff/Debuff Layout & Widget
  QHBoxLayout* buff_debuffLayout = new QHBoxLayout();
  buff_debuffLayout -> addWidget( buffsGroupBox, 1 );
  buff_debuffLayout -> addWidget( debuffsGroupBox, 1 );

  QGroupBox* buff_debuffGroupBox = new QGroupBox();
  buff_debuffGroupBox -> setLayout( buff_debuffLayout );

  // Add Widget as Buffs/Debuffs tab
  QScrollArea* buff_debuffGroupBoxScrollArea = new QScrollArea;
  buff_debuffGroupBoxScrollArea -> setWidget( buff_debuffGroupBox );
  buff_debuffGroupBoxScrollArea -> setWidgetResizable( true );
  addTab( buff_debuffGroupBoxScrollArea, tr( "Buffs / Debuffs" ) );
}

void SC_OptionsTab::createScalingTab()
{
  // layout for entire tab
  QVBoxLayout* scalingLayout = new QVBoxLayout( this );

  // Box containing enable button
  QGroupBox* enableScalingButtonGroupBox = new QGroupBox( this );
  enableScalingButtonGroupBox -> setTitle( tr( "Enable Scaling" ) );
  enableScalingButtonGroupBox -> setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed ) );
  scalingLayout -> addWidget( enableScalingButtonGroupBox );

  QFormLayout* enableScalingButtonGroupBoxLayout = new QFormLayout();
  
  QLabel* enableScalingButtonLabel = new QLabel( tr( "This button enables/disables scale factor calculations, allowing you to toggle scaling while keeping a particular set of stats selected." ) );
  enableScalingButtonGroupBoxLayout -> addWidget( enableScalingButtonLabel );
  
  scalingButtonGroup = new QButtonGroup( this );
  scalingButtonGroup -> setExclusive( false );

  appendCheckBox( tr("Enable Scaling" ), "", tr( "Enable Scaling. This box MUST be checked to enable scaling calculations." ), enableScalingButtonGroupBoxLayout, scalingButtonGroup );
  enableScalingButtonGroupBox -> setLayout( enableScalingButtonGroupBoxLayout );
  
  // Box containing additional options
  QGroupBox* scalingOptionsGroupBox = new QGroupBox( this );
  scalingOptionsGroupBox -> setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed ) );
  scalingOptionsGroupBox -> setTitle( tr( "Scaling Options" ) );
  scalingLayout -> addWidget( scalingOptionsGroupBox );

  QFormLayout* scalingOptionsGroupBoxLayout = new QFormLayout();
  scalingOptionsGroupBoxLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  // Create Combo Boxes
  choice.center_scale_delta = createChoice( 2, "Yes", "No" );
  scalingOptionsGroupBoxLayout -> addRow( tr( "Center Scale Delta" ), choice.center_scale_delta );

  choice.scale_over = createChoice( 9, "Default", "DPS", "HPS", "DTPS", "HTPS", "Raid_DPS", "Raid_HPS", "TMI", "ETMI" );
  scalingOptionsGroupBoxLayout -> addRow( tr( "Scale Over" ), choice.scale_over );

  scalingOptionsGroupBox -> setLayout( scalingOptionsGroupBoxLayout );

  // Box containing buttons for each stat to scale
  QGroupBox* scalingButtonsGroupBox = new QGroupBox( this );
  scalingButtonsGroupBox->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed) );
  scalingButtonsGroupBox->setMinimumHeight( 275 );
  scalingButtonsGroupBox -> setTitle( tr( "Stats to scale" ) );
  scalingLayout -> addWidget( scalingButtonsGroupBox );

  QVBoxLayout* scalingButtonsLayout = new QVBoxLayout();

  QLabel* scalingButtonToggleAllLabel = new QLabel( tr( "This button toggles scaling for all stats except Latency.\nNote that additional simulations will only be run for RELEVANT stats.\nIn other words, Agility and Intellect would be skipped for a Warrior even if they are checked." ) );
  scalingButtonsLayout -> addWidget( scalingButtonToggleAllLabel );

  appendCheckBox( tr( "Toggle All Character Stats" ), "",           tr( "Toggles all stats except Latency."                          ), scalingButtonsLayout, scalingButtonGroup );

  // spacer to separate toggle all button from rest of options
  QSpacerItem* spacer0 = new QSpacerItem( 20, 20 );
  scalingButtonsLayout -> addSpacerItem( spacer0 );

  QLabel* plotHelpertext = new QLabel( tr( "Calculate scale factors for:" ) );
  scalingButtonsLayout -> addWidget( plotHelpertext );

  QVBoxLayout* primaryStatsLayout = new QVBoxLayout();
  QVBoxLayout* secondaryStatsLayout = new QVBoxLayout();
  QVBoxLayout* miscStatsLayout = new QVBoxLayout();

  QHBoxLayout* statChoiceLayout = new QHBoxLayout();

  scalingButtonsLayout -> addLayout( statChoiceLayout );

  statChoiceLayout->addLayout( primaryStatsLayout );
  statChoiceLayout->addLayout( secondaryStatsLayout );
  statChoiceLayout->addLayout( miscStatsLayout );

  appendCheckBox( tr( "Strength"                   ), "str",        tr( "Calculate scale factors for Strength"                       ), primaryStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Agility"                    ), "agi",        tr( "Calculate scale factors for Agility"                        ), primaryStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Stamina"                    ), "sta",        tr( "Calculate scale factors for Stamina"                        ), primaryStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Intellect"                  ), "int",        tr( "Calculate scale factors for Intellect"                      ), primaryStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Spell Power"                ), "sp",         tr( "Calculate scale factors for Spell Power"                    ), primaryStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Attack Power"               ), "ap",         tr( "Calculate scale factors for Attack Power"                   ), primaryStatsLayout, scalingButtonGroup );

  appendCheckBox( tr( "Crit Rating"                ), "crit",       tr( "Calculate scale factors for Crit Rating"                    ), secondaryStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Haste Rating"               ), "haste",      tr( "Calculate scale factors for Haste Rating"                   ), secondaryStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Mastery Rating"             ), "mastery",    tr( "Calculate scale factors for Mastery Rating"                 ), secondaryStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Versatility Rating"         ), "vers",       tr( "Calculate scale factors for Versatility Rating"             ), secondaryStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Weapon DPS"                 ), "wdps",       tr( "Calculate scale factors for Weapon DPS"                     ), miscStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Off-hand Weapon DPS"        ), "wohdps",     tr( "Calculate scale factors for Off-hand Weapon DPS"            ), miscStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Armor"                      ), "armor",      tr( "Calculate scale factors for Armor"                          ), miscStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Bonus Armor"                ), "bonusarmor", tr( "Calculate scale factors for Bonus Armor"                    ), miscStatsLayout, scalingButtonGroup );
  //appendCheckBox( tr( "Avoidance (tertiary)"       ), "avoidance",  tr( "Calculate scale factors for Avoidance (tertiary stat)"      ), miscStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Leech (tertiary)"           ), "leech",      tr( "Calculate scale factors for Leech (tertiary stat)"          ), miscStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Movement Speed (tertiary)"  ), "runspeed",   tr( "Calculate scale factors for Movement Speed (tertiary stat)" ), miscStatsLayout, scalingButtonGroup );
  appendCheckBox( tr( "Latency"                    ), "latency",    tr( "Calculate scale factors for Latency"                        ), miscStatsLayout, scalingButtonGroup );

  // spacer to eat up rest of space (makes scalingButtonsGroupBoxLayout look less silly)
  QSpacerItem* spacer1 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding );
  scalingButtonsLayout -> addSpacerItem( spacer1 );
  scalingButtonsGroupBox -> setLayout( scalingButtonsLayout );

  // Now put the tab together
  QScrollArea* scalingGroupBoxScrollArea = new QScrollArea;
  scalingGroupBoxScrollArea -> setLayout( scalingLayout );
  addTab( scalingGroupBoxScrollArea, tr( "Scaling" ) );
}

void SC_OptionsTab::createPlotsTab()
{
  // layout for entire tab
  QVBoxLayout* plotLayout = new QVBoxLayout();

  // Box containing enable button
  QGroupBox* enablePlotsButtonGroupBox = new QGroupBox();
  enablePlotsButtonGroupBox -> setTitle( tr( "Enable Scaling Plots" ) );
  enablePlotsButtonGroupBox -> setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed ) );
  plotLayout -> addWidget( enablePlotsButtonGroupBox );
  
  QFormLayout* enablePlotsButtonGroupBoxLayout = new QFormLayout();
  
  QLabel* enablePlotsButtonLabel = new QLabel( tr( "This button enables/disables scaling plots, allowing you to toggle calculation of scaling plots while keeping a particular set of stats selected." ) );
  enablePlotsButtonGroupBoxLayout -> addWidget( enablePlotsButtonLabel );
  
  plotsButtonGroup = new QButtonGroup( this );
  plotsButtonGroup -> setExclusive( false );

  appendCheckBox( tr("Enable Plots" ), "", tr( "Enable scaling plots. This box MUST be checked to generate scaling plots." ), enablePlotsButtonGroupBoxLayout, plotsButtonGroup );
  enablePlotsButtonGroupBox -> setLayout( enablePlotsButtonGroupBoxLayout );
  
  // Box containing additional options
  QGroupBox* plotOptionsGroupBox = new QGroupBox();
  plotOptionsGroupBox -> setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed ) );
  plotOptionsGroupBox -> setTitle( tr( "Scaling Plot Options" ) );
  plotLayout -> addWidget( plotOptionsGroupBox );

  QFormLayout* plotOptionsGroupBoxLayout = new QFormLayout();
  plotOptionsGroupBoxLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  // Create Combo Boxes
  choice.plots_points = addValidatorToComboBox( 1, INT_MAX, createChoice( 6, "10", "20", "30", "40", "50", "100" ) );
  plotOptionsGroupBoxLayout -> addRow( tr( "Number of Plot Points" ), choice.plots_points );

  choice.plots_step = addValidatorToComboBox( 1, INT_MAX, createChoice( 8, "25", "50", "100", "150", "200", "250", "500", "1000" ) );
  plotOptionsGroupBoxLayout -> addRow( tr( "Plot Step Amount" ), choice.plots_step );

  choice.plots_target_error = createChoice( 9, "N/A", "Auto", "1%", "0.5%", "0.3%", "0.1%", "0.05%", "0.03%", "0.01%" ); 
  plotOptionsGroupBoxLayout -> addRow( tr( "Plot Target Error" ), choice.plots_target_error );

  choice.plots_iterations = createChoice( 7, "100", "1000", "10000", "25000", "50000", "Iter/10", "Iter/100" );
  plotOptionsGroupBoxLayout -> addRow( tr( "Plot Iterations" ), choice.plots_iterations );

  plotOptionsGroupBox -> setLayout( plotOptionsGroupBoxLayout );

  // Box containing buttons for each stat to scale
  QGroupBox* plotButtonsGroupBox = new QGroupBox();
  plotButtonsGroupBox -> setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Expanding ) );
  plotButtonsGroupBox -> setTitle( tr( "Stats to plot" ) );
  plotLayout -> addWidget( plotButtonsGroupBox );

  QVBoxLayout* plotButtonsLayout = new QVBoxLayout();

  QLabel* plotHelperText = new QLabel( tr( "Check the box for each stat you would like to show on the scaling plots.\n\nPlot scaling for:" ) );
  plotButtonsLayout -> addWidget( plotHelperText );

  appendCheckBox( tr( "Strength"           ), "str",        tr( "Generate Scaling curve for Strength"           ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Agility"            ), "agi",        tr( "Generate Scaling curve for Agility"            ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Stamina"            ), "sta",        tr( "Generate Scaling curve for Stamina"            ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Intellect"          ), "int",        tr( "Generate Scaling curve for Intellect"          ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Spirit"             ), "spi",        tr( "Generate Scaling curve for Spirit"             ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Spell Power"        ), "sp",         tr( "Generate Scaling curve for Spell Power"        ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Attack Power"       ), "ap",         tr( "Generate Scaling curve for Attack Power"       ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Crit Rating"        ), "crit",       tr( "Generate Scaling curve for Crit Rating"        ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Haste Rating"       ), "haste",      tr( "Generate Scaling curve for Haste Rating"       ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Mastery Rating"     ), "mastery",    tr( "Generate Scaling curve for Mastery Rating"     ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Versatility Rating" ), "vers",       tr( "Generate Scaling curve for Versatility Rating" ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Weapon DPS"         ), "wdps",       tr( "Generate Scaling curve for Weapon DPS"         ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Weapon OH DPS"      ), "wohdps",     tr( "Generate Scaling curve for Weapon OH DPS"      ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Armor"              ), "armor",      tr( "Generate Scaling curve for Armor"              ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Bonus Armor"        ), "bonusarmor", tr( "Generate Scaling curve for Bonus Armor"        ), plotButtonsLayout, plotsButtonGroup );
  //appendCheckBox( tr( "Avoidance (tertiary)"       ), "avoidance",  tr( "Generate Scaling curve for Avoidance (tertiary stat)"      ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Leech (tertiary)"           ), "leech",      tr( "Generate Scaling curve for Leech (tertiary stat)"          ), plotButtonsLayout, plotsButtonGroup );
  appendCheckBox( tr( "Movement Speed (tertiary)"  ), "runspeed",      tr( "Generate Scaling curve for Movement Speed (tertiary stat)" ), plotButtonsLayout, plotsButtonGroup );

  // spacer to eat up rest of space (makes plotsButtonGroupBox look less silly)
  QSpacerItem* spacer1 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding );
  plotButtonsLayout -> addSpacerItem( spacer1 );
  plotButtonsGroupBox -> setLayout( plotButtonsLayout );
  
  // Now put the tab together
  QScrollArea* plotsGroupBoxScrollArea = new QScrollArea;
  plotsGroupBoxScrollArea -> setLayout( plotLayout );
  plotsGroupBoxScrollArea -> setWidgetResizable( true );
  addTab( plotsGroupBoxScrollArea, tr( "Plots" ) );
}

void SC_OptionsTab::createReforgePlotsTab()
{
  QFormLayout* reforgePlotsLayout = new QFormLayout();
  reforgePlotsLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  // Create Combo Boxes
  choice.reforgeplot_amount = addValidatorToComboBox( 1, INT_MAX, createChoice( 10, "100", "200", "300", "400", "500", "750", "1000", "1250", "1500", "2000" ) );
  reforgePlotsLayout -> addRow( tr( "Reforge Amount" ), choice.reforgeplot_amount );

  choice.reforgeplot_step = addValidatorToComboBox( 1, INT_MAX, createChoice( 5, "25", "50", "100", "200", "250" ) );
  reforgePlotsLayout -> addRow( tr( "Step Amount" ), choice.reforgeplot_step );

  QLabel* messageText = new QLabel( tr( "A maximum of three stats may be ran at once.\n" ) );
  reforgePlotsLayout -> addRow( messageText );

  reforgeplotsButtonGroup = new SC_ReforgeButtonGroup( this );
  reforgeplotsButtonGroup -> setExclusive( false );

  messageText = new QLabel( tr( "Secondary Stats" ) );
  reforgePlotsLayout -> addRow( messageText );

  appendCheckBox( tr( "Plot Reforge Options for Spirit"             ), "spi",        tr( "Generate reforge plot data for Spirit"             ), reforgePlotsLayout, reforgeplotsButtonGroup );
  appendCheckBox( tr( "Plot Reforge Options for Crit Rating"        ), "crit",       tr( "Generate reforge plot data for Crit Rating"        ), reforgePlotsLayout, reforgeplotsButtonGroup );
  appendCheckBox( tr( "Plot Reforge Options for Haste Rating"       ), "haste",      tr( "Generate reforge plot data for Haste Rating"       ), reforgePlotsLayout, reforgeplotsButtonGroup );
  appendCheckBox( tr( "Plot Reforge Options for Mastery Rating"     ), "mastery",    tr( "Generate reforge plot data for Mastery Rating"     ), reforgePlotsLayout, reforgeplotsButtonGroup );
  appendCheckBox( tr( "Plot Reforge Options for Versatility Rating" ), "vers",       tr( "Generate reforge plot data for Versatility Rating" ), reforgePlotsLayout, reforgeplotsButtonGroup );
  appendCheckBox( tr( "Plot Reforge Options for Bonus Armor Rating" ), "bonusarmor", tr( "Generate reforge plot data for Bonus Armor"        ), reforgePlotsLayout, reforgeplotsButtonGroup );

  messageText = new QLabel( "\n" + tr( "Primary Stats" ) );
  reforgePlotsLayout -> addRow( messageText );

  appendCheckBox( tr( "Plot Reforge Options for Strength"           ), "str",        tr( "Generate reforge plot data for Intellect"          ), reforgePlotsLayout, reforgeplotsButtonGroup );
  appendCheckBox( tr( "Plot Reforge Options for Agility"            ), "agi",        tr( "Generate reforge plot data for Agility"            ), reforgePlotsLayout, reforgeplotsButtonGroup );
  appendCheckBox( tr( "Plot Reforge Options for Stamina"            ), "sta",        tr( "Generate reforge plot data for Stamina"            ), reforgePlotsLayout, reforgeplotsButtonGroup );
  appendCheckBox( tr( "Plot Reforge Options for Intellect"          ), "int",        tr( "Generate reforge plot data for Intellect"          ), reforgePlotsLayout, reforgeplotsButtonGroup );

  QGroupBox* reforgeplotsGroupBox = new QGroupBox();
  reforgeplotsGroupBox -> setLayout( reforgePlotsLayout );

  QScrollArea* reforgeplotsGroupBoxScrollArea = new QScrollArea;
  reforgeplotsGroupBoxScrollArea -> setWidget( reforgeplotsGroupBox );
  reforgeplotsGroupBoxScrollArea -> setWidgetResizable( true );
  addTab( reforgeplotsGroupBoxScrollArea, tr( "Reforge Plots" ) );
}

void SC_OptionsTab::decodeOptions()
{
  QSettings settings;
  settings.beginGroup( "options" );
  load_setting( settings, "version", choice.version );
  load_setting( settings, "target_error", choice.target_error, "Auto" );
  load_setting( settings, "iterations", choice.iterations, "10000" );
  load_setting( settings, "fight_length", choice.fight_length, "300" ); //More representative of raid fights nowadays. - Collision 9/3/2016
  load_setting( settings, "fight_variance", choice.fight_variance, "20" );
  load_setting( settings, "fight_style", choice.fight_style );
  load_setting( settings, "target_race", choice.target_race );
  load_setting( settings, "num_target", choice.num_target );
  load_setting( settings, "threads", choice.threads, QString::number( QThread::idealThreadCount() ) );
  load_setting( settings, "process_priority", choice.process_priority, "Low" );
  load_setting( settings, "auto_save", choice.auto_save, "No" );
  auto_save_location = settings.value( "auto_save_location" ).isNull() ? mainWindow -> AppDataDir : settings.value( "auto_save_location" ).toString();
  load_setting( settings, "armory_region", choice.armory_region );
  load_setting( settings, "armory_spec", choice.armory_spec );
  load_setting( settings, "gui_localization", choice.gui_localization );
  load_setting( settings, "update_check", choice.update_check );
  load_setting( settings, "default_role", choice.default_role );
  load_setting( settings, "boss_type", choice.boss_type, "Custom" );
  load_setting( settings, "pvp_crit", choice.pvp_crit, "Disable" );
  load_setting( settings, "tank_dummy", choice.tank_dummy, "None" );
  load_setting( settings, "tmi_boss", choice.tmi_boss, "None" );
  load_setting( settings, "tmi_window_global", choice.tmi_window, "6" );
  load_setting( settings, "show_etmi", choice.show_etmi );
  load_setting( settings, "world_lag", choice.world_lag, "Medium - 100 ms" );
  load_setting( settings, "api_client_id", api_client_id );
  load_setting( settings, "api_client_secret", api_client_secret );
  load_setting( settings, "debug", choice.debug, "None" );
  load_setting( settings, "target_level", choice.target_level );
  load_setting( settings, "report_pets", choice.report_pets, "No" );
  load_setting( settings, "statistics_level", choice.statistics_level, "1" );
  load_setting( settings, "deterministic_rng", choice.deterministic_rng, "No" );
  load_setting( settings, "challenge_mode", choice.challenge_mode );

  load_setting( settings, "center_scale_delta", choice.center_scale_delta, "No" );
  load_setting( settings, "scale_over", choice.scale_over );

  load_setting( settings, "plot_points", choice.plots_points, "20" );
  load_setting( settings, "plot_step", choice.plots_step, "150" );
  load_setting( settings, "plot_target_error", choice.plots_target_error, "0.5%" );
  load_setting( settings, "plot_iterations", choice.plots_iterations, "10000" );

  load_setting( settings, "reforgeplot_amount", choice.reforgeplot_amount, "500" );
  load_setting( settings, "reforgeplot_step", choice.reforgeplot_step, "50" );

  load_buff_debuff_group( settings, "buff_buttons", buffsButtonGroup );
  load_buff_debuff_group( settings, "debuff_buttons", debuffsButtonGroup );

  // Default settings

  load_scaling_groups( settings, "scaling_buttons", scalingButtonGroup );
  load_scaling_groups( settings, "plots_buttons", plotsButtonGroup );
  load_scaling_groups( settings, "reforgeplots_buttons", reforgeplotsButtonGroup );

  QStringList item_db_order = settings.value( "item_db_order" ).toString().split( "/", QString::SkipEmptyParts );
  if ( !item_db_order.empty() )
  {
    for( int i = 0; i < item_db_order.size(); ++i )
    {
      for( int k = 0; k < itemDbOrder->count(); ++k)
      {
        if ( ! item_db_order[ i ].compare( itemDbOrder -> item( k ) -> data( Qt::UserRole ).toString() ) )
        {
          itemDbOrder -> addItem( itemDbOrder -> takeItem( k ) );
        }
      }

    }
  }
  else
  {
    for ( unsigned i = 0; i < sizeof_array( itemSourceOptions ); ++i )
    {
      for( int k = 0; k < itemDbOrder->count(); ++k)
      {
        if ( ! QString(itemSourceOptions[ i ].option).compare( itemDbOrder -> item( k ) -> data( Qt::UserRole ).toString() ) )
        {
          itemDbOrder -> addItem( itemDbOrder -> takeItem( k ) );
        }
      }
    }
  }
  settings.endGroup();
}

/**
 * Encode all options/setting into a string ( to be able to save it to the history )
 */
void SC_OptionsTab::encodeOptions()
{
  QSettings settings;
  settings.beginGroup( "options" );
  settings.setValue( "version", choice.version -> currentText() );
  settings.setValue( "target_error", choice.target_error -> currentText() );
  settings.setValue( "iterations", choice.iterations -> currentText() );
  settings.setValue( "fight_length", choice.fight_length -> currentText() );
  settings.setValue( "fight_variance", choice.fight_variance -> currentText() );
  settings.setValue( "fight_style", choice.fight_style -> currentText() );
  settings.setValue( "target_race", choice.target_race -> currentText() );
  settings.setValue( "num_target", choice.num_target -> currentText() );
  settings.setValue( "threads", choice.threads -> currentText() );
  settings.setValue( "process_priority", choice.process_priority -> currentText() );
  settings.setValue( "auto_save", choice.auto_save -> currentText() );
  settings.setValue( "auto_save_location", auto_save_location );
  settings.setValue( "armory_region", choice.armory_region -> currentText() );
  settings.setValue( "gui_localization", choice.gui_localization -> currentText() );
  settings.setValue( "update_check", choice.update_check -> currentText() );
  settings.setValue( "armory_spec", choice.armory_spec -> currentText() );
  settings.setValue( "default_role", choice.default_role -> currentText() );
  settings.setValue( "boss_type", choice.boss_type -> currentText() );
  settings.setValue( "tank_dummy", choice.tank_dummy -> currentText() );
  settings.setValue( "tmi_boss", choice.tmi_boss -> currentText() );
  settings.setValue( "tmi_window_global", choice.tmi_window -> currentText() );
  settings.setValue( "show_etmi", choice.show_etmi -> currentText() );
  settings.setValue( "world_lag", choice.world_lag -> currentText() );
  settings.setValue( "api_client_id", api_client_id -> text() );
  settings.setValue( "api_client_secret", api_client_secret -> text() );
  settings.setValue( "debug", choice.debug -> currentText() );
  settings.setValue( "target_level", choice.target_level -> currentText() );
  settings.setValue( "pvp_crit", choice.pvp_crit -> currentText() );
  settings.setValue( "report_pets", choice.report_pets -> currentText() );
  settings.setValue( "statistics_level", choice.statistics_level -> currentText() );
  settings.setValue( "deterministic_rng", choice.deterministic_rng -> currentText() );
  settings.setValue( "center_scale_delta", choice.center_scale_delta -> currentText() );
  settings.setValue( "scale_over", choice.scale_over -> currentText() );
  settings.setValue( "challenge_mode", choice.challenge_mode -> currentText() );

  settings.setValue( "plot_points", choice.plots_points -> currentText() );
  settings.setValue( "plot_step", choice.plots_step -> currentText() );
  settings.setValue( "plot_target_error", choice.plots_target_error -> currentText() );
  settings.setValue( "plot_iterations", choice.plots_iterations -> currentText() );

  store_button_group( settings, "buff_buttons", buffsButtonGroup );
  store_button_group( settings, "debuff_buttons", debuffsButtonGroup );
  store_button_group( settings, "scaling_buttons", scalingButtonGroup );
  store_button_group( settings, "plots_buttons", plotsButtonGroup );
  store_button_group( settings, "reforgeplots_buttons", reforgeplotsButtonGroup );
  QString item_db_order;
  for( int i = 0; i < itemDbOrder->count(); ++i )
  {
    if ( i > 0 )
      item_db_order += "/";
    item_db_order += itemDbOrder->item( i ) -> data( Qt::UserRole ).toString();
  }
  settings.setValue( "item_db_order", item_db_order );

  settings.endGroup(); // end group "options"
}

void SC_OptionsTab::createToolTips()
{
  choice.version -> setToolTip( tr( "Live:  Use mechanics on Live servers. ( WoW Build %1 )" ).arg( dbc::build_level( false ) ) + "\n" +
                              #if SC_BETA
                                tr( "Beta:  Use mechanics on Beta servers. ( WoW Build %1 )" ).arg( dbc::build_level( true ) ) + "\n" +
                                tr( "Both: Create Evil Twin with Beta mechanics" ) );
#else
                                tr( "PTR:  Use mechanics on PTR servers. ( WoW Build %1 )" ).arg( dbc::build_level( true ) ) + "\n" +
                                tr( "Both: Create Evil Twin with PTR mechanics" ) );
#endif
  choice.target_error -> setToolTip( tr( "This options sets a target error threshold and\n"
                                         "runs iterations until that threshold is reached.\n" ) +
                                     tr( "N/A:  Do not use this feature.\n" ) +
                                     tr( "Auto: use sim defaults based on other options\n     (0.2%, 0.05% for scale factors).\n" ) +
                                     tr( "X%:   Run until DPS error is less than X%." ) );
  choice.iterations -> setToolTip( tr( "%1:    Fast and Rough" ).arg( 100 ) + "\n" +
                                   tr( "%1:   Sufficient for DPS Analysis" ).arg( 1000 ) + "\n" +
                                   tr( "%1: Recommended for Scale Factor Generation" ).arg( 10000 ) + "\n" +
                                   tr( "%1: Use if %2 isn't enough for Scale Factors" ).arg( 25000 ).arg( 10000 ) + "\n" +
                                   tr( "%1: If you're patient" ).arg( 50000 ) );

  choice.fight_variance -> setToolTip( tr( "Varying the fight length over a given spectrum improves\n"
                                           "the analysis of trinkets and abilities with long cooldowns." ) );

  choice.fight_style -> setToolTip( tr( "Patchwerk: Tank-n-Spank" ) + "\n" +
                                    tr( "HecticAddCleave:\n"
                                        "    Heavy Movement, Frequent Add Spawns" ) + "\n" +
                                    tr( "HelterSkelter:\n"
                                        "    Movement, Stuns, Interrupts,\n"
                                        "    Target-Switching (every 2min)" ) + "\n" +
                                    tr( "Ultraxion:\n"
                                        "    Periodic Stuns, Raid Damage" ) + "\n" +
                                    tr( "LightMovement:\n"
                                        "    %1s Movement, %2s CD,\n"
                                        "    %3% into the fight until %4% before the end" ).arg( 7 ).arg( 85 ).arg( 10 ).arg( 20 ) + "\n" +
                                    tr( "HeavyMovement:\n"
                                        "    %1s Movement, %2s CD,\n"
                                        "    beginning %3s into the fight" ).arg( 4 ).arg( 10 ).arg( 10 ) + "\n" +
                                    tr( "Beastlord:\n"
                                        "    Random Movement, Advanced Positioning,\n"
                                        "    Frequent Single and Wave Add Spawns" ) + "\n" +
                                    tr( "CastingPatchwerk: Tank-n-Spank\n"
                                        "    Boss considered always casting\n"
                                        "    (to test interrupt procs on cooldown)" ) + "\n" + 
                                    tr( "DungeonSlice:\n"
                                        "    Multi-segment simulation meant to\n"
                                        "    approximate M+ dungeon and boss pulls" ) );

  choice.target_race -> setToolTip( tr( "Race of the target and any adds." ) );

  choice.challenge_mode -> setToolTip( tr( "Enables/Disables the challenge mode setting, downscaling items to level 630." ) );

  choice.num_target -> setToolTip( tr( "Number of enemies." ) );

  choice.target_level -> setToolTip( tr( "Level of the target and any adds." ) );

  choice.pvp_crit -> setToolTip( tr( "In PVP, critical strikes deal 150% damage instead of 200%.\n"
                                     "Enabling this option will set target level to max player level." ) );


  choice.threads -> setToolTip( tr( "Match the number of CPUs for optimal performance.\n"
                                    "Most modern desktops have at least two CPU cores." ) );
   
  choice.process_priority -> setToolTip( tr( "This can allow for a more responsive computer while simulations are running.\n"
                                            "When set to 'Lowest', it will be possible to use your computer as normal while SimC runs in the background." ) );

  choice.auto_save -> setToolTip( tr( "This will allow automatic saving of html reports to the simc folder." ) );

  choice.armory_region -> setToolTip( tr( "United States, Europe, Taiwan, China, Korea" ) );

  choice.armory_spec -> setToolTip( tr( "Controls which Talent specification is used when importing profiles from the Armory." ) );

  choice.gui_localization -> setToolTip( tr( "Controls the GUI display language." ) );

  choice.update_check -> setToolTip( tr( "Check Simulationcraft updates on startup." ) );

  choice.default_role -> setToolTip( tr( "Specify the character role during import to ensure correct action priority list." ) );

  choice.boss_type -> setToolTip( tr( "Choose the type of target. Some choices can be refined further by the next two drop-down boxes" ) );

  choice.tank_dummy -> setToolTip( tr( "If \"Tank Dummy\" is chosen above, this drop-down selects the type of tank dummy used.\n"
                                       "Leaving at *None* will default back to a Fluffy Pillow." ) );
  
  choice.tmi_boss -> setToolTip( tr( "If \"TMI Standard Boss\" is chosen in \"Target Type\", this box selects the TMI standard.\n"
                                     "TMI Standard Bosses provide damage output similar to bosses in the appropriate tier.\n"
                                     "Leaving at *None* will default back to a Fluffy Pillow." ) );

  choice.tmi_window -> setToolTip( tr( "Specify window duration for calculating TMI. Default is 6 sec.\n"
                                       "Reducing this increases the metric's sensitivity to shorter damage spikes.\n"
                                       "Set to 0 if you want to vary on a per-player basis in the Simulate tab using \"tmi_window=#\"." ) );

  choice.show_etmi -> setToolTip( tr( "Controls when ETMI is displayed in the HTML report.\n"
                                      "TMI only includes damage taken and self-healing/absorbs, and treats overhealing as effective healing.\n"
                                      "ETMI includes all sources of healing and absorption, and ignores overhealing." ) );

  choice.report_pets -> setToolTip( tr( "Specify if pets get reported separately in detail." ) );

  choice.statistics_level -> setToolTip( tr( "Determines how much detailed statistical information besides count & mean will be collected during simulation.\n"
                                             " Higher Statistics Level require more memory." ) + "\n" +
                                         tr( " Level %1: Only Simulation Length data is collected." ).arg( 0 ) +"\n" +
                                         tr( " Level %1: DPS/HPS data is collected. *default*" ).arg( 1 ) +"\n" +
                                         tr( " Level %1: Player Fight Length, Death Time, DPS(e), HPS(e), DTPS, HTPS, DMG, HEAL data is collected." ).arg( 2 ) +"\n" +
                                         tr( " Level %1: Ability Amount and  portion APS is collected." ).arg( 3 ) );

  choice.debug -> setToolTip( tr( "When a log is generated, only one iteration is used.\n"
                                  "Gory details are very gory.  No documentation will be forthcoming.\n"
                                  "Due to the forced single iteration, no scale factor calculation." ) );

  choice.deterministic_rng -> setToolTip( tr( "Deterministic Random Number Generator creates all random numbers with a given, constant seed.\n"
                                              "This allows to better observe marginal changes which aren't influenced by rng, \n"
                                              " or check for other influences without having to reduce statistic noise" ) );

  choice.world_lag -> setToolTip( tr( "World Lag is the equivalent of the 'world lag' shown in the WoW Client.\n"
                                      "It is currently used to extend the cooldown duration of user executable abilities "
                                      " that have a cooldown.\n"
                                      "Each setting adds an amount of 'lag' with a default standard deviation of 10%:" ) + "\n" +
                                  tr( "    'Super Low' : %1ms" ).arg( 25 ) + "\n" +
                                  tr( "    'Low'   : %1ms" ).arg( 50 ) + "\n" +
                                  tr( "    'Medium': %1ms" ).arg( 100 ) + "\n" +
                                  tr( "    'High'  : %1ms" ).arg( 150 ) + "\n" + 
                                  tr( "    'Australia' : %1ms" ).arg( 200 ) );
  
  choice.center_scale_delta -> setToolTip( tr( "Controls the simulations that the tool compares to determine stat weights.\nIf set to No, it will sim once at profile stats and again with +2X of each selected stat.\nIf set to Yes, it will sim once at profile-X and once at profile+X." ) );
  choice.scale_over -> setToolTip( tr( "Choose the stat over which you're primarily interested in scaling.\nThis is the metric that will be displayed on the Scale Factors plot.\nNote that the sim will still generate and display scale factors for all other metrics in tabular form." ) );

  choice.plots_points -> setToolTip( tr( "The number of points that will appear on the graph" ) );
  choice.plots_step -> setToolTip( tr( "The delta between two points of the graph.\n"
                                       "The deltas on the horizontal axis will be within the [-points * steps / 2 ; +points * steps / 2] interval" ) );

  choice.plots_target_error -> setToolTip( tr( "Target error for plots.\n" ) +
                                           tr( "N/A:  Do not use this feature.\n" ) +
                                           tr( "Auto: Use simulation defaults (0.5%).\n" ) +
                                           tr( "X%:   Each plot point will sim until less than X% DPS error is reached." ) );
  choice.plots_iterations -> setToolTip( tr( "Number of iterations for each plot point.\n"
                                             "Iter/10 and Iter/100 scale with the number of\n"
                                             "iterations selected on the general options tab." ) );

  choice.reforgeplot_amount -> setToolTip( tr( "The maximum amount to reforge per stat." ) );
  choice.reforgeplot_step -> setToolTip( tr( "The stat difference between two points.\n"
                                             "It's NOT the number of steps: a lower value will generate more points!" ) );
}

QString SC_OptionsTab::get_globalSettings()
{
  QString options = "";

#if SC_USE_PTR
  options += "ptr=";
  options += ( ( choice.version->currentIndex() == 1 ) ? "1" : "0" );
  options += "\n";
#endif
  options += "item_db_source=" + get_db_order() + '\n';

  // iterations/error stuff
  if ( choice.target_error -> currentIndex() < 2 )
  {
    options += "target_error=0\n";

    if ( choice.target_error -> currentIndex() == 0 )
      options += "iterations=" + choice.iterations->currentText() + "\n";
    else
      options += "iterations=0\n";
  }
  else
  {
    std::vector<std::string> splits = util::string_split( choice.target_error -> currentText().toStdString(), "%" );
    assert( splits.size() > 0 );
    options += "target_error=" + QString( splits[ 0 ].c_str() ) + "\n";
    options += "iterations=0\n";
  }  

  const char *world_lag[] = { "0.025", "0.05", "0.1", "0.15", "0.20" };
  options += "default_world_lag=";
  options += world_lag[ choice.world_lag->currentIndex() ];
  options += "\n";

  options += "max_time=" + choice.fight_length->currentText() + "\n";

  options += "vary_combat_length=";
  double fight_variance_ = choice.fight_variance -> currentText().toDouble();
  fight_variance_ /= 100;
  options += QString::number( fight_variance_ );
  options += "\n";

  if ( choice.fight_style->currentText() != "Patchwerk" )
  {
    options += "fight_style=" + choice.fight_style->currentText() + "\n";
  }

  if ( choice.challenge_mode -> currentIndex() > 0 )
    options += "challenge_mode=1\n";

  if ( choice.show_etmi -> currentIndex() != 0 )
    options += "show_etmi=1\n";

  if ( choice.tmi_window -> currentIndex() != 0 )
    options += "tmi_window_global=" + choice.tmi_window-> currentText() + "\n";

  // choice.boss_type controls what type of enemy is spawned
  // TMI Bosses and Tank Dummies have special module commands, and skip some settings (target_level & _race)
  if ( choice.boss_type -> currentIndex() > 1 )
  {
    if ( choice.boss_type -> currentText() == "Tank Dummy" && choice.tank_dummy -> currentIndex() != 0 )
    {
      // boss setup
      options += "tank_dummy=Tank_Dummy_" + choice.tank_dummy -> currentText() + "\n";
      options += "tank_dummy_type=" + choice.tank_dummy -> currentText() + "\n";
    }

    if ( choice.boss_type -> currentText() == "TMI Standard Boss" && choice.tmi_boss -> currentIndex() != 0 )
    {
      // boss setup
      options += "tmi_boss=TMI_Standard_Boss_" + choice.tmi_boss -> currentText() + "\n";
      options += "tmi_boss_type=" + choice.tmi_boss -> currentText() + "\n";
    }
  }
  else
  {
    static const char* const targetlevel[] = { "3", "2", "1", "0" };
    if ( choice.pvp_crit -> currentIndex() == 1 )
    {
      options += "target_level+=0\n";
      options += "pvp=1\n";
    }
    else
    {
      options += "target_level+=";
      options += targetlevel[choice.target_level -> currentIndex()];
      options += "\n";
      options += "target_race=" + choice.target_race->currentText() + "\n";
    }
  }
  // end target spawning

  options += "optimal_raid=0\n";

  foreach ( QAbstractButton* button, buffsButtonGroup -> buttons() )
  {
    if ( !button -> objectName().isEmpty() )
      options += QString( "%1=%2\n" ).arg( button -> objectName() ).arg( button -> isChecked() ? "1" : "0" );
  }
  foreach ( QAbstractButton* button, debuffsButtonGroup->buttons() )
  {
    if ( !button -> objectName().isEmpty() )
      options += QString( "%1=%2\n" ).arg( button -> objectName() ).arg( button -> isChecked() ? "1" : "0" );
  }

  if ( choice.deterministic_rng->currentIndex() == 0 )
  {
    options += "deterministic=1\n";
  }

  return options;
}

QString SC_OptionsTab::getReportlDestination() const
{
   assert( mainWindow );
  // Setup html report destination
  QString html_filename = "simc_report";
  if ( choice.auto_save -> currentIndex() != 0 )
  {
    QString text;
    if ( choice.auto_save -> currentIndex() == 1 )
    {
      QDateTime dateTime = QDateTime::currentDateTime();
      text += dateTime.toString( Qt::ISODate );
      text.replace( ":", "" );
    }
    else
    {
      bool ok;
      text = QInputDialog::getText( mainWindow, tr( "Report File Name" ),
        tr( "What would you like to name the report files?" ), QLineEdit::Normal,
        QDir::home().dirName(), &ok );
    }
    RemoveBadFileChar( text );
    if ( ! text.isEmpty() )
    {
      html_filename = text;
    }
  }
  if ( auto_save_location.isEmpty() || !QDir( auto_save_location ).exists() )
  {
   return mainWindow -> AppDataDir + QDir::separator() + html_filename;
  }
  return auto_save_location + QDir::separator() + html_filename;
}

QString SC_OptionsTab::mergeOptions()
{
  QString options = "";
  options += "### Begin GUI options ###\n";
  QString spell_query = mainWindow -> cmdLine -> commandLineText();
  if ( spell_query.startsWith( "spell_query" ) )
  {
    options += spell_query;
    return options;
  }

  options += get_globalSettings();
  options += "threads=" + choice.threads -> currentText() + "\n";
  options += "process_priority=" + choice.process_priority -> currentText() + "\n";

  QList<QAbstractButton*> buttons = scalingButtonGroup -> buttons();

  /////// Scaling Options ///////
  // skip if enable checkbox isn't checked
  if ( buttons.at( 0 ) -> isChecked() )
  {
    // make a list of enabled scaling
    QStringList scales;
    foreach ( QAbstractButton* button, buttons )
    {
      if ( !button -> objectName().isEmpty() && button -> isChecked() )
        scales.append( button -> objectName() );
    }

    if ( scales.count() > 0 )
    {
      options += "calculate_scale_factors=1\n";
      if ( scales.removeAll( "latency" ) > 0 )  // "latency" needs to be removed
      {
        options += "scale_lag=1\n";
      }
      options += "scale_only=" + scales.join( "," ) + "\n"; // dump list of scaling
    }

    if ( choice.center_scale_delta->currentIndex() == 0 )
    {
      options += "center_scale_delta=1\n";
    }
  }

  // always print scale_over in case we want to use plots
  if ( choice.scale_over -> currentIndex() != 0 )
  {
    options += "scale_over=";
    options += choice.scale_over -> currentText();
    options += "\n";
  }

  /////// DPS Plot Options ///////
  // skip if enable checkbox isn't checked
  buttons = plotsButtonGroup -> buttons();
  if ( buttons.at( 0 ) -> isChecked() )
  {
    QStringList plots;
    foreach ( QAbstractButton* button, buttons)
    {
      if ( !button -> objectName().isEmpty() && button -> isChecked() )
        plots.append( button -> objectName() );
    }
    if ( plots.count() > 0 )
    {
      options += "dps_plot_stat=" + plots.join( "," ) + "\n";
      options += "dps_plot_points=" + choice.plots_points -> currentText() + "\n";
      options += "dps_plot_step=" + choice.plots_step -> currentText() + "\n";

      if ( choice.plots_target_error -> currentIndex() < 2 )
      {
        options += "dps_plot_target_error=0\n";
        if ( choice.plots_target_error -> currentIndex() == 0 )
        {
          options += "dps_plot_iterations=";

          std::vector<std::string> splits = util::string_split( choice.plots_iterations -> currentText().toStdString(), "/" );
          if ( splits.size() > 1 )
          {
            int base_iter = std::stoi( choice.iterations -> currentText().toStdString() );
            int divisor = std::stoi( splits[1] );
            options += QString::number( base_iter / divisor ) + "\n";
          }
          else
            options += choice.plots_iterations -> currentText() + "\n";
        }
        else
          options += "dps_plot_iterations=0\n";
      }
      else
      {
        std::vector<std::string> splits = util::string_split( choice.plots_target_error -> currentText().toStdString(), "%" );
        assert( splits.size() > 0 );
        options += "dps_plot_target_error=" + QString( splits[0].c_str() ) + "\n";
        options += "dps_plot_iterations=0\n";
      }
    }
  }
  /////// Reforge Plot Options ///////

  QStringList reforges;
  foreach ( QAbstractButton* button, reforgeplotsButtonGroup -> buttons() )
  {
    if ( button -> isChecked() )
    {
      reforges.append( button -> objectName() );
    }
  }
  if ( reforges.count() > 0 )
  {
    options += "reforge_plot_stat=" + reforges.join( "," ) + "\n";
    options += "reforge_plot_amount=" + choice.reforgeplot_amount -> currentText() + "\n";
    options += "reforge_plot_step=" + choice.reforgeplot_step -> currentText() + "\n";
  }

  if ( choice.statistics_level->currentIndex() >= 0 )
  {
    options += "statistics_level=" + choice.statistics_level->currentText() + "\n";
  }

  if ( choice.report_pets->currentIndex() != 1 )
  {
    options += "report_pets_separately=1\n";
  }

  options += "### End GUI options ###\n"

      "### Begin simulateText ###\n";
  options += mainWindow -> simulateTab -> current_Text() -> toPlainText();
  options += "\n"
      "### End simulateText ###\n";

  options += "desired_targets=" + QString::number( choice.num_target -> currentIndex() + 1 ) + "\n";

  options += "### Begin overrides ###\n";
  options += mainWindow -> overridesText -> toPlainText();
  options += "\n";
  options += "### End overrides ###\n";

/* Disabled by scamille 2015-09-09: Unclear advantage of feature with override tab available;
 * potential to cause hard to understand problems for user, see https://github.com/simulationcraft/simc/issues/2618
 */
//      "### Begin command line ###\n";
//  options += mainWindow -> cmdLine -> commandLineText();
//  options += "\n"
//      "### End command line ###\n"

  options += "### Begin final options ###\n";
#if SC_USE_PTR
  if ( choice.version->currentIndex() == 2 )
  {
    options += "ptr=1\n";
    options += "copy=EvilTwinPTR\n";
    options += "ptr=0\n";
  }
#endif

  if ( choice.debug->currentIndex() != 0 )
  {
    options += "log=1\n";
    options += "scale_only=none\n";
    options += "dps_plot_stat=none\n";
  }
  options += "### End final options ###\n"

      "### END ###";

  return options;
}

QString SC_OptionsTab::get_active_spec()
{
  return choice.armory_spec -> currentText();
}

QString SC_OptionsTab::get_player_role()
{
  return choice.default_role -> currentText();
}

QString SC_OptionsTab::get_api_key()
{
  if ( api_client_id->text().size() && api_client_secret->text().size() )
  {
    return api_client_id->text() + ':' + api_client_secret->text();
  }

  return {};
}

void SC_OptionsTab::createItemDataSourceSelector( QFormLayout* layout )
{
  itemDbOrder = new QListWidget( this );
  itemDbOrder -> setDragDropMode( QAbstractItemView::InternalMove );
  itemDbOrder -> setResizeMode( QListView::Fixed );
  itemDbOrder -> setSelectionRectVisible( false );
  itemDbOrder -> setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
  itemDbOrder -> setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  itemDbOrder -> setMaximumWidth( 200 );

  for ( unsigned i = 0; i < sizeof_array( itemSourceOptions ); ++i )
  {
    QListWidgetItem* item = new QListWidgetItem( itemSourceOptions[ i ].label );
    item -> setData( Qt::UserRole, QVariant( itemSourceOptions[ i ].option ) );
    item -> setToolTip( itemSourceOptions[ i ].tooltip );
    itemDbOrder -> addItem( item );
  }

  itemDbOrder -> setFixedHeight( ( itemDbOrder -> model() -> rowCount() + 1 ) * itemDbOrder -> sizeHintForRow( 0 ) );
  layout->addRow( "Item Source Order", itemDbOrder );
}

QComboBox* SC_OptionsTab::addValidatorToComboBox( int lowerBound, int upperBound, QComboBox* comboBox )
{
  return SC_ComboBoxIntegerValidator::ApplyValidatorToComboBox( new SC_ComboBoxIntegerValidator( lowerBound, upperBound, comboBox ), comboBox );
}

QString SC_OptionsTab::get_db_order() const
{
  QString options;

  assert( itemDbOrder -> count() > 0 );

  for ( int i = 0; i < itemDbOrder -> count(); i++ )
  {
    QListWidgetItem *it = itemDbOrder -> item( i );
    options += it -> data( Qt::UserRole ).toString();
    if ( i < itemDbOrder -> count() - 1 )
      options += '/';
  }
  return options;
}

void SC_OptionsTab::toggleInterdependentOptions()
{
  // disable iterations box based on target_error setting
  if ( choice.target_error -> currentIndex() > 0 )
    choice.iterations -> setDisabled( true );
  else
    choice.iterations -> setEnabled( true );

  // same thing for plot tab
  if ( choice.plots_target_error -> currentIndex() > 0 )
    choice.plots_iterations -> setDisabled( true );
  else
    choice.plots_iterations -> setEnabled( true );

  //on Globals tab, toggle Tank Dummy and TMI Standard Boss settings
  choice.tank_dummy -> setDisabled( true );
  choice.tmi_boss -> setDisabled( true );
  choice.target_level -> setEnabled( true );
  choice.target_race -> setEnabled( true );
  if ( choice.boss_type -> currentIndex() == 2 )
  {
    choice.tank_dummy -> setEnabled( true );
    choice.target_level -> setDisabled( true );
    choice.target_race -> setDisabled( true );
  }
  if ( choice.boss_type -> currentIndex() == 3 )
  {
    choice.tmi_boss -> setEnabled( true );
    choice.target_level -> setDisabled( true );
    choice.target_race -> setDisabled( true );
  }
  // others go here
}

// ============================================================================
// Private Slots
// ============================================================================

void SC_OptionsTab::allBuffsChanged( bool checked )
{
  QList<QAbstractButton*> buttons = buffsButtonGroup -> buttons();
  int count = buttons.count();
  for ( int i = 1; i < count; i++ )
  {
    buttons.at( i ) -> setChecked( checked );
  }
}

void SC_OptionsTab::allDebuffsChanged( bool checked )
{
  QList<QAbstractButton*> buttons = debuffsButtonGroup->buttons();
  int count = buttons.count();
  for ( int i = 1; i < count; i++ )
  {
    buttons.at( i ) -> setChecked( checked );
  }
}

void SC_OptionsTab::allScalingChanged( bool checked )
{
  QList<QAbstractButton*> buttons = scalingButtonGroup->buttons();
  int count = buttons.count();
  for ( int i = 2; i < count - 1; i++ )
  {
    buttons.at( i ) -> setChecked( checked );
  }
}

void SC_OptionsTab::_optionsChanged()
{
  // Maybe hook up history save, depending on IO cost.
  emit optionsChanged();
  toggleInterdependentOptions();
}

/* Reset all settings, with q nice question box asking for confirmation
 */
void SC_OptionsTab::_resetallSettings()
{
  int confirm = QMessageBox::question( this, tr( "Reset all Settings" ), tr( "Do you really want to reset all Settings to default?" ), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
  if ( confirm == QMessageBox::Yes )
  {
    QSettings settings;
    settings.clear();
    decodeOptions();
  }
}

void SC_OptionsTab::_savefilelocation()
{
  QFileDialog f( this );
  f.setDirectory( auto_save_location );
  f.setFileMode( QFileDialog::DirectoryOnly );
  f.setWindowTitle( tr( "Default Save Location" ) );

  f.exec();

  auto_save_location = f.selectedFiles().at( 0 );
}
