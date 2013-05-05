require 'apple_png/apple_png'

class NotValidApplePngError < StandardError; end

class ApplePng
  attr_accessor :width, :height
  attr_reader :raw_data

  # Create a new ApplePng instance from Apple PNG data in a string
  # @param apple_png_data [String] Binary string containing Apple PNG data, probably read from a file
  def initialize(apple_png_data)
    self.get_dimensions(apple_png_data)
    @raw_data = apple_png_data
  end

  # Get the PNG data as string. The conversion from Apple PNG data to standard PNG data will be performed when this method is first called.
  # @return [String] Binary string containing standard PNG data
  def data
    @data = self.convert_apple_png(@raw_data) if @data.nil?
    return @data
  end
end
