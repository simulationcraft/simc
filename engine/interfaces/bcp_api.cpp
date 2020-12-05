// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "interfaces/bcp_api.hpp"

#include "class_modules/class_module.hpp"
#include "dbc/dbc.hpp"
#include "dbc/item_database.hpp"
#include "interfaces/sc_http_curl.hpp"
#include "interfaces/sc_http_wininet.hpp"
#include "interfaces/sc_http.hpp"
#include "item/item.hpp"
#include "player/azerite_data.hpp"
#include "player/covenant.hpp"
#include "player/sc_player.hpp"
#include "sc_enums.hpp"
#include "sim/sc_sim.hpp"
#include "util/concurrency.hpp"
#include "util/io.hpp"
#include "util/static_map.hpp"
#include "util/string_view.hpp"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "utf8-h/utf8.h"

#include <iostream>

// ==========================================================================
// Blizzard Community Platform API
// ==========================================================================

namespace { // UNNAMED NAMESPACE

// Due to differences in item-level computation between in-game and armory profiles, some
// items require simc to override the item ilevel with the armory import one (instead of
// computing it using the in-game rules)
static constexpr auto __ILEVEL_OVERRIDE_MAP = util::make_static_set<unsigned>( {
  167555U, // Pocket-Sized Computation Device
  158075U, // Heart of Azeroth
} );

struct player_spec_t
{
  std::string region, server, name, url, origin, talent_spec;

  std::string local_json;
  std::string local_json_spec;
  std::string local_json_equipment;
  std::string local_json_media;
  std::string local_json_soulbinds;
};

static constexpr util::string_view GLOBAL_OAUTH_ENDPOINT_URI = "https://{}.battle.net/oauth/token";
static constexpr util::string_view CHINA_OAUTH_ENDPOINT_URI = "https://www.battlenet.com.cn/oauth/token";

static constexpr util::string_view GLOBAL_GUILD_ENDPOINT_URI = "https://{}.api.blizzard.com/wow/guild/{}/{}?fields=members&locale={}";
static constexpr util::string_view CHINA_GUILD_ENDPOINT_URI = "https://gateway.battlenet.com.cn/wow/guild/{}/{}?fields=members&locale={}";

static constexpr util::string_view GLOBAL_PLAYER_ENDPOINT_URI = "https://{}.api.blizzard.com/profile/wow/character/{}/{}?namespace=profile-{}&locale={}";
static constexpr util::string_view CHINA_PLAYER_ENDPOINT_URI = "https://gateway.battlenet.com.cn/profile/wow/character/{}/{}?namespace=profile-cn";

static constexpr util::string_view GLOBAL_ITEM_ENDPOINT_URI = "https://{}.api.blizzard.com/wow/item/{}?locale={}";
static constexpr util::string_view CHINA_ITEM_ENDPOINT_URI = "https://gateway.battlenet.com.cn/wow/item/{}";

static constexpr util::string_view GLOBAL_ORIGIN_URI = "https://worldofwarcraft.com/{}/character/{}/{}";
static constexpr util::string_view CHINA_ORIGIN_URI = "https://www.wowchina.com/zh-cn/character/{}/{}";

static constexpr auto LOCALES_DATA = util::make_static_map<util::string_view, std::array<util::string_view, 2>>( {
  { "us", {{ "en_US", "en-us" }} },
  { "eu", {{ "en_GB", "en-gb" }} },
  { "kr", {{ "ko_KR", "ko-kr" }} },
  { "tw", {{ "zh_TW", "zh-tw" }} }
} );
static constexpr struct {
  const std::array<util::string_view, 2>& operator[](util::string_view str) const {
    static constexpr std::array<util::string_view, 2> deflt{};
    auto it = LOCALES_DATA.find( str );
    return it != LOCALES_DATA.end() ? it->second : deflt;
  };
} LOCALES;

static std::string token_path = "";
static std::string token = "";
static bool authorization_failed = false;
mutex_t token_mutex;

#ifndef SC_NO_NETWORKING

std::vector<std::string> token_paths()
{
  std::vector<std::string> paths;

  paths.emplace_back("./simc-apitoken" );

  if ( const char* home_path = getenv( "HOME" ) )
  {
    paths.push_back( std::string( home_path ) + "/.simc-apitoken" );
  }

  if ( const char* home_drive = getenv( "HOMEDRIVE" ) )
  {
    if ( const char* home_path_ansi = getenv( "HOMEPATH" ) )
    {
      std::string home_path = std::string( home_drive ) + std::string( home_path_ansi ) + "/simc-apitoken";
#ifdef SC_WINDOWS
      home_path = io::ansi_to_utf8( home_path.c_str() );
#endif
      paths.push_back( home_path );
    }
  }

  return paths;
}

// Authorize to the blizzard api
void authorize( sim_t* sim, const std::string& region )
{
  if ( !sim->user_apitoken.empty() )
  {
    return;
  }

  try
  {
    // Authorization needs to be a single threaded process, all threads will re-use the same token
    // once a single thread properly fetches it
    auto_lock_t lock( token_mutex );

    // If an authorization process failed on any thread attempting to perform it, no point trying it
    // again, just fail the rest
    if ( authorization_failed )
    {
      throw std::runtime_error("Previous authorization failed");
    }

    // A token has already been loaded, so don't re-authorize
    if ( !token.empty() )
    {
      return;
    }

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
      oauth_endpoint = std::string( CHINA_OAUTH_ENDPOINT_URI );
    }

    auto pool = http::pool();
    auto handle = pool->handle( oauth_endpoint );

    handle->set_basic_auth( sim->apikey.c_str() );
    auto res = handle->post( oauth_endpoint,
      "grant_type=client_credentials", "application/x-www-form-urlencoded" );

    if ( !res || handle->response_code() != 200 )
    {
      authorization_failed = true;
      throw std::runtime_error(
          fmt::format( "Unable to fetch bearer token from {}, response={}, error={}, result=", oauth_endpoint,
                      handle->response_code(), handle->error(), handle->result() ) );
    }

    rapidjson::Document response;
    response.Parse< 0 >( handle->result() );
    if ( response.HasParseError() )
    {
      authorization_failed = true;
      throw std::runtime_error(
          fmt::format( "Unable to parse response message from {}: {}", oauth_endpoint,
                      rapidjson::GetParseError_En(response.GetParseError())) );
    }

    if ( !response.HasMember( "access_token" ) )
    {
      authorization_failed = true;
      throw std::runtime_error(
          fmt::format( "Malformed JSON object from {}", oauth_endpoint ) );
    }

    token = response[ "access_token" ].GetString();
  }
  catch(const std::exception&)
  {
    std::throw_with_nested(std::runtime_error("Unable to authorize"));
  }
}
#else
void authorize( sim_t*, const std::string& )
{
}
#endif /* SC_NO_NETWORKING */

