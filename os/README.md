## os -- common OS-related functionality

### Module tree

[os](#os) -- miscellaneous functionality
- fs -- file system interface
- path -- abstract path handling
- process -- creating and managing processes

### Index
namespace [os](#os)

Constants:
- ATOMIC_WRITE_CAP -- Maximum number of bytes which can be atomically written to a device

Functions:
- [run](#run)(_command_: string) => int
- [pclose](#pclose)(_pipe_: os::PipeStream) => int
- [sleep](#sleep)(_seconds_: float)
- [exit](#exit)(_code_ = 0)
- [clock](#clock)() => float
- [setlocale](#setlocale)(_category_: enum&lt;all,collate,ctype,monetary,numeric,time&gt; = $all, _locale_ = "")
- [environ](#environ)() => map&lt;string,string&gt;
- [getenv](#getenv)( _name_: string ) => string
- [putenv](#putenv)( _name_: string, value = "" )
- [user](#user)() => string
- [uname](#uname)() => tuple&lt;_system_: string, _version_: string, _release_: string, _host_: string&gt;
- [null](#null)() => io::Stream

<a name="os"></a>
### Functions
<a name="run"></a>
```ruby
run(command: string) => int
```
Executes the given *command* via system shell and returns the resulting exit code
<a name="sleep"></a>
```ruby
sleep(seconds: float)
```
Suspends execution of the current thread for the specified amount of *seconds*

**Errors:** `Param` when *seconds* is negative
<a name="exit"></a>
```ruby
exit(code = 0)
```
Exits immediately, setting the specified exit *code*
<a name="clock"></a>
```ruby
clock(clock) => float
```
CPU time of the current process in seconds
<a name="setlocale"></a>
```ruby
setlocale(category: enum<all,collate,ctype,monetary,numeric,time> = $all, locale = "") => string
```
Sets *locale* for the given *category* and returns previous locale

**Errors:** `System` when *locale* is invalid
<a name="environ"></a>
```ruby
environ() => map<string,string>
```
Process environment
<a name="getenv"></a>
```ruby
getenv(name: string) => string
```
Value of environment variable *name*
<a name="putenv"></a>
```ruby
putenv(name: string, value = "")
```
Sets environment variable *name* to the given *value*

**Errors:** `System` if failed to set environment variable
<a name="user"></a>
```ruby
user(name: string, value = "")
```
Name of the user associated with the current process

**Errors:** `System` if failed to get information about the user
<a name="uname"></a>
```ruby
uname() => tuple<system: string, version: string, release: string, host: string>
```
Generic system information: operating system name, version and release, computer host name

**Errors:** `System` if failed to get information about the system
<a name="null"></a>
```ruby
null() => io::Stream
```
Returns write-only stream corresponding to system null device

**Errors:** `System` if failed to open the null device
