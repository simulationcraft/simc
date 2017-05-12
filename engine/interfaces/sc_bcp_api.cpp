// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "util/rapidjson/document.h"
#include "util/rapidjson/stringbuffer.h"
#include "util/rapidjson/prettywriter.h"

// ==========================================================================
// Blizzard Community Platform API
// ==========================================================================

namespace { // UNNAMED NAMESPACE

struct player_spec_t
{
  std::string region, server, name, url, cleanurl, local_json, origin, talent_spec;
};

// download

bool download( sim_t*               sim,
               rapidjson::Document& d,
               std::string&         result,
               const std::string&   url,
               const std::string&   cleanurl,
               cache::behavior_e    caching )
{
  int attempt = 0;
  do
  {
    if ( http::get( result, url, cleanurl, caching ) )
    {
      d.Parse< 0 >( result.c_str() );

      if ( ! d.HasParseError() && d.HasMember( "status" ) )
      {
        std::string status = d[ "status" ].GetString();
        // Would be nicer to use status codes, but this will do for now ...
        if ( status == "nok" && util::str_in_str_ci( d[ "reason" ].GetString(), "not found" ) )
        {
          break;
        }
      }
      else
      {
        break;
      }
    }
  } while ( ++attempt < sim -> armory_retries );

  return attempt < sim -> armory_retries;
}

// download_id ==============================================================

bool download_id( sim_t* sim,
                  rapidjson::Document& d,
                  const std::string& region,
                  unsigned item_id,
                  std::string apikey,
                  cache::behavior_e caching )
{
  if ( item_id == 0 )
    return false;

  std::string url;
  std::string cleanurl;

  if ( apikey.size() == 32 && region != "cn" ) //China does not have new api endpoints yet.
  {
    cleanurl = "https://" + region + ".api.battle.net/wow/item/" + util::to_string( item_id ) + "?locale=en_us&apikey=";
    url = cleanurl + apikey;
  }
  else
  {
    url = "http://" + region + ".battle.net/api/wow/item/" + util::to_string( item_id ) + "?locale=en_US";
    cleanurl = url;
  }

  std::string result;
  if ( ! download( sim, d, result, url, cleanurl, caching ) )
    return false;

  return true;
}

// parse_profession =========================================================

void parse_profession( std::string&               professions_str,
                       const rapidjson::Value&    profile,
                       int                        index )
{
  if ( ! profile.HasMember( "primary" ) )
    return;

  const rapidjson::Value& professions = profile[ "primary" ];

  if ( professions.Size() >= rapidjson::SizeType( index + 1 ) )
  {
    const rapidjson::Value& profession = professions[ index ];
    if ( ! profession.HasMember( "id" ) || ! profession.HasMember( "rank" ) )
      return;

    if ( professions_str.length() > 0 )
      professions_str += '/';

    professions_str += util::profession_type_string( util::translate_profession_id( profession[ "id" ].GetUint() ) );
    professions_str += "=";
    professions_str += util::to_string( profession[ "rank" ].GetUint() );
  }
}

// parse_talents ============================================================

const rapidjson::Value* choose_talent_spec( const rapidjson::Value& talents,
                                                   const std::string& )
{
  for ( size_t i = 0; i < talents.Size(); ++i )
  {
    if ( talents[ i ].HasMember( "selected" ) )
    {
      return &talents[ i ];
    }
  }

  return nullptr;
}

bool parse_talents( player_t*  p,
                    const rapidjson::Value& talents,
                    const std::string& specifier )
{
  const rapidjson::Value* spec = choose_talent_spec( talents, specifier );
  if ( ! spec )
    return false;

  if ( ! spec -> HasMember( "calcSpec" ) || ! spec -> HasMember( "calcTalent" ) )
    return false;

  const char* buffer = (*spec)[ "calcSpec" ].GetString();
  unsigned sid;
  switch ( buffer[ 0 ] )
  {
    case 'a': sid = 0; break;
    case 'Z': sid = 1; break;
    case 'b': sid = 2; break;
    case 'Y': sid = 3; break;
    default:  sid = 99; break;
  }
  p -> _spec = p -> dbc.spec_by_idx( p -> type, sid );

  std::string talent_encoding = (*spec)[ "calcTalent" ].GetString();

  for ( size_t i = 0; i < talent_encoding.size(); ++i )
  {
    switch ( talent_encoding[ i ] )
    {
      case '.':
        talent_encoding[ i ] = '0';
        break;
      case '0':
      case '1':
      case '2':
        talent_encoding[ i ] += 1;
        break;
      default:
        p -> sim -> errorf( "BCP API: Invalid character '%c' in talent encoding for player %s.\n", talent_encoding[ i ], p -> name() );
        return false;
    }
  }

  if ( ! p -> parse_talents_numbers( talent_encoding ) )
  {
    p -> sim -> errorf( "BCP API: Can't parse talent encoding '%s' for player %s.\n", talent_encoding.c_str(), p -> name() );
    return false;
  }

  p -> create_talents_armory();

  return true;
}

// parse_items ==============================================================

void parse_artifact( item_t& item, const rapidjson::Value& artifact )
{
  if ( ! artifact.HasMember( "artifactId" ) || ! artifact.HasMember( "artifactTraits" ) )
  {
    return;
  }

  auto artifact_id = artifact[ "artifactId" ].GetUint();
  if ( artifact_id == 0 )
  {
    return;
  }

  auto powers = item.player -> dbc.artifact_powers( artifact_id );
  for ( auto power_idx = 0U, end = artifact[ "artifactTraits" ].Size(); power_idx < end; ++power_idx )
  {
    const auto& trait = artifact[ "artifactTraits" ][ power_idx ];
    if ( ! trait.HasMember( "id" ) || ! trait.HasMember( "rank" ) )
    {
      continue;
    }

    auto trait_id = trait[ "id" ].GetUint();
    auto rank = trait[ "rank" ].GetUint();

    auto it = range::find_if( powers, [ trait_id ]( const artifact_power_data_t* p ) {
      return p -> id == trait_id;
    } );

    if ( it == powers.end() )
    {
      continue;
    }

    // Internal trait index (we order by id), might differ from blizzard's ordering
    auto trait_index = std::distance( powers.begin(), it );
    item.player -> artifact.points[ trait_index ].first = rank;
    // The initial trait may also be included, if it is the only trait available. In that case, we
    // should not increase the number of points purchased, so that we wont inflate the artificial
    // damage/stamina passives.
    if ( ( *it ) -> power_type != ARTIFACT_TRAIT_INITIAL )
    {
      item.player -> artifact.n_points += rank;
      item.player -> artifact.n_purchased_points += rank;
    }
  }

  auto initial_trait_it = range::find_if( powers, []( const artifact_power_data_t* p ) {
    return p -> power_type == ARTIFACT_TRAIT_INITIAL;
  } );

  // Blizzard will conditionally either include (if its the only one) or not include (if you have
  // purchased other traits) the initial trait for the artifact, for some unknown reason.  Thus, we
  // need to forcibly enable the initial trait in cases where the player has purchased any (other)
  // artifact traits.
  if ( initial_trait_it != powers.end() && item.player -> artifact.n_purchased_points > 0 )
  {
    auto trait_index = std::distance( powers.begin(), initial_trait_it );
    item.player -> artifact.points[ trait_index ].first = 1; // Data has max rank at 0
  }

  // If no relics inserted, bail out early
  if ( ! artifact.HasMember( "relics" ) || artifact[ "relics" ].Size() == 0 )
  {
    return;
  }

  for ( auto relic_idx = 0U, end = artifact[ "relics" ].Size(); relic_idx < end; ++relic_idx )
  {
    const auto& relic = artifact[ "relics" ][ relic_idx ];
    if ( ! relic.HasMember( "socket" ) )
    {
      continue;
    }

    auto relic_socket = relic[ "socket" ].GetUint();
    if ( relic.HasMember( "bonusLists" ) )
    {
      const auto& bonuses = relic[ "bonusLists" ];
      for ( auto bonus_idx = 0U, end = bonuses.Size(); bonus_idx < end; ++bonus_idx )
      {
        item.parsed.relic_data[ relic_socket ].push_back( bonuses[ bonus_idx ].GetUint() );
      }
    }

    // Blizzard includes both purchased and relic ranks in the same data, so we need to separate
    // the data so our artifact data is correct.
    auto relic_id = relic[ "itemId" ].GetUint();

    auto relic_trait_data = item.player -> dbc.artifact_relic_rank_index( artifact_id, relic_id );
    if ( relic_trait_data.first > 0 )
    {
      auto relic_power_idx = range::find_if( powers, [ &relic_trait_data ]( const artifact_power_data_t* p ) {
        return p -> id == relic_trait_data.first;
      } );

      if ( relic_power_idx != powers.end() )
      {
        auto trait_index = std::distance( powers.begin(), relic_power_idx );

        // So, Blizzard apparently decided that if your relic increases a trait you have not
        // chosen, they don't bother adding it to the artifact trait data, so add a bounds check
        // here so we don't go overeboard
        if ( item.player -> artifact.points[ trait_index ].first >= relic_trait_data.second )
        {
          item.player -> artifact.points[ trait_index ].first -= relic_trait_data.second;
          item.player -> artifact.n_purchased_points -= relic_trait_data.second;
          item.player -> artifact.n_points -= relic_trait_data.second;
        }
      }
    }
  }

  // Finally, for some completely insane reason Blizzard decided to put in the Relic-based ilevel
  // increases as bonus ids to the base item. Thus, we need to remove those since we actually use
  // the relic data to figure out the ilevel increases properly. Sigh. Since we do not know which
  // would be real ones and which not, just remove all bonus ids that "adjust ilevel" from the
  // artifact weapon itself.
  auto it = item.parsed.bonus_id.begin();
  while ( it != item.parsed.bonus_id.end() )
  {
    if ( *it == 0 )
    {
      continue;
    }

    const auto bonus_data = item.player -> dbc.item_bonus( *it );
    auto ilevel_it = range::find_if( bonus_data, []( const item_bonus_entry_t* entry ) {
      return entry -> type == ITEM_BONUS_ILEVEL;
    } );

    if ( ilevel_it != bonus_data.end() )
    {
      it = item.parsed.bonus_id.erase( it );
    }
    else
    {
      ++it;
    }
  }
}

bool parse_items( player_t*  p,
                  const rapidjson::Value& items )
{
  static const char* const slot_map[] =
  {
    "head",
    "neck",
    "shoulder",
    "shirt",
    "chest",
    "waist",
    "legs",
    "feet",
    "wrist",
    "hands",
    "finger1",
    "finger2",
    "trinket1",
    "trinket2",
    "back",
    "mainHand",
    "offHand",
    "ranged",
    "tabard"
  };

  assert( sizeof_array( slot_map ) == SLOT_MAX );

  for ( unsigned i = 0; i < SLOT_MAX; ++i )
  {
    item_t& item = p -> items[ i ];

    if ( ! items.HasMember( slot_map[ i ] ) )
      continue;

    const rapidjson::Value& data = items[ slot_map[ i ] ];
    if ( ! data.HasMember( "id" ) )
      continue;
    else
      item.parsed.data.id = data[ "id" ].GetUint();

    if ( data.HasMember( "bonusLists" ) )
    {
      for ( rapidjson::SizeType k = 0, n = data[ "bonusLists" ].Size(); k < n; ++k )
      {
        item.parsed.bonus_id.push_back( data[ "bonusLists" ][ k ].GetInt() );
      }
    }

    if ( ! data.HasMember( "tooltipParams" ) )
      continue;

    const rapidjson::Value& params = data[ "tooltipParams" ];

    if ( params.HasMember( "gem0" ) ) item.parsed.gem_id[ 0 ] = params[ "gem0" ].GetUint();
    if ( params.HasMember( "gem1" ) ) item.parsed.gem_id[ 1 ] = params[ "gem1" ].GetUint();
    if ( params.HasMember( "gem2" ) ) item.parsed.gem_id[ 2 ] = params[ "gem2" ].GetUint();
    if ( params.HasMember( "enchant" ) ) item.parsed.enchant_id = params[ "enchant" ].GetUint();
    if ( params.HasMember( "tinker" ) ) item.parsed.addon_id = params[ "tinker" ].GetUint();
    if ( params.HasMember( "suffix" ) ) item.parsed.suffix_id = params[ "suffix" ].GetInt();

    if ( params.HasMember( "upgrade" ) )
    {
      const rapidjson::Value& upgrade = params[ "upgrade" ];
      if ( upgrade.HasMember( "current" ) ) item.parsed.upgrade_level = upgrade[ "current" ].GetUint();
    }

    // Artifact
    if ( data.HasMember( "quality" ) && data[ "quality" ].GetUint() == ITEM_QUALITY_ARTIFACT )
    {
      parse_artifact( item, data );
    }

    // Since Armory API does not give us the drop level of items (such as quest items), we will need
    // to implement a hack here. We do this by checking if Blizzard includes a ITEM_BONUS_SCALING_2
    // type bonus in their bonusLists array, and set the drop level to the player's level. Note that
    // this may result in incorrect stats if the player attained the item during leveling, but
    // there's little else we can do, as the "itemLevel" value of the item info is the base ilevel,
    // which in many cases is very incorrect.
    if ( item_database::has_item_bonus_type( item, ITEM_BONUS_SCALING_2 ) )
    {
      auto item_data = p -> dbc.item( item.parsed.data.id );
      item.parsed.drop_level = p -> true_level;
      p -> sim -> errorf( "Player %s item '%s' in slot '%s' uses drop-level based scaling, setting drop level to %u.",
        p -> name(), item_data ? item_data -> name : "unknown", item.slot_name(), item.parsed.drop_level );
    }
  }

  return true;
}

// parse_player =============================================================

player_t* parse_player( sim_t*             sim,
                        player_spec_t&     player,
                        cache::behavior_e  caching )
{
  sim -> current_slot = 0;

  std::string result;
  rapidjson::Document profile;

  // China does not have mashery endpoints, so no point in even trying to get anything here
  if ( util::str_compare_ci( player.region, "cn" ) )
  {
    return nullptr;
  }
  else if ( player.local_json.empty() )
  {
    if ( ! download( sim, profile, result, player.url, player.cleanurl, caching ) )
    {
      return nullptr;
    }
  }
  else
  {
    io::ifstream ifs;
    ifs.open( player.local_json );
    result.assign( ( std::istreambuf_iterator<char>( ifs ) ),
                   ( std::istreambuf_iterator<char>()    ) );
  }

  if ( profile.HasParseError() )
  {
    sim -> errorf( "BCP API: Unable to download player from '%s', JSON parse error\n", player.cleanurl.c_str() );
    if ( sim -> apikey.empty() ) { sim -> errorf( "If you built this from source, remember to add your own api key." ); }
    else if ( sim -> apikey.size() != 32 ) { sim -> errorf( "Check api key, must be 32 characters long." ); }
    return nullptr;
  }

  if ( sim -> debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    profile.Accept( writer );
    sim -> out_debug.raw() << b.GetString();
  }

  if ( profile.HasMember( "status" ) && util::str_compare_ci( profile[ "status" ].GetString(), "nok" ) )
  {
    sim -> errorf( "BCP API: Unable to download player from '%s', reason: %s\n",
                   player.cleanurl.c_str(),
                   profile[ "reason" ].GetString() );
    return nullptr;
  }

  if ( profile.HasMember( "name" ) )
    player.name = profile[ "name" ].GetString();

  if ( ! profile.HasMember( "level" ) )
  {
    sim -> errorf( "BCP API: Unable to extract player level from '%s'. Using fallback method to import armory.\n", player.cleanurl.c_str() );
    return nullptr;
  }

  if ( ! profile.HasMember( "class" ) )
  {
    sim -> errorf( "BCP API: Unable to extract player class from '%s'.\n", player.cleanurl.c_str() );
    return nullptr;
  }

  if ( ! profile.HasMember( "race" ) )
  {
    sim -> errorf( "BCP API: Unable to extract player race from '%s'.\n", player.cleanurl.c_str() );
    return nullptr;
  }

  if ( ! profile.HasMember( "talents" ) )
  {
    sim -> errorf( "BCP API: Unable to extract player talents from '%s'.\n", player.cleanurl.c_str() );
    return nullptr;
  }

  std::string class_name = util::player_type_string( util::translate_class_id( profile[ "class" ].GetUint() ) );
  race_e race = util::translate_race_id( profile[ "race" ].GetUint() );
  const module_t* module = module_t::get( class_name );

  if ( ! module || ! module -> valid() )
  {
    sim -> errorf( "\nModule for class %s is currently not available.\n", class_name.c_str() );
    return nullptr;
  }

  std::string name = player.name;

  if ( player.talent_spec != "active" && ! player.talent_spec.empty() )
  {
    name += '_';
    name += player.talent_spec;
  }

  if ( ! name.empty() )
    sim -> current_name = name;

  player_t* p = sim -> active_player = module -> create_player( sim, name, race );

  if ( ! p )
  {
    sim -> errorf( "BCP API: Unable to build player with class '%s' and name '%s' from '%s'.\n",
                   class_name.c_str(), name.c_str(), player.cleanurl.c_str() );
    return nullptr;
  }

  p -> true_level = profile[ "level" ].GetUint();
  p -> region_str = player.region.empty() ? sim -> default_region_str : player.region;
  if ( ! profile.HasMember( "realm" ) && ! player.server.empty() )
    p -> server_str = player.server;
  else
    p -> server_str = profile[ "realm" ].GetString();

  if ( ! player.origin.empty() )
    p -> origin_str = player.origin;

  if ( profile.HasMember( "thumbnail" ) )
    p -> report_information.thumbnail_url = "https://render-api-" + p -> region_str + ".worldofwarcraft.com/static-render/" +
                                            p -> region_str + '/' + profile[ "thumbnail" ].GetString();

  if ( profile.HasMember( "professions" ) )
  {
    parse_profession( p -> professions_str, profile[ "professions" ], 0 );
    parse_profession( p -> professions_str, profile[ "professions" ], 1 );
  }

  if ( ! parse_talents( p, profile[ "talents" ], player.talent_spec ) )
    return nullptr;

  if ( profile.HasMember( "items" ) && ! parse_items( p, profile[ "items" ] ) )
    return nullptr;

  if ( ! p -> server_str.empty() )
    p -> armory_extensions( p -> region_str, p -> server_str, player.name, caching );

  return p;
}

// download_item_data =======================================================

bool download_item_data( item_t& item, cache::behavior_e caching )
{
  rapidjson::Document js;
  if ( ! download_id( item.sim, js, item.player -> region_str, item.parsed.data.id, item.sim -> apikey, caching ) ||
       js.HasParseError() )
  {
    if ( caching != cache::ONLY )
    {
      item.sim -> errorf( "BCP API: Player '%s' unable to download item id '%u' at slot %s.\n",
                          item.player -> name(), item.parsed.data.id, item.slot_name() );
    }
    return false;
  }

  if ( item.sim -> debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    js.Accept( writer );
    item.sim -> out_debug.raw() << b.GetString();
  }

  try
  {
    if ( ! js.HasMember( "id" ) ) throw( "id" );
    if ( ! js.HasMember( "itemLevel" ) ) throw( "item level" );
    if ( ! js.HasMember( "quality" ) ) throw( "quality" );
    if ( ! js.HasMember( "inventoryType" ) ) throw( "inventory type" );
    if ( ! js.HasMember( "itemClass" ) ) throw( "item class" );
    if ( ! js.HasMember( "itemSubClass" ) ) throw( "item subclass" );
    if ( ! js.HasMember( "name" ) ) throw( "name" );

    item.parsed.data.id = js[ "id" ].GetUint();
    item.parsed.data.level = js[ "itemLevel" ].GetUint();
    item.parsed.data.quality = js[ "quality" ].GetUint();
    item.parsed.data.inventory_type = js[ "inventoryType" ].GetUint();
    item.parsed.data.item_class = js[ "itemClass" ].GetUint();
    item.parsed.data.item_subclass = js[ "itemSubClass" ].GetUint();
    item.name_str = js[ "name" ].GetString();

    util::tokenize( item.name_str );

    if ( js.HasMember( "icon" ) ) item.icon_str = js[ "icon" ].GetString();
    if ( js.HasMember( "requiredLevel" ) ) item.parsed.data.req_level = js[ "requiredLevel" ].GetUint();
    if ( js.HasMember( "requiredSkill" ) ) item.parsed.data.req_skill = js[ "requiredSkill" ].GetUint();
    if ( js.HasMember( "requiredSkillRank" ) ) item.parsed.data.req_skill_level = js[ "requiredSkillRank" ].GetUint();
    if ( js.HasMember( "itemBind" ) ) item.parsed.data.bind_type = js[ "itemBind" ].GetUint();

    if ( js.HasMember( "weaponInfo" ) )
    {
      const rapidjson::Value& weaponInfo = js[ "weaponInfo" ];
      if ( ! weaponInfo.HasMember( "dps" ) ) throw( "dps" );
      if ( ! weaponInfo.HasMember( "weaponSpeed" ) ) throw( "weapon speed" );
      if ( ! weaponInfo.HasMember( "damage" ) ) throw( "damage" );

      const rapidjson::Value& damage = weaponInfo[ "damage" ];
      if ( ! damage.HasMember( "exactMin" ) ) throw( "weapon minimum damage" );

      item.parsed.data.delay = static_cast< unsigned >( weaponInfo[ "weaponSpeed" ].GetDouble() * 1000.0 );
      item.parsed.data.dmg_range = 2 - 2 * damage[ "exactMin" ].GetDouble() / ( weaponInfo[ "dps" ].GetDouble() * weaponInfo[ "weaponSpeed" ].GetDouble() );
    }

    if ( js.HasMember( "allowableClasses" ) )
    {
      for ( rapidjson::SizeType i = 0, n = js[ "allowableClasses" ].Size(); i < n; ++i )
        item.parsed.data.class_mask |= ( 1 << ( js[ "allowableClasses" ][ i ].GetInt() - 1 ) );
    }
    else
      item.parsed.data.class_mask = -1;

    if ( js.HasMember( "allowableRaces" ) )
    {
      for ( rapidjson::SizeType i = 0, n = js[ "allowableRaces" ].Size(); i < n; ++i )
        item.parsed.data.race_mask |= ( 1 << ( js[ "allowableRaces" ][ i ].GetInt() - 1 ) );
    }
    else
      item.parsed.data.race_mask = -1;

    if ( js.HasMember( "bonusStats" ) )
    {
      for ( rapidjson::SizeType i = 0, n = js[ "bonusStats" ].Size(); i < n; ++i )
      {
        const rapidjson::Value& stat = js[ "bonusStats" ][ i ];
        if ( ! stat.HasMember( "stat" ) ) throw( "bonus stat" );
        if ( ! stat.HasMember( "amount" ) ) throw( "bonus stat amount" );

        item.parsed.data.stat_type_e[ i ] = stat[ "stat" ].GetInt();
        item.parsed.data.stat_val[ i ] =  stat[ "amount" ].GetInt();

        if ( js.HasMember( "weaponInfo" ) && 
             ( item.parsed.data.stat_type_e[ i ] == ITEM_MOD_INTELLECT ||
               item.parsed.data.stat_type_e[ i ] == ITEM_MOD_SPIRIT ||
               item.parsed.data.stat_type_e[ i ] == ITEM_MOD_SPELL_POWER ) )
          item.parsed.data.flags_2 |= ITEM_FLAG2_CASTER_WEAPON;
      }
    }

    if ( js.HasMember( "socketInfo" ) && js[ "socketInfo" ].HasMember( "sockets" ) )
    {
      const rapidjson::Value& sockets = js[ "socketInfo" ][ "sockets" ];

    for (rapidjson::SizeType i = 0, n = as<rapidjson::SizeType>( std::min(static_cast< size_t >(sockets.Size()), sizeof_array(item.parsed.data.socket_color))); i < n; ++i)
      {
        if ( ! sockets[ i ].HasMember( "type" ) )
          continue;

        std::string color = sockets[ i ][ "type" ].GetString();

        if ( color == "META" )
          item.parsed.data.socket_color[ i ] = SOCKET_COLOR_META;
        else if ( color == "RED" )
          item.parsed.data.socket_color[ i ] = SOCKET_COLOR_RED;
        else if ( color == "YELLOW" )
          item.parsed.data.socket_color[ i ] = SOCKET_COLOR_YELLOW;
        else if ( color == "BLUE" )
          item.parsed.data.socket_color[ i ] = SOCKET_COLOR_BLUE;
        else if ( color == "PRISMATIC" )
          item.parsed.data.socket_color[ i ] = SOCKET_COLOR_PRISMATIC;
        else if ( color == "COGWHEEL" )
          item.parsed.data.socket_color[ i ] = SOCKET_COLOR_COGWHEEL;
        else if ( color == "HYDRAULIC" )
          item.parsed.data.socket_color[ i ] = SOCKET_COLOR_HYDRAULIC;
      }

      if ( js[ "socketInfo" ].HasMember( "socketBonus" ) )
      {
        std::string socketBonus = js[ "socketInfo" ][ "socketBonus" ].GetString();
        std::string stat;
        util::fuzzy_stats( stat, socketBonus );
        std::vector<stat_pair_t> bonus = item_t::str_to_stat_pair( stat );
        item.parsed.socket_bonus_stats.insert( item.parsed.socket_bonus_stats.end(), bonus.begin(), bonus.end() );
      }
    }

    if ( js.HasMember( "itemSet" ) && js[ "itemSet" ].HasMember( "id" ) )
    {
      item.parsed.data.id_set = js[ "itemSet" ][ "id" ].GetUint();
    }

    if ( js.HasMember( "nameDescription" ) )
    {
      std::string nameDescription = js[ "nameDescription" ].GetString();

      if ( util::str_in_str_ci( nameDescription, "heroic" ) )
        item.parsed.data.type_flags |= RAID_TYPE_HEROIC;
      else if ( util::str_in_str_ci( nameDescription, "raid finder" ) )
        item.parsed.data.type_flags |= RAID_TYPE_LFR;
      else if ( util::str_in_str_ci( nameDescription, "mythic" ) )
        item.parsed.data.type_flags |= RAID_TYPE_MYTHIC;

      if ( util::str_in_str_ci( nameDescription, "warforged" ) )
        item.parsed.data.type_flags |= RAID_TYPE_WARFORGED;
    }

    if ( js.HasMember( "itemSpells" ) )
    {
      const rapidjson::Value& spells = js[ "itemSpells" ];
      size_t spell_idx = 0;
      for ( rapidjson::SizeType i = 0, n = spells.Size(); i < n && spell_idx < sizeof_array( item.parsed.data.id_spell ); ++i )
      {
        const rapidjson::Value& spell = spells[ i ];
        if ( ! spell.HasMember( "spellId" ) || ! spell.HasMember( "trigger" ) )
          continue;

        int spell_id = spell[ "spellId" ].GetInt();
        int trigger_type = -1;

        if ( util::str_compare_ci( spell[ "trigger" ].GetString(), "ON_EQUIP" ) )
          trigger_type = ITEM_SPELLTRIGGER_ON_EQUIP;
        else if ( util::str_compare_ci( spell[ "trigger" ].GetString(), "ON_USE" ) )
          trigger_type = ITEM_SPELLTRIGGER_ON_USE;
        else if ( util::str_compare_ci( spell[ "trigger" ].GetString(), "ON_PROC" ) )
          trigger_type = ITEM_SPELLTRIGGER_CHANCE_ON_HIT;

        if ( trigger_type != -1 && spell_id > 0 )
        {
          item.parsed.data.id_spell[ spell_idx ] = spell_id;
          item.parsed.data.trigger_spell[ spell_idx ] = trigger_type;
          spell_idx++;
        }
      }
    }
  }
  catch ( const char* fieldname )
  {
    std::string error_str;
    if ( js.HasMember( "reason" ) )
      error_str = js[ "reason" ].GetString();

    if ( caching != cache::ONLY )
      item.sim -> errorf( "BCP API: Player '%s' unable to parse item '%u' %s at slot '%s': %s\n",
                          item.player -> name(), item.parsed.data.id, fieldname, item.slot_name(), error_str.c_str() );
    return false;
  }

  return true;
}

// download_roster ==========================================================

bool download_roster( rapidjson::Document& d,
                      sim_t* sim,
                      const std::string& region,
                      const std::string& server,
                      const std::string& name,
                      cache::behavior_e  caching )
{
  std::string url;
  std::string cleanurl;
  if ( sim -> apikey.size() == 32 && region != "cn" ) //China does not have new api endpoints yet.
  {
    cleanurl = "https://" + region + ".api.battle.net/wow/guild/" + server + '/' + name + "?fields=members&apikey=";
    url = cleanurl + sim -> apikey;
  }
  else
  {
    url = "http://" + region + ".battle.net/api/wow/guild/" + server + '/' +
      name + "?fields=members";
    cleanurl = url;
  }

  std::string result;
  if ( ! download( sim, d, result, url, cleanurl, caching ) )
  {
    return false;
  }

  if ( d.HasParseError() )
  {
    sim -> errorf( "BCP API: Unable to parse guild from '%s': Parse error '%s' @ %lu\n",
      cleanurl.c_str(), d.GetParseError(), d.GetErrorOffset() );

    return false;
  }

  if ( sim -> debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    d.Accept( writer );
    sim -> out_debug.raw() << b.GetString();
  }

  return true;
}

} // close anonymous namespace ==============================================

