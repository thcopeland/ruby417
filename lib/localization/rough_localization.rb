module Ruby417
  module Localization
    include Ruby417::Tools::Geometry

    class RoughLocalization
      def estimate_guard_location_from_points(points, window_size, guard_side)
        distribution = point_distribution(points, window_size)
        threshold    = calculate_threshold(distribution, points.last.y - points.first.y, window_size, points.length)
        thresholded  = threshold_area_distribution(distribution, threshold)
        longest_run  = find_longest_run(distribution, thresholded)
        endpoints    = guard_endpoints(points, distribution, longest_run)

        fuzz_endpoints(endpoints, threshold, guard_side == :left ? -1 : 1)
      end

      def fuzz_endpoints(endpoints, threshold, horizontal_fuzzing_direction)
        upper_endpoint, lower_endpoint = endpoints
        horizontal_fuzzing = threshold * (upper_endpoint.x - lower_endpoint.x).abs / (lower_endpoint.y - upper_endpoint.y)
        vertical_fuzzing   = threshold

        [Point.new(upper_endpoint.x + horizontal_fuzzing*horizontal_fuzzing_direction, upper_endpoint.y - vertical_fuzzing),
         Point.new(lower_endpoint.x + horizontal_fuzzing*horizontal_fuzzing_direction, lower_endpoint.y + vertical_fuzzing)]
      end

      def guard_endpoints(points, distribution, longest_run)
        [points[distribution[0..longest_run.first-1].sum],
         points[distribution[0..longest_run.last-1].sum-1]]
      end

      def find_longest_run(distribution, thresholded_distribution)
        runs(thresholded_distribution).max_by { |run| run_length(run) }
      end

      def run_length(run)
        run.last - run.first
      end

      def runs(thresholded_distribution)
        [].tap do |runs|
          index = 0

          while index < thresholded_distribution.length
            if thresholded_distribution[index]
              runs << [index, consume_truthy(thresholded_distribution, index)]
              index = runs[-1][-1]
            end

            index += 1
          end
        end
      end

      def consume_truthy(array, index)
        index += 1 while array[index]; index
      end

      def threshold_area_distribution(distribution, threshold)
        distribution.map { |count| count >= threshold }
      end

      def calculate_threshold(distribution, vertical_range, window_size, point_count)
        (window_size * point_count / vertical_range + distribution.max(3).min) / 2 - distribution.min(3).max
      end

      def point_distribution(points, window_size)
        [].tap do |counts|
          consumed_index = 0
          window_limit = window_size

          while consumed_index < points.length
            counts << consume_within_window(points, consumed_index, window_limit) - consumed_index
            consumed_index += counts.last
            window_limit   += window_size
          end
        end
      end

      def consume_within_window(points, offset, limit)
        offset += 1 while offset < points.length && points[offset].y < limit; offset
      end
    end
  end
end
