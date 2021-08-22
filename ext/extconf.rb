require "mkmf"

def detect_platform_features
  if have_func("void qsort_r(void *, size_t, size_t, int (*)(const void *, const void *, void *), void *)", "stdlib.h")
    $defs << "-DHAVE_GNU_QSORT_R"
  elsif have_func("void qsort_r(void *, size_t, size_t, void *, int (*)(void *, const void *, const void *))", "stdlib.h")
    $defs << "-DHAVE_BSD_QSORT_R"
  end

  $defs << "-DM_PI=#{Math::PI}" unless have_macro("M_PI", "math.h")
  $defs << "-DM_PI_2=#{Math::PI/2}" unless have_macro("M_PI_2", "math.h")
end

def check_imagemagick
  path = find_executable("magick")

  if path
    print "checking magick version..."
    version = `#{path} -version`[/([0-9\.\-]+)/]
    puts version

    if version !~ /^7/
      puts "\
**************************** WARNING ****************************
  The installed version of ImageMagick (version #{version}) is not
  supported. This doesn't affect Ruby417 installation, but you
  should download a recent version from imagemagick.org before
  using Ruby417, or you'll receive errors at runtime.
*****************************************************************"
    end
  else
    puts "\
**************************** WARNING ****************************
  Unable to find an ImageMagick executable. This doesn't affect
  Ruby417 installation, but you should download a recent version
  from imagemagick.org before using Ruby417, or you'll receive
  errors at runtime.
*****************************************************************"
  end
end

def set_flags
  $defs << "-DBUILD_RUBY_EXT"
  $CFLAGS << " -O3 -Wno-unused-function"
end

detect_platform_features
set_flags
check_imagemagick
create_makefile("ruby417/ext/ruby417")
