// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_mainwindowcommandline.hpp"

SC_MainWindowCommandLine::SC_MainWindowCommandLine( QWidget* parent ) :
    QWidget( parent ), statesStackedLayout( nullptr ),
      current_tab( CMDLINE_TAB_WELCOME ), current_state( IDLE )
{
  init();
}

SC_MainWindowCommandLine::state_e SC_MainWindowCommandLine::currentState() const
{
  return current_state;
}

void SC_MainWindowCommandLine::setSimulatingProgress( int value, QString format,
                                                      QString toolTip )
{
  updateProgress( PROGRESSBAR_SIMULATING, value, format, toolTip );
}

int SC_MainWindowCommandLine::getSimulatingProgress()
{
  return getProgressBarProgressForState( PROGRESSBAR_SIMULATING );
}

void SC_MainWindowCommandLine::setImportingProgress( int value, QString format,
                                                     QString toolTip )
{
  updateProgress( PROGRESSBAR_IMPORTING, value, format, toolTip );
}

int SC_MainWindowCommandLine::getImportingProgress()
{
  return getProgressBarProgressForState( PROGRESSBAR_IMPORTING );
}

void SC_MainWindowCommandLine::setBattleNetLoadProgress( int value,
                                                         QString format,
                                                         QString toolTip )
{
  updateProgress( PROGRESSBAR_BATTLE_NET, value, format, toolTip );
}

int SC_MainWindowCommandLine::getBattleNetProgress()
{
  return getProgressBarProgressForState( PROGRESSBAR_BATTLE_NET );
}

void SC_MainWindowCommandLine::setHelpViewProgress( int value, QString format,
                                                    QString toolTip )
{
  updateProgress( PROGRESSBAR_HELP, value, format, toolTip );
}

int SC_MainWindowCommandLine::getHelpViewProgress()
{
  return getProgressBarProgressForState( PROGRESSBAR_HELP );
}

QString SC_MainWindowCommandLine::commandLineText()
{
  // gets commandline text for the current tab
  return commandLineText( current_tab );
}

QString SC_MainWindowCommandLine::commandLineText( main_tabs_e tab )
{
  return commandLineText( convertTabsEnum( tab ) );
}

QString SC_MainWindowCommandLine::commandLineText( import_tabs_e tab )
{
  return commandLineText( convertTabsEnum( tab ) );
}

QString SC_MainWindowCommandLine::commandLineText( tabs_e tab )
{
  // returns current commandline text for the specified tab in the current state
  QString* text = getText( current_state, tab, TEXTEDIT_CMDLINE );
  QString retval;

  if ( text != nullptr )
  {
    retval = *text;
  }

  return retval;
}

void SC_MainWindowCommandLine::setCommandLineText( QString text )
{
  // sets commandline text for the current tab
  setCommandLineText( current_tab, text );
}

void SC_MainWindowCommandLine::setCommandLineText( main_tabs_e tab,
                                                   QString text )
{
  setCommandLineText( convertTabsEnum( tab ), text );
}

void SC_MainWindowCommandLine::setCommandLineText( import_tabs_e tab,
                                                   QString text )
{
  setCommandLineText( convertTabsEnum( tab ), text );
}

void SC_MainWindowCommandLine::setCommandLineText( tabs_e tab, QString text )
{
  adjustText( current_state, tab, TEXTEDIT_CMDLINE, text );
  updateWidget( current_state, tab, TEXTEDIT_CMDLINE );
}

void SC_MainWindowCommandLine::togglePaused()
{
  setPaused( !isPaused() );
}

void SC_MainWindowCommandLine::setPaused( bool pause )
{
  switch ( current_state )
  {
  case SIMULATING:
    if ( pause )
    {
      setState (SIMULATING_PAUSED);
    }
    break;
  case SIMULATING_MULTIPLE:
    if ( pause )
    {
      setState (SIMULATING_MULTIPLE_PAUSED);
    }
    break;
  case SIMULATING_PAUSED:
    if ( !pause )
    {
      setState (SIMULATING);
    }
    break;
  case SIMULATING_MULTIPLE_PAUSED:
    if ( !pause )
    {
      setState (SIMULATING_MULTIPLE);
    }
    break;
  default:
    break;
  }
}

