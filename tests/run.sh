#!/bin/bash

### Defaults:
# Iterations
if [ -z "${SIMC_ITERATIONS}" ]; then
  export SIMC_ITERATIONS=20
fi
# Simc executable
if [ -z "${SIMC_CLI_PATH}" ]; then
  export SIMC_CLI_PATH="../engine/simc"
fi
# Profiles directory
if [ -z "${SIMC_PROFILES_PATH}" ]; then
  export SIMC_PROFILES_PATH="../profiles"
fi
###

# OS X does not include "timeout" command by default, it can be installed
# through coreutils from macports/homebrew. Macports at least names it
# "gtimeout" to differentiate between OS X version of coreutils, so perform
# some detection on the correct command.
SIMC_TIMEOUT=$(which timeout)
if [ $? != 0 ]; then
  SIMC_TIMEOUT=$(which gtimeout)
  if [ $? != 0 ]; then
    echo "Could not find 'timeout' command."
    exit 1
  fi
fi
export SIMC_TIMEOUT

# Check for arguments
if [ -z "$1" ]; then
  echo "Usage: $0 <bats file> ..."
  exit 1
fi

# Check if we can run simc
if [ ! -x "${SIMC_CLI_PATH}" ]; then
  echo "Not executable: ${SIMC_CLI_PATH}"
  exit 1
fi

# Check if the profiles directory exists
if [ ! -d "${SIMC_PROFILES_PATH}" ]; then
  echo "Not a directory: ${SIMC_PROFILES_PATH}."
  exit 1
fi

# Look for a suitable profile, unless supplied
if [ -z "${SIMC_PROFILE}" ]; then
  export SIMC_PROFILE=$(/bin/ls "${SIMC_PROFILES_PATH}"/PR_Raid.simc|tail -1)
  export SIMC_PROFILE_DIR="${SIMC_PROFILES_PATH}/PreRaids"
  if [ -z "${SIMC_PROFILE}" ]; then
    echo "Could not find a suitable profile and none was supplied."
    exit 1
  fi
fi

# Check for bats
if ! which bats > /dev/null; then
  echo "Cannot find 'bats' in $PATH".
  exit 1
fi

# Run tests
bats --tap $@
