// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include "SC_OptionsTab.hpp"
#include "util/sc_mainwindowcommandline.hpp"

namespace { // unnamed namespace

struct OptionEntry
{
  const char* label;
  const char* option;
  const char* tooltip;
};

const OptionEntry buffOptions[] =
{
  { "Toggle All Buffs",             "",                                 "Toggle all buffs on/off"                         },
  { "Attack Power Multiplier",      "override.attack_power_multiplier", "+10% Attack Power Multiplier"                    },
  { "Spell Power Multiplier",       "override.spell_power_multiplier",  "+10% Spell Power Multiplier"                     },
  { "Critical Strike",              "override.critical_strike",         "+5% Melee/Ranged/Spell Critical Strike Chance"   },
  { "Haste",                        "override.haste",                   "+5% Haste"                                       },
  { "Multistrike",                  "override.multistrike",             "+5% Multistrike"                                 },
  { "Mastery",                      "override.mastery",                 "+5 Mastery"                                      },
  { "Stamina",                      "override.stamina",                 "+10% Stamina"                                    },
  { "Strength, Agility, Intellect", "override.str_agi_int",             "+5% Strength, Agility, Intellect"                },
  { "Versatility",                  "override.versatility",             "+3% Versatility"                                 },

  { "Bloodlust",                    "override.bloodlust",               "Ancient Hysteria\nBloodlust\nHeroism\nTime Warp" },
  { NULL, NULL, NULL }
};

const OptionEntry itemSourceOptions[] =
{
  { "Local Item Database", "local",   "Use Simulationcraft item database" },
  { "Blizzard API",        "bcpapi",  "Remote Blizzard Community Platform API source" },
  { "Wowhead.com",         "wowhead", "Remote Wowhead.com item data source" },
  { "Wowhead.com (PTR)",   "ptrhead", "Remote Wowhead.com PTR item data source" },
#if SC_BETA
  { "Wowhead.com (Beta)",  SC_BETA_STR "head", "Remote Wowhead.com Beta item data source" },
#endif
};

const OptionEntry debuffOptions[] =
{
  { "Toggle All Debuffs",     "",                                "Toggle all debuffs on/off"      },

  { "Bleeding",               "override.bleeding",               "Rip\nRupture"   },
  { "Mortal Wounds",          "override.mortal_wounds",          "Healing Debuff"                 },

  { NULL, NULL, NULL }
};

const OptionEntry scalingOptions[] =
{
  { "Analyze All Stats",                "",         "Scale factors are necessary for gear ranking.\nThey only require an additional simulation for each RELEVANT stat." },
  { "Use Positive Deltas Only",         "",         "This option forces a positive scale delta, which is useful for classes with soft caps."  },
  { "Analyze Strength",                 "str",      "Calculate scale factors for Strength"                 },
  { "Analyze Agility",                  "agi",      "Calculate scale factors for Agility"                  },
  { "Analyze Stamina",                  "sta",      "Calculate scale factors for Stamina"                  },
  { "Analyze Intellect",                "int",      "Calculate scale factors for Intellect"                },
  { "Analyze Spirit",                   "spi",      "Calculate scale factors for Spirit"                   },
  { "Analyze Spell Power",              "sp",       "Calculate scale factors for Spell Power"              },
  { "Analyze Attack Power",             "ap",       "Calculate scale factors for Attack Power"             },
  { "Analyze Crit Rating",              "crit",     "Calculate scale factors for Crit Rating"              },
  { "Analyze Haste Rating",             "haste",    "Calculate scale factors for Haste Rating"             },
  { "Analyze Mastery Rating",           "mastery",  "Calculate scale factors for Mastery Rating"           },
  { "Analyze Multistrike Rating",       "mult",     "Calculate scale factors for Multistrike Rating"       },
  { "Analyze Versatility Rating",       "vers",     "Calculate scale factors for Versatility Rating"       },
  { "Analyze Weapon DPS",               "wdps",     "Calculate scale factors for Weapon DPS"               },
  { "Analyze Off-hand Weapon DPS",      "wohdps",   "Calculate scale factors for Off-hand Weapon DPS"      },
  { "Analyze Armor",                    "armor",    "Calculate scale factors for Armor"                    },
  { "Analyze Bonus Armor",              "bonusarmor",   "Calculate scale factors for Bonus Armor"          },
  { "Analyze Latency",                  "",         "Calculate scale factors for Latency"                  },
  { NULL, NULL, NULL }
};

const OptionEntry plotOptions[] =
{
  { "Plot Scaling per Strength",         "str",     "Generate Scaling curve for Strength"         },
  { "Plot Scaling per Agility",          "agi",     "Generate Scaling curve for Agility"          },
  { "Plot Scaling per Stamina",          "sta",     "Generate Scaling curve for Stamina"          },
  { "Plot Scaling per Intellect",        "int",     "Generate Scaling curve for Intellect"        },
  { "Plot Scaling per Spirit",           "spi",     "Generate Scaling curve for Spirit"           },
  { "Plot Scaling per Spell Power",      "sp",      "Generate Scaling curve for Spell Power"      },
  { "Plot Scaling per Attack Power",     "ap",      "Generate Scaling curve for Attack Power"     },
  { "Plot Scaling per Crit Rating",      "crit",    "Generate Scaling curve for Crit Rating"      },
  { "Plot Scaling per Haste Rating",     "haste",   "Generate Scaling curve for Haste Rating"     },
  { "Plot Scaling per Mastery Rating",   "mastery", "Generate Scaling curve for Mastery Rating"   },
  { "Plot Scaling per Multistrike Rating", "mult",  "Generate Scaling curve for Multistrike Rating" },
  { "Plot Scaling per Versatility Rating", "vers",  "Generate Scaling curve for Versatility Rating" },
  { "Plot Scaling per Weapon DPS",       "wdps",    "Generate Scaling curve for Weapon DPS"       },
  { "Plot Scaling per Weapon OH DPS", "wohdps", "Generate Scaling curve for Weapon OH DPS" },
  { "Plot Scaling per Armor",            "armor",   "Generate Scaling curve for Armor"            },
  { "Plot Scaling per Bonus Armor",      "bonusarmor",  "Generate Scaling curve for Bonus Armor"      },
  { NULL, NULL, NULL }
};

const OptionEntry reforgePlotOptions[] =
{
  { "Plot Reforge Options for Spirit",           "spi",     "Generate reforge plot data for Spirit"           },
  { "Plot Reforge Options for Crit Rating",      "crit",    "Generate reforge plot data for Crit Rating"      },
  { "Plot Reforge Options for Haste Rating",     "haste",   "Generate reforge plot data for Haste Rating"     },
  { "Plot Reforge Options for Mastery Rating",   "mastery", "Generate reforge plot data for Mastery Rating"   },
  { "Plot Reforge Options for Multistrike Rating", "mult",  "Generate reforge plot data for Multistrike Rating" },
  { "Plot Reforge Options for Versatility Rating", "vers",  "Generate reforge plot data for Versatility Rating" },
  { "Plot Reforge Options for Bonus Armor Rating", "bonusarmor", "Generate reforge plot data for Bonus Armor" },

  { "Plot Reforge Options for Strength",         "str",     "Generate reforge plot data for Intellect"        },
  { "Plot Reforge Options for Agility",          "agi",     "Generate reforge plot data for Agility"          },
  { "Plot Reforge Options for Stamina",          "sta",     "Generate reforge plot data for Stamina"          },
  { "Plot Reforge Options for Intellect",        "int",     "Generate reforge plot data for Intellect"        },
  { NULL, NULL, NULL }
};
const int reforgePlotOption_cut = 7; // separate between secondary and primary stats

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

} // end unnamed namespace

