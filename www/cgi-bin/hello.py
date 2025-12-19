#!/usr/bin/python3
import os
import datetime

print("Content-Type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>Hello from CGI!</h1>")
print(f"<p>Time is: {datetime.datetime.now()}</p>")
print("<h2>Environment Variables:</h2><ul>")
for k, v in os.environ.items():
    print(f"<li><b>{k}</b>: {v}</li>")
print("</ul></body></html>")
