module Ruby417
  module Localization
    # This method works by finding the PDF417 edge guards and using them to
    # determine barcode position.
    #
    # It should do particularly well in images with many extraneous features
    # such as text or objects.
    #
    # It will fail for truncated barcodes, which lack a right edge guard, or
    # when the edge guards are badly damaged.
    class Guards
      include Tools::Geometry

      PI_2 = Math::PI/2
      PI_4 = Math::PI/4

      attr_reader :settings

      def initialize(preprocessing:, area_threshold:, fitting_threshold:, guard_aspect:, barcode_aspect:, angle_variation:, area_variation:)
        @settings = {
          preprocessing:     preprocessing,
          area_threshold:    area_threshold,
          fitting_threshold: fitting_threshold,
          guard_aspect:      guard_aspect,
          barcode_aspect:    barcode_aspect,
          angle_variation:   angle_variation,
          area_variation:    area_variation
        }
      end

      # Locate the barcodes in the image at the given path, and sort them in
      # order of descending score
      def run(path)
        image = MiniMagick::Image.open(path)

        rectangles = RectangleDetection.process_image_data(preprocess_image(path), image.width, image.height, settings[:area_threshold], 128)
        rectangles.each(&:normalize!)

        filtered = process_matches(rectangles, settings[:fitting_threshold], settings[:guard_aspect])

        locate_barcodes(filtered, settings[:angle_variation], settings[:area_variation], settings[:barcode_aspect])
      end

      # Determine which potential guards are valid and determine barcode locations
      def locate_barcodes(guards, angle_variation, area_variation, barcode_aspect_range)
        # note: the guards array must be sorted by area
        barcodes = []
        guards.each_with_index do |guard, i|
          j = i-1

          while j >= 0 && (guard.true_area - guards[j].true_area) < guard.true_area*area_variation
            if guard_pair?(guard, guards[j], angle_variation, barcode_aspect_range)
              barcodes << determine_barcode_location(guard, guards[j])
            end
            j -= 1
          end
        end

        # sort by score descending
        barcodes.sort { |a, b| b.score <=> a.score }
      end

      # Calculate the vertices of a quadrilateral that contains the barcode
      # described by two edge guards.
      def determine_barcode_location(g1, g2)
        # estimated features of the barcode
        center = Point.new((g1.cx+g2.cx)/2, (g1.cy+g2.cy)/2)
        orientation = Math.atan2(g2.cy - g1.cy, g2.cx - g1.cx)

        # the angle needed to rotate the barcode to a 90-degree vertical or
        # horizontal position
        rotation = (orientation/PI_2).round*PI_2 - orientation

        if orientation.abs <= PI_4 || orientation.abs >= 3*PI_4
          barcode_orientation = :horizontal
        else
          barcode_orientation = :vertical
        end

        # rotate the corners of the guards about the center of the barcode so
        # that the virtual barcode is vertical or horizontal
        rotated_corners = (g1.corners + g2.corners).map { |corner| corner.rotate(center, rotation) }

        upper_left  = rotated_corners.min_by { |p| p.x + p.y }
        upper_right = rotated_corners.max_by { |p| p.x - p.y }
        lower_right = rotated_corners.max_by { |p| p.x + p.y }
        lower_left  = rotated_corners.max_by { |p| p.y - p.x }

        # #rotate() restores the corners to their original locations
        LocatedBarcode.new(barcode_orientation, score_guard_pair(g1, g2),
                                                upper_left.rotate(center,  -rotation),
                                                upper_right.rotate(center, -rotation),
                                                lower_right.rotate(center, -rotation),
                                                lower_left.rotate(center,  -rotation))
      end

      # Calculate a measure of an edge guard pair's likelihood of being legitimate
      def score_guard_pair(g1, g2)
        rectangularity_score = g1.true_area/(g1.width*g1.height).to_f * g2.true_area/(g2.width*g2.height).to_f
        min_area, max_area = [g1.true_area, g2.true_area].minmax
        similarity_score = 1 - (15*max_area/16 - min_area).abs/max_area.to_f
        area_score = Math.sqrt(min_area + max_area)
        guard_aspect_score = (5 - g1.height / g1.width.to_f).abs
        barcode_aspect_score = (3 - Math.hypot(g1.cy-g2.cy, g1.cx-g2.cx)/g1.height).abs
        orientation_score = 1 - Math.sin(g1.orientation - g2.orientation).abs

        area_score * orientation_score * rectangularity_score * similarity_score - guard_aspect_score - barcode_aspect_score
      end

      # Determine whether the given rectangles form an edge guard pair
      def guard_pair?(a, b, angle_variation, aspect_range)
        within_angular_threshold?(a.orientation, b.orientation, angle_variation) &&
          similar_dimensions?(a, b) &&
          sized_well?(a, b, aspect_range) &&
          oriented_well?(a, b, angle_variation)
      end

      # Test the given dimensions for similarity
      def similar_dimensions?(a, b)
        width_threshold = [a.width, b.width, Math.sqrt(a.width*a.height) / 4].min
        height_threshold = [a.height, b.height, Math.sqrt(a.width*a.height) / 4].min

        (a.width-b.width).abs < width_threshold && (a.height-b.height).abs < height_threshold
      end

      # Test whether two potential edge guards are oriented properly (the angle
      # of the line between the guards perpendicular to the guards)
      def oriented_well?(a, b, threshold)
        relative_angle = (a.cx == b.cx ? PI_2 : Math.atan((a.cy-b.cy)/(a.cx-b.cx).to_f))

        within_angular_threshold?(relative_angle, a.orientation, threshold) &&
          within_angular_threshold?(relative_angle, b.orientation, threshold)
      end

      # Test whether the barcode aspect is within the specified range
      def sized_well?(a, b, aspect_range)
        width = Math.hypot(a.cy-b.cy, a.cx-b.cx)
        height = (a.height + b.height) / 2

        aspect_range.cover?(width/height) && width.between?(a.width, a.width * 70)
      end

      # Drop poor guard matches and sort the survivors by area (a sorted array
      # is necessary in #locate_barcodes)
      def process_matches(rectangles, rectangle_threshold, aspect)
        rectangles.select { |rect| matches_well?(rect, rectangle_threshold, aspect) }
                  .sort_by!(&:true_area)
      end

      # Quick test to whether the rectangle is a good candidate for an edge
      # guard. Returns true if the given rectangle is "rectangular" (has a
      # sufficiently high region-area to rectangle-area ratio) and if the
      # dimensions are appropriately different.
      def matches_well?(rect, threshold, guard_aspect_range)
        rect.true_area > threshold*rect.width*rect.height &&
          guard_aspect_range.cover?(rect.height/rect.width.to_f)
      end

      # Test whether the angles are near each other or near opposite each other
      def within_angular_threshold?(a1, a2, t)
        Math.sin(a1 - a2).abs < t
      end

      # Perform preprocessing and get image pixel data.
      def preprocess_image(path)
        MiniMagick::Tool::Convert.new.yield_self do |convert|
          convert << path

          unless settings[:preprocessing] == :none
            # general preprocessing
            convert.colorspace "Gray"
            convert.normalize

            # try to remove shadows, often unnecessary for clean images
            if settings[:preprocessing] == :full
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
            convert.morphology "Close:3", "Square:1" unless settings[:preprocessing] == :half

            convert.auto_threshold "Otsu"
            convert.negate
          end

          convert.depth 8
          convert << "gray:-"

          MiniMagick::Shell.new.run(convert.command)
        end.first
      end
    end
  end
end
