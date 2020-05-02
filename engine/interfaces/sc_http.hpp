// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_HTTP_HPP
#define SC_HTTP_HPP

#include <string>
#include <vector>
#include <unordered_map>

#include "util/cache.hpp"

struct sim_t;

namespace http
{
struct proxy_t
{
  std::string type;
  std::string host;
  int port;
};

class http_handle_t
{
public:
  http_handle_t();
  virtual ~http_handle_t();

  virtual bool initialized() const = 0;
  virtual std::string error() const = 0;
  virtual int response_code() const = 0;
  virtual const std::string& result() const = 0;
  virtual const std::unordered_map<std::string, std::string>& headers() const = 0;

  virtual void add_request_headers( const std::vector<std::string>& headers ) = 0;
  virtual void add_request_header( const std::string& header ) = 0;
  virtual bool set_basic_auth( const std::string& userpwd ) = 0;
  virtual bool set_basic_auth( const std::string& user, const std::string& pwd ) = 0;

  virtual bool get( const std::string& url ) = 0;
  virtual bool post( const std::string& url, const std::string& data, const std::string& content_type ) = 0;
};

class http_connection_pool_t
{
public:
  http_connection_pool_t();
  virtual ~http_connection_pool_t();

  virtual std::unique_ptr<http_handle_t> handle( const std::string& url ) = 0;
};

void set_proxy( const std::string& type, const std::string& host, const unsigned port );
const proxy_t& get_proxy();

void cache_load( const std::string& file_name );
void cache_save( const std::string& file_name );
bool clear_cache( sim_t*, const std::string& name, const std::string& value );

std::tuple<std::string, std::string> normalize_header( const std::string& );

int get( std::string& result, const std::string& url,
         cache::behavior_e caching, const std::string& confirmation = "",
         const std::vector<std::string>& headers = {} );

http_connection_pool_t* pool();
} /* Namespace http ends */
#endif
