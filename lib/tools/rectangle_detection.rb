require "mini_magick"
require "ruby417/ext/rectangle_detection"

module Ruby417
  module RectangleDetection
    def self.detect(path, area_threshold: 0, intensity_threshold: 0)
      image = MiniMagick::Image.open(path)

      # look in ext/ for definion of .process_image_data()
      self.process_image_data(read_image_data(path),
                              image.width, image.height,
                              area_threshold, intensity_threshold)
    end

    def self.read_image_data(path)
      MiniMagick::Tool::Convert.new.yield_self do |convert|
        convert << path
        convert.depth(8)
        convert << "gray:-"

        MiniMagick::Shell.new.run(convert.command)
      end.first
    end

    class Rectangle
      include Tools::Geometry

      # Detected rectangles are normalized to have an orientation between 0 and
      # pi/2. One result of this is that a rectangle with an unnormalized
      # orientation pi/2+0.1 is normalized to 0.1 (with the width and height
      # swapped), and a rectangle with unnormalized orientation pi/2-0.1 remains
      # at pi/2-0.1 after normalization. Thus, although the orientations vary by
      # only 0.2 radians, the orientations are totally different. This function
      # performs a normalization that partially reverses the first one, so that
      # the (renormalized) orientations may be compared more easily.
      #
      # There is another useful effect of renormalization. The width and height
      # of detected rectangles have nothing to do with the vertical and
      # horizontal image axis, instead, the width is the length of the side
      # parallel to the local orientation axis. After renormalization, the width
      # is the smaller side length, and height is the larger.
      #
      # These properties are used extensively in Localization::Guards.
      def normalize!
        if width > height
          @width, @height = height, width
          @orientation += Math::PI/2
        end
      end

      def corners
        [ corner(-1, -1), corner(-1, 1), corner(1, -1), corner(1, 1) ]
      end

    private

      def corner(i, j)
        Point.new(cx+i*width/2.0, cy+j*height/2.0).rotate(Point.new(cx, cy), orientation)
      end
    end
  end
end