bool SC_MainWindowCommandLine::isPaused()
{
  bool retval = false;

  switch ( current_state )
  {
  case SIMULATING_PAUSED:
    retval = true;
    break;
  case SIMULATING_MULTIPLE_PAUSED:
    retval = true;
    break;
  default:
    break;
  }

  return retval;
}

void SC_MainWindowCommandLine::init()
{
  initStateInfo();
  initStackedLayout();

  setState( current_state );
}

void SC_MainWindowCommandLine::initStateInfo()
{
  // Initializes all state info
  // use setText() to set text for widgets in states/tabs
  // use setProgressBarState() to set progressbar's state in states/tabs
  // using text_hide_widget as the text, will hide that widget ONLY in that state/tab, and show it in others

  initWidgetsToNull();
  initTextStrings();
  initStatesStructToNull();
  initDefaultStates();

  // states are defaulted: special cases

  initImportStates();
  initLogResultsStates();
  initProgressBarStates();
  initCommandLineBuffers();
  initPauseStates();
}

void SC_MainWindowCommandLine::initWidgetsToNull()
{
  for ( state_e state = IDLE; state < STATE_COUNT; state++ )
  {
    for ( widgets_e widget = BUTTON_MAIN; widget < WIDGET_COUNT; widget++ )
    {
      widgets[state][widget] = nullptr;
    }
  }
}

void SC_MainWindowCommandLine::initTextStrings()
{
  // strings shared by widgets
  text_simulate = tr( "Simulate!" );
  text_pause = tr( "Pause!" );
  text_resume = tr( "Resume!" );
  text_queue = tr( "Queue!" );
  text_queue_tooltip = tr(
      "Click to queue a simulation to run after the current one" );
  text_cancel = tr( "Cancel! " );
  text_cancel_all = tr( "Cancel All!" );
  text_cancel_all_tooltip = tr(
      "Cancel ALL simulations, including what is queued" );
  text_save = tr( "Save!" );
  text_import = tr( "Import!" );
  text_spellquery = tr( "Query!" );
  text_prev = tr( "<" );
  text_next = tr( ">" );
  text_prev_tooltip = tr( "Backwards" );
  text_next_tooltip = tr( "Forwards" );
  text_hide_widget = "hide_widget";
  // actually manually typing in hide_widget into the command line will actually HIDE IT
  // so append some garbage to it
  text_hide_widget.append( QString::number( rand() ) );
}

void SC_MainWindowCommandLine::initStatesStructToNull()
{
  for ( state_e state = IDLE; state < STATE_COUNT; state++ )
  {
    for ( tabs_e tab = CMDLINE_TAB_WELCOME; tab < CMDLINE_TAB_COUNT; tab++ )
    {
      for ( widgets_e widget = BUTTON_MAIN; widget < WIDGET_COUNT; widget++ )
      {
        states[state][tab][widget].text = nullptr;
        states[state][tab][widget].tool_tip = nullptr;
        states[state][tab][widget].progressbar_state = PROGRESSBAR_IGNORE;
      }
    }
  }
}

