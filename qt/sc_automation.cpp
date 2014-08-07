// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
  This file contains all of the methods required for automated comparison sims
*/
#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"

///////////////////////////////////////////////////////////////////////////////
////
////                        Logic Stuff
////
///////////////////////////////////////////////////////////////////////////////


  // stuff goes here!
QString automation::tokenize( QString qstr )
{
  std::string temp = qstr.toLocal8Bit().constData();
  util::tokenize( temp );
  
  return QString::fromStdString( temp );
}

QString automation::do_something( int sim_type,
                                  QString player_class,
                                  QString player_spec,
                                  QString player_race,
                                  QString player_level,
                                  QString player_talents,
                                  QString player_glyphs,
                                  QString player_gear,
                                  QString player_rotation,
                                  QString advanced_text
                                ) 
{
  // Dummy code - this just sets up a profile using the strings we pass. 
  // This is where we'll actually split off into different automation schemes.

  QString profile;
  QString base_profile_info; // class, spec, race & level definition

  // basic profile information
  base_profile_info += tokenize( player_class ) + "=Name\n";
  base_profile_info += "specialization=" + tokenize( player_spec ) + "\n";
  base_profile_info += "race=" + tokenize( player_race ) + "\n";
  base_profile_info += "level=" + player_level + "\n";

  // simulation type check
  switch ( sim_type )
  {
    case 1: // talent simulation
      return auto_talent_sim( base_profile_info, player_glyphs, player_gear, player_rotation, advanced_text );
    case 2: // glyph simulation
      return auto_glyph_sim( base_profile_info, player_talents, player_gear, player_rotation, advanced_text );
    case 3: // rotation simulation
      return auto_rotation_sim( base_profile_info, player_talents, player_glyphs, player_rotation, advanced_text );
    case 4: // gear simulation
      return auto_gear_sim( base_profile_info, player_talents, player_glyphs, player_gear, advanced_text );
    default: // default profile creation
      profile += base_profile_info;
      profile += "talents=" + player_talents + "\n";
      profile += "glyphs=" + player_glyphs + "\n";
      profile += player_gear + "\n";
      profile += player_rotation;
      return profile;
  }

  return profile;
}

  // Method for profile creation for the specific TALENT simulation
QString automation::auto_talent_sim( QString base_profile_info, 
                                     QString advanced_talents,
                                     QString player_glyphs,
                                     QString player_gear,
                                     QString player_rotation
                                   )
{
  QString profile = "NYI";

  return profile;
}

  // Method for profile creation for the specific GLYPHS simulation
QString automation::auto_glyph_sim( QString base_profile_info,
                                    QString player_talents,
                                    QString player_gear,
                                    QString player_rotation,
                                    QString advanced_text
                                  )
{
  QString profile = "NYI";
  
  return profile;
}

  // Method for profile creation for the specific GEAR simulation
QString automation::auto_gear_sim( QString base_profile_info,
                                   QString player_talents,
                                   QString player_glyphs,
                                   QString player_rotation,
                                   QString advanced_text
                                 )
{
  QString profile = "NYI";

  return profile;
}

// Method for profile creation for the specific ROTATION simulation
QString automation::auto_rotation_sim( QString base_profile_info,
                                       QString player_talents,
                                       QString player_glyphs,
                                       QString player_gear,
                                       QString advanced_text
                                     )
{
  QString profile = "NYI";

  return profile;
}


///////////////////////////////////////////////////////////////////////////////
////
////                        GUI Stuff
////
///////////////////////////////////////////////////////////////////////////////



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


