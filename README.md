## parsimon 

A C++17 header only generic monadic parser combinator library loosely based on Haskell's parsec. 

### Features

All parsers and combinators are `constexpr` meaning no run time cost for constructing a parser.

In addition, all parsers and combinators, with two exceptions (`many_to_vector` and `many_to_map`), 
are allocation free, and can be evaluated at compile time.
This enables compile time parsing as long as used operations on the input iterators and corresponding
elements are `constexpr`.

### Examples

See the provided test parsers
- [JSON parser](test/json/json_parser.h): JSON DOM parser. It's only ~30 LOC and gives a good overview on 
how to use the library, including recursive parsers.
- [Simple syntax parser](test/tests_perf.cpp): a parser for a simple example syntax inteded for an application
launcher/information dashboard.
- [Expression parser](test/calc/calc.h): a simple expression evaluator supporting basic arithmetic operations
with correct precedence. This example showcases how to use `chain` to eliminate left recursion in expression
grammars.

### Dependencies

For library: None

For tests: Catch2

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

As an example, using `-finline-limit=3500` in GCC 9.2.0 resulted in a ~14% performance increase for 
the [JSON parser](test/json/json_parser.h) on my machine.

### TODO

- Add "Getting started"/wiki
- More extensive test cases
- Improve compilation errors (somewhat done). A parser is not type checked until a parse is 
commenced with some input. If a complex parser doesn't type check, a flywheel scroll or a very 
tall display is highly recommended.
