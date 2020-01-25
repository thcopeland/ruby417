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
  end
end
