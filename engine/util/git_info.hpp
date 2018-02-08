// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

namespace git_info {
  /// True if git revision information is available
  bool available();
  /// git revision in short form
  const char* revision();
  /// current git branch
  const char* branch();
}
