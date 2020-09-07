// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>

class SC_SearchBoxLineEdit : public QLineEdit
{
  Q_OBJECT
public:
  SC_SearchBoxLineEdit( QWidget* parent = nullptr ) : QLineEdit( parent )
  {
  }
signals:
  void lostFocus();

protected:
  void focusOutEvent( QFocusEvent* e ) override;
};

// ============================================================================
// SC_SearchBox
// ============================================================================

class SC_SearchBox : public QWidget
{
  Q_OBJECT
  SC_SearchBoxLineEdit* searchBox;
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
  SC_SearchBox( QWidget* parent = 0, Qt::Corner corner = Qt::BottomLeftCorner, bool show_arrows = true,
                QBoxLayout::Direction direction = QBoxLayout::LeftToRight );

  void setFocus()
  {
    searchBox->setFocus( Qt::MouseFocusReason );
  }
  QString text() const
  {
    return searchBox->text();
  }
  void setText( const QString& text )
  {
    searchBox->setText( text );
  }
  void clear()
  {
    searchBox->clear();
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
  void focusLostOnChild();

protected:
  void keyPressEvent( QKeyEvent* e ) override;
  void showEvent( QShowEvent* e ) override;
private slots:
  void searchBoxTextChanged( const QString& text );
  void searchBoxContextMenuRequested( const QPoint& p );
};
