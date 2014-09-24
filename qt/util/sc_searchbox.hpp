// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <QtWidgets/QtWidgets>
#include <QtGui/QtGui>

// ============================================================================
// SC_SearchBox
// ============================================================================

class SC_SearchBox : public QWidget
{
  Q_OBJECT
  QLineEdit* searchBox;
  QMenu* searchBoxContextMenu;
  QToolButton* searchBoxPrev;
  QToolButton* searchBoxNext;
  Qt::Corner corner;
  QAction* reverseAction;
  QAction* wrapAction;
  QAction* hideArrowsAction;
  bool reverse;
  bool wrap;
  bool enableReverseContextMenuItem;
  bool enableWrapAroundContextMenuItem;
  bool enableFindPrevNextContextMenuItem;
  bool enableHideArrowsContextMenuItem;
  bool emitFindOnTextChange;
  bool grabFocusOnShow;
  bool highlightTextOnShow;
public:
  SC_SearchBox( QWidget* parent = nullptr,
      Qt::Corner corner = Qt::BottomLeftCorner,
      bool show_arrows = true,
      QBoxLayout::Direction direction = QBoxLayout::LeftToRight );

  void setFocus()
  {
    searchBox -> setFocus();
  }
  QString text() const
  {
    return searchBox -> text();
  }
  void setText( const QString& text )
  {
    searchBox -> setText( text );
  }
  void clear()
  {
    searchBox -> clear();
  }
  void setEmitFindOnTextChange( bool emitOnChange )
  {
    emitFindOnTextChange = emitOnChange;
  }
  void updateGeometry();
  Qt::Corner getCorner()
  {
    return corner;
  }
  bool reverseSearch() const
  {
    return reverse;
  }
  bool wrapSearch() const
  {
    return wrap;
  }
signals:
  void textChanged( const QString& );
  void findNext();
  void findPrev();
public slots:
  void find();
  void setReverseSearch( bool reversed );
  void setWrapSearch( bool wrapped );
  void setHideArrows( bool hideArrows );
protected:
  virtual void keyPressEvent( QKeyEvent* e );
  virtual void showEvent( QShowEvent* e );
private slots:
  void searchBoxTextChanged( const QString& text );
  void searchBoxContextMenuRequested( const QPoint& p );
};
