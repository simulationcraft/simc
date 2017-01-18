function sim() {
  PROFILE_NAME="$(basename ${SIMC_PROFILE})"
  OPTIONS="$@"
  cd "${SIMC_PROFILES_PATH}"
#  run ${SIMC_TIMEOUT} 300 "${SIMC_CLI_PATH}" "${SIMC_PROFILE}" iterations=${SIMC_ITERATIONS} output=/dev/null $@
  run "${SIMC_CLI_PATH}" "${SIMC_PROFILE}" iterations=${SIMC_ITERATIONS} $@
  cd -
}

function class_sim() {
  PROFILE_DIR=$(dirname "${SIMC_PROFILE}")
  PROFILES=$(ls "${PROFILE_DIR}"/$1_*_*.simc)
  PROFILE_TALENTS=$("${BATS_TEST_DIRNAME}"/talent_options $1)
  for spec in $PROFILES; do
    SIMC_PROFILE=$spec
    sim default_actions=1
    [ "${status}" -eq 0 ]
  done
}

function class_sim2() {
  PROFILE_DIR=$(dirname "${SIMC_PROFILE}")
  PROFILES=( $(ls "${PROFILE_DIR}"/$1_*_*.simc) )
  PROFILE_TALENTS=( $("${BATS_TEST_DIRNAME}"/talent_options $1) )
  for spec in ${PROFILES[@]}; do
    SIMC_PROFILE=${spec}
    for talent in ${PROFILE_TALENTS[@]}; do
      sim talents=${talent} threads=2 default_actions=1
      if [ ! "${status}" -eq 0 ]; then
        echo "Error in sim: "${SIMC_CLI_PATH}" "${SIMC_PROFILE}" iterations=${SIMC_ITERATIONS} talents=${talent} threads=2 default_actions=1"
        echo "=== Output begins here ==="
        echo "${output}"
        echo "=== Output ends here ==="
      fi 
      [ "${status}" -eq 0 ]
    done
  done
}

teardown() {
  if [ ! "${status}" -eq 0 ]; then
    BATS_TEST_DESCRIPTION="<strong>${BATS_TEST_DESCRIPTION}</strong>"
    BATS_TEST_DESCRIPTION+="<br/>Profile: $(echo "${PROFILE_NAME}" | sed -e 's/^ *//g' -e 's/ *$//g')"
    if [ ! -z "${OPTIONS}" ]; then
      BATS_TEST_DESCRIPTION+="<br/>Options: $(echo "${OPTIONS}" | sed -e 's/^ *//g' -e 's/ *$//g')"
    fi

    if [ "${#lines[@]}" -gt 0 ]; then
      BATS_TEST_DESCRIPTION+="<br/>Test output:"
      for line in ${lines[@]}; do
        BATS_TEST_DESCRIPTION+="<br/>${line}"
      done
    fi
  fi
}
