# fjson
Minimalist, recursive, Finite-State Machine JSON parser

## Overview
fjson is a recursive Finite-State Machine JSON parser which operates on streams (byte per byte!) by keeping track internally
of the current state of parsing.

## API
fjson's API are just 5
- fjson_new() Allocates a new `fjson_t` structure.
- fjson_free() Frees the `fjson_t` structure.
- fjson_putbyte() Accepts one byte and processes it. Returns 0 on still-parsing, -1 on error, 1 on successful parsing.
- fjson_putbuf() If you want to process entire buffers (even partial json documents are okay) you can call this function. It just uses fjson_putbyte() internally.
- fjson_free_element() Not implemented, don't kill me! I had no time!

## How to use it
As I said, this is a Finite State Machine parser that operates byte per byte.  
This means that you can literally start to download a json document and parsing it on-the-fly without the need of caching it.  
It is easy:  
1. Allocate the `fjson_t`structure
2. When you have one byte or a buffer, just process it using the fjson_putbyte() or fjson_putbuf() APIs.
3. Once finished the parsing you can free the `fjson_t` structure.
4. Walk the `fjson_element_t` structure and search for what you want.
5. Free the `fjson_element_t` structure by using the fjson_free_element() API.

## The `fjson_element_t` structure
