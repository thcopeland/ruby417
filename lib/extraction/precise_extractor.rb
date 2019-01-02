module Ruby417
  module Extraction
    class PDF417EdgeGuardLocator
      include Ruby417::Tool

      attr_reader :edge, :black_threshold, :scan_guard_qualifier, :scan_min_anchor_width, :scan_resolution, :trace_distance_threshold, :trace_resolution, :collinearity_distance_threshold, :hough_line_threshold, :hough_rho_resolution, :hough_theta_resolution

      def initialize(edge:, black_threshold:, scan_guard_qualifier:, scan_min_anchor_width:, scan_resolution:, trace_distance_threshold:, trace_resolution:, collinearity_distance_threshold:, hough_line_threshold:, hough_rho_resolution:, hough_theta_resolution:)
        @edge = edge
        @black_threshold = black_threshold

        @scan_guard_qualifier  = scan_guard_qualifier
        @scan_min_anchor_width = scan_min_anchor_width
        @scan_resolution       = scan_resolution

        @trace_distance_threshold = trace_distance_threshold
        @trace_resolution         = trace_resolution

        @collinearity_distance_threshold = collinearity_distance_threshold

        @hough_line_threshold   = hough_line_threshold
        @hough_rho_resolution   = hough_rho_resolution
        @hough_theta_resolution = hough_theta_resolution
      end

      def locate_edge_guard(pixels)
        guard_candidates = scan_for_guard(pixels)
        best_guard_line  = approximate_guard_line(guard_candidates)
        points_on_guard  = points_near_line(best_guard_line, guard_candidates)
        lower_start_point = points_on_guard[points_on_guard.length*2/3]
        upper_start_point = points_on_guard[points_on_guard.length / 3]

        trace_delta = pixels.length / trace_resolution

        [
          trace_guard(pixels, upper_start_point, best_guard_line, -trace_delta),
          trace_guard(pixels, lower_start_point, best_guard_line, +trace_delta)
        ]
      end

      def scan_for_guard(pixels)
        AnchoredScan.new(black_threshold:  black_threshold,
                         min_anchor_width: scan_min_anchor_width,
                         qualifier:  scan_guard_qualifier,
                         resolution: scan_resolution,
                         edge: edge).scan(pixels)
      end

      def approximate_guard_line(points)
        transform = HoughLines.new(hough_rho_resolution, hough_theta_resolution, HoughLines.maximum_rho(points))
        transform.extract_lines(points, threshold: hough_line_threshold).max_by(&:votes).to_std
      end

      def trace_guard(pixels, starting_point, guide_line, delta_y)
        GuardTracer.new(black_threshold:    black_threshold,
                        distance_threshold: trace_distance_threshold,
                        min_anchor_width:   scan_min_anchor_width,
                        resolution: trace_resolution,
                        edge: edge).trace(pixels, starting_point, guide_line, delta_y)
      end

      def points_near_line(line, points)
        points.select { |p| line.normal_distance_to(p) <= collinearity_distance_threshold }
      end
    end


    class GuardTracer
      include Ruby417::Tool::Geometry

      attr_reader :black_threshold, :distance_threshold, :min_anchor_width, :resolution, :edge

      def initialize(black_threshold:, distance_threshold:, min_anchor_width:, resolution:, edge:)
        @black_threshold    = black_threshold
        @distance_threshold = distance_threshold
        @min_anchor_width   = min_anchor_width
        @resolution = resolution
        @edge = edge
      end

      def trace(pixels, point, guide, dy)
        while point.y.between?(0, pixels.length-1) && guard_anchor_found?(pixels[point.y], point)
          last  = point
          point = next_point(point, guide, dy)
        end

        Point.new(best_anchor(pixels[last.y], last), last.y)
      end

      def guard_anchor_found?(pixel_row, point)
        guard_anchors(pixel_row, point).any? { |x| (x - point.x).abs <= distance_threshold }
      end

      def best_anchor(pixel_row, point)
        guard_anchors(pixel_row, point).min_by { |x| (x-point.x).abs }
      end

      def guard_anchors(pixel_row, point)
        AnchoredScan.new(black_threshold: black_threshold,
                         min_anchor_width: min_anchor_width,
                         qualifier: ->(*a){ true },
                         resolution: nil,
                         edge: edge).scan_row(pixel_row, *scan_range(pixel_row, point))
      end

      def next_point(current, guide, dy)
        Point.new(guide.x_given_y(current.y + dy).to_i, current.y + dy)
      end

      def scan_range(pixel_row, point)
        if edge == :left
          range = [point.x - min_anchor_width * 2, point.x + min_anchor_width]
        else
          range = [point.x - min_anchor_width, point.x + min_anchor_width * 2]
        end

        range.map { |n| n.clamp(0, pixel_row.length-1) }
      end
    end


    class AnchoredScan
      include Ruby417::Tool::Geometry

      attr_reader :black_threshold, :min_anchor_width, :qualifier, :resolution, :edge

      def initialize(black_threshold:, min_anchor_width:, qualifier:, resolution:, edge:)
        @black_threshold  = black_threshold
        @min_anchor_width = min_anchor_width
        @qualifier  = qualifier
        @resolution = resolution
        @edge = edge
      end

      def scan(pixels)
        resolution.times.flat_map do |i|
          y = pixels.length * i / resolution
          scan_row(pixels[y]).map { |x| Point.new(x, y) }
        end
      end

      def scan_row(pixel_row, from=0, to=pixel_row.length)
        p = from
        indexes = []

        while p < to
          if dark?(pixel_row[p])
            q = consume_dark(pixel_row, p)
            anchor_width = q - p

            if anchor_width >= min_anchor_width
              left  = pixel_row.slice((p-anchor_width).clamp(0, p), anchor_width)
              right = pixel_row.slice(q, anchor_width)

              if qualifier.call(left, anchor_width, right)
                indexes << (edge == :left ? p : q)
              end
            end

            p = q + 1
          else
            p = consume_bright(pixel_row, p)
          end
        end

        indexes
      end

      def consume_dark(pixels, p)
        p += 1 while p < pixels.length && dark?(pixels[p]); p
      end

      def consume_bright(pixels, p)
        p += 1 while p < pixels.length && bright?(pixels[p]); p
      end

      def dark?(intensity)
        intensity <= black_threshold
      end

      def bright?(intensity)
        intensity > black_threshold
      end
    end
  end
end
