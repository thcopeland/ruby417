require "spec_helper"

RSpec.describe RectangleDetection::Rectangle do
  before(:all) do
    # there's probably a better way to do this
    Rectangle = RectangleDetection::Rectangle
    Point = Tools::Geometry::Point
  end

  describe "#point_transform" do
    it "maps a point from rectangle coordinates to image coordinates" do
      rect = Rectangle.new(25, 42, 8  , 8, Math::PI/4, 100, 0, 0)

      expect(rect.send(:point_transform, 0, 0)).to eq(Point.new(25, 42))
      expect(rect.send(:point_transform, -1, 0)).to eq(Point.new(25 - 2**-0.5, 42 - 2**-0.5))
      expect(rect.send(:point_transform, 1, 1)).to eq(Point.new(25, 42 + 2**0.5))
    end
  end

  describe "corners" do
    let(:rect1) { Rectangle.new(0, 0, 8*Math.sqrt(3), 8, Math::PI/8, 0, 0, 0) }
    let(:rect2) { Rectangle.new(0, 0, 8*Math.sqrt(3), 8, Math::PI/3, 0, 0, 0) }
    let(:half_diagonal) { Math.hypot(rect1.width, rect1.height) / 2 }

    describe "#upper_left" do
      it "returns upper left corner" do
        expect(rect1.upper_left).to eq(Point.new(-half_diagonal*Math.cos(Math::PI/6 + rect1.orientation),
                                                 -half_diagonal*Math.sin(Math::PI/6 + rect1.orientation)))
        expect(rect2.upper_left).to eq(Point.new(-half_diagonal*Math.cos(Math::PI/6 - rect2.orientation),
                                                 +half_diagonal*Math.sin(Math::PI/6 - rect2.orientation)))
      end
    end

    describe "#upper_right" do
      it "returns upper right corner" do
        expect(rect1.upper_right).to eq(Point.new(+half_diagonal*Math.cos(Math::PI/6 - rect1.orientation),
                                                  -half_diagonal*Math.sin(Math::PI/6 - rect1.orientation)))
        expect(rect2.upper_right).to eq(Point.new(-half_diagonal*Math.cos(Math::PI/6 + rect2.orientation),
                                                  -half_diagonal*Math.sin(Math::PI/6 + rect2.orientation)))
      end
    end

    describe "#lower_left" do
      it "returns lower left corner" do
        expect(rect1.lower_left).to eq(Point.new(-half_diagonal*Math.cos(Math::PI/6 - rect1.orientation),
                                                 +half_diagonal*Math.sin(Math::PI/6 - rect1.orientation)))
        expect(rect2.lower_left).to eq(Point.new(-half_diagonal*Math.cos(Math::PI/6 + rect2.orientation),
                                                 +half_diagonal*Math.sin(Math::PI/6 + rect2.orientation)))
      end
    end

    describe "#lower_right" do
      it "returns lower right corner" do
        expect(rect1.lower_right).to eq(Point.new(+half_diagonal*Math.cos(Math::PI/6 + rect1.orientation),
                                                  +half_diagonal*Math.sin(Math::PI/6 + rect1.orientation)))
        expect(rect2.lower_right).to eq(Point.new(+half_diagonal*Math.cos(Math::PI/6 - rect2.orientation),
                                                  -half_diagonal*Math.sin(Math::PI/6 - rect2.orientation)))
      end
    end
  end
end
