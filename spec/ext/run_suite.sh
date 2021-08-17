#!/bin/bash

error () {
  echo "$(basename $0): $1"
  exit 1
}

egcc () {
  gcc $@

  if [ $? -ne 0 ]; then
    error "Compilation failed on $(basename $1)"
  fi
}

base_dir="$(dirname $0)/../.."
source_dir="$base_dir/ext/ruby417"
test_dir="$base_dir/spec/ext"

echo "Checking source..."
egcc $source_dir/ruby417.c -Wall -Wextra -fsyntax-only

echo "Compiling..."
flags="-g -lm -I$source_dir"
egcc $test_dir/test_darray.c $flags -o $test_dir/exec_test_darray
egcc $test_dir/test_image.c $flags -o $test_dir/exec_test_image
egcc $test_dir/test_rectangles.c $flags -o $test_dir/exec_test_rectangles

echo "Running tests..."
pushd $test_dir > /dev/null
for file in exec_*; do
  ./$file
  rm $file
done

popd > /dev/null

echo "Done."
