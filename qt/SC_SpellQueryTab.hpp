#ifndef SC_SPELLQUERYTAB_HPP
#define SC_SPELLQUERYTAB_HPP

#ifdef QT_VERSION_5
#include <QtWidgets/QtWidgets>
#else
#include <QtGui/QtGui>
#endif

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

  void    decodeOptions();
  void    encodeOptions();
  void    run_spell_query();

  void createToolTips();

  struct choices_t
  {
    // options
    QComboBox* spell;
    QComboBox* filter;
  } choice;

  struct labels_t
  {
    QLabel* input;
    QLabel* output;
  } label;

  struct textboxes_t
  {
    QLineEdit* arg;
    SC_TextEdit* result;
  } textbox;

//  
//public slots:
//  void _resetallSettings();
protected:
  SC_MainWindow* mainWindow;
  void load_setting( QSettings& s, const QString& name, QComboBox* choice, const QString& default_value );
};

#endif // SC_SPELLQUERYTAB_HPP
