## web.mime -- basic MIME type identification

Extension-based MIME type identification

### Index
namespace [mime](#mime)

Functions:
- [identify](#identify)(_target_: string) => list&lt;string>
- [updateDb](#updatedb)(_source_: io::Stream) => int

<a name="mime"></a>
### Functions
<a name="identify"></a>
```ruby
identify(target: string) => list<string>
```
Returns list of MIME type defined for the given *target* which may be a file name or an extension. If no appropriate MIME type was found, empty list is returned

__Note:__ The identification involves extension matching only, file contents are not read. *target* is assumed to name a directory if it ends with '/', in which case its MIME type is "inode/directory". If *target* ends with '~', it is considered to be a backup file with "application/x-trash" type
<a name="updatedb"></a>
```ruby
identify(target: string) => list<string>
```
Updates the MIME types database from the specified *source*, which should have format compatible with '/etc/mime.types' found on typical Unix systems. Returns the number of inserted/updated entries
