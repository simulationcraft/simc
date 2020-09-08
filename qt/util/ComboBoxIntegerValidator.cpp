// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "ComboBoxIntegerValidator.hpp"

#include "util/util.hpp"

SC_ComboBoxIntegerValidator::SC_ComboBoxIntegerValidator( int lowerBoundIntegerInclusive,
                                                          int upperBoundIntegerInclusive, QComboBox* parent )
  : QValidator( parent ),
    lowerBoundInclusive( lowerBoundIntegerInclusive ),
    upperBoundInclusive( upperBoundIntegerInclusive ),
    lowerBoundDigitCount( 0 ),
    upperBoundDigitCount( 0 ),
    nonIntegerRegExp( "\\D*" ),
    comboBox( parent )
{
  Q_ASSERT( lowerBoundInclusive <= upperBoundInclusive && "Invalid Arguments" );

  lowerBoundDigitCount = util::numDigits( lowerBoundInclusive );
  upperBoundDigitCount = util::numDigits( upperBoundInclusive );
}
SC_ComboBoxIntegerValidator* SC_ComboBoxIntegerValidator::CreateBoundlessValidator( QComboBox* parent )
{
  return new SC_ComboBoxIntegerValidator( std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), parent );
}

SC_ComboBoxIntegerValidator* SC_ComboBoxIntegerValidator::CreateUpperBoundValidator( int upperBound, QComboBox* parent )
{
  return new SC_ComboBoxIntegerValidator( std::numeric_limits<int>::min(), upperBound, parent );
}

SC_ComboBoxIntegerValidator* SC_ComboBoxIntegerValidator::CreateLowerBoundValidator( int lowerBound, QComboBox* parent )
{
  return new SC_ComboBoxIntegerValidator( lowerBound, std::numeric_limits<int>::max(), parent );
}

QComboBox* SC_ComboBoxIntegerValidator::ApplyValidatorToComboBox( QValidator* validator, QComboBox* comboBox )
{
  if ( comboBox != 0 )
  {
    if ( validator != 0 )
    {
      comboBox->setEditable( true );
      comboBox->setValidator( validator );
    }
  }
  return comboBox;
}

QValidator::State SC_ComboBoxIntegerValidator::isNumberValid( int number ) const
{
  if ( isNumberInRange( number ) )
  {
    return QValidator::Acceptable;
  }
  else
  {
    // number is not in range... maybe it COULD be in range if the user types more
    if ( util::numDigits( number ) < lowerBoundDigitCount )
    {
      return QValidator::Intermediate;
    }
    // has enough digits... not valid
  }

  return QValidator::Invalid;
}

bool SC_ComboBoxIntegerValidator::isNumberInRange( int number ) const
{
  return ( number >= lowerBoundInclusive && number <= upperBoundInclusive );
}

void SC_ComboBoxIntegerValidator::stripNonNumbersAndAdjustCursorPos( QString& input, int& cursorPos ) const
{
  // remove erroneous characters
  QString modifiedInput = input.remove( nonIntegerRegExp );
  if ( cursorPos > 0 )
  {
    // move the cursor to the left by how many characters to the left gets removed
    QString charactersLeftOfCursor = input.leftRef( cursorPos ).toString();
    int characterCountLeftOfCursor = charactersLeftOfCursor.length();
    // count how many characters are removed left of cursor
    charactersLeftOfCursor                = charactersLeftOfCursor.remove( nonIntegerRegExp );
    int removedCharacterCountLeftOfCursor = characterCountLeftOfCursor - charactersLeftOfCursor.length();
    int newCursorPos                      = qAbs( cursorPos - removedCharacterCountLeftOfCursor );
    // just double check for sanity that it is in bounds
    Q_ASSERT( qBound( 0, newCursorPos, modifiedInput.length() ) == newCursorPos );
    cursorPos = newCursorPos;
  }
  input = modifiedInput;
}

void SC_ComboBoxIntegerValidator::fixup( QString& input ) const
{
  int cursorPos = 0;
  stripNonNumbersAndAdjustCursorPos( input, cursorPos );
}

QValidator::State SC_ComboBoxIntegerValidator::validate( QString& input, int& cursorPos ) const
{
  State retval = QValidator::Invalid;

  if ( input.length() != 0 )
  {
    stripNonNumbersAndAdjustCursorPos( input, cursorPos );

    bool conversionToIntWentOk;
    int number = input.toInt( &conversionToIntWentOk );

    if ( conversionToIntWentOk )
    {
      retval = isNumberValid( number );
    }
  }
  else
  {
    // zero length
    retval = QValidator::Intermediate;
  }

  return retval;
}
