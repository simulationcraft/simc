
#include "simcraft.h"
using namespace std;


// utility functions
std::string tolower( std::string src )
{
  std::string dest=src;
  for ( unsigned i=0; i<dest.length(); i++ )
    dest[i]=tolower( dest[i] );
  return dest;
}

std::string trim( std::string src )
{
  if (src=="") return "";
  std::string dest=src;
  //remove left
  int p=0;
  while ((p<(int)dest.length())&&(dest[p]==' ')) p++;
  if (p>0) 
    dest.erase(0,p);
  //remove right
  p=dest.length()-1;
  while ((p>=0)&&(dest[p]==' ')) p--;
  if (p<(int)dest.length()-1) 
    dest.erase(p+1);
  //return trimmed string
  return dest;
}


void replace_char( std::string& src, char old_c, char new_c  )
{
  for (int i=0; i<(int)src.length(); i++)
    if (src[i]==old_c)
      src[i]=new_c;
}

void replace_str( std::string& src, std::string old_str, std::string new_str  )
{
  if (old_str=="") return;
  std::string dest="";
  size_t p;
  while ((p=src.find(old_str))!=std::string::npos){
    dest+=src.substr(0,p)+new_str;
    src.erase(0,p+old_str.length());
  }
  dest+=src;
  src=dest;
}


// convert full descriptive name into "option name"
// spaces become underscores, all lower letters
// remove apostrophes and colons
std::string proper_option_name( const std::string& full_name )
{
  if ( full_name=="" ) return full_name;
  // first to lower letters and _ for spaces
  std::string newName( full_name.length(),' ' );
  newName="";
  for ( size_t i=0; i<full_name.length(); i++ )
  {
    char c=full_name[i];
    c= tolower( c );            // lower case
    if ( c==' ' ) c='_';        // spaces to underscores
    if ( c!='\'' && c!=':' )      // remove apostrophes and colons
      newName+=c;
  }
  // then remove first "of" for glyphs (legacy)
  if ( newName.substr( 0,9 )=="glyph_of_" )   newName.erase( 6,3 );
  // return result
  return newName;
}

bool my_isdigit( char c )
{
  if ( c=='+' ) return true;
  if ( c=='-' ) return true;
  return isdigit( c )!=0;
}

bool str_to_float(std::string src, double& dest )
{
  bool was_sign=false;
  bool was_digit=false;
  bool was_dot=false;
  bool res=true;
  //check each char
  for (size_t p=0; res&&(p<src.length()); p++){
    char ch=src[p];
    if (ch==' ') continue;
    if (((ch=='+')||(ch=='-'))&& !(was_sign||was_digit)){
      was_sign=true;
      continue;
    }
    if ((ch=='.')&& was_digit && !was_dot){
      was_dot=true;
      continue;
    }
    if ((ch>='0')&&(ch<='9')){
      was_digit=true;
      continue;
    }
    //if none of above, fail
    res=false;
  }
  //check if we had at least one digit
  if (!was_digit) res=false;
  //return result
  dest=atof(src.c_str());
  return res;
}
