## THIS FILE WAS CREATED TO PRESERVE TESTING CODE FOR THE AdaptiveAnchorScan

require "mini_magick"
require "tempfile"
include Ruby417::Tool

def draw_pixels(pixels)
  pixels.map { |p| ["\e[1;37;40m ", "\e[0;40;37m░", "\e[1;40;37m░", "\e[0;40;37m▒", "\e[0;40;37m▓", "\e[1;40;37m▓", "\e[0m ", "\e[41m "][p*6/255]}.join + "\e[0m"
end

def image_from_pixels(pixels, name)
  Tempfile.open("blob") do |f|
    f.write(pixels.flatten.pack("C*"))
    f.rewind

    MiniMagick::Tool::Convert.new do |convert|
      convert.size "#{pixels.first.length}x#{pixels.length}"
      convert.depth 8
      convert << "rgb:#{f.path}"
      convert << "#{name}.jpg"
    end
  end
end

def detected_bars_to_image(pixels, anchors, name)
  new_pixels = pixels.map.with_index do |row, y|
    row_anchors = anchors.select { |a| a.y == y }

    row_pixels = row.map { |p| [p, p, p] }

    row_anchors.each do |a|
      row_pixels[a.left] = [0, 255, 0]
      # row_pixels[a.left+1] = [0, 255, 0]
      # row_pixels[a.right] = [255, 0, 0]
      row_pixels[a.right-1] = [255, 0, 0]
    end

    row_pixels
  end

  image_from_pixels(new_pixels, name)
  new_pixels.clear
end

def runlength(pixels, threshold)
  [0].tap do |result|
    last = pixels.first

    pixels.each do |v|
      if v < threshold == last < threshold
        result[-1] += 1
      else
        result << 1
        last = v
      end
    end
  end
end

pixels = MiniMagick::Image.open("samples/working.jpg").get_pixels.map { |row| a = row.map(&:first) }

intensity = ->(region) do
  black = region.min(2).sum / 2
  white = region.max(2).sum / 2
  gray  = (black + white) / 2
  range = white - black

  if range < 50
    threshold = 0
    deviation = 0
  else
    threshold = gray
    deviation = range / 16
  end

  AdaptiveThreshold.new(threshold, deviation)
end

qualifier = ->(left, anchor_width, right, pixels) do
  #   region = pixels.slice(right, anchor_width * 11 / 8)
  #   unit = anchor_width / 8
  #   left_pattern = [1, 1, 1, 1, 1, 1, 3, 1]
  #   score = 0
  #
  #   threshold = intensity.call(region).intensity
  #   bars = runlength(region, threshold)
  #   # puts left_pattern.inspect if left < 30
  # # puts bars.inspect if left < 30
  #   bars.each_with_index do |b, i|
  #     if i < left_pattern.length
  #       if (b - left_pattern[i]*unit) < unit / 2
  #         score += left_pattern[i]
  #       else
  #         score -= left_pattern[i]
  #       end
  #     else
  #       break
  #     end
  #   end
  # puts "#{score}\t#{left}" if score > 1
  #   score


  region = pixels[right..(right+anchor_width*2)]
  score = 0

  unless region.empty?
    intensity = (region.max(3) + region.min(3)).sum/6
    pattern = [1, 0, 1, 0, 1, 0, 1, 1, 1, 0]
    unit = anchor_width / 8

    pattern.each_with_index do |p, i|
      range = (i*unit-unit/4)..(i*unit+unit/4)

      if region[range]
        high = region[range].max(2).sum / 2
        low  = region[range].min(2).sum / 2
        real = region[range].sum / range.size

        # if real && (p == 1 && p == high / intensity) || (p == 0 && p == low / intensity)
        #   score += 2
        # else
        #   score -= 1
        # end

        if p == region[i*unit + unit / 2].to_i / intensity
          score += 2
        else
          score -= 1
        end

        # if high / intensity == p || low / intensity == p
        #   score += 2
        # else
        #   score -= 1
        # end
      end
    end
  end

  score
end

scanner = AdaptiveAnchorScan.new(calibration_resolution: [8],
                                 min_anchor_width: 4,
                                 threshold_calibrator: intensity,
                                 anchor_qualifier: qualifier,
                                 blurring: 2)

anchors = scanner.scan_image(pixels)
puts "ADAPTIVE SCANNER: #{anchors.length}"

def find_lines(points)
  xs = points.map(&:left)
  x_offset = xs.min
  y_offset = points.first.y
  width  = xs.max - x_offset
  height = points.last.y - y_offset + 1

  data = Array.new(width*height, 0)

  points.each do |p|
    data[width*(p.y - y_offset) + p.left - x_offset] = 255
  end

  im_hough_lines(data.pack("C*"), width, height)

  data.clear
end

def im_hough_lines(blob, w, h)
  # Tempfile.open("raw_blob") do |f|
  #   f.write(blob)
  #   f.rewind
  #
  #   MiniMagick::Tool::Convert.new do |convert|
  #     convert.size "#{w}x#{h}"
  #     convert.depth 8
  #     convert << "gray:#{f.path}"
  #     convert << "-hough-lines"<<"20x20+20"<<"test.mvg"
  #   end
  # end
  convert=MiniMagick::Tool::Convert.new# do |convert|
    convert.size "#{w}x#{h}"
    convert.depth 8
    convert << "gray:-"
    convert << "-hough-lines"<<"10x10+30"<<"test.mvg"
  #end
  convert.call(stdin: blob)
end

require "benchmark"
require_relative "geometry"
require_relative "hough"
print "ImageMagick:\t"
puts Benchmark.measure { find_lines(anchors) }
print "Pure Ruby:\t"
m=nil
puts Benchmark.measure {
  points = anchors.map { |a| Geometry::Point.new(a.left, a.y) }
  h = Ruby417::Tool::HoughLines.new(Ruby417::Tool::HoughLines.calculate_diagonal(points) / 8, 45, Ruby417::Tool::HoughLines.calculate_diagonal(points))
  m=h.extract_lines(points, 10, threshold: points.length / 5).sort_by &:votes
}
puts m.inspect
#puts anchors.map {|a| "Point.new(#{a.left}, #{a.y})"}.join(", ")
#detected_bars_to_image(pixels, anchors, "out")
