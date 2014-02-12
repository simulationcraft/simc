load test_helper

@test "simulation style Patchwerk" {
  sim fight_style=Patchwerk
  [ "${status}" -eq 0 ]
}

@test "simulation style LightMovement" {
  sim fight_style=LightMovement
  [ "${status}" -eq 0 ]
}

@test "simulation style HeavyMovement" {
  sim fight_style=HeavyMovement
  [ "${status}" -eq 0 ]
}

@test "simulation style HecticAddCleave" {
  sim fight_style=HecticAddCleave
  [ "${status}" -eq 0 ]
}

@test "simulation style HelterSkelter" {
  sim fight_style=HelterSkelter
  [ "${status}" -eq 0 ]
}
