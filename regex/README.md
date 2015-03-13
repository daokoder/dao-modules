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
Currently, Onigmo should be built manually from the source provided with the module. In order to link it statically to `regex` module,  Onigmo should be
configured as `./configure CFLAGS=-fPIC LFLAGS=-fPIC` to enable position-independent code (on Windows, the relevant makefiles need to be edited). Consult
`Onigmo/README` for details as to how to build the library on various platforms.

### Index
namespace [re](#re)

invar class [Regex](#regex)
- [.pattern](#pattern)(_self_: Regex) => string
- [.groupCount](#groupcount)(_self_: Regex) => int
- [.ignoresCase](#ignorescase)(_self_: Regex) => bool
- [fetch](#fetch)(_self_: Regex, _target_: string, _group_: int|string = 0, _start_ = 0, _end_ = -1) => string
- [search](#search)(_self_: Regex, _target_: string, _start_ = 0, _end_ = -1) => Match|none
- [matches](#matches)(_self_: Regex, _target_: string) => bool
- [extract](#extract)(_self_: Regex, _target_: string, _matchType_: enum&lt;both,matched,unmatched&gt; = $matched) => list&lt;string&gt;
- [replace](#replace)(_self_: Regex, _target_: string, _format_: string, _start_ = 0, _end_ = -1) => string
- [scan](#scan)(_self_: Regex, _target_: string, _start_ = 0, _end_ = -1)[_found_: Match => none|@V] => list&lt;@V&gt;
- [replace](#replace2)(_self_: Regex, _target_: string, _start_ = 0, _end_ = -1)[_found_: Match => string] => string
- [iter](#iter)(_self_: Regex, _target_: string, _start_ = 0, _end_ = -1) => Iter

invar class [Match](#match)
- [string](#string)(_self_: Match, _group_: int|string = 0) => string
- [size](#size)(_self_: Match, _group_: int|string = 0) => int
- [start](#start)(_self_: Match, _group_: int|string = 0) => int
- [end](#end)(_self_: Match, _group_: int|string = 0) => int
- [.groupCount](#groupcount2)(_self_: Match) => int

class [Iter](#iter2)
- [for](#for)(_self_: Iter, _iterator_: ForIterator)
- [<span>[]</span>](#index)(_self_: Iter, _index_: ForIterator) => Match

Functions:
- [compile](#compile)(_pattern_: string) => Regex
- [compile](#compile)(_pattern_: string, _options_: enum&lt;strictSpacing;impliedSpacing;ignoreCase;allowComments;useBackslash&gt;) => Regex

<a name="re"></a>
### Classes
#### <a name="Regex">`re::Regex`</a>
Regular expression using [Onigmo fork](https://github.com/k-takata/Onigmo) of [Oniguruma](http://www.geocities.jp/kosako3/oniguruma/) library with Ruby grammar as backend. See [compile()(#compile) for usage details.
#### Methods
<a name="pattern"></a>
```ruby
.pattern(self: Regex) => string
```
String pattern

__Note:__ The pattern is stored in canonical form, i.e. with strict spacing, '\' as escape character and without comments
<a name="groupcount"></a>
```ruby
.groupCount(self: Regex) => int
```
Number of capture groups in the pattern
<a name="ignorescase"></a>
```ruby
.ignoresCase(self: Regex) => bool
```
Case-insensitivity
<a name="fetch"></a>
```ruby
fetch(self: Regex, target: string, group: int|string = 0, start = 0, end = -1) => string
```
Finds the first match in *target* in the range [*start*; *end*] and returns sub-match specified by *group*.

__Note:__ For the interpretation of group numbers, see [Match](#match)
<a name="search"></a>
```ruby
search(self: Regex, target: string, start = 0, end = -1) => Match|none
```
Returns the first match in *target* in the range [*start*; *end*], or `none` if no match was found
<a name="match"></a>
```ruby
matches(self: Regex, target: string) => bool
```
Checks if the entire *target* is matched by the regex
<a name="extract"></a>
```ruby
extract(self: Regex, target: string, matchType: enum<both,matched,unmatched> = $matched) => list<string>
```
Returns all matches in *target* (or unmatched, or both, depending on *matchType*)
<a name="replace"></a>
```ruby
replace(self: Regex, target: string, format: string, start = 0, end = -1) => string
```
Replaces all matches in *target* in the range [*start*; *end*] with *format* string. Returns the entire resulting string. *format* may contain backreferences
in the form '$<group number from 0 to 9>' or '$(<group name>)'; '$$' can be to escape '$'
 <a name="scan"></a>
 ```ruby
 scan(self: Regex, target: string, start = 0, end = -1)[found: Match => none|@V] => list<@V>
 ```
 Iterates over all matches in *target* in the range [*start*; *end*], yielding each match as *found*. Returns the list of values obtained from the code
 section
 <a name="replace2"></a>
 ```ruby
 replace(self: Regex, target: string, start = 0, end = -1)[found: Match => string] => string
 ```
 Iterates over all matches in *target*, yielding each of them as *found*. Returns the string formed by replacing each match in *target* by the corresponding
 string returned from the code section
 <a name="iter"></a>
 ```ruby
 iter(self: Regex, target: string, start = 0, end = -1) => Iter
 ```
 Returns `for` iterator to iterate over all matches in *target* in the range [*start*; *end*].

__Note:__ Changing *target* has no effect on the iteration process (the iterator will still be bound to the original string)

------
#### <a name="match">`re::Match`</a>
Single regular expression match providing information on matched sub-string and individual captured groups.

*group* parameter in `Match` methods may either be a group number or its name.

Group number is interpreted the following way:
-*group* == 0 -- entire matched sub-string
-*group* > 0 and *group* <= `groupCount()` -- corresponding sub-match
-*group* < 0 or *group* > `groupCount()` -- not permitted

If *group* is a name, the last group in the pattern with this name is assumed (at least one such group must exist).
#### Methods
<a name="string"></a>
```ruby
string(self: Match, group: int|string = 0) => string
```
Sub-string captured by *group*
<a name="size"></a>
```ruby
size(self: Match, group: int|string = 0) => int
```
Size of the sub-string captured by *group*
<a name="start"></a>
```ruby
start(self: Match, group: int|string = 0) => int
```
Start position of the sub-string captured by *group*
<a name="end"></a>
```ruby
end(self: Match, group: int|string = 0) => int
```
End position of the sub-string captured by *group*
<a name="groupCount2"></a>
```ruby
.groupCount(self: Match) => int
```
Number of captured groups

------
#### <a name="iter">`re::Iter`</a>
`for` iterator to iterate over regular expression matches in a string
<a name="for"></a>
```ruby
for(self: Iter, iterator: ForIterator)
```
<a name="index"></a>
```ruby
[](self: Iter, index: ForIterator) => Match
```
### Functions
<a name="compile"></a>
```ruby
compile(pattern: string) => Regex
compile(pattern: string, options: enum<strictSpacing;impliedSpacing;ignoreCase;allowComments;useBackslash>) => Regex
```
Constructs regular expression from *pattern* using specified *options* (if provided).

Default options mimic Dao string patterns syntax:
- free spacing -- whitespace is ignored outside of '[...]'
- '%' is used as control character
- the pattern is treated as case-sensitive

This behavior can be overridden with the following values of *options*:
- `$strictSpacing` -- whitespace characters in the pattern are treated 'as is' (canonical behavior)
- `$impliedSpacing` -- outside of '[ ... ]', a standalone whitespace character or '\r\n' are interpreted as '\\s*',
and a pair of equal whitespace characters is interpreted as '\\s+'
- `$ignoreCase` -- the pattern is treated as case-insensitive
- `$allowComments` -- all characters starting from '#' up to '\n' (or end of string) are ignored as comments ('#' can be escaped)
- `$useBackslash` -- use canonical '\' as control character

__Note:__ Regular expression engine presumes UTF-8-encoded patterns
