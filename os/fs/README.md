## os.fs -- generic file system interface

The module provides interface to common operations on files and directories in platform-independent way.

### Index
namespace [fs](#fs)

class [Entry](#entry)
- [.path](#path)(invar _self_: Entry) => string
- [.name](#name)(invar _self_: Entry) => string
- [.basename](#basename)(invar _self_: Entry) => string
- [.suffix](#suffix)(invar _self_: Entry) => string
- [.kind](#kind)(invar _self_: Entry ) => enum&lt;file,dir&gt;
- [.dirup](#dirup)(invar _self_: Entry) => Dir|none
- [.time](#time)(invar _self_: Entry) => tuple&lt;_created_: int, _modified_: int, _accessed_: int&gt;
- [.owner](#owner)(invar _self_: Entry) => string
- [.access](#access)(invar _self_: Entry) => tuple&lt;_user_: string, _group_: string, _other_: string&gt;
- [.access=](#access)(self: _entry_, _value_: tuple&lt;_user_: string, _group_: string, _other_: string&gt;)
- [move](#move)(_self_: Entry, _path_: string)
- [delete](#delete)(_self_: Entry)
- [refresh](#refresh)(_self_: Entry)
- [(string)](#cast_string)(invar _self_: Entry) => string

class [File](#file): Entry
- [.size](#size)(invar _self_: File) => int
- [.size=](#size)(_self_: File, _size_: int)
- [copy](#copy)(_self_: File, _path_: string) => File
- [copy](#copy2)(_self_: File, _to_: Dir) => File

class [Dir](#dir): Entry
- [newFile](#mkfile)(_self_: Dir, _path_: string) => File
- [newDir](#mkdir)(_self_: Dir, _path_: string) => Dir
- [entries](#entries)(invar _self_: Dir, _filter_ = '*', _filtering_: enum&lt;wildcard,pattern&gt; = $wildcard) => list&lt;Entry&gt;
- [files](#files)(invar _self_: Dir, _filter_ = '*', _filtering_: enum&lt;wildcard,pattern&gt; = $wildcard) => list&lt;File&gt;
- [dirs](#dirs)(invar _self_: Dir, _filter_ = '*', _filtering_: enum&lt;wildcard,pattern&lt;= $wildcard) => list&lt;Dir&gt;
- [<span>[]</span>](#op_index)(invar _self_: Dir, _path_: string) => Entry|none
- [exists](#exists)(invar _self_: Dir, _path_: string) => bool
- [newTmpFile](#mktemp)(_self_: Dir, _prefix_ = '') => File

Functions:
- [entry](#entry_ctor)(_path_: string) => Entry
- [file](#file_ctor)(_path_: string) => File
- [dir](#dir_ctor)(_path_: string) => Dir
- [cwd](#cwd)() => Dir
- [cd](#cd)(invar _path_: Dir)
- [cd](#cd)(_path_: string)
- [mkdir](#fs_mkdir)(_path_: string)
- [ls](#ls)(invar _path_: Dir) => list&lt;string&gt;
- [ls](#ls)(_path_ = '.') => list&lt;string&gt;
- [rm](#rm)(_path_: string)
- [realpath](#realpath)(_path_: string) => string
- [symlink](#symlink)(_path_: string, _link_: string)
- [readlink](#readlink)(_link_: string) => string
- [exists](#exists)(_path_: string) => bool
- [roots](#roots)() => list&lt;string&gt;
- [home](#home)() => Dir

<a name="fs"></a>
### Classes
#### <a name="entry">`fs::Entry`</a>
Represents a generic file system object, namely a file or directory (links and other special types of file-like objects are not supported). `entry` is inherited by `File` and `Dir`, which provide operations specific to files and directories accordingly.

A file object is operated by its name, no descriptors or locks are kept for the lifetime of the associated `Entry`. File attributes are cached and are only updated when necessary (e.g., `resize()` will update the size attribute); use `refresh()` to re-read them.

`Entry` uses '/' as the unified path separator on all platforms, Windows paths (including UNC) are automatically normalized to this form. Relative paths and symbolic links are automatically expanded to their absolute form.

**Note:** On Windows, all path strings are assumed to be encoded in UTF-8, and are implicitly converted to UTF-16 in order to support Unicode file names on this platform.
#### Methods
<a name="entry_ctor"></a>
<a name="path"></a>
```ruby
.path(invar self: Entry) => string
```
Full path
<a name="name"></a>
```ruby
.name(invar self: Entry) => string
```
Entry name (last component of path)
<a name="basename"></a>
```ruby
.basename(invar self: Entry) => string
```
Base name (up to, but not including, the first '.' in name)
<a name="suffix"></a>
```ruby
.suffix(invar self: Entry) => string
```
Name part after the last '.'
<a name="kind"></a>
```ruby
.kind(invar self: Entry ) => enum<file,dir>
```
File object kind: file or directory
<a name="dirup"></a>
```ruby
.dirup(invar self: Entry)=> Dir|none
```
Directory which contains this entry

**Errors:** `File` if failed to read upper directory data
<a name="time"></a>
```ruby
.time(invar self: Entry) => tuple<created: int, modified: int, accessed: int>
```
Time of creation, last modification and access (use `time` module to operate it)
<a name="owner"></a>
```ruby
.owner(invar self: Entry) => string
```
Owner name

**Errors:** `File` if failed to get information about owner
<a name="access"></a>
```ruby
.access(invar self: Entry) => tuple<user: string, group: string, other: string>
.access=(self: Entry, value: tuple<user: string, group: string, other: string>)
```
Access mode as a combination of 'r', 'w' and 'x' flags. On Windows, only permissions for the current user are affected

**Errors:** `Param` when trying to set invalid mode *value*, `File` if failed to set access mode
<a name="move"></a>
```ruby
move(self: Entry, path: string)
```
Moves (renames) entry within the file system so that its full path becomes *path*. *path* may end with directory separator, omitting the entry name, in which case the current name is assumed

**Errors:** `File` if failed
<a name="delete"></a>
```ruby
delete(self: Entry)
```
Deletes file or empty directory

**Note:** Doing this does not invalidate the entry
**Errors:** `File` if failed
<a name="refresh"></a>
```ruby
refresh(self: Entry)
```
Re-reads all entry attributes

**Errors:** `File` if failed to read entry data
<a name="cast_string"></a>
```ruby
(string)(invar self: Entry) => string
```
String representation of the entry (its full path)

------
#### <a name="file">`fs::File`</a>
Inherits `fs::Entry`. Represents file.
#### Methods
<a name="size"></a>
```ruby
.size(invar self: File) => int
.size=(self: File, size: int)
```
Size of the file in bytes

**Errors:** `File` when resizing failed or when trying to set size of a directory
<a name="copy"></a>
```ruby
copy(self: File, path: string) => File
```
Copies the file to *path* and returns `File` object of its copy. *path* may end with '/' to indicate the directory to copy to (preserving the original file name)

**Errors:** `Param` if *path* is empty, `File` if failed to copy the file or read data of the copy
<a name="copy2"></a>
```ruby
copy(self: File, to: Dir) => File
```
Copies the file to the directory specified by *to* and returns *File* object of its copy

------
#### <a name="dir">`fs::Dir`</a>
Inherits `fs::Entry`. Represents directory.
#### Methods
<a name="mkfile"></a>
```ruby
newFile(self: Dir, path: string) => File
```
Creates new file given relative *path* and returns its `File` object

**Errors:** `File` if failed
<a name="mkdir"></a>
```ruby
newDir(self: Dir, path: string) => Dir
```
Creates new directory given relative *path* and returns its `Dir` object

**Errors:** `File` if failed
<a name="entries"></a>
```ruby
entries(invar self: Dir, filter = '*', filtering: enum<wildcard,pattern> = $wildcard) => list<Entry>
```
Returns the list of inner entries with names matching *filter*, where *filter* type is defined by *filtering* and can be either a wildcard pattern or Dao string pattern

**Errors:** `Param` when *filter* is too large, `File` in case of file system related error
<a name="files"></a>
```ruby
files(invar self: Dir, filter = '*', filtering: enum<wildcard,pattern> = $wildcard) => list<File>
```
Returns the list of inner files with names matching *filter*, where *filter* type is defined by *filtering and can be either a wildcard pattern or Dao string pattern
<a name="dirs"></a>
```ruby
dirs(invar self: Dir, filter = '*', filtering: enum<wildcard,pattern> = $wildcard) => list<Dir>
```
Returns the list of inner directories with names matching *filter*, where *filter* type is defined by *filtering* and can be either a wildcard pattern or Dao string pattern
<a name="op_index"></a>
```ruby
[](invar self: Dir, path: string) => Entry|none
```
Returns sub-entry given its relative *path*, or `none` if *path* does not point to existing file or directory

**Errors:** `File` if failed to read sub-entry data
<a name="exists"></a>
```ruby
exists(invar self: Dir, path: string) => bool
```
Returns `true` if sub-entry specified by relative *path* exists
<a name="mktemp"></a>
```ruby
newTmpFile(self: Dir, prefix = '') => File
```
Creates file with unique name prefixed by *prefix* in this directory. Returns the corresponding `Entry`

**Errors:** `Param` if *prefix* contains directory separators, `File` if failed to create file

------
### Functions
<a name="entry_ctor"></a>
```ruby
entry(path: string) => Entry
```
Returns new `Entry` bound to *path* of file or directory

**Errors:** `File` if failed to read entry data or it is not a file or directory
<a name="file_ctor"></a>
```ruby
file(path: string) => File
```
Returns `File` object bound to *path* if it points to a file, otherwise raises exception

**Errors:** `File` if failed to read entry data or it is not a file
<a name="dir_ctor"></a>
```ruby
dir(path: string) => Dir
```
Returns `Dir` object bound to *path* if it points to a directory, otherwise raises exception

**Errors:** `File` if failed to read entry data or it is not a directory
<a name="cwd"></a>
```ruby
cwd() => Dir
```
Returns the current working directory

**Errors:** `File` if failed to read entry data of the current directory
<a name="cd"></a>
```ruby
cd(invar path: Dir)
cd(path: string)
```
Makes *Dir* the current working directory

**Errors:** `File` if failed
<a name="fs_mkdir"></a>
```ruby
mkdir(path: string)
```
Creates new directory given its relative *path*

**Errors:** `File` if failed
<a name="ls"></a>
```ruby
ls(invar path: Dir) => list<string>
ls(path = '.') => list<string>
```
Returns list of names of all file objects in the directory specified by *path*

**Errors:** `File` if failed
<a name="rm"></a>
```ruby
rm(path: string)
```
Deletes file object specified by *path*

**Errors:** `File` if failed
<a name="realpath"></a>
```ruby
realpath(path: string) => string
```
Returns absolute form of *path*, which must point to an existing file or directory. On Windows, replaces all '\' in path with '/'

**Errors:** `File` if there were errors resolving the path
<a name="symlink"></a>
```ruby
symlink(path: string, link: string)
```
Creates symbolic *link* to *path* (Unix-specific)

**Errors:** `File` if failed
<a name="readlink"></a>
```ruby
readlink(link: string) => string
```
Returns file name to which symbolic *link* is pointed (Unix-specific). If *link* does not specify a symbolic link, returns empty string

**Errors:** `File` if failed
<a name="exists"></a>
```ruby
exists(path: string) => bool
```
Returns `true` if *path* exists
<a name="roots"></a>
```ruby
roots() => list<string>
```
On Windows, returns list of root directories (drives). On other systems returns `{'/'}`

**Errors:** `File` if failed
<a name="home"></a>
```ruby
home() => Dir
```
Returns home directory for the current user (on Windows, 'Documents' directory is assumed)

**Errors:** `File` if failed
