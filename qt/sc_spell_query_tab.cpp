// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include "SC_SpellQueryTab.hpp"
#include "util/sc_mainwindowcommandline.hpp"

namespace { // unnamed namespace

struct FilterEntry
{
  const QString name;
  const bool spell_source;
  const bool talent_source;
  const QString operand_type;
};

const QString sources[] = 
{
  " ",
  "spell",
  "talent",
  "talent_spell",
  "class_spell",
  "race_spell",
  "mastery",
  "spec_spell",
  "glyph",
  "set_bonus",
  "effect",
  "perk_spell",
  NULL
};

const FilterEntry filters[] = 
{
  //                   source
  // filter name     spell talent argument type/description
  { " ",             true, true, "none" },
  { "name",          true, true, "string" },
  { "id",            true, true, "number" },
  { "flags",         true, true, "number (not used for anything currently) " },
  { "speed",         true, false, "number (projectile speed) " },
  { "school",        true, false, "string (spell school name) " },
  { "class",         true, true, "string (class name) " },
  { "pet_class",     false, true, "string (pet talent tree name) " },
  { "scaling",       true, false, "number (spell scaling type, -1 for \"generic scaling\", 0 for no scaling, otherwise class number) " },
  { "extra_coeff",   true, false, "number (spell-wide coefficient, usually used for spells scaling with both SP and AP) " },
  { "level",         true, false, "number (spell learned level) " },
  { "max_level",     true, false, "number (spell \"maximum\" level in a scaling sense) " },
  { "min_range",     true, false, "number (minimum range in yards) " },
  { "max_range",     true, false, "number (maximum range in yards) " },
  { "cooldown",      true, false, "number (spell cooldown, in milliseconds) " },
  { "gcd",           true, false, "number (spell gcd duration, in milliseconds) " },
  { "category",      true, false, "number (spell cooldown category) " },
  { "duration",      true, false, "number (spell duration in milliseconds) " },
  { "rune",          true, false, "string (b = blood, f = frost, u = unholy, will match minimum rune requirement)" },
  { "power_gain",    true, false, "number (amount of runic power gained)" },
  { "max_stack",     true, false, "number (maximum stack of spell)" },
  { "proc_chance",   true, false, "number (spell proc chance in percent (0..100))" },
  { "icd",           true, false, "number (internal cooldown of a spell in milliseconds)" },
  { "initial_stack", true, false, "number (initial amount of stacks)" },
  { "cast_min",      true, false, "number (minimum cast time in milliseconds)" },
  { "cast_max",      true, false, "number (maximum cast time in milliseconds)" },
  { "cast_div",      true, false, "number (scaling divisor for cast time, always 20)" },
  { "m_scaling",     true, false, "number (unknown scaling multiplier)" },
  { "scaling_level", true, false, "number (level threshold for m_scaling)" },
  { "desc",          true, false, "string (spell description)" },
  { "tooltip",       true, false, "string (spell tooltip)" },
  { "tab",           false, true, "number (talent tab number, 0..2)" },
  { "dependence",    false, true, "number (talent id this talent depends on)" },
  { "depend_rank",   false, true, "number (talent rank of talent id this talent depends on)" },
  { "col",           false, true, "number (talent column 0..3)" },
  { "row",           false, true, "number (talent \"tier\" 0..6) " },
  { NULL,            NULL,  NULL, NULL }
};

const QString numericOperators[] = 
{ "==", "!=", ">", "<", ">=", "<=", NULL };

const QString stringOperators[] = 
{ "==", "!=", "~", "!~", NULL };

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

QComboBox* createChoiceFromList( const QString list[] )
{
  QComboBox* choice = new QComboBox();
  for ( int i = 0; list[ i ].length() > 0; i++ )
  {
    choice -> addItem( list[ i ] );
  }
  return choice;
}

} // end unnamed namespace

void SC_SpellQueryTab::sourceTypeChanged( const int source_index )
{
  choice.filter -> clear();
  for ( int i = 0; filters[ i ].name.length() > 0; i++ )
    if ( ( choice.source -> itemText( source_index) != "talent" && filters[ i ].spell_source ) || 
         ( choice.source -> itemText( source_index) == "talent" && filters[ i ].talent_source ) )
      choice.filter -> addItem( filters[ i ].name );

  filterTypeChanged( choice.filter -> currentIndex() );
}

void SC_SpellQueryTab::filterTypeChanged( const int filter_index )
{
  choice.operatorString -> clear();
  QString filter_text = choice.filter -> itemText( filter_index );

  for ( int i = 0; filters[ i ].name.length() > 0; i++ )
    if ( filter_text == filters[ i ].name )
    {
      if ( filters[ i ].operand_type.startsWith( "number" ) )
        for ( int j = 0; numericOperators[ j ].length() > 0; j++ )
          choice.operatorString -> addItem( numericOperators[ j ] );
      else if ( filters[ i ].operand_type.startsWith( "string" ) )
        for ( int j = 0; stringOperators[ j ].length() > 0; j++ )
          choice.operatorString -> addItem( stringOperators[ j ] );

      return;
    }
}

