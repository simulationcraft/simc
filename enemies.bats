load test_helper

@test "simulation against HelterSkelter" {
  sim fight_style=HelterSkelter
  [ "${status}" -eq 0 ]
}
