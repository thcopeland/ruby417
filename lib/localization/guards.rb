module Ruby417
  module Localization
    class Guards
      include Tools::Geometry

      attr_reader :settings

      def initialize(preprocessing: :full, area_threshold:, fitting_threshold:, guard_aspect:, angle_variation:, area_variation:)
        @settings = {
          preprocessing:     preprocessing,
          area_threshold:    area_threshold,
          fitting_threshold: fitting_threshold,
          guard_aspect:      guard_aspect,
          angle_variation:   angle_variation,
          area_variation:    area_variation
        }
      end

      def run(path)
        image = MiniMagick::Image.open(path)
        rectangles = RectangleDetection.process_image_data(preprocess_image(path), image.width, image.height, settings[:area_threshold], 128)

        filtered = process_matches(rectangles, settings[:fitting_threshold], settings[:guard_aspect])

        locate_barcodes(filtered, settings[:angle_variation], settings[:area_variation])
      end

      def locate_barcodes(guards, angle_variation, area_variation)
        [].tap do |barcodes|
          guards.each_with_index do |guard, i|
            j = i-1

            while j >= 0 && (guard.true_area - guards[j].true_area) < guard.true_area*area_variation
              if guard_pair?(guard, guards[j], angle_variation)
                barcodes << determine_barcode_location(guard, guards[j])
              end
              j -= 1
            end
          end
        end
      end

      def determine_barcode_location(g1, g2)
        center = Point.new((g1.cx+g2.cx)/2, (g1.cy+g2.cy)/2)
        orientation = Math.atan2(g2.cy - g1.cy, g2.cx - g1.cx)

        bounds_width  = Math.hypot(g1.cy-g2.cy, g1.cx-g2.cx)
        bounds_height = [g1.height, g1.width, g2.height, g2.width].max

        dx, dh = bounds_width/2, bounds_height/2

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

        upper_left  = corners.max_by { |corner| corner.distance(bounds_lower_right) }
        upper_right = corners.max_by { |corner| corner.distance(bounds_lower_left) }
        lower_right = corners.max_by { |corner| corner.distance(bounds_upper_left) }
        lower_left  = corners.max_by { |corner| corner.distance(bounds_upper_right) }

        LocatedBarcode.new(barcode_orientation, upper_left, upper_right, lower_right, lower_left)
      end

      def guard_pair?(a, b, angle_variation)
        if (a.orientation - b.orientation).abs < angle_variation
          similar_dimensions?(a.width, b.width, a.height, b.height) &&
            oriented_well?(a, b, angle_variation) && positioned_well?(a, b)
        elsif (a.orientation - b.orientation).abs - Math::PI/2 < angle_variation
          similar_dimensions?(a.width, b.height, a.height, b.width) &&
            oriented_well?(a, b, angle_variation) && positioned_well?(a, b)
        end
      end

      def similar_dimensions?(w1, w2, h1, h2)
        (w1 - w2).abs < [w1, w2].min/2 && (h1 - h2).abs < [h1, h2].min/2
      end

      def oriented_well?(a, b, threshold)
        relative_angle = (a.cx == b.cx ? Math::PI/2 : Math.atan((a.cy-b.cy)/(a.cx-b.cx).to_f))

        # Note: sine is used as an approximation to the "distance from nearest
        # multiple of pi" function (1-(1-2*x/PI).abs).abs
        if a.width > a.height
          Math.sin(relative_angle - (Math::PI/2 + a.orientation)).abs < threshold
        else
          Math.sin(relative_angle - a.orientation).abs < threshold
        end
      end

      def positioned_well?(a, b)
        Math.hypot(a.cx - b.cx, a.cy - b.cy).between?(2*[a.width, a.height].max, 70*[a.width, a.height].min)
      end

      def process_matches(rectangles, rectangle_threshold, aspect)
        rectangles.select { |rect| matches_well?(rect, rectangle_threshold, aspect) }
                  .sort_by!(&:true_area)
      end

      def matches_well?(rect, threshold, aspect)
        rect.true_area > threshold*rect.width*rect.height &&
          rect.width*aspect < rect.height || rect.height*aspect < rect.width
      end

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
