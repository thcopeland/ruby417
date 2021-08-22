require "spec_helper"

include Localization

RSpec.describe LocatedBarcode do
  let(:code) { LocatedBarcode.new(0, Point.new(0, 0), Point.new(0, 1), Point.new(5, 2), Point.new(4, 1)) }

  describe "#width" do
    it "should be the largest horizontal side" do
      expect(code.width).to eq(Point.new(0, 1).distance(Point.new(5, 2)))
    end
  end

  describe "#height" do
    it "should be the largest vertical side" do
      expect(code.height).to eq(Point.new(4, 1).distance(Point.new(5, 2)))
    end
  end
end

RSpec.describe Point do
  describe "#distance" do
    it "should return the Euclidean distance between two points" do
      expect(Point.new(1, 2).distance(Point.new(5, 5))).to eq(5)
    end
  end
end