// download


// Check for errors and return the (HTTP) return code from the status message, if applicable. Return
// value of 0 indicates error.
int check_for_error( const rapidjson::Document& d,
                     std::vector<int>           allowed_codes = {} )
{
  auto ret_code = 200;

  // Old community API NOK status, report Blizzard API reason to the user. Note that there's no way
  // to check for the return codes in the json object response, since they don't exist there
  if ( d.IsObject() && d.HasMember( "status" ) &&
       util::str_compare_ci( d[ "status" ].GetString(), "nok" ) )
  {
    throw std::runtime_error(fmt::format( "Error response from Blizzard API: {}", d[ "reason" ].GetString() ) );
  }

  // New community API status code handling
  if ( d.IsObject() && d.HasMember( "code" ) )
  {
    ret_code = d[ "code" ].GetInt();
    if ( range::find( allowed_codes, ret_code ) == allowed_codes.end() )
    {
      throw std::runtime_error(fmt::format( "Error response {} from Blizzard API: {}", ret_code, d[ "detail" ].GetString() ) );
    }
  }

  return ret_code;
}

// Check if HTTP response code falls on the ranges of successful responses from the Blizzard API
bool check_response_code( int response_code )
{
  // 401 implies reauthentication required, so it's not a valid response
  return ( response_code >= 200 && response_code < 300 ) ||
         ( response_code >= 400 && response_code < 500 && response_code != 401 );
}

