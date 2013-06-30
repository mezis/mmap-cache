# -*- encoding: utf-8 -*-
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'mmap/cache/version'

Gem::Specification.new do |gem|
  gem.name          = "mmap-cache"
  gem.version       = Mmap::Cache::VERSION
  gem.authors       = ["Julien Letessier"]
  gem.email         = ["julien.letessier@gmail.com"]
  gem.description   = %q{Fast shared memory LRU cache}
  gem.summary       = %q{Fast shared memory LRU cache}
  gem.homepage      = "http://github.com/mezis/mmap-cache"

  gem.extensions    = ['ext/mmap/cache/extconf.rb']
  gem.files         = Dir.glob('lib/**/*.rb') +
                      Dir.glob('ext/**/*.{c,h,rb}') +
                      Dir.glob('*.{md,txt}')
  gem.executables   = gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.test_files    = gem.files.grep(%r{^spec/})
  gem.require_paths = ["lib"]
end
