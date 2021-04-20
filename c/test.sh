#!/bin/bash

for i in {1..50}; do
  ./cmake-build-debug/hive_run 200 >> out_MM_allmoves.txt
done
