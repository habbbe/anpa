## parsimon 

A C++17 header only generic monadic parser combinator library loosely based on Haskell's parsec. 

### Features

All parsers and combinators are `constexpr` meaning no run time cost for constructing a parser.

In addition, all parsers and combinators, with two exceptions (`many_to_vector` and `many_to_map`. 
Will this change with C++20?), can be evaluated at compile time.
This enables compile time parsing as long as dereferencing and incrementing the input iterator are 
`constexpr` operations.

### Examples

See the provided test parsers
- [JSON parser](test/json/json_parser.h): barebones but functional JSON parser. It's only ~30 LOC and gives
a good overview on how to use the library, including recursive parsers.
- [Simple syntax parser](test/tests_perf.cpp): a parser for a simple example syntax inteded for an application
launcher/information dashboard.
- [Expression parser](test/calc/calc.h): a simple expression evaluator supporting basic arithmetic operations
with correct precedence. This example showcases how to use `chain` to eliminate left recursion in expression
grammars.

### Dependencies

Catch2 (tests only)

### Installing


```
$ git clone https://github.com/habbbe/parsimon
$ cd parsimon
$ cmake -B <BUILD_DIR> -DCMAKE_INSTALL_PREFIX=<PREFIX>
$ cmake --install build

```

If you want to build and run the tests instead, use
```
$ cmake -B <BUILD_DIR> -DBUILD_TESTS=ON
$ cmake --build <BUILD_DIR> -j <NUM_OF_PARALLEL_JOBS> --target parsimon_tests test

```

### Tips

Try increasing the inlining limit for your compiler for better performance (your mileage may vary).

As an example, using `-finline-limit=2000` in GCC 9.2.0 resulted in a ~50% performance increase for 
the [JSON parser](test/json/json_parser.h) on my machine.

### TODO

- Add wiki
- More extensive test cases
- Improve compilation errors (somewhat done). The combined parsers are type checked first when a
parse is commenced with some input. If a deeply nested parser doesn't type check, a flywheel scroll
or a very tall display is highly recommended.
