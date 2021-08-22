## Ruby417
Ruby417 is a work in progress. The eventual goal is a Ruby library that can locate and decode [PDF417](https://en.wikipedia.org/wiki/PDF417) barcodes from images.

## Requirements
[ImageMagick](https://github.com/ImageMagick/ImageMagick) must be installed, as it is used to decode and preprocess images. Version >= 7.0.9 was used during development and will definitely work, but earlier 7.0.x versions are known to fail. [MiniMagick](https://github.com/minimagick/minimagick) is used to send commands to ImageMagick in Ruby.

So far, Ruby417 has been tested only on MRI Ruby 3.0.2 (because that's what I'm using). It doesn't use any exotic features, however, and should run fine on 2.x versions.

In order to achieve reasonable performance, a significant portion of Ruby417 is written as a C native extension. This must be compiled before running.

## Installation & Usage
Ruby417 is currently incomplete. In fact, the only existing functionality is barcode localization. If you'd like to check it out, though, clone the repository or download the code. Then navigate to the base `ruby417` directory and compile and install the native extensions (in the future, this will be simpler):

```
$ ruby ext/extconf.rb
checking for void qsort_r(void *, size_t, size_t, int (*)(const void *, const void *, void *), void *) in stdlib.h... yes
checking for M_PI in math.h... yes
checking for M_PI_2 in math.h... yes
checking for magick... yes
checking magick version...7.0.11-6
creating Makefile
$ make
compiling ext/ruby417.c
linking shared-object ruby417/ext/ruby417.so
$ make install  # this may require sudo privileges, depending where your Ruby installation is
/usr/bin/install -c -m 0755 ruby417.so $RUBY_ROOT/lib/ruby/site_ruby/3.0.0/x86_64-linux/ruby417/ext
```

This will place `ruby417.so` in `$RUBY_ROOT/lib/ruby/site_ruby/3.0.0/x86_64-linux/ruby417/ext` or somewhere similar. You can just delete it to uninstall the native extension.

You may also want to run the C tests (this can be done before installing too):

```
$ ./spec/ext/run_suite.sh
Checking source...
Compiling...
Running tests...
** test test **
Done.
```

At this point, you can use any functionality by `require`ing Ruby417 in the usual way. Here's the code I use to test things out:

```ruby
# ruby417/test_localization.rb

$LOAD_PATH << "#{__dir__}/lib" # change the load path as necessary

require "ruby417"

Ruby417.configure do |config|
  config.localization_strictness = :basic
end

# "edge guards" refers to the vertical black strips on either side of a PDF417 barcode
scanner = Ruby417::Localization::Guards.new

# print out ImageMagick draw commands
scanner.run(ARGV[0]).each do |barcode|
  print " fill none stroke lime stroke-width 3"
  print " path 'M #{barcode.upper_left.x},#{barcode.upper_left.y}\
                  #{barcode.upper_right.x},#{barcode.upper_right.y}\
                  #{barcode.lower_right.x},#{barcode.lower_right.y}\
                  #{barcode.lower_left.x},#{barcode.lower_left.y} Z'"
end
```

And run

```
$ ruby test_localization.rb IMAGE_PATH
fill none stroke lime stroke-width 3 path 'M 650,379                  1167,634                  1066,836                  527,549 Z'
$ magick IMAGE_PATH -draw "$(ruby test_localization.rb IMAGE_PATH)" detected.jpg
```

and open `detected.jpg` in an image viewer (as a test image, try using `spec/fixtures/sir_walter_scott_blurred_rotated.jpg`). The barcode should be outlined in a green quadrilateral. The entire detection process on a 1603x1202 image with a single barcode takes about 0.6 seconds, half of which is spent in ImageMagick, preprocessing the image.

Stay tuned!
