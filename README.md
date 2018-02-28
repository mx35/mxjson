# mxjson

mxjson is a C based JSON parser implemented as a header-only library.
The design and approach of mxjson is inspired by JSMN
(https://github.com/zserge/jsmn),
Both mxjson and JSMN have a focus on parsing the JSON input into a form
suitable for an application to interpret and translate.

mxjson is distributed under the MIT License.

## Comparison with JSMN

The differences with JSMN are:
 * mxjson is implemented as a header-only library.
 * Strict validation of the JSON input against RFC-8295
   (https://tools.ietf.org/html/rfc8259)
 * Less focus on support for embedded systems.
 * Automatic resizing of the token array.
 * An object member name/value pair is stored as a single token.
 * Support for interpreting/translating JSON strings that contain escape
   characters.
 * Provides parent and next tokens for each JSON value to support navigation of
   the JSON hierarchy.
 * Similar/better performance to JSMN, provided the code is compiled with
   optimisation.

## Building

To get started, clone the git repository:

    git clone https://github.com/mx35/mxjson

and run `make`

This builds:
 * `bin/mxjson` - Minimal example which checks a JSON input is valid
 * `bin/mxjson-tree` - Example application to display the JSON hierarchy
 * `bin/mxjson-test` - Test suite
 * `bin/mxjson-test-coverage` - Test suite, with GCOV code coverage enabled

See sections below for details on usage of these binaries.

To use mxjson, simply `#include "mxjson.h"`, and add the following header
files to a location covered by the include path:
 * `mxjson.h` - The JSON parser
 * `mxstr.h` - String API
 * `mxutil.h` - Miscellaneous utility functions

It is recommended to compile mxjson with optimisation (`-O2`) to achieve
the best parsing performance. The mxstr library is designed with an
assumption that a reasonable level of compiler optimisation is
performed.

## API Usage

The mxjson API consists of:
 * 3 functions to perform parsing:
   * `mxjson_init` - Initialise a parsing context
   * `mxjson_parse` - Parse a JSON input
   * `mxjson_free` - Free resources associated with the parsing context
 * 2 functions to aid with navigating the parsed tokens
   * `mxjson_first` - Get the index for the first child of a token
   * `mxjson_next` - Get the index for the next JSON value.
 * 2 functions to handle interpretation of escaped characters in strings.
   * `mxjson_token_name` - Get a token name, unescaping if necessary.
   * `mxjson_token_string` - Get a string representation of a token value,
     unescaping if necessary.

A typical flow for parsing and processing a JSON input is:

```C
  mxjson_parser_t  p;
  mxjson_token_t  *t;
  bool             valid;
  int              i;

  mxjson_init(&p, 0, NULL, mxjson_resize);
  valid = mxjson_parse(&p, mxstr(json_ptr, json_size));

  if (valid) {
      for (i = 1; i <= p.idx; i++) {
          t = &p.tokens[i];
	  // Process token t
      }
  }

  mxjson_free(&p);
```

The following sections cover the API in more detail.

### Parsing

Parsing a JSON input consists of the following steps:

 1. Initialise a parser context
```C
  mxjson_parser_t p;

  mxjson_init(&p, 0, NULL, mxjson_resize);
```
In the call to `mxjson_init()`, `mxjson_resize` is specified to manage the
memory for the tokens array. This is sufficient and suitable for most use
cases. Alternatively, or in addition, a pointer to a locally allocated array
may be passed to `mxjson_init` and/or a custom allocation function used in
place of `mxjson_resize`.

 2. Create a string representation for the JSON input.
```C
  char *json_ptr = ...;
  size_t json_size = strlen(json_ptr);
  mxstr_t str;

  str = mxstr(json_ptr, json_size);
```
The JSON input is passed as a `mxstr_t` string type. It is not necessary for
the string to be null-terminated.

3. Parse the JSON input
```C
  bool valid;

  valid = mxjson_parse(&p, str);
```
The return value indicates whether the JSON was successfully parsed. The
parser context contains the token array for the parsed JSON.

The parser context may be re-used by calling `mxjson_parse` again with another
JSON input.

4. Free the parser context

Once the parser context is no longer required, any memory allocated for the
token array by `mxjson_resize` is released by calling `mxjson_free`:
```C
mxjson_free(&p);
```

### Navigating

Tokens for the parsed JSON are stored in the `tokens` array inside the parser
context, and are accessed by index (`mxjson_idx_t` type). The first token
is at index 1, and `idx` in the parser context provides the index of the last
token.

The parsed tokens may be processed in depth-first order (i.e. left-to-right
order as they appear in the JSON input) by visiting array locations
`1..(p.idx)`:

```C
  mxjson_token_t *t;

  for (i = 1; i <= p.idx; i++) {
      t = &p.tokens[i];
      // Process token t
  }

```

The tokens reference strings in the input data, using offsets into the input
data (`p.json.ptr`). The strings may be accessed a `mxstr_t` strings using
`mxjson_token_name` and `mxjson_token_string` - see below.

A token has the following fields:
 * `name`: Offset into the input data for the token name. The name is present
   for elements of a JSON object value, otherwise both `name` and `name_size`
   are 0.
 * `name_size`: Length of the token name.
 * `name_esc`: Indicates whether the token name contains escaped characters.
 * `value_esc`: Indicates whether the token value contains escaped characters.
 * `value_type`: Type of value stored in the token:
   * `MXJSON_NULL` - null value
   * `MXJSON_BOOL` - bool value, available in `t->boolean`
   * `MXJSON_NUMBER` - number value, stored as a string: `t->str` gives the
     offset into the input data for the token value and `t->str_size` gives
     the length of the string. This string may be accessed using
     `mxjson_token_string`.
   * `MXJSON_STRING` - string value: `t->str` gives the offset into the input
     data for the token value and `t->str_size` gives the length of the
     string. Note: the string referenced does not include quotes. The string
     value may be accessed using `mxjson_token_string`.
   * `MXJSON_OBJECT` - JSON object value. `t->children` gives the number of
     name/value pairs in the object, and `t->next` gives the index for the
     next token following the object. Note: the first member of the object is
     at the current index + 1.
   * `MXJSON_ARRAY` - JSON array value. `t->children` gives the number of
     members of the array, and `t->next` gives the index for the next token
     following the array. Note: the first member of the array is at the
     current index + 1.
 * `parent`: Gives the index for the parent object/array that contains the
   current token. Note: The root token, at array index 1 has a parent with
   index 0.

`mxjson_first()` returns the index of first child of the current token or,
if the current token has no children, the index of the next token following
the current token.

`mxjson_next()` returns the index of the next token following the current
token. This function allows an entire object or array value to be skipped over
if its contents are not of interest.

For example, the set of immediate children of an object or array value can be
processed using:
```C
mxjson_idx_t parent_index;
mxjson_idx_t index;
mxjson_idx_t last;

parent_index = <index for parent token>
last = mxjson_next(&p, parent_index);
index = mxjson_first(&p, parent_index);

while (index != last) {
    t = &p.tokens[index];
    // Process token t
    index = mxjson_next(&p, index);
}

// index now points to first token after the JSON value at parent_index
```

### Interpreting

The `mxjson_token_name` and `mxjson_token_string` APIs may be used to get
a `mxstr_t` string value for the token name and token value respectively.

These functions generally return a reference to the string in the input
JSON. However, if the string contains escaped characters, an unescaped
version of the string is constructed in a temporary buffer supplied by the
caller.

```C
mxbuf_t buffer;
bool    valid;

buffer = mxbuf_create(&buffer, NULL, 0);
str1 = mxjson_token_name(&p, index1, &buffer, &valid);
str2 = mxjson_token_string(&p, index2, &buffer, &valid);
str3 = mxjson_token_name(&p, index3, &buffer, &valid);
// Process str1, str2 and str3

mxbuf_reset(&buffer); // re-use space in the buffer
str4 = mxjson_token_name(&p, index4, &buffer, &valid);
str5 = mxjson_token_string(&p, index5, &buffer, &valid);
str6 = mxjson_token_string(&p, index6, &buffer, &valid);
// Process str4, str5 and str6

mxbuf_free(&buffer); // release memory associated with buffer
```

Note: the `mxjson_token_name` and `mxjson_token_string` functions may detect
an error when unescaping the string if it contains an invalid UTF-16
surrogate pair. In this situation, the functions return the unescaped version
of the string, along with an indication that the error occurred.

## Tests

The tests for mxjson are built and run using `make test` or `make coverage`
to build an run the tests with code coverage enabled. This generates
`mxjson.h.gcov` which contains code coverage details for `mxjson.h`

The test suite is an implementation of the set of JSON parsing tests
designed by Nicolas Seriot at https://github.com/nst/JSONTestSuite.
Each test name has a prefix that indicates the expected result for
that test:
 * `y_` content must be accepted by parsers
 * `n_` content must be rejected by parsers
 * `i_` implementation dependent test. Parsers are free to accept or reject
   content. mxjson accepts the content for these tests.
 * `u_` implementation dependent test. Parsers are free to accept or reject
   content. mxjson rejects the content for these tests.

For the set of implmentation dependent tests, mxjson rejects only those
related to UTF-16 support:
 * `u_string_UTF-16LE_with_BOM`
 * `u_string_utf16BE_no_BOM`
 * `u_string_utf16LE_no_BOM`

This is consistent with the RFC-8259 specification
(https://tools.ietf.org/html/rfc8259):

> JSON text exchanged between systems that are not part of a closed
>  ecosystem MUST be encoded using UTF-8

## Examples

The `examples` sub-directory contains a couple of examples illustrating the
use of the mxjson API. These examples are built by typing `make`, and the
binaries are placed in the `bin` sub-directory.

### mxjson

`examples/mxjson.c` provides a minimal example which reads and parses a JSON
file. The process exits with exit-code 0 if the JSON is valid, or
exit-code 1 if the JSON is invalid.

Usage: `bin/mxjson <filename>`

### mxjson-tree

`examples/mxjson-tree.c` is a larger example that reads and parses a JSON file
and displays information about the contents of the JSON. It illustrates
iteration of the token based representation constructed by mxjson.

Usage: `bin/mxjson-tree -s <filename>`

Display statistics about the types of JSON values present in the specified
file.

For example, using the `citm_catalog.json` file from
`https://github.com/miloyip/nativejson-benchmark`:

```
bin/mxjson-tree -s citm_catalog.json

Parsed: 1727204 / 1727204 bytes (Valid JSON)

 JSON   | Total    | Named    |         Name          |         Size
 Type   | Count    | Count    | Min    Average   Max  |  Min   Average   Max
--------+----------+----------+-------+-------+-------+-------+-------+-------
   null |     1263 |     1263 |     4 |     8 |    12 |     0 |     0 |     0
 number |    14392 |    13226 |     2 |     8 |    21 |     5 |     9 |    13
 string |      735 |      735 |     4 |     6 |    13 |     4 |    22 |    55
 object |    10937 |      194 |     6 |     9 |    24 |     1 |     2 |   184
  array |    10451 |    10451 |     5 |     8 |    14 |     1 |     1 |   243
--------+----------+----------+
 Total: |    37778 |    25869


 JSON   | Total    | Named    | Escaped  | Escaped
 Type   | Count    | Count    | Name     | Value
--------+----------+----------+----------+----------
 string |      735 |      735 |        0 |        1
```

Usage: `bin/mxjson-tree -t -d <num> -a <num> -o <num> -i <num> <filename>`

Display the contents of the JSON file as a tree.

Where:
 * `-d`  Maximum depth (i.e. nested objects/arrays) to display.
 * `-a`  How many elements from each array value to display
 * `-o`  How many elements from each object value to display
 * `-i`  Threshold at which to annotate array/object entries with an index

For example:

```
bin/mxjson-tree -t -a 2 -o 5 -i 100 citm_catalog.json

Parsed: 1727204 / 1727204 bytes (Valid JSON)

{11}
├─ areaNames{17}
│  ├─ 205705993: "Arrière-scène central"
│  ├─ 205705994: "1er balcon central"
│  ├─ 205705995: "2ème balcon bergerie cour"
│  ├─ 205705996: "2ème balcon bergerie jardin"
│  ├─ 205705998: "1er balcon bergerie jardin"
│  └─... (12 more, 17 total)
├─ audienceSubCategoryNames{1}
│  └─ 337100890: "Abonné"
├─ blockNames{0}
├─ events{184}
│  ├─1/184─ 138586341{8}
│  │  ├─ description: null
│  │  ├─ id: 138586341
│  │  ├─ logo: null
│  │  ├─ name: "30th Anniversary Tour"
│  │  ├─ subTopicIds[2]
│  │  │  ├─ 337184269
│  │  │  └─ 337184283
│  │  └─... (3 more, 8 total)
│  ├─2/184─ 138586345{8}
│  │  ├─ description: null
│  │  ├─ id: 138586345
│  │  ├─ logo: "/images/UE0AAAAACEKo6QAAAAZDSVRN"
│  │  ├─ name: "Berliner Philharmoniker"
│  │  ├─ subTopicIds[3]
│  │  │  ├─ 337184268
│  │  │  ├─ 337184283
│  │  │  └─... (1 more, 3 total)
│  │  └─... (3 more, 8 total)
│  ├─3/184─ 138586349{8}
│  │  ├─ description: null
│  │  ├─ id: 138586349
│  │  ├─ logo: "/images/UE0AAAAACEKo7QAAAAZDSVRN"
│  │  ├─ name: "Berliner Philharmoniker"
│  │  ├─ subTopicIds[4]
│  │  │  ├─ 337184268
│  │  │  ├─ 337184288
│  │  │  └─... (2 more, 4 total)
│  │  └─... (3 more, 8 total)
│  ├─4/184─ 138586353{8}
│  │  ├─ description: null
│  │  ├─ id: 138586353
│  │  ├─ logo: "/images/UE0AAAAACEKo8QAAAAZDSVRN"
│  │  ├─ name: "Pittsburgh Symphony Orchestra"
│  │  ├─ subTopicIds[3]
│  │  │  ├─ 337184268
│  │  │  ├─ 337184283
│  │  │  └─... (1 more, 3 total)
│  │  └─... (3 more, 8 total)
│  ├─5/184─ 138586357{8}
│  │  ├─ description: null
│  │  ├─ id: 138586357
│  │  ├─ logo: "/images/UE0AAAAACEKo9QAAAAhDSVRN"
│  │  ├─ name: "Orchestre Philharmonique de Radio France"
│  │  ├─ subTopicIds[3]
│  │  │  ├─ 337184268
│  │  │  ├─ 337184283
│  │  │  └─... (1 more, 3 total)
│  │  └─... (3 more, 8 total)
│  └─... (179 more, 184 total)
├─ performances[243]
│  ├─1/243─ {9}
│  │  ├─ eventId: 138586341
│  │  ├─ id: 339887544
│  │  ├─ logo: null
│  │  ├─ name: null
│  │  ├─ prices[2]
│  │  │  ├─ {3}
│  │  │  │  ├─ amount: 90250
│  │  │  │  ├─ audienceSubCategoryId: 337100890
│  │  │  │  └─ seatCategoryId: 338937295
│  │  │  └─ {3}
│  │  │     ├─ amount: 66500
│  │  │     ├─ audienceSubCategoryId: 337100890
│  │  │     └─ seatCategoryId: 338937296
│  │  └─... (4 more, 9 total)
│  ├─2/243─ {9}
│  │  ├─ eventId: 339420802
│  │  ├─ id: 339430296
│  │  ├─ logo: null
│  │  ├─ name: null
│  │  ├─ prices[2]
│  │  │  ├─ {3}
│  │  │  │  ├─ amount: 28500
│  │  │  │  ├─ audienceSubCategoryId: 337100890
│  │  │  │  └─ seatCategoryId: 338937295
│  │  │  └─ {3}
│  │  │     ├─ amount: 23750
│  │  │     ├─ audienceSubCategoryId: 337100890
│  │  │     └─ seatCategoryId: 338937296
│  │  └─... (4 more, 9 total)
│  └─... (241 more, 243 total)
└─... (6 more, 11 total)
```

## mxstr String Library

The mxstr library aims to provide a lightweight and easy-to-use string
handling API for C with a strong guarantee of robustness (i.e. avoiding
buffer overflows and null-pointer dereferences).

The initial motivation for writing mxjson was as an example/proof-of-concept
for the string parsing functionality in the mxstr string library.

The syntax for JSON is relatively simple (see https://www.json.org/ for
a summary). For example, the syntax for a JSON number value is given by:

![JSON Number Syntax](https://www.json.org/number.gif)

The results from Nicolas Seriot's analysis of JSON parsers
suggest it is difficult to both efficiently and robustly parse an input.
See https://github.com/nst/JSONTestSuite and Section 4.8 of
http://seriot.ch/parsing_json.php, which provides some discussion
on parsing JSON number values.

The code fragment below shows the mxjson use of the mxstr API to parse a
JSON number value. This is a straightforward translation from
the JSON specification, yet aims to be robust against invalid inputs:

```C
mxjson_parse_number()
{
    mxstr_t s = <number string to be parsed>;
    bool    ok;
    uint8_t c;

    /*
     * Optional '-' followed by at least one digit
     */
    (void)mxstr_consume_char(&s, &c, (c == '-'));
    ok = mxstr_consume_char(&s, &c, isdigit(c));

    if (ok) {
	/*
	 * If the first digit is not '0', there may be zero or more
	 * successive digits.
	 */
	if (c != '0') {
	    mxstr_consume_chars(&s, &c, isdigit(c));
	}

	/*
	 * Optional '.' followed by one or more digits.
	 */
	if (mxstr_consume_char(&s, &c, (c == '.'))) {
	    ok = mxstr_consume_char(&s, &c, isdigit(c));
	    mxstr_consume_chars(&s, &c, isdigit(c));
	}
    }

    /*
     * Optional 'e' or 'E' followed by optional '+' or '-' followed by
     * one or more digits.
     */
    if (ok && mxstr_consume_char(&s, &c, (c == 'e' || c == 'E'))) {
	(void)mxstr_consume_char(&s, &c, (c == '+' || c == '-'));
	ok = mxstr_consume_char(&s, &c, isdigit(c));
	mxstr_consume_chars(&s, &c, isdigit(c));
    }
}
```

For more details of the mxstr String Library, see
https://github.com/mx35/mxstr.
