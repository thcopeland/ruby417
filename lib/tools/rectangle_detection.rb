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
