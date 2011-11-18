// ====================================================================================================================
// ====================================================================================================================
//  xs_Float.h
// ====================================================================================================================
// ====================================================================================================================
#ifndef _xs_FLOAT_H_
#define _xs_FLOAT_H_

#ifndef _xs_BigEndian_
#define _xs_BigEndian_              0       //intel is little endian
#endif




// ====================================================================================================================
//  Defines
// ====================================================================================================================
#ifndef _xs_DEFAULT_CONVERSION
#define _xs_DEFAULT_CONVERSION      0
#endif //_xs_DEFAULT_CONVERSION


#if _xs_BigEndian_
  #define _xs_iexp_       0
  #define _xs_iman_       1
#else
  #define _xs_iexp_       1       //intel is little endian
  #define _xs_iman_       0
#endif //BigEndian_

#define _xs_doublecopysgn(a,b)      ((int32_t*)&a)[_xs_iexp_]&=~(((int32_t*)&b)[_xs_iexp_]&0x80000000) 
#define _xs_doubleisnegative(a)     ((((int32_t*)&a)[_xs_iexp_])|0x80000000) 

// ====================================================================================================================
//  Constants
// ====================================================================================================================
const double _xs_doublemagic            = double (6755399441055744.0);      //2^52 * 1.5,  uses limited precisicion to floor
const double _xs_doublemagicdelta       = (1.5e-8);                         //almost .5f = .5f + 1e^(number of exp bit)
const double _xs_doublemagicroundeps    = (.5f-_xs_doublemagicdelta);       //almost .5f = .5f - 1e^(number of exp bit)


// ====================================================================================================================
//  Prototypes
// ====================================================================================================================
finline int32_t xs_CRoundToInt      (double val, double dmr =  _xs_doublemagic) SC_FINLINE_EXT;
finline int32_t xs_ToInt            (double val, double dme = -_xs_doublemagicroundeps) SC_FINLINE_EXT;
finline int32_t xs_FloorToInt       (double val, double dme =  _xs_doublemagicroundeps) SC_FINLINE_EXT;
finline int32_t xs_CeilToInt        (double val, double dme =  _xs_doublemagicroundeps) SC_FINLINE_EXT;
finline int32_t xs_RoundToInt       (double val) SC_FINLINE_EXT;

//int32_t versions
finline int32_t xs_CRoundToInt      (int32_t val) SC_FINLINE_EXT;
finline int32_t xs_ToInt            (int32_t val) SC_FINLINE_EXT;

finline int32_t xs_CRoundToInt      (int32_t val) {return val;}
finline int32_t xs_ToInt            (int32_t val) {return val;}

// ====================================================================================================================
// ====================================================================================================================
//  Inline implementation
// ====================================================================================================================
// ====================================================================================================================
finline int32_t xs_CRoundToInt(double val, double dmr)
{
#if _xs_DEFAULT_CONVERSION==0
  val = val + dmr;
  return ((int32_t*)&val)[_xs_iman_];
#else
  return int32_t(floor(val+.5));
#endif
}


// ====================================================================================================================
finline int32_t xs_ToInt(double val, double dme)
{
#if _xs_DEFAULT_CONVERSION==0
  return (val<0) ? xs_CRoundToInt(val-dme) : xs_CRoundToInt(val+dme);
#else
  return int32_t(val);
#endif
}


// ====================================================================================================================
finline int32_t xs_FloorToInt(double val, double dme)
{
#if _xs_DEFAULT_CONVERSION==0
  return xs_CRoundToInt (val - dme);
#else
  return floor(val);
#endif
}


// ====================================================================================================================
finline int32_t xs_CeilToInt(double val, double dme)
{
#if _xs_DEFAULT_CONVERSION==0
  return xs_CRoundToInt (val + dme);
#else
  return ceil(val);
#endif
}


// ====================================================================================================================
finline int32_t xs_RoundToInt(double val)
{
#if _xs_DEFAULT_CONVERSION==0
  return xs_CRoundToInt (val + _xs_doublemagicdelta);
#else
  return floor(val+.5);
#endif
}

// ====================================================================================================================
// ====================================================================================================================
#endif // _xs_FLOAT_H_
