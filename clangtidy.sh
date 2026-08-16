#!/bin/bash
clang-tidy --config-file=.clang-tidy --verify-config
clang-tidy --config-file=.clang-tidy -p build src/*.c
clang-tidy --config-file=.clang-tidy -p build src/*.h
clang-tidy --config-file=.clang-tidy -p build test/*.c