#include "class_modules/apl/druid/druid.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace druid_apl
{
void apl_balance( player_t* p )
{
#include "class_modules/apl/druid/balance_apl.inc"
}

void apl_balance_ptr( player_t* p )
{
#include "class_modules/apl/druid/balance_apl_ptr.inc"
}

void apl_feral( player_t* p )
{
#include "class_modules/apl/druid/feral_apl.inc"
}

void apl_feral_ptr( player_t* p )
{
#include "class_modules/apl/druid/feral_apl_ptr.inc"
}

void apl_guardian( player_t* p )
{
#include "class_modules/apl/druid/guardian_apl.inc"
}

void apl_guardian_ptr( player_t* p )
{
#include "class_modules/apl/druid/guardian_apl_ptr.inc"
}

void apl_restoration( player_t* p )
{
#include "class_modules/apl/druid/restoration_druid_apl.inc"
}

void apl_restoration_ptr( player_t* p )
{
#include "class_modules/apl/druid/restoration_druid_apl_ptr.inc"
}
}  // namespace druid_apl
