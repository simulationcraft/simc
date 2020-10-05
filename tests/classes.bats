load test_helper

@test "Talent test" {
	talent_sim ${SIMC_CLASS}
}

@test "Trinkets test" {
	trinket_sim ${SIMC_CLASS}
}

@test "Kyrian" {
	covenant_sim "${SIMC_CLASS}" kyrian
}

@test "Venthyr" {
	covenant_sim "${SIMC_CLASS}" venthyr
}

@test "Night Fae" {
	covenant_sim "${SIMC_CLASS}" night_fae
}

@test "Necrolord" {
	covenant_sim "${SIMC_CLASS}" necrolord
}
