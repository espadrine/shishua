#!/bin/bash
for s in `seq 0 20`; do $1 -s $s | ./fingerprint.sh; done
