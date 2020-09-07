// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>

class SC_ComboBoxIntegerValidator : public QValidator
{
  Q_OBJECT
  int lowerBoundInclusive;
  int upperBoundInclusive;
  int lowerBoundDigitCount;
  int upperBoundDigitCount;
  QRegExp nonIntegerRegExp;
  QComboBox* comboBox;

public:
  SC_ComboBoxIntegerValidator( int lowerBoundIntegerInclusive, int upperBoundIntegerInclusive, QComboBox* parent );

  static SC_ComboBoxIntegerValidator* CreateBoundlessValidator( QComboBox* parent = nullptr );

  static SC_ComboBoxIntegerValidator* CreateUpperBoundValidator( int upperBound, QComboBox* parent = nullptr );

  static SC_ComboBoxIntegerValidator* CreateLowerBoundValidator( int lowerBound, QComboBox* parent = nullptr );

  static QComboBox* ApplyValidatorToComboBox( QValidator* validator, QComboBox* comboBox );

  QValidator::State isNumberValid( int number ) const;

  bool isNumberInRange( int number ) const;

  void stripNonNumbersAndAdjustCursorPos( QString& input, int& cursorPos ) const;

  void fixup( QString& input ) const override;

  QValidator::State validate( QString& input, int& cursorPos ) const override;
};
