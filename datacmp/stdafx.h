// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <stdio.h>
#include <tchar.h>



// To compare two versions:
// 1. put their header files (dbc/data_definitions.hh, dbc/data_enums.hh) and inc
//    files (talent and spell data) in separate directories.
// 2. Change the #ifdef/#define in the header files so they're unique, e.g.
//    for 13117/dbc/data_definitions.hh use DATA_DEFINITIONS_13117_HH instead
// 3. If it's ptr inc files, remove the "ptr_" from the variable names,
//    so they're called __talent_data, __spell_data etc
// 4. Compile and run!

namespace v1 {
#include "13117/dbc/data_definitions.hh"
#include "13117/sc_talent_data.inc"
#include "13117/sc_spell_data.inc"
}

namespace v2 {
#include "13189/dbc/data_definitions.hh"
#include "13189/sc_talent_data_ptr.inc"
#include "13189/sc_spell_data_ptr.inc"
}
