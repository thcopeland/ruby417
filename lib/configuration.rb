module Ruby417
  class Configuration
    extend Utils::AttrMethods

    attr_accessor_with_default :localization_method, :guards # (perhaps :hough, :morphology in the future)

    attr_accessor_with_default :localization_strictness, :basic # :lax, :strict

    attr_accessor_with_default :localization_preprocessing, :full # :none, :half, :full

    attr_accessor_with_calc :localization_guard_area_threshold do
      { lax: 0.0003, basic: 0.0007, strict: 0.001 }[localization_strictness]
    end

    attr_accessor_with_calc :localization_guard_aspect do
      { lax: 2..50, basic: 3..40, strict: 3..40 }[localization_strictness]
    end

    attr_accessor_with_calc :localization_barcode_aspect do
      { lax: 1..20, basic: 2..10, strict: 3..10 }[localization_strictness]
    end

    attr_accessor_with_calc :localization_guard_rectangularity_threshold do
      { lax: 0.5, basic: 0.8, strict: 0.9 }[localization_strictness]
    end

    attr_accessor_with_calc :localization_angle_variation_threshold do
      { lax: Math::PI/8, basic: Math::PI/16, strict: Math::PI/32 }[localization_strictness]
    end

    attr_accessor_with_calc :localization_guard_area_variation_threshold do
      { lax: 0.5, basic: 0.4, strict: 0.2 }[localization_strictness]
    end

    attr_accessor_with_calc :localization_guard_width_variation_threshold do
      { lax: 0.5, basic: 0.3, strict: 0.2 }[localization_strictness]
    end

    attr_accessor_with_calc :localization_guard_height_variation_threshold do
      { lax: 0.2, basic: 0.1, strict: 0.05 }[localization_strictness]
    end
  end

  class << self
    def configuration
      @configuration ||= Configuration.new
    end

    def config
      yield configuration
    end
  end
end
