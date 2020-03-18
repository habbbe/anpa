# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.0] - 2020-03-18
### Added
- Changelog for project
- Parser combinator `cond` for creating branching parsers based on input
- Parser combinator `flip` that reverses the result (success -> fail/fail -> success)
- Option `no_leading_zero` for `integer` and `floating` parsers
- Option `decimal_comma` for `floating` parser
- Support parsing of aggregate types
- `range` is now convertible to std::string

### Changed
JSON parser now doesn't allow leading zero for numbers
`<<` operator now uses `lift` for GCC >= 9 and Clang (optimization)

## [0.3.0] - 2020-01-15
### Added
- Parser `item_if_not`
- Option to disallow trailing sparator in `many*` parsers
- Options for `integer` parser
- FailOnPartial template argument for `||`
- `|` operator that fails on partial parses
- `parse_error` type
  - `no_negative`: do not parser negative numbers (even with unsigned types)
  - `leading_plus`: allow leading plus sign
- `floating` parser now supports optional plus sign for exponent component
   

### Changed
- `integer` parser now doesn't consume any input on "-" or "+".
- JSON parser now doesn't allow control characters if not escaped.
- JSON parser now fails on partial escaped sequences.

### Removed
- `first` parser

## [0.2.0] - 2020-01-13
### Added
- Option to disallow trailing sparator in `many*` parsers

### Changed
- Use enum `options` in place of `bool` template parameters
- `many_to_vector` now doesn't reserve if `Reserve` template
  argument is 0
- Disallow trailing commas for collections in JSON parser

### Removed
- `number` parser
