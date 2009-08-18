// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace   // ANONYMOUS NAMESPACE ==========================================
{

// translate_glyph_name =====================================================

static const char* translate_glyph_name( player_t* p,
                                         int       index )
{
  switch ( p -> type )
  {
  case DRUID:
    switch ( index )
    {
    case  0: return "mangle";
    case  1: return "shred";
    case  2: return "rip";
    case  3: return "berserk";
    case  4: return "savage_roar";
    case  8: return "focus";
    case  9: return "insect_swarm";
    case 11: return "moonfire";
    case 12: return "starfall";
    case 13: return "starfire";
    case 16: return "innervate";
    default: return 0;
    }
  case HUNTER:
    switch ( index )
    {
    case  0: return "aimed_shot";
    case  2: return "aspect_of_the_viper";
    case  3: return "bestial_wrath";
    case  4: return "chimera_shot";
    case  7: return "explosive_shot";
    case 12: return "hunters_mark";
    case 14: return "kill_shot";
    case 19: return "rapid_fire";
    case 24: return "serpent_sting";
    case 26: return "steady_shot";
    case 28: return "the_hawk";
    case 30: return "trueshot_aura";
    default: return 0;
    }
  case MAGE:
    switch ( index )
    {
    case  0: return "fire_ball";
    case  1: return "frostfire";
    case  2: return "frost_bolt";
    case  4: return "improved_scorch";
    case  5: return "mage_armor";
    case  6: return "mana_gem";
    case  7: return "molten_armor";
    case  8: return "water_elemental";
    case 10: return "arcane_power";
    case 11: return "arcane_blast";
    case 12: return "arcane_missiles";
    case 13: return "arcane_barrage";
    case 14: return "living_bomb";
    case 15: return "ice_lance";
    default: return 0;
    }
  case PRIEST:
    switch ( index )
    {
    case  2: return "dispersion";
    case 16: return "penance";
    case 22: return "shadow";
    case 23: return "shadow_word_death";
    case 24: return "shadow_word_pain";
    default: return 0;
    }
  case ROGUE:
    switch ( index )
    {
    case  0: return "backstab";
    case  1: return "eviscerate";
    case  2: return "mutilate";
    case  3: return "hunger_for_blood";
    case  4: return "killing_spree";
    case  5: return "vigor";
    case  7: return "expose_armor";
    case  8: return "sinister_strike";
    case  9: return "slice_and_dice";
    case 10: return "feint";
    case 11: return "ghostly_strike";
    case 12: return "rupture";
    case 13: return "blade_flurry";
    case 14: return "adrenaline_rush";
      // NYI by Rawr:
      // case 0: return "shadow_dance";
      // case 0: return "tricks_of_the_trade";
      // case 0: return "preparation";
      // case 0: return "hemorrhage";
    default: return 0;
    }
  case SHAMAN:
    switch ( index )
    {
    case  1: return "lightning_bolt";
    case  2: return "shocking";
    case  3: return "lightning_shield";
    case  4: return "flame_shock";
    case  5: return "flametongue_weapon";
    case  6: return "lava_lash";
    case 13: return "windfury_weapon";
    case 14: return "chain_lightning";
    case 17: return "feral_spirit";
    case 19: return "mana_tide";
    case 22: return "stormstrike";
    case 24: return "totem_of_wrath";
    case 25: return "elemental_mastery";
    case 26: return "lava";
    case 28: return "thunderstorm";
    default: return 0;
    }
  case WARLOCK:
    switch ( index )
    {
    case  0: return "chaos_bolt";
    case  1: return "conflagrate";
    case  2: return "corruption";
    case  3: return "curse_of_agony";
    case  4: return "felguard";
    case  5: return "haunt";
    case  6: return "immolate";
    case  7: return "imp";
    case  8: return "incinerate";
    case  9: return "life_tap";
    case 10: return "metamorphosis";
    case 11: return "searing_pain";
    case 12: return "shadow_bolt";
    case 13: return "shadow_burn";
    case 14: return "unstable_affliction";
    default: return 0;
    }
  case WARRIOR:
    switch ( index )
    {
    case  1: return "bladestorm";
    case  2: return "blocking";
    case  7: return "execution";
    case  9: return "heroic_strike";
    case 12: return "mortal_strike";
    case 13: return "overpower";
    case 15: return "rending";
    case 26: return "whirlwind";
    default: return 0;
    }
  }

  return 0;
}

// translate_inventory_id ===================================================

static const char* translate_inventory_id( int slot )
{
  switch ( slot )
  {
  case SLOT_HEAD:      return "Head/.";
  case SLOT_NECK:      return "Neck/.";
  case SLOT_SHOULDERS: return "Shoulders/.";
  case SLOT_CHEST:     return "Chest/.";
  case SLOT_WAIST:     return "Waist/.";
  case SLOT_LEGS:      return "Legs/.";
  case SLOT_FEET:      return "Feet/.";
  case SLOT_WRISTS:    return "Wrist/.";
  case SLOT_HANDS:     return "Hands/.";
  case SLOT_FINGER_1:  return "Finger1/.";
  case SLOT_FINGER_2:  return "Finger2/.";
  case SLOT_TRINKET_1: return "Trinket1/.";
  case SLOT_TRINKET_2: return "Trinket2/.";
  case SLOT_BACK:      return "Back/.";
  case SLOT_MAIN_HAND: return "MainHand/.";
  case SLOT_OFF_HAND:  return "OffHand/.";
  case SLOT_RANGED:    return "Ranged/.";
  }

  return "unknown";
}

static int translate_rawr_race_str( const std::string& name )
{
  if ( ! name.compare( "Human"    ) ) return RACE_HUMAN;
  if ( ! name.compare( "Orc"      ) ) return RACE_ORC;
  if ( ! name.compare( "Dwarf"    ) ) return RACE_DWARF;
  if ( ! name.compare( "NightElf" ) ) return RACE_NIGHT_ELF;
  if ( ! name.compare( "Undead"   ) ) return RACE_UNDEAD;
  if ( ! name.compare( "Tauren"   ) ) return RACE_TAUREN;
  if ( ! name.compare( "Gnome"    ) ) return RACE_GNOME;
  if ( ! name.compare( "Troll"    ) ) return RACE_TROLL;
  if ( ! name.compare( "BloodElf" ) ) return RACE_BLOOD_ELF;
  if ( ! name.compare( "Draenei"  ) ) return RACE_DRAENEI;

  return RACE_NONE;
}

} // ANONYMOUS NAMESPACE ====================================================

