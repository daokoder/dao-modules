## os.path -- abstract file path handling

### Index
namespace [os](#os)

class [Path](#path)
- [.path](#path_prop)() => string
- [.kind](#kind)() => enum&lt;win,unix>
- [.root](#root)() => string
- [.name](#name)() => string
- [check](#check)(_what_: enum&lt;absolute,canonical>) => bool
- [.suffix](#suffix)() => string
- [.basename](#basename)() => string
- [join](#join)(_other_: Path) => Path
- [join](#join)(_other_: string) => Path
- static [/](#join)(_a_: Path, _b_: Path) => Path
- static [/](#join)(_a_: Path, _b_: string) => Path
- [split](#split)() => list&lt;string>
- [normalize](#normalize)() => Path
- static [==](#cmp)(_a_: Path, _b_: Path) => bool
- static [!=](#cmp)(_a_: Path, _b_: Path) => bool
- [(string)](#string)(_hashing_ = false)
- [relativeTo](#relativeto)(_base_: Path) => Path
- [relativeTo](#relativeto)(_base_: string) => Path
- [<span>[]</span>](#relativeto)(_target_: Path) => Path
- [<span>[]</span>](#relativeto)(_target_: string) => Path
- [clone](#clone)() => Path

Functions:
- [path](#newpath)(_kind_: enum&lt;win,unix>, _value_: string) => Path
- [path](#newpath)(_value_: string) => Path

<a name="os"></a>
### Classes
####<a name="path">`os::Path`</a>
Abstract file system path
#### Methods
<a name="path_prop"></a>
```ruby
.path() => string
```
Path string
<a name="kind"></a>
```ruby
.kind() => enum<win,unix>
```
Path kind (Windows or Unix)
<a name="root"></a>
```ruby
.root() => string
```
Root, if present
<a name="name"></a>
```ruby
.name() => string
```
Final path component
<a name="check"></a>
```ruby
check(what: enum<absolute,canonical>) => bool
```
Checks if path is absolute or canonical, depending on *what*
<a name="suffix"></a>
```ruby
.suffix() => string
```
Path name suffix
<a name="basename"></a>
```ruby
.basename() => string
```
Name without suffix
<a name="join"></a>
```ruby
join(other: Path) => Path
join(other: string) => Path
static /(a: Path, b: Path) => Path
static /(a: Path, b: string) => Path
```
Joins two paths and returns the resulting path

 **Errors:** `Param` if joining paths of different kinds, `Path` when appending an absolute path
 <a name="split"></a>
 ```ruby
split() => list<string>
 ```
 Splits path into individual components
 <a name="normalize"></a>
 ```ruby
normalize() => Path
 ```
 Normalizes path by removing '.', '..' and multiple consecutive path separators

 **Errors:** `Path` if backtracking with '..' exceeded path boundaries
 <a name="cmp"></a>
 ```ruby
static ==(a: Path, b: Path) => bool
static !=(a: Path, b: Path) => bool
 ```
Compares *a* and *b* component-wise
<a name="string"></a>
```ruby
(string)(hashing = false)
```
Conversion to string
<a name="relativeto"></a>
```ruby
relativeTo(base: Path) => Path
relativeTo(base: string) => Path
[](target: Path) => Path
[](target: string) => Path
```
 Returns relative path from *base* path to this path or from this path to *target* path
 <a name="clone"></a>
 ```ruby
clone() => Path
 ```
Returns path copy
### Functions
<a name="newpath"></a>
```ruby
path(kind: enum<win,unix>, value: string) => Path
path(value: string) => Path
```
Constructs path object with the given *value* and *kind* (system-dependent if omitted)

**Errors:**: `Path` for empty on invalid path
