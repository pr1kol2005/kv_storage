#!/bin/bash

# ./tests/run_tests.sh --output-xml

GTEST_OUTPUT_XML=0

if [[ "$1" == "--output-xml" ]]; then
  GTEST_OUTPUT_XML=1
  mkdir -p tests/reports
fi

if [[ "$GTEST_OUTPUT_XML" == 1 ]]; then
  ./bin/unit_tests --gtest_output=xml:tests/reports/unit_tests_results.xml
else
  ./bin/unit_tests
fi

exit 0
