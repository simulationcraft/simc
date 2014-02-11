#!/bin/bash

### Defaults:
# Iterations
if [ -z "${SIMC_ITERATIONS}" ]; then
  SIMC_ITERATIONS=100
fi
# Simc executable
if [ -z "${SIMC_CLI_PATH}" ]; then
  SIMC_CLI_PATH="/var/lib/jenkins/jobs/simc-cli/workspace/engine/simc"
fi
# Profiles directory
if [ -z "${SIMC_PROFILES_PATH}" ]; then
  SIMC_PROFILES_PATH="/var/lib/jenkins/jobs/simc-cli/workspace/profiles"
fi
###

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
  SIMC_PROFILE=$(/bin/ls "${SIMC_PROFILES_PATH}"/Tier??H/Raid_T??H.simc|tail -1)
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
