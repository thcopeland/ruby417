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

      class HesseLine
        attr_accessor :rho, :theta

        def initialize(rho, theta)
          @rho, @theta = rho, theta # x*sin(theta) - y*cos(theta) = rho
        end

        def to_std
          StandardLine.new(Math.sin(theta), -Math.cos(theta), -rho)
        end
      end

      class StandardLine
        attr_accessor :a, :b, :c

        def initialize(a, b, c)
          @a, @b, @c = a, b, c # a*x + b*y + c = 0
        end

        def normal_distance_to(point)
          (a*point.x + b*point.y + c).abs/Math.sqrt(a**2 + b**2)
        end
      end
    end
  end
end
