require 'minitest/autorun'
require 'apple_png'

class ApplePngTest < Minitest::Test
  def test_it_converts_apple_png_files_to_standard_png_files
    data = File.open('test/icon.apple.png', 'rb') do |f|
      f.read()
    end
    png_file = ApplePng.new(data)

    File.open('test/icon.png', 'rb') do |f|
      assert_equal f.read(), png_file.data
    end
  end

  def test_it_doesnt_crash_with_large_png_files
    data = File.open('test/icon.apple.png', 'rb') do |f|
      f.read()
    end
    png_file = ApplePng.new(data)
    png_file.data
  end

  def test_it_extracts_the_correct_sizes
    data = File.open('test/large.apple.png', 'rb') do |f|
      f.read()
    end
    png_file = ApplePng.new(data)

    assert_equal 640, png_file.width
    assert_equal 960, png_file.height
  end

  def test_it_raises_on_invalid_input
    assert_raises NotValidApplePngError do
      png_file = ApplePng.new("xyz")
    end
  end

  def test_it_raises_if_input_can_not_be_parsed_and_raw_data_can_still_be_accessed
    File.open('test/canabalt.png', 'rb') do |f|
      png = ApplePng.new(f.read)
      assert_raises NotValidApplePngError do
        png.data
      end
      refute_nil png.raw_data
    end
  end

  def test_it_raises_when_null
    assert_raises TypeError do
      ApplePng.new(nil).data
    end
  end
end