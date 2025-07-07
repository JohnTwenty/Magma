# Tests

This directory contains a lightweight test runner for the project.  For now it only provides a unit test for `String::endsWith`.

## Building with g++

From the repository root run:

```sh
g++ tests/Stubs.cpp Src/Foundation/String.cpp tests/StringTest.cpp tests/test_main.cpp -std=c++11 -I Src -o tests/magma_tests
```

Execute the resulting binary:

```sh
./tests/magma_tests --list            # list available tests
./tests/magma_tests --filter=String   # run tests containing 'String'
```

The program exits with 0 on success.
