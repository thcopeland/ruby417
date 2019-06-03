module Ruby417
  module Tools
    class Collinearity
      attr_reader :hough_rho_resolution_factor, :hough_theta_resolution_factor, :hough_search_window_size

      def initialize(hough_rho_resolution_factor, hough_theta_resolution_factor, hough_search_window_size)
        @hough_rho_resolution_factor   = hough_rho_resolution_factor
        @hough_theta_resolution_factor = hough_theta_resolution_factor
        @hough_search_window_size      = hough_search_window_size
      end

      def find_collinear_points(points, line_strength_threshold:, distance_threshold:)
        find_lines(points, line_strength_threshold).sort_by(&:score).map do |hesse_line|
          std_line = hesse_line.to_std
          points.select { |p| std_line.normal_distance_to(p) <= distance_threshold }
        end
      end

      def find_lines(points, threshold)
        Hough.new((hough_rho_resolution_factor * base_hough_rho_resolution(points)).to_i,
                  (hough_theta_resolution_factor * base_hough_theta_resolution).to_i,
                  Hough.calculate_diagonal(points)).find_lines(points, window: hough_search_window_size, threshold: threshold)
      end

      def base_hough_rho_resolution(points)
        Hough.calculate_diagonal(points)
      end

      def base_hough_theta_resolution
        180 # => increments of 1 degree
      end
    end
  end
end
