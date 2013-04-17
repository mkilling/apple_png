require 'rake/testtask'
#require 'rake/clean'
require 'rake/extensiontask'

NAME = 'apple_png'

Rake::ExtensionTask.new do |ext|
  ext.name = NAME                # indicate the name of the extension.
  ext.ext_dir = "ext/#{NAME}"         # search for 'hello_world' inside it.
  ext.lib_dir = "lib/#{NAME}"
  # ext.config_script = 'custom_extconf.rb' # use instead of the default 'extconf.rb'.
  # ext.tmp_dir = 'tmp'                     # temporary folder used during compilation.
  # ext.source_pattern = "*.{c,cpp}"        # monitor file changes to allow simple rebuild.
  # ext.config_options << '--with-foo'      # supply additional options to configure script.
  # ext.gem_spec = spec                     # optionally indicate which gem specification
  #                                         # will be used.
end


# make the :test task depend on the shared
# object, so it will be built automatically
# before running the tests
task :test => 'compile'

# use 'rake clean' and 'rake clobber' to
# easily delete generated files
# CLEAN.include('ext/**/*{.o,.log,.so,.bundle}')
# CLEAN.include('ext/**/Makefile')
# CLEAN.include('lib/**/*{.bundle,.so}')
# CLOBBER.include('lib/**/*{.bundle,.so}')

# the same as before
Rake::TestTask.new do |t|
  t.libs << 'test'
end

desc "Run tests"
task :default => :test