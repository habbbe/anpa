## parsimon 

A C++17 header only generic monadic parser combinator library loosely based on Haskell's parsec. 

### Features

All parsers and combinators are `constexpr` meaning no run time cost for constructing a parser.

In addition, all parsers and combinators, with two exceptions (`many_to_vector` and `many_to_map`. 
Will this change with C++20?), can be evaluated at compile time.
This enables compile time parsing as long as dereferencing and incrementing the input iterator are 
`constexpr` operations.

### Example

See the provided [JSON parser](test/json/json_parser.h) for a simple but functional JSON parser. It's only ~30 LOC and gives
a good overview on how to use the library.

### Dependencies

Catch2 (tests only)

### Installing


```
$ git clone https://github.com/habbbe/parsimon
$ cd parsimon
$ mkdir build && cd build
$ cmake -DCMAKE_INSTALL_PREFIX=<PREFIX> ..
$ make install

```

If you don't want to build and run the tests, use
```
$ cmake -DCMAKE_INSTALL_PREFIX=<PREFIX> -DRUN_TESTS=OFF ..

```
instead.


### TODO

- Make it possible to use non-copyable types as parse results
- Add "Getting started" to README.md
- More extensive test cases
- Proper documentation
- Improve compilation errors. The combined parsers are type checked first when a
parse is commenced with some input. If a deeply nested parser doesn't type check, a flywheel scroll
or a very tall display is highly recommended.
