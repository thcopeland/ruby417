#!/usr/bin/env bash

# This script can be run from anywhere, but it assumes the following directory
# structure:
#
# ext/
#   ruby417/
#     ...
#   test/
#     fixtures/
#     run_suite.sh
#     ...

base_dir="$(dirname $0)/.."
test_dir="$base_dir/test"
out_file="test_$(date +%s).out"

if [[ ! -e $test_dir/tests.c ]]; then
  echo "$(basename $0): unable to find tests"
  exit -1
fi

if [[ ! -d $test_dir/fixtures ]]; then
  echo "$(basename $0): unable to find fixtures"
  exit -1
fi

echo "Recompiling suite..."
gcc $test_dir/tests.c $test_dir/utilities.c       \
  `pkg-config --libs --cflags glib-2.0` -lm       \
  -o $test_dir/$out_file

echo "Running tests..."
(cd $test_dir; ./$out_file $@)

rm $test_dir/$out_file

echo "Done."
