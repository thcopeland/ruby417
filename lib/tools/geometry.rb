module Ruby417
  module Tools
    module Geometry
      Point = Struct.new(:x, :y)

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
