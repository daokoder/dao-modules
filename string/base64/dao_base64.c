/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2014, Limin Fu
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

// 2014-05: Danilov Aleksey, initial implementation

#include<string.h>
#include"dao_base64.h"

const char base64_chars[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void EncodeBase64( DString *src, DString *dest )
{
	daoint i, j = 0;
	DString_Resize( dest, ( src->size/3 )*4 + ( src->size%3? 4 : 0 ) );
	for ( i = 0; i < src->size/3; i++ ){
		uchar_t high = src->chars[i*3], mid = src->chars[i*3 + 1], low = src->chars[i*3 + 2];
		uint_t triplet = ( (uint_t)high << 16 ) | ( (uint_t)mid << 8 ) | low;
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 18 ) ) >> 18];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 12 ) ) >> 12];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 6 ) ) >> 6];
		dest->chars[j++] = base64_chars[triplet & 63];
	}
	if ( src->size%3 == 1 ){
		uint_t triplet = ( (uint_t)src->chars[src->size - 1] ) << 16;
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 18 ) ) >> 18];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 12 ) ) >> 12];
		dest->chars[j++] = '=';
		dest->chars[j] = '=';
	}
	else if ( src->size%3 == 2 ){
		uint_t triplet = ( ( (uint_t)src->chars[src->size - 2] ) << 16 ) | ( ( (uint_t)src->chars[src->size - 1] ) << 8 );
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 18 ) ) >> 18];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 12 ) ) >> 12];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 6 ) ) >> 6];
		dest->chars[j] = '=';
	}
}

const uchar_t base64_nums[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 0, 0, 0, 64,
	53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 0, 0, 0, 0,
	0, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
	42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int DecodeBase64( DString *src, DString *dest, daoint *errpos )
{
	daoint i, padding = 0, j = 0;
	daoint size = ( src->size/4 )*3;
	if ( src->size%4 ){
		*errpos = -1;
		return 0;
	}
	if ( src->size ){
		if ( src->chars[src->size - 1] == '=' )
			padding++;
		if ( src->size > 1 && src->chars[src->size - 2] == '=' )
			padding++;
	}
	DString_Resize( dest, size - padding );
	for ( i = 0; i < src->size/4 - ( padding? 1 : 0 ); i++ ){
		uchar_t highest = base64_nums[src->chars[i*4]], higher = base64_nums[src->chars[i*4 + 1]];
		uchar_t lower = base64_nums[src->chars[i*4 + 2]], lowest = base64_nums[src->chars[i*4 + 3]];
		uint_t triplet;
		if ( !highest-- ){
			*errpos = i*4;
			return 0;
		}
		if ( !higher-- ){
			*errpos = i*4 + 1;
			return 0;
		}
		if ( !lower-- ){
			*errpos = i*4 + 2;
			return 0;
		}
		if ( !lowest-- ){
			*errpos = i*4 + 3;
			return 0;
		}
		triplet = ( highest << 18 ) | ( higher << 12 ) | ( lower << 6 ) | lowest;
		dest->chars[j++] = ( triplet & ( 0xFF << 16 ) ) >> 16;
		dest->chars[j++] = ( triplet & ( 0xFF << 8 ) ) >> 8;
		dest->chars[j++] = triplet & 0xFF;
	}
	if ( padding ){
		uchar_t highest = base64_nums[src->chars[src->size - 4]], higher = base64_nums[src->chars[src->size - 3]];
		uchar_t lower = 0;
		uint_t triplet;
		if ( !highest-- ){
			*errpos = src->size - 4;
			return 0;
		}
		if ( !higher-- ){
			*errpos = src->size - 3;
			return 0;
		}
		if ( padding == 1 ){
			lower = base64_nums[src->chars[src->size - 2]];
			if ( !lower-- ){
				*errpos = src->size - 2;
				return 0;
			}
		}
		triplet = ( highest << 18 ) | ( higher << 12 ) | ( lower << 6 );
		dest->chars[j++] = ( triplet & ( 0xFF << 16 ) ) >> 16;
		if ( padding == 1 )
			dest->chars[j++] = ( triplet & ( 0xFF << 8 ) ) >> 8;
	}
	return 1;
}

static void DaoXML_ToBase64( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *src = p[0]->xString.value;
	DString *dest = DaoProcess_PutChars( proc, "" );
	EncodeBase64( src, dest );
}

static void DaoXML_FromBase64( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *src = p[0]->xString.value;
	DString *dest = DString_New();
	daoint errpos;
	int res = DecodeBase64( src, dest, &errpos );
	if ( res )
		DaoProcess_PutString( proc, dest );
	else if ( errpos < 0 )
		DaoProcess_RaiseError( proc, "Base64", "Not a valid Base64-encoded string" );
	else {
		char buf[50];
		snprintf( buf, sizeof(buf), "Non-Base64 character at index %" DAO_INT_FORMAT, errpos );
		DaoProcess_RaiseError( proc, "Base64", buf );
	}
	DString_Delete( dest );
}

static DaoFuncItem b64Meths[] = {
	/*! Returns \a str encoded in Base64 */
	{ DaoXML_ToBase64,				"encode(str: string) => string" },

	/*! Returns \a str decoded from Base64 */
	{ DaoXML_FromBase64,			"decode(str: string) => string" },
	{ NULL, NULL }
};

DAO_DLL int DaoBase64_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *b64ns;
	b64ns = DaoVmSpace_GetNamespace( vmSpace, "base64" );
	DaoNamespace_AddConstValue( ns, "base64", (DaoValue*)b64ns );
	DaoNamespace_WrapFunctions( b64ns, b64Meths );
	return 0;
}