// Method to set the "Spec" drop-down based on "Class" selection
void SC_ImportTab::setSpecDropDown( const int player_class )
{
  // If we have a fourth spec, remove it here
  if ( choice.player_spec -> count() > 3 )
    choice.player_spec -> removeItem( 3 );

  switch ( player_class )
  {
    case 0:
      choice.player_spec -> setItemText( 0, "Blood" );
      choice.player_spec -> setItemText( 1, "Frost" );
      choice.player_spec -> setItemText( 2, "Unholy" );
      break;
    case 1:
      choice.player_spec -> setItemText( 0, "Balance" );
      choice.player_spec -> setItemText( 1, "Feral" );
      choice.player_spec -> setItemText( 2, "Guardian" );
      choice.player_spec -> addItem( "Restoration" );
      break;
    case 2:
      choice.player_spec -> setItemText( 0, "Beast Mastery" );
      choice.player_spec -> setItemText( 1, "Marksmanship" );
      choice.player_spec -> setItemText( 2, "Survival" );
      break;
    case 3:
      choice.player_spec -> setItemText( 0, "Arcane" );
      choice.player_spec -> setItemText( 1, "Fire" );
      choice.player_spec -> setItemText( 2, "Frost" );
      break;
    case 4:
      choice.player_spec -> setItemText( 0, "Brewmaster" );
      choice.player_spec -> setItemText( 1, "Mistweaver" );
      choice.player_spec -> setItemText( 2, "Windwalker" );
      break;
    case 5:
      choice.player_spec -> setItemText( 0, "Holy" );
      choice.player_spec -> setItemText( 1, "Protection" );
      choice.player_spec -> setItemText( 2, "Retribution" );
      break;
    case 6:
      choice.player_spec -> setItemText( 0, "Discipline" );
      choice.player_spec -> setItemText( 1, "Holy" );
      choice.player_spec -> setItemText( 2, "Shadow" );
      break;
    case 7:
      choice.player_spec -> setItemText( 0, "Assassination" );
      choice.player_spec -> setItemText( 1, "Combat" );
      choice.player_spec -> setItemText( 2, "Subtlety" );
      break;
    case 8:
      choice.player_spec -> setItemText( 0, "Elemental" );
      choice.player_spec -> setItemText( 1, "Enhancement" );
      choice.player_spec -> setItemText( 2, "Restoration" );
      break;
    case 9:
      choice.player_spec -> setItemText( 0, "Affliction" );
      choice.player_spec -> setItemText( 1, "Demonology" );
      choice.player_spec -> setItemText( 2, "Destruction" );
      break;
    case 10:
      choice.player_spec -> setItemText( 0, "Arms" );
      choice.player_spec -> setItemText( 1, "Fury" );
      choice.player_spec -> setItemText( 2, "Protection" );
      break;
    default:
      choice.player_spec -> setItemText( 0, "What" );
      choice.player_spec -> setItemText( 1, "The" );
      choice.player_spec -> setItemText( 2, "F*&!" );
      break;
  }
}

// 
QString sidebarText[ 11 ][ 4 ] = {
  { "Blood shorthand goes here.", "Frost shorthand goes here", "Unholy shorthand goes here", "N/A" },
  { "Balance shorthand goes here.", "Feral shorthand goes here.", "Guardian shorthand goes here.", "Restoration shorthand goes here." },
  { "", "", "", "N/A" },
  { "", "", "", "N/A" },
  { "Holy shorthand goes here.", "Protection shorthand goes here.", "Retribution shorthand goes here.", "N/A" },
  { "", "", "", "N/A" },
  { "", "", "", "N/A" },
  { "", "", "", "N/A" },
  { "", "", "", "N/A" },
  { "Affliction shorthand goe shere.", "Demonology shorthand goes here.", "Destruction shorthand goes here.", "N/A" },
  { "Arms shorthand goes here.", "Fury shorthand goes here.", "Protection shorthand goes here.", "N/A" },
};

QString advancedText[ 5 ] = {
  "Unused", "Talent Configurations", "Glyph Configurations", "Gear Configurations", "Rotation Shorthands"
};

// method to set the sidebar text based on class slection
void SC_ImportTab::setSidebarClassText()
{
  textbox.sidebar -> setText( sidebarText[ choice.player_class -> currentIndex() ][ choice.player_spec -> currentIndex() ] );
}

