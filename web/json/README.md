## web.json -- JSON parsing and serialization

The module provides way to map JSON data to Dao data and vice versa.

### Index
namespace [json](#json)

Types:
- [Data](#data)

Functions:
- [parse](#parse)(_str_: string) => map&lt;string,Data&gt;|list&lt;Data&gt;
- [serialize](#serialize1)(invar _data_: map&lt;string,Data&gt;|list&lt;Data&gt;, _style_: enum&lt;pretty,compact&gt; = $pretty) => string
- [serialize](#serialize2)(invar _data_: any, _style_: enum&lt;pretty,compact&gt; = $pretty)[invar _item_: any => Data|list&lt;any&gt;|map&lt;string,any&gt;] => string

<a name="json"></a>
### Types
<a name="data"></a>
```ruby
type Data = none|bool|int|float|string|list<Data>|map<string,Data>
```
Matches any JSON data type.
### Functions
<a name="parse"></a>
```ruby
parse(str: string) => map<string,Data>|list<Data>
```
Parses JSON in *str* and returns the corresponding map or list.

Deserialization of values (JSON => Dao):
- array  => list
- object => map
- number => int or double (depending on the presence of decimal separator)
- string => string
- null   => none
- bool   => bool
<a name="serialize1"></a>
```ruby
serialize(invar data: map<string,Data>|list<Data>, style: enum<pretty,compact> = $pretty) => string
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
