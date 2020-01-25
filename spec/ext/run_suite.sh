#!/usr/bin/env bash

# This script can be run from anywhere, but it assumes the following directory
# structure:
#
# ext/
#   ruby417/
#     darray/
#       darray.h
#     rectangles/
#       rectangles.c
#     ...
# spec/
#   ext/
#     fixtures/
#     run_suite.sh
#     ...
#

function error {
  echo "$(basename $0): $1"
  exit -1
}

base_dir="$(dirname $0)/../.."
source_dir="$base_dir/ext/ruby417"
test_dir="$base_dir/spec/ext"

if [[ ! -e $source_dir/rectangles/rectangles.h ]]; then
  error "unable to find sources"
fi

if [[ ! -d $test_dir/fixtures ]]; then
  error "unable to find fixtures"
fi

echo "Recompiling suite..."

for file in $test_dir/test_*.c; do
  gcc $file -I$source_dir                           \
    `pkg-config --libs --cflags glib-2.0` -lm       \
    -DNO_RUBY -o $test_dir/"exec_$(date +%s)_$(basename $file).out"

  if [ $? -ne 0 ]; then
    error "Compilation failed on $(basename $file)"
  else
    basename $file
  fi
done

echo "Running tests..."

pushd $test_dir > /dev/null

for file in exec_*.out; do
  ./$file $@
  rm $file
done

popd > /dev/null

echo "Done."