void SC_SpellQueryTab::browseForFile()
{
  QString directory = QFileDialog::getSaveFileName( this, tr( "Choose File" ), QDir::currentPath() + "/query.txt", tr( "Text files (*.txt *.simc)" ) );

  if ( ! directory.isEmpty() )
  {
    if ( choice.directory -> findText( directory ) == -1 )
      choice.directory -> addItem( directory );
    choice.directory -> setCurrentIndex( choice.directory -> findText( directory ) );
  }
}

SC_SpellQueryTab::SC_SpellQueryTab( SC_MainWindow* parent ) :
  QWidget( parent ), mainWindow( parent )
{
  // define a grid layout for placement of objects
  QGridLayout* gridLayout = new QGridLayout;

  // Add a label
  label.input = new QLabel( tr( "Input Options" ) );
  gridLayout -> addWidget( label.input, 0, 0, 0 );

  // Element (1,0) is a GroupBox containing the inputs
  QGroupBox* inputGroupBox = new QGroupBox();
  gridLayout -> addWidget( inputGroupBox, 1, 0, 0 );

  // Layout of the groupbox
  QGridLayout* inputGroupBoxLayout = new QGridLayout();
  //inputGroupBoxLayout -> setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );


  // Add a combo box and label
  label.source = new QLabel( tr( "data source" ) );
  choice.source = createChoiceFromList( sources );
  inputGroupBoxLayout -> addWidget( label.source,  0, 0 );
  inputGroupBoxLayout -> addWidget( choice.source, 1, 0 );

  // add another combo box
  label.filter = new QLabel( tr( "filter" ) );
  choice.filter = createChoice( 2, "1", "2" );
  inputGroupBoxLayout -> addWidget( label.filter,  0, 1 );
  inputGroupBoxLayout -> addWidget( choice.filter, 1, 1 );

  // add a combo box for operators
  label.operatorString = new QLabel( tr( "operator" ) );
  choice.operatorString = createChoice( 2, "1", "2" );
  inputGroupBoxLayout -> addWidget( label.operatorString,  0, 2 );
  inputGroupBoxLayout -> addWidget( choice.operatorString, 1, 2 );

  // initialize the filter and operator combo boxes
  sourceTypeChanged( choice.source -> currentIndex() );
  filterTypeChanged( choice.filter -> currentIndex() );

  // add a line edit for text input
  label.arg = new QLabel( tr( "argument" ) );
  textbox.arg = new QLineEdit;
  inputGroupBoxLayout -> addWidget( label.arg,   0, 3 );
  inputGroupBoxLayout -> addWidget( textbox.arg, 1, 3 );

  // random label to use as a spacer
  QLabel* dummy = new QLabel( tr( "   " ) );
  inputGroupBoxLayout -> addWidget( dummy, 2, 0 );

  // Checkbox for save to file goes here
  choice.saveToFile = new QCheckBox( "Save to file?" );
  inputGroupBoxLayout -> addWidget( choice.saveToFile, 3, 1 );

  // push button for browsing to find a file
  button.save = new QPushButton( "Browse" );
  connect( button.save, SIGNAL( clicked() ), SLOT( browseForFile() ) );
  inputGroupBoxLayout -> addWidget( button.save, 3, 2 );

  // Editable Combo Box for filename
  choice.directory = new QComboBox;
  choice.directory -> setEditable( true );
  inputGroupBoxLayout -> addWidget( choice.directory, 3, 3 );

  // this adjusts the relative width of each column
  //inputGroupBoxLayout -> setColumnStretch( 0, 2 );
  //inputGroupBoxLayout -> setColumnStretch( 1, 2 );
  //inputGroupBoxLayout -> setColumnStretch( 2, 2 );
  //inputGroupBoxLayout -> setColumnStretch( 3, 5 );

  // apply the layout to the group box
  inputGroupBox -> setLayout( inputGroupBoxLayout );

  // text output box
  label.output = new QLabel( tr( "Output" ) );
  textbox.result = new SC_TextEdit;
  textbox.result -> setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  textbox.result -> setLineWrapMode( SC_TextEdit::WidgetWidth );
  gridLayout -> addWidget( label.output, 2, 0, 0 );
  gridLayout -> addWidget( textbox.result, 3, 0, 0 );
  
  // this adjusts the relative width of each column
  //gridLayout -> setColumnStretch( 0, 1 );
  //gridLayout -> setColumnStretch( 1, 3 );

  setLayout( gridLayout );

  // connect source drop-down to method that swaps filter options
  connect( choice.source, SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( sourceTypeChanged( const int ) ) );
  connect( choice.filter, SIGNAL( currentIndexChanged( const int& ) ), this, SLOT( filterTypeChanged( const int ) ) );

  // create tooltips
  createToolTips();
}