SC_OptionsTab::SC_OptionsTab( SC_MainWindow* parent ) :
  QTabWidget( parent ), mainWindow( parent )
{
  createGlobalsTab();
  createBuffsDebuffsTab();
  createScalingTab();
  createPlotsTab();
  createReforgePlotsTab();


  QAbstractButton* allBuffs   =   buffsButtonGroup -> buttons().at( 0 );
  QAbstractButton* allDebuffs = debuffsButtonGroup -> buttons().at( 0 );
  QAbstractButton* allScaling = scalingButtonGroup -> buttons().at( 0 );

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
  connect( choice.tmi_boss,           SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.tmi_window,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.show_etmi,          SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.deterministic_rng,  SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.fight_length,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.fight_style,        SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.fight_variance,     SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.iterations,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.num_target,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.player_skill,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.plots_points,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.plots_step,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.print_style,        SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.reforgeplot_amount, SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.reforgeplot_step,   SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.report_pets,        SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.scale_over,         SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.statistics_level,   SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.target_level,       SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.target_race,        SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.threads,            SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.thread_priority,    SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.version,            SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );
  connect( choice.world_lag,          SIGNAL( currentIndexChanged( int ) ), this, SLOT( _optionsChanged() ) );

  connect( buffsButtonGroup,          SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );
  connect( debuffsButtonGroup,        SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );
  connect( scalingButtonGroup,        SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );
  connect( plotsButtonGroup,          SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );
  connect( reforgeplotsButtonGroup,   SIGNAL( buttonClicked( int ) ), this, SLOT( _optionsChanged() ) );

  connect( itemDbOrder,               SIGNAL( itemSelectionChanged() ), this, SLOT( _optionsChanged() ) );
}

