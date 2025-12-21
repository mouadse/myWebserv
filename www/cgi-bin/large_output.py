#!/usr/bin/env python3
"""
CGI large output test script - produces output exceeding buffer cap
Expected behavior: Server returns 504 or truncated response (not crash)
"""
import sys

print("Content-Type: text/plain")
print()
sys.stdout.flush()

# Output ~20MB (exceeds 16MB cap)
for i in range(200000):
    print("x" * 100)
