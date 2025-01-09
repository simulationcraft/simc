#!/usr/bin/env sh
set -e

alias py=python3

ROOT=$(cd $(dirname $0)/ && pwd)

if [ "$(pwd)" != "${ROOT}" ]; then
  echo "you must run this script in the root directory for this repository" >&2
  exit 1
fi

classes_to_generate=(
  shaman
)

for class in $classes_to_generate; do
  pushd "engine/class_modules/apl/${class}"
  ./generate_${class}.sh
  popd
done

echo 'done'
