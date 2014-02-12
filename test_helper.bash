function sim() {
  cd "${SIMC_PROFILES_PATH}"
#  run timeout 5m "${SIMC_CLI_PATH}" "${SIMC_PROFILE}" iterations="${SIMC_ITERATIONS}" $@
  run "${SIMC_CLI_PATH}" "${SIMC_PROFILE}" iterations="${SIMC_ITERATIONS}" $@
  cd -
  BATS_TEST_DESCRIPTION+=" ($(basename ${SIMC_PROFILE}) iterations=${SIMC_ITERATIONS} $@)"
}

function class_sim() {
  PROFILE_DIR=$(dirname "${SIMC_PROFILE}")
  PROFILES=$(ls "${PROFILE_DIR}"/$1*.simc)
  for spec in $PROFILES; do
    SIMC_PROFILE=$spec
    sim default_actions=1
    [ "${status}" -eq 0 ]
  done
}

