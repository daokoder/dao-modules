## crypto -- common cryptographic tasks

The module provides functions to compute cryptographic hashes, encrypt and decrypt data, produce cryptographically secure pseudo-random number sequences

### Index
namespace [crypto](#crypto)

Functions:
- [hash](#hash)(_str_: string, _method_: enum<md5,sha1>) => string
- [encrypt](#encrypt)(_source_: string, _key_: string, _format_: enum<regular,hex> = $regular) => string
- [decrypt](#decrypt)(_source_: string, _key_: string, _format_: enum<regular,hex> = $regular) => string
- [random](#random)(_count_: int) => string

<a name="crypto"></a>
### Functions
<a name="hash"></a>
```ruby
hash(str: string, method: enum<md5,sha1>) => string
```
Returns hash string of *str* using the specified *method*
<a name="encrypt"></a>
```ruby
encrypt(source: string, key: string, format: enum<regular,hex> = $regular) => string
```
Encrypts *source* with XXTEA algorithm using the given *key*. Returns the resulting data in the specified *format*
<a name="decrypt"></a>
```ruby
decrypt(source: string, key: string, format: enum<regular,hex> = $regular) => string
```
Decrypts *source* with XXTEA algorithm using the given *key*. Returns the resulting data in the specified *format*
<a name="random"></a>
```ruby
random(count: int) => string
```
Returns binary string of *count* bytes read from system random number generator (uses `/dev/urandom` on Unix, `CryptGenRandom()` on Windows)
