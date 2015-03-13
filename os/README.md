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
- [system](#system)(_command_: string) => int
- [popen](#popen)(_command_: string, mode: string) => os::PipeStream
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
<a name="system"></a>
```ruby
system(command: string) => int
```
Executes the given *command* via system shell and returns the resulting exit code
<a name="popen"></a>
```ruby
popen(command: string, mode: string) => os::PipeStream
```
Spawns sub-process which executes the given shell *command* with redirected standard input or output depending on *mode*.
If *mode* is 'r', returns readable stream of the process output; if *mode* is 'w', returns writable stream of the process input.
Resulting `os::PipeStream` inherits `io::Stream` and can be used the same way
<a name="pclose"></a>
```ruby
pclose(pipe: os::PipeStream) => int
```
Closes \a pipe created by `popen()`, waits for the sub-process to finish and returns its exit code
<a name="sleep"></a>
```ruby
sleep(seconds: float)
```
Suspends execution of the current thread for the specified amount of *seconds*
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
<a name="user"></a>
```ruby
user(name: string, value = "")
```
Name of the user associated with the current process
<a name="uname"></a>
```ruby
uname() => tuple<system: string, version: string, release: string, host: string>
```
Generic system information: operating system name, version and release, computer host name
<a name="null"></a>
```ruby
null() => io::Stream
```
Returns write-only stream corresponding to system null device
