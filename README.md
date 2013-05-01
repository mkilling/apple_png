# apple_png [![Build Status](https://travis-ci.org/mkilling/apple_png.png?branch=master)](https://travis-ci.org/mkilling/apple_png) [![Gem Version](https://badge.fury.io/rb/apple_png.png)](http://badge.fury.io/rb/apple_png)

by Marvin Killing

## DESCRIPTION

Reads Apple's PNG format and converts it to plain PNG. This is useful when you're working with iOS .ipa packages from within Ruby.

This is an extension written in C, which runs about 100x faster than a similar approach in Ruby.

Tested on Ubuntu Oneiric and Mountain Lion. Not tested on Windows, but making it work there should not be too hard.

## USAGE

```ruby
require 'apple_png'

File.open('path/to/apple.png', 'rb') do |in|
  apple_png = ApplePng.new(in.read)
  puts apple_png.width
  puts apple_png.height

  File.open('path/to/standard.png', 'wb') do |out|
  	out.write(apple_png.data)
  end
end
```

## INSTALL

`gem install apple_png`

## LICENSE

(The MIT License)

Copyright (c) 2013

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