void SC_OptionsTab::createGlobalsTab()
{

  // Create left side global options
  QFormLayout* globalsLayout_left = new QFormLayout();
  globalsLayout_left -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
#if SC_BETA
  globalsLayout_left -> addRow(       tr(  "Version" ),        choice.version = createChoice( 3, "Live", "Beta", "Both" ) );
#else
#if SC_USE_PTR
  globalsLayout_left -> addRow(        tr( "Version" ),        choice.version = createChoice( 3, "Live", "PTR", "Both" ) );
#else
  globalsLayout_left -> addRow(        tr( "Version" ),        choice.version = createChoice( 1, "Live" ) );
#endif
#endif
  globalsLayout_left -> addRow( tr(    "Iterations" ),     choice.iterations = addValidatorToComboBox( 1, INT_MAX, createChoice( 8, "1", "100", "1000", "10000", "25000", "50000", "100000", "250000" ) ) );
  globalsLayout_left -> addRow( tr(     "World Lag" ),      choice.world_lag = createChoice( 5, "Super Low - 25 ms", "Low - 50 ms", "Medium - 100 ms", "High - 150 ms", "Australia - 200 ms" ) );
  globalsLayout_left -> addRow( tr(  "Length (sec)" ),   choice.fight_length = addValidatorToComboBox( 1, 1000, createChoice( 10, "100", "150", "200", "250", "300", "350", "400", "450", "500", "600" ) ) );
  globalsLayout_left -> addRow( tr(   "Vary Length" ), choice.fight_variance = createChoice( 3, "0%", "10%", "20%" ) );
  globalsLayout_left -> addRow( tr(   "Fight Style" ),    choice.fight_style = createChoice( 6, "Patchwerk", "HecticAddCleave", "HelterSkelter", "Ultraxion", "LightMovement", "HeavyMovement" ) );
  globalsLayout_left -> addRow( tr(  "Target Level" ),   choice.target_level = createChoice( 4, "Raid Boss", "5-Man Heroic", "5-Man Normal", "Max Player Level" ) );
  globalsLayout_left -> addRow( tr(   "Target Race" ),    choice.target_race = createChoice( 7, "Humanoid", "Beast", "Demon", "Dragonkin", "Elemental", "Giant", "Undead" ) );
  globalsLayout_left -> addRow( tr(   "Num Enemies" ),     choice.num_target = createChoice( 9, "1", "2", "3", "4", "5", "6", "7", "8", "20" ) );
  globalsLayout_left -> addRow( tr( "Challenge Mode" ),   choice.challenge_mode = createChoice( 2, "Disabled", "Enabled" ) );
  globalsLayout_left -> addRow( tr(  "Player Skill" ),   choice.player_skill = createChoice( 4, "Elite", "Good", "Average", "Ouch! Fire is hot!" ) );
  globalsLayout_left -> addRow( tr(       "Threads" ),        choice.threads = addValidatorToComboBox( 1, QThread::idealThreadCount(), createChoiceFromRange( 1, QThread::idealThreadCount() ) ) );
  globalsLayout_left -> addRow( tr(  "Thread Priority" ), choice.thread_priority = createChoice( 5, "Highest", "High", "Normal", "Lower", "Lowest" ) );
  globalsLayout_left -> addRow( tr( "Armory Region" ),  choice.armory_region = createChoice( 5, "US", "EU", "TW", "CN", "KR" ) );
  globalsLayout_left -> addRow( tr(   "Armory Spec" ),    choice.armory_spec = createChoice( 2, "Active", "Inactive" ) );
  globalsLayout_left -> addRow( tr(  "Default Role" ),   choice.default_role = createChoice( 4, "Auto", "DPS", "Heal", "Tank" ) );
  globalsLayout_left -> addRow( tr( "TMI Standard Boss" ),   choice.tmi_boss = createChoice( 8, "Custom", "T17M", "T17H", "T17N", "T16H25", "T16H10", "T16N25", "T16N10" ) );
  globalsLayout_left -> addRow( tr( "TMI Window (sec)" ),  choice.tmi_window = createChoice( 10, "0", "2", "3", "4", "5", "6", "7", "8", "9", "10" ) );
  globalsLayout_left -> addRow( tr( "Show ETMI" ),          choice.show_etmi = createChoice( 2, "Only when in group", "Always" ) );

  QGroupBox* globalsGroupBox_left = new QGroupBox( tr( "Basic Options" ) );
  globalsGroupBox_left -> setLayout( globalsLayout_left );

  // Create right side of global options
  QFormLayout* globalsLayout_right = new QFormLayout();
  globalsLayout_right -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  globalsLayout_right -> addRow( tr( "Generate Debug" ),                choice.debug = createChoice( 3, "None", "Log Only", "Gory Details" ) );
  globalsLayout_right -> addRow( tr( "Report Pets Separately" ),  choice.report_pets = createChoice( 2, "Yes", "No" ) );
  globalsLayout_right -> addRow( tr( "Report Print Style" ),      choice.print_style = createChoice( 3, "MoP", "White", "Classic" ) );
  globalsLayout_right -> addRow( tr( "Statistics Level" ),   choice.statistics_level = createChoice( 4, "0", "1", "2", "3" ) );
  globalsLayout_right -> addRow( tr( "Deterministic RNG" ), choice.deterministic_rng = createChoice( 2, "Yes", "No" ) );
  QPushButton* resetb = new QPushButton( tr("Reset all Settings"), this );
  connect( resetb, SIGNAL(clicked()), this, SLOT(_resetallSettings()) );
  globalsLayout_right -> addWidget( resetb );


  createItemDataSourceSelector( globalsLayout_right );

  QGroupBox* globalsGroupBox_right = new QGroupBox( tr( "Advanced Options" ) );
  globalsGroupBox_right -> setLayout( globalsLayout_right );

  QHBoxLayout* globalsLayout = new QHBoxLayout();
  globalsLayout -> addWidget( globalsGroupBox_left, 2 );
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
  QVBoxLayout* buffsLayout = new QVBoxLayout(); // Buff Layout
  buffsButtonGroup = new QButtonGroup();
  buffsButtonGroup -> setExclusive( false );
  for ( int i = 0; buffOptions[ i ].label; ++i )
  {
    QCheckBox* checkBox = new QCheckBox( buffOptions[ i ].label );

    checkBox -> setToolTip( buffOptions[ i ].tooltip );
    buffsButtonGroup -> addButton( checkBox );
    buffsLayout -> addWidget( checkBox );
  }
  buffsLayout -> addStretch( 1 );

  QGroupBox* buffsGroupBox = new QGroupBox( tr( "Buffs" ) ); // Buff Widget
  buffsGroupBox -> setLayout( buffsLayout );

  // Debuffs
  QVBoxLayout* debuffsLayout = new QVBoxLayout(); // Debuff Layout
  debuffsButtonGroup = new QButtonGroup();
  debuffsButtonGroup -> setExclusive( false );
  for ( int i = 0; debuffOptions[ i ].label; ++i )
  {
    QCheckBox* checkBox = new QCheckBox( debuffOptions[ i ].label );

    checkBox -> setToolTip( debuffOptions[ i ].tooltip );
    debuffsButtonGroup -> addButton( checkBox );
    debuffsLayout -> addWidget( checkBox );
  }
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
  QVBoxLayout* scalingLayout = new QVBoxLayout();
  scalingButtonGroup = new QButtonGroup();
  scalingButtonGroup -> setExclusive( false );
  for ( int i = 0; scalingOptions[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( scalingOptions[ i ].label );

    checkBox -> setToolTip( scalingOptions[ i ].tooltip );
    scalingButtonGroup -> addButton( checkBox );
    scalingLayout -> addWidget( checkBox );
  }
  //scalingLayout->addStretch( 1 );
  QGroupBox* scalingGroupBox = new QGroupBox();
  scalingGroupBox -> setLayout( scalingLayout );

  QFormLayout* scalingLayout2 = new QFormLayout();
  scalingLayout2 -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  scalingLayout2 -> addRow( tr( "Center Scale Delta" ),  choice.center_scale_delta = createChoice( 2, "Yes", "No" ) );
  scalingLayout2 -> addRow( tr( "Scale Over" ),  choice.scale_over = createChoice( 9, "Default", "DPS", "HPS", "DTPS", "HTPS", "Raid_DPS", "Raid_HPS", "TMI", "ETMI" ) );

  scalingLayout -> addLayout( scalingLayout2 );

  QScrollArea* scalingGroupBoxScrollArea = new QScrollArea;
  scalingGroupBoxScrollArea -> setWidget( scalingGroupBox );
  scalingGroupBoxScrollArea -> setWidgetResizable( true );
  addTab( scalingGroupBoxScrollArea, tr ( "Scaling" ) );
}

void SC_OptionsTab::createPlotsTab()
{
  QFormLayout* plotsLayout = new QFormLayout();
  plotsLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  // Create Combo Boxes
  choice.plots_points = addValidatorToComboBox( 1, INT_MAX, createChoice( 4, "10", "20", "30", "40" ) );
  plotsLayout -> addRow( tr( "Number of Plot Points" ), choice.plots_points );

  choice.plots_step = addValidatorToComboBox( 1, INT_MAX, createChoice( 6, "25", "50", "100", "150", "200", "250" ) );
  plotsLayout -> addRow( tr( "Plot Step Amount" ), choice.plots_step );

  plotsButtonGroup = new QButtonGroup();
  plotsButtonGroup -> setExclusive( false );
  for ( int i = 0; plotOptions[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( plotOptions[ i ].label );
    checkBox -> setToolTip( plotOptions[ i ].tooltip );
    plotsButtonGroup -> addButton( checkBox );
    plotsLayout -> addWidget( checkBox );
  }
  QGroupBox* plotsGroupBox = new QGroupBox();
  plotsGroupBox -> setLayout( plotsLayout );

  QScrollArea* plotsGroupBoxScrollArea = new QScrollArea;
  plotsGroupBoxScrollArea -> setWidget( plotsGroupBox );
  plotsGroupBoxScrollArea -> setWidgetResizable( true );
  addTab( plotsGroupBoxScrollArea, "Plots" );
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

  messageText = new QLabel( "Secondary Stats" );
  reforgePlotsLayout -> addRow( messageText );

  reforgeplotsButtonGroup = new SC_ReforgeButtonGroup( this );
  reforgeplotsButtonGroup -> setExclusive( false );
  for ( int i = 0; i < reforgePlotOption_cut && reforgePlotOptions[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( reforgePlotOptions[ i ].label );
    checkBox -> setToolTip( reforgePlotOptions[ i ].tooltip );
    reforgeplotsButtonGroup -> addButton( checkBox );
    reforgePlotsLayout -> addWidget( checkBox );
    QObject::connect( checkBox, SIGNAL( stateChanged( int ) ),
                      reforgeplotsButtonGroup, SLOT( setSelected( int ) ) );
  }

  messageText = new QLabel( "\n" + tr( "Primary Stats" ) );
  reforgePlotsLayout -> addRow( messageText );

  for ( int i = reforgePlotOption_cut; reforgePlotOptions[ i ].label; i++ )
  {
    QCheckBox* checkBox = new QCheckBox( reforgePlotOptions[ i ].label );
    checkBox -> setToolTip( reforgePlotOptions[ i ].tooltip );
    reforgeplotsButtonGroup -> addButton( checkBox );
    reforgePlotsLayout -> addWidget( checkBox );
    QObject::connect( checkBox, SIGNAL( stateChanged( int ) ),
                      reforgeplotsButtonGroup, SLOT( setSelected( int ) ) );
  }

  QGroupBox* reforgeplotsGroupBox = new QGroupBox();
  reforgeplotsGroupBox -> setLayout( reforgePlotsLayout );

  QScrollArea* reforgeplotsGroupBoxScrollArea = new QScrollArea;
  reforgeplotsGroupBoxScrollArea -> setWidget( reforgeplotsGroupBox );
  reforgeplotsGroupBoxScrollArea -> setWidgetResizable( true );
  addTab( reforgeplotsGroupBoxScrollArea, tr( "Reforge Plots" ) );
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
    int default_index = choice -> findText( default_value );
    if ( default_index != -1 )
      choice -> setCurrentIndex( default_index );
  }
  else
  {
    choice -> setCurrentIndex( 0 );
  }
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

void SC_OptionsTab::decodeOptions()
{
  QSettings settings;
  settings.beginGroup( "options" );
  load_setting( settings, "version", choice.iterations );
  load_setting( settings, "iterations", choice.iterations, "10000" );
  load_setting( settings, "fight_length", choice.fight_length, "450" );
  load_setting( settings, "fight_variance", choice.fight_variance, "20%" );
  load_setting( settings, "fight_style", choice.fight_style );
  load_setting( settings, "target_race", choice.target_race );
  load_setting( settings, "num_target", choice.num_target );
  load_setting( settings, "player_skill", choice.player_skill );
  load_setting( settings, "threads", choice.threads, QString::number( QThread::idealThreadCount() ) );
  load_setting( settings, "thread_priority", choice.thread_priority, "Lowest" );
  load_setting( settings, "armory_region", choice.armory_region );
  load_setting( settings, "armory_spec", choice.armory_spec );
  load_setting( settings, "default_role", choice.default_role );
  load_setting( settings, "tmi_boss", choice.tmi_boss, "T17M" );
  load_setting( settings, "tmi_window_global", choice.tmi_window, "6" );
  load_setting( settings, "show_etmi", choice.show_etmi );
  load_setting( settings, "world_lag", choice.world_lag, "Medium - 100 ms" );
  load_setting( settings, "target_level", choice.target_level );
  load_setting( settings, "report_pets", choice.report_pets, "No" );
  load_setting( settings, "print_style", choice.print_style );
  load_setting( settings, "statistics_level", choice.statistics_level, "1" );
  load_setting( settings, "deterministic_rng", choice.deterministic_rng, "No" );
  load_setting( settings, "challenge_mode", choice.challenge_mode );

  load_setting( settings, "center_scale_delta", choice.center_scale_delta, "No" );
  load_setting( settings, "scale_over", choice.scale_over );

  load_setting( settings, "plot_points", choice.plots_points, "40" );
  load_setting( settings, "plot_step", choice.plots_step, "50" );

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

// Encode all options/setting into a string ( to be able to save it to the history )
// Decode / Encode order needs to be equal!

void SC_OptionsTab::encodeOptions()
{
  QSettings settings;
  settings.beginGroup( "options" );
  settings.setValue( "version", choice.version -> currentText() );
  settings.setValue( "iterations", choice.iterations -> currentText() );
  settings.setValue( "fight_length", choice.fight_length -> currentText() );
  settings.setValue( "fight_variance", choice.fight_variance -> currentText() );
  settings.setValue( "fight_style", choice.fight_style -> currentText() );
  settings.setValue( "target_race", choice.target_race -> currentText() );
  settings.setValue( "num_target", choice.num_target -> currentText() );
  settings.setValue( "player_skill", choice.player_skill -> currentText() );
  settings.setValue( "threads", choice.threads -> currentText() );
  settings.setValue( "thread_priority", choice.thread_priority -> currentText() );
  settings.setValue( "armory_region", choice.armory_region -> currentText() );
  settings.setValue( "armory_spec", choice.armory_spec -> currentText() );
  settings.setValue( "default_role", choice.default_role -> currentText() );
  settings.setValue( "tmi_boss", choice.tmi_boss -> currentText() );
  settings.setValue( "tmi_window_global", choice.tmi_window -> currentText() );
  settings.setValue( "show_etmi", choice.show_etmi -> currentText() );
  settings.setValue( "world_lag", choice.world_lag -> currentText() );
  settings.setValue( "target_level", choice.target_level -> currentText() );
  settings.setValue( "report_pets", choice.report_pets -> currentText() );
  settings.setValue( "print_style", choice.print_style -> currentText() );
  settings.setValue( "statistics_level", choice.statistics_level -> currentText() );
  settings.setValue( "deterministic_rng", choice.deterministic_rng -> currentText() );
  settings.setValue( "center_scale_delta", choice.center_scale_delta -> currentText() );
  settings.setValue( "scale_over", choice.scale_over -> currentText() );
  settings.setValue( "challenge_mode", choice.challenge_mode -> currentText() );

  QString encoded;

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
                                        "    beginning %3s into the fight" ).arg( 4 ).arg( 10 ).arg( 10 ) );

  choice.target_race -> setToolTip( tr( "Race of the target and any adds." ) );

  choice.challenge_mode -> setToolTip( tr( "Enables/Disables the challenge mode setting, downscaling items to level 660." ) );

  choice.num_target -> setToolTip( tr( "Number of enemies." ) );

  choice.target_level -> setToolTip( tr( "Level of the target and any adds." ) );

  choice.player_skill -> setToolTip( tr( "Elite:       No mistakes.  No cheating either." ) + "\n" +
                                     tr( "Fire-is-Hot: Frequent DoT-clipping and skipping high-priority abilities." ) );

  choice.threads -> setToolTip( tr( "Match the number of CPUs for optimal performance.\n"
                                    "Most modern desktops have at least two CPU cores." ) );

  choice.thread_priority -> setToolTip( tr( "This can allow for a more responsive computer while simulations are running.\n"
                                            "When set to 'Lowest', it will be possible to use your computer as normal while SimC runs in the background." ) );

  choice.armory_region -> setToolTip( tr( "United States, Europe, Taiwan, China, Korea" ) );

  choice.armory_spec -> setToolTip( tr( "Controls which Talent/Glyph specification is used when importing profiles from the Armory." ) );

  choice.default_role -> setToolTip( tr( "Specify the character role during import to ensure correct action priority list." ) );

  choice.tmi_boss -> setToolTip( tr( "Specify boss damage output based on a TMI standard.\n"
                                     "Leaving at *custom* will use the SimC defaults unless overwritten by the user." ) );

  choice.tmi_window -> setToolTip( tr( "Specify window duration for calculating TMI. Default is 6 sec.\n"
                                       "Reducing this increases the metric's sensitivity to shorter damage spikes.\n"
                                       "Set to 0 if you want to vary on a per-player basis in the Simulate tab using \"tmi_window=#\"." ) );

  choice.show_etmi -> setToolTip( tr( "Controls when ETMI is displayed in the HTML report.\n"
                                      "TMI only includes damage taken and self-healing/absorbs, and treats overhealing as effective healing.\n"
                                      "ETMI includes all sources of healing and absorption, and ignores overhealing." ) );

  choice.report_pets -> setToolTip( tr( "Specify if pets get reported separately in detail." ) );

  choice.print_style -> setToolTip( tr( "Specify HTML report print style." ) );


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

  choice.plots_points -> setToolTip( tr( "The number of points that will appear on the graph" ) );
  choice.plots_step -> setToolTip( tr( "The delta between two points of the graph.\n"
                                       "The deltas on the horizontal axis will be within the [-points * steps / 2 ; +points * steps / 2] interval" ) );

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
  options += "iterations=" + choice.iterations->currentText() + "\n";
  if ( choice.iterations->currentText() == "10000" )
  {
    options += "dps_plot_iterations=1000\n";
  }

  const char *world_lag[] = { "0.025", "0.05", "0.1", "0.15", "0.20" };
  options += "default_world_lag=";
  options += world_lag[ choice.world_lag->currentIndex() ];
  options += "\n";

  options += "max_time=" + choice.fight_length->currentText() + "\n";

  options += "vary_combat_length=";
  const char *variance[] = { "0.0", "0.1", "0.2" };
  options += variance[ choice.fight_variance->currentIndex() ];
  options += "\n";

  options += "fight_style=" + choice.fight_style->currentText() + "\n";

  if ( choice.challenge_mode -> currentIndex() > 0 )
    options += "challenge_mode=1\n";

  if ( choice.show_etmi -> currentIndex() != 0 )
    options += "show_etmi=1\n";

  if ( choice.tmi_window -> currentIndex() != 0 )
    options += "tmi_window_global=" + choice.tmi_window-> currentText() + "\n";

  if ( choice.tmi_boss -> currentIndex() != 0 )
  {
    // boss setup
    options += "enemy=TMI_Standard_Boss_";
    options += choice.tmi_boss -> currentText();
    options += "\n";
    options += "tmi_boss=" + choice.tmi_boss -> currentText();
    options += "\n";
    options += "role=tank\n";
    options += "position=front\n";
  }
  else
  {
    static const char* const targetlevel[] = { "3", "2", "1", "0" };
    options += "target_level+=";
    options += targetlevel[ choice.target_level -> currentIndex() ];
    options += "\n";
    options += "target_race=" + choice.target_race->currentText() + "\n";
  }
  options += "default_skill=";
  const char *skill[] = { "1.0", "0.9", "0.75", "0.50" };
  options += skill[ choice.player_skill->currentIndex() ];
  options += "\n";

  options += "optimal_raid=0\n";
  QList<QAbstractButton*> buttons = buffsButtonGroup -> buttons();
  for ( int i = 1; buffOptions[ i ].label; i++ )
  {
    options += buffOptions[ i ].option;
    options += "=";
    options += buttons.at( i )->isChecked() ? "1" : "0";
    options += "\n";
  }
  buttons = debuffsButtonGroup->buttons();
  for ( int i = 1; debuffOptions[ i ].label; i++ )
  {
    options += debuffOptions[ i ].option;
    options += "=";
    options += buttons.at( i )->isChecked() ? "1" : "0";
    options += "\n";
  }

  if ( choice.deterministic_rng->currentIndex() == 0 )
  {
    options += "deterministic_rng=1\n";
  }

  return options;
}

QString SC_OptionsTab::mergeOptions()
{
  QString options = "### Begin GUI options ###\n";
  QString spell_query = mainWindow -> cmdLine -> commandLineText();
  if ( spell_query.startsWith( "spell_query" ) )
  {
    options += spell_query;
    return options;
  }

  options += get_globalSettings();
  options += "threads=" + choice.threads -> currentText() + "\n";
  options += "thread_priority=" + choice.thread_priority -> currentText() + "\n";

  QList<QAbstractButton*> buttons = scalingButtonGroup -> buttons();
  for ( int i = 2; i < buttons.size(); i++ )
  {
    if ( buttons.at( i ) -> isChecked() )
    {
      options += "calculate_scale_factors=1\n";
      break;
    }
  }

  if ( buttons.at( 1 )->isChecked() ) options += "positive_scale_delta=1\n";
  if ( buttons.at( buttons.size() - 1 )->isChecked() ) options += "scale_lag=1\n";
  if ( buttons.at( 15 )->isChecked() || buttons.at( 17 )->isChecked() ) options += "weapon_speed_scale_factors=1\n";

  options += "scale_only=none";
  for ( int i = 2; scalingOptions[ i ].label; i++ )
  {
    if ( buttons.at( i ) -> isChecked() )
    {
      options += ",";
      options += scalingOptions[ i ].option;
    }
  }
  options += "\n";

  if ( choice.center_scale_delta->currentIndex() == 0 )
  {
    options += "center_scale_delta=1\n";
  }

  if ( choice.scale_over -> currentIndex() != 0 )
  {
    options += "scale_over=";
    options +=  choice.scale_over -> currentText();
    options += "\n";
  }

  options += "dps_plot_stat=none";
  buttons = plotsButtonGroup->buttons();
  for ( int i = 0; plotOptions[ i ].label; i++ )
  {
    if ( buttons.at( i )->isChecked() )
    {
      options += ",";
      options += plotOptions[ i ].option;
    }
  }
  options += "\n";

  options += "dps_plot_points=" + choice.plots_points -> currentText() + "\n";
  options += "dps_plot_step=" + choice.plots_step -> currentText() + "\n";

  options += "reforge_plot_stat=none";
  buttons = reforgeplotsButtonGroup->buttons();
  for ( int i = 0; reforgePlotOptions[ i ].label; i++ )
  {
    if ( buttons.at( i )->isChecked() )
    {
      options += ",";
      options += reforgePlotOptions[ i ].option;
    }
  }
  options += "\n";

  options += "reforge_plot_amount=" + choice.reforgeplot_amount -> currentText() + "\n";
  options += "reforge_plot_step=" + choice.reforgeplot_step -> currentText() + "\n";

  if ( choice.statistics_level->currentIndex() >= 0 )
  {
    options += "statistics_level=" + choice.statistics_level->currentText() + "\n";
  }

  if ( choice.report_pets->currentIndex() != 1 )
  {
    options += "report_pets_separately=1\n";
  }

  if ( choice.print_style -> currentIndex() != 0 )
  {
    options += "print_styles=";
    options += util::to_string( choice.print_style -> currentIndex() ).c_str();
    options += "\n";
  }
  options += "### End GUI options ###\n"

      "### Begin simulateText ###\n";
  options += mainWindow -> simulateTab -> current_Text() -> toPlainText();
  options += "\n"
      "### End simulateText ###\n";

  if ( choice.num_target -> currentIndex() >= 1 )
    options += "desired_targets=" + QString::number( choice.num_target -> currentIndex() + 1 ) + "\n";

  options += "### Begin overrides ###\n";
  options += mainWindow -> overridesText -> toPlainText();
  options += "\n";
  options += "### End overrides ###\n"

      "### Begin command line ###\n";
  options += mainWindow -> cmdLine -> commandLineText();
  options += "\n"
      "### End command line ###\n"

      "### Begin final options ###\n";
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

void SC_OptionsTab::createItemDataSourceSelector( QFormLayout* layout )
{
  itemDbOrder = new QListWidget( this );
  itemDbOrder -> setDragDropMode( QAbstractItemView::InternalMove );
  itemDbOrder -> setResizeMode( QListView::Fixed );
  itemDbOrder -> setSelectionRectVisible( false );
  itemDbOrder -> setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
  itemDbOrder -> setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

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
