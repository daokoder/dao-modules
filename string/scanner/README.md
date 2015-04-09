## string.scanner -- stateful string scanning

This module implements string scanner which advances through target string on successful match, keeping current position and last matched sub-string.

### Index
namespace [str](#str)

class [Scanner](#scanner)
- [Scanner](#scanner_ctor)(_context_: string, _pos_ = 0) => Scanner
- [.context](#context)(invar _self_: Scanner) => string
- [.pos](#pos)(invar _self_: Scanner) => int
- [.pos=](#pos)(_self_: Scanner, _value_: int)
- [.rest](#rest)(invar _self_: Scanner) => int
- [append](#append)(_self_: Scanner, _str_: string)
- [fetch](#fetch)(_self_: Scanner, _count_: int) => string
- [peek](#peek)(invar _self_: Scanner, _count_: int) => string
- [scan](#scan)(_self_: Scanner, _pattern_: string) => int
- [seek](#seek)(_self_: Scanner, _pattern_: string) => int
- [matched](#matched)(invar _self_: Scanner, _group_ = 0) => string
- [matchedPos](#matchedpos)(invar _self_: Scanner, _group_ = 0) => tuple&lt;start: int, end: int&gt;|none
- [line](#line)(invar _self_: Scanner) => int
- [follows](#follows)(invar _self_: Scanner, _pattern_: string) => bool
- [precedes](#precedes)(invar _self_: Scanner, _pattern_: string) => bool

<a name="str"></a>
### Classes
#### <a name="scanner">`str::Scanner`</a>
Provides way to successively process textual data using Dao string patterns.
#### Methods
<a name="scanner_ctor"></a>
```ruby
Scanner(context: string, pos = 0) => Scanner
```
Constructs scanner operating on string *context* starting at position *pos*
<a name="context"></a>
```ruby
.context(invar self: Scanner) => string
```
String being scanned
<a name="pos"></a>
```ruby
.pos(invar self: Scanner) => int
.pos=(self: Scanner, value: int)
```
Current position
<a name="rest"></a>
```ruby
.rest(invar self: Scanner) => int
```
Number of bytes left to scan (from the current position to the end of the context)
<a name="append"></a>
```ruby
append(self: Scanner, str: string)
```
Appends *str* to the context
<a name="fetch"></a>
```ruby
fetch(self: Scanner, count: int) => string
```
Returns *count* bytes starting from the current position and advances the scanner

**Errors:** `Param` when *count* < 0
<a name="peek"></a>
```ruby
peek(invar self: Scanner, count: int) => string
```
Returns *count* bytes starting from the current position without advancing the scanner
<a name="scan"></a>
```ruby
scan(self: Scanner, pattern: string) => int
```
Mathes *pattern* immediately at the current position; on success, the scanner is advanced and its last match information is updated. Returns the number of bytes the scanner has advanced through
<a name="seek"></a>
```ruby
scan(self: Scanner, pattern: string) => int
```
Mathes *pattern* anywhere in the string after the current position; on success, the scanner is advanced and its last match information is updated. Returns the number of bytes the scanner has advanced through
<a name="matched"></a>
```ruby
matched(invar self: Scanner, group = 0) => string
```
The last matched sub-string or its group *group* (if *group* is greater then zero)
<a name="matchedPos"></a>
```ruby
matchedPos(invar self: Scanner, group = 0) => tuple<start: int, end: int>|none
```
Position of the last matched sub-string or its group *group* (if *group* is greater then zero)
<a name="line"></a>
```ruby
line(invar self: Scanner) => int
```
Line number at the current position
<a name="follows"></a>
```ruby
follows(invar self: Scanner, pattern: string) => bool
```
Matches *pattern* immediately before the current position without affecting the state of the scanner
<a name="precedes"></a>
```ruby
precedes(invar self: Scanner, pattern: string) => bool
```
Matches *pattern* immediately after the current position without affecting the state of the scanner
