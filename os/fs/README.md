## os.fs -- generic file system interface

The module provides interface to common operations on files and directories in platform-independent way.

### Index
namespace [fs](#fs)

class [entry](#entry)
- routine [entry](#entry_ctor)(_path_: string) => entry
- routine [.path](#path)(invar _self_: entry) => string
- routine [.name](#name)(invar _self_: entry) => string
- routine [.basename](#basename)(invar _self_: entry) => string
- routine [.suffix](#suffix)(invar _self_: entry) => string
- routine [.kind](#kind)(invar _self_: entry ) => enum&lt;file,dir&gt;
- routine [.dirup](#dirup)(invar _self_: entry) => dir|none
- routine [.time](#time)(invar _self_: entry) => tuple&lt;_created_: int, _modified_: int, _accessed_: int&gt;
- routine [.owner](#owner)(invar _self_: entry) => string
- routine [.access](#access)(invar _self_: entry) => tuple&lt;_user_: string, _group_: string, _other_: string&gt;
- routine [.access=](#access)(self: _entry_, _value_: tuple&lt;_user_: string, _group_: string, _other_: string&gt;)
- routine [move](#move)(_self_: entry, _path_: string)
- routine [delete](#delete)(_self_: entry)
- routine [refresh](#refresh)(_self_: entry)

class [file](#file): entry
- routine [file](#file_ctor)(_path_: string) => file
- routine [.size](#size)(invar _self_: file) => int
- routine [.size=](#size)(_self_: file, _size_: int)
- routine [copy](#copy)(_self_: file, _path_: string) => file

class [dir](#dir): entry
 - routine [dir](#dir_ctor)(_path_: string) => dir
 - routine [mkfile](#mkfile)(_self_: dir, _path_: string) => file
 - routine [mkdir](#mkdir)(_self_: dir, _path_: string) => dir
 - routine [entries](#entries)(invar _self_: dir, _filter_ = '*', _filtering_: enum&lt;wildcard,pattern&gt; = $wildcard) => list&lt;entry&gt;
 - routine [files](#files)(invar _self_: dir, _filter_ = '*', _filtering_: enum&lt;wildcard,pattern&gt; = $wildcard) => list&lt;file&gt;
 - routine [dirs](#dirs)(invar _self_: dir, _filter_ = '*', _filtering_: enum&lt;wildcard,pattern&lt;= $wildcard) => list&lt;dir&gt;
 - routine [<span>[]</span>](#op_index)(invar _self_: dir, _path_: string) => entry
 - routine [exists](#exists)(invar _self_: dir, _path_: string) => bool
 - routine [mktemp](#mktemp)(_self_: dir, _prefix_ = '') => file

routine [cwd](#cwd)() => dir
routine [cd](#cd)(invar _path_: dir)
routine [cd](#cd)(path: _string_)
routine [ls](#ls)(invar _path_: dir) => list&lt;string&gt;
routine [ls](#ls)(_path_ = '.') => list&lt;string&gt;
routine [realpath](#realpath)(_path_: string) => string
routine [exists](#exists)(_path_: string) => bool
routine [roots](#roots)() => list&lt;string&gt;
routine [home](#home)() => dir

<a name="fs"></a>
### Classes
#### <a name="entry">`fs::entry`</a>
Represents a generic file system object, namely a file or directory (links and other special types of file-like objects are not supported). `entry` is inherited by `file` and `dir`, which provide operations specific to files and directories accordingly.

A file object is operated by its name, no descriptors or locks are kept for the lifetime of the associated `entry`. File attributes are cached and are only updated when necessary (e.g., `resize()` will update the size attribute); use `refresh()` to re-read them.

`entry` uses '/' as the unified path separator on all platforms, Windows paths (including UNC) are automatically normalized to this form. Relative paths and symbolic links are automatically expanded to their absolute form.

**Note:** On Windows, all path strings are assumed to be encoded in UTF-8, and are implicitly converted to UTF-16 in order to support Unicode file names on this platform.
#### Methods
<a name="entry_ctor"></a>
```ruby
entry(path: string) => entry
```
Returns new `entry` bound to *path* of file or directory
<a name="path"></a>
```ruby
.path(invar self: entry) => string
```
Full path
<a name="name"></a>
```ruby
.name(invar self: entry) => string
```
Entry name (last component of path)
<a name="basename"></a>
```ruby
.basename(invar self: entry) => string
```
Base name (up to, but not including, the first '.' in name)
<a name="suffix"></a>
```ruby
.suffix(invar self: entry) => string
```
Name part after the last '.'
<a name="kind"></a>
```ruby
.kind(invar self: entry ) => enum<file, dir>
```
File object kind: file or directory
<a name="dirup"></a>
```ruby
.dirup(invar self: entry)=> dir|none
```
Directory which contains this entry
<a name="time"></a>
```ruby
.time(invar self: entry) => tuple<created: int, modified: int, accessed: int>
```
Time of creation, last modification and access (use `time` module to operate it)
<a name="owner"></a>
```ruby
.owner(invar self: entry) => string
```
Owner name
<a name="access"></a>
```ruby
.access(invar self: entry) => tuple<user: string, group: string, other: string>
.access=(self: entry, value: tuple<user: string, group: string, other: string>)
```
Access mode as a combination of 'r', 'w' and 'x' flags. On Windows, only permissions for the current user are affected
<a name="move"></a>
```ruby
move(self: entry, path: string)
```
Moves (renames) entry within the file system so that its full path becomes *path*. *path* may end with directory separator, omitting the entry name, in which case the current name is assumed
<a name="delete"></a>
```ruby
delete(self: entry)
```
Deletes file or empty directory

**Note:** Doing this does not invalidate the entry
<a name="refresh"></a>
```ruby
refresh(self: entry)
```
Re-reads all entry attributes

------
#### <a name="file">`fs::file`</a>
Inherits `fs::entry`. Represents file.
#### Methods
<a name="file_ctor"></a>
```ruby
file(path: string) => file
```
Returns `file` object bound to *path* if it points to a file, otherwise raises exception
<a name="size"></a>
```ruby
.size(invar self: file) => int
.size=(self: file, size: int)
```
Size of the file in bytes
<a name="copy"></a>
```ruby
copy(self: file, path: string) => file
```
Copies the file and returns `file` object of its copy

------
#### <a name="dir">`fs::dir`</a>
Inherits `fs::entry`. Represents directory.
#### Methods
<a name="dir_ctor"></a>
```ruby
dir(path: string) => dir
```
Returns `dir` object bount to *path* if it points to a directory, otherwise raises exception
<a name="mkfile"></a>
```ruby
mkfile(self: dir, path: string) => file
```
Creates new file given relative *path* and returns its `file` object
<a name="mkdir"></a>
```ruby
mkdir(self: dir, path: string) => dir
```
Creates new directory given relative *path* and returns its `dir` object
<a name="entries"></a>
```ruby
entries(invar self: dir, filter = '*', filtering: enum<wildcard,pattern> = $wildcard) => list<entry>
```
Returns the list of inner entries with names matching *filter*, where *filter* type is defined by *filtering* and can be either a wildcard pattern or Dao string pattern
<a name="files"></a>
```ruby
files(invar self: dir, filter = '*', filtering: enum<wildcard,pattern> = $wildcard) => list<file>
```
Returns the list of inner files with names matching *filter*, where *filter* type is defined by *filtering and can be either a wildcard pattern or Dao string pattern
<a name="dirs"></a>
```ruby
dirs(invar self: dir, filter = '*', filtering: enum<wildcard,pattern> = $wildcard) => list<dir>
```
Returns the list of inner directories with names matching *filter*, where *filter* type is defined by *filtering* and can be either a wildcard pattern or Dao string pattern
<a name="op_index"></a>
```ruby
[](invar self: dir, path: string) => entry
```
Returns sub-entry given its relative *path*
<a name="exists"></a>
```ruby
exists(invar self: dir, path: string) => bool
```
Returns `$true` if sub-entry specified by relative *path* exists
<a name="mktemp"></a>
```ruby
mktemp(self: dir, prefix = '') => file
```
Creates file with unique name prefixed by *prefix* in this directory. Returns the corresponding `entry`

------
### Routines
<a name="cwd"></a>
```ruby
cwd() => dir
```
Returns the current working directory
<a name="cd"></a>
```ruby
cd(invar path: dir)
cd(path: string)
```
Makes *dir* the current working directory
<a name="ls"></a>
```ruby
ls(invar path: dir) => list<string>
ls(path = '.') => list<string>
```
Returns list of names of all file objects in the directory specified by *path*
<a name="realpath"></a>
```ruby
realpath(path: string) => string
```
Returns absolute form of *path*, which must point to an existing file or directory. On Windows, replaces all '\' in path with '/'
<a name="exists"></a>
```ruby
exists(path: string) => bool
```
Returns `$true` if *path* exists and points to a file or directory
<a name="roots"></a>
```ruby
roots() => list<string>
```
On Windows, returns list of root directories (drives). On other systems returns `{'/'}`
<a name="home"></a>
```ruby
home() => dir
```
Returns home directory for the current user (on Windows, 'Documents' directory is assumed)
