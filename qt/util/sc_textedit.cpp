// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_textedit.hpp"

#include "sc_searchbox.hpp"

// ============================================================================
// SC_TextEdit
// ============================================================================

// public

SC_TextEdit::SC_TextEdit( QWidget* parent, bool accept_drops, bool enable_search )
  : QPlainTextEdit( parent ),
    searchBox( nullptr ),
    searchBoxCorner( Qt::BottomLeftCorner ),
    enable_search( enable_search ),
    edited_by_user( false )
{
  textformat_error.setFontPointSize( 20 );
  setAcceptDrops( accept_drops );
  setLineWrapMode( QPlainTextEdit::NoWrap );
  // setAcceptRichText( false );
  QFont font;
  font.setPointSize( 10 );
  setFont( font );
  QList<Qt::KeyboardModifier> ctrl;
  ctrl.push_back( Qt::ControlModifier );
  addIgnoreKeyPressEvent( Qt::Key_Tab, ctrl );
  addIgnoreKeyPressEvent( Qt::Key_W, ctrl );  // Close tab
  addIgnoreKeyPressEvent( Qt::Key_T, ctrl );  // New tab
  QList<Qt::KeyboardModifier> nothing;
  addIgnoreKeyPressEvent( Qt::Key_Backtab, nothing );

  connect( this, SIGNAL( textChanged() ), this, SLOT( text_edited() ) );

  if ( enable_search )
  {
    initSearchBox();
  }
}

void SC_TextEdit::addIgnoreKeyPressEvent( Qt::Key k, const QList<Qt::KeyboardModifier>& s )
{
  QPair<Qt::Key, QList<Qt::KeyboardModifier> > p( k, s );
  if ( !ignoreKeys.contains( p ) )
    ignoreKeys.push_back( p );
}

bool SC_TextEdit::removeIgnoreKeyPressEvent( Qt::Key k, const QList<Qt::KeyboardModifier>& s )
{
  QPair<Qt::Key, QList<Qt::KeyboardModifier> > p( k, s );
  return ignoreKeys.removeAll( p );
}

void SC_TextEdit::removeAllIgnoreKeyPressEvent()
{
  QList<QPair<Qt::Key, QList<Qt::KeyboardModifier> > > emptyList;
  ignoreKeys = emptyList;
}

// protected virtual overrides

void SC_TextEdit::keyPressEvent( QKeyEvent* e )
{
  int k                   = e->key();
  Qt::KeyboardModifiers m = e->modifiers();

  QList<QPair<Qt::Key, QList<Qt::KeyboardModifier> > >::iterator i = ignoreKeys.begin();
  for ( ; i != ignoreKeys.end(); ++i )
  {
    if ( ( *i ).first == k )
    {
      bool passModifiers                      = true;
      QList<Qt::KeyboardModifier>::iterator j = ( *i ).second.begin();

      for ( ; j != ( *i ).second.end(); ++j )
      {
        if ( !m.testFlag( ( *j ) ) )
        {
          passModifiers = false;
          break;
        }
      }

      if ( passModifiers )
      {
        // key combination matches, send key to base classe's base
        QAbstractScrollArea::keyPressEvent( e );
        return;
      }
    }
  }
  // no key match
  QPlainTextEdit::keyPressEvent( e );
}

void SC_TextEdit::focusInEvent( QFocusEvent* e )
{
  if ( enable_search )
  {
    hideSearchBox();
  }
  QPlainTextEdit::focusInEvent( e );
}

void SC_TextEdit::resizeEvent( QResizeEvent* e )
{
  Q_UNUSED( e );
  searchBox->updateGeometry();
}

// public slots

void SC_TextEdit::find()
{
  if ( enable_search )
  {
    searchBox->show();
  }
}

void SC_TextEdit::findText( const QString& text )
{
  QTextDocument::FindFlags flags;
  if ( searchBox->reverseSearch() )
    flags |= QTextDocument::FindBackward;
  findSomeText( text, flags );
}

void SC_TextEdit::findNext()
{
  if ( enable_search )
    findSomeText( searchBox->text(), nullptr, textCursor().selectionStart() + 1 );
}

void SC_TextEdit::findPrev()
{
  if ( enable_search )
    findSomeText( searchBox->text(), QTextDocument::FindBackward );
}

void SC_TextEdit::hideSearchBox()
{
  if ( searchBox->isVisible() )
  {
    searchBox->hide();
    setFocus();
  }
}

// private

void SC_TextEdit::initSearchBox()
{
  if ( !searchBox )
  {
    searchBox = new SC_SearchBox( this->viewport(), searchBoxCorner );

    QShortcut* find = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_F ), this );
    connect( find, SIGNAL( activated() ), this, SLOT( find() ) );
    QShortcut* escape = new QShortcut( QKeySequence( Qt::Key_Escape ), this );
    connect( escape, SIGNAL( activated() ), this, SLOT( hideSearchBox() ) );
    connect( searchBox, SIGNAL( textChanged( const QString& ) ), this, SLOT( findText( const QString& ) ) );

    connect( searchBox, SIGNAL( findPrev() ), this, SLOT( findPrev() ) );
    connect( searchBox, SIGNAL( findNext() ), this, SLOT( findNext() ) );

    searchBox->hide();
  }
}

void SC_TextEdit::findSomeText( const QString& text, QTextDocument::FindFlags options, int position )
{
  if ( !text.isEmpty() )
  {
    QList<QTextEdit::ExtraSelection> extraSelections;
    QColor color( Qt::red );
    QTextEdit::ExtraSelection extra;
    extra.format.setBackground( color );
    QTextDocument* doc = document();
    if ( position < 0 )
      position = textCursor().selectionStart();
    QTextCursor found = doc->find( text, position, options );
    if ( !found.isNull() )
    {
      setTextCursor( found );
      extra.cursor = found;
      extraSelections.append( extra );
    }
    else if ( searchBox->wrapSearch() )
    {
      if ( options & QTextDocument::FindBackward )
      {
        position = doc->characterCount();
      }
      else
      {
        position = 0;
      }
      found = doc->find( text, position, options );
      if ( !found.isNull() )
      {
        setTextCursor( found );
        extra.cursor = found;
        extraSelections.append( extra );
      }
    }
  }
  if ( searchBox->isVisible() )
  {
    searchBox->updateGeometry();
  }
}
