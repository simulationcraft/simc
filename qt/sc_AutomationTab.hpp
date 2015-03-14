#pragma once
#include <QtWidgets/QtWidgets>
#include <QtGui/QtGui>
#include "simulationcraftqt.hpp"

class QComboBox;
class QLabel;
class QLineEdit;
class SC_TextEdit;
class QSettings;

class SC_AutomationTab : public QScrollArea
{
    Q_OBJECT
public:

    // these are options on the Automation tab
    struct choices_t
    {
      QComboBox* player_class;
      QComboBox* player_spec;
      QComboBox* player_race;
      QComboBox* player_level;
      QComboBox* comp_type;
    } choice;

    struct labels_t
    {
      QLabel* talents;
      QLabel* glyphs;
      QLabel* gear;
      QLabel* rotationHeader;
      QLabel* rotationFooter;
      QLabel* advanced;
      QLabel* sidebar;
      QLabel* footer;
    } label;

    struct textBoxes_t
    {
      QLineEdit* talents;
      QLineEdit* glyphs;
      SC_TextEdit* gear;
      SC_TextEdit* rotationHeader;
      SC_TextEdit* rotationFooter;
      SC_TextEdit* advanced;
      SC_TextEdit* sidebar;
      SC_TextEdit* helpbar;
      SC_TextEdit* footer;
    } textbox;

    QString advTalent;
    QString advGlyph;
    QString advGear;
    QString advRotation;
    SC_AutomationTab( QWidget* parent = 0 );
    void encodeSettings();
    void load_setting( QSettings& s, const QString& name, QComboBox* choice, const QString& default_value );
    void load_setting( QSettings& s, const QString& name, QString* text, const QString& default_value );
    void load_setting( QSettings& s, const QString& name, QLineEdit* textbox, const QString& default_value );
    void load_setting( QSettings& s, const QString& name, SC_TextEdit* textbox, const QString& default_value );
    void decodeSettings();
    void createTooltips();

public slots:
    QString startImport();
    void setSpecDropDown( const int player_class );
    void setSidebarClassText();
    void compTypeChanged( const int comp );

};
