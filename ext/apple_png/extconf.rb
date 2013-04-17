require 'mkmf'

if have_library('z', 'inflate')
  create_makefile('apple_png/apple_png')
else
  puts "Could not find zlib library"
end