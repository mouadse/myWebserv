#!/usr/bin/env python3
import os
import html

print("Content-Type: text/plain")
print()
print("Hello from CGI 👋")
print(f"REQUEST_METHOD = {os.environ.get('REQUEST_METHOD','')}")
print(f"QUERY_STRING   = {os.environ.get('QUERY_STRING','')}")
print(f"SCRIPT_NAME    = {os.environ.get('SCRIPT_NAME','')}")
print(f"CONTENT_TYPE   = {os.environ.get('CONTENT_TYPE','')}")
print(f"CONTENT_LENGTH = {os.environ.get('CONTENT_LENGTH','')}")

