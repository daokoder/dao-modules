## serializer -- serialization and deserialization of Dao data types

The module implements conversion of Dao data types to internal textual serialization format and vice versa

### Index
namespace [std](#std)

Functions:
- [serialize](#serialize)(invar value: any) => string
- [deserialize](#deserialize)(text: string) => any
- [backup](#backup)(dest = "backup.sdo", limit = 0)
- [restore](#restor)(source = "backup.sdo")

<a name="std"></a>
### Functions
<a name="serialize"></a>
```ruby
serialize(invar value: any) => string
```
Serializes *value* to text. For a class instance to be serializeable, its class should define `serialize()` method returning instance-related data to be serialized
<a name="deserialize"></a>
```ruby
deserialize(text: string ) => any
```
Deserializes value from *text*. For a class instance to be deserializeable, its class should provide constructor compatible with the data returned
by the `serialize()` method; if the latter returns a tuple, the constructor should accept parameters corresponding to the individual fields of that tuple
<a name="backup"></a>
```ruby
backup(dest = "backup.sdo", limit = 0)
```
Saves the current state of the program to the file specified by *dest*. if *limit* is greater then 0, objects whose serialized size exceeds *limit* * 1000 bytes
are not	included in the backup
<a name="restore"></a>
```ruby
restore(source = "backup.sdo")
```
Restores previously saved program state from the file specified by *source*
