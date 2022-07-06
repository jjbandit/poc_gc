#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

clang++ \
  -D GC_ALLOC=1 \
  -fsanitize=address \
  -fno-omit-frame-pointer \
  -ggdb \
  test.cc -o __test_gc
./__test_gc

