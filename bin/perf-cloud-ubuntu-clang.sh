#!/bin/bash
DIR="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"
cd "$DIR/.."
sudo apt-get update
sudo apt-get install -y make clang
make CC=clang test/benchmark-perf