// bcp_api::download_player =================================================

player_t* bcp_api::download_player( sim_t*             sim,
                                    const std::string& region,
                                    const std::string& server,
                                    const std::string& name,
                                    const std::string& talents,
                                    cache::behavior_e  caching )
{
  sim -> current_name = name;

  player_spec_t player;

  if ( sim -> apikey.size() == 32 && region != "cn" ) // China does not have new api endpoints yet.
  {
    std::string battlenet = "https://" + region + ".api.battle.net/";

    player.cleanurl = battlenet + "wow/character/" +
      server + '/' + name + "?fields=talents,items,professions&locale=en_US&apikey=";
    player.url = player.cleanurl + sim -> apikey;
    player.origin = battlenet + "wow/character/" + server + '/' + name + "/advanced";
#ifdef SC_DEFAULT_APIKEY
  if ( sim -> apikey == std::string( SC_DEFAULT_APIKEY ) )
  //This is needed to prevent hitting the 'per second' api call limit.
  // If the character is cached, it still counts as a api use, even though we don't download anything.
  // With cached characters, it's common for 30-40 calls to be made per second when downloading a guild.
  // This is only enabled when the person is using the default apikey.
#if defined ( SC_WINDOWS )
    _sleep( 250 );
#else
    usleep( 250000 );
#endif
#endif
  }
  else
  {
    std::string battlenet = "http://" + region + ".battle.net/";

    player.url = battlenet + "api/wow/character/" +
      server + '/' + name + "?fields=talents,items,professions&locale=en_US";
    player.cleanurl = player.url;
    player.origin = battlenet + "wow/en/character/" + server + '/' + name + "/advanced";
  }

  player.region = region;
  player.server = server;
  player.name = name;

  player.talent_spec = talents;

  return parse_player( sim, player, caching );
}

