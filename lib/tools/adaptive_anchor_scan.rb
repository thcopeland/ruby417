module Ruby417
  module Tools
    DetectedAnchor = Struct.new(:left, :right, :y, :score)
    AdaptiveThreshold = Struct.new(:intensity, :deviation)

    SELECTION_RATE = 1

    class AdaptiveAnchorScan
      attr_reader :resolution, :min_width, :qualifier, :calibrator

      def initialize(calibration_resolution:, min_anchor_width:, anchor_qualifier:, threshold_calibrator:)
        @resolution = calibration_resolution
        @min_width  = min_anchor_width
        @qualifier  = anchor_qualifier
        @calibrator = threshold_calibrator
      end

      def scan_image(pixels)
        [].tap do |matches|
          region_height = adaptive_region_height(pixels.length)

          pixels.each_slice(region_height).with_index do |horizontal_region, index|
            regional_thresholds = calculate_thresholds(horizontal_region[horizontal_region.length / 2])

            horizontal_region.each_with_index do |pixel_row, offset|
              matches.concat(scan_row(pixel_row, region_height * index + offset, regional_thresholds))
            end
          end
        end
      end

      def calculate_thresholds(pixel_row)
        pixel_row.each_slice(adaptive_region_width(pixel_row.length)).map { |slice| calibrator.call(slice) }
      end

      def adaptive_region_height(image_height)
        image_height / resolution.last
      end

      def adaptive_region_width(image_width)
        image_width / resolution.first
      end

      def scan_row(pixel_row, y, thresholds)
        matches = []
        scanner = 0

        while scanner < pixel_row.length
          if dark?(pixel_row[scanner], threshold(scanner, pixel_row, thresholds))
            lookahead = consume_dark(scanner, pixel_row, thresholds)
            anchor_width = lookahead - scanner

            if anchor_width >= min_width
              score = qualifier.call(scanner, anchor_width, lookahead, pixel_row)
              matches << DetectedAnchor.new(scanner, lookahead, y, score) if score != false
            end

            scanner = lookahead + 1
          else
            scanner = consume_bright(scanner, pixel_row, thresholds)
          end
        end

        matches.max_by(SELECTION_RATE, &:score)
      end

      def consume_bright(x, pixels, thresholds, threshold=threshold(x, pixels, thresholds))
        x += 1 while x < pixels.length && bright?(pixels[x], threshold); x
      end

      def consume_dark(x, pixels, thresholds, threshold=threshold(x, pixels, thresholds))
        x += 1 while x < pixels.length && dark?(pixels[x], threshold); x
      end

      def threshold(x, pixel_row, thresholds)
        thresholds[x * thresholds.length / pixel_row.length]
      end

      def bright?(intensity, threshold)
        intensity >= threshold.intensity - threshold.deviation
      end

      def dark?(intensity, threshold)
        intensity <= threshold.intensity + threshold.deviation
      end
    end
  end
end
