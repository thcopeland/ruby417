require "spec_helper"

include Tools::Geometry
include RectangleDetection

RSpec.describe Rectangle do
  describe "#corners" do
    it "calculates the corners of an axis-aligned rectangle" do
      rect = Rectangle.new(30, 30, 8, 8, 0.0, 0, 0, 0)
      upper_left  = Point.new(rect.cx-rect.width/2, rect.cy-rect.height/2)
      upper_right = Point.new(rect.cx+rect.width/2, rect.cy-rect.height/2)
      lower_right = Point.new(rect.cx-rect.width/2, rect.cy+rect.height/2)
      lower_left  = Point.new(rect.cx+rect.width/2, rect.cy+rect.height/2)

      expect(rect.corners).to match_array [upper_left, upper_right, lower_right, lower_left]
    end

    it "calculates the corners of arbitrarily rotated rectangle" do
      rect = Rectangle.new(0, 0, 8*Math.sqrt(3), 8, Math::PI/3, 0, 0, 0)

      # Math::PI/6 is a consequence of the rectangle's dimensions, the orientation
      # of the rectangle, however, is arbitrary.
      upper_left  = Point.new(-8*Math.cos(Math::PI/6+rect.orientation), -8*Math.sin(Math::PI/6+rect.orientation))
      upper_right = Point.new(+8*Math.cos(Math::PI/6-rect.orientation), -8*Math.sin(Math::PI/6-rect.orientation))
      lower_right = Point.new(+8*Math.cos(Math::PI/6+rect.orientation), +8*Math.sin(Math::PI/6+rect.orientation))
      lower_left  = Point.new(-8*Math.cos(Math::PI/6-rect.orientation), +8*Math.sin(Math::PI/6-rect.orientation))

      expect(rect.corners).to match_array [upper_left, upper_right, lower_right, lower_left]
    end
  end
end
