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
3. Once finished the parsing you can access the `fjson_t`->el which is the parsed `fjson_element_t` structure and then you can free the `fjson_t` structure.
4. Walk the `fjson_element_t` structure and search for what you want.
5. Free the `fjson_element_t` structure by using the fjson_free_element() API.

## The `fjson_element_t` structure
This structure contains a `fjson_type_t` enumerator and a union that contains:
- `char *str` in case the type is FJSON_TYPE_STRING.
- `int64_t` in case the type is FJSON_TYPE_NUMBER. (To be changed with double)
- `struct fjson_array_s`in case the type is FJSON_TYPE_ARRAY. This struct is a linked list of other elements.
- `struct fjson_pair_s` in case the type is FJSON_TYPE_OBJECT. This struct is a linked list of key-value, where the key is always a `fjson_element_t` with type FJSON_TYPE_STRING and the value is just a `fjson_element_t` with variable type.
- `unsigned char bool_val` in case the TYPE is FJSON_TYPE_BOOLEAN. If it equals to 0 the boolean value in the JSON element is false, else it is true.
- `void *content` not really used. Can be used to access the data in a raw format.  

When the type is FJSON_TYPE_NULL there is nothing inside the union to be checked for.


## Missing & TODO
There is no support for special chars inside strings. Don't blame me, no time :(  
There is no support for decimal numbers.  
There is not the fjson_free_element() API. I AM SORRY, but again, time.  
  
I hope to get everything done ASAP.  
  
## Thanks
A special thanks to my friend Davide, that gave me the idea and the method to develop it.
