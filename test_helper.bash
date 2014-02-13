function sim() {
  PROFILE_NAMES+=" $(basename ${SIMC_PROFILE})"
  OPTIONS="$@"
  cd "${SIMC_PROFILES_PATH}"
  run ${SIMC_TIMEOUT} 5m "${SIMC_CLI_PATH}" "${SIMC_PROFILE}" iterations="${SIMC_ITERATIONS}" $@
  cd -
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

teardown() {
  BATS_TEST_DESCRIPTION="<strong>${BATS_TEST_DESCRIPTION}</strong>"
  BATS_TEST_DESCRIPTION+="<br/>Profiles: $(echo "${PROFILE_NAMES}" | sed -e 's/^ *//g' -e 's/ *$//g')"
  if [ ! -z "${OPTIONS}" ]; then
    BATS_TEST_DESCRIPTION+="<br/>Options: $(echo "${OPTIONS}" | sed -e 's/^ *//g' -e 's/ *$//g')"
  fi
}
