/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2014, Limin Fu
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<math.h>
#include"daoString.h"
#include"daoValue.h"
#include "dao_crypto.h"

static void MD5_Append( DString *md5, uint32_t h )
{
	const char *hex = "0123456789abcdef";
	uint32_t k;
	DString_Reserve( md5, md5->size + 8 );
	k = (h>> 0)&0xff;
	md5->chars[md5->size++] = hex[k>>4];
	md5->chars[md5->size++] = hex[k&0xf];
	k = (h>> 8)&0xff;
	md5->chars[md5->size++] = hex[k>>4];
	md5->chars[md5->size++] = hex[k&0xf];
	k = (h>>16)&0xff;
	md5->chars[md5->size++] = hex[k>>4];
	md5->chars[md5->size++] = hex[k&0xf];
	k = (h>>24)&0xff;
	md5->chars[md5->size++] = hex[k>>4];
	md5->chars[md5->size++] = hex[k&0xf];
	md5->chars[md5->size] = '\0';
}
static void MD5_Update( uint32_t H[4], uint32_t W[16], uint32_t K[64] )
{
	static uint32_t R[64] = {
		7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
		5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
		4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
		6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
	};
	uint32_t A = H[0];
	uint32_t B = H[1];
	uint32_t C = H[2];
	uint32_t D = H[3];
	uint32_t k;
	for(k=0; k<16; k++){
		uint32_t f = (B & C) | ((~B) & D);
		uint32_t g = k;
		uint32_t t = D;
		uint32_t x = A + f + K[k] + W[g];
		D = C;
		C = B;
		B = B + ((x << R[k]) | (x >> (32-R[k])));
		A = t;
	}
	for(k=16; k<32; k++){
		uint32_t f = (D & B) | ((~D) & C);
		uint32_t g = (k*5 + 1) % 16;
		uint32_t t = D;
		uint32_t x = A + f + K[k] + W[g];
		D = C;
		C = B;
		B = B + ((x << R[k]) | (x >> (32-R[k])));
		A = t;
	}
	for(k=32; k<48; k++){
		uint32_t f = B ^ C ^ D;
		uint32_t g = (k*3 + 5) % 16;
		uint32_t t = D;
		uint32_t x = A + f + K[k] + W[g];
		D = C;
		C = B;
		B = B + ((x << R[k]) | (x >> (32-R[k])));
		A = t;
	}
	for(k=48; k<64; k++){
		uint32_t f = C ^ (B | (~D));
		uint32_t g = (k*7) % 16;
		uint32_t t = D;
		uint32_t x = A + f + K[k] + W[g];
		D = C;
		C = B;
		B = B + ((x << R[k]) | (x >> (32-R[k])));
		A = t;
	}
	H[0] += A;
	H[1] += B;
	H[2] += C;
	H[3] += D;
}

void DString_MD5( DString *self, DString *md5 )
{
	DString *padding = md5;
	uint64_t i, k, m, n, twop32 = ((uint64_t)1)<<32;
	uint32_t H[4] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
	uint32_t K[64], W[16];
	int32_t size = self->size;
	int32_t chunks = self->size / 64;
	uint8_t *data = (uint8_t*) self->chars;

	for(i=0; i<64; i++) K[i] = (uint32_t) floor( fabs( sin(i+1) ) * twop32 );
	for(i=0; i<chunks; i++){
		for(k=0; k<16; k++){
			uint32_t b = i*64 + k*4;
			uint32_t m = data[b];
			m |= ((uint32_t)data[b+1])<<8;
			m |= ((uint32_t)data[b+2])<<16;
			m |= ((uint32_t)data[b+3])<<24;
			W[k] = m;
		}
		MD5_Update( H, W, K );
	}
	DString_Reserve( padding, 128 );
	padding->size = 64;
	m = size - chunks*64;
	if( m ) memcpy( padding->chars, data + chunks*64, m*sizeof(char) );
	if( m + 8 > 64 ) padding->size = 128;
	chunks = padding->size / 64;

	data = (uint8_t*) padding->chars;
	data[m] = 1<<7; // first bit 1 followed by bit 0s;
	for(i=m+1; i<padding->size-8; i++) data[i] = 0;
	n = size * 8;
	// last 64 bits to store the string size in little endian:
	data[i] = n & 0xff;
	data[i+1] = (n >> 8) & 0xff;
	data[i+2] = (n >> 16) & 0xff;
	data[i+3] = (n >> 24) & 0xff;
	data[i+4] = (n >> 32) & 0xff;
	data[i+5] = (n >> 40) & 0xff;
	data[i+6] = (n >> 48) & 0xff;
	data[i+7] = (n >> 56) & 0xff;
	for(i=0; i<chunks; i++){
		for(k=0; k<16; k++){
			uint32_t b = i*64 + k*4;
			uint32_t m = data[b];
			m |= ((uint32_t)data[b+1])<<8;
			m |= ((uint32_t)data[b+2])<<16;
			m |= ((uint32_t)data[b+3])<<24;
			W[k] = m;
		}
		MD5_Update( H, W, K );
	}
	md5->size = 0;
	MD5_Append( md5, H[0] );
	MD5_Append( md5, H[1] );
	MD5_Append( md5, H[2] );
	MD5_Append( md5, H[3] );
}

