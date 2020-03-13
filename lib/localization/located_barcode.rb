module Ruby417
  module Localization
    class LocatedBarcode
      attr_reader :orientation, :upper_left, :upper_right, :lower_right, :lower_left

      def initialize(orientation, upper_left, upper_right, lower_right, lower_left)
        @orientation = orientation
        @upper_left  = upper_left
        @upper_right = upper_right
        @lower_left  = lower_left
        @lower_right = lower_right
      end

      def width
        [upper_left.distance(upper_right), lower_left.distance(lower_right)].max
      end

      def height
        [upper_left.distance(lower_left), upper_right.distance(lower_right)].max
      end
    end
  end
end
