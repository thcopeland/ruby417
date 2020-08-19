## Ruby417
Ruby417 is a work in progress. The eventual goal is a Ruby library that can locate and decode [PDF417](https://en.wikipedia.org/wiki/PDF417) barcodes from images.

## Requirements
[ImageMagick](https://github.com/ImageMagick/ImageMagick) must be installed, as it is used to decode and preprocess images. Version >= 7.0.9 was used during development and will definitely work, but earlier 7.0.x versions are known to fail. [MiniMagick](https://github.com/minimagick/minimagick) is used to send commands to ImageMagick in Ruby.

So far, Ruby417 has been tested only on MRI Ruby 2.7. It should run fine on previous versions, however.

In order to achieve reasonable performance, a significant portion of Ruby417 is written as a C native extension. This must be compiled before running.

## Installation & Usage
Ruby417 is currently incomplete. In fact, the only existing functionality is barcode localization. If you'd like to check it out, though, clone the repository or download the code. Then navigate to `ruby417/ext/ruby417/rectangles` and compile and install the native extensions (in the future, this will be simpler):

```
$ ruby extconf.rb
creating Makefile
$ make
compiling rectangles.c
linking shared-object ruby417/ext/rectangle_detection.so
$ sudo make install
/usr/bin/install -c -m 0755 rectangle_detection.so /usr/local/src/rubies/ruby-2.7.0/lib/ruby/site_ruby/2.7.0/x86_64-linux/ruby417/ext
```

This will place `rectangle_detection.so` in `$RUBY_ROOT/lib/ruby/site_ruby/2.7.0/x86_64-linux/ruby417/ext` or somewhere similar. You can just delete it to uninstall the native extension.

If pkg-config and GLib are available, you can run the C tests (this can be done before installing too):

```
$ ./spec/ext/run_suite.sh
Recompiling suite...
Running suite...
```

At this point, you can use any functionality by `require`ing Ruby417 in the usual way. Here's the code I use to test things out:

```ruby
# test_localization.rb

require_relative "PATH_TO_RUBY417/lib/ruby417"

# "edge guards" refers to the vertical black strips on either side of a PDF417 barcode
scanner =
  Ruby417::Localization::Guards.new(
    preprocessing:     :full,       # amount of preprocessing, one of :full, :basic, :half, :none
    area_threshold:    400,         # minimum area of edge guards, often as high as 2000
    fitting_threshold: 0.75,        # determines what qualifies as a rectangle; 1.0 = perfect, 0 = anything
    guard_aspect:      3..30,       # the height-width ratio bounds for edge guards
    barcode_aspect:    2..8,        # the bounds of the barcode aspect ratio
    angle_variation:   Math::PI/16, # the angle variation allowed between edge guards
    area_variation:    0.35)        # the area variation allowed between guards

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
**ImageMagick drawing commands**
$ magick IMAGE_PATH -draw "$(ruby test_localization.rb IMAGE_PATH)" detected.jpg
```

and open `detected.jpg` in an image viewer (as a test image, try using `spec/fixtures/sir_walter_scott_blurred_rotated.jpg`). The barcode should be outlined in a green quadrilateral. The entire detection process on a 1603x1202 image with a single barcode takes about half a second, most of which is spent in ImageMagick, preprocessing the image.

Stay tuned!
