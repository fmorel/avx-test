#!/bin/bash

perf stat -e cycles -e cache-misses -e instructions ./mat_mult 1
perf stat -e cycles -e cache-misses -e instructions ./mat_mult 2
perf stat -e cycles -e cache-misses -e instructions ./mat_mult 3
