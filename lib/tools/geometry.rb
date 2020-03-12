module Ruby417
  module Tools
    module Geometry
      class Point
        attr_accessor :x, :y

        def initialize(x, y)
          @x, @y = x, y
        end

        def distance(p)
          Math.hypot(x-p.x, y-p.y)
        end

        def rotate(axis, angle)
          Point.new(axis.x+(x-axis.x)*Math.cos(angle)-(y-axis.y)*Math.sin(angle),
                    axis.y+(x-axis.x)*Math.sin(angle)+(y-axis.y)*Math.cos(angle))
        end

        def ==(b)
          b.is_a?(Point) && (x - b.x).abs < 0.00001 && (y - b.y).abs < 0.00001
        end
      end
    end
  end
end
