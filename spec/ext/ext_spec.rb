require "spec_helper"
require "open3"

RSpec.describe Ext do
  describe ".locate_via_guards" do
    it "locates and scores potential barcodes" do
      data = File.read("spec/fixtures/256x256_assorted_rectangles.raw")
      codes = Ext.locate_via_guards(data, 256, 256, 100, 0.8, 0.314, 0.5, 0.4, 0.3, 3, 50, 0, 10)
      code1, code2 = codes

      expect(codes.length).to eq(2)
      expect(code1.first).to be > code2.first
      expect(code1[1..]).to match_array([21, 19, 15, 111, 164, 124, 166, 28])
      expect(code2[1..]).to match_array([99, 148, 15, 190, 52, 259, 125, 240])
    end
  end

  describe "C tests" do
    it "run successfully" do
      expect(Open3.capture2e("#{__dir__}/run_suite.sh").last).to be_success
    end
  end
end
