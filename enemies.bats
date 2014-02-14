load test_helper

@test "Simulation style HeavyMovement" {
  sim fight_style=HeavyMovement
  [ "${status}" -eq 0 ]
}

@test "Simulation style HecticAddCleave" {
  sim fight_style=HecticAddCleave
  [ "${status}" -eq 0 ]
}

@test "Simulation style HelterSkelter" {
  sim fight_style=HelterSkelter
  [ "${status}" -eq 0 ]
}

@test "Test multiple (4) enemies" {
  sim enemy=Enemy1 enemy=Enemy2 enemy=Enemy3 enemy=Enemy4
  [ "${status}" -eq 0 ]
}

