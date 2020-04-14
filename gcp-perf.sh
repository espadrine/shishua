#!/bin/bash
sudo apt update
sudo apt install -y make gcc
make test/perf
