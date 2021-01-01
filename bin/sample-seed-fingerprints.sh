#!/bin/bash
DIR="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"
help () {
  echo >&2 'Usage: sample-seed-fingerprints.sh ./prng'
  exit 1
}
cmd=$@
if [[ -z $cmd ]]; then help; fi

for s in `seq 0 20`; do $1 -s $s | "$DIR"/fingerprint.sh; done
