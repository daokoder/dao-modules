## web.json -- JSON parsing and serialization

The module provides way to map JSON data to Dao data and vice versa.

### Index
namespace [json](#json)

Types:
- [Data](#data)
- [Object](#othertypes)
- [Array](#othertypes)

interface [Encodable](#encodable)
- [encode](#encode)(invar _self_: Encodable) => Data

interface [Decodable](#decodable)
- [decode](#decode)(invar _data_: Data) => Decodable

interface [Marshallable](#marshallable)
- [marshall](#marshall)(invar _self_: Marshallable) => Object|Array

interface [Unmarshallable](#unmarshallable)
- [unmarshall](#unmarshall)(invar _document_: Object|Array) => Unmarshallable

Functions:
- [parse](#parse)(_str_: string) => Object|Array
- [serialize](#serialize1)(invar _data_: Object|Array, _style_: enum&lt;pretty,compact&gt; = $pretty) => string
- [serialize](#serialize2)(invar _data_: any, _style_: enum&lt;pretty,compact&gt; = $pretty)[invar _item_: any => Data|list&lt;any&gt;|map&lt;string,any&gt;] => string

<a name="json"></a>
### Types
<a name="data"></a>
```ruby
type Data = none|bool|int|float|string|list<Data>|map<string,Data>
```
Matches any JSON data type.
<a name="othertypes"></a>
```ruby
type Object = map<string,Data>
type Array = list<Data>
```
Aliases for JSON array and object
### Interfaces
#### <a name="encodable">json::Encodable</a>
A type which can be encoded to JSON data. Use it in conjunction with [Marshallable](#marshallable) to define serialization of custom data structures to JSON
#### Methods
```ruby
encode(invar self: Encodable) => Data
```
Serializes self to JSON data

-----
#### <a name="decodable">json::Decodable</a>
A type which can be decoded from JSON data. Use it in conjunction with [Unmarshallable](#unmarshallable) to define deserialization of custom data structures from JSON
#### Methods
```ruby
decode(invar data: Data) => Decodable
```
Deserializes self from the provided JSON *data*

-----
#### <a name="marshallable">json::Marshallable</a>
A type which can be marshalled to a JSON document
#### Methods
```ruby
marshal(invar self: Marshallable) => Object|Array
```
Serializes self to JSON document

-----
#### <a name="unmarshallable">json::Unmarshallable</a>
A type which can be unmarshalled from a JSON document
#### Methods
```ruby
unmarshal(invar document: Object|Array) => Unmarshallable
```
Deserializes self from the given JSON *document*
### Functions
<a name="parse"></a>
```ruby
parse(str: string) => Object|Array
```
Parses JSON in *str* and returns the corresponding map or list.

Deserialization of values (JSON => Dao):
- array  => list
- object => map
- number => int or double (depending on the presence of decimal separator)
- string => string
- null   => none
- bool   => bool

**Errors:** `JSON` in case of parsing error, or when resulting data is not a single JSON array or object
<a name="serialize1"></a>
```ruby
serialize(invar data: Object|Array, style: enum<pretty,compact> = $pretty) => string
```
Serializes *data* to JSON and returns the resulting string. When *style* is $pretty, the output includes newlines and indentation for readability, otherwise
the result is put on single line.

Serialization of values (Dao => JSON):
- list => array
- map  => object
- int, float => number
- string => string
- none => null
- bool => bool
<a name="serialize2"></a>
```ruby
serialize(invar data: any, style: enum<pretty,compact> = $pretty)[invar item: any => Data|list<any>|map<string,any>] => string
```
Similar to usual [serialize()](#serialize1), but accepts arbitrary data as input. Each item of type other then `int`, `float`, `bool`,`none`, `string`, `map` or `list`
found in *data* is passed to the specified code section, which should implement its conversion to [Data](#data), `list` or `map`. The serialization then proceeds recursively
