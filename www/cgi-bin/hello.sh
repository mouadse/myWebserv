#!/bin/bash
printf "Content-Type: text/html\r\n\r\n"
printf "<html><body>"
printf "<h1>Hello from Bash CGI!</h1>"
printf "<p>Time is: %s</p>" "$(date)"
sleep 4
printf "</body></html>"
