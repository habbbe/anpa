## parsimon 

A C++17 header only generic monadic parser combinator library loosely based on Haskell's parsec. 

### Features

All parsers and combinators are `constexpr` meaning no run time cost for constructing a parser.

In addition, all parsers and combinators, with two exceptions (`many_to_vector` and `many_to_map`. Will 
this change with C++20?), can be evaluated at compile time.
This enables compile time parsing as long as dereferencing a and incrementing the input iterator are 
`constexpr` operations.

### Example

See `src/json/json_parser.h` for a simple but functional JSON parser. It's only ~30 LOC and gives
a good overview on how to use the library.

### TODO

- Add "How to use" to README.md
- Proper documentation
- Improve compilation errors. The combined parsers are type checked first when a
parse is commenced with some input. If a deeply nested parser doesn't type check, a flywheel scroll
or a very tall display is highly recommended.
- Installing the library to directory
