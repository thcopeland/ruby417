module Ruby417
  module Utils
    module AttrMethods
      def attr_reader_with_default(option, default)
        define_method(option) do
          val = instance_variable_get("@#{option}")
          val.nil? ? default : val
        end
      end

      def attr_accessor_with_default(option, default)
        attr_writer option
        attr_reader_with_default option, default
      end

      def attr_reader_with_calc(option, &block)
        define_method(option) do
          val = instance_variable_get("@#{option}")
          val.nil? ? instance_exec(&block) : val
        end
      end

      def attr_accessor_with_calc(option, &block)
        attr_writer option
        attr_reader_with_calc option, &block
      end
    end
  end
end
