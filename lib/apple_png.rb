require 'apple_png/apple_png'

class ApplePng
  attr_accessor :width, :height

  def initialize(apple_png_data)
    self.get_dimensions(apple_png_data)
    @raw_data = apple_png_data
  end

  def data
    if @data.nil?
      @data = self.convert_apple_png(@raw_data)
      @raw_data = nil
    end
    return @data
  end
end
