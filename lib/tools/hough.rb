module Ruby417
  module Tools
    class Hough
      MAX_THETA_VALUE  = Math::PI
      RHO_SCALE_FACTOR = 2

      attr_reader :rho_resolution, :theta_resolution, :max_rho_value

      def initialize(rho_resolution, theta_resolution, max_rho_value)
        @rho_resolution   = rho_resolution
        @theta_resolution = theta_resolution
        @max_rho_value    = max_rho_value
      end

      def self.calculate_diagonal(points)
        max = points.max_by { |p| p.x**2 + p.y**2 }
        (Math.sqrt(max.x**2+max.y**2)+1).to_i
      end

      def accumulate_votes(points)
        accumulator = create_accumulator_array(rho_resolution*RHO_SCALE_FACTOR, theta_resolution)
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

      def find_lines(points, window:, threshold:)
        [].tap do |lines|
          accumulated = accumulate_votes(points)

          accumulated.each_with_index do |row, rho_index|
            row.each_with_index do |score, theta_index|
              if score >= threshold
                case test_maximality(accumulated, rho_index, theta_index, window)
                when :tie
                  accumulated[rho_index][theta_index] = 0
                when :max
                  rho   = rho_value_from_index(rho_index)
                  theta = theta_value_from_index(theta_index)
                  lines << HoughLine.new(rho, theta, score)
                end
              end
            end
          end

          accumulated.clear
        end
      end

      def test_maximality(accumulator, rho, theta, window)
        score = accumulator[rho][theta]
        rho_from,   rho_to   = window_range(rho, window, accumulator.length-1)
        theta_from, theta_to = window_range(theta, window, accumulator.first.length-1)

        rho_from.upto(rho_to) do |r|
          theta_from.upto(theta_to) do |t|
            if accumulator[r][t] > score
              return :not # => not a max at all
            elsif accumulator[r][t] == score && (r != rho || t != theta)
              return :tie # => not unique, though may or may not equal to max
            end
          end
        end

        :max # => local max
      end

      def window_range(x, window, max)
        [(x-window).clamp(0, max), (x+window).clamp(0, max)]
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

      def create_accumulator_array(length, width)
        Array.new(length) { Array.new(width, 0) }
      end
    end

    class HoughLine < Ruby417::Tools::Geometry::HesseLine
      attr_reader :score

      def initialize(rho, theta, score)
        @score = score
        super(rho, theta)
      end
    end
  end
end