void SC_MainWindowCommandLine::initDefaultStates()
{
  // default states for widgets
  for ( tabs_e tab = CMDLINE_TAB_WELCOME; tab < CMDLINE_TAB_COUNT; tab++ )
  {
    // set the same defaults for every state
    for ( state_e state = IDLE; state < STATE_COUNT; state++ )
    {
      // buttons
      setText( state, tab, BUTTON_MAIN, &text_simulate );
      setText( state, tab, BUTTON_QUEUE, &text_queue, &text_queue_tooltip );
      setText( state, tab, BUTTON_PAUSE, &text_pause );
      setText( state, tab, BUTTON_PREV, &text_prev, &text_prev_tooltip );
      setText( state, tab, BUTTON_NEXT, &text_next, &text_next_tooltip );
      // progressbar
      setText( state, tab, PROGRESSBAR_WIDGET,
               &progressBarFormat[PROGRESSBAR_SIMULATING].text,
               &progressBarFormat[PROGRESSBAR_SIMULATING].tool_tip );
      setProgressBarState( state, tab, PROGRESSBAR_SIMULATING );
      // commandline buffer
      setText( state, tab, TEXTEDIT_CMDLINE, &commandLineBuffer_DEFAULT );
    }

    // simulating defaults:
    // mainbutton: simulate => cancel
    setText( SIMULATING, tab, BUTTON_MAIN, &text_cancel ); // instead of text_simulate
    setText( SIMULATING_PAUSED, tab, BUTTON_MAIN, &text_cancel );
    setText( SIMULATING_MULTIPLE, tab, BUTTON_MAIN, &text_cancel ); // instead of text_simulate
    setText( SIMULATING_MULTIPLE_PAUSED, tab, BUTTON_MAIN, &text_cancel ); // instead of text_simulate
    setText( SIMULATING, tab, BUTTON_PAUSE, &text_pause );

    // simulating_multiple defaults:
    // cancel => text_cancel_all
    for ( widgets_e widget = BUTTON_MAIN; widget < WIDGET_COUNT; widget++ )
    {
      if ( getText( SIMULATING_MULTIPLE, tab, widget ) == text_cancel )
      {
        setText( SIMULATING_MULTIPLE, tab, widget, &text_cancel_all,
                 &text_cancel_all_tooltip );
        setText( SIMULATING_MULTIPLE_PAUSED, tab, widget, &text_cancel_all,
                 &text_cancel_all_tooltip );
      }
    }
  }
  setText( IDLE, CMDLINE_TAB_SPELLQUERY, BUTTON_MAIN, &text_spellquery );
}

void SC_MainWindowCommandLine::initImportStates()
{
  // TAB_IMPORT + Subtabs
  setText( IDLE, CMDLINE_TAB_IMPORT, BUTTON_MAIN, &text_import );
  // special cases for all TAB_IMPORT subtabs
  // button_main: text_import
  // button_queue: text_import
  for ( import_tabs_e tab = TAB_BATTLE_NET; tab <= TAB_CUSTOM; tab++ )
  {
    tabs_e subtab = convertTabsEnum( tab );
    setText( IDLE, subtab, BUTTON_MAIN, &text_import );
    setText( SIMULATING, subtab, BUTTON_QUEUE, &text_import );
    setText( SIMULATING_MULTIPLE, subtab, BUTTON_QUEUE, &text_import );
  }
}

void SC_MainWindowCommandLine::initStackedLayout()
{
  // initializes the stackedlayout with all states
  statesStackedLayout = new QStackedLayout;

  for ( state_e state = IDLE; state < STATE_COUNT; state++ )
  {
    statesStackedLayout->addWidget( createState( state ) );
  }

  setLayout( statesStackedLayout );
}
void SC_MainWindowCommandLine::initLogResultsStates()
{
  // log/results: change button text so save is somewhere
  // IDLE: main button => save
  setText( IDLE, CMDLINE_TAB_LOG, BUTTON_MAIN, &text_save );
  setText( IDLE, CMDLINE_TAB_RESULTS, BUTTON_MAIN, &text_save );
  // SIMULATING + SIMULATING_MULTIPLE: queue button => save
  for ( state_e state = SIMULATING; state <= SIMULATING_MULTIPLE_PAUSED;
      state++ )
  {
    setText( state, CMDLINE_TAB_LOG, BUTTON_QUEUE, &text_save );
    setText( state, CMDLINE_TAB_RESULTS, BUTTON_QUEUE, &text_save );
  }
}

