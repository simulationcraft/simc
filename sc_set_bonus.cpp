// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// set_bonus_t::set_bonus_t =================================================

set_bonus_t::set_bonus_t()
{
  memset( (void*) this, 0x00, sizeof( set_bonus_t ) );
  
  count[ SET_T4_2PC  ] = count[ SET_T4_4PC  ] = -1;
  count[ SET_T5_2PC  ] = count[ SET_T5_4PC  ] = -1;
  count[ SET_T6_2PC  ] = count[ SET_T6_4PC  ] = -1;
  count[ SET_T7_2PC  ] = count[ SET_T7_4PC  ] = -1;
  count[ SET_T8_2PC  ] = count[ SET_T8_4PC  ] = -1;
  count[ SET_T9_2PC  ] = count[ SET_T9_4PC  ] = -1;
  count[ SET_T10_2PC ] = count[ SET_T10_4PC ] = -1;
}

// set_bonus_t::tier4 =======================================================

int set_bonus_t::tier4_2pc() SC_CONST { return ( count[ SET_T4_2PC ] > 0 || count[ SET_T4_2PC ] < 0 && count[ SET_T4 ] >= 2 ) ? 1 : 0; }

int set_bonus_t::tier4_4pc() SC_CONST { return ( count[ SET_T4_4PC ] > 0 || count[ SET_T4_4PC ] < 0 && count[ SET_T4 ] >= 4 ) ? 1 : 0; }

// set_bonus_t::tier5 =======================================================

int set_bonus_t::tier5_2pc() SC_CONST { return ( count[ SET_T5_2PC ] > 0 || count[ SET_T5_2PC ] < 0 && count[ SET_T5 ] >= 2 ) ? 1 : 0; }

int set_bonus_t::tier5_4pc() SC_CONST { return ( count[ SET_T5_4PC ] > 0 || count[ SET_T5_4PC ] < 0 && count[ SET_T5 ] >= 4 ) ? 1 : 0; }

// set_bonus_t::tier6 =======================================================

int set_bonus_t::tier6_2pc() SC_CONST { return ( count[ SET_T6_2PC ] > 0 || count[ SET_T6_2PC ] < 0 && count[ SET_T6 ] >= 2 ) ? 1 : 0; }

int set_bonus_t::tier6_4pc() SC_CONST { return ( count[ SET_T6_4PC ] > 0 || count[ SET_T6_4PC ] < 0 && count[ SET_T6 ] >= 4 ) ? 1 : 0; }

// set_bonus_t::tier7 =======================================================

int set_bonus_t::tier7_2pc() SC_CONST { return ( count[ SET_T7_2PC ] > 0 || count[ SET_T7_2PC ] < 0 && count[ SET_T7 ] >= 2 ) ? 1 : 0; }

int set_bonus_t::tier7_4pc() SC_CONST { return ( count[ SET_T7_4PC ] > 0 || count[ SET_T7_4PC ] < 0 && count[ SET_T7 ] >= 4 ) ? 1 : 0; }

// set_bonus_t::tier8 =======================================================

int set_bonus_t::tier8_2pc() SC_CONST { return ( count[ SET_T8_2PC ] > 0 || count[ SET_T8_2PC ] < 0 && count[ SET_T8 ] >= 2 ) ? 1 : 0; }

int set_bonus_t::tier8_4pc() SC_CONST { return ( count[ SET_T8_4PC ] > 0 || count[ SET_T8_4PC ] < 0 && count[ SET_T8 ] >= 4 ) ? 1 : 0; }

// set_bonus_t::tier9 =======================================================

int set_bonus_t::tier9_2pc() SC_CONST { return ( count[ SET_T9_2PC ] > 0 || count[ SET_T9_2PC ] < 0 && count[ SET_T9 ] >= 2 ) ? 1 : 0; }

int set_bonus_t::tier9_4pc() SC_CONST { return ( count[ SET_T9_4PC ] > 0 || count[ SET_T9_4PC ] < 0 && count[ SET_T9 ] >= 4 ) ? 1 : 0; }

// set_bonus_t::tier10 =======================================================

int set_bonus_t::tier10_2pc() SC_CONST { return ( count[ SET_T10_2PC ] > 0 || count[ SET_T10_2PC ] < 0 && count[ SET_T10 ] >= 2 ) ? 1 : 0; }

