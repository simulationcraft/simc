// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_searchbox.hpp"


void SC_SearchBoxLineEdit::focusOutEvent( QFocusEvent* e )
{
  QLineEdit::focusOutEvent( e );
  emit(lostFocus());
}

// ============================================================================
// SC_SearchBox
// ============================================================================

SC_SearchBox::SC_SearchBox( QWidget* parent,
                            Qt::Corner corner,
                            bool show_arrows,
                            QBoxLayout::Direction direction ) :
  QWidget( parent ),
  searchBox( new SC_SearchBoxLineEdit( this ) ),
  searchBoxContextMenu( nullptr ),
  searchBoxPrev( new QToolButton( this ) ),
  searchBoxNext( new QToolButton( this ) ),
  corner( corner ),
  reverseAction( nullptr ),
  wrapAction( nullptr ),
  hideArrowsAction( nullptr ),
  reverse( false ),
  wrap( true ),
  enableReverseContextMenuItem( true ),
  enableWrapAroundContextMenuItem( true ),
  enableFindPrevNextContextMenuItem( true ),
  enableHideArrowsContextMenuItem( true ),
  emitFindOnTextChange( false ),
  grabFocusOnShow( true ),
  highlightTextOnShow( true )
{
  QSettings settings;
  bool hide_arrows_setting = settings.value( "gui/search_hide_arrows", ! show_arrows ).toBool();

  QAction* altLeftAction =  new QAction( tr( "Find Previous" ), searchBox );
  altLeftAction -> setShortcut( QKeySequence( Qt::ALT + Qt::Key_Left  ) );
  QAction* altRightAction =  new QAction( tr( "Find Next" ), searchBox );
  altRightAction -> setShortcut( QKeySequence( Qt::ALT + Qt::Key_Right ) );
  QShortcut* altLeft =  new QShortcut( altLeftAction -> shortcut(), searchBox );
  QShortcut* altUp =    new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Up    ), searchBox );
  QShortcut* altRight = new QShortcut( altRightAction -> shortcut(), searchBox );
  QShortcut* altDown =  new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Down  ), searchBox );

  if ( enableReverseContextMenuItem ||
       enableWrapAroundContextMenuItem ||
       enableFindPrevNextContextMenuItem )
  {
    searchBoxContextMenu = searchBox -> createStandardContextMenu();
    searchBoxContextMenu -> addSeparator();
    searchBox -> setContextMenuPolicy( Qt::CustomContextMenu );
    connect( searchBox,     SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( searchBoxContextMenuRequested( const QPoint& ) ) );
  }
  if ( enableFindPrevNextContextMenuItem )
  {
    searchBoxContextMenu -> addAction( altRightAction );
    searchBoxContextMenu -> addAction( altLeftAction );
  }
  if ( enableReverseContextMenuItem )
  {
    reverseAction = searchBoxContextMenu -> addAction( tr( "Reverse" ) );
    reverseAction -> setCheckable( true );
    reverseAction -> setChecked( reverse );
    connect( reverseAction, SIGNAL( triggered( bool ) ), this, SLOT( setReverseSearch( bool ) ) );
  }
  if ( enableWrapAroundContextMenuItem )
  {
    wrapAction = searchBoxContextMenu -> addAction( tr( "&Wrap search" ) );
    wrapAction -> setCheckable( true );
    wrapAction -> setChecked( wrap );
    connect( wrapAction,    SIGNAL( triggered( bool ) ), this, SLOT( setWrapSearch( bool ) ) );
  }
  if ( enableHideArrowsContextMenuItem )
  {
    hideArrowsAction = searchBoxContextMenu -> addAction( tr( "Hide Arrows" ) );
    hideArrowsAction -> setCheckable( true );
    hideArrowsAction -> setChecked( hide_arrows_setting ); // TODO connect to QSettings && sync all other SC_SearchBoxes on change
    connect( hideArrowsAction, SIGNAL( triggered( bool ) ), this, SLOT( setHideArrows( bool ) ) );
  }

  connect( searchBox,     SIGNAL( textChanged( const QString& ) ), this, SIGNAL( textChanged( const QString& ) ) );
  connect( searchBox,     SIGNAL( textChanged( const QString& ) ), this,   SLOT( searchBoxTextChanged( const QString& ) ) );
  connect( altLeftAction, SIGNAL( triggered( bool ) ),             this, SIGNAL( findPrev() ) );
  connect( altLeft,       SIGNAL( activated() ),                   this, SIGNAL( findPrev() ) );
  connect( altUp,         SIGNAL( activated() ),                   this, SIGNAL( findPrev() ) );
  connect( altRightAction,SIGNAL( triggered( bool ) ),             this, SIGNAL( findNext() ) );
  connect( altRight,      SIGNAL( activated() ),                   this, SIGNAL( findNext() ) );
  connect( altDown,       SIGNAL( activated() ),                   this, SIGNAL( findNext() ) );
  connect( searchBoxPrev, SIGNAL( pressed() ),                     this, SIGNAL( findPrev() ) );
  connect( searchBoxNext, SIGNAL( pressed() ),                     this, SIGNAL( findNext() ) );
  connect( searchBox,     SIGNAL( lostFocus() ),                   this,   SLOT( focusLostOnChild() ) );

  QIcon searchPrevIcon(":/icon/leftarrow.png");
  QIcon searchNextIcon(":/icon/rightarrow.png");
  if ( ! searchPrevIcon.pixmap( QSize( 64,64 ) ).isNull() )
  {
    searchBoxPrev -> setIcon( searchPrevIcon );
  }
  else
  {
    searchBoxPrev -> setText( tr( "<" ) );
  }
  if ( ! searchNextIcon.pixmap( QSize( 64,64 ) ).isNull() )
  {
    searchBoxNext -> setIcon( searchNextIcon );
  }
  else
  {
    searchBoxNext -> setText( tr( ">" ) );
  }

  QWidget* first = searchBoxPrev;
  QWidget* second = searchBoxNext;
  if ( direction == QBoxLayout::RightToLeft )
  {
    QWidget* tmp = first;
    first = second;
    second = tmp;
  }
  else
    direction = QBoxLayout::LeftToRight;

  QBoxLayout* searchBoxLayout = new QBoxLayout( direction );
  searchBoxLayout -> addWidget( searchBox );
  searchBoxLayout -> addWidget( first );
  searchBoxLayout -> addWidget( second );
  if ( hide_arrows_setting )
  {
    searchBoxPrev -> hide();
    searchBoxNext -> hide();
  }
  setLayout( searchBoxLayout );

  // If this is not set, focus will be sent to parent when user tries to click
  // Which will usually be a SC_TextEdit which will call hide() upon grabbing focus
  searchBoxPrev -> setFocusPolicy( Qt::ClickFocus );
  searchBoxNext -> setFocusPolicy( Qt::ClickFocus );
}

