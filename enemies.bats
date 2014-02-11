load test_helper

@test "simulation against HelterSkelter" {
  sim fight_style=HelterSkelter
  echo $status
  echo $output
  [ "${status}" -eq 0 ]
}
