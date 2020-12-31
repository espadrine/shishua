#!/bin/bash
apt update
apt install -y make clang
CC=clang make test/benchmark-perf