static uint_t SHA1_S( uint_t x, int n )
{
	return ( x << n ) | ( x >> ( 32 - n ) );
}

static uint_t SHA1_F( int t, uint_t B, uint_t C, uint_t D )
{
	if ( t < 20 )
		return ( B & C ) | ( ~B & D );
	else if ( t < 40 || t >= 60 )
		return B ^ C ^ D;
	else
		return ( B & C ) | ( B & D ) | ( C & D );
}

void DString_SHA1( DString *self, DString *sha1 )
{
	uint_t K[] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};
	uint_t H[] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
	DString *str = DString_Copy( self );
	unsigned long long bits = ( (unsigned long long)str->size )*8;
	uint_t W[80], TEMP, A, B, C, D, E;
	daoint i;
	int t, pad;
	DString_AppendChar( str, 0x80 );
	pad = 64 - str->size%64;
	if ( pad < 8 )
		pad += 64;
	DString_Resize( str, str->size + pad );
	for ( i = 0; i < 8; i++ )
		str->chars[str->size - i - 1] = ( bits >> 8*i ) & 0xFF;
	for ( i = 0; i < str->size; i += 64 ){
		uchar_t *chs = str->chars + i;
		for ( t = 0; t < 16; t++ )
			W[t] = ( chs[t*4] << 24 ) | ( chs[t*4 + 1] << 16 ) | ( chs[t*4 + 2] << 8 ) | chs[t*4 + 3];
		for ( t = 16; t < 80; t++ )
			W[t] = SHA1_S( W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1 );
		A = H[0];
		B = H[1];
		C = H[2];
		D = H[3];
		E = H[4];
		for ( t = 0; t < 80; t++ ){
			TEMP = SHA1_S( A, 5 ) + SHA1_F( t, B, C, D ) + E + W[t] + K[t/20];
			E = D;
			D = C;
			C = SHA1_S( B, 30 );
			B = A;
			A = TEMP;
		}
		H[0] += A;
		H[1] += B;
		H[2] += C;
		H[3] += D;
		H[4] += E;
	}
	DString_Resize( sha1, 40 );
	snprintf( sha1->chars, sha1->size, "%x%x%x%x%x", H[0], H[1], H[2], H[3], H[4] );
	DString_Delete( str );
}

/* Corrected Block Tiny Encryption Algorithm (Corrected Block TEA, or XXTEA)
 * by David Wheeler and Roger Needham
 */
#define MX  ( (((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(k[(p&3)^e]^z)) )

int btea(int* v, int n, int *k)
{
	unsigned int z, y=v[0], sum=0, e, DELTA=0x9e3779b9;
	int p, q ;
	if (n > 1) { /* Coding Part */
		z=v[n-1];
		q = 6+52/n ;
		while (q-- > 0) {
			sum += DELTA;
			e = (sum>>2) & 3 ;
			for (p=0; p<n-1; p++) y = v[p+1], z = v[p] += MX;
			y = v[0];
			z = v[n-1] += MX;
		}
		return 0 ;
	} else if (n < -1) {  /* Decoding Part */
		n = -n ;
		q = 6+52/n ;
		sum = q*DELTA ;
		while (sum != 0) {
			e = (sum>>2) & 3;
			for (p=n-1; p>0; p--) z = v[p-1], y = v[p] -= MX;
			z = v[n-1];
			y = v[0] -= MX;
			sum -= DELTA;
		}
		return 0;
	}
	return 1;
}

const char *dec2hex = "0123456789abcdef";

