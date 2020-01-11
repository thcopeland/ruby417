#!/usr/bin/env bash

# This script can be run from anywhere, but it assumes the following directory
# structure:
#
# ext/
#   ruby417/
#     rectangles/
#       ...
#     ...
#   test/
#     fixtures/
#     run_suite.sh
#     ...

function error() {
  echo "$(basename $0): $1"
  exit -1
}

base_dir="$(dirname $0)/.."
source_dir="$base_dir/ruby417"
test_dir="$base_dir/test"
out_file="test_$(date +%s).out"

if [[ ! -e $source_dir/rectangles/rectangles.h ]]; then
  error "unable to find sources"
fi

if [[ ! -e $test_dir/tests.c ]]; then
  error "unable to find tests"
fi

if [[ ! -d $test_dir/fixtures ]]; then
  error "unable to find fixtures"
fi

echo "Recompiling suite..."
gcc $test_dir/tests.c $test_dir/utilities.c         \
  `pkg-config --libs --cflags glib-2.0` -lm         \
  -DNO_RUBY                                         \
  -o $test_dir/$out_file

echo "Running tests..."
(cd $test_dir; ./$out_file $@)

rm $test_dir/$out_file

echo "Done."
