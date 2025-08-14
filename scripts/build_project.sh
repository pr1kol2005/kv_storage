#!/bin/bash

echo "Starting the build..."

if [ "$1" == "BUILD_TESTS=ON" ]; then
  BUILD_TESTS="ON"
else
  BUILD_TESTS="OFF"
fi

cmake -B build -D BUILD_TESTS=$BUILD_TESTS

cmake --build build

if [ $? -eq 0 ]; then
  echo "✅ Build successful!"
  exit 0
else
  echo "❌ Build failed!"
  exit 1
fi
