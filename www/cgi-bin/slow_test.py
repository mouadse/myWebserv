#!/usr/bin/env python3
"""
CGI timeout test script - sleeps longer than server timeout (5s)
Expected behavior: Server returns 504 Gateway Timeout
"""
import time
import sys

print("Content-Type: text/plain")
print()
sys.stdout.flush()

# Sleep longer than CGI_TIMEOUT_MS (5000ms)
time.sleep(10)

print("This line should never be seen")