// rawr_t::load_player ======================================================

player_t* rawr_t::load_player( sim_t* sim,
                               const std::string& character_file )
{
  FILE* f = fopen( character_file.c_str(), "r" );
  if ( ! f )
  {
    util_t::printf( "\nsimcraft: Unable to open Rawr Character Save file '%s'\n", character_file.c_str() );
    return false;
  }

  std::string buffer;
  char c;
  while ( ( c = fgetc( f ) ) != EOF ) buffer += c;
  fclose( f );

  player_t* p = load_player( sim, character_file, buffer );

  return p;
}

// rawr_t::load_player ======================================================

player_t* rawr_t::load_player( sim_t* sim,
                               const std::string& character_file,
                               const std::string& character_xml )
{
  xml_node_t* root_node = xml_t::create( character_xml );
  if ( ! root_node )
  {
    util_t::printf( "\nsimcraft: Unable to parse Rawr Character Save XML.\n" );
    return 0;
  }

  if ( sim -> debug ) xml_t::print( root_node );

  std::string class_str, race_str;
  if ( ! xml_t::get_value( class_str, root_node, "Class/." ) ||
       ! xml_t::get_value(  race_str, root_node, "Race/."  ) )
  {
    util_t::printf( "\nsimcraft: Unable to determine character class and race in Rawr Character Save XML.\n" );
    return 0;
  }

  std::string name_str;
  if ( ! xml_t::get_value(  name_str, root_node, "Name/."  ) )
  {
    std::vector<std::string> tokens;
    int num_tokens = util_t::string_split( tokens, character_file, "\\/" );
    assert( num_tokens > 0 );
    name_str = tokens[ num_tokens-1 ];
  }

  std::string talents_parm = class_str + "Talents/.";

  armory_t::format(  name_str );
  armory_t::format( class_str );

  if ( class_str == "paladin" || class_str == "death_knight" )
  {
    util_t::printf( "\nsimcraft: The Death Knight and Paladin modules are still in development, so Rawr import is disabled.\n" );
    return 0;
  }

  int race_type = translate_rawr_race_str( race_str );

  player_t* p = player_t::create( sim, class_str, name_str, race_type );
  if ( ! p )
  {
    util_t::printf( "\nsimcraft: Unable to build player with class '%s' and name '%s'.\n", class_str.c_str(), name_str.c_str() );
    return 0;
  }

  p -> origin_str = character_file;

  xml_t::get_value( p -> region_str, root_node, "Region/." );
  xml_t::get_value( p -> server_str, root_node, "Realm/."  );

  std::string talents_str;
  if ( ! xml_t::get_value( talents_str, root_node, talents_parm ) )
  {
    util_t::printf( "\nsimcraft: Player %s unable to determine character talents in Rawr Character Save XML.\n", p -> name() );
    return 0;
  }

  std::string talents_encoding, glyphs_encoding;
  if ( 2 != util_t::string_split( talents_str, ".", "S S", &talents_encoding, &glyphs_encoding ) )
  {
    util_t::printf( "\nsimcraft: Player %s expected 'talents.glyphs' in Rawr Character Save XML, but found: %s\n", p -> name(), talents_str.c_str() );
    return 0;
  }

  if ( ! p -> parse_talents_armory( talents_encoding ) )
  {
    util_t::printf( "\nsimcraft: Player %s unable to parse talent encoding '%s'.\n", p -> name(), talents_encoding.c_str() );
    return 0;
  }

  p -> talents_str = "http://www.wowarmory.com/talent-calc.xml?cid=";
  p -> talents_str += util_t::class_id_string( p -> type );
  p -> talents_str += "&tal=" + talents_encoding;

  p -> glyphs_str = "";
  for ( int i=0; glyphs_encoding[ i ]; i++ )
  {
    if ( glyphs_encoding[ i ] == '1' )
    {
      const char* glyph_name = translate_glyph_name( p, i );
      if ( ! glyph_name )
      {
        util_t::printf( "\nsimcraft: Player %s unable to parse glyph encoding '%s'.\n", p -> name(), glyphs_encoding.c_str() );
        return 0;
      }
      if ( p -> glyphs_str.size() ) p -> glyphs_str += "/";
      p -> glyphs_str += glyph_name;
    }
  }

  for ( int i=0; i < SLOT_MAX; i++ )
  {
    const char* slot_name = translate_inventory_id( i );
    if ( ! slot_name ) continue;

    item_t& item = p -> items[ i ];

    std::string slot_encoding;
    if ( xml_t::get_value( slot_encoding, root_node, slot_name ) )
    {
      std::string item_id, gem_ids[ 3 ], enchant_id;

      if ( 5 != util_t::string_split( slot_encoding, ".", "S S S S S", &item_id, &( gem_ids[ 0 ] ), &( gem_ids[ 1 ] ), &( gem_ids[ 2 ] ), &enchant_id ) )
      {
        util_t::printf( "\nsimcraft: Player %s unable to parse slot encoding '%s'.\n", p -> name(), slot_encoding.c_str() );
        return 0;
      }

      bool success = item_t::download_slot( item, item_id, enchant_id, gem_ids );

      if ( ! success )
      {
        return 0;
      }
    }
  }

  return p;
}
