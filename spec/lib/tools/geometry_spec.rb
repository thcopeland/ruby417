require "spec_helper"

include Tools
include Geometry

RSpec.describe Geometry do
  describe Point do
    let(:point) { Point.new(10, 11) }

    describe "#distance" do
      it "calculates Euclidian distance between two points" do
        expect(point.distance(Point.new(3, 2))).to be_within(0.0001).of(11.40175)
      end
    end

    describe "#rotate" do
      it "should rotate the point about an axis" do
        include Geometry
        expect(point.rotate(Point.new(6, 7), Math::PI/4)).to eq(Point.new(6, 7+4*Math.sqrt(2)))
      end
    end
  end
end
