#!/bin/bash

# Binary POST Support Test Script
# This script tests raw binary POST requests to the webserver

set -e

echo "=== Binary POST Support Test ==="
echo

# Configuration
SERVER_URL="http://127.0.0.1:8085"
TEST_DIR="/tmp/webserv_binary_test"
UPLOAD_DIR="www/uploads"

# Clean and create test directory
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
mkdir -p "$UPLOAD_DIR"

# Test 1: Binary file upload (image)
echo "Test 1: Uploading binary image file..."
dd if=/dev/urandom of="$TEST_DIR/test_image.bin" bs=1024 count=10 2>/dev/null
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@$TEST_DIR/test_image.bin" \
  "$SERVER_URL/uploads/test_image.bin")

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | head -n-1)

if [ "$HTTP_CODE" = "201" ]; then
  echo "✓ Binary upload succeeded (HTTP 201)"
  if [ -f "$UPLOAD_DIR/test_image.bin" ]; then
    echo "✓ File exists at target location"
    # Verify file content matches
    if cmp -s "$TEST_DIR/test_image.bin" "$UPLOAD_DIR/test_image.bin"; then
      echo "✓ File content matches original"
    else
      echo "✗ File content does not match!"
      exit 1
    fi
  else
    echo "✗ File not found at target location"
    exit 1
  fi
else
  echo "✗ Binary upload failed (HTTP $HTTP_CODE)"
  echo "Response: $BODY"
  exit 1
fi
echo

# Test 2: Text file upload
echo "Test 2: Uploading text file as binary..."
echo "Hello, World! This is a test." > "$TEST_DIR/test.txt"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST \
  -H "Content-Type: text/plain" \
  --data-binary "@$TEST_DIR/test.txt" \
  "$SERVER_URL/uploads/test.txt")

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)

if [ "$HTTP_CODE" = "201" ]; then
  echo "✓ Text file upload succeeded (HTTP 201)"
  if cmp -s "$TEST_DIR/test.txt" "$UPLOAD_DIR/test.txt"; then
    echo "✓ File content matches original"
  else
    echo "✗ File content does not match!"
    exit 1
  fi
else
  echo "✗ Text file upload failed (HTTP $HTTP_CODE)"
  exit 1
fi
echo

# Test 3: POST to directory should fail with 400
echo "Test 3: Attempting to POST to directory (should fail)..."
mkdir -p "$UPLOAD_DIR/testdir"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST \
  -H "Content-Type: application/octet-stream" \
  --data-binary "test data" \
  "$SERVER_URL/uploads/testdir")

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)

if [ "$HTTP_CODE" = "400" ]; then
  echo "✓ Directory POST correctly rejected (HTTP 400)"
else
  echo "✗ Directory POST should return 400, got HTTP $HTTP_CODE"
  exit 1
fi
echo

# Test 4: POST to path ending with '/' should fail with 400
echo "Test 4: Attempting to POST to path ending with '/' (should fail)..."
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST \
  -H "Content-Type: application/octet-stream" \
  --data-binary "test data" \
  "$SERVER_URL/uploads/testfile/")

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)

if [ "$HTTP_CODE" = "400" ]; then
  echo "✓ Path ending with '/' correctly rejected (HTTP 400)"
else
  echo "✗ Path ending with '/' should return 400, got HTTP $HTTP_CODE"
  exit 1
fi
echo

# Test 5: Empty binary POST
echo "Test 5: Uploading empty file..."
touch "$TEST_DIR/empty.bin"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@$TEST_DIR/empty.bin" \
  "$SERVER_URL/uploads/empty.bin")

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)

if [ "$HTTP_CODE" = "201" ]; then
  echo "✓ Empty file upload succeeded (HTTP 201)"
  if [ -f "$UPLOAD_DIR/empty.bin" ]; then
    echo "✓ Empty file created at target location"
    SIZE=$(stat -c%s "$UPLOAD_DIR/empty.bin" 2>/dev/null || stat -f%z "$UPLOAD_DIR/empty.bin" 2>/dev/null)
    if [ "$SIZE" = "0" ]; then
      echo "✓ File is empty as expected"
    else
      echo "✗ File should be empty but has size $SIZE"
      exit 1
    fi
  else
    echo "✗ Empty file not found at target location"
    exit 1
  fi
else
  echo "✗ Empty file upload failed (HTTP $HTTP_CODE)"
  exit 1
fi
echo

# Cleanup
echo "Cleaning up test files..."
rm -rf "$TEST_DIR"
rm -rf "$UPLOAD_DIR"

echo
echo "=== All Tests Passed! ==="
