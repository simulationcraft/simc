// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_HTTP_CURL_HPP
#define SC_HTTP_CURL_HPP

#ifdef SC_USE_CURL

#include <curl/curl.h>

#include "sc_http.hpp"

namespace http
{

class curl_handle_t : public http_handle_t
{
  CURL*                m_handle;
  curl_slist*          m_header_opts;

  std::string          m_buffer;
  std::unordered_map<std::string, std::string> m_headers;
  char                 m_error[ CURL_ERROR_SIZE ];

  void set_proxy();

  size_t read_data( const char* data_in, size_t total_bytes );
  size_t add_response_header( const char* header_str, size_t total_bytes );

  static size_t data_cb( void* contents, size_t size, size_t nmemb, void* usr );
  static size_t header_cb( char* contents, size_t size, size_t nmemb, void* usr );

public:
  curl_handle_t();
  curl_handle_t( CURL* handle );
  ~curl_handle_t();

  bool initialized() const override;
  std::string error() const override;
  int response_code() const override;
  const std::string& result() const override;
  const std::unordered_map<std::string, std::string>& headers() const override;

  void add_request_headers( const std::vector<std::string>& headers ) override;
  void add_request_header( const std::string& header ) override;
  bool set_basic_auth( const std::string& userpwd ) override;
  bool set_basic_auth( const std::string& user, const std::string& pwd ) override;

  bool get( const std::string& url ) override;
  bool post( const std::string& url, const std::string& data, const std::string& content_type = std::string() ) override;
};

class curl_connection_pool_t : public http_connection_pool_t
{
  CURL* m_handle;

public:
  curl_connection_pool_t();
  ~curl_connection_pool_t();

  std::unique_ptr<http_handle_t> handle( const std::string& url ) override;
};

} /* Namespace http ends */

#endif /* SC_USE_CURL */

#endif /* SC_HTTP_CURL_HPP */