void download( sim_t*               sim,
               rapidjson::Document& d,
               const std::string&   region,
               const std::string&   url,
               cache::behavior_e    caching )
{
  std::vector<std::string> headers;
  std::string result;

  authorize( sim, region );

  if ( !sim->user_apitoken.empty() )
  {
    headers.push_back( "Authorization: Bearer " + sim->user_apitoken );
  }
  else
  {
    headers.push_back( "Authorization: Bearer " + token );
  }

  // We can make two attempts at most
  for ( size_t i = 0; i < 2; ++i )
  {
    auto response_code = http::get( result, url, caching, "", headers );
    if ( check_response_code( response_code ) )
    {
      break;
    }

    // Blizzard's issue, lets not bother trying again
    if ( response_code >= 500 && response_code < 600 )
    {
      throw std::runtime_error(fmt::format("Blizzard API responded with internal server error ({}), aborting",
        response_code));
    }
    // Bearer token is invalid, lets try to regenerate if we can
    else if ( response_code == 401 )
    {
      // Loaded token is bogus, so clear it
      token.clear();

      // If there's an user provided apitoken and we get 401, or if we already regenerated
      // our bearer token successfully, there's no point in trying again
      if ( !sim->user_apitoken.empty() )
      {
        throw std::runtime_error(fmt::format("Invalid user 'apitoken' option value '{}'", sim->user_apitoken ) );
      }

      // Re-Authorization failed
      authorize( sim, region );

      // Clear old bearer token from headers and add the new one
      headers.clear();
      headers.push_back( "Authorization: Bearer " + token );
    }
    // Note, not modified is automatically handled by http::get, and translated into 200 OK
    else
    {
      throw std::runtime_error(fmt::format( "Blizzard API responded with an unhandled HTTP response code {}",
        response_code ) );
    }
  }

  d.Parse<0>( result.c_str() );

  // Corrupt data
  if ( !result.empty() && d.HasParseError() )
  {
    throw std::runtime_error(fmt::format( "Malformed response from Blizzard API" ) );
  }

  if ( sim->debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    d.Accept( writer );
    sim->out_debug.raw() << b.GetString();
  }
}

// download_item ==============================================================

void download_item( sim_t* sim,
                    rapidjson::Document& d,
                    const std::string& region,
                    unsigned item_id,
                    cache::behavior_e caching )
{
  if ( item_id == 0 )
  {
    throw std::runtime_error("Invalid item id '0'.");
  }

  std::string url;

  if ( !util::str_compare_ci( region, "cn" ) )
  {
    url = fmt::format( GLOBAL_ITEM_ENDPOINT_URI, region, item_id, LOCALES[ region ][ 0 ] );
  }
  else
  {
    url = fmt::format( CHINA_ITEM_ENDPOINT_URI, item_id );
  }

  download( sim, d, region, url, caching );

  check_for_error( d );
}


void parse_file( sim_t* sim, const std::string& path, rapidjson::Document& d )
{
  std::string result;
  io::ifstream ifs;

  ifs.open( path );
  result.assign( std::istreambuf_iterator<char>( ifs ), std::istreambuf_iterator<char>() );

  d.Parse<0>( result.c_str() );

  // Corrupt data
  if ( ! result.empty() && d.HasParseError() )
  {
    throw std::runtime_error(fmt::format( "Malformed data in '{}'", path ) );
  }

  if ( sim->debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    d.Accept( writer );
    sim->out_debug.raw() << b.GetString();
  }
}

// parse_talents ============================================================

void parse_talents( player_t* p, const player_spec_t& spec_info, const std::string& url, cache::behavior_e caching )
{
  rapidjson::Document spec;

  if ( spec_info.local_json.empty() && spec_info.local_json_spec.empty() )
  {
    try
    {
      download( p->sim, spec, p->region_str, url + "&locale=en_US", caching );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested(std::runtime_error(fmt::format("Unable to download talent JSON from '{}'.",
          url )));
    }
  }
  else if ( !spec_info.local_json_spec.empty() )
  {
    try
    {
      parse_file( p->sim, spec_info.local_json_spec, spec );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested( std::runtime_error( fmt::format( "Unable to parse local JSON from '{}'.",
        spec_info.local_json_spec ) ) );
    }
  }

  if ( !spec.IsObject() )
  {
    return;
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
      const auto talent = p->dbc->talent( talent_id );
      if ( talent->id() != talent_id )
      {
        p->sim->error( "Warning: Unable to find talent id {} for {} from Simulationcraft client data",
          talent_id, p->name() );
        continue;
      }

      p->talent_points.select_row_col( talent->row(), talent->col() );
    }
  }

  p->recreate_talent_str(talent_format::ARMORY );
}