static int HexDigit( char d )
{
	d = d | 0x20;
	if( ( (size_t)(d-'0') ) < 10 ) return d - '0';
	else if( ( (size_t)(d-'a') ) < 26 ) return d + 10 - 'a';
	return -1;
}
static int STR_Cipher( DString *self, DString *key, int hex, int flag )
{
	unsigned char ks[16];
	unsigned char *data = NULL;
	size_t size = 0;
	int i;
	DString_Detach( self, self->size );
	if( self->size == 0 ) return 0;
	if( key->size >= 32 ){
		for(i=0; i<16; i++){
			signed char c1 = HexDigit( key->chars[2*i] );
			signed char c2 = HexDigit( key->chars[2*i+1] );
			if( c1 <0 || c2 <0 ) return 1;
			ks[i] = 16 * c1 + c2;
		}
	}else if( key->size >= 16 ){
		memcpy( ks, key->chars, 16 );
	}else{
		return 1;
	}
	if( flag == 1 ){
		size = self->size;
		i = size % 4;
		if( i ) i = 4 - i;
		DString_Resize( self, size + 4 + i );
		memmove( self->chars + 4, self->chars, size );
		*(int*) self->chars = size;
		data = (unsigned char*) self->chars;
		btea( (int*)self->chars, self->size / 4, (int*) ks );
		if( hex ){
			size = self->size;
			DString_Resize( self, 2 * size );
			data = (unsigned char*) self->chars;
			for(i=size-1; i>=0; i--){
				self->chars[2*i+1] = dec2hex[ data[i] % 16 ];
				self->chars[2*i] = dec2hex[ data[i] / 16 ];
			}
		}
	}else{
		if( hex ){
			if( self->size % 2 ) return 1;
			data = (unsigned char*) self->chars;
			size = self->size / 2;
			for(i=0; i<size; i++){
				char c1 = HexDigit( data[2*i] );
				char c2 = HexDigit( data[2*i+1] );
				if( c1 <0 || c2 <0 ) return 1;
				data[i] = 16 * c1 + c2;
			}
			DString_Resize( self, size );
		}
		btea( (int*)self->chars, - (int)(self->size / 4), (int*) ks );
		size = *(int*) self->chars;
		if( size > self->size ) return 2;
		DString_Erase( self, 0, 4 );
		self->size = size;
	}
	return 0;
}
int DString_Encrypt( DString *self, DString *key, int hex )
{
	return STR_Cipher( self, key, hex, 1 );
}
int DString_Decrypt( DString *self, DString *key, int hex )
{
	return STR_Cipher( self, key, hex, 0 );
}

static const char *errmsg[2] =
{
	"invalid key",
	"invalid source"
};
static void DaoSTR_Encrypt( DaoProcess *proc, DaoValue *p[], int N )
{
	int rc = DString_Encrypt( p[0]->xString.value, p[1]->xString.value, p[2]->xEnum.value );
	if( rc ) DaoProcess_RaiseError( proc, NULL, errmsg[rc-1] );
	DaoProcess_PutValue( proc, p[0] );
}
static void DaoSTR_Decrypt( DaoProcess *proc, DaoValue *p[], int N )
{
	int rc = DString_Decrypt( p[0]->xString.value, p[1]->xString.value, p[2]->xEnum.value );
	if( rc ) DaoProcess_RaiseError( proc, NULL, errmsg[rc-1] );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoCrypto_Hash( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *str = p[0]->xString.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	if ( p[1]->xEnum.value == 0 )
		DString_MD5( str, res );
	else
		DString_SHA1( str, res );
}

static DaoFuncItem cryptoMeths[] =
{
	/*! Returns hash string of \a str using the specified \a method */
	{ DaoCrypto_Hash,	"hash(str: string, method: enum<md5,sha1>) => string" },

	/*! Encripts \a source with XXTEA algorithm using the given \a key. Returns the resulting data in the specified \a format */
	{ DaoSTR_Encrypt,	"encrypt( source :string, key :string, format :enum<regular,hex> = $regular )=>string" },

	/*! Decripts \a source with XXTEA algorithm using the given \a key. Returns the resulting data in the specified \a format */
	{ DaoSTR_Decrypt,	"decrypt( source :string, key :string, format :enum<regular,hex> = $regular )=>string" },

	{ NULL, NULL }
};

DAO_DLL int DaoCrypto_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *crns = DaoNamespace_GetNamespace( ns, "crypto" );
	DaoNamespace_WrapFunctions( crns, cryptoMeths );
	return 0;
}
