#!/bin/bash

echo "Launching clang-format check..."

FAILED=0

FILES=$(find include tests -type f \( -name '*.cpp' -o -name '*.hpp' \))

for file in $FILES; do
  if ! diff -u "$file" <(clang-format --style=Google "$file"); then
    echo "File $file is not properly formatted"
    FAILED=1
  fi
done

if [[ $FAILED -ne 0 ]]; then
  echo "❌ clang-format check failed"
  exit 1
fi

echo "✅ clang-format check passed"
exit 0
