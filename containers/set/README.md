## containers.set -- mutable set type

A mutable tree and hash set implementation based on the `map` type

### Index
namespace [std](#std)

class [Set](#set)
- [Set<@T>](#set_ctor)(_kind_: enum&lt;tree,hash> = $tree)
- [.size](#size)(invar _self_: Set&lt;@T>) => int
- [insert](#insert)(_self_: Set&lt;@T>, invar value: @T)
- [contains](#contains)(invar _self_: Set&lt;@T>, invar _value_: @T) => bool
- [<span>[]</span>](#contains)(invar _self_: Set&lt;@T>, invar _value_: @T) => bool
- [contains](#contains2)(invar _self_: Set&lt;@T>, invar other: Set&lt;@T>|list&lt;@T>) => bool
- [erase](#erase)(_self_: Set&lt;@T>, invar _value_: @T)
- [clone](#clone)(invar _self_: Set&lt;@T>) => Set&lt;@T>
- [clear](#clear)(_self_: Set&lt;@T>)
- [+=](#op_add_set)(_self_: Set&lt;@T>, invar _other_: Set&lt;@T>|list&lt;@T>) => Set&lt;int>
- [|=](#op_add_set)(_self_: Set&lt;@T>, invar _other_: Set&lt;@T>|list&lt;@T>) => Set&lt;int>
- [-=](#op_sub_set)(_self_: Set&lt;@T>, invar _other_: Set&lt;@T>|list&lt;@T>) => Set&lt;int>
- [+](#op_add)(invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [+](#op_add)(c: Set&lt;@T>, invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [|](#op_add)(invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [|](#op_add)(c: Set&lt;@T>, invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [-](#op_sub)(invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [-](#op_sub)(c: Set&lt;@T>, invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [&](#op_and)(invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [&](#op_and)(c: Set&lt;@T>, invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [^](#op_xor)(invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [^](#op_xor)(c: Set&lt;@T>, invar _a_: Set&lt;@T>, invar _b_: Set&lt;@T>|list&lt;@T>) => Set&lt;@T>
- [*](#op_mul)(invar _a_: Set&lt;@X>, invar _b_: Set&lt;@Y>|list&lt;@Y>) => Set&lt;tuple&lt;@X,@Y>>
- [*](#op_mul)(_c_: Set&lt;tuple&lt;@X,@Y>>, invar _a_: Set&lt;@X>, invar _b_: Set&lt;@Y>|list&lt;@Y>) => Set&lt;tuple&lt;@X,@Y>>
- [(string)](#string)(invar _self_: Set&lt;@T>)
- [items](#items)(invar _self_: Set&lt;@T>) => list&lt;@T>
- [==](#cmp)(invar _a_: Set&lt;@T>, invar b: Set&lt;@T>|list&lt;@T>) => bool
- [!=](#cmp)(invar _a_: Set&lt;@T>, invar b: Set&lt;@T>|list&lt;@T>) => bool
- [for](#for)(invar _self_: Set&lt;@T>, _iterator_: ForIterator)
- [<span>[]</span>](#for)(invar _self_: Set&lt;@T>, _index_: ForIterator) => @T
- [collect](#collect)(invar _self_: Set&lt;@T>)[invar _item_: @T => @V|none] => Set&lt;@V>
- [collect](#collect2)(invar _self_: Set&lt;@X>, invar _other_: Set&lt;@Y>)[invar _item_: @X, invar _item2_: @Y => @V|none] => Set&lt;@V>
- [associate](#associate)(invar _self_: Set&lt;@T>, _hashing_: enum&lt;none,auto,random>|int = $none)[invar _item_: @T => none|tuple&lt;@K,@V>] => map&lt;@K,@V>
- [associate](#associate2)(invar _self_: Set&lt;@X>, invar _other_: Set&lt;@Y>, _hashing_: enum&lt;none,auto,random>|int = $none)[invar _item_: @X, invar _item2_: @Y => none|tuple&lt;@K,@V>] => map&lt;@K,@V>
- [find](#find)(invar _self_: Set&lt;@T>)[invar _item_: @T => bool] => @T|none
- [reduce](#reduce)(invar _self_: Set&lt;@T>)[invar _item_: @T, _value_: @T => @T] => @T|none
- [reduce](#reduce)(invar _self_: Set&lt;@T>, init: @V)[invar _item_: @T, _value_: @V => @V] => @V
- [select](#select)(invar _self_: Set&lt;@T>)[invar _item_: @T => bool] => Set&lt;@T>
- [iterate](#iterate)(invar _self_: Set&lt;@T>)[invar _item_: @T]

<a name="std"></a>
### Classes
#### <a name="set">`std::Set`</a>
Tree- or hash-based set type.

__Note:__ For set operations involving two sets and producing a new set, the kind of the resulting set is determined by the left operand
#### Methods
<a name="set_ctor"></a>
```ruby
Set<@T>(kind: enum<tree,hash> = $tree)
```
Constructs new tree- or hash-based set depending on *kind*
<a name="size"></a>
```ruby
.size(invar self: Set<@T>) => int
```
Set size
<a name="insert"></a>
```ruby
insert(self: Set<@T>, invar value: @T)
```
Inserts \a value into the set
<a name="contains"></a>
```ruby
contains(invar self: Set<@T>, invar value: @T) => bool
[](invar self: Set<@T>, invar value: @T) => bool
```
Check if the set contains the given *value*
<a name="contains2"></a>
```ruby
contains(invar self: Set<@T>, invar other: Set<@T>|list<@T>) => bool
```
Check if the set contains *other* set
<a name="erase"></a>
```ruby
erase(self: Set<@T>, invar value: @T)
```
Removes *value* from the set
<a name="clone"></a>
```ruby
clone(invar self: Set<@T>) => Set<@T>
```
Returns set copy
<a name="clear"></a>
```ruby
clear(self: Set<@T>)
```
Removes all items from the set
<a name="op_add_set"></a>
```ruby
+=(self: Set<@T>, invar other: Set<@T>|list<@T>) => Set<int>
|=(self: Set<@T>, invar other: Set<@T>|list<@T>) => Set<int>
```
Insert *other* items in the set and returns self
<a name="op_sub_set"></a>
```ruby
-=(self: Set<@T>, invar other: Set<@T>|list<@T>) => Set<int>
```
Removes *other* items from the set and returns self
<a name="op_add"></a>
```ruby
+(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
+(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
|(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
|(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
```
Union of *a* and *b*
<a name="op_sub"></a>
```ruby
-(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
-(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
```
Difference of *a* and *b*
<a name="op_and"></a>
```ruby
&(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
&(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
```
Intersection of *a* and *b*
<a name="op_xor"></a>
```ruby
^(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
^(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>
```
Symmetric defference of *a* and *b*
<a name="op_mul"></a>
```ruby
*(invar a: Set<@X>, invar b: Set<@Y>|list<@Y>) => Set<tuple<@X,@Y>>
*(c: Set<tuple<@X,@Y>>, invar a: Set<@X>, invar b: Set<@Y>|list<@Y>) => Set<tuple<@X,@Y>>
```
Cartesian product *a* and *b*
<a name="string"></a>
```ruby
(string)(invar self: Set<@T>)
```
String conversion
<a name="items"></a>
```ruby
items(invar self: Set<@T>) => list<@T>
```
Set items as list
<a name="cmp"></a>
```ruby
==(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => bool
!=(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => bool
```
Set comparison
<a name="for"></a>
```ruby
for(invar self: Set<@T>, iterator: ForIterator)
[](invar self: Set<@T>, index: ForIterator) => @T
```
For-in iteration support
<a name="collect"></a>
```ruby
collect(invar self: Set<@T>)[invar item: @T => @V|none] => Set<@V>
```
Iterates over the set, collecting non-`none` values returned from the code section into a new set
<a name="collect2"></a>
```ruby
collect(invar self: Set<@X>, invar other: Set<@Y>)[invar item: @X, invar item2: @Y => @V|none] => Set<@V>
```
Iterates over the two sets in parallel, collecting non-`none` values returned from the code section into a new set
<a name="associate"></a>
```ruby
associate(invar self: Set<@T>, hashing: enum<none,auto,random>|int = $none)[invar item: @T => none|tuple<@K,@V>] => map<@K,@V>
```
Iterates over the set, forming a map out of the key-value tuples returned from the code section
<a name="associate2"></a>
```ruby
associate(invar self: Set<@X>, invar other: Set<@Y>, hashing: enum<none,auto,random>|int = $none)[invar item: @X, invar item2: @Y => none|tuple<@K,@V>] => map<@K,@V>
```
Iterates over the two sets in parallel, forming a map out of the key-value tuples returned from the code section
<a name="find"></a>
```ruby
find(invar self: Set<@T>)[invar item: @T => bool] => @T|none
```
Returns the first item in the set satisfying the condition specified by the code section (`none` if not found)
<a name="reduce"></a>
```ruby
reduce(invar self: Set<@T>)[invar item: @T, value: @T => @T] => @T|none
reduce(invar self: Set<@T>, init: @V)[invar item: @T, value: @V => @V] => @V
```
Iterates over the set, yielding each *item* and accumulated *value* (with the initial value of *init* or the first item in the set); code section result forms the new value to be passed to the next iteration. Returns the accumulated value
<a name="select"></a>
```ruby
select(invar self: Set<@T>)[invar item: @T => bool] => Set<@T>
```
Selects all items satisfying the condition specified by the code section and returns them in a new set
<a name="iterate"></a>
```ruby
iterate(invar self: Set<@T>)[invar item: @T]
```
Iterates over set items
