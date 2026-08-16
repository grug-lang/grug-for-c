#!/bin/bash
alltests=$(for dir in grug-tests/tests/{err,err_mod_api,err_runtime,err_spaces,ok}; do for file in "$dir"/*; do basename "$file"; done; done)

total_tests=0
tests_passed=0

for test in $alltests; do

if ./build/test_harness $test &> /dev/null; then
echo "$test -> success"
total_tests=$((total_tests + 1))
tests_passed=$((tests_passed + 1))
else
echo "$test -> failed"
total_tests=$((total_tests + 1))
fi

done

echo "$tests_passed / $total_tests passed"
