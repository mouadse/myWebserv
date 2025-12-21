#!/usr/bin/env python3
import sys
import os
import html

# 1. Header
print("Content-Type: text/html\r\n\r\n")

def get_multipart_data():
    content_type = os.environ.get('CONTENT_TYPE', '')
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    
    if 'multipart/form-data' not in content_type or content_length == 0:
        return {}

    # Extract the boundary string
    # Content-Type looks like: multipart/form-data; boundary=----WebKitFormBoundary...
    try:
        boundary = content_type.split("boundary=")[1].strip()
        boundary = ("--" + boundary).encode()
    except IndexError:
        return {}

    # Read raw bytes from stdin
    raw_data = sys.stdin.buffer.read(content_length)
    
    # Split by boundary
    parts = raw_data.split(boundary)
    parsed_results = {}

    for part in parts:
        if not part or part == b'--\r\n' or part == b'--':
            continue
        
        # Split headers from body (separated by \r\n\r\n)
        if b'\r\n\r\n' in part:
            head, body = part.split(b'\r\n\r\n', 1)
            head = head.decode(errors='ignore')
            
            # Extract name from: Content-Disposition: form-data; name="Name"
            if 'name="' in head:
                name = head.split('name="')[1].split('"')[0]
                # Clean up the body (remove trailing \r\n)
                value = body.rstrip(b'\r\n').decode(errors='ignore')
                parsed_results[name] = value

    return parsed_results

# Execute Parsing
data = get_multipart_data()

# Prepare Output
table_rows = ""
if data:
    for key, val in data.items():
        table_rows += f"<tr><td><strong>{html.escape(key)}</strong></td><td>{html.escape(val)}</td></tr>"
else:
    table_rows = "<tr><td colspan='2'>No data found or encoding mismatch.</td></tr>"

# Final HTML
print(f"""
<!DOCTYPE html>
<html>
<head>
    <style>
        body {{ font-family: sans-serif; padding: 20px; background: #f4f4f4; }}
        .box {{ background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }}
        table {{ width: 100%; border-collapse: collapse; margin-top: 10px; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
    </style>
</head>
<body>
    <div class="box">
        <h1>Manual Multipart Parser</h1>
        <table>
            <tr><th>Field</th><th>Value</th></tr>
            {table_rows}
        </table>
    </div>
</body>
</html>
""")
