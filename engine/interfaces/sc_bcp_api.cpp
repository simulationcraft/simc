// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "util/rapidjson/document.h"
#include "util/rapidjson/stringbuffer.h"
#include "util/rapidjson/prettywriter.h"

#ifndef SC_NO_NETWORKING
#include <curl/curl.h>
#endif

// ==========================================================================
// Blizzard Community Platform API
// ==========================================================================

namespace { // UNNAMED NAMESPACE

struct player_spec_t
{
  std::string region, server, name, url, local_json, origin, talent_spec;
};

static const std::string GLOBAL_OAUTH_ENDPOINT_URI = "https://{}.battle.net/oauth/token";
static const std::string CHINA_OAUTH_ENDPOINT_URI = "https://www.battlenet.com.cn/oauth/token";

static const std::string GLOBAL_GUILD_ENDPOINT_URI = "https://{}.api.blizzard.com/wow/guild/{}/{}?fields=members&locale={}";
static const std::string CHINA_GUILD_ENDPOINT_URI = "https://gateway.battlenet.com.cn/wow/guild/{}/{}?fields=members&locale={}";

static const std::string GLOBAL_PLAYER_ENDPOINT_URI = "https://{}.api.blizzard.com/profile/wow/character/{}/{}?namespace=profile-{}&locale={}";
static const std::string CHINA_PLAYER_ENDPOINT_URI = "https://gateway.battlenet.com.cn/profile/wow/character/{}/{}?namespace=profile-cn";

static const std::string GLOBAL_ITEM_ENDPOINT_URI = "https://{}.api.blizzard.com/wow/item/{}?locale={}";
static const std::string CHINA_ITEM_ENDPOINT_URI = "https://gateway.battlenet.com.cn/wow/item/{}";

static const std::string GLOBAL_ORIGIN_URI = "https://worldofwarcraft.com/{}/character/{}/{}";
static const std::string CHINA_ORIGIN_URI = "https://www.wowchina.com/zh-cn/character/{}/{}";

static std::unordered_map<std::string, std::pair<std::string, std::string>> LOCALES {
  { "us", { "en_US", "en-us" } },
  { "eu", { "en_GB", "en-gb" } },
  { "kr", { "ko_KR", "ko-kr" } },
  { "tw", { "zh_TW", "zh-tw" } }
};

static std::string token_path = "";
static std::string token = "";
static bool authorization_failed = false;
mutex_t token_mutex;

#ifndef SC_NO_NETWORKING

size_t data_cb( void* contents, size_t size, size_t nmemb, void* usr )
{
  std::string* obj = reinterpret_cast<std::string*>( usr );
  obj->append( reinterpret_cast<const char*>( contents ), size * nmemb );
  return size * nmemb;
}

std::vector<std::string> token_paths()
{
  std::vector<std::string> paths;

  paths.push_back( "./simc-apitoken" );

  if ( const char* home_path = getenv( "HOME" ) )
  {
    paths.push_back( std::string( home_path ) + "/.simc-apitoken" );
  }

  if ( const char* home_drive = getenv( "HOMEDRIVE" ) )
  {
    if ( const char* home_path = getenv( "HOMEPATH" ) )
    {
      paths.push_back( std::string( home_drive ) + std::string( home_path ) + "/simc-apitoken" );
    }
  }

  return paths;
}

// Authorize to the blizzard api
bool authorize( sim_t* sim, const std::string& region )
{
  if ( !sim->user_apitoken.empty() )
  {
    return true;
  }

  // Authorization needs to be a single threaded process, all threads will re-use the same token
  // once a single thread properly fetches it
  auto_lock_t lock( token_mutex );

  // If an authorization process failed on any thread attempting to perform it, no point trying it
  // again, just fail the rest
  if ( authorization_failed )
  {
    return false;
  }

  // A token has already been loaded, so don't re-authorize
  if ( !token.empty() )
  {
    return true;
  }

  std::string ua_str = "Simulationcraft/" + std::string( SC_VERSION );

  std::string oauth_endpoint;
  if ( util::str_compare_ci( region, "eu" ) || util::str_compare_ci( region, "us" ) )
  {
    oauth_endpoint = fmt::format( GLOBAL_OAUTH_ENDPOINT_URI, region );
  }
  else if ( util::str_compare_ci( region, "kr" ) || util::str_compare_ci( region, "tw" ) )
  {
    oauth_endpoint = fmt::format( GLOBAL_OAUTH_ENDPOINT_URI, "apac" );
  }
  else if ( util::str_compare_ci( region, "cn" ) )
  {
    oauth_endpoint = CHINA_OAUTH_ENDPOINT_URI;
  }

  auto handle = curl_easy_init();
  std::string buffer;
  char error_buffer[ CURL_ERROR_SIZE ];
  error_buffer[ 0 ] = '\0';

  curl_easy_setopt( handle, CURLOPT_URL, oauth_endpoint.c_str() );
  //curl_easy_setopt( handle, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt( handle, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt( handle, CURLOPT_POSTFIELDS, "grant_type=client_credentials" );
  curl_easy_setopt( handle, CURLOPT_USERPWD, sim->apikey.c_str() );
  curl_easy_setopt( handle, CURLOPT_TIMEOUT, 15L );
  curl_easy_setopt( handle, CURLOPT_FOLLOWLOCATION, 1L );
  curl_easy_setopt( handle, CURLOPT_MAXREDIRS, 5L );
  curl_easy_setopt( handle, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt( handle, CURLOPT_USERAGENT, ua_str.c_str() );
  curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, data_cb );
  curl_easy_setopt( handle, CURLOPT_WRITEDATA, reinterpret_cast<void*>( &buffer ) );
  curl_easy_setopt( handle, CURLOPT_ERRORBUFFER, error_buffer );

  auto res = curl_easy_perform( handle );
  if ( res != CURLE_OK )
  {
    std::cerr << "Unable to fetch bearer token from " << oauth_endpoint << ", " << error_buffer << std::endl;
    curl_easy_cleanup( handle );
    authorization_failed = true;
    return false;
  }

  rapidjson::Document response;
  response.Parse< 0 >( buffer );
  if ( response.HasParseError() )
  {
    std::cerr << "Unable to parse response message from " << oauth_endpoint << std::endl;
    curl_easy_cleanup( handle );
    authorization_failed = true;
    return false;
  }

  if ( !response.HasMember( "access_token" ) )
  {
    std::cerr << "Malformed JSON object from " << oauth_endpoint << std::endl;
    curl_easy_cleanup( handle );
    authorization_failed = true;
    return false;
  }

  token = response[ "access_token" ].GetString();

  curl_easy_cleanup( handle );

  return true;
}
#else
bool authorize( sim_t*, const std::string& )
{
  return false;
}
#endif /* SC_NO_NETWORKING */

// download

bool download( sim_t*               sim,
               rapidjson::Document& d,
               std::string&         result,
               const std::string&   region,
               const std::string&   url,
               cache::behavior_e    caching )
{
  std::vector<std::string> headers;

  if ( !authorize( sim, region ) )
  {
    return false;
  }

  if ( !sim->user_apitoken.empty() )
  {
    headers.push_back( "Authorization: Bearer " + sim->user_apitoken );
  }
  else
  {
    headers.push_back( "Authorization: Bearer " + token );
  }

  int response_code = 0;
  // We can make two attempts at most
  for ( size_t i = 0; i < 2; ++i )
  {
    response_code = http::get( result, url, caching, "", headers );
    // 200 OK, 404 Not Found, consider them "successful" results
    if ( response_code == 200 || response_code == 404 )
    {
      break;
    }

    // Blizzard's issue, lets not bother trying again
    if ( response_code >= 500 && response_code < 600 )
    {
      return false;
    }
    // Bearer token is invalid, lets try to regenerate if we can
    else if ( response_code == 401 || response_code == 403 )
    {
      // Loaded token is bogus, so clear it
      token.clear();

      // If there's an user provided apitoken and we get 401/403, or if we already regenerated
      // our bearer token successfully, there's no point in trying again
      if ( !sim->user_apitoken.empty() )
      {
        sim->error( "Invalid user 'apitoken' option value '{}'", sim->user_apitoken );
        return false;
      }

      // Re-Authorization failed
      if ( !authorize( sim, region ) )
      {
        return false;
      }

      // Clear old bearer token from headers and add the new one
      headers.clear();
      headers.push_back( "Authorization: Bearer " + token );
    }
  }

  d.Parse<0>( result.c_str() );

  // Corrupt data
  if ( ! result.empty() && d.HasParseError() )
  {
    sim->error( "Malformed response from Blizzard API" );
    return false;
  }

  // NOK status, report Blizzard API reason to the user
  if ( d.IsObject() && d.HasMember( "status" ) && util::str_compare_ci( d[ "status" ].GetString(), "nok" ) )
  {
    sim->error( "Error response from Blizzard API: {}", d[ "reason" ].GetString() );
    return false;
  }

  // Download is successful only with 200 OK response code
  return response_code == 200;
}

// download_id ==============================================================

bool download_id( sim_t* sim,
                  rapidjson::Document& d,
                  const std::string& region,
                  unsigned item_id,
                  cache::behavior_e caching )
{
  if ( item_id == 0 )
    return false;

  std::string url;

  if ( !util::str_compare_ci( region, "cn" ) )
  {
    url = fmt::format( GLOBAL_ITEM_ENDPOINT_URI, region, item_id, LOCALES[ region ].first );
  }
  else
  {
    url = fmt::format( CHINA_ITEM_ENDPOINT_URI, item_id );
  }

  std::string result;

  if ( ! download( sim, d, result, region, url, caching ) )
    return false;

  return true;
}

// parse_professions ========================================================

void parse_professions( player_t* p, const std::string& url, cache::behavior_e caching )
{
  rapidjson::Document collections_data;
  std::string result;

  if ( ! download( p->sim, collections_data, result, p->region_str, url + "&locale=en_US", caching ) )
  {
    throw std::runtime_error( fmt::format("Unable to download collections data JSON from '{}'.",
        url ) );
  }

  if ( p->sim->debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    collections_data.Accept( writer );
    p->sim->out_debug.raw() << b.GetString();
  }

  /*
  if ( ! profile.HasMember( "primary" ) )
    return;

  const rapidjson::Value& professions = profile[ "primary" ];

  std::vector<profession_e> base_professions;

  // First, find the two base professions
  for ( auto idx = 0u, end = professions.Size(); idx < end && base_professions.size() < 2; ++idx )
  {
    const rapidjson::Value& profession = professions[ idx ];
    if ( ! profession.HasMember( "id" ) )
    {
      continue;
    }

    auto internal_profession = util::translate_profession_id( profession[ "id" ].GetUint() );
    if ( internal_profession == PROFESSION_NONE )
    {
      continue;
    }

    base_professions.push_back( internal_profession );
  }

  // Grab the rank from the first non-base profession entry, hoping that blizzard orders them
  // sensibly
  for ( auto profession_id : base_professions )
  {
    auto profession_token = util::profession_type_string( profession_id );

    for ( auto idx = 0u, end = professions.Size(); idx < end; ++idx )
    {
      const rapidjson::Value& profession = professions[ idx ];
      if ( ! profession.HasMember( "id" ) || ! profession.HasMember( "rank" ) || ! profession.HasMember( "name" ) )
      {
        continue;
      }

      // Skip the base profession
      auto internal_profession = util::translate_profession_id( profession[ "id" ].GetUint() );
      if ( internal_profession != PROFESSION_NONE )
      {
        continue;
      }

      std::string profession_name = profession[ "name" ].GetString();
      util::tokenize( profession_name );

      if ( ! util::str_in_str_ci( profession_name, profession_token ) )
      {
        continue;
      }

      if ( professions_str.length() > 0 )
        professions_str += '/';

      professions_str += util::profession_type_string( profession_id );
      professions_str += "=";
      professions_str += util::to_string( profession[ "rank" ].GetUint() );
      break;
    }
  }
  */
}

// parse_talents ============================================================

void parse_talents( player_t* p, const std::string& url, cache::behavior_e caching )
{
  rapidjson::Document spec;
  std::string result;

  if ( ! download( p->sim, spec, result, p->region_str, url + "&locale=en_US", caching ) )
  {
    throw std::runtime_error(fmt::format("Unable to download talent JSON from '{}'.",
        url ));
  }

  if ( p->sim->debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    spec.Accept( writer );
    p->sim->out_debug.raw() << b.GetString();
  }

  if ( !spec.HasMember( "active_specialization" ) ||
       !spec[ "active_specialization" ].HasMember( "id" ) )
  {
    throw std::runtime_error( fmt::format( "Unable to determine active spec for talent parsing" ) );
  }

  unsigned spec_id = spec[ "active_specialization" ][ "id" ].GetUint();

  // Iterate over talent specs, choosing the correct one
  for ( auto idx = 0U, end = spec[ "specializations" ].Size(); idx < end; ++idx )
  {
    const auto& spec_data = spec[ "specializations" ][ idx ];
    if ( !spec_data.HasMember( "specialization" ) ||
         !spec_data[ "specialization" ].HasMember( "id" ) )
    {
      throw std::runtime_error( fmt::format( "Unable to determine talent spec for talent parsing" ) );
    }

    if ( spec_data[ "specialization" ][ "id" ].GetUint() != spec_id )
    {
      continue;
    }

    if ( !spec_data.HasMember( "talents" ) )
    {
      continue;
    }

    const auto& talents = spec_data[ "talents" ];

    for ( auto talent_idx = 0u, talent_end = talents.Size(); talent_idx < talent_end; ++talent_idx )
    {
      const auto& talent_data = talents[ talent_idx ];

      if ( !talent_data.HasMember( "talent" ) || !talent_data[ "talent" ].HasMember( "id" ) )
      {
        throw std::runtime_error( "Unable to determine talent id for talent parsing" );
      }

      auto talent_id = talent_data[ "talent" ][ "id" ].GetUint();
      const auto talent = p->dbc.talent( talent_id );
      if ( talent->id() != talent_id )
      {
        p->sim->error( "Warning: Unable to find talent id {} for {} from Simulationcraft client data",
          talent_id, p->name() );
        continue;
      }

      p->talent_points.select_row_col( talent->row(), talent->col() );
    }
  }
}

// parse_items ==============================================================

bool parse_artifact( item_t& item, const rapidjson::Value& artifact )
{
  if ( ! artifact.HasMember( "artifactId" ) || ! artifact.HasMember( "artifactTraits" ) )
  {
    return true;
  }

  auto artifact_id = artifact[ "artifactId" ].GetUint();
  if ( artifact_id == 0 )
  {
    return true;
  }

  // If no relics inserted, bail out early
  if ( ! artifact.HasMember( "relics" ) || artifact[ "relics" ].Size() == 0 )
  {
    return true;
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

    if ( relic_trait_data.first > 0 && relic_trait_data.second > 0 )
    {
      item.player -> artifact -> move_purchased_rank( relic_idx,
                                                      relic_trait_data.first,
                                                      relic_trait_data.second );
    }
  }

  return true;
}

void parse_items( player_t* p, const std::string& url, cache::behavior_e caching )
{
  rapidjson::Document equipment_data;
  std::string result;

  if ( ! download( p->sim, equipment_data, result, p->region_str, url + "&locale=en_US", caching ) )
  {
    throw std::runtime_error(fmt::format("Unable to download equipment JSON from '{}'.",
        url ));
  }

  if ( p->sim->debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    equipment_data.Accept( writer );
    p->sim->out_debug.raw() << b.GetString();
  }

  if ( !equipment_data.HasMember( "equipped_items" ) )
  {
    return;
  }

  for ( auto idx = 0u, end = equipment_data[ "equipped_items" ].Size(); idx < end; ++idx )
  {
    const auto& slot_data = equipment_data[ "equipped_items" ][ idx ];

    if ( !slot_data.HasMember( "item" ) || !slot_data[ "item" ].HasMember( "id" ) )
    {
      throw std::runtime_error( "Unable to parse item data: Missing item information" );
    }

    if ( !slot_data.HasMember( "slot" ) || !slot_data[ "slot" ].HasMember( "type" ) )
    {
      throw std::runtime_error( "Unable to parse item data: Missing slot information" );
    }

    slot_e slot = bcp_api::translate_api_slot( slot_data[ "slot" ][ "type" ].GetString() );
    if ( slot == SLOT_INVALID )
    {
      throw std::runtime_error( fmt::format( "Unknown slot '{}'",
        slot_data[ "slot" ][ "type" ].GetString() ) );
    }

    auto& item = p->items[ slot ];

    item.parsed.data.id = slot_data[ "item" ][ "id" ].GetUint();

    if ( slot_data.HasMember( "bonus_list" ) )
    {
      for ( auto bonus_idx = 0u, end = slot_data[ "bonus_list" ].Size(); bonus_idx < end; ++bonus_idx )
      {
        item.parsed.bonus_id.push_back( slot_data[ "bonus_list" ][ bonus_idx ].GetInt() );
      }
    }

    if ( slot_data.HasMember( "sockets" ) )
    {
      for ( auto gem_idx = 0u, end = slot_data[ "sockets" ].Size(); gem_idx < end; ++gem_idx )
      {
        const auto& socket_data = slot_data[ "sockets" ][ gem_idx ];
        if ( !socket_data.HasMember( "item" ) )
        {
          continue;
        }

        if ( !socket_data[ "item" ].HasMember( "id" ) )
        {
          throw std::runtime_error( "Unable to parse socket data: Missing item information" );
        }

        item.parsed.gem_id[ gem_idx ] = socket_data[ "item" ][ "id" ].GetInt();

        if ( socket_data.HasMember( "bonus_list" ) )
        {
          for ( auto gbonus_idx = 0u, end = socket_data[ "bonus_list" ].Size(); gbonus_idx < end; ++gbonus_idx )
          {
            item.parsed.gem_bonus_id[ gem_idx ].push_back( socket_data[ "bonus_list" ][ gbonus_idx ].GetInt() );
          }
        }
      }
    }

    azerite::parse_blizzard_azerite_information( item, slot_data );
  }
}

void parse_media( player_t* p, const std::string& url, cache::behavior_e caching )
{
  rapidjson::Document media_data;
  std::string result;

  if ( ! download( p->sim, media_data, result, p->region_str, url + "&locale=en_US", caching ) )
  {
    throw std::runtime_error(fmt::format("Unable to download media JSON from '{}'.",
        url ));
  }

  if ( p->sim->debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    media_data.Accept( writer );
    p->sim->out_debug.raw() << b.GetString();
  }

  if ( media_data.HasMember( "bust_url" ) )
  {
    p->report_information.thumbnail_url = media_data[ "bust_url" ].GetString();
  }
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
  if ( player.local_json.empty() )
  {
    if ( ! download( sim, profile, result, player.region, player.url, caching ) )
    {
      throw std::runtime_error(fmt::format("Unable to download JSON from '{}'.",
          player.url ));
    }
  }
  else
  {
    io::ifstream ifs;
    ifs.open( player.local_json );
    result.assign( ( std::istreambuf_iterator<char>( ifs ) ),
                   ( std::istreambuf_iterator<char>()    ) );
  }

  profile.Parse< 0 >(result.c_str());

  if ( profile.HasParseError() )
  {
    throw std::runtime_error("Unable to parse JSON." );

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
    throw std::runtime_error(fmt::format("Unavailable/Not-OK status: {}",
                   profile[ "reason" ].GetString() ));
  }

  if ( profile.HasMember( "name" ) )
    player.name = profile[ "name" ].GetString();

  if ( ! profile.HasMember( "level" ) )
  {
    throw std::runtime_error("Unable to extract player level.");
  }

  if ( ! profile.HasMember( "character_class" ) )
  {
    throw std::runtime_error("Unable to extract player class.");
  }

  if ( ! profile.HasMember( "race" ) )
  {
    throw std::runtime_error("Unable to extract player race.");
  }

  if ( ! profile.HasMember( "specializations" ) )
  {
    throw std::runtime_error("Unable to extract player talents.");
  }

  std::string class_name = util::player_type_string( util::translate_class_id( profile[ "character_class" ][ "id" ].GetUint() ) );
  race_e race = util::translate_race_id( profile[ "race" ][ "id" ].GetUint() );
  const module_t* module = module_t::get( class_name );

  if ( ! module || ! module -> valid() )
  {
    throw std::runtime_error(fmt::format("Module for class '{}' is currently not available.", class_name ));
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
    throw std::runtime_error(fmt::format("Unable to build player with class '{}' and name '{}'.",
                   class_name, name ));
  }

  p -> true_level = profile[ "level" ].GetUint();
  p -> region_str = player.region.empty() ? sim -> default_region_str : player.region;
  if ( ! profile.HasMember( "realm" ) && ! player.server.empty() )
    p -> server_str = player.server;
  else
    p -> server_str = profile[ "realm" ][ "name" ].GetString();

  if ( ! player.origin.empty() )
    p -> origin_str = player.origin;

  if ( profile.HasMember( "active_spec" ) && profile[ "active_spec" ].HasMember( "id" ) )
  {
    p->_spec = static_cast<specialization_e>( profile[ "active_spec" ][ "id" ].GetInt() );
  }

  if ( profile.HasMember( "media" ) && profile[ "media" ].HasMember( "href" ) )
  {
    parse_media( p, profile[ "media" ][ "href" ].GetString(), caching );
  }

  // TODO: Professions
  if ( profile.HasMember( "collections" ) )
  {
    //parse_professions( p, profile[ "collections" ][ "href" ].GetString(), caching );
  }

  if ( profile.HasMember( "specializations" ) )
  {
    parse_talents( p, profile[ "specializations" ][ "href" ].GetString(), caching );
  }

  if ( profile.HasMember( "equipment" ) )
  {
    parse_items( p, profile[ "equipment" ][ "href" ].GetString(), caching );
  }

  if ( ! p -> server_str.empty() )
    p -> armory_extensions( p -> region_str, p -> server_str, player.name, caching );

  p->profile_source_ = profile_source::BLIZZARD_API;

  return p;
}

// download_item_data =======================================================

bool download_item_data( item_t& item, cache::behavior_e caching )
{
  rapidjson::Document js;
  if ( ! download_id( item.sim, js, item.player -> region_str, item.parsed.data.id, caching ) ||
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
        item.parsed.data.race_mask |= ( uint64_t(1) << ( js[ "allowableRaces" ][ i ].GetInt() - 1 ) );
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
        item.parsed.stat_val[ i ]         = stat[ "amount" ].GetInt();

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

    // Convert Blizzard item stat values into actual stat allocation percents, so we can do normal
    // stat computation on the item. This presumes that blizzard item data will always give us the
    // correct data for the item (in relation to the item level reported)
    item_database::convert_stat_values( item );
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
  if ( !util::str_compare_ci( region, "cn" ) )
  {
    url = fmt::format( GLOBAL_GUILD_ENDPOINT_URI, region, server, name, LOCALES[ region ].first );
  }
  else
  {
    url = fmt::format( CHINA_GUILD_ENDPOINT_URI, server, name );
  }

  std::string result;
  if ( ! download( sim, d, result, region, url, caching ) )
  {
    return false;
  }

  if ( d.HasParseError() )
  {
    sim -> errorf( "BCP API: Unable to parse guild from '%s': Parse error '%s' @ %lu\n",
      url.c_str(), d.GetParseError(), d.GetErrorOffset() );

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

// bcp_api::translate_api_slot ==============================================

slot_e bcp_api::translate_api_slot( const std::string& slot_str )
{
  static const std::unordered_map<std::string, slot_e> slot_map = {
    { "HEAD",      SLOT_HEAD      },
    { "NECK",      SLOT_NECK      },
    { "SHOULDER",  SLOT_SHOULDERS },
    { "SHIRT",     SLOT_SHIRT     },
    { "CHEST",     SLOT_CHEST     },
    { "WAIST",     SLOT_WAIST     },
    { "LEGS",      SLOT_LEGS      },
    { "FEET",      SLOT_FEET      },
    { "WRIST",     SLOT_WRISTS    },
    { "HANDS",     SLOT_HANDS     },
    { "FINGER_1",  SLOT_FINGER_1  },
    { "FINGER_2",  SLOT_FINGER_2  },
    { "TRINKET_1", SLOT_TRINKET_1 },
    { "TRINKET_2", SLOT_TRINKET_2 },
    { "BACK",      SLOT_BACK      },
    { "MAIN_HAND", SLOT_MAIN_HAND },
    { "OFF_HAND",  SLOT_OFF_HAND  },
    { "TABARD",    SLOT_TABARD    }
  };

  auto it = slot_map.find( slot_str );
  if ( it == slot_map.end() )
  {
    return SLOT_INVALID;
  }

  return it->second;
}

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

  auto normalized_name = name;
  util::tolower( normalized_name );

  if ( !util::str_compare_ci( region, "cn" ) )
  {
    player.url = fmt::format( GLOBAL_PLAYER_ENDPOINT_URI, region, server, normalized_name, region, LOCALES[ region ].first );
    player.origin = fmt::format( GLOBAL_ORIGIN_URI, LOCALES[ region ].second, server, normalized_name );
  }
  else
  {
    player.url = fmt::format( CHINA_PLAYER_ENDPOINT_URI, server, normalized_name );
    player.origin = fmt::format( CHINA_ORIGIN_URI, server, normalized_name );
  }

#ifdef SC_DEFAULT_APIKEY
  if ( sim->apikey == std::string( SC_DEFAULT_APIKEY ) )
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

  for ( size_t i = 0, end = item.parsed.bonus_id.size(); ret && i < end; i++ )
  {
    auto item_bonuses = item.player -> dbc.item_bonus( item.parsed.bonus_id[ i ] );
    // Apply bonuses
    for ( const auto bonus : item_bonuses )
    {
      if ( ! item_database::apply_item_bonus( item, *bonus ) )
        return false;
    }
  }

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

#ifndef SC_NO_NETWORKING
void bcp_api::token_load()
{
  for ( auto& path : token_paths() )
  {
    io::ifstream file;

    file.open( path );
    if ( !file.is_open() )
    {
      continue;
    }

    std::getline( file, token );
    token_path = path;
    break;
  }
}

void bcp_api::token_save()
{
  if ( token.empty() )
  {
    return;
  }

  // Token path is empty, so grab the least-preferred default path for the file
  if ( token_path.empty() )
  {
    token_path = token_paths().back();
  }

  io::ofstream file;
  file.exceptions( std::ios::failbit | std::ios::badbit );

  try
  {
    file.open( token_path );
    if ( !file.is_open() )
    {
      return;
    }

    file << token;
  }
  catch ( const std::exception& )
  {
    std::cerr << "[Warning] Unable to cache Blizzard API bearer token to " << token_path << ": " <<
      strerror(errno) << std::endl;
  }
}
#endif
