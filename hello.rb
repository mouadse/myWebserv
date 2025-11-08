#!/usr/bin/env ruby

def env(name)
  ENV[name] || ''
end

puts "Content-Type: text/plain"
puts
puts "Hello from CGI 👋"
puts "REQUEST_METHOD = #{env('REQUEST_METHOD')}"
puts "QUERY_STRING   = #{env('QUERY_STRING')}"
puts "SCRIPT_NAME    = #{env('SCRIPT_NAME')}"
puts "CONTENT_TYPE   = #{env('CONTENT_TYPE')}"
puts "CONTENT_LENGTH = #{env('CONTENT_LENGTH')}"
