#!/bin/bash

echo "Launching clang-format..."

find include tests -name '*.hpp' -o -name '*.cpp' | while read file; do
  clang-format --style=Google -i "$file";
done

echo "✅ clang-format completed"
exit 0