void SC_MainWindowCommandLine::initProgressBarStates()
{
  // progressbar: site/help
  // certain tabs have their own progress bar states
  for ( state_e state = IDLE; state < STATE_COUNT; state++ )
  {
    // import: TAB_IMPORT and all subtabs have the importing state
    setProgressBarState( state, CMDLINE_TAB_IMPORT, PROGRESSBAR_IMPORTING );
    for ( import_tabs_e tab = TAB_BATTLE_NET; tab <= TAB_CUSTOM; tab++ )
    {
      tabs_e importSubtabs = convertTabsEnum( tab );
      setProgressBarState( state, importSubtabs, PROGRESSBAR_IMPORTING );
    }
    // battlenet: battlenet has its own state
    setProgressBarState( state, CMDLINE_TAB_BATTLE_NET,
                         PROGRESSBAR_BATTLE_NET );
    // help: has its own state
    setProgressBarState( state, CMDLINE_TAB_HELP, PROGRESSBAR_HELP );
  }

  // set default format for every progressbar state to %p%
  QString defaultProgressBarFormat = "%p%";
  for ( progressbar_states_e state = PROGRESSBAR_IDLE;
      state < PROGRESSBAR_STATE_COUNT; state++ )
  {
    // QProgressBar -> setFormat()
    setProgressBarFormat( state, defaultProgressBarFormat, false );
    // set all progress to 100
    setProgressBarProgress( state, 100 );
  }
}

void SC_MainWindowCommandLine::initCommandLineBuffers()
{
  // commandline: different tabs have different buffers
  for ( state_e state = IDLE; state < STATE_COUNT; state++ )
  {
    // log has its own buffer (for save file name)
    setText( state, CMDLINE_TAB_LOG, TEXTEDIT_CMDLINE,
             &commandLineBuffer_TAB_LOG );
    // battlenet has its own buffer (for url)
    setText( state, CMDLINE_TAB_BATTLE_NET, TEXTEDIT_CMDLINE,
             &commandLineBuffer_TAB_BATTLE_NET );
    // help has its own buffer (for url)
    setText( state, CMDLINE_TAB_HELP, TEXTEDIT_CMDLINE,
             &commandLineBuffer_TAB_HELP );
    // results has its own buffer (for save file name)
    setText( state, CMDLINE_TAB_RESULTS, TEXTEDIT_CMDLINE,
             &commandLineBuffer_TAB_RESULTS );
    // everything else shares the default buffer
  }
}

void SC_MainWindowCommandLine::initPauseStates()
{
  for ( tabs_e tab = CMDLINE_TAB_WELCOME; tab < CMDLINE_TAB_COUNT; tab++ )
  {
    for ( state_e state = SIMULATING_PAUSED;
        state <= SIMULATING_MULTIPLE_PAUSED; state++ )
    {
      setText( state, tab, BUTTON_PAUSE, &text_resume );
    }
  }
}

QWidget* SC_MainWindowCommandLine::createState( state_e state )
{
  // Create the widget for the state
  QWidget* stateWidget = new QGroupBox;
  QLayout* stateWidgetLayout = new QHBoxLayout;

  stateWidget->setLayout( stateWidgetLayout );

  switch ( state )
  {
  case STATE_COUNT:
    break;
  default:
    createCommandLine( state, stateWidget );
    break;
  }

  switch ( state )
  {
  case IDLE:
    createState_IDLE( stateWidget );
    break;
  case SIMULATING:
    createState_SIMULATING( stateWidget );
    break;
  case SIMULATING_MULTIPLE:
    createState_SIMULATING_MULTIPLE( stateWidget );
    break;
  case SIMULATING_PAUSED:
    createState_SIMULATING_PAUSED( stateWidget );
    break;
  case SIMULATING_MULTIPLE_PAUSED:
    createState_SIMULATING_MULTIPLE_PAUSED( stateWidget );
    break;
  default:
    break;
  }

  stateWidget->setLayout( stateWidgetLayout );

  return stateWidget;
}

