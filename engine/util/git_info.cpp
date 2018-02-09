// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "git_info.hpp"

#if defined( SC_GIT_REV ) && defined( SC_GIT_BRANCH )

bool git_info::available()
{
  return true;
}
const char* git_info::revision()
{
  return SC_GIT_REV;
}

const char* git_info::branch()
{
  return SC_GIT_BRANCH;
}

#else

bool git_info::available()
{
  return false;
}
const char* git_info::revision()
{
  return "";
}

const char* git_info::branch()
{
  return "";
}

#endif
