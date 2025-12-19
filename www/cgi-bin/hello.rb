#!/usr/bin/ruby
puts "Content-Type: text/html\r\n\r\n"
puts "<html><body>"
puts "<h1>Hello from Ruby CGI!</h1>"
puts "<p>Time is: #{Time.now}</p>"
puts "</body></html>"