void SC_SearchBox::updateGeometry()
{
  QLayout* widgetLayout = layout();
  QSize widgetSizeHint = widgetLayout -> sizeHint();
  QRect geo( QPoint( 0, 0 ), widgetSizeHint );

  QWidget* parentWidget = qobject_cast< QWidget* >( parent() );
  if ( parentWidget != nullptr )
  {
    QRect parentGeo = parentWidget -> geometry();

    switch( corner )
    {
    case Qt::BottomLeftCorner:
      geo.moveBottomLeft( parentGeo.bottomLeft() );
      break;
    case Qt::TopRightCorner:
      geo.moveTopRight( parentGeo.topRight() );
      break;
    case Qt::BottomRightCorner:
      geo.moveBottomRight( parentGeo.bottomRight() );
      break;
    case Qt::TopLeftCorner:
    default:
      break;
    }

    setGeometry( geo );
  }
}

void SC_SearchBox::keyPressEvent( QKeyEvent* e )
{
  // Sending other keys to a QTextEdit (which is probably the parent)
  // Such as return, causes the textedit to not only insert a newline
  // while we are searching... but messes up the input of the textedit pretty badly

  // Only propagate Ctrl+Tab & Backtab
  bool propagate = false;
  switch( e -> key() )
  {
  case Qt::Key_Tab:
    if ( e -> modifiers().testFlag( Qt::ControlModifier ) )
      propagate = true;
    break;
  case Qt::Key_Backtab:
    propagate = true;
    break;
  case Qt::Key_Return:
    // We can use searchBox's returnPressed() signal, but then Shift+Return won't work
    if ( e -> modifiers().testFlag( Qt::ShiftModifier ) )
    {
      findPrev();
    }
    else
    {
      find();
    }
    break;
  default:
    break;
  }
  if ( propagate )
    QWidget::keyPressEvent( e );
}

void SC_SearchBox::showEvent( QShowEvent* e )
{
  Q_UNUSED( e );
  updateGeometry();
  if ( grabFocusOnShow )
  {
    setFocus();
  }
  if ( highlightTextOnShow )
  {
    searchBox -> selectAll();
  }
}


// public slots

void SC_SearchBox::find()
{
  if ( ! reverse )
  {
    emit( findNext() );
  }
  else
  {
    emit( findPrev() );
  }
}

void SC_SearchBox::setReverseSearch( bool reversed )
{
  reverse = reversed;
  if ( reverseAction && reverseAction -> isChecked() != reverse )
  {
    reverseAction -> setChecked( reverse );
  }
}

void SC_SearchBox::setWrapSearch( bool wrapped )
{
  wrap = wrapped;
  if ( wrapAction && wrapAction -> isChecked() != wrap )
  {
    wrapAction -> setChecked( wrap );
  }
}

void SC_SearchBox::setHideArrows( bool hideArrows )
{
  searchBoxPrev -> setVisible( ! hideArrows );
  searchBoxNext -> setVisible( ! hideArrows );
  updateGeometry();
  if ( hideArrowsAction && hideArrowsAction -> isChecked() != hideArrows )
  {
    hideArrowsAction -> setChecked( hideArrows );
  }
}

void SC_SearchBox::focusLostOnChild()
{
    bool any_focus = false;
    for( QWidget* child : {(QWidget*)searchBox, (QWidget*)searchBoxPrev, (QWidget*)searchBoxNext} )
    {
        if ( child->hasFocus())
        {
            any_focus = true;
        }
    }
    if ( !any_focus )
    {
        hide();
    }
}

// private slots

void SC_SearchBox::searchBoxTextChanged( const QString& text )
{
  Q_UNUSED( text );
  if ( emitFindOnTextChange )
    find();
}

void SC_SearchBox::searchBoxContextMenuRequested( const QPoint& p )
{
  searchBoxContextMenu -> exec( searchBox -> mapToGlobal( p ) );
}
