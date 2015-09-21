## net -- basic networking

### Module tree

[net](#net) -- sockets, addresses and miscellaneous functionality
- http -- generic low-level HTTP support

### Index
namespace [net](#net)

Constants
- MAX_BACKLOG -- Largest possible `backLog` value for [TcpListener::listen()](#tcp_listen)
- SELECT_CAP -- Maximum number of file descriptors [net.select()](#select) can handle for each input list

invar type [Ipv4Addr](#ipv4)
- [Ipv4Addr](#ipv4_ctor)(_value_: string)
- [Ipv4Addr](#ipv4_ctor)(_a_: int, _b_: int, _c_: int, _d_: int)
- [.value](#ipv4_value)(_self_: Ipv4Addr) => string
- [.octets](#ipv4_octets)(_self_: Ipv4Addr) => tuple&lt;int,int,int,int>
- [(string)](#ipv4_string)(_self_: Ipv4Addr)
- [is](#ipv4_is)(_self_: Ipv4Addr, _what_: enum&lt;unspecified,multicast,private,global,loopback,linkLocal,broadcast>) => bool
- [<](#ipv4_cmp)(_a_: Ipv4Addr, _b_: Ipv4Addr) => bool
- [<=](#ipv4_cmp)(_a_: Ipv4Addr, _b_: Ipv4Addr) => bool
- [==](#ipv4_cmp)(_a_: Ipv4Addr, _b_: Ipv4Addr) => bool
- [!=](#ipv4_cmp)(_a_: Ipv4Addr, _b_: Ipv4Addr) => bool
- [+](#ipv4_arith)(_a_: Ipv4Addr, _b_: int) => Ipv4Addr
- [-](#ipv4_arith)(_a_: Ipv4Addr, _b_: int) => Ipv4Addr

invar class [SocketAddr](#sockaddr)
- [SocketAddr](#sockaddr_ctor1)(_value_: string)
- [SocketAddr](#sockaddr_ctor2)(_ip_: Ipv4Addr, _port_: int)
- [.ip](#sockaddr_ip)(_self_: SocketAddr) => Ipv4Addr
- [.port](#sockaddr_port)(_self_: SocketAddr) => int
- [.value](#sockaddr_value)(_self_: SocketAddr) => string
- [(string)](#sockaddr_string)(_self_: SocketAddr)
- [==](#sockaddr_cmp)(_a_: SocketAddr, _b_: SocketAddr) => bool
- [!=](#sockaddr_cmp)(_a_: SocketAddr, _b_: SocketAddr) => bool

class [Socket](#socket)
- [fd](#sock_fd)(invar _self_: Socket) => int
- [.localAddr](#sock_localaddr)(invar _self_: Socket) => SocketAddr
- [.state](#sock_state)(invar _self_: Socket ) => enum&lt;closed,bound,listening,connected>
- [close](#sock_close)(_self_: Socket)

class [TcpStream](#tcpstream)
- [connect](#tcp_connect)(_self_: TcpStream, _addr_: string|SocketAddr)
- [write](#tcp_write)(_self_: TcpStream, _data_: string)
- [writeFile](#tcp_writefile)(_self_: TcpStream, _file_: string)
- [read](#tcp_read)(_self_: TcpStream, _count_ = -1) => string
- [readAll](#tcp_readall)(_self_: TcpStream, _count_: int, _prepend_ = '') => string
- [.peerAddr](#tcp_peeraddr)(invar _self_: TcpStream) => SocketAddr
- [check](#tcp_check)(_self_: TcpStream, _what_: enum&lt;readable,writable,open,eof>) => bool
- [shutdown](#tcp_shutdown)(_self_: TcpStream, _what_: enum&lt;send,receive,all>)
- [.keepAlive](#tcp_keepalive)(invar _self_: TcpStream ) => bool
- [.keepAlive=](#tcp_keepalive)(_self_: TcpStream, _value_: bool)
- [.noDelay](#tcp_nodealy)(invar _self_: TcpStream) => bool
- [.noDelay=](#tcp_nodealy)(_self_: TcpStream, _value_: bool)

class [TcpListener](#tcplistener)
- [listen](#listen)(_self_: TcpListener, _addr_: string|SocketAddr, _backLog_ = 10, _binding_: enum&lt;exclusive,reused> = $exclusive)
- [accept](#accept)(_self_: TcpListener) => tuple&lt;stream: TcpStream, addr: SocketAddr>
- [for](#for)(_self_: TcpListener, _iterator_: ForIterator)
- [<span>[]</span>](_self_: TcpListener, _index_: ForIterator) => tuple&lt;stream: TcpStream, addr: SocketAddr>

class [UdpSocket](#udpsocket)
- [bind](#udp_bind)(_self_: UdpSocket, _addr_: string|SocketAddr, _binding_: enum&lt;exclusive,reused> = $exclusive)
- [write](#udp_write)(_self_: UdpSocket, _addr_: string|SocketAddr, _data_: string)
- [read](#udp_read)(_self_: UdpSocket, _limit_ = 4096) => tuple&lt;addr: SocketAddr, data: string>
- [.broadcast](#udp_broadcast)(invar _self_: UdpSocket) => bool
- [.broadcast=](#udp_broadcast)(_self_: UdpSocket, _value_: bool)
- [.multicastLoop](#udp_multicastloop)(invar _self_: UdpSocket) => bool
- [.multicastLoop=](#udp_multicastloop)(_self_: UdpSocket, _value_: bool)
- [joinGroup](#udp_join)(_self_: UdpSocket, _group_: string|Ipv4Addr)
- [leaveGroup](#udp_leave)(_self_: UdpSocket, _group_: string|Ipv4Addr)
- [.multicastTtl](#udp_multicastttl)(invar _self_: UdpSocket) => int
- [.multicastTtl=](#udp_multicastttl)(_self_: UdpSocket, _value_: int)
- [.ttl](#udp_ttl)(invar _self_: UdpSocket) => int
- [.ttl=](#udp_ttl)(_self_: UdpSocket, _value_: int)

Functions
- [listen](#listen)(_addr_: string|SocketAddr, _backLog_ = 10, _binding_: enum&lt;exclusive,reused> = $exclusive) => TcpListener
- [bind](#bind)(_addr_: string|SocketAddr, _binding_: enum&lt;exclusive,reused> = $exclusive) => UdpSocket
- [connect](#connect)(_addr_: string|SocketAddr) => TcpStream
- [host](#host)(_id_: string) => tuple<name: string, aliases: list<string>, addrs: list<Ipv4Addr>>|none
- [service](#service)(_id_: string|int, _proto_: enum&lt;tcp,udp>) => tuple&lt;name: string, port: int, aliases: list<string>>|none
- [select](#select)(invar _read_: list&lt;@R&lt;Socket|int>>, invar _write_: list&lt;@W&lt;Socket|int>>, _timeout_: float ) => tuple&lt;status: enum&lt;selected,timeouted,interrupted>, read: list&lt;@R>, write: list&lt;@W>>

<a name="net"></a>
### Types
####<a name="ipv4">`net::Ipv4Addr`</a>

IPv4 address as a CPOD type
#### Methods
<a name="ipv4_ctor"></a>
```ruby
Ipv4Addr(value: string)
Ipv4Addr(a: int, b: int, c: int, d: int)
```
Creates IPv4 address from dotted string *value* or given the individual octets *a*, *b*, *c* and *d*

**Errors:** `Param` in case of invalid address
<a name="ipv4_value"></a>
```ruby
.value(self: Ipv4Addr) => string
```
Address as a dotted string value
<a name="ipv4_octets"></a>
```ruby
.octets(self: Ipv4Addr) => tuple<int,int,int,int>
```
Individual address octets
<a name="ipv4_string"></a>
```ruby
(string)(self: Ipv4Addr)
```
Same as [.value()](#ipv4_value)
<a name="ipv4_is"></a>
```ruby
is(self: Ipv4Addr, kind: enum<unspecified,multicast,private,global,loopback,linkLocal,broadcast>) => bool
```
Returns `true` if the address belongs to the specified kind
<a name="ipv4_cmp"></a>
```ruby
<(a: Ipv4Addr, b: Ipv4Addr) => bool
<=(a: Ipv4Addr, b: Ipv4Addr) => bool
==(a: Ipv4Addr, b: Ipv4Addr) => bool
!=(a: Ipv4Addr, b: Ipv4Addr) => bool
```
Address comparison
<a name="ipv4_arith"></a>
```ruby
+(a: Ipv4Addr, b: int) => Ipv4Addr
-(a: Ipv4Addr, b: int) => Ipv4Addr
```
Address arithmetics

-----
####<a name="sockaddr">`net::SocketAddr`</a>

Socket address, consisting of [Ipv4Addr](#ipv4) and port number
#### Methods
<a name="sockaddr_ctor1"></a>
```ruby
SocketAddr(value: string)
```
Creates socket address given 'host:port' *value* (if the host part is empty, '0.0.0.0' is assumed)

**Errors:** `Param` in case of invalid address, 'Network::Host' in case of host lookup error
<a name="sockaddr_ctor2"></a>
```ruby
SocketAddr(ip: Ipv4Addr, port: int)
```
Creates socket address given or *ip* and *port*

**Errors:** `Param` in case of invalid address
<a name="sockaddr_ip"></a>
```ruby
.ip(self: SocketAddr) => Ipv4Addr
```
IP address
<a name="sockaddr_port"></a>
```ruby
.port(self: SocketAddr) => int
```
Port number
<a name="sockaddr_value"></a>
```ruby
.value(self: SocketAddr) => string
```
'ip:port' string
<a name="sockaddr_value"></a>
```ruby
(string)(self: SocketAddr)
```
Same as [.value()](#sockaddr_value)
<a name="sockaddr_cmp"></a>
```ruby
==(a: SocketAddr, b: SocketAddr) => bool
!=(a: SocketAddr, b: SocketAddr) => bool
```
Address comparison

-----
####<a name="socket">`net::Socket`</a>

Abstract socket type
#### Methods
<a name="sock_fd"></a>
```ruby
.fd(invar self: Socket) => int
```
Socket file descriptor
<a name="sock_localaddr"></a>
```ruby
.localAddr(invar self: Socket) => SocketAddr
```
Address to which the socket is bound

**Errors:** `Network` in case of failure
<a name="sock_state"></a>
```ruby
.state( invar self: Socket ) => enum<closed,bound,listening,connected>
```
Current socket state
<a name="sock_close"></a>
```ruby
close(self: Socket)
```
Closes the socket

-----
####<a name="tcpstream">`net::TcpStream`</a>

Connected TCP socket
#### Methods
<a name="tcp_connect"></a>
```ruby
connect(self: TcpStream, addr: string|SocketAddr)
```
Connects to address *addr* which may be either a 'host:port' string or [SocketAddr](#sockaddr)

**Errors:** [SocketAddr()](#sockaddr_ctor1) errors, `Network` in case of failure
<a name="tcp_write"></a>
```ruby
write(self: TcpStream, data: string)
```
Sends *data*

**Errors:** `Network` in case of failure
<a name="tcp_writefile"></a>
```ruby
writeFile(self: TcpStream, file: string)
```
Sends *file*

**Errors:** `Network` in case of failure
<a name="tcp_read"></a>
```ruby
read(self: TcpStream, count = -1) => string
```
Receives at most *count* bytes (64Kb max, 4Kb if *count* <= 0) and returnes the received data

**Errors:** `Network` in case of failure
<a name="tcp_readall"></a>
```ruby
readAll(self: TcpStream, count: int, prepend = '') => string
```
Attempts to receive exactly *count* bytes of data and return it. Will return less data only if the connection was closed.

If *prepend* is given, it will be prepended to the resulting string. Use case: receiving a message with a header and a large body (e.g., HTTP response). First, the header is read, by which the body size is determined. While doing this, part of the message body might be read along with the header. Concatenating it with the rest of the body after receiving the latter would nullify the performance advantage of using *readAll()*, so that chunk should be passed as *prepend* parameter

**Errors:** `Param` in case of invalid *count*, `Network` in case of failure
<a name="tcp_peeraddr"></a>
```ruby
.peerAddr(invar self: TcpStream) => SocketAddr
```
Peer address of the connected socket

**Errors:** `Network` in case of failure
<a name="tcp_peeraddr"></a>
```ruby
check(self: TcpStream, what: enum<readable,writable,open,eof>) => bool
```
Checks the property specified by *what*; required to satisfy `io::Device` interface
<a name="tcp_shutdown"></a>
```ruby
shutdown(self: TcpStream, what: enum<send,receive,all>)
```
Fully or partially shuts down the connection, stopping further operations specified by *what*

**Errors:** `Network` in case of failure
<a name="tcp_keepalive"></a>
```ruby
.keepAlive(invar self: TcpStream) => bool
.keepAlive=(self: TcpStream, value: bool)
```
 TCP keep-alive option (SO_KEEPALIVE)

 **Errors:** `Network` in case of get/set failure
 <a name="tcp_keepalive"></a>
 ```ruby
 .noDelay(invar self: TcpStream) => bool
 .noDelay=(self: TcpStream, value: bool)
 ```
 TCP no-delay option (SO_NODELAY)

 **Errors:** `Network` in case of get/set failure

 -----
 ####<a name="tcplistener">`net::TcpListener`</a>

 Listening TCP socket
 #### Methods
 <a name="tcp_listen"></a>
 ```ruby
listen(self: TcpListener, addr: string|SocketAddr, backLog = 10, binding: enum<exclusive,reused> = $exclusive)
 ```
 Binds the socket to address *addr* (either a 'host:port' string or [SocketAddr](#sockaddr)) using *binding* option (see [net.listen()](#listen) for its description). Sets the socket into the listening state using *backLog* as the maximum size of the queue of pending connections (use `MAX_BACKLOG` constant to assign the maximum queue size)

**Errors:** [SocketAddr()](#sockaddr_ctor1) errors, `Param` on invalid *backLog*, `Network` in case of failure
 <a name="tcp_accept"></a>
 ```ruby
accept(self: TcpListener) => tuple<stream: TcpStream, addr: SocketAddr>
 ```
 Accepts new connection, returning the corresponding [TcpStream](#tcpstream) and peer address

 **Errors:** `Network` in case of failure
 ```ruby
for(self: TcpListener, iterator: ForIterator)
[](self: TcpListener, index: ForIterator) => tuple<stream: TcpStream, addr: SocketAddr>
 ```
Equivalient to an infinite loop calling [.accept()](#tcp_accept) on each iteration

-----
####<a name="udpsocket">`net::UdpSocket`</a>

UDP socket
#### Methods
<a name="udp_bind"></a>
```ruby
bind(self: UdpSocket, addr: string|SocketAddr, binding: enum<exclusive,reused> = $exclusive)
```
Binds the socket to address *addr* (either a 'host:port' string or [SocketAddr](#sockaddr)) using *binding* option (see [net.listen()](#listen) for its description)

**Errors:** [SocketAddr()](#sockaddr_ctor1) errors, `Network` in case of failure
<a name="udp_write"></a>
```ruby
write(self: UdpSocket, addr: string|SocketAddr, data: string)
```
Sends *data* to the receiver specified by address *addr* which is either a 'host:port' string or [SocketAddr](#sockaddr)

**Errors:** `Network` in case of failure
<a name="udp_read"></a>
```ruby
read(self: UdpSocket, limit = 4096 ) => tuple<addr: SocketAddr, data: string>)
```
Receives at most *limit* bytes and returnes the received data and the address of its sender

**Errors:** `Network` in case of failure
<a name="udp_broadcast"></a>
```ruby
.broadcast(invar self: UdpSocket) => bool
.broadcast=(self: UdpSocket, value: bool)
```
UDP broadcast option (SO_BROADCAST)

**Errors:** `Network` in case of get/set failure
<a name="udp_multicastloop"></a>
```ruby
.multicastLoop(invar self: UdpSocket) => bool
.multicastLoop=(self: UdpSocket, value: bool)
```
Multicast loop socket option (IP_MULTICAST_LOOP)

**Errors:** `Network` in case of get/set failure
<a name="udp_join"></a>
```ruby
joinGroup(self: UdpSocket, group: string|Ipv4Addr)
```
Joins multicast *group* specified by its host name or ip address

**Errors:** `Network::Host` on host lookup error, `Network` in case of failure
<a name="udp_leave"></a>
```ruby
leaveGroup(self: UdpSocket, group: string|Ipv4Addr)

**Errors:** `Network::Host` on host lookup error, `Network` in case of failure
```
<a name="udp_multicastttl"></a>
```ruby
.multicastTtl(invar self: UdpSocket) => int
.multicastTtl=(self: UdpSocket, value: int)
```
Multicast TTL value (IP_MULTICAST_TTL)

**Errors:** `Network` in case of get/set failure
<a name="udp_ttl"></a>
```ruby
.ttl(invar self: UdpSocket) => int
.ttl=(self: UdpSocket, value: int)
```
TTL value (IP_TTL)

**Errors:** `Network` in case of get/set failure
### Functions

<a name="listen"></a>
```ruby
listen(addr: string|SocketAddr, backLog = 10, binding: enum<exclusive,reused> = $exclusive) => TcpListener
```
Returns new TCP listener bound to address *addr* (either a 'host:port' string or [SocketAddr](#sockaddr)) with *binding* option used to regulate the possibility of address rebinding. The socket is put in the listening state using *backLog* as the maximum size of the queue of pending connections (use `MAX_BACKLOG` constant to assign the maximum queue size).

Meaning of *binding* values:
-`exclusive` -- exclusive binding of the address which excludes rebinding (SO_EXCLUSIVEADDRUSE on Windows, ignored on Unix)
-`reused` -- allows the address to be rebound if it is not already bound exclusively (SO_REUSEADDR on Windows and Unix)

**Errors:** [SocketAddr()](#sockaddr_ctor1) errors, `Param` on invalid *backLog*, `Network` in case of failure
<a name="bind"></a>
```ruby
bind(addr: string|SocketAddr, binding: enum<exclusive,reused> = $exclusive) => UdpSocket
```
Returns new UDP socket bound to address *addr* (either a 'host:port' string or [SocketAddr](#sockaddr)) with *binding* option (see [net.listen()](#listen) for its description)

**Errors:** [SocketAddr()](#sockaddr_ctor1) errors, `Network` in case of failure
<a name="connect"></a>
```ruby
connect(addr: string|SocketAddr) => TcpStream
```
Returns client-side TCP connection endpoint connected to address *addr* which may be either a 'host:port' string or [SockAddr](#sockaddr)

**Errors:** [SocketAddr()](#sockaddr_ctor1) errors, `Network` in case of failure
<a name="host"></a>
```ruby
host(id: string) => tuple<name: string, aliases: list<string>, addrs: list<Ipv4Addr>>|none
```
Returns information for host with the given *id* (which may be either a name or an IPv4 address in dotted form). Returns `none` if the host is not found or does not have an IP address

**Errors:** `Network::Host` on host lookup error
<a name="service"></a>
```ruby
service(id: string|int, proto: enum<tcp,udp>) => tuple<name: string, port: int, aliases: list<string>>|none
```
Returns information for service with the given *id* (which may be either a name or a port number) to be used with the protocol *proto*. Returns `none` if the corresponding service was not found
<a name="select"></a>
```ruby
select(invar read: list<@R<Socket|int>>, invar write: list<@W<Socket|int>>, timeout: float) => tuple<status: enum<selected,timeouted,interrupted>, read: list<@R>, write: list<@W>>
```
Waits *timeout* seconds for any *Socket* or file descriptor in *read* or *write* list to become available for reading or writing accordingly. Returns sub-lists of *read* and *write* containing available sockets/descriptors.

__Note:__ On Windows, only socket descriptors can be selected
**Errors:** `Param` if both lists are empty or one of them contains an invalid file descriptor or closed socket, `Network` in case of failure
