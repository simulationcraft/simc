function sim() {
  cd "${SIMC_PROFILES_PATH}"
  run timeout 5m "${SIMC_CLI_PATH}" "${SIMC_PROFILE}" iterations="${SIMC_ITERATIONS}" $@
  cd -
  BATS_TEST_DESCRIPTION+=" ($(basename ${SIMC_PROFILE}) iterations=${SIMC_ITERATIONS} $@)"
}

