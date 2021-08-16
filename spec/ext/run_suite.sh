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

echo "Compiling..."
egcc $source_dir/darray.c -g -Wall -Wextra -c -o $test_dir/darray.o
egcc $source_dir/image.c -g -Wall -Wextra -c -o $test_dir/image.o
egcc $source_dir/rectangles.c -g -Wall -lm -Wextra -c -o $test_dir/rectangles.o

egcc $test_dir/test_darray.c $test_dir/darray.o -g -lm -I$source_dir -o $test_dir/exec_test_darray
egcc $test_dir/test_image.c $test_dir/image.o -g -lm -I$source_dir -o $test_dir/exec_test_image
egcc $test_dir/test_rectangles.c $test_dir/image.o $test_dir/darray.o -g -lm -I$source_dir -o $test_dir/exec_test_rectangles

echo "Running tests..."
pushd $test_dir > /dev/null
for file in exec_*; do
  ./$file
  rm $file
done
rm *.o

popd > /dev/null

echo "Done."