// bcp_api::from_local_json =================================================

player_t* bcp_api::from_local_json( sim_t*             sim,
                                    const std::string& name,
                                    const std::string& file_path,
                                    const std::string& talents
                                  )
{
  sim -> current_name = name;

  player_spec_t player;

  player.local_json = file_path;
  player.origin = file_path;

  player.name = name;
  player.talent_spec = talents;

  return parse_player( sim, player, cache::ANY );
}

// bcp_api::download_item() =================================================

bool bcp_api::download_item( item_t& item, cache::behavior_e caching )
{
  bool ret = download_item_data( item, caching );
  if ( ret )
    item.source_str = "Blizzard";
  return ret;
}

// bcp_api::download_guild ==================================================

bool bcp_api::download_guild( sim_t* sim,
                              const std::string& region,
                              const std::string& server,
                              const std::string& name,
                              const std::vector<int>& ranks,
                              int player_filter,
                              int max_rank,
                              cache::behavior_e caching )
{
  rapidjson::Document js;

  if ( ! download_roster( js, sim, region, server, name, caching ) )
    return false;

  if ( ! js.HasMember( "members" ) )
  {
    sim -> errorf( "No members in guild %s,%s,%s", region.c_str(), server.c_str(), name.c_str() );
    return false;
  }

  std::vector<std::string> names;

  for ( rapidjson::SizeType i = 0, end = js[ "members" ].Size(); i < end; i++ )
  {
    const rapidjson::Value& member = js[ "members" ][ i ];
    if ( ! member.HasMember( "character" ) )
      continue;

    if ( ! member.HasMember( "rank" ) )
      continue;

    int rank = member[ "rank" ].GetInt();
    if ( ( max_rank > 0 && rank > max_rank ) || 
         ( ! ranks.empty() && range::find( ranks, rank ) == ranks.end() ) )
      continue;

    const rapidjson::Value& character = member[ "character" ];

    if ( ! character.HasMember( "level" ) || character[ "level" ].GetUint() < 85 )
      continue;

    if ( ! character.HasMember( "class" ) )
      continue;

    if ( ! character.HasMember( "name" ) )
      continue;

    if ( player_filter != PLAYER_NONE &&
         player_filter != util::translate_class_id( character[ "class" ].GetInt() ) )
      continue;

    names.push_back( character[ "name" ].GetString() );
  }

  if ( names.empty() ) return true;

  range::sort( names );

  for (auto & cname : names)
  {
    
    std::cout << "Downloading character: " << cname << std::endl;
    download_player( sim, region, server, cname, "active", caching );
  }

  return true;
}
