find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_FOUND)
  message("No python3 interpreter found. Not adding any tests")
  return()
endif()

set(SIMC_TEST_RUNNER ${CMAKE_CURRENT_LIST_DIR}/run.py)

set(SIMC_TEST_SPECS Death_Knight_Blood, Death_Knight_Frost, Death_Knight_Unholy,
Demon_Hunter_Havoc, Demon_Hunter_Vengeance,
Druid_Balance, Druid_Feral, Druid_Guardian,
Evoker_Devastation, Evoker_Preservation,
Hunter_Beast_Mastery, Hunter_Marksmanship, Hunter_Survival,
Mage_Arcane, Mage_Fire, Mage_Frost,
Monk_Brewmaster, Monk_Windwalker,
Paladin_Holy, Paladin_Protection, Paladin_Retribution,
Priest_Discipline, Priest_Shadow,
Rogue_Assassination, Rogue_Outlaw, Rogue_Subtlety,
Shaman_Elemental, Shaman_Enhancement, Shaman_Restoration,
Warlock_Affliction, Warlock_Demonology, Warlock_Destruction,
Warrior_Arms, Warrior_Fury, Warrior_Protection,)

set(SIMC_TESTS Trinket)
foreach(SIMC_TEST_SPEC IN LISTS SIMC_TEST_SPECS)
  foreach(SIMC_TEST IN LISTS SIMC_TESTS)
    string(TOLOWER ${SIMC_TEST} SIMC_TEST_LOWER)
    add_test(NAME ${SIMC_TEST}_${SIMC_TEST_SPEC}
      COMMAND ${CMAKE_COMMAND} -E env SIMC_CLI_PATH=$<TARGET_FILE:simc> ${Python_EXECUTABLE} ${SIMC_TEST_RUNNER} ${SIMC_TEST_SPEC} -tests ${SIMC_TEST_LOWER} --max-profiles-to-use 1
    )
  endforeach()
endforeach()