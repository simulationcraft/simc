// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>

class SC_SearchBox;

// ============================================================================
// SC_TextEdit
// ============================================================================

class SC_TextEdit : public QPlainTextEdit
{
  Q_OBJECT
private:
  QTextCharFormat textformat_default;
  QTextCharFormat textformat_error;
  QList<QPair<Qt::Key, QList<Qt::KeyboardModifier> > > ignoreKeys;
  SC_SearchBox* searchBox;
  Qt::Corner searchBoxCorner;
  bool enable_search;

public:
  bool edited_by_user;

  SC_TextEdit( QWidget* parent = nullptr, bool accept_drops = true, bool enable_search = true );

  void addIgnoreKeyPressEvent( Qt::Key k, const QList<Qt::KeyboardModifier>& s );

  bool removeIgnoreKeyPressEvent( Qt::Key k, const QList<Qt::KeyboardModifier>& s );

  void removeAllIgnoreKeyPressEvent();

protected:
  void keyPressEvent( QKeyEvent* e ) override;
  void focusInEvent( QFocusEvent* e ) override;
  void resizeEvent( QResizeEvent* e ) override;

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
