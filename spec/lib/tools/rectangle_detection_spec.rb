require "spec_helper"

RSpec.describe RectangleDetection do
  describe ".detect" do
    it "detects axis-oriented rectangles" do
      rectangles = RectangleDetection.detect("spec/fixtures/rectangles_1.png")

      expect(rectangles.length).to eq 2

      small, big = rectangles

      # at this time, we're still getting off-by-one errors
      expect(small.width).to be_within(1).of(300)
      expect(small.height).to be_within(1).of(290)
      expect(small.width * small.height).to eq(small.true_area);
      expect(small.orientation).to be_within(0.001).of(0.0)

      expect(big.width).to be_within(1).of(640)
      expect(big.height).to be_within(1).of(480)
      expect(big.orientation).to be_within(0.001).of(0.0)
    end

    it "detects arbitrarily rotated rectangles" do
      rectangles = RectangleDetection.detect("spec/fixtures/rectangles_2.png")

      expect(rectangles.length).to eq 3

      rect = rectangles.min_by(&:width)
      expect(rect.width).to be_within(1).of(200)
      expect(rect.height).to be_within(1).of(40)
      expect(rect.width * rect.height).to eq(rect.true_area)
      expect(rect.orientation).to be_within(0.001).of(Math::PI/12)
    end

    it "fits rectangles to features that aren't" do
      rectangles = RectangleDetection.detect("spec/fixtures/rectangles_2.png")

      expect(rectangles.length).to eq 3

      rect = rectangles.min_by(2, &:width).last
      expect(rect.width).to be_within(1).of(505)
      expect(rect.height).to be_within(1).of(94)
      expect(rect.width * rect.height).not_to eq(rect.true_area)
    end

    it "allows thresholding by region area and color" do
      rectangles = RectangleDetection.detect("spec/fixtures/rectangles_2.png", area_threshold: 9000)
      expect(rectangles.length).to eq(2)

      rectangles = RectangleDetection.detect("spec/fixtures/rectangles_2.png", area_threshold: 25000)
      expect(rectangles.length).to eq(1)

      rectangles = RectangleDetection.detect("spec/fixtures/rectangles_2.png", intensity_threshold: 128)
      expect(rectangles.length).to eq(1)
    end
  end
end
