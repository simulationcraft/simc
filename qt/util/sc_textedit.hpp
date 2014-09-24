// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <QtWidgets/QtWidgets>
#include <QtGui/QtGui>

class SC_SearchBox;

// ============================================================================
// SC_TextEdit
// ============================================================================

class SC_TextEdit : public QTextEdit
{
  Q_OBJECT
private:
  QTextCharFormat textformat_default;
  QTextCharFormat textformat_error;
  QList< QPair< Qt::Key, QList< Qt::KeyboardModifier > > > ignoreKeys;
  SC_SearchBox* searchBox;
  Qt::Corner searchBoxCorner;
  bool enable_search;
public:
  bool edited_by_user;

  SC_TextEdit( QWidget* parent = nullptr, bool accept_drops = true, bool enable_search = true );

  void setformat_error()
  { //setCurrentCharFormat( textformat_error );
  }

  void resetformat()
  { //setCurrentCharFormat( textformat_default );
  }

  /*
  protected:
  virtual void dragEnterEvent( QDragEnterEvent* e )
  {
    e -> acceptProposedAction();
  }
  virtual void dropEvent( QDropEvent* e )
  {
    appendPlainText( e -> mimeData()-> text() );
    e -> acceptProposedAction();
  }
  */

  void addIgnoreKeyPressEvent( Qt::Key k, QList<Qt::KeyboardModifier> s);

  bool removeIgnoreKeyPressEvent( Qt::Key k, QList<Qt::KeyboardModifier> s);

  void removeAllIgnoreKeyPressEvent();

protected:
  virtual void keyPressEvent( QKeyEvent* e );
  virtual void focusInEvent( QFocusEvent* e );
  virtual void resizeEvent( QResizeEvent* e );

private slots:
  void text_edited()
  {
    edited_by_user = true;
  }
public slots:
  void find();
  void findText( const QString& text );
  void findNext();
  void findPrev();
  void hideSearchBox();
private:

  void initSearchBox();
  void findSomeText( const QString& text, QTextDocument::FindFlags options = 0, int position = -1 );
};
