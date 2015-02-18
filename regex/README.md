## regex -- Oniguruma (Onigmo) regular expressions

### Overview
This module provides regular expressions based on [Onigmo](https://github.com/k-takata/Onigmo) fork of [Oniguruma](http://www.geocities.jp/kosako3/oniguruma/)
library. it uses Ruby grammar with several customizable syntax modifications:
- by default, Dao string patterns are mimicked: `%` is used instead of `\`, whitespace characters are ignored outside of `[...]` groups
- one-line comments starting with `#` can be used
- implicit spacing mode: outside of `[...]`, a standalone whitespace character or `\r\n` are interpreted as `\\s*`, and a pair of equal whitespace characters
is interpreted as `\\s+`

Grammar description can be found in `Onigmo/doc/RE`.

### Installation
Currently, Onigmo should be built manually from the source provided with the module. In order to link it statically to `regex` module, Onigmo should be
configured as `./configure CFLAGS=-fPIC LFLAGS=-fPIC` to enable position-independent code (on Windows, the relevant makefiles need to be edited). Consult
`Onigmo/README` for details as to how to build the library on various platforms.
