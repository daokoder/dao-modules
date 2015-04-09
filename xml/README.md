## xml -- reading, writing and modifying XML documents

The module implements simple XML parser (without DTD support), tree-based representation of XML documents and streaming XML writer.

### Index
namespace [xml](#xml)

class [Document](#document)
- [Document](#document_ctor)(invar _root_: Element) => Document
- [.version](#version)(invar self: Document) => string
- [.version=](#version)(_self_: Document, _value_: string)
- [.encoding](#encoding)(invar _self_: Document) => string
- [.encoding=](#encoding)(_self_: Document, _value_: string)
- [.standalone](#standalone)(_self_: Document) => bool
- [.standalone=](#standalone)(_self_: Document, _value_: bool)
- [.doctype](#doctype)(invar _self_: Document) => string
- [.doctype=](#doctype)(_self_: Document, _value_: string)
- [.instructions](#instructions)(invar _self_: Document) => list&lt;Instruction&gt;
- [.instructions=](#instructions)(_self_: Document, _value_: list&lt;Instruction&gt;)
- [.root](#root)(invar _self_: Document) => Element
- [.root=](#root)(_self_: Document, _value_: Element)
- [serialize](#serialize)(invar _self_: Document) => string

class [Instruction](#instruction)
- [Instruction](#instruction_ctor)(_name_: string, _data_ = '') => Instruction
- [.target](#target)(invar _self_: Instruction) => string
- [.target=](#target)(_self_: Instruction, _value_      : string)
- [.data](#in_data)(invar _self_: Instruction) => string
- [.data=](#in_data)(_self_: Instruction, _value_: string)

class [Element](#element)
- [Element](#element_ctor)(_tag_: string, ...: tuple&lt;enum, string&gt;) => Element
- [.tag](#tag)(invar _self_: Element) => string
- [.tag=](#tag)(_self_: Element, _value_: string)
- [<span>[]</span>](#index)(invar _self_: Element, _attrib_: string) => string
- [<span>[]=</span>](#index)(_self_: Element, _value_: string|none, _attrib_: string)
- [.attribs](#attribs)(invar _self_: Element) => map&lt;string,string&gt;
- [has](#has)(invar _self_: Element, _attrib_: string) => bool
- [.text](#text)(invar _self_: Element) => string
- [.text=](#text)(self: _Element_, value: string)
- [.size](#el_size)(invar _self_: Element) => int
- [map](#map)(invar _self_: Element, _what_: enum&lt;attribs,children&gt;, _mapping_: type&lt;@T&lt;tuple&lt;...&gt;&gt;&gt;) => @T
- [extend](#extend)(_self_: Element, ...: tuple&lt;enum, any&gt;)
- [.empty](#empty)(invar _self_: Element) => bool
- [.empty=](#empty)(_self_: Element, _value_: bool)
- [clear](#clear)(_self_: Element)
- [.children](#children)(invar _self_: Element) => list&lt;Element|Instruction|CharData&gt;
- [.children=](#children)(_self_: Element, _value_: list&lt;Element|Instruction|CharData&gt;)
- [child](#child)(invar _self_: Element, _at_: int) => Element|Instruction|CharData
- [.elements](#elements)(invar _self_: Element) => list&lt;Element&gt;
- [find](#find)(_self_: Element, _path_: string, ...: tuple&lt;enum, string&gt; => Element|none
- [select](#select)(invar _self_: Element, _path_: string, ...: tuple<&lt;num, string&gt;) => list&lt;Element&gt;
- [append](#el_append)(_self_: Element, _item_: Element|Instruction|CharData)
- [insert](#insert)(_self_: Element, _item_: Element|Instruction|CharData, _at_: int)
- [drop](#drop)(_self_: Element, _at_: int, _count_ = 1)
- [drop](#drop2)(_self_: Element, _child_: Element|Instruction|CharData)
- [namespaceName](#namespacename)(invar _self_: Element, _prefix_ = '') => string

class [CharData](#chardata)
- [CharData](#chardata_ctor)(_data_ = '', _kind_: enum&lt;text,cdata&gt; = $text) => CharData
- [.kind](#kind)(invar _self_: CharData) => enum&lt;text,cdata&gt;
- [.kind=](#kind)(_self_: CharData, _value_: enum&lt;text,cdata&gt;)
- [.data](#cd_data)(invar _self_: CharData) => string
- [.data=](#cd_data)(_self_: CharData, _value_: string)
- [.size](#cd_size)(invar _self_: CharData) => int
- [append](#cd_append)(self: CharData, value: string)

class [Writer](#writer)
- [Writer](#writer_ctor)(_dest_: io::Stream) => Writer
- [Writer](#writer_ctor2)() => Writer
- [.stream](#stream)(invar _self_: Writer) => invar&lt;io::Stream&gt;
- [flush](#flush)(_self_: Writer) => Writer
- [close](#close)(_self_: Writer)
- [raw](#raw)(_self_: Writer, _data_: string) => Writer
- [text](#wr_text)(_self_: Writer, _value_: int|float|enum|string) => Writer
- [cdata](#cdata)(_self_: Writer, _data_: string) => Writer
- [comment](#comment)(_self_: Writer, _text_: string) => Writer
- [tag](#wr_tag)(_self_: Writer, _name_: string, ...: tuple&lt;enum, string&gt;) => Writer
- [tag](#wr_tag2)(_self_: Writer, _name_: string, _attribs_: map&lt;string,string&gt;) => Writer
- [end](#end)(_self_: Writer) => Writer
- [header](#header)(_self_: Writer, _version_ = '1.0', _encoding_ = '', _standalone_ = '') => Writer
- [instruction](#wr_instruction)(_self_: Writer, _name_: string, _data_ = '') => Writer
- [doctype](#wr_doctype)(_self_: Writer, _dtd_: string) => Writer
<a name="xml"></a>

Functions:
- [parse](#parse)(_str_: string) => xml::Document

### Classes
#### <a name="document">`xml::Document`</a>
XML document.
#### Methods
<a name="document_ctor"></a>
```ruby
Document(invar root: Element) => Document
```
Constructs new XML document with *root* as its root element
<a name="version"></a>
```ruby
.version(invar self: Document) => string
.version=(self: Document, value: string)
```
XML version (in the header)

**Errors:** `XML` when version is not valid
<a name="encoding"></a>
```ruby
.encoding(invar self: Document) => string
.encoding=(self: Document, value: string)
```
Encoding (in the header)

__Note:__ Specifying encoding has no effect on actual encoding of resulting XML document
**Errors:** `XML` when encoding is not valid
<a name="standalone"></a>
```ruby
.standalone(self: Document) => bool
.standalone=(self: Document, value: bool)
```
Standalone document parameter (in the header)
<a name="doctype"></a>
```ruby
.doctype(invar self: Document) => string
.doctype=(self: Document, value: string)
```
Internal DTD section ('<!DOCTYPE ... >')

__Note:__ DTD is not interpreted and thus has no effect on treatment of elements and attributes
**Errors:** `XML` when DTD is not valid
<a name="instructions"></a>
```ruby
.instructions(invar self: Document) => list<Instruction>
.instructions=(self: Document, value: list<Instruction>)
```
Processing instructions outside of root element (placed before root element during serialization)

__Note:__ The list contains references to instructions; however, modifying the list itself has no effect on the document
<a name="root"></a>
```ruby
.root(invar self: Document) => Element
.root=(self: Document, value: Element)
```
Root element
<a name="serialize"></a>
```ruby
serialize(invar self: Document) => string
```
Returns XML representation of the whole document

------
#### <a name="instruction">`xml::Instruction`</a>
XML processing instruction.
#### Methods
<a name="instruction_ctor"></a>
```ruby
Instruction(name: string, data = '') => Instruction
```
Constructs XML processing instruction given its *name* and *data*
<a name="target"></a>
```ruby
.target(invar self: Instruction) => string
.target=(self: Instruction, value: string)
```
Instruction target

**Error:** `XML` when trying to set invalid target
<a name="in_data"></a>
```ruby
.data(invar self: Instruction) => string
.data=(self: Instruction, value: string)
```
Instruction data

**Error:** `XML` when trying to set invalid XML data
------
#### <a name="element">`xml::Element`</a>
XML element.
#### Methods
<a name="element_ctor"></a>
```ruby
Element(tag: string, ...: tuple<enum, string>) => Element
```
Constructs XML element with the given *tag*; if *tag* ends with '/', empty element ('<tag .../>') is created. Element attributes may be provided as name-value pairs via additional named parameters

**Error:** `XML` when tag or atributes are not valid
<a name="tag"></a>
```ruby
.tag(invar self: Element) => string
.tag=(self: Element, value: string)
```
Tag

**Error:** `XML` when trying to set invalid tag
<a name="index"></a>
```ruby
[](invar self: Element, attrib: string) => string
[]=(self: Element, value: string|none, attrib: string)
```
Attribute *attrib*

**Error:** `Param` upon getting when the element has no attribute *attrib*, `XML` upon setting when attribute name or value are not valid
<a name="index"></a>
```ruby
.attribs(invar self: Element) => map<string,string>
```
Returns map of all attributes

__Note:__ Modifying the returned map has no effect on the element
<a name="has"></a>
```ruby
has(invar self: Element, attrib: string) => bool
```
Returns `true` if element has attribute *attrib*
<a name="text"></a>
```ruby
.text(invar self: Element) => string
.text=(self: Element, value: string)
```
Treats element as one containing character data only. Getting text succeeds if element has single child representing character data, or has no chidren at all (but is not empty). Setting text of an element clears its list of children

**Error:** `Value` when the element is not a single character data item
<a name="el_size"></a>
```ruby
.size(invar self: Element) => int
```
Number of direct children
<a name="map"></a>
```ruby
map(invar self: Element, what: enum<attribs,children>, mapping: type<@T<tuple<...>>>) => @T
```
Maps either element attributes or its children depending on *what* and returns the resulting data. *mapping* must be a tuple type with named items only, each of which refers to an existing attribute or child element by its name/tag (if there are multiple elements with the given tag, the first one is taken). Leaf elements (containing character data only) and attributes can be mapped to `int`, `float`, `string`, `enum` or `bool`. Non-leaf elements can be mapped to a tuple type, in which case the mapping proceeds recursively. 8mapping* may omit unneeded attributes and elements

__Note:__ Use `tuple<tag: T,>` to map elements containing single sub-element

**Errors:** `Type` when *mapping* contains unnamed fields or fields with unsupported types, `Conversion` in case of mapping error, `Value` when the element does not contain some of requested attributes or sub-elements, or when sub-elements cannot be mapped
<a name="extend"></a>
```ruby
extend(self: Element, ...: tuple<enum, any>)
```
Additional named parameters of this method are converted into elements and appended to the list of children. For each parameter in the form *name => value*, an element '<name>value</name>' is created; specifying a tuple as value continues the conversion recursively. For leaf elements (containing character data only), supported types are `int`, `float`, `string`, `enum` and `bool`; `enum` flags are written separated by ';'

**Errors:** `Type` when tuple arguments contain unnamed fields or fields with unsupported types, `XML` in case of invalid character data
<a name="empty"></a>
```ruby
.empty(invar self: Element) => bool
.empty=(self: Element, value: bool)
```
Is `true` if element is an empty element ('<tag ... />'). Making an element empty erases its list of children
<a name="clear"></a>
```ruby
clear(self: Element)
```
Removes all attributes; for a non-empty element, sets its content to empty string
<a name="clear"></a>
```ruby
.children(invar self: Element) => list<Element|Instruction|CharData>
.children=(self: Element, value: list<Element|Instruction|CharData>)
```
The list of direct children

__Note:__ The returned list contains references to child items; however, modifying the list itself has no effect on the element
<a name="child"></a>
```ruby
child(invar self: Element, at: int) => Element|Instruction|CharData
```
Returns direct child with index *at*

**Error:** `Index::Range` wheh *at* is not valid
<a name="elements"></a>
```ruby
.elements(invar self: Element) => list<Element>
```
Returns the list of direct child elements

__Note:__ The returned list contains references to child items; however, modifying the list itself has no effect on the element
<a name="find"></a>
```ruby
find(self: Element, path: string, ...: tuple<enum, string>) => Element|none
```
Returns the first element among the children which matches specified description. *path* is a sequence of tags delimited by '/' (e.g. 'a', 'a/b', '/b', 'a/', 'a//c', '') pointing to the element; empty tag matches any element. Element attributes may be provided as name-value pairs via additional variadic parameters
<a name="select"></a>
```ruby
select(invar self: Element, path: string, ...: tuple<enum, string>) => list<Element>
```
Returns all elements among the children which match specified description. *path* is a sequence of tags delimited by '/' (e.g. 'a', 'a/b', '/b', 'a/', 'a//c', '') pointing to the elements; empty tag matches any element (for non-end tags, the first element is picked). Element attributes may be provided as name-value pairs via additional variadic parameters
<a name="el_append"></a>
```ruby
append(self: Element, item: Element|Instruction|CharData)
```
Appends *item* to the list of children

__Note:__ You cannot add the same element twice
**Error:** `XML` when *item* already has parent, or when attempting to append an element to itself
<a name="insert"></a>
```ruby
insert(self: Element, item: Element|Instruction|CharData, at: int)
```
Inserts *item* in the list of children at index *at*

**Error:** `Index::Range` when *at* in invalid, `XML` when *item* already has parent, or when attempting to append an element to itself
<a name="drop"></a>
```ruby
drop(self: Element, at: int, count = 1)
```
Removes at most *count* children at index *at* in the list of children

**Error:** `Index::Range` when *at* is invalid, `Param` when *count* is invalid
<a name="drop2"></a>
```ruby
drop(self: Element, child: Element|Instruction|CharData)
```
Removes *child* from the list of children
<a name="namespacename"></a>
```ruby
namespaceName(invar self: Element, prefix = '') => string
```
Returns namespace name (URI) associated with *prefix* (empty string if not found). If *prefix* is empty string, default namespace is assumed

__Warning:__ Obtaining inherited namespace succeeds only if parent elements are preserved

------
#### <a name="chardata">`xml::CharData`</a>
XML character data
#### Methods
<a name="chardata_ctor"></a>
```ruby
CharData(data = '', kind: enum<text,cdata> = $text) => CharData
```
Constructs XML character data containing *data*. Data representation form depends on *kind* and can be either plain text or CDATA section

**Error:** `XML` when data is not valid, or when it contains ']]>' while *kind* is `cdata`
<a name="kind"></a>
```ruby
.kind(invar self: CharData) => enum<text,cdata>
.kind=(self: CharData, value: enum<text,cdata>)
```
Representation form: plain text or CDATA section

**Error:** `XML` if setting `cdata` kind for text which contains ']]>'
<a name="cd_data"></a>
```ruby
.data(invar self: CharData) => string
.data=(self: CharData, value: string)
```
Represented character data

**Error:** `XML` when trying to set invalid data, or when it contains ']]>' and [kind](#kind) is `cdata`
<a name="cd_size"></a>
```ruby
.size(invar self: CharData) => int
```
Returns the size of character data
<a name="cd_append"></a>
```ruby
append(self: CharData, value: string)
```
Appends *value* to character data

**Error:** `XML` when trying to append invalid data, or when it contains ']]>' while [kind](#kind) is `cdata`

------
#### <a name="writer">`xml::Writer`</a>
Writable stream of XML data.
#### Methods
<a name="writer_ctor"></a>
```ruby
Writer(dest: io::Stream) => Writer
```
Creates XML writer which writes to stream *dest*

**Error:** `Param` when *dest* is not writable
<a name="writer_ctor2"></a>
```ruby
Writer() => Writer
```
Creates XML writer which writes to internal string stream
<a name="stream"></a>
```ruby
.stream(invar self: Writer) => invar<io::Stream>
```
Output stream
<a name="flush"></a>
```ruby
flush(self: Writer) => Writer
```
Flushes output stream and returns *self*
<a name="close"></a>
```ruby
close(self: Writer)
```
Closes output stream
<a name="raw"></a>
```ruby
raw(self: Writer, data: string) => Writer
```
Writes *data* as raw data (without escaping) and returns *self*

**Error:** `XML` in case of invalid data, `Param` when the stream is closed
<a name="wr_text"></a>
```ruby
text(self: Writer, value: int|float|enum|string) => Writer
```
Writes *value* as text and returns *self*. XML markup characters in resulting text are replaced with XML references

**Errors:** `XML` in case of invalid character data, `Param` when the stream is closed, `XML` when not inside a tag
<a name="cdata"></a>
```ruby
cdata(self: Writer, data: string) => Writer
```
Writes CDATA section containing *data* (not escaped) and returns *self*

**Errors:** `XML` in case of invalid character data, `Param` when the stream is closed, `XML` when not inside a tag
<a name="comment"></a>
```ruby
comment(self: Writer, text: string) => Writer
```
Writes comment containing escaped *text* and returns *self*

**Errors:** `XML` in case of invalid character data, `Param` when the stream is closed
<a name="wr_tag"></a>
```ruby
tag(self: Writer, name: string, ...: tuple<enum, string>) => Writer
```
Writes start tag or empty-element *name* and returns *self*. An empty element is assumed if *name* ends with '/'. Attributes may be provided as name-value pairs via additional named parameters

**Error:** `XML` in case of invalid tag or attributes, `Param` when the stream is closed
<a name="wr_tag2"></a>
```ruby
tag(self: Writer, name: string, attribs: map<string,string>) => Writer
```
Writes start tag or empty-element *name* with *attribs* and returns *self*. An empty element is assumed if *name* ends with '/'

**Error:** `XML` in case of invalid tag or attributes, `Param` when the stream is closed
<a name="end"></a>
```ruby
end(self: Writer) => Writer
```
Writes end tag matching the last written start tag and returns *self*

**Error:** `XML` when no tag is open
<a name="header"></a>
```ruby
header(self: Writer, version = '1.0', encoding = '', standalone = '') => Writer
```
Writes XML declaration containing *version*, *encoding* and *standalone* parameters (*encoding* and *standalone* will be omitted if emtpy), returns *self*

__Note:__ Specifying encoding has no effect on actual encoding of resulting XML document
**Error:** `XML` in case of invalid version, encoding or standalone parameter, or when header was already written, or when it is misplaced, `Param` when the stream is closed
<a name="wr_instruction"></a>
```ruby
instruction(self: Writer, name: string, data = '') => Writer
```
Writes processing instruction with *name* and escaped *data* and returns *self*

**Error:** `XML` in case of invalid name or data, `Param` when the stream is closed
<a name="wr_doctype"></a>
```ruby
doctype(self: Writer, dtd: string) => Writer
```
Writes DTD section and returns *self*; *dtd* should be in form of '<!DOCTYPE ... >'

**Error:** `XML` in case of invalid DTD, or when it was already written, or when it is misplaced, `Param` when the stream is closed
### Functions
<a name="parse"></a>
```ruby
parse(str: string) => xml::Document
```
Returns XML document parsed from *str*

**Errors:** `XML` in case of parsing error