void SC_ImportTab::compTypeChanged( const int comp )
{
  advancedLabel -> setText( advancedText[ comp ] );
  
  // TODO: set the text of the box with some sort of default for each case

  textbox.advanced -> setDisabled( false );
  textbox.gear     -> setDisabled( false );
  textbox.glyphs   -> setDisabled( false );
  textbox.rotation -> setDisabled( false );
  textbox.talents  -> setDisabled( false );

  switch ( comp )
  {
    case 0: 
      textbox.advanced -> setDisabled( true );break;
    case 1:
      textbox.talents -> setDisabled( true );break;
    case 2:
      textbox.glyphs -> setDisabled( true );break;
    case 3:
      textbox.gear -> setDisabled( true );break;
    case 4:
      textbox.rotation -> setDisabled( true );break;
  }
}

void SC_MainWindow::startAutomationImport( int tab )
{
  QString profile;

  profile = automation::do_something( importTab -> choice.comp_type -> currentIndex(),
                                      importTab -> choice.player_class -> currentText(),
                                      importTab -> choice.player_spec -> currentText(),
                                      importTab -> choice.player_race -> currentText(),
                                      importTab -> choice.player_level -> currentText(),
                                      importTab -> textbox.talents -> document() -> toPlainText(),
                                      importTab -> textbox.glyphs -> document() -> toPlainText(),
                                      importTab -> textbox.gear -> document() -> toPlainText(),
                                      importTab -> textbox.rotation -> document() -> toPlainText(),
                                      importTab -> textbox.advanced -> document() -> toPlainText()
                                    );

  simulateTab -> add_Text( profile,  tr( "Testing" ) );
  
  mainTab -> setCurrentTab( TAB_SIMULATE );
}

