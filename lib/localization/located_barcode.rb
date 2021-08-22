module Ruby417
  module Localization
    class LocatedBarcode
      attr_reader :upper_left, :lower_left, :lower_right, :upper_right, :score

      def initialize(score, upper_left, lower_left, lower_right, upper_right)
        @upper_left  = upper_left
        @lower_left  = lower_left
        @lower_right = lower_right
        @upper_right = upper_right
        @score = score
      end

      def width
        [upper_left.distance(upper_right), lower_left.distance(lower_right)].max
      end

      def height
        [upper_left.distance(lower_left), upper_right.distance(lower_right)].max
      end
    end

    class Point
      attr_accessor :x, :y

      def initialize(x, y)
        @x, @y = x, y
      end

      def distance(p)
        Math.hypot(x-p.x, y-p.y)
      end

      def ==(b)
        b.is_a?(Point) && (x - b.x).abs < 0.00001 && (y - b.y).abs < 0.00001
      end
    end
  end
end