void SC_SpellQueryTab::run_spell_query()
{
  // local copy of the argument
  std::string arg = textbox.arg -> text().toStdString();

  // construct the query string
  QString command = "spell_query=";

  // if we're using the simple style, we have a data source spec
  if ( choice.source -> currentText() != sources[ 0 ] )
  {
    // add source, filter (if present), and operator string
    command += choice.source -> currentText();

    if ( choice.filter -> currentText() != filters[ 0 ].name )
      command += "." + choice.filter -> currentText();

    command += choice.operatorString -> currentText();

    // tokenize the argument
    util::tokenize( arg );
  }

  // Add the argument (to support advanced mode, this isn't tokenized unless we use a source)
  command += QString::fromStdString( arg );

  // set the command line (mostly so we can see the query)
  mainWindow -> cmdLine -> setCommandLineText( command );
  
  // call the sim - results will be stuffed back into textbox in SC_MainWindow::deleteSim()
  mainWindow -> simulationQueue.enqueue( "Spell Query", "", command );
}

void SC_SpellQueryTab::checkForSave()
{
  // save to file
  if ( choice.saveToFile -> isChecked() )
  {
    QFile file( choice.directory -> currentText() );
    // if the file can't be opened, skip
    if ( ! file.open( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) )
      return;
    QTextStream ts( &file );

    ts << textbox.result -> document() -> toPlainText();
    file.close();
  }
}


void SC_SpellQueryTab::createToolTips()
{
  choice.source -> setToolTip( tr( "Data Source, determines which list of identifiers to search within" ) );
  choice.filter -> setToolTip( tr( "Filter, for filtering the data in the data source." ) );
  choice.operatorString -> setToolTip( tr( "Operand. Different for string and numeric arguments.\n\n Strings:\n\n == : case-insensitive equality\n != : case-insensitive inequality\n ~  : case-insensitive substring in field\n !~ : case-insensitive substring not in field\n" ) );
  textbox.arg -> setToolTip( tr( "Argument, what you're searching for within the filtered data.\n\n For more complicated queries, set the Data Source and Filter to blank and write your full query string in here." ) );
  
  choice.saveToFile -> setToolTip( tr( "Enabling this will save the results to a file." ) );
  button.save -> setToolTip( tr( "Browse for the file name of the results file." ) );
  choice.directory -> setToolTip( tr( "Filename of the results file." ) );

  textbox.result -> setToolTip( tr( "Result of the query." ) );

  
}


void SC_SpellQueryTab::load_setting( QSettings& s, const QString& name, QComboBox* choice, const QString& default_value = QString() )
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

void SC_SpellQueryTab::load_setting( QSettings& s, const QString& name, QLineEdit* textbox, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  if ( !v.isEmpty() )
    textbox -> setText( v );
  else if ( !default_value.isEmpty() )
    textbox -> setText( default_value );
}

void SC_SpellQueryTab::load_setting( QSettings& s, const QString& name, SC_TextEdit* textbox, const QString& default_value = QString() )
{
  const QString& v = s.value( name ).toString();

  if ( !v.isEmpty() )
    textbox -> setText( v );
  else if ( !default_value.isEmpty() )
    textbox -> setText( default_value );
}

// Encode all options/setting into a string ( to be able to save it to the history )
// Decode / Encode order needs to be equal!

void SC_SpellQueryTab::encodeSettings()
{
  QSettings settings;
  settings.beginGroup( "spell_query" );
  settings.setValue( "source", choice.source -> currentText() );
  settings.setValue( "filter", choice.filter -> currentText() );
  settings.setValue( "operatorString", choice.operatorString -> currentIndex() );
  settings.setValue( "arg", textbox.arg -> text() );

  QStringList directories;
  for ( int i = 0; i < choice.directory -> count(); i++ )
    directories.append( choice.directory -> itemText( i ) );
  settings.setValue( "directory", directories );
  
  settings.setValue( "saveCheckBox", choice.saveToFile -> isChecked() );

  QString encoded;

  settings.endGroup(); // end group "options"
}

/* Decode all options/setting from a string ( loaded from the history ).
 * Decode / Encode order needs to be equal!
 *
 * If no default_value is specified, index 0 is used as default.
 */


void SC_SpellQueryTab::decodeSettings()
{
  QSettings settings;
  settings.beginGroup( "spell_query" );
  load_setting( settings, "source", choice.source );
  load_setting( settings, "filter", choice.filter );
  load_setting( settings, "operatorString", choice.operatorString );
  load_setting( settings, "arg", textbox.arg );

  QStringList directories = settings.value( "directory" ).toStringList();
  choice.directory -> addItems( directories );

  choice.saveToFile -> setChecked( settings.value( "saveCheckBox" ).toBool() );

  settings.endGroup();
}
