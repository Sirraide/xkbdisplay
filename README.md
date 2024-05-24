# XKB Keyboard Layout Generator

This program takes a `.kb` file (described below) and translates it to an XKB
keyboard; I use it to simplify editing my keyboard layout.

## KB file format
KB files are text files that consist of a series of records. Each record has the
form:
```ini
# this is a comment
key = [ a, b, c, d, "comma", '"', ... ]
```

The keys are numbered top to bottom, left to right, starting from 0. E.g. the
key to the left of the '1' key is '00', the '1' key is '01', and so on.
Whitespace is ignored entirely. Quotes are optional and may be included
for readability.