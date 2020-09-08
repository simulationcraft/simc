// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "WebEngineConfig.hpp"

class SC_SearchBox;
class SC_MainWindow;

// ============================================================================
// SC_WebView
// ============================================================================

class SC_WebView : public SC_WebEngineView
{
  Q_OBJECT
  SC_SearchBox* searchBox;
  QString previousSearch;
  bool allow_mouse_navigation;
  bool allow_keyboard_navigation;
  bool allow_searching;

public:
  SC_MainWindow* mainWindow;
  int progress;
  QString html_str;
  QString url_to_show;
  QByteArray out_html;

  SC_WebView( SC_MainWindow* mw, QWidget* parent = nullptr, const QString& h = QString() );

  void store_html( const QString& s );

  virtual ~SC_WebView() = default;

  void loadHtml();

  void enableMouseNavigation();

  void disableMouseNavigation();

  void enableKeyboardNavigation();

  void disableKeyboardNavigation();

private:
  void update_progress( int p );

protected:
  void mouseReleaseEvent( QMouseEvent* e ) override;

  void keyReleaseEvent( QKeyEvent* e ) override;

  void resizeEvent( QResizeEvent* e ) override;

  void focusInEvent( QFocusEvent* e ) override;

private slots:
  void loadProgressSlot( int p );

  void loadFinishedSlot( bool /* ok */ );

  void urlChangedSlot( const QUrl& url );

public slots:
  void hideSearchBox();

  void findSomeText( const QString& text, SC_WebEnginePage::FindFlags options );

  void findSomeText( const QString& text );

  void findPrev();

  void findNext();
};
