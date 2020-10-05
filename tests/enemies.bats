load test_helper

@test "Simulation style HeavyMovement" {
  sim threads=2 fight_style=HeavyMovement
  [ "${status}" -eq 0 ]
}

@test "Simulation style HecticAddCleave" {
  sim threads=2 fight_style=HecticAddCleave
  [ "${status}" -eq 0 ]
}

@test "Simulation style HelterSkelter" {
  sim threads=2 fight_style=HelterSkelter
  [ "${status}" -eq 0 ]
}

@test "Test multiple (4) enemies" {
  sim threads=2 enemy=Enemy1 enemy=Enemy2 enemy=Enemy3 enemy=Enemy4
  [ "${status}" -eq 0 ]
}

@test "Test Beastlord style encounter" {
  sim threads=2 fight_style=Beastlord
  [ "${status}" -eq 0 ]
}

