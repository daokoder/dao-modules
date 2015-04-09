## os.process -- creating child processes and communicating with them

This module provides cross-platform interface to spawn OS sub-processes, control their execution and communicate with them via pipes.

### Index
namespace [os](#os)

class [Process](#process)
- [.id](#procid)(invar _self_: Process) => int
- [.program](#program)(invar _self_: Process) => string
- [.arguments](#arguments)(invar _self_: Process) => list&lt;string&gt;
- [.workDir](#workdir)(invar _self_: Process) => string
- [.environment](#environment)(invar _self_: Process) => list&lt;string&gt;|none
- [.status](#status)(invar _self_: Process) => enum&lt;running,finished,terminated,detached&gt;
- [.exitCode](#.exitcode)(invar _self_: Process) => int|none
- [wait](#procwait)(invar _self_: Process, _timeout_ = -1.0) => bool
- [terminate](#terminate)(_self_: Process, _how_: enum&lt;gracefully,forcibly&gt; = $forcibly)
- [.input](#input)(invar _self_: Process) => Pipe|none
- [.output](#output)(invar _self_: Process) => Pipe|none
- [.errors](#errors)(invar _self_: Process) => Pipe|none

class [Pipe](#pipe)
- [.id](#pipeid)(invar _self_: Pipe) => tuple&lt;read: int, write: int&gt;
- [.syncRead](#syncread)(invar _self_: Pipe) => bool
- [.syncRead=](#syncread)(_self_: Pipe, _value_: bool)
- [.syncWrite](#syncwrite)(invar _self_: Pipe) => bool
- [.syncWrite=](#syncwrite)(_self_: Pipe, _value_: bool)
- [.autoClose](#autoclose)(invar _self_: Pipe) => bool
- [check](#check)(_self_: Pipe, _what_: enum&lt;readable,writable,open,eof&gt;) => bool
- [read](#read)(_self_: Pipe, _count_ = -1) => string
- [write](#write)(_self_: Pipe, _data_: string)
- [wait](#pipewait)(invar _self_: Pipe, _timeout_ = -1.0) => bool
- [close](#close)(_self_: Pipe)
- [close](#close)(_self_: Pipe, _end_: enum&lt;read,write,both&gt;)

Functions:
- [exec](#exec)(_path_: string, invar _arguments_: list&lt;string&gt;, ...:
    tuple&lt;enum&lt;dir&gt;, string&gt; | tuple&lt;enum&lt;environ&gt;, invar&lt;list&lt;string&gt;&gt;&gt; |
    tuple&lt;enum&lt;stdin,stdout,stderr&gt;, Pipe|io::Stream&gt; | tuple&lt;enum&lt;detached&gt;, bool&gt;) => Process
- [shell](#shell)(_command_: string, ...:
    tuple&lt;enum&lt;dir&gt;, string&gt; | tuple&lt;enum&lt;environ&gt;, invar&lt;list&lt;string&gt;&gt;&gt; |
    tuple&lt;enum&lt;stdin,stdout,stderr&gt;, Pipe|io::Stream&gt; | tuple&lt;enum&lt;detached&gt;, bool&gt;) => Process
- [wait](#funwait)(invar _children_: list&lt;Process&gt;, &lt;timeout&gt; = -1.0) => Process|none
- [pipe](#pipe_ctor)(_autoClose_ = true) => Pipe
- [pipe](#pipe_ctor2)(_name_: string, _mode_: string, _action_: enum&lt;create&gt;, _autoClose_ = true) => Pipe
- [pipe](#pipe_ctor3)(_name_: string, _mode_: string, _action_: enum&lt;open&gt;, _autoClose_ = true) => Pipe
- [select](#select)(invar _pipes_: list&lt;Pipe&gt;, _timeout_ = -1.0) => Pipe|none

<a name="os"></a>
### Classes
#### <a name="process">`os::Process`</a>
Represents child process. All child processes are automatically tracked by single background tasklet, which automatically updates `Process` objects associated with the sub-processes.
#### Methods
<a name="procid"></a>
```ruby
.id(invar self: Process) => int
```
PID (0 if the process exited)
<a name="program"></a>
```ruby
.program(invar self: Process) => string
```
Name of the program being executed
<a name="arguments"></a>
```ruby
.arguments(invar self: Process) => list<string>
```
Program arguments
<a name="workdir"></a>
```ruby
.workDir(invar self: Process) => string
```
Working directory (may be relative to the current working directory at the time of process creation)
<a name="environment"></a>
```ruby
.environment(invar self: Process) => list<string>|none
```
Process environment in the form of a list of 'name=value' strings (`none` if the process environment is inherited)
<a name="status"></a>
```ruby
.status(invar self: Process) => enum<running,finished,terminated,detached>
```
Current process status (detached processes always have `$detached` status).

__Note:__ On Windows, a process is deemed terminated if its exit code is less then 0. On Unix, it means that the process exited because it received a signal
<a name="exitcode"></a>
```ruby
.exitCode(invar self: Process) => int|none
```
Process exit code, or `none` if the process is still running or is detached.

__Note:__ On Unix, exit code consists of the lowest 8 bits of exit status when the process exited normally. On Windows, it is complete 32-bit value returned by the process

<a name="procwait"></a>
```ruby
wait(invar self: Process, timeout = -1.0) => bool
```
Waits *timeout* seconds for process to exit.  If *timeout* is less then 0, waits indefinitely. Returns `true` if not timeouted

__Note:__ Cannot be used on detached processes
**Errors:** `Process` if used on a detached process
<a name="terminate"></a>
```ruby
terminate(self: Process, how: enum<gracefully,forcibly> = $forcibly)
```
Attempts to terminate the process the way specified by *how*. On Unix, sends SIGTERM (*how* is `$gracefully`) or SIGKILL (*how* is `$forcibly`). On Windows, terminates the process forcibly (regardless of *how*) and sets its exit code to -1
<a name="input"></a>
```ruby
.input(invar self: Process) => Pipe|none
```
Writable input pipe (stdin stream of the process), or `none` if input stream was redirected to a file
<a name="output"></a>
```ruby
.output(invar self: Process) => Pipe|none
```
Readable output pipe (stdout stream of the process), or `none` if output stream was redirected to a file
<a name="errors"></a>
```ruby
.errors(invar self: Process) => Pipe|none
```
Readable error pipe (stderr stream of the process), or `none` if error stream was redirected to a file

------
#### <a name="pipe">`os::Pipe`</a>
Represents pipe to be used for inter-process communication, implements `io::Device`.
#### Methods
<a name="pipeid"></a>
```ruby
.id(invar self: Pipe) => tuple<read: int, write: int>
```
ID of the read and write ends of the pipe (file descriptors on Unix, handles on Windows)
<a name="syncread"></a>
```ruby
.syncRead(invar self: Pipe) => bool
.syncRead=(self: Pipe, value: bool)
```
Determines if synchronous (blocking) mode is used for reading (`true` by default)

**Errors:** `Pipe` if failed to set new mode
<a name="syncwrite"></a>
```ruby
.syncWrite(invar self: Pipe) => bool
.syncWrite=(self: Pipe, value: bool)
```
Determines if synchronous (blocking) mode is used for writing (`true` by default)

**Errors:** `Pipe` if failed to set new mode
<a name="autoclose"></a>
```ruby
.autoClose(invar self: Pipe) => bool
```
Indicates that unused pipe end is automatically closed when the pipe is passed to a process (`true` by default)
<a name="check"></a>
```ruby
check(self: Pipe, what: enum<readable,writable,open,eof>) => bool
```
Checks if the pipe is in the state specified by *what*
<a name="read"></a>
```ruby
read(self: Pipe, count = -1) => string
```
Reads at most *count* bytes from the pipe, or all available data if *count* is less then 0

**Errors:** `Pipe` if the pipe is not readable or if failed to open it for reading
<a name="write"></a>
```ruby
write(self: Pipe, data: string)
```
Writes *data* to the input stream of the process. If *data* were not fully written, the method will raise `Pipe::Buffer` error containing the number of bytes which were not written

**Errors:** `Pipe::Buffer` on partial write (see above), `Pipe` if the pipe is not writable or if failed to open it for writing
<a name="pipewait"></a>
```ruby
wait(invar self: Pipe, timeout = -1.0) => bool
```
Waits *timeout* seconds for the pipe to become available for reading. If *timeout* is less then 0, waits indefinitely. Returns `true` if not timeouted

__Warning:__ On Windows, `wait()` uses system clock for short-term sleeping, thus the accuracy of waiting with timeout may vary depending on the current clock resolution
**Errors:** `Pipe` in case of system error
<a name="close"></a>
```ruby
close(self: Pipe)
close(self: Pipe, end: enum<read,write,both>)
```
Closes the specified *end* of the pipe, or both ends if *end* is not given

------
### Functions
<a name="exec"></a>
```ruby
exec(path: string, invar arguments: list<string>, ...:
    tuple<enum<dir>, string> | tuple<enum<environ>, invar<list<string>>> |
    tuple<enum<stdin,stdout,stderr>, Pipe|io::Stream> | tuple<enum<detached>, bool>) => Process
```

Creates new child process executing the file specified by *path* with the *arguments* (if given). *path* may omit the full path to the file, in that case its resolution is system-dependent. Returns the corresponding [Process](#proces) object.

Additional variadic parameters may specify the following process parameters:
- `dir` -- working directory
- `environ` -- environment (list of 'name=value' items specifying environment variables)
- `detached` -- if `true`, the process is detached from the calling process (on Unix, it also starts new session and becomes leader of new process group)

If pipe for any of the standard streams is not specified, it is automatically created with non-blocking mode set. Unless the user-specified pipe is created with [autoClose](#autoclose) set to `false`, its unused end is automatically closed.

__Note:__ On Windows, the given *path* and *arguments* are concatenated into command line, where all component are wrapped in quotes (existing quotes are escaped) and separated by space characters. If such behavior is undesirable for some case, use [shell()](#shell) instead.

__Note:__ The routine will fail with error on Windows if the execution cannot be started (for instance, *path* is not valid); on Unix, you may need to examine the process status or exit code (which will be set to 1) to detect execution failure.

__Warning:__ On Windows, environment variable strings are assumed to be in local or ASCII encoding

**Errors:** `Value` in case of invalid environment variable definition, `Value` when `io::Stream` used for redirection is not a file or is not readable/writable (depending on what is redirected), `Process` if failed to start new process
<a name="shell"></a>
```ruby
shell(command: string, ...:
    tuple<enum<dir>, string> | tuple<enum<environ>, invar<list<string>>> |
    tuple<enum<stdin,stdout,stderr>, Pipe|io::Stream> | tuple<enum<detached>, bool>) => Process
```
Similar to [exec()](#exec), but calls system shell ('/bin/sh -c' on Unix, 'cmd /C' on Windows) with the given *command*. *command* is passed to the shell 'as-is', so you need to ensure that it is properly escaped
<a name="funwait"></a>
```ruby
wait(invar children: list<Process>, timeout = -1.0) => Process|none
```
Waits for one of child processes in *children* to exit for *timeout* seconds (waits indefinitely if *timeout* is less then 0). Returns the first found exited process, or `none` if timeouted or if *children* is empty

__Warning:__ If the function is called concurrently from multiple threads, it will ignore the processes which are currently
tracked by its other invocations. Detached processes are ignored as well
<a name="pipe_ctor"></a>
```ruby
pipe(autoClose = true) => Pipe
```
Creates new pipe and returns the corresponding [Pipe](#pipe) object. If *autoClose* is `true`, unused pipe end is automatically closed when the pipe is passed to [exec()](#exec) or [shell()](#shell); setting this parameter to `false` allows to pass single pipe to multiple processes, in which case you should manually close the unused pipe end (if any) in order to enable EOF check

**Errors:** `Pipe` if failed to create new pipe
<a name="pipe_ctor2"></a>
```ruby
pipe(name: string, mode: string, action: enum<create>, autoClose = true) => Pipe
```
Creates named pipe with the specified *name* and access *mode* ('r', 'w' or 'rw'), returns the corresponding [Pipe](#pipe) object. For the description of  *autoClose* parameter, see [pipe()](#pipe_ctor).

On Windows, named pipe is created with name '\\.\pipe\' + *name*, open mode corresponding to *mode* (inbound, outbound or duplex for 'r', 'w' and 'rw' accordingly) and full sharing. This pipe is removed from the system when no process has references to it (including the process which created the pipe).

On Unix, FIFO file is created with the path specified by *name*, and is opened with read or write access (both cannot be specified). Depending on the system, the routine may block until another process opens this file with the opposite access type. The file is automatically unlinked from the file system when the `Pipe` object returned by this routine is fully closed, but it will remain accessible via the existing references to it

**Errors:** `Param` when *mode* is not valid, `Pipe` if failed to create new pipe
<a name="pipe_ctor3"></a>
```ruby
pipe(name: string, mode: string, action: enum<open>, autoClose = true) => Pipe
```
Opens existing named pipe with the specified *name* using the given access *mode* ('r', 'w' or 'rw'), returns the corresponding [Pipe](#pipe) object. For the description of *autoClose* parameter, see [pipe()](#pipe_ctor).

On Windows, named pipe with name '\\.\pipe\' + *name* is opened. For named pipe created with duplex mode ('rw'), any of the possible *mode* values are acceptable, pipe created with inbound ('r') or outbound('w') mode can only be opened with the opposite access type.

On Unix, FIFO file with path specified by *name* is opened with read or write access (both cannot be specified), the routine may block (depending on the system) until another process opens this file with the opposite access type

**Errors:** `Value` when *name* does not point to existing named pipe, `Param` when *mode* is not valid, `Pipe` if failed to open the pipe
<a name="select"></a>
```ruby
select(invar pipes: list<Pipe>, timeout = -1.0) => Pipe|none
```
Waits *timeout* seconds for one of *pipes* to become available for reading. If *timeout* is less then 0, waits
indefinitely. Returns the first found readable pipe, or `none` if timeouted or if *pipes* is empty.

__Warning:__ On Windows, `select()` uses system clock for short-term sleeping, thus the accuracy of waiting with timeout may vary depending on the current clock resolution
**Errors:** `Pipe` if failed to select pipes
