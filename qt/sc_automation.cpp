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


// Local method to perform tokenize on QStrings
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
  base_profile_info += "specialization=" + tokenize( player_spec ) + "\n";
  base_profile_info += "race=" + tokenize( player_race ) + "\n";
  base_profile_info += "level=" + player_level + "\n";

  // simulation type check
  switch ( sim_type )
  {
    case 1: // talent simulation
      return auto_talent_sim( player_class, base_profile_info, advanced_text, player_glyphs, player_gear, player_rotation );
    case 2: // glyph simulation
      return auto_glyph_sim( player_class, base_profile_info, player_talents, advanced_text, player_gear, player_rotation );
    case 3: // gear simulation
      return auto_gear_sim( player_class, base_profile_info, player_talents, player_glyphs, advanced_text, player_rotation );
    case 4: // rotation simulation
      return auto_rotation_sim( player_class, base_profile_info, player_talents, player_glyphs, player_rotation, advanced_text );
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
QString automation::auto_talent_sim( QString player_class,
                                     QString base_profile_info, 
                                     QString advanced_text,
                                     QString player_glyphs,
                                     QString player_gear,
                                     QString player_rotation
                                   )
{
  QStringList talentList = advanced_text.split( "\n", QString::SkipEmptyParts );

  QString profile;


  // make profile
  for ( int i = 0; i < talentList.size(); i++ )
  {
    profile += tokenize( player_class ) + "=T_" + talentList[ i ] + "\n";
    profile += base_profile_info;
    profile += "talents=" + talentList[ i ] + "\n";
    profile += "glyphs=" + player_glyphs + "\n";
    profile += player_gear + "\n";
    profile += player_rotation + "\n";
    profile += "\n";
  }

  return profile;
}

  // Method for profile creation for the specific GLYPHS simulation
QString automation::auto_glyph_sim( QString player_class,
                                    QString base_profile_info,
                                    QString player_talents,
                                    QString advanced_text,
                                    QString player_gear,
                                    QString player_rotation
                                  )
{
  QStringList glyphList = advanced_text.split("\n", QString::SkipEmptyParts);

  QString profile;

   // make profile
   for ( int i = 0; i < glyphList.size(); i++ )
  {
    profile += tokenize( player_class ) + "=G_" + QString::number( i ) + "\n";
    profile += base_profile_info;
    profile += "talents=" + player_talents + "\n";
    profile += "glyphs=" + glyphList[ i ] + "\n";
    profile += player_gear + "\n";
    profile += player_rotation + "\n";
    profile += "\n";
  }

  return profile;
}

  // Method for profile creation for the specific GEAR simulation
QString automation::auto_gear_sim( QString player_class,
                                   QString base_profile_info,
                                   QString player_talents,
                                   QString player_glyphs,
                                   QString advanced_text,
                                   QString player_rotation
                                 )
{
  // split into gearSets
  QStringList gearList = advanced_text.split( "\n\n", QString::SkipEmptyParts );

  QString profile;

  // make profile
  for ( int i = 0; i < gearList.size(); i++ )
  {
    profile += tokenize( player_class ) + "=G_" + QString::number( i ) + "\n";
    profile += base_profile_info;
    profile += "talents=" + player_talents + "\n";
    profile += "glyphs=" + player_glyphs + "\n";
    // split gearSets into single item lines
    QStringList gearSet = gearList[ i ].split( "\n", QString::SkipEmptyParts );
    for ( int j = 0; j < gearSet.size(); j++ )
    {
      profile += gearSet[ j ] + "\n";
    }
    profile += player_rotation + "\n";
    profile += "\n";
  }
  return profile;
}

// Method for profile creation for the specific ROTATION simulation
QString automation::auto_rotation_sim( QString player_class,
                                       QString base_profile_info,
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

// constant for sidebar text (this will eventually get really long)
QString sidebarText[ 11 ][ 4 ] = {
  { "Blood shorthand goes here.", "Frost shorthand goes here", "Unholy shorthand goes here", "N/A" },
  { "Balance shorthand goes here.", "Feral shorthand goes here.", "Guardian shorthand goes here.", "Restoration shorthand goes here." },
  { "Beast Master shorthand goes here.", "Marksmanship shorthand goes here.", "Survival shorthand goes here." },
  { "Arcane shorthand goes here.", "Fire shorthand goes here.", "Frost shorthand goes here.", "N/A" },
  { "Brewmaster shorthand goes here.", "Mistweaver shorthand goes here.", "Windwalker shorthand goes here.", "N/A" },
  { "Holy shorthand goes here.", "Protection shorthand goes here.", "Retribution shorthand goes here.", "N/A" },
  { "Discipline shorthand goes here.", "Holy shorthand goes here.", "Shadow shorthand goes here.", "N/A" },
  { "Assasination shorthand goes here.", "Combat shorthand goes here.", "Subtlety shorthand goes here.", "N/A" },
  { "Elemental shorthand goes here.", "Enhancement shorthand goes here.", "Restoration shorthand goes here.", "N/A" },
  { "Affliction shorthand goes here.", "Demonology shorthand goes here.", "Destruction shorthand goes here.", "N/A" },
  { "Arms shorthand goes here.", "Fury shorthand goes here.", "Protection shorthand goes here.", "N/A" },
};

// constant for the varying labels of the advanced text box
QString advancedText[ 5 ] = {
  "Unused", "Talent Configurations", "Glyph Configurations", "Gear Configurations", "Rotation Shorthands"
};
// constant for the varying labels of the helpbar text box
QString helpbarText[ 5 ] = {
  "To automate generation of a comparison, choose a comparison type from the drop-down box to the left.",
  "To specify different talent configurations, list the talent configurations you want to test (as seven-digit integer strings, i.e. 1231231) in the box below. Each configuration should be its own new line.",
  "To specify different glyph configurations, list the glyph configurations you want to test (i.e. alabaster_shield/focused_shield/word_of_glory) in the box below. Each configuration should be its own new line.",
  "To specify different gear configurations, we need to implement some things still.",
  "To specify different rotations, list the rotation shorthands you want to test in the box below. The sidebar lists the shorthand conventions for your class and spec. If a shorthand you want isn't documented, please contact the simulationcraft team to have it added."
};


// method to set the sidebar text based on class slection
void SC_ImportTab::setSidebarClassText()
{
  textbox.sidebar -> setText( sidebarText[ choice.player_class -> currentIndex() ][ choice.player_spec -> currentIndex() ] );
}

void SC_ImportTab::compTypeChanged( const int comp )
{
  // store whatever text is in the advanced window in the appropriate QString 
  if ( ! textbox.talents -> isEnabled() )
    advTalent = textbox.advanced -> document() -> toPlainText();
  else if ( ! textbox.glyphs -> isEnabled() )
    advGlyph = textbox.advanced -> document() -> toPlainText();
  else if ( ! textbox.gear -> isEnabled() )
    advGear = textbox.advanced -> document() -> toPlainText();
  else if ( ! textbox.rotation -> isEnabled() )
    advRotation = textbox.advanced -> document() -> toPlainText();

  // set the label of the Advanced tab appropriately
  label.advanced -> setText( advancedText[ comp ] );

  // set the text of the help bar appropriately
  textbox.helpbar -> setText( helpbarText[ comp ] );
  
  // TODO: set the text of the box with some sort of default for each case

  textbox.advanced -> setDisabled( false );
  textbox.gear     -> setDisabled( false );
  textbox.glyphs   -> setDisabled( false );
  textbox.rotation -> setDisabled( false );
  textbox.talents  -> setDisabled( false );

  switch ( comp )
  {
    case 0: 
      textbox.advanced -> setDisabled( true );
      break;
    case 1:
      textbox.advanced -> setText( advTalent );
      textbox.talents -> setDisabled( true );
      break;
    case 2:
      textbox.glyphs -> setDisabled( true );
      textbox.advanced -> setText( advGlyph );
      break;
    case 3:
      textbox.gear -> setDisabled( true );
      textbox.advanced -> setText( advGear );
      break;
    case 4:
      textbox.rotation -> setDisabled( true );
      textbox.advanced -> setText( advRotation );
      break;
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
                                      importTab -> textbox.talents -> text(),
                                      importTab -> textbox.glyphs -> text(),
                                      importTab -> textbox.gear -> document() -> toPlainText(),
                                      importTab -> textbox.rotation -> document() -> toPlainText(),
                                      importTab -> textbox.advanced -> document() -> toPlainText()
                                    );

  simulateTab -> add_Text( profile,  tr( "Testing" ) );
  
  mainTab -> setCurrentTab( TAB_SIMULATE );
}

void SC_ImportTab::createTooltips()
{
  choice.comp_type -> setToolTip( "Choose the comparison type." );
  

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
  QGroupBox* compGroupBox = new QGroupBox();
  gridLayout -> addWidget( compGroupBox, 0, 0, 0 );

  // Define a layout for the box
  QFormLayout* compLayout = new QFormLayout();
  compLayout -> setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  
  // Create Combo Box and add it to the layout
  choice.comp_type = createChoice( 5 , "None", "Talents", "Glyphs", "Gear", "Rotation" );
  compLayout -> addRow( tr( "Comparison Type" ), choice.comp_type );

  // Apply the layout to the compGroupBox
  compGroupBox -> setLayout( compLayout );
  

  // Elements (0,1) - (0,2) are the helpbar
  textbox.helpbar = new SC_TextEdit;
  textbox.helpbar -> setDisabled( true );
  textbox.helpbar -> setMaximumHeight( 50 );
  textbox.helpbar -> setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  gridLayout -> addWidget( textbox.helpbar, 0, 1, 1, 2, 0 );


  // Elements (1,0) & (2,0) are a QLabel and QGroupBox for some of the defaults  
  QLabel* defaultsLabel = new QLabel( tr( "Defaults" ) );
  gridLayout -> addWidget( defaultsLabel, 1, 0, 0 );

  // create a box for the defaults section
  QGroupBox* defaultsGroupBox = new QGroupBox();
  gridLayout -> addWidget( defaultsGroupBox, 2, 0, 0 );

  // define a FormLayout for the GroupBox
  QFormLayout* defaultsFormLayout = new QFormLayout();
  defaultsFormLayout -> setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );

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
  label.talents = new QLabel( tr("Default Talents" ) );
  textbox.talents = new QLineEdit;
  defaultsFormLayout -> addRow( label.talents, textbox.talents );
  
  label.glyphs = new QLabel( tr("Default Glypyhs" ) );
  textbox.glyphs = new QLineEdit;
  defaultsFormLayout -> addRow( label.glyphs, textbox.glyphs );

  // set the GroupBox's layout now that we've defined it
  defaultsGroupBox -> setLayout( defaultsFormLayout );

  
  // Elements (3,0) - (6,0) are the default gear and rotation labels and text boxes
  
  // Create a label and an edit box for gear
  label.gear = new QLabel( tr( "Default Gear" ) );
  textbox.gear = new SC_TextEdit;
  // assign the label and edit box to cells
  gridLayout -> addWidget( label.gear,   3, 0, 0 );
  gridLayout -> addWidget( textbox.gear, 4, 0, 0 );
  
  // and again for rotation
  label.rotation = new QLabel( tr( "Default Rotation" ) );
  textbox.rotation = new SC_TextEdit;
  gridLayout -> addWidget( label.rotation,   5, 0, 0 );
  gridLayout -> addWidget( textbox.rotation, 6, 0, 0 );


  // Elements (2,1) - (6,1) are the Advanced text box, which is actually
  // N different text boxes that get cycled depending on sim type choice

  label.advanced = new QLabel( tr( "Advanced Text Box" ) );
  textbox.advanced = new SC_TextEdit;
  gridLayout -> addWidget( label.advanced,   1, 1, 0 );
  gridLayout -> addWidget( textbox.advanced, 2, 1, 5, 1, 0 );


  // Eleements (2,2) - (6,2) are the Rotation Conversions text box 

  label.sidebar = new QLabel( tr( "Rotation Abbreviations" ) );
  textbox.sidebar = new SC_TextEdit;
  textbox.sidebar -> setText( " Stuff Goes Here" );
  textbox.sidebar -> setDisabled( true );
  gridLayout -> addWidget( label.sidebar,   1, 2, 0 );
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

  // set up all the tooltips
  createTooltips();
}


// Encode all options and text fields into a string ( to be able to save it to the history )
// Decode / Encode order needs to be equal!

void SC_ImportTab::encodeSettings()
{
  QSettings settings;
  settings.beginGroup( "automation" );
  settings.setValue( "comp_type", choice.comp_type -> currentText() );
  settings.setValue( "class", choice.player_class -> currentText() );
  settings.setValue( "spec", choice.player_spec -> currentText() );
  settings.setValue( "race", choice.player_race -> currentText() );
  settings.setValue( "level", choice.player_level -> currentText() );
  settings.setValue( "talentsbox", textbox.talents -> text() );
  settings.setValue( "glyphsbox", textbox.glyphs -> text() );
  settings.setValue( "gearbox", textbox.gear -> document() -> toPlainText() );
  settings.setValue( "rotationbox", textbox.rotation -> document() -> toPlainText() );
  settings.setValue( "advancedbox", textbox.advanced -> document() -> toPlainText() );
  settings.setValue( "advTalent", advTalent );
  settings.setValue( "advGlyph", advGlyph );
  settings.setValue( "advGear", advGear );
  settings.setValue( "advRotation", advRotation );
  
  QString encoded;

  settings.endGroup(); // end group "automation"
}

/* Decode all options/setting from a string ( loaded from the history ).
 * Decode / Encode order needs to be equal!
 *
 * If no default_value is specified, index 0 is used as default.
 */

void SC_ImportTab::load_setting( QSettings& s, const QString& name, QComboBox* choice, const QString& default_value = QString() )
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

void SC_ImportTab::load_setting( QSettings& s, const QString& name, QString text, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  text = v;
}

void SC_ImportTab::load_setting( QSettings& s, const QString& name, QLineEdit* textbox, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  textbox -> setText( v );
}

void SC_ImportTab::load_setting( QSettings& s, const QString& name, SC_TextEdit* textbox, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  textbox -> setText( v );
}

void SC_ImportTab::decodeSettings()
{
  QSettings settings;
  settings.beginGroup( "automation" );
  load_setting( settings, "comp_type", choice.comp_type );
  load_setting( settings, "class", choice.player_class );
  load_setting( settings, "spec", choice.player_spec );
  load_setting( settings, "race", choice.player_race );
  load_setting( settings, "level", choice.player_level );
  load_setting( settings, "talentbox", textbox.talents, "0000000" );
  load_setting( settings, "glyphbox", textbox.glyphs );
  load_setting( settings, "gearbox", textbox.gear, "head=\nneck=\n" );
  load_setting( settings, "rotationbox", textbox.rotation );
  load_setting( settings, "advancedbox", textbox.advanced, "default" );
  load_setting( settings, "advTalent", advTalent, "0000000\n1111111\n2222222" );
  load_setting( settings, "advGlyph", advGlyph, "alabaster_shield/focused_shield" );
  load_setting( settings, "advGear", advGear );
  load_setting( settings, "advRotation", advRotation, "CS>J>AS>Cons" );
  
  settings.endGroup();
}