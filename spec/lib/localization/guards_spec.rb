require "spec_helper"

include Localization
include RectangleDetection

RSpec.describe Guards do
  let(:scanner) do
    Guards.new(preprocessing:     :full,
               area_threshold:    1000,
               fitting_threshold: 0.85,
               guard_aspect:      Range.new(3, 34),
               barcode_aspect:    Range.new(1, 8),
               angle_variation:   Math::PI/16,
               area_variation:    0.35)
  end

  describe "#run" do
    it "locates a solitary barcode" do
      barcodes = scanner.run("spec/fixtures/sir_walter_scott_blurred_rotated.jpg")
      barcode = barcodes.first

      expect(barcodes).to be_one
      expect(barcode.width).to be_within(3).of(609)
      expect(barcode.height).to be_within(3).of(225)
      expect(barcode.orientation).to eq(:horizontal)
    end
  end

  describe "#determine_barcode_location" do
    it "works for horizontal orientations" do
      rect1 = Rectangle.new(0, 0, 17, 183, Math::PI/6, 3000, 0, 0)
      rect2 = Rectangle.new(400, 300, 17, 183, Math::PI/6, 3000, 0, 0)
      barcode = scanner.determine_barcode_location(rect1, rect2)

      expect(barcode.orientation).to eq(:horizontal)
      expect(barcode.upper_left).to  eq(rect1.corners.min_by { |c| c.x + c.y })
      expect(barcode.lower_left).to  eq(rect1.corners.min_by { |c| c.x - c.y })
      expect(barcode.upper_right).to eq(rect2.corners.max_by { |c| c.x - c.y })
      expect(barcode.lower_right).to eq(rect2.corners.max_by { |c| c.x + c.y })

      backwards = scanner.determine_barcode_location(rect2, rect1)
      expect(backwards.upper_left).to  eq(barcode.upper_left)
      expect(backwards.lower_left).to  eq(barcode.lower_left)
      expect(backwards.upper_right).to eq(barcode.upper_right)
      expect(backwards.lower_right).to eq(barcode.lower_right)
    end

    it "works for vertical orientations" do
      rect1 = Rectangle.new(380, 100, 17, 183, Math::PI*2/5, 3000, 0, 0)
      rect2 = Rectangle.new(600, 800, 17, 183, Math::PI*2/5, 3000, 0, 0)
      barcode = scanner.determine_barcode_location(rect1, rect2)

      expect(barcode.orientation).to eq(:vertical)
      expect(barcode.upper_left).to  eq(rect1.corners.min_by { |c| c.x + c.y })
      expect(barcode.lower_left).to  eq(rect2.corners.min_by { |c| c.x - c.y })
      expect(barcode.upper_right).to eq(rect1.corners.max_by { |c| c.x - c.y })
      expect(barcode.lower_right).to eq(rect2.corners.max_by { |c| c.x + c.y })
    end

    it "handles edge cases" do
      rect1 = Rectangle.new(200, 200, 17, 183, Math::PI/4-0.1, 3000, 0, 0)
      rect2 = Rectangle.new(599, 600, 17, 183, Math::PI/4+0.1, 3000, 0, 0)
      barcode = scanner.determine_barcode_location(rect1, rect2)

      expect(barcode.orientation).to eq(:vertical)
      expect(barcode.upper_left).to  eq(rect1.corners.max_by { |c| c.y - c.x })
      expect(barcode.lower_left).to  eq(rect2.corners.max_by { |c| c.y - c.x })
      expect(barcode.upper_right).to eq(rect1.corners.min_by { |c| c.x + c.y })
      expect(barcode.lower_right).to eq(rect2.corners.max_by { |c| c.x + c.y })
    end
  end

  describe "#guard_pair?" do
    it "returns true for rectangle pairs that pass qualifications" do
      rect1 = Rectangle.new(0, 0, 17, 183, Math::PI/4, 3000, 0, 0)
      rect2 = Rectangle.new(400, 500, 17, 183, Math::PI/4, 3000, 0, 0)
      rect3 = Rectangle.new(400, 500, 20, 170, Math::PI*2/7, 2800, 0, 0)

      expect(scanner.guard_pair?(rect1, rect2, Math::PI/16, 1..8)).to be true
      expect(scanner.guard_pair?(rect1, rect3, Math::PI/16, 1..8)).to be true
    end

    it "returns false for bad rectangle pairs" do
      rect1 = Rectangle.new(0, 0, 17, 183, Math::PI/4, 3000, 0, 0)
      rect2 = Rectangle.new(400, 500, 17, 183, Math::PI/2, 3000, 0, 0)
      rect3 = Rectangle.new(400, 500, 17, 130, Math::PI/4, 3000, 0, 0)
      rect4 = Rectangle.new(100, 100, 17, 180, Math::PI/4, 3000, 0, 0)

      expect(scanner.guard_pair?(rect1, rect2, Math::PI/16, 1..8)).to be false
      expect(scanner.guard_pair?(rect1, rect3, Math::PI/16, 1..8)).to be false
      expect(scanner.guard_pair?(rect1, rect4, Math::PI/16, 1..8)).to be false
    end
  end

  describe "#similar_dimensions?" do
    it "returns true for similar rectangle dimensions" do
      rect1 = Rectangle.new(0, 0, 10, 170, Math::PI, 0, 0, 0)
      rect2 = Rectangle.new(0, 0, 14, 173, Math::PI, 0, 0, 0)

      expect(scanner.similar_dimensions?(rect1, rect2)).to be true
    end

    it "returns false for dissimilar rectangle dimensions" do
      rect1 = Rectangle.new(0, 0, 10, 170, Math::PI, 0, 0, 0)
      rect2 = Rectangle.new(0, 0, 30, 173, Math::PI, 0, 0, 0)

      expect(scanner.similar_dimensions?(rect1, rect2)).to be false
    end
  end

  describe "#oriented_well?" do
    it "returns true for edge guards that are oriented well" do
      rect1 = Rectangle.new(0, 0, 17, 183, Math::PI/4, 3000, 0, 0)
      rect2 = Rectangle.new(400, 500, 17, 183, Math::PI/4, 3000, 0, 0)

      expect(scanner.oriented_well?(rect1, rect2, Math::PI/16)).to be true
      expect(scanner.oriented_well?(rect2, rect1, Math::PI/16)).to be true
    end

    it "returns false for poorly oriented edge guards" do
      rect1 = Rectangle.new(0, 0, 17, 183, Math::PI/4, 3000, 0, 0)
      rect2 = Rectangle.new(400, 650, 17, 183, Math::PI/4, 3000, 0, 0)
      rect3 = Rectangle.new(400, 400, 17, 183, Math::PI/3, 3000, 0, 0)
      rect4 = Rectangle.new(400, 500, 17, 183, Math::PI/2, 3000, 0, 0)

      expect(scanner.oriented_well?(rect1, rect2, Math::PI/16)).to be false
      expect(scanner.oriented_well?(rect3, rect1, Math::PI/16)).to be false
      expect(scanner.oriented_well?(rect1, rect4, Math::PI/16)).to be false
    end
  end

  describe "#sized_well?" do
    it "returns true if edge guards are spaced well" do
      rect1 = Rectangle.new(0, 0, 17, 183, Math::PI, 3000, 0, 0)
      rect2 = Rectangle.new(600, 0, 17, 183, Math::PI, 3000, 0, 0)

      expect(scanner.sized_well?(rect1, rect2, 1..8)).to be true
    end

    it "returns false for badly spaced edge guards" do
      rect1 = Rectangle.new(0, 0, 17, 183, Math::PI, 3000, 0, 0)
      rect2 = Rectangle.new(100, 0, 17, 183, Math::PI, 3000, 0, 0)
      rect3 = Rectangle.new(1500, 0, 17, 183, Math::PI, 3000, 0, 0)

      expect(scanner.sized_well?(rect1, rect2, 1..8)).to be false
      expect(scanner.sized_well?(rect1, rect3, 1..8)).to be false
    end
  end

  describe "#matches_well?" do
    it "returns true for likely edge guards" do
      rect = Rectangle.new(0, 0, 17, 183, Math::PI, 3000, 0, 0)

      expect(scanner.matches_well?(rect, 0.9, 3..30)).to be true
      expect(scanner.matches_well?(rect, 0.95, 3..30)).to be true
    end

    it "returns false for unlikely edge guards" do
      rect1 = Rectangle.new(0, 0, 17, 18,  Math::PI, 300, 0, 0) # bad width-height ratio
      rect2 = Rectangle.new(0, 0, 17, 183, Math::PI, 300, 0, 0) # bad area

      expect(scanner.matches_well?(rect1, 0.9, 1..30)).to be true
      expect(scanner.matches_well?(rect1, 0.9, 3..30)).to be false
      expect(scanner.matches_well?(rect2, 0.9, 1..30)).to be false
    end
  end
end
