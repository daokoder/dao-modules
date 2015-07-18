## binary -- handling of binary data

The module contains functions to read and write binary data, encode it in textual form and decode it back.  

### Index
namespace [bin](#bin)

class [Encoder](#encoder)
- [Encoder](#encoder_ctor)()
- [Encoder](#encoder_ctor)(_sink_: io::Stream)
- [.stream](#encoder_stream)(invar _self_: Encoder) => io::Stream
- [.bytesWritten](#byteswritten)(invar _self_: Encoder) => int
- [i8](#writenum)(_self_: Encoder, _value_: int) => Encoder
- [u8](#writenum)(_self_: Encoder, _value_: int) => Encoder
- [i16](#writenum)(_self_: Encoder, _value_: int) => Encoder
- [u16](#writenum)(_self_: Encoder, _value_: int) => Encoder
- [i32](#writenum)(_self_: Encoder, _value_: int) => Encoder
- [u32](#writenum)(_self_: Encoder, _value_: int) => Encoder
- [i64](#writenum)(_self_: Encoder, _value_: int) => Encoder
- [u64](#writenum)(_self_: Encoder, _value_: int) => Encoder
- [f32](#writenum)(_self_: Encoder, _value_: float) => Encoder
- [f64](#writenum)(_self_: Encoder, _value_: float) => Encoder
- [bytes](#writebytes)(_self_: Encoder, _bytes_: string) => Encoder

class [Decoder](#decoder)
- [Decoder](#decoder_ctor)(_sink_: io::Stream)
- [.stream](#decoder_stream)(invar _self_: Encoder) => io::Stream
- [.bytesWritten](#bytesread)(invar _self_: Encoder) => int
- [i8](#readnum)(_self_: Encoder) => int
- [u8](#readnum)(_self_: Encoder) => int
- [i16](#readnum)(_self_: Encoder) => int
- [u16](#readnum)(_self_: Encoder) => int
- [i32](#readnum)(_self_: Encoder) => int
- [u32](#readnum)(_self_: Encoder) => int
- [i64](#readnum)(_self_: Encoder) => int
- [u64](#readnum)(_self_: Encoder) => int
- [f32](#readnum)(_self_: Encoder) => float
- [f64](#readnum)(_self_: Encoder) => float
- [bytes](#readbytes)(_self_: Encoder, _count_: int) => string

interface [Encodable](#encodable)
- [encode](#encode_in)(invar _self_: Encodable, _encoder_: Encoder)

interface [Decodable](#decodable)
- [decode](#decode_in)(_decoder_: Decoder) => Decodable

Functions:
- [read](#read)(_source_: io::FileStream, _dest_: array&lt;@T&lt;int|float|complex&gt;&gt;, _count_ = 0) => int
- [unpack](#unpack)(_source_: io::FileStream, _dest_: array&lt;int&gt;, _size_: enum&lt;byte,word,dword&gt;, _count_ = 0) => int
- [pack](#pack)(invar _source_: array&lt;int&gt;, _dest_: io::FileStream, _size_: enum&lt;byte,word,dword&gt;, _count_ = 0) => int
- [write](#write)(invar _source_: array&lt;@T&lt;int|float|complex&gt;&gt;, _dest_: io::FileStream, _count_ = 0) => int
- [get](#get1)(invar _source_: array&lt;int&gt;|string, _what_: enum&lt;byte,ubyte,word,uword,dword,udword,qword,uqword&gt;, _offset_: int) => int
- [get](#get1)(invar _source_: array&lt;int&gt;|string, _what_: enum&lt;float&gt;, _offset_: int) => float
- [get](#get2)(invar _source_: array&lt;int&gt;|string, _what_: enum&lt;bits&gt;, _offset_: int, _count_: int) => int
- [set](#set1)(_dest_: array&lt;int&gt;, _what_: enum&lt;byte,ubyte,word,uword,dword,udword,qword,uqword&gt;, _offset_: int, value: int)
- [set](#set1)(_dest_: array&lt;int&gt;, _what_: enum&lt;float&gt;, _offset_: int, _value_: float)
- [set](#set2)(_dest_: array&lt;int&gt;, _what_: enum&lt;bits&gt;, _offset_: int, _count_: int, _value_: int)
- [encode](#encode)(_str_: string, _codec_: enum&lt;base64,z85,hex&gt;) => string
- [decode](#decode)(_str_: string, _codec_: enum&lt;base64,z85,hex&gt;) => string

<a name="bin"></a>
### Classes
#### <a name="encoder">`bin::Encoder`</a>
Stateful binary encoder which uses an `io::Stream` as sink
#### Methods
<a name="encoder_ctor"></a>
```ruby
Encoder()
Encoder(sink: io::Stream)
```
Creates an encoder writing to *sink* (if omitted, new string stream is created)

**Errors:** `Param` when *sink* is not writable
<a name="encoder_stream"></a>
```ruby
.stream(invar self: Encoder) => io::Stream
```
Sink stream
<a name="byteswritten"></a>
```ruby
.bytesWritten(invar self: Encoder) => int
```
Number of bytes successfully written to the sink
<a name="writenum"></a>
```ruby
i8(self: Encoder, value: int) => Encoder
u8(self: Encoder, value: int) => Encoder
i16(self: Encoder, value: int) => Encoder
u16(self: Encoder, value: int) => Encoder
i32(self: Encoder, value: int) => Encoder
u32(self: Encoder, value: int) => Encoder
i64(self: Encoder, value: int) => Encoder
u64(self: Encoder, value: int) => Encoder
f32(self: Encoder, value: float) => Encoder
f64(self: Encoder, value: float) => Encoder
```
Writes *value* and returns self.

__Note:__ Multibyte integer values are written with big-endian byte order
__Warning:__ Floating point value representation may vary on different platforms

**Errors:** `Value` if the stream was closed
<a name="writebytes"></a>
```ruby
bytes(self: Encoder, bytes: string) => Encoder
```
Writes *bytes* to the sink and returns self

**Errors:** 'Value' if the stream was closed

------
#### <a name="decoder">`bin::Decoder`</a>
Stateful binary decoder which uses an `io::Stream` as source
#### Methods
<a name="decoder_ctor"></a>
```ruby
Decoder(source: io::Stream)
```
Creates a decoder readin from *source*

**Errors:** `Param` when *source* is not readable
<a name="decoder_stream"></a>
```ruby
.stream(invar self: Decoder) => io::Stream
```
Source stream
<a name="bytesread"></a>
```ruby
.bytesRead(invar self: Decoder) => int
```
Number of bytes successfully written to the sink
<a name="writenum"></a>
```ruby
i8(self: Decoder) => int
u8(self: Decoder) => int
i16(self: Decoder) => int
u16(self: Decoder) => int
i32(self: Decoder) => int
u32(self: Decoder) => int
i64(self: Decoder) => int
u64(self: Decoder) => int
f32(self: Decoder) => float
f64(self: Decoder) => float
```
Reads a value from the source.

__Note:__ Multibyte integer values are assumed to be in big-endian byte order
__Warning:__ Floating point value representation may vary on different platforms

**Errors:** `Value` if the stream was closed
<a name="writebytes"></a>
```ruby
bytes(self: Encoder, bytes: string) => Encoder
```
Reads at most *count* bytes from the source

**Errors:** `Value` if the stream was closed

### Interfaces
#### <a name="encodable">`bin::Encodable`</a>
A type which can be encoded to binary form. Use it to define conversions to specific serialization formats
#### Methods
<a name="encode_in"></a>
```ruby
encode(invar self: Encodable, encoder: Encoder)
```
Serializes self using the provided *encoder*

------
#### <a name="decodable">`bin::Decodable`</a>
A type which can be decoded from binary form. Use it to define conversions from specific serialization formats
#### Methods
<a name="decode_in"></a>
```ruby
decode(decoder: Decoder) => Decodable
```
Deserializes self using the provided *decoder*

### Functions
<a name="read"></a>
```ruby
read(source: io::FileStream, dest: array<@T<int|float|complex>>, count = 0) => int
```
Reads *count* elements from *source* to *dest*. If *count* is zero, or greater than *dest* size, *dest* size is assumed. Returns the number of elements actually read

**Errors:** `Param` when *source* is not a file stream or is not readable
<a name="unpack"></a>
```ruby
unpack(source: io::FileStream, dest: array<int>, size: enum<byte,word,dword>, count = 0) => int
```
Reads *count* chunks of size *size* from *source* into *dest* so that each chunk corresponds to a single *dest* element (with possible widening). If *count* is zero, or greater than *dest* element size, *dest* element size is assumed. Returns the number of chunks actually read

**Errors:** `Param` when *source* is not a file stream or is not readable
<a name="pack"></a>
```ruby
pack(invar source: array<int>, dest: io::FileStream, size: enum<byte,word,dword>, count = 0) => int
```
Writes *count* chunks of size *size* to *dest* so that each *source* element corresponds to a single chunk (with possible narrowing). Returns the number of chunks actually written

**Errors:** `Param` when *dest* is not a file stream or is not writable
<a name="write"></a>
```ruby
write(invar source: array<@T<int|float|complex>>, dest: io::FileStream, count = 0) => int
```
Writes *count* elements from *source* to *dest*. If *count* is zero, or greater than *dest* size, all *dest* data is written. Returns the number of elements
actually written

**Errors:** `Param` when *dest* is not a file stream or is not writable
<a name="get1"></a>
```ruby
get(invar source: array<int>|string, what: enum<byte,ubyte,word,uword,dword,udword,qword,uqword>, offset: int) => int
get(invar source: array<int>|string, what: enum<float>, offset: int) => float
```
Reads value described by *what* from *source* at the given byte *offset*

**Errors:** `Index::Range` when *offset* is invalid
<a name="get2"></a>
```ruby
get(invar source: array<int>|string, what: enum<bits>, offset: int, count: int) => int
```
Reads *count* bits from *source* at the given bit *offset*

**Note:** *count* must not exceed 32
**Errors:** `Index::Range` for invalid *offset*, `Param` for invalid *count*
<a name="set1"></a>
```ruby
set(dest: array<int>, what: enum<byte,ubyte,word,uword,dword,udword,qword,uqword>,offset: int, value: int)
set(dest: array<int>, what: enum<float>, offset: int, value: float)
```
Writes *value* described by *what* to *dest* at the given byte *offset*

**Errors:** `Index::Range` when *offset* is invalid
<a name="set2"></a>
```ruby
set(dest: array<int>, what: enum<bits>, offset: int, count: int, value: int)
```
Writes *count* lower bits of *value* to *dest* at the given *offset*

**Note:** *count* must not exceed 32
**Errors:** `Index::Range` for invalid *offset*, `Param` for invalid *count*
<a name="encode"></a>
```ruby
encode(str: string, codec: enum<base64,z85,hex>) => string
```
Returns *str* encoded with the given *codec*.

**Note:** For Z85 codec, *str* size must be a multiple of 4
**Errors:** `Bin` when the above does not hold
<a name="decode"></a>
```ruby
decode(str: string, codec: enum<base64,z85,hex>) => string
```
Returns *str* decoded with the given *codec*

**Errors:** `Bin` when *str* is not a valid *codec*-encoded string