void SC_MainWindowCommandLine::createState_IDLE( QWidget* parent )
{
  // creates the IDLE state
  QLayout* parentLayout = parent->layout();

  QPushButton* buttonMain = new QPushButton(
      *getText( IDLE, CMDLINE_TAB_WELCOME, BUTTON_MAIN ), parent );
  setWidget( IDLE, BUTTON_MAIN, buttonMain );
  parentLayout->addWidget( buttonMain );

  connect( buttonMain, SIGNAL( clicked() ), this, SLOT( mainButtonClicked() ) );
}

void SC_MainWindowCommandLine::createState_SIMULATING( QWidget* parent )
{
  // creates the SIMULATING state
  _createState_SIMULATING( SIMULATING, parent );
}

void SC_MainWindowCommandLine::createState_SIMULATING_PAUSED(
    QWidget* parent )
{
  _createState_SIMULATING( SIMULATING_PAUSED, parent );
}

void SC_MainWindowCommandLine::createState_SIMULATING_MULTIPLE(
    QWidget* parent )
{
  // creates the SIMULATING_MULTIPLE state
  _createState_SIMULATING( SIMULATING_MULTIPLE, parent );
}

void SC_MainWindowCommandLine::createState_SIMULATING_MULTIPLE_PAUSED(
    QWidget* parent )
{
  _createState_SIMULATING( SIMULATING_MULTIPLE_PAUSED, parent );
}

void SC_MainWindowCommandLine::_createState_SIMULATING(
    state_e state, QWidget* parent )
{
  // creates either the SIMULATING or SIMULATING_MULTIPLE states
  QLayout* parentLayout = parent->layout();

  QPushButton* buttonQueue = new QPushButton(
      *getText( IDLE, CMDLINE_TAB_WELCOME, BUTTON_QUEUE ), parent );
  QPushButton* buttonMain = new QPushButton(
      *getText( IDLE, CMDLINE_TAB_WELCOME, BUTTON_MAIN ), parent );
  QPushButton* buttonPause = new QPushButton(
      *getText( IDLE, CMDLINE_TAB_WELCOME, BUTTON_PAUSE ), parent );
  setWidget( state, BUTTON_QUEUE, buttonQueue );
  setWidget( state, BUTTON_MAIN, buttonMain );
  setWidget( state, BUTTON_PAUSE, buttonPause );
  parentLayout->addWidget( buttonQueue );
  parentLayout->addWidget( buttonMain );
  parentLayout->addWidget( buttonPause );

  connect( buttonMain, SIGNAL( clicked() ), this, SLOT( mainButtonClicked() ) );
  connect( buttonQueue, SIGNAL( clicked() ), this,
           SLOT( queueButtonClicked() ) );
  connect( buttonPause, SIGNAL( clicked() ), this,
           SLOT( pauseButtonClicked() ) );
}

