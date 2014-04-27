/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011,2012, Limin Fu
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
#include<string.h>
#include"dao_xxtea.h"
#include"daoString.h"
#include"daoValue.h"


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
	DString_ToMBS( self );
	if( self->size == 0 ) return 0;
	DString_ToMBS( key );
	if( key->size >= 32 ){
		for(i=0; i<16; i++){
			signed char c1 = HexDigit( key->mbs[2*i] );
			signed char c2 = HexDigit( key->mbs[2*i+1] );
			if( c1 <0 || c2 <0 ) return 1;
			ks[i] = 16 * c1 + c2;
		}
	}else if( key->size >= 16 ){
		memcpy( ks, key->mbs, 16 );
	}else{
		return 1;
	}
	if( flag == 1 ){
		size = self->size;
		i = size % 4;
		if( i ) i = 4 - i;
		DString_Resize( self, size + 4 + i );
		memmove( self->mbs + 4, self->mbs, size );
		*(int*) self->mbs = size;
		data = (unsigned char*) self->mbs;
		btea( (int*)self->mbs, self->size / 4, (int*) ks );
		if( hex ){
			size = self->size;
			DString_Resize( self, 2 * size );
			data = (unsigned char*) self->mbs;
			for(i=size-1; i>=0; i--){
				self->mbs[2*i+1] = dec2hex[ data[i] % 16 ];
				self->mbs[2*i] = dec2hex[ data[i] / 16 ];
			}
		}
	}else{
		if( hex ){
			if( self->size % 2 ) return 1;
			data = (unsigned char*) self->mbs;
			size = self->size / 2;
			for(i=0; i<size; i++){
				char c1 = HexDigit( data[2*i] );
				char c2 = HexDigit( data[2*i+1] );
				if( c1 <0 || c2 <0 ) return 1;
				data[i] = 16 * c1 + c2;
			}
			DString_Resize( self, size );
		}
		btea( (int*)self->mbs, - (int)(self->size / 4), (int*) ks );
		size = *(int*) self->mbs;
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
	int rc = DString_Encrypt( p[0]->xString.data, p[1]->xString.data, p[2]->xEnum.value );
	if( rc ) DaoProcess_RaiseException( proc, DAO_ERROR, errmsg[rc-1] );
	DaoProcess_PutReference( proc, p[0] );
}
static void DaoSTR_Decrypt( DaoProcess *proc, DaoValue *p[], int N )
{
	int rc = DString_Decrypt( p[0]->xString.data, p[1]->xString.data, p[2]->xEnum.value );
	if( rc ) DaoProcess_RaiseException( proc, DAO_ERROR, errmsg[rc-1] );
	DaoProcess_PutReference( proc, p[0] );
}

const char *enc = "encrypt( source :string, key :string, format :enum<regular,hex> = $regular )=>string";
const char *dec = "decrypt( source :string, key :string, format :enum<regular,hex> = $regular )=>string";

DAO_DLL int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace_WrapFunction( ns, DaoSTR_Encrypt, enc );
	DaoNamespace_WrapFunction( ns, DaoSTR_Decrypt, dec );
	return 0;
}
