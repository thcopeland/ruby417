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

      attr_reader :settings

      def initialize(preprocessing:, area_threshold:, fitting_threshold:, guard_aspect:, angle_variation:, area_variation:)
        @settings = {
          preprocessing:     preprocessing,
          area_threshold:    area_threshold,
          fitting_threshold: fitting_threshold,
          guard_aspect:      guard_aspect,
          angle_variation:   angle_variation,
          area_variation:    area_variation
        }
      end

      # Locate the barcodes in the image at the given path, and sort them in
      # order of descending score
      def run(path)
        image = MiniMagick::Image.open(path)

        rectangles = RectangleDetection.process_image_data(preprocess_image(path), image.width, image.height, settings[:area_threshold], 128)

        filtered = process_matches(rectangles, settings[:fitting_threshold], settings[:guard_aspect])

        locate_barcodes(filtered, settings[:angle_variation], settings[:area_variation])
      end

      # Determine which potential guards are valid and determine barcode locations
      def locate_barcodes(guards, angle_variation, area_variation)
        # note: the guards array must be sorted by area
        barcodes = []
        guards.each_with_index do |guard, i|
          j = i-1

          while j >= 0 && (guard.true_area - guards[j].true_area) < guard.true_area*area_variation
            if guard_pair?(guard, guards[j], angle_variation)
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
        # The technique used to determine the vertices below is to calculate the
        # upper left, upper right, lower left, lower right corners of a rectangle
        # that approximates the shape of the barcode. Then the upper left
        # vertex of the barcode quad is the one farthest from the lower right
        # corner of the rectangle, the upper right is farthest from the lower
        # left, etc.
        center = Point.new((g1.cx+g2.cx)/2, (g1.cy+g2.cy)/2)
        orientation = Math.atan2(g2.cy - g1.cy, g2.cx - g1.cx)

        # determine the dimensions of the rectangle
        bounds_width  = Math.hypot(g1.cy-g2.cy, g1.cx-g2.cx) + [g1.width, g1.height].min
        bounds_height = [g1.height, g1.width, g2.height, g2.width].max

        dx, dh = bounds_width/2, bounds_height/2

        # calculate the corners of the bounding rectangle. this is done in order
        # that bounds_upper_left is actually the upper left corner in the image
        # plane, etc.
        if orientation.abs <= Math::PI/4
          barcode_orientation = :horizontal
          bounds_upper_left  = Point.new(center.x-dx, center.y-dh).rotate(center, orientation)
          bounds_upper_right = Point.new(center.x+dx, center.y-dh).rotate(center, orientation)
          bounds_lower_right = Point.new(center.x+dx, center.y+dh).rotate(center, orientation)
          bounds_lower_left  = Point.new(center.x-dx, center.y+dh).rotate(center, orientation)
        elsif orientation.abs >= Math::PI*3/4
          barcode_orientation = :horizontal
          bounds_upper_left  = Point.new(center.x-dx, center.y-dh).rotate(center, orientation-Math::PI)
          bounds_upper_right = Point.new(center.x+dx, center.y-dh).rotate(center, orientation-Math::PI)
          bounds_lower_right = Point.new(center.x+dx, center.y+dh).rotate(center, orientation-Math::PI)
          bounds_lower_left  = Point.new(center.x-dx, center.y+dh).rotate(center, orientation-Math::PI)
        elsif orientation.positive?
          barcode_orientation = :vertical
          bounds_upper_left  = Point.new(center.x-dh, center.y-dx).rotate(center, orientation-Math::PI/2)
          bounds_upper_right = Point.new(center.x+dh, center.y-dx).rotate(center, orientation-Math::PI/2)
          bounds_lower_right = Point.new(center.x+dh, center.y+dx).rotate(center, orientation-Math::PI/2)
          bounds_lower_left  = Point.new(center.x-dh, center.y+dx).rotate(center, orientation-Math::PI/2)
        else
          barcode_orientation = :vertical
          bounds_upper_left  = Point.new(center.x-dh, center.y-dx).rotate(center, orientation+Math::PI/2)
          bounds_upper_right = Point.new(center.x+dh, center.y-dx).rotate(center, orientation+Math::PI/2)
          bounds_lower_right = Point.new(center.x+dh, center.y+dx).rotate(center, orientation+Math::PI/2)
          bounds_lower_left  = Point.new(center.x-dh, center.y+dx).rotate(center, orientation+Math::PI/2)
        end

        corners = g1.corners + g2.corners

        # identify the corners of the barcode
        upper_left  = corners.max_by { |corner| corner.distance(bounds_lower_right) - corner.distance(bounds_upper_left) }
        upper_right = corners.max_by { |corner| corner.distance(bounds_lower_left) - corner.distance(bounds_upper_right) }
        lower_right = corners.max_by { |corner| corner.distance(bounds_upper_left) - corner.distance(bounds_lower_right) }
        lower_left  = corners.max_by { |corner| corner.distance(bounds_upper_right) - corner.distance(bounds_lower_left) }

        LocatedBarcode.new(barcode_orientation, upper_left, upper_right, lower_right, lower_left, score_guard_pair(g1, g2))
      end

      # Calculate a measure of an edge guard pair's likelihood of being legitimate
      def score_guard_pair(g1, g2)
        rectangularity_score = g1.true_area/(g1.width*g1.height).to_f *
                               g2.true_area/(g2.width*g2.height).to_f

        area_score = g1.true_area + g2.true_area

        guard_aspect_score = [g1.width/g1.height, g1.height/g1.width].max

        barcode_aspect_score = Math.hypot(g1.cy-g2.cy, g1.cx-g2.cx)/[g1.width, g1.height].max

        orientation_score = (normalized_orientation(g1) - normalized_orientation(g2)).abs

        Math.sqrt(area_score)*(1 - orientation_score/Math::PI/2)*rectangularity_score + 8*guard_aspect_score - 2*barcode_aspect_score
      end

      # Determine whether the given rectangles form an edge guard pair
      def guard_pair?(a, b, angle_variation)
        if (a.orientation - b.orientation).abs < angle_variation
          similar_dimensions?(a.width, b.width, a.height, b.height) &&
            oriented_well?(a, b, angle_variation) && positioned_well?(a, b)
        elsif (a.orientation - b.orientation).abs - Math::PI/2 < angle_variation
          similar_dimensions?(a.width, b.height, a.height, b.width) &&
            oriented_well?(a, b, angle_variation) && positioned_well?(a, b)
        end
      end

      # Test the given dimensions for similarity
      def similar_dimensions?(w1, w2, h1, h2)
        width_threshold = [w1, w2, Math.sqrt(w1*h1) / 4].min
        height_threshold = [h1, h2, Math.sqrt(w1*h1) / 4].min

        (w1 - w2).abs < width_threshold && (h1 - h2).abs < height_threshold
      end

      # Test whether two potential edge guards are oriented properly (the angle
      # of the line between the guards perpendicular to the guards)
      def oriented_well?(a, b, threshold)
        relative_angle = (a.cx == b.cx ? Math::PI/2 : Math.atan((a.cy-b.cy)/(a.cx-b.cx).to_f))

        # note: sine is used as an approximation to the "distance to nearest
        # multiple of pi" function
        Math.sin(relative_angle - normalized_orientation(a)).abs < threshold &&
          Math.sin(relative_angle - normalized_orientation(b)).abs < threshold
      end

      # Test whether two potential edge guards are sufficently far apart.
      def positioned_well?(a, b)
        width, height = [a.width, a.height].minmax

        Math.hypot(a.cx-b.cx, a.cy-b.cy).between?(1.5*height, 80*width)
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
      def matches_well?(rect, threshold, aspect)
        rect.true_area > threshold*rect.width*rect.height &&
          (rect.width*aspect < rect.height || rect.height*aspect < rect.width)
      end

      # Rectangles detected by Ruby417::RectangleDetection are normalized to have
      # an orientation between 0 and pi/2. One result of this is that rectangle
      # A with an unnormalized orientation pi/2+0.1 is normalized to 0.1 with
      # the width and height swapped, and rectangle B with unnormalized
      # orientation pi/2-0.1 remains at pi/2-0.1 after normalization. Thus,
      # although the orientations vary by only 0.2 radians, the orientations are
      # totally different. This function performs a normalization that reverses
      # the RectangleDetection one partially, so that the (renormalized)
      # orientations may be compared.
      def normalized_orientation(rect)
        if rect.width > rect.height
          rect.orientation + Math::PI/2
        else
          rect.orientation
        end
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