void SC_MainWindowCommandLine::createCommandLine( state_e state,
                                                          QWidget* parent )
{
  // creates the commandline
  QLayout* parentLayout = parent->layout();
  // Navigation buttons + Command Line
  QPushButton* buttonPrev = new QPushButton( tr( "<" ), parent );
  QPushButton* buttonNext = new QPushButton( tr( ">" ), parent );
  SC_CommandLine* commandLineEdit = new SC_CommandLine( parent );

  setWidget( state, BUTTON_PREV, buttonPrev );
  setWidget( state, BUTTON_NEXT, buttonNext );
  setWidget( state, TEXTEDIT_CMDLINE, commandLineEdit );

  buttonPrev->setMaximumWidth( 30 );
  buttonNext->setMaximumWidth( 30 );

  parentLayout->addWidget( buttonPrev );
  parentLayout->addWidget( buttonNext );
  parentLayout->addWidget( commandLineEdit );

  connect( buttonPrev, SIGNAL( clicked( bool ) ), this, SIGNAL( backButtonClicked() ) );
  connect( buttonNext, SIGNAL( clicked( bool ) ), this, SIGNAL( forwardButtonClicked() ) );
  connect( commandLineEdit, SIGNAL( switchToLeftSubTab() ), this,
           SIGNAL( switchToLeftSubTab() ) );
  connect( commandLineEdit, SIGNAL( switchToRightSubTab() ), this,
           SIGNAL( switchToRightSubTab() ) );
  connect( commandLineEdit, SIGNAL( currentlyViewedTabCloseRequest() ), this,
           SIGNAL( currentlyViewedTabCloseRequest() ) );
  connect( commandLineEdit, SIGNAL( returnPressed() ), this,
           SIGNAL( commandLineReturnPressed() ) );
  connect( commandLineEdit, SIGNAL( textEdited( const QString& ) ), this, SLOT( commandLineTextEditedSlot( const QString& ) ) );
  // Progress bar
  QProgressBar* progressBar = new QProgressBar( parent );

  setWidget( state, PROGRESSBAR_WIDGET, progressBar );

  progressBar->setMaximum( 100 );
  progressBar->setMaximumWidth( 200 );
  progressBar->setMinimumWidth( 150 );
  QFont override_font = QFont();
  override_font.setPixelSize( 20 );

  commandLineEdit -> setFont( override_font );

  QFont progressBarFont( progressBar->font() );
  progressBarFont.setPointSize( 14 );
  progressBar->setFont( progressBarFont );
  progressBar->setStyleSheet( QString::fromUtf8( "text-align: center;" ) );

  parentLayout->addWidget( progressBar );
}

bool SC_MainWindowCommandLine::tryToHideWidget( QString* text, QWidget* widget )
{
  // if the widget has the special text to hide it, then hide it; else show
  if ( text != nullptr )
  {
    if ( text == text_hide_widget )
    {
      widget->hide();
      return true;
    } else
    {
      widget->show();
    }
  }
  return false;
}

void SC_MainWindowCommandLine::emitSignal( QString* text )
{
  // emit the proper signal for the given button text
  if ( text != nullptr )
  {
    if ( text == text_pause )
      emit( pauseClicked() );
    else if ( text == text_resume )
      emit( resumeClicked() );
    else if ( text == text_simulate )
    {
#ifdef SC_PAPERDOLL
      if ( current_tab == TAB_PAPERDOLL )
      {
        emit( simulatePaperdollClicked() );
      }
      else
#endif
      {
        emit( simulateClicked() );
      }
    } else if ( text == text_cancel )
    {
      switch ( current_tab )
      {
      case CMDLINE_TAB_IMPORT:
        emit( cancelImportClicked() );
        break;
      default:
        emit( cancelSimulationClicked() );
        break;
      }
    } else if ( text == text_cancel_all )
    {
      emit( cancelImportClicked() );
      emit( cancelAllSimulationClicked() );
    } else if ( text == text_import )
    {
      emit( importClicked() );
    } else if ( text == text_queue )
    {
      emit( queueClicked() );
    } else if ( text == text_spellquery )
    {
      emit( queryClicked() );
    } else if ( text == text_save )
    {
      switch ( current_tab )
      {
      case CMDLINE_TAB_LOG:
        emit( saveLogClicked() );
        break;
      case CMDLINE_TAB_RESULTS:
        emit( saveResultsClicked() );
        break;
      default:
        assert( 0 );
      }
    }
  }
}

// Accessors for widgets/displayable text; hide underlying implementation of states even from init()
void SC_MainWindowCommandLine::setProgressBarProgress(
    progressbar_states_e state, int value )
{
  // update progress for the given progressbar state
  progressBarFormat[state].progress = value;
  updateWidget( current_state, current_tab, PROGRESSBAR_WIDGET );
}

int SC_MainWindowCommandLine::getProgressBarProgressForState(
    progressbar_states_e state )
{
  return progressBarFormat[state].progress;
}

