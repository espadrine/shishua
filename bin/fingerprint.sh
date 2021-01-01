#!/bin/bash
cat | tr -dc a-z | head -c 10; echo
