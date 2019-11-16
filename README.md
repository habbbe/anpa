# parsimon

parsimon is a C++17 header only generic parser combinator library loosely based on Haskell's parsec library. 

All parsers and combinators are `constexpr` meaning no run time cost for constructing a parser.

In addition, all parsers and combinators, with two exceptions[0], can be evaluated at compile time.
This enables compile time parsing as long as dereferencing and incrementing the input iterator are 
`constexpr` operations.

## Example

TODO