void SC_MainWindowCommandLine::updateProgressBars()
{
  // actually updates currently visible progressbar's progress
  QProgressBar* progressBar = qobject_cast<QProgressBar*>(
      getWidget( current_state, PROGRESSBAR_WIDGET ) );
  if ( progressBar != nullptr )
  {
    progressBar->setValue(
        getProgressBarProgressForState(
            getProgressBarStateForState( current_state, current_tab ) ) );
  }
}

void SC_MainWindowCommandLine::adjustText( state_e state, tabs_e tab,
                                           widgets_e widget, QString text )
{
  // only change the text's value, not where it points to, only if its not null
  if ( states[state][tab][widget].text != nullptr )
  {
    ( *states[state][tab][widget].text ) = text;
  }
}

void SC_MainWindowCommandLine::setText( state_e state, tabs_e tab,
                                        widgets_e widget, QString* text,
                                        QString* tooltip )
{
  // change the text and tooltip's pointer values
  states[state][tab][widget].text = text;
  states[state][tab][widget].tool_tip = tooltip;
}

void SC_MainWindowCommandLine::setProgressBarState(
    state_e state, tabs_e tab, progressbar_states_e progressbar_state )
{
  // set the given state->tab->progressbar's state
  states[state][tab][PROGRESSBAR_WIDGET].text =
      &( progressBarFormat[progressbar_state].text );
  states[state][tab][PROGRESSBAR_WIDGET].tool_tip =
      &( progressBarFormat[progressbar_state].tool_tip );
  states[state][tab][PROGRESSBAR_WIDGET].progressbar_state = progressbar_state;
}

SC_MainWindowCommandLine::progressbar_states_e SC_MainWindowCommandLine::getProgressBarStateForState(
    state_e state, tabs_e tab )
{
  // returns state->tab->progressbar's state
  return states[state][tab][PROGRESSBAR_WIDGET].progressbar_state;
}

void SC_MainWindowCommandLine::updateWidget( state_e state, tabs_e tab,
                                             widgets_e widget )
{
  // update a given widget
  QString* text = getText( state, tab, widget );
  QString* toolTip = getToolTip( state, tab, widget );
  if ( text != nullptr )
  {
    if ( tab == current_tab )
    {
      switch ( widget )
      {
      case TEXTEDIT_CMDLINE:
      {
        SC_CommandLine* commandLine = qobject_cast<SC_CommandLine*>(
            getWidget( state, TEXTEDIT_CMDLINE ) );
        if ( commandLine != nullptr )
        {
          commandLine->setText( *text );
          if ( toolTip != nullptr )
          {
            commandLine->setToolTip( *toolTip );
          }
        }
        break;
      }
      case PROGRESSBAR_WIDGET:
      {
        QProgressBar* progressBar = qobject_cast<QProgressBar*>(
            getWidget( state, PROGRESSBAR_WIDGET ) );
        if ( progressBar != nullptr )
        {
          progressBar->setFormat( *text );
          progressBar->setValue(
              getProgressBarProgressForState(
                  getProgressBarStateForState( current_state, current_tab ) ) );
          if ( toolTip != nullptr )
          {
            progressBar->setToolTip( *toolTip );
          }
        }
        break;
      }
      default:
        QPushButton* button = qobject_cast<QPushButton*>(
            getWidget( state, widget ) );
        if ( button != nullptr )
        {
          button->setText( *text );
          if ( toolTip != nullptr )
          {
            button->setToolTip( *toolTip );
          }
          break;
        }
      }
    }
  }
}

QString* SC_MainWindowCommandLine::getText( state_e state, tabs_e tab,
                                            widgets_e widget )
{
  // returns text to set the specified widget to
  return states[state][tab][widget].text;
}

QString* SC_MainWindowCommandLine::getToolTip( state_e state, tabs_e tab,
                                               widgets_e widget )
{
  return states[state][tab][widget].tool_tip;
}

QWidget* SC_MainWindowCommandLine::getWidget( state_e state, widgets_e widget )
{
  // returns the widget for the specified state
  return widgets[state][widget];
}

