require "spec_helper"

include Localization

RSpec.describe Guards do
  describe "#run" do
    it "locates a solitary barcode" do
      codes = Guards.new.run("spec/fixtures/sir_walter_scott_blurred_rotated.jpg")

      expect(codes).to be_one
      expect(codes.first.width).to be_within(3).of(609)
      expect(codes.first.height).to be_within(3).of(225)
    end
  end
end
