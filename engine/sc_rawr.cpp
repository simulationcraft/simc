// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace {  // ANONYMOUS NAMESPACE ==========================================

// translate_glyph_name =====================================================

static const char* translate_glyph_name( player_t* p,
                                         int       index )
{
  switch ( p -> type )
  {
  case DEATH_KNIGHT:
    switch ( index )
    {
    case  0: return "antimagic_shell";
    case  1: return "blood_strike";
    case  2: return "bone_shield";
    case  3: return "chains_of_ice";
    case  4: return "dancing_rune_weapon";
    case  5: return "dark_command";
    case  6: return "dark_death";
    case  7: return "death_and_decay";
    case  8: return "death_grip";
    case  9: return "death_strike";
    case 10: return "disease";
    case 11: return "frost_strike";
    case 12: return "heart_strike";
    case 13: return "howling_blast";
    case 14: return "hungering_cold";
    case 15: return "icebound_fortitude";
    case 16: return "icy_touch";
    case 17: return "obliterate";
    case 18: return "plague_strike";
    case 19: return "rune_strike";
    case 20: return "rune_tap";
    case 21: return "scourge_strike";
    case 22: return "strangulate";
    case 23: return "the_ghoul";
    case 24: return "unbreakable_armor";
    case 25: return "unholy_blight";
    case 26: return "vampiric_blood";
    case 27: return "blood_tap";
    case 28: return "corpse_explosion";
    case 29: return "deaths_embrace";
    case 30: return "horn_of_winter";
    case 31: return "pestilence";
    case 32: return "raise_dead";
    default: return 0;
    }
  case DRUID:
    switch ( index )
    {
    case  0: return "mangle";
    case  1: return "shred";
    case  2: return "rip";
    case  3: return "berserk";
    case  4: return "savage_roar";
    case  5: return "growl";
    case  6: return "maul";
    case  7: return "frenzied_regeneration";
    case  8: return "focus";
    case  9: return "insect_swarm";
    case 10: return "monsoon";
    case 11: return "moonfire";
    case 12: return "starfall";
    case 13: return "starfire";
    case 14: return "wrath";
    case 15: return "healing_touch";
    case 16: return "innervate";
    case 17: return "lifebloom";
    case 18: return "nourish";
    case 19: return "rebirth";
    case 20: return "regrowth";
    case 21: return "rejuvenation";
    case 22: return "swiftmend";
    case 23: return "wild_growth";
    case 24: return "rapid_rejuvenation";
    default: return 0;
    }
  case HUNTER:
    switch ( index )
    {
    case  0: return "aimed_shot";
    case  1: return "arcane_shot";
    case  2: return "aspect_of_the_viper";
    case  3: return "bestial_wrath";
    case  4: return "chimera_shot";
    case  5: return "deterrence";
    case  6: return "disengage";
    case  7: return "explosive_shot";
    case  8: return "explosive_trap";
    case  9: return "freezing_trap";
    case 10: return "frost_trap";
    case 11: return "hunters_mark";
    case 12: return "immolation_trap";
    case 13: return "kill_shot";
    case 14: return "mending";
    case 15: return "multishot";
    case 16: return "rapid_fire";
    case 17: return "raptor_strike";
    case 18: return "scatter_shot";
    case 19: return "serpent_sting";
    case 20: return "snake_trap";
    case 21: return "steady_shot";
    case 22: return "the_beast";
    case 23: return "the_hawk";
    case 24: return "trueshot_aura";
    case 25: return "volley";
    case 26: return "wyvern_sting";
    case 27: return "feign_death";
    case 28: return "mend_pet";
    case 29: return "possessed_strength";
    case 30: return "revive_pet";
    case 31: return "scare_beast";
    case 32: return "the_pack";
    default: return 0;
    }
  case MAGE:
    switch ( index )
    {
    case  0: return "fire_ball";
    case  1: return "frostfire";
    case  2: return "frost_bolt";
    case  3: return "ice_armor";
    case  4: return "improved_scorch";
    case  5: return "mage_armor";
    case  6: return "mana_gem";
    case  7: return "molten_armor";
    case  8: return "water_elemental";
    case  9: return "arcane_explosion";
    case 10: return "arcane_power";
    case 11: return "arcane_blast";
    case 12: return "arcane_missiles";
    case 13: return "arcane_barrage";
    case 14: return "living_bomb";
    case 15: return "ice_lance";
    case 16: return "mirror_image";
    case 17: return "deep_freeze";
    case 18: return "eternal_water";
    default: return 0;
    }
  case PALADIN:
    switch ( index )
    {
    case  0: return "avenging_wrath";
    case  1: return "avengers_shield";
    case  2: return "seal_of_blood";
    case  3: return "seal_of_righteousness";
    case  4: return "seal_of_vengeance";
    case  5: return "hammer_of_wrath";
    case  6: return "beacon_of_light";
    case  7: return "divine_plea";
    case  8: return "divine_storm";
    case  9: return "hammer_of_the_righteous";
    case 10: return "holy_shock";
    case 11: return "salvation";
    case 12: return "shield_of_righteousness";
    case 13: return "seal_of_wisdom";
    case 14: return "cleansing";
    case 15: return "seal_of_light";
    case 16: return "the_wise";
    case 17: return "turn_evil";
    case 18: return "consecration";
    case 19: return "crusader_strike";
    case 20: return "exorcism";
    case 21: return "flash_of_light";
    case 22: return "seal_of_command";
    case 23: return "blessing_of_kings";
    case 24: return "sense_undead";
    case 25: return "righteous_defense";
    case 26: return "spiritual_attunement";
    case 27: return "divinity";
    case 28: return "blessing_of_wisdom";
    case 29: return "hammer_of_justice";
    case 30: return "lay_on_hands";
    case 31: return "judgement";
    case 32: return "holy_light";
    case 33: return "blessing_of_might";
    default: return 0;
    }
  case PRIEST:
    switch ( index )
    {
    case  0: return "circle_of_healing";
    case  1: return "dispel_magic";
    case  2: return "dispersion";
    case  3: return "fade";
    case  4: return "fear_ward";
    case  5: return "flash_heal";
    case  6: return "guardian_spirit";
    case  7: return "holy_nova";
    case  8: return "hymn_of_hope";
    case  9: return "inner_fire";
    case 10: return "lightwell";
    case 11: return "mass_dispel";
    case 12: return "mind_control";
    case 13: return "mind_flay";
    case 14: return "mind_sear";
    case 15: return "pain_suppression";
    case 16: return "penance";
    case 17: return "power_word_shield";
    case 18: return "prayer_of_healing";
    case 19: return "psychic_scream";
    case 20: return "renew";
    case 21: return "scourge_imprisonment";
    case 22: return "shadow";
    case 23: return "shadow_word_death";
    case 24: return "shadow_word_pain";
    case 25: return "smite";
    case 26: return "spirit_of_redemption";
    case 27: return "fading";
    case 28: return "fortitude";
    case 29: return "levitate";
    case 30: return "shackle_undead";
    case 31: return "shadow_protection";
    case 32: return "shadowfiend";
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
    case  6: return "fan_of_knives";
    case  7: return "expose_armor";
    case  8: return "sinister_strike";
    case  9: return "slice_and_dice";
    case 10: return "feint";
    case 11: return "ghostly_strike";
    case 12: return "rupture";
    case 13: return "blade_flurry";
    case 14: return "adrenaline_rush";
    case 15: return "evasion";
    case 16: return "garrote";
    case 17: return "gouge";
    case 18: return "sap";
    case 19: return "sprint";
    case 20: return "ambush";
    case 21: return "crippling_poison";
    case 22: return "hemorrhage";
    case 23: return "preparation";
    case 24: return "shadow_dance";
    case 25: return "deadly_throw";
    case 26: return "cloak_of_shadows";
    case 27: return "tricks_of_the_trade";
    case 28: return "blurred_speed";
    case 29: return "pick_pocket";
    case 30: return "pick_lock";
    case 31: return "distract";
    case 32: return "vanish";
    case 33: return "safe_fall";
    default: return 0;
    }
  case SHAMAN:
    switch ( index )
    {
    case  0: return "healing_wave";
    case  1: return "lightning_bolt";
    case  2: return "shocking";
    case  3: return "lightning_shield";
    case  4: return "flame_shock";
    case  5: return "flametongue_weapon";
    case  6: return "lava_lash";
    case  7: return "fire_nova";
    case  8: return "frost_shock";
    case  9: return "healing_stream_totem";
    case 10: return "lesser_healing_wave";
    case 11: return "water_mastery";
    case 12: return "earthliving_weapon";
    case 13: return "windfury_weapon";
    case 14: return "chain_lightning";
    case 15: return "chain_heal";
    case 16: return "earth_shield";
    case 17: return "feral_spirit";
    case 18: return "hex";
    case 19: return "mana_tide";
    case 20: return "riptide";
    case 21: return "stoneclaw_totem";
    case 22: return "stormstrike";
    case 23: return "thunder";
    case 24: return "totem_of_wrath";
    case 25: return "elemental_mastery";
    case 26: return "lava";
    case 27: return "fire_elemental_totem";
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
    case  4: return "death_coil";
    case  5: return "felguard";
    case  6: return "haunt";
    case  7: return "immolate";
    case  8: return "imp";
    case  9: return "incinerate";
    case 10: return "life_tap";
    case 11: return "metamorphosis";
    case 12: return "quick_decay";
    case 13: return "searing_pain";
    case 14: return "shadow_bolt";
    case 15: return "shadow_burn";
    case 16: return "unstable_affliction";
    default: return 0;
    }
  case WARRIOR:
    switch ( index )
    {
    case  0: return "barbaric_insults";
    case  1: return "bladestorm";
    case  2: return "blocking";
    case  3: return "bloodthirst";
    case  4: return "cleaving";
    case  5: return "devastate";
    case  6: return "enraged_regeneration";
    case  7: return "execution";
    case  8: return "hamstring";
    case  9: return "heroic_strike";
    case 10: return "intervene";
    case 11: return "last_stand";
    case 12: return "mortal_strike";
    case 13: return "overpower";
    case 14: return "rapid_charge";
    case 15: return "rending";
    case 16: return "resonating_power";
    case 17: return "revenge";
    case 18: return "shield_wall";
    case 19: return "shockwave";
    case 20: return "spell_reflection";
    case 21: return "sunder_armor";
    case 22: return "sweeping_strikes";
    case 23: return "taunt";
    case 24: return "victory_rush";
    case 25: return "vigilance";
    case 26: return "whirlwind";
    case 27: return "battle";
    case 28: return "bloodrage";
    case 29: return "charge";
    case 30: return "enduring_victory";
    case 31: return "mocking_blow";
    case 32: return "thunder_clap";
    case 33: return "command";
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
    sim -> errorf( "Unable to open Rawr Character Save file '%s'\n", character_file.c_str() );
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
  xml_node_t* root_node = xml_t::create( sim, character_xml );
  if ( ! root_node )
  {
    sim -> errorf( "Unable to parse Rawr Character Save XML.\n" );
    return 0;
  }

  if ( sim -> debug ) xml_t::print( root_node );

  std::string class_str, race_str;
  if ( ! xml_t::get_value( class_str, root_node, "Class/." ) ||
       ! xml_t::get_value(  race_str, root_node, "Race/."  ) )
  {
    sim -> errorf( "Unable to determine character class and race in Rawr Character Save XML.\n" );
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

  sim -> current_slot = 0;
  sim -> current_name = name_str;

  std::string talents_parm = class_str + "Talents/.";

  armory_t::format(  name_str );
  armory_t::format( class_str );

  if ( class_str == "deathknight" ) class_str = "death_knight";

  int race_type = translate_rawr_race_str( race_str );

  player_t* p = player_t::create( sim, class_str, name_str, race_type );
  sim -> active_player = p;
  if ( ! p )
  {
    sim -> errorf( "Unable to build player with class '%s' and name '%s'.\n", class_str.c_str(), name_str.c_str() );
    return 0;
  }

  p -> origin_str = character_file;

  xml_t::get_value( p -> region_str, root_node, "Region/." );
  xml_t::get_value( p -> server_str, root_node, "Realm/."  );

  std::string talents_str;
  if ( ! xml_t::get_value( talents_str, root_node, talents_parm ) )
  {
    sim -> errorf( "Player %s unable to determine character talents in Rawr Character Save XML.\n", p -> name() );
    return 0;
  }

  std::string talents_encoding, glyphs_encoding;
  if ( 2 != util_t::string_split( talents_str, ".", "S S", &talents_encoding, &glyphs_encoding ) )
  {
    sim -> errorf( "Player %s expected 'talents.glyphs' in Rawr Character Save XML, but found: %s\n", p -> name(), talents_str.c_str() );
    return 0;
  }

  if ( ! p -> parse_talents_armory( talents_encoding ) )
  {
    sim -> errorf( "Player %s unable to parse talent encoding '%s'.\n", p -> name(), talents_encoding.c_str() );
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
        sim -> errorf( "Player %s unable to parse glyph encoding '%s'.\n", p -> name(), glyphs_encoding.c_str() );
        return 0;
      }
      if ( p -> glyphs_str.size() ) p -> glyphs_str += "/";
      p -> glyphs_str += glyph_name;
    }
  }

  for ( int i=0; i < SLOT_MAX; i++ )
  {
    sim -> current_slot = i;
    if( sim -> canceled ) return 0;

    const char* slot_name = translate_inventory_id( i );
    if ( ! slot_name ) continue;

    item_t& item = p -> items[ i ];

    std::string slot_encoding;
    if ( xml_t::get_value( slot_encoding, root_node, slot_name ) )
    {
      std::string item_id, gem_ids[ 3 ], enchant_id;

      if ( 5 != util_t::string_split( slot_encoding, ".", "S S S S S", &item_id, &( gem_ids[ 0 ] ), &( gem_ids[ 1 ] ), &( gem_ids[ 2 ] ), &enchant_id ) )
      {
        sim -> errorf( "Player %s unable to parse slot encoding '%s'.\n", p -> name(), slot_encoding.c_str() );
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
