// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#pragma once
#include "config.hpp"

#include <QtWidgets/QtWidgets>

class SC_MainWindow;
class SC_TextEdit;

// ============================================================================
// SC_SpellQueryTab Widget
// ============================================================================

class SC_SpellQueryTab : public QWidget
{
  Q_OBJECT
public:
  SC_SpellQueryTab( SC_MainWindow* parent );

  void decodeSettings();
  void encodeSettings();
  void run_spell_query();
  void checkForSave();

  void createToolTips();

  struct choices_t
  {
    // options
    QComboBox* source;
    QComboBox* filter;
    QComboBox* operatorString;
    QCheckBox* saveToFile;
    QComboBox* directory;
  } choice;

  struct labels_t
  {
    QLabel* source;
    QLabel* filter;
    QLabel* operatorString;
    QLabel* arg;
    QLabel* input;
    QLabel* output;
  } label;

  struct textboxes_t
  {
    QLineEdit* arg;
    SC_TextEdit* result;
  } textbox;

  struct buttons_t
  {
    QPushButton* save;
  } button;

  //
public slots:
  void sourceTypeChanged( const int source_index );
  void filterTypeChanged( const int filter_index );
  void runSpellQuerySlot();
  void browseForFile();

protected:
  SC_MainWindow* mainWindow;
  void load_setting( QSettings& s, const QString& name, QComboBox* choice, const QString& default_value );
  void load_setting( QSettings& s, const QString& name, QLineEdit* textbox, const QString& default_value );
  void load_setting( QSettings& s, const QString& name, SC_TextEdit* textbox, const QString& default_value );
};
