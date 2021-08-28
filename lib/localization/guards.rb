require "mini_magick"

module Ruby417
  module Localization
    # This method works by finding the PDF417 edge guards and using them to
    # determine the barcode location. It should be able to handle images with
    # many extraneous features, such as text or images. But it cannot handle
    # truncated barcodes (which have only the left edge guard) or barcodes with
    # severely damaged edge guards.
    class Guards
      attr_reader :config

      def initialize(config=Ruby417.configuration)
        @config = config
      end

      def run(path)
        image = MiniMagick::Image.open(path)
        pixels = preprocess_image(path)

        if config.localization_guard_area_threshold.between?(0, 1)
          guard_area_threshold = (config.localization_guard_area_threshold * image.width * image.height).to_i
        else
          guard_area_threshold = config.localization_guard_area_threshold
        end

        barcode_data = Ruby417::Ext.locate_via_guards(
          pixels, image.width, image.height,
          guard_area_threshold,
          config.localization_guard_rectangularity_threshold,
          config.localization_angle_variation_threshold,
          config.localization_guard_area_variation_threshold,
          config.localization_guard_width_variation_threshold,
          config.localization_guard_height_variation_threshold,
          config.localization_guard_aspect.min,
          config.localization_guard_aspect.max,
          config.localization_barcode_aspect.min,
          config.localization_barcode_aspect.max
        )

        barcode_data.map do |data|
          LocatedBarcode.new(
            data[0],
            Point.new(data[1], data[2]),
            Point.new(data[3], data[4]),
            Point.new(data[5], data[6]),
            Point.new(data[7], data[8])
          )
        end
      end

      # Perform preprocessing and get image pixel data
      def preprocess_image(path)
        MiniMagick::Tool::Convert.new.yield_self do |convert|
          convert << path

          if config.localization_preprocessing != :none
            # general preprocessing
            convert.colorspace "Gray"
            convert.normalize

            # try to remove shadows, often unnecessary for clean images
            if config.localization_preprocessing == :full
              convert.stack do |stack|
                convert.clone 0
                convert.sample "25%"
                convert.blur "30x10"
                convert.resize "400%"
              end
              convert.compose "DivideSrc"
              convert.composite
            end

            # remove short vertical features
            convert.morphology "Close", "3x6: 1,-,1 1,-,1 1,-,1 1,-,1 1,-,1 1,-,1"
            # remove small features and flaws
            convert.morphology "Close:3", "Square:1" unless config.localization_preprocessing == :half

            convert.auto_threshold "Otsu"
            convert.negate
          end

          convert.depth 8
          # convert.write "preprocessed.png"
          convert << "gray:-"

          MiniMagick::Shell.new.run(convert.command)
        end.first
      end
    end
  end
end
