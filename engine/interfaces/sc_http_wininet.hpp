// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_HTTP_WININET_HPP
#define SC_HTTP_WININET_HPP

#ifdef SC_USE_WININET

#include "config.hpp"

#include <windows.h>
#include <WinInet.h>

#include "sc_http.hpp"

namespace http
{
static bool parse_url( const std::string& url_in, URL_COMPONENTSA& url_out );

class wininet_handle_t : public http_handle_t
{
  HINTERNET   m_connection_handle;
  HINTERNET   m_request_handle;
  std::string m_buffer;
  std::unordered_map<std::string, std::string> m_headers;
  std::vector<std::string> m_request_headers;

  HINTERNET create_handle( const std::string& verb, const std::string& url );
  bool read_data();
  bool parse_headers();
  bool set_headers();

public:
  wininet_handle_t();
  wininet_handle_t( HINTERNET connection_handle );
  ~wininet_handle_t();

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
  bool post( const std::string& url, const std::string& data, const std::string& content_type ) override;
};

class wininet_connection_pool_t : public http_connection_pool_t
{
  HINTERNET                                  m_root_handle;
  std::unordered_map<std::string, HINTERNET> m_handle_db;

  void initialize();

public:
  wininet_connection_pool_t();
  ~wininet_connection_pool_t();

  std::unique_ptr<http_handle_t> handle( const std::string& url ) override;
};

} /* Namespace http ends */

#endif /* SC_USE_WININET */

#endif /* SC_HTTP_WININET_HPP */

