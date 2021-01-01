#!/bin/bash
DIR="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"
cd "$DIR/.."
sudo apt-get update
sudo apt-get install -y make gcc
make test/benchmark-perf