void SC_MainWindowCommandLine::setWidget( state_e state, widgets_e widget,
                                          QWidget* widgetPointer )
{
  // sets the widget for the specified state
  widgets[state][widget] = widgetPointer;
}

void SC_MainWindowCommandLine::setProgressBarFormat( progressbar_states_e state,
                                                     QString format,
                                                     bool update )
{
  // sets the QProgressBar->setFormat(format) text value for the specified state's progress bar
  progressBarFormat[state].text = format;
  if ( update
      && getProgressBarStateForState( current_state, current_tab ) == state )
  {
    updateWidget( current_state, current_tab, PROGRESSBAR_WIDGET );
  }
}

void SC_MainWindowCommandLine::setProgressBarToolTip(
    progressbar_states_e state, QString toolTip, bool update )
{
  progressBarFormat[state].tool_tip = toolTip;
  if ( update
      && getProgressBarStateForState( current_state, current_tab ) == state )
  {
    updateWidget( current_state, current_tab, PROGRESSBAR_WIDGET );
  }
}

void SC_MainWindowCommandLine::updateProgress( progressbar_states_e state,
                                               int value, QString format,
                                               QString toolTip )
{
  setProgressBarFormat( state, format );
  setProgressBarToolTip( state, toolTip );
  setProgressBarProgress( state, value );
}

SC_MainWindowCommandLine::tabs_e SC_MainWindowCommandLine::convertTabsEnum( main_tabs_e tab )
{
  return static_cast<tabs_e>( tab );
}

SC_MainWindowCommandLine::tabs_e SC_MainWindowCommandLine::convertTabsEnum( import_tabs_e tab )
{
  return static_cast<tabs_e>( tab + TAB_COUNT );
}

void SC_MainWindowCommandLine::mainButtonClicked()
{
  emitSignal( getText( current_state, current_tab, BUTTON_MAIN ) );
}

void SC_MainWindowCommandLine::pauseButtonClicked()
{
  emitSignal( getText( current_state, current_tab, BUTTON_PAUSE ) );
}

void SC_MainWindowCommandLine::queueButtonClicked()
{
  emitSignal( getText( current_state, current_tab, BUTTON_QUEUE ) );
}

void SC_MainWindowCommandLine::setTab( main_tabs_e tab )
{
  setTab( convertTabsEnum( tab ) );
}

void SC_MainWindowCommandLine::setTab( import_tabs_e tab )
{
  setTab( convertTabsEnum( tab ) );
}

void SC_MainWindowCommandLine::setTab( tabs_e tab )
{
  assert(
      static_cast<int>( tab ) >= 0
          && static_cast<int>( tab ) < CMDLINE_TAB_COUNT
          && "setTab argument out of bounds" );
  current_tab = tab;
  // sets correct text for the current_state for the given tab on all widgets
  for ( widgets_e widget = BUTTON_MAIN; widget < WIDGET_COUNT; widget++ )
  {
    QWidget* wdgt = getWidget( current_state, widget );
    if ( wdgt != nullptr )
    {
      if ( !tryToHideWidget( getText( current_state, current_tab, widget ),
                             wdgt ) )
      {
        // only change text if it will not be hidden
        updateWidget( current_state, current_tab, widget );
      }
    }
  }
}

void SC_MainWindowCommandLine::setState( state_e state )
{
  // change to the specified state
  current_state = state;
  statesStackedLayout->setCurrentIndex( static_cast<int>( state ) );
  setTab( current_tab );
}

void SC_MainWindowCommandLine::commandLineTextEditedSlot( const QString& text )
{
  QString* current_text = getText( current_state, current_tab,
                                   TEXTEDIT_CMDLINE );
  if ( current_text != nullptr )
  {
    ( *current_text ) = text;
  } else
  {
    //TODO: something if current_text is null
  }

  emit( commandLineTextEdited( text ) );
}
