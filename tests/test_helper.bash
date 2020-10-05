function sim() {
  PROFILE_NAME="$(basename ${SIMC_PROFILE})"
  OPTIONS="$@"
  run "${SIMC_CLI_PATH}" "${SIMC_PROFILE}" iterations=${SIMC_ITERATIONS} threads=${SIMC_THREADS} fight_style=${SIMC_FIGHT_STYLE} output=/dev/null cleanup_threads=1 $@
  if [ ! "${status}" -eq 0 ]; then
    echo "Error in sim: "${SIMC_CLI_PATH}" "${SIMC_PROFILE}" iterations=${SIMC_ITERATIONS} threads=${SIMC_THREADS} fight_style=${SIMC_FIGHT_STYLE} output=/dev/null cleanup_threads=1 ${OPTIONS}"
    echo "=== Output begins here ==="
    echo "${output}"
    echo "=== Output ends here ==="
  fi 
}

function class_sim() {
  PROFILE_DIR=${SIMC_PROFILE_DIR}
  PROFILES=$(ls "${PROFILE_DIR}"/*_$1_*.simc)
  PROFILE_TALENTS=$("${BATS_TEST_DIRNAME}"/talent_options $1)
  for spec in $PROFILES; do
    SIMC_PROFILE=$spec
    sim default_actions=1
    [ "${status}" -eq 0 ]
  done
}

function talent_sim() {
  PROFILE_DIR=${SIMC_PROFILE_DIR}
  PROFILES=( $(ls "${PROFILE_DIR}"/*_$1_*.simc) )
  PROFILE_TALENTS=( $("${BATS_TEST_DIRNAME}"/talent_options $1) )
  for spec in ${PROFILES[@]}; do
    SIMC_PROFILE=${spec}
    for talent in ${PROFILE_TALENTS[@]}; do
      sim talents=${talent} default_actions=1
      [ "${status}" -eq 0 ]
    done
  done
}

function covenant_sim() {
  PROFILE_DIR=${SIMC_PROFILE_DIR}
  PROFILES=( $(ls "${PROFILE_DIR}"/*_$1_*.simc) )
  COVENANT=$2
  for spec in ${PROFILES[@]}; do
    SIMC_PROFILE=${spec}
    sim covenant="${COVENANT}" default_actions=1 level=60
    [ "${status}" -eq 0 ]
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
