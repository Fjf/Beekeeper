#!/bin/bash

for playouts in 10 20 50 100 200 500 1000 2000; do
  echo $playouts >> out_$playouts.txt
  for i in {1..5}; do
    ./cmake-build-debug/hive_run $playouts >> out_$playouts.txt
  done
done