void SC_ImportTab::createAutomationTab()
{ 
  // layout building based on 
  // http://qt-project.org/doc/qt-4.8/layouts-basiclayouts.html

  // This scroll area is the parent of the entire tab
  QScrollArea* automationTabScrollArea = new QScrollArea;

  // Define a grid Layout which we will eventually apply to the scroll area
  QGridLayout* gridLayout = new QGridLayout;

  // Now we start adding things to the grid layout

  // Element (0,0) is a GroupBox containing a FormLayout with one ComboBox (choice of sim type)
  // Create box and add to Layout
  QGroupBox* compGroupBox = new QGroupBox( tr( "Comparison Form Layout" ) );
  gridLayout -> addWidget( compGroupBox, 0, 0, 0 );

  // Define a layout for the box
  QFormLayout* compLayout = new QFormLayout();
  compLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  
  // Create Combo Box and add it to the layout
  choice.comp_type = createChoice( 5 , "None", "Talents", "Glyphs", "Gear", "Rotation" );
  compLayout -> addRow( tr( "Comparison Type" ), choice.comp_type );

  // Apply the layout to the compGroupBox
  compGroupBox -> setLayout( compLayout );


  // Elements (1,0) & (2,0) are a QLabel and QGroupBox for some of the defaults  
  QLabel* defaultsLabel = new QLabel( tr( "Defaults" ) );
  gridLayout -> addWidget( defaultsLabel, 1, 0, 0 );

  // create a box for the defaults section
  QGroupBox* defaultsGroupBox = new QGroupBox(tr("Defaults Form layout"));
  gridLayout -> addWidget( defaultsGroupBox, 2, 0, 0 );

  // define a FormLayout for the GroupBox
  QFormLayout* defaultsFormLayout = new QFormLayout();
  defaultsFormLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );

  // Create Combo Boxes and add to the FormLayout
  choice.player_class = createChoice( 11 , "Death Knight", "Druid", "Hunter", "Mage", "Monk", "Paladin", "Priest", "Rogue", "Shaman", "Warlock", "Warrior" );
  defaultsFormLayout -> addRow( tr( "Class" ), choice.player_class );

  choice.player_spec = createChoice( 3, "1", "2", "3" );
  choice.player_spec -> setSizeAdjustPolicy( QComboBox::AdjustToContents );
  setSpecDropDown( choice.player_class -> currentIndex() );
  defaultsFormLayout -> addRow( tr( "Spec" ), choice.player_spec );

  choice.player_race = createChoice( 13, "Blood Elf", "Draenei", "Dwarf", "Gnome", "Goblin", "Human", "Night Elf", "Orc", "Pandaren", "Tauren", "Troll", "Undead", "Worgen");
  defaultsFormLayout -> addRow( tr( "Race" ), choice.player_race );

  choice.player_level = createChoice( 2, "100", "90" );
  defaultsFormLayout -> addRow( tr( "Level" ), choice.player_level );

  // Create text boxes for default talents and glyphs, and add them to the FormLayout
  textbox.talents = new QTextEdit;
  textbox.talents -> setMinimumHeight( 15 );
  textbox.talents -> setFixedHeight( 15 );
  textbox.talents -> setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  textbox.talents -> setPlainText( "0000000" );
  defaultsFormLayout -> addRow( tr("Default Talents" ), textbox.talents );
  
  textbox.glyphs = new QTextEdit;
  textbox.glyphs -> setMinimumHeight( 15 );
  textbox.glyphs -> setFixedHeight( 15 );
  textbox.glyphs -> setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  textbox.glyphs -> setPlainText( "focused_shield/alabaster_shield" );
  defaultsFormLayout -> addRow( tr("Default Glypyhs" ), textbox.glyphs );

  // set the GroupBox's layout now that we've defined it
  defaultsGroupBox -> setLayout( defaultsFormLayout );

  
  // Elements (3,0) - (6,0) are the default gear and rotation labels and text boxes
  
  // Create a label and an edit box for gear
  QLabel* gearLabel = new QLabel( tr( "Default Gear" ) );
  textbox.gear = new SC_TextEdit;
  //textbox.gear -> resize( 200, 250 );
  textbox.gear -> setPlainText( "head=\nneck=\n " );  
  // assign the label and edit box to cells
  gridLayout -> addWidget( gearLabel,    3, 0, 0 );
  gridLayout -> addWidget( textbox.gear, 4, 0, 0 );
  
  // and again for rotation
  QLabel* rotationLabel = new QLabel( tr( "Default Rotation" ) );
  textbox.rotation = new SC_TextEdit;
  //textbox.rotation -> resize( 200, 250 );
  textbox.rotation -> setPlainText( "actions=/auto_attack\n" );  
  gridLayout -> addWidget( rotationLabel,    5, 0, 0 );
  gridLayout -> addWidget( textbox.rotation, 6, 0, 0 );


  // Elements (2,1) - (6,1) are the Advanced text box

  advancedLabel = new QLabel( tr( "Advanced Text Box" ) );
  textbox.advanced = new SC_TextEdit;
  textbox.advanced -> setPlainText( "default" );
  gridLayout -> addWidget( advancedLabel,    1, 1, 0 );
  gridLayout -> addWidget( textbox.advanced, 2, 1, 5, 1, 0 );


  // Eleements (2,2) - (6,2) are the Rotation Conversions text box 

  QLabel* sidebarLabel = new QLabel( tr( "Rotation Abbreviations" ) );
  textbox.sidebar = new SC_TextEdit;
  textbox.sidebar -> setText( " Stuff Goes Here" );
  textbox.sidebar -> setDisabled( true );
  gridLayout -> addWidget( sidebarLabel,    1, 2, 0 );
  gridLayout -> addWidget( textbox.sidebar, 2, 2, 5, 1, 0 );
  
  // this adjusts the relative width of each column
  gridLayout -> setColumnStretch( 0, 1 );
  gridLayout -> setColumnStretch( 1, 2 );
  gridLayout -> setColumnStretch( 2, 1 );
  
  // set the tab's layout to mainLayout
  automationTabScrollArea -> setLayout( gridLayout );

  // Finally, add the tab
  addTab( automationTabScrollArea, tr( "Automation" ) );

  connect( choice.player_class, SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( setSpecDropDown( const int ) ) );
  connect( choice.player_class, SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( setSidebarClassText() ) );
  connect( choice.player_spec,  SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( setSidebarClassText() ) );
  connect( choice.comp_type,    SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( compTypeChanged( const int ) ) );

  // do some initialization
  compTypeChanged( choice.comp_type -> currentIndex() );

}