// parse_items ==============================================================

void parse_items( player_t* p, const player_spec_t& spec, const std::string& url, cache::behavior_e caching )
{
  rapidjson::Document equipment_data;

  if ( spec.local_json.empty() && spec.local_json_equipment.empty() )
  {
    try
    {
      download( p->sim, equipment_data, p->region_str, url + "&locale=en_US", caching );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested( std::runtime_error(fmt::format("Unable to download equipment JSON from '{}'.",
          url )) );
    }
  }
  else if ( !spec.local_json_equipment.empty() )
  {
    try
    {
      parse_file( p->sim, spec.local_json_equipment, equipment_data );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested(  std::runtime_error( fmt::format( "Unable to parse equipment JSON from '{}'.",
          spec.local_json_equipment )) );
    }
  }

  if ( !equipment_data.IsObject() || !equipment_data.HasMember( "equipped_items" ) )
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

    if ( slot_data.HasMember( "timewalker_level" ) )
    {
      item.parsed.drop_level = slot_data[ "timewalker_level" ].GetUint();
    }

    if ( slot_data.HasMember( "bonus_list" ) )
    {
      for ( auto bonus_idx = 0u, end = slot_data[ "bonus_list" ].Size(); bonus_idx < end; ++bonus_idx )
      {
        item.parsed.bonus_id.push_back( slot_data[ "bonus_list" ][ bonus_idx ].GetInt() );
      }
    }

    if ( slot_data.HasMember( "enchantments" ) )
    {
      for ( auto ench_idx = 0u, end = slot_data[ "enchantments" ].Size(); ench_idx < end; ++ench_idx )
      {
        const auto& ench_data = slot_data[ "enchantments" ][ ench_idx ];

        if ( !ench_data.HasMember( "enchantment_id" ) )
        {
          throw std::runtime_error( "Unable to parse enchant data: Missing enchantment ID" );
        }
        if ( !ench_data.HasMember( "enchantment_slot" ) )
        {
          throw std::runtime_error( "Unable to parse enchant data: Missing enchantment slot data" );
        }

        switch (ench_data[ "enchantment_slot" ][ "id" ].GetInt()) {
          // PERMANENT
          case 0:
            item.parsed.enchant_id = ench_data[ "enchantment_id" ].GetInt();
            break;
          // BONUS_SOCKETS
          case 6:
            break;
          // ON_USE_SPELL
          case 7:
            item.parsed.addon_id = ench_data[ "enchantment_id" ].GetInt();
            break;
        }
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

    if ( slot_data.HasMember( "modified_crafting_stat" ) )
    {
      for ( auto idx = 0u, end = slot_data[ "modified_crafting_stat" ].Size(); idx < end; ++idx )
      {
        const auto& stat_data = slot_data[ "modified_crafting_stat" ][ idx ];
        item.parsed.crafted_stat_mod.push_back( stat_data[ "id" ].GetInt() );
      }
    }

    azerite::parse_blizzard_azerite_information( item, slot_data );

    auto it = __ILEVEL_OVERRIDE_MAP.find( item.parsed.data.id );
    if ( it != __ILEVEL_OVERRIDE_MAP.end() )
    {
      item.option_ilevel_str =  util::to_string( slot_data[ "level" ][ "value" ].GetUint() );
    }
  }
}

void parse_soulbinds( player_t*            p,
                      const player_spec_t& spec,
                      const rapidjson::Value&   covenant_info,
                      cache::behavior_e    caching )
{
  rapidjson::Document soulbinds_data;

  if ( spec.local_json.empty() && spec.local_json_soulbinds.empty() )
  {
    std::string url;

    if ( !covenant_info.HasMember( "soulbinds" ) )
    {
      return;
    }

    url = covenant_info[ "soulbinds" ][ "href" ].GetString();

    try
    {
      download( p->sim, soulbinds_data, p->region_str, url + "&locale=en_US", caching );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested(std::runtime_error(fmt::format("Unable to download soulbinds JSON from '{}'.", url )));
    }
  }
  else if ( !spec.local_json_soulbinds.empty() )
  {
    try
    {
      parse_file( p->sim, spec.local_json_soulbinds, soulbinds_data );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested( std::runtime_error( fmt::format( "Unable to parse soulbinds information JSON from '{}'.",
        spec.local_json_soulbinds ) ) );
    }
  }

  covenant::parse_blizzard_covenant_information( p, soulbinds_data );
}

void parse_media( player_t*            p,
                  const player_spec_t& spec,
                  const std::string&   url,
                  cache::behavior_e    caching )
{
  rapidjson::Document media_data;

  if ( spec.local_json.empty() && spec.local_json_media.empty() )
  {
    try
    {
      download( p->sim, media_data, p->region_str, url + "&locale=en_US", caching );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested(std::runtime_error(fmt::format("Unable to download media JSON from '{}'.", url )));
    }
  }
  else if ( !spec.local_json_media.empty() )
  {
    try
    {
      parse_file( p->sim, spec.local_json_media, media_data );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested( std::runtime_error( fmt::format( "Unable to parse media information JSON from '{}'.",
        spec.local_json_media ) ) );
    }
  }

  if ( !media_data.IsObject() )
  {
    return;
  }

  if ( media_data.HasMember( "bust_url" ) )
  {
    p->report_information.thumbnail_url = media_data[ "bust_url" ].GetString();
  }
}

// parse_player =============================================================

player_t* parse_player( sim_t*            sim,
                        player_spec_t&    player,
                        cache::behavior_e caching,
                        bool              allow_failures = false )
{
  sim -> current_slot = 0;

  rapidjson::Document profile;

  // China does not have mashery endpoints, so no point in even trying to get anything here
  if ( player.local_json.empty() )
  {
    try
    {
      download( sim, profile, player.region, player.url + "&locale=en_US", caching );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested( std::runtime_error(fmt::format("Unable to download JSON from '{}'.",
          player.url )) );
    }
  }
  else
  {
    try
    {
      parse_file( sim, player.local_json, profile );
    }
    catch(const std::exception&)
    {
      std::throw_with_nested( std::runtime_error( fmt::format( "Unable to parse JSON from '{}'.",
        player.local_json ) ) );
    }
  }

  if ( !allow_failures && !check_for_error( profile ) )
  {
    throw std::runtime_error(fmt::format("Unable to download JSON from '{}'.",
        player.url ));
  }
  // 200, 403, 404 results are OK, anything else not OK
  else if ( allow_failures )
  {
    auto ret_code = check_for_error( profile, {403, 404} );
    if ( !ret_code )
    {
      throw std::runtime_error(fmt::format("Unable to download JSON from '{}'.",
          player.url ));
    }
    else if ( ret_code == 403 || ret_code == 404 )
    {
      return nullptr;
    }
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
  
  auto player_type = util::parse_player_type( class_name );
  if (player_type == PLAYER_NONE)
  {
    throw std::runtime_error(fmt::format("No class module could be found for '{}'.", class_name ));
  }
  const module_t* module = module_t::get( player_type );

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
    parse_media( p, player, profile[ "media" ][ "href" ].GetString(), caching );
  }

  if ( profile.HasMember( "specializations" ) )
  {
    parse_talents( p, player, profile[ "specializations" ][ "href" ].GetString(), caching );
  }

  if ( profile.HasMember( "equipment" ) )
  {
    parse_items( p, player, profile[ "equipment" ][ "href" ].GetString(), caching );
  }

  if ( profile.HasMember( "covenant_progress" ) )
  {
    parse_soulbinds( p, player, profile[ "covenant_progress" ], caching );
  }

  if ( ! p -> server_str.empty() )
    p -> armory_extensions( p -> region_str, p -> server_str, player.name, caching );

  p->profile_source_ = profile_source::BLIZZARD_API;

  return p;
}

// download_item_data =======================================================

void download_item_data( item_t& item, cache::behavior_e caching )
{
  rapidjson::Document js;
  download_item( item.sim, js, item.player -> region_str, item.parsed.data.id, caching );
  if ( js.HasParseError() )
  {
    throw std::runtime_error(fmt::format("Item JSON data has parse error: {}", rapidjson::GetParseError_En(js.GetParseError())));
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

      item.parsed.data.delay = weaponInfo[ "weaponSpeed" ].GetFloat() * 1000.0f;
      item.parsed.data.dmg_range = 2 - 2 * damage[ "exactMin" ].GetFloat() / ( weaponInfo[ "dps" ].GetFloat() * weaponInfo[ "weaponSpeed" ].GetFloat() );
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

    for (rapidjson::SizeType i = 0, n = as<rapidjson::SizeType>( std::min(static_cast< size_t >(sockets.Size()), range::size(item.parsed.data.socket_color))); i < n; ++i)
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
      for ( rapidjson::SizeType i = 0, n = spells.Size(); i < n; ++i )
      {
        const rapidjson::Value& spell = spells[ i ];
        if ( ! spell.HasMember( "spellId" ) || ! spell.HasMember( "trigger" ) )
          continue;

        unsigned spell_id = spell[ "spellId" ].GetUint();
        int trigger_type = -1;

        if ( util::str_compare_ci( spell[ "trigger" ].GetString(), "ON_EQUIP" ) )
          trigger_type = ITEM_SPELLTRIGGER_ON_EQUIP;
        else if ( util::str_compare_ci( spell[ "trigger" ].GetString(), "ON_USE" ) )
          trigger_type = ITEM_SPELLTRIGGER_ON_USE;
        else if ( util::str_compare_ci( spell[ "trigger" ].GetString(), "ON_PROC" ) )
          trigger_type = ITEM_SPELLTRIGGER_CHANCE_ON_HIT;

        if ( trigger_type != -1 && spell_id > 0 )
        {
          item.parsed.data.add_effect( spell_id, trigger_type );
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

    throw std::runtime_error(fmt::format("Cannot parse field '{}': {}", fieldname, error_str));
  }
}

// download_roster ==========================================================

void download_roster( rapidjson::Document& d,
                      sim_t* sim,
                      const std::string& region,
                      const std::string& server,
                      const std::string& name,
                      cache::behavior_e  caching )
{
  std::string url;
  if ( !util::str_compare_ci( region, "cn" ) )
  {
    url = fmt::format( GLOBAL_GUILD_ENDPOINT_URI, region, server, name, LOCALES[ region ][ 0 ] );
  }
  else
  {
    url = fmt::format( CHINA_GUILD_ENDPOINT_URI, server, name );
  }

  download( sim, d, region, url, caching );

  if ( d.HasParseError() )
  {
    throw std::runtime_error(fmt::format("BCP API: Unable to parse guild from '{}': Parse error '{}' @ {}",
    url, d.GetParseError(), d.GetErrorOffset() ) );
  }

  if ( sim -> debug )
  {
    rapidjson::StringBuffer b;
    rapidjson::PrettyWriter< rapidjson::StringBuffer > writer( b );

    d.Accept( writer );
    sim -> out_debug.raw() << b.GetString();
  }
}

} // close anonymous namespace ==============================================

// bcp_api::translate_api_slot ==============================================

slot_e bcp_api::translate_api_slot( const std::string& slot_str )
{
  using record_t = std::pair<util::string_view, slot_e>;
  static constexpr record_t slot_map[] = {
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

  auto it = range::find( slot_map, slot_str, &record_t::first );
  if ( it == range::end( slot_map ) )
  {
    return SLOT_INVALID;
  }

  return it->second;
}

// bcp_api::download_player =================================================

player_t* bcp_api::download_player(sim_t* sim,
  const std::string& region,
  const std::string& server,
  const std::string& name,
  const std::string& talents,
  cache::behavior_e  caching,
  bool               allow_failures)
{
  sim->current_name = name;

  player_spec_t player;

  std::vector<std::string::value_type> chars{ name.begin(), name.end() };
  chars.push_back(0);

  utf8lwr(reinterpret_cast<void*>(chars.data()));

  auto normalized_name = std::string{ chars.begin(), chars.end() };
  util::urlencode(normalized_name);

  auto normalized_server = server;
  util::tolower(normalized_server);

  if (!util::str_compare_ci(region, "cn"))
  {
    player.url = fmt::format(GLOBAL_PLAYER_ENDPOINT_URI, region, normalized_server, normalized_name, region, LOCALES[region][0]);
    player.origin = fmt::format(GLOBAL_ORIGIN_URI, LOCALES[region][1], normalized_server, normalized_name);
  }
  else
  {
    player.url = fmt::format(CHINA_PLAYER_ENDPOINT_URI, normalized_server, normalized_name);
    player.origin = fmt::format(CHINA_ORIGIN_URI, normalized_server, normalized_name);
  }

#ifdef SC_DEFAULT_APIKEY
  if (sim->apikey == std::string(SC_DEFAULT_APIKEY))
  {
    //This is needed to prevent hitting the 'per second' api call limit.
    // If the character is cached, it still counts as a api use, even though we don't download anything.
    // With cached characters, it's common for 30-40 calls to be made per second when downloading a guild.
    // This is only enabled when the person is using the default apikey.
    sc_thread_t::sleep_seconds(0.25);
  }
#endif

  player.region = region;
  player.server = server;
  player.name = name;
  player.talent_spec = talents;

  return parse_player( sim, player, caching, allow_failures );
}

// bcp_api::from_local_json =================================================

player_t* bcp_api::from_local_json( sim_t*             sim,
                                    const std::string& name,
                                    const std::string& value,
                                    const std::string& talents
                                  )
{
  player_spec_t player_spec;

  player_spec.origin = value;

  std::array<std::unique_ptr<option_t>, 4> options { {
    opt_string( "spec", player_spec.local_json_spec ),
    opt_string( "equipment", player_spec.local_json_equipment ),
    opt_string( "media", player_spec.local_json_media ),
    opt_string( "soulbinds", player_spec.local_json_soulbinds ),
  } };

  std::string options_str;
  auto first_comma = value.find( ',' );
  if ( first_comma != std::string::npos )
  {
    player_spec.local_json = value.substr( 0, first_comma );
    options_str = value.substr( first_comma + 1 );
  }
  else
  {
    player_spec.local_json = value;
  }

  try
  {
    opts::parse( sim, "local_json", options, options_str,
      [ sim ]( opts::parse_status status, util::string_view name, util::string_view value ) {
        // Fail parsing if strict parsing is used and the option is not found
        if ( sim->strict_parsing && status == opts::parse_status::NOT_FOUND )
        {
          return opts::parse_status::FAILURE;
        }

        // .. otherwise, just warn that there's an unknown option
        if ( status == opts::parse_status::NOT_FOUND )
        {
          sim->error( "Warning: Unknown local_json option '{}' with value '{}', ignoring",
            name, value );
        }

        return status;
      } );
  }
  catch ( const std::exception& )
  {
    std::throw_with_nested( std::invalid_argument(
      fmt::format( "Cannot parse option from '{}'", options_str ) ) );
  }

  player_spec.name = name;
  player_spec.talent_spec = talents;

  sim->current_name = name;

  return parse_player( sim, player_spec, cache::ANY );
}

// bcp_api::download_item() =================================================

bool bcp_api::download_item( item_t& item, cache::behavior_e caching )
{
  try
  {
    download_item_data( item, caching );
    item.source_str = "Blizzard";

    for ( const auto& id : item.parsed.bonus_id)
    {
      auto item_bonuses = item.player->dbc->item_bonus( id );
      // Apply bonuses
      for ( const auto& bonus : item_bonuses )
      {
        if ( ! item_database::apply_item_bonus( item, bonus ) )
          return false;
      }
    }

    return true;
  }
  catch(const std::exception&)
  {
    std::throw_with_nested(std::runtime_error("Error retrieving item from BCP API"));
  }
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

  std::vector<std::string::value_type> chars { name.begin(), name.end() };
  chars.push_back( 0 );

  utf8lwr( reinterpret_cast<void*>( chars.data() ) );

  auto normalized_name = std::string { chars.begin(), chars.end() };
  util::urlencode( normalized_name );

  auto normalized_server = server;
  util::tolower( normalized_server );

  download_roster( js, sim, region, normalized_server, normalized_name, caching );

  if ( !check_for_error( js ) )
  {
    return false;
  }

  if ( ! js.HasMember( "members" ) )
  {
    throw std::runtime_error(fmt::format( "No members in guild {},{},{}", region, server, name ) );
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

    names.emplace_back(character[ "name" ].GetString() );
  }

  if ( names.empty() ) return true;

  range::sort( names );

  for (auto & cname : names)
  {
    fmt::print("Downloading character: {}\n", cname);
    download_player( sim, region, server, cname, "active", caching, true );
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
