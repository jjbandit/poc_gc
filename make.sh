#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

echo "LOC : $(find . -name \*.cc -o -name \*.h|xargs cat | wc -l )"

clang++                   \
  -Wall                   \
  -D GC_ALLOC=1           \
  -fsanitize=address      \
  -fno-omit-frame-pointer \
  -ggdb                   \
  test.cc -o __test_gc

./__test_gc

