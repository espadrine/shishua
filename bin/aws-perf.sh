#!/bin/bash
sudo apt-get update
sudo apt-get install -y make clang
make CC=clang test/benchmark-perf
