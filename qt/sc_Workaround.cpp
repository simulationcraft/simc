// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "sc_Workaround.hpp"

#if defined( SC_WINDOWS )
#include <locale>
#include <codecvt>

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>


namespace
{
void __unload_module( MODULEENTRY32* module, sim_t* /* sim */ )
{
  FreeLibrary( module->hModule );
}

using dll_workaround_entry_t = std::pair<std::string, std::function<void( MODULEENTRY32*, sim_t* )>>;

static const std::vector<dll_workaround_entry_t> __dll_workarounds = {
  { "LavasoftTcpService", __unload_module },
};

std::string convert_tchar( const TCHAR* carr )
{
#ifdef UNICODE
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  return converter.to_bytes( carr );
#else
  return std::string( carr );
#endif
}

void win32_dll_workarounds( sim_t* sim )
{
  HANDLE processHandle = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, 0 );
  if ( processHandle == INVALID_HANDLE_VALUE )
  {
    return;
  }

  MODULEENTRY32 module;
  module.dwSize = sizeof( MODULEENTRY32 );
  if ( ! Module32First( processHandle, &module ) )
  {
    CloseHandle( processHandle );
    return;
  }

  do
  {
    std::string module_name = convert_tchar( module.szModule );
    auto it = range::find_if( __dll_workarounds, [ &module_name ]( const dll_workaround_entry_t& entry ) {
      return util::str_in_str_ci( module_name, entry.first );
    } );

    if ( it != __dll_workarounds.end() )
    {
      it->second( &module, sim );
    }
  } while ( Module32Next( processHandle, &module ) );

  CloseHandle( processHandle );
}
}
#endif

void workaround::apply_workarounds( sim_t* sim )
{
#if defined( SC_WINDOWS )
  win32_dll_workarounds( sim );
#endif
}
