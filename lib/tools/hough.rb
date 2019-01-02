module Ruby417
  module Tool
    class HoughLines
      include Ruby417::Tool::Geometry

      class HoughLine < HessLine
        attr_accessor :votes

        def initialize(rho, theta, votes)
          @votes = votes
          super(rho, theta)
        end
      end

      MAX_THETA_VALUE  = Math::PI
      RHO_SCALE_FACTOR = 2

      attr_reader :rho_resolution, :theta_resolution, :max_rho_value

      def initialize(rho_resolution, theta_resolution, max_rho_value)
        @rho_resolution   = rho_resolution
        @theta_resolution = theta_resolution
        @max_rho_value    = max_rho_value
      end

      def self.maximum_rho(points)
        max = points.max_by { |p| p.x**2 + p.y**2 }
        (Math.sqrt(max.x**2+max.y**2)+1).to_i
      end

      def extract_lines(points, threshold:)
        lines = []

        accumulate_votes(points).each_with_index do |row, rho_index|
          row.each_with_index do |vote_count, theta_index|
            if vote_count >= threshold
              rho   = rho_value_from_index(rho_index)
              theta = theta_value_from_index(theta_index)
              lines << HoughLine.new(rho, theta, vote_count)
            end
          end
        end

        lines
      end

      def accumulate_votes(points)
        accumulator = create_zeros_array(rho_resolution*RHO_SCALE_FACTOR, theta_resolution)
        stored_thetas = expand_range(0, MAX_THETA_VALUE, theta_resolution)

        points.each do |point|
          stored_thetas.each_with_index do |theta, theta_index|
            rho_value = rho_given_theta_x_y(theta, point.x, point.y)
            rho_index = rho_index_from_value(rho_value)
            accumulator[rho_index][theta_index] += 1
          end
        end

        accumulator
      end

      def expand_range(min, max, resolution)
        resolution.times.map { |n| n*(max-min)/resolution.to_f + min }
      end

      def rho_index_from_value(value)
        value * rho_resolution / max_rho_value + rho_resolution
      end

      def rho_value_from_index(index)
        (index - rho_resolution) * max_rho_value / rho_resolution.to_f
      end

      def theta_value_from_index(index)
        index / theta_resolution.to_f * MAX_THETA_VALUE
      end

      def rho_given_theta_x_y(theta, x, y)
        x*Math.sin(theta) - y*Math.cos(theta)
      end

      def create_zeros_array(length, width)
        Array.new(length) { Array.new(width, 0) }
      end
    end
  end
end