int set_bonus_t::tier10_4pc() SC_CONST { return ( count[ SET_T10_4PC ] > 0 || count[ SET_T10_4PC ] < 0 && count[ SET_T10 ] >= 4 ) ? 1 : 0; }

// set_bonus_t::spellstrike =================================================

int set_bonus_t::spellstrike() SC_CONST { return ( count[ SET_SPELLSTRIKE ] >= 2 ) ? 1 : 0; }

// set_bonus_t::decode ======================================================

int set_bonus_t::decode( const std::string& name ) SC_CONST
{
  if( name.empty() ) return SET_NONE;

  // Death Knight
  if( name.find( "scourgeborne" ) != std::string::npos ) return SET_T7;
  if( name.find( "darkruned"    ) != std::string::npos ) return SET_T8;
  if( name.find( "koltira"      ) != std::string::npos ) return SET_T9;
  if( name.find( "thassarian"   ) != std::string::npos ) return SET_T9;

  // Druid
  if( name.find( "dreamwalker" ) != std::string::npos ) return SET_T7;
  if( name.find( "nightsong"   ) != std::string::npos ) return SET_T8;
  if( name.find( "stormrage"   ) != std::string::npos ) return SET_T9;
  if( name.find( "runetotem"   ) != std::string::npos ) return SET_T9;

  // Hunter
  if( name.find( "cryptstalker"   ) != std::string::npos ) return SET_T7;
  if( name.find( "scourgestalker" ) != std::string::npos ) return SET_T8;
  if( name.find( "windrunner"     ) != std::string::npos ) return SET_T9;
  // Horde T9 name?
  // if( name.find( ""   ) != std::string::npos ) return SET_T9;

  // Mage
  if( name.find( "frostfire"   ) != std::string::npos ) return SET_T7;
  if( name.find( "kirin_tor"   ) != std::string::npos ) return SET_T8;
  if( name.find( "sundstrider" ) != std::string::npos ) return SET_T9;
  if( name.find( "khadgar"     ) != std::string::npos ) return SET_T9;

  // Paladin
  if( name.find( "redemption" ) != std::string::npos ) return SET_T7;
  if( name.find( "aegis"      ) != std::string::npos ) return SET_T8;
  if( name.find( "turalyon"   ) != std::string::npos ) return SET_T9;
  if( name.find( "liadrin"    ) != std::string::npos ) return SET_T9;

  // Priest
  if( name.find( "faith"          ) != std::string::npos ) return SET_T7;
  if( name.find( "sanctification" ) != std::string::npos ) return SET_T8;
  if( name.find( "zabra"          ) != std::string::npos ) return SET_T9;
  if( name.find( "velen"          ) != std::string::npos ) return SET_T9;

  // Rogue
  if( name.find( "bonescythe"  ) != std::string::npos ) return SET_T7;
  if( name.find( "terrorblade" ) != std::string::npos ) return SET_T8;
  if( name.find( "vancleef"    ) != std::string::npos ) return SET_T9;
  if( name.find( "garona"      ) != std::string::npos ) return SET_T9;

  // Shaman
  if( name.find( "earthshatter" ) != std::string::npos ) return SET_T7;
  if( name.find( "worldbreaker" ) != std::string::npos ) return SET_T8;
  if( name.find( "nobundo"      ) != std::string::npos ) return SET_T9;
  if( name.find( "thrall"       ) != std::string::npos ) return SET_T9;

  // Warlock
  if( name.find( "plagueheart"  ) != std::string::npos ) return SET_T7;
  if( name.find( "deathbringer" ) != std::string::npos ) return SET_T8;
  if( name.find( "kelthuzad"    ) != std::string::npos ) return SET_T9;
  if( name.find( "guldan"       ) != std::string::npos ) return SET_T9;

  // Warrior
  if( name.find( "dreadnaught"  ) != std::string::npos ) return SET_T7;
  if( name.find( "siegebreaker" ) != std::string::npos ) return SET_T8;
  if( name.find( "wrynn"        ) != std::string::npos ) return SET_T9;
  if( name.find( "hellscream"   ) != std::string::npos ) return SET_T9;

  return SET_NONE;
}

// set_bonus_t::init ========================================================

bool set_bonus_t::init( player_t* p )
{
  int num_items = p -> items.size();

  for( int i=0; i < num_items; i++ )
  {
    count[ decode( p -> items[ i ].encoded_name_str ) ] += 1;
  }

  return true;
}

