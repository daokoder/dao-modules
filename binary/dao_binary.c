/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2015, Limin Fu
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

// 2011-01: Danilov Aleksey, initial implementation.

#include<string.h>
#include<stdint.h>

#include"daoValue.h"
#include"daoStdtype.h"
#include"daoNumtype.h"
#include"daoStream.h"

#include"dao_binary.h"

void DString_EncodeBase64( DString *self, DString *dest )
{
	const char base64_chars[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	daoint i, j = 0;
	DString_Resize( dest, ( self->size/3 )*4 + ( self->size%3? 4 : 0 ) );
	for ( i = 0; i < self->size/3; i++ ){
		uint8_t high = self->chars[i*3], mid = self->chars[i*3 + 1], low = self->chars[i*3 + 2];
		uint_t triplet = ( (uint_t)high << 16 ) | ( (uint_t)mid << 8 ) | low;
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 18 ) ) >> 18];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 12 ) ) >> 12];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 6 ) ) >> 6];
		dest->chars[j++] = base64_chars[triplet & 63];
	}
	if ( self->size%3 == 1 ){
		uint_t triplet = ( (uint_t)self->chars[self->size - 1] ) << 16;
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 18 ) ) >> 18];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 12 ) ) >> 12];
		dest->chars[j++] = '=';
		dest->chars[j] = '=';
	}
	else if ( self->size%3 == 2 ){
		uint_t triplet = ( ( (uint_t)self->chars[self->size - 2] ) << 16 ) | ( ( (uint_t)self->chars[self->size - 1] ) << 8 );
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 18 ) ) >> 18];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 12 ) ) >> 12];
		dest->chars[j++] = base64_chars[( triplet & ( 63 << 6 ) ) >> 6];
		dest->chars[j] = '=';
	}
}

int DString_DecodeBase64( DString *self, DString *dest, daoint *errpos )
{
	const uint8_t base64_nums[] = {
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
	daoint i, padding = 0, j = 0;
	daoint size = ( self->size/4 )*3;
	if ( self->size%4 ){
		*errpos = -1;
		return 0;
	}
	if ( self->size ){
		if ( self->chars[self->size - 1] == '=' )
			padding++;
		if ( self->size > 1 && self->chars[self->size - 2] == '=' )
			padding++;
	}
	DString_Resize( dest, size - padding );
	for ( i = 0; i < self->size/4 - ( padding? 1 : 0 ); i++ ){
		uint8_t highest = base64_nums[self->chars[i*4]], higher = base64_nums[self->chars[i*4 + 1]];
		uint8_t lower = base64_nums[self->chars[i*4 + 2]], lowest = base64_nums[self->chars[i*4 + 3]];
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
		uint8_t highest = base64_nums[self->chars[self->size - 4]], higher = base64_nums[self->chars[self->size - 3]];
		uint8_t lower = 0;
		uint_t triplet;
		if ( !highest-- ){
			*errpos = self->size - 4;
			return 0;
		}
		if ( !higher-- ){
			*errpos = self->size - 3;
			return 0;
		}
		if ( padding == 1 ){
			lower = base64_nums[self->chars[self->size - 2]];
			if ( !lower-- ){
				*errpos = self->size - 2;
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

int DString_EncodeZ85( DString *self, DString *dest )
{
	const char *z85_chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#";
	daoint i, j;
	uint8_t *chs = self->chars;
	if ( self->size%4 )
		return 0;
	DString_Resize( dest, self->size/4*5 );
	for ( i = 0, j = 0; i < self->size; i += 4, j += 5 ){
		uint_t val = ( (uint_t)chs[i] << 24 ) | ( (uint_t)chs[i + 1] << 16 ) | ( (uint_t)chs[i + 2] << 8 ) | (uint_t)chs[i + 3];
		int ind;
		for ( ind = 4; ind >= 0; ind-- ){
			dest->chars[j + ind] = z85_chars[val%85];
			val /= 85;
		}
	}
	return 1;
}

int DString_DecodeZ85( DString *self, DString *dest, daoint *errpos )
{
	const uint8_t z85_nums[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 69, 0, 85, 84, 83, 73, 0, 76, 77, 71, 66, 0, 64, 63, 70,
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 65, 0, 74, 67, 75, 72,
		82, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 78, 0, 79, 68, 0,
		0, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
		26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 80, 0, 81, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};
	daoint i, j;
	uint8_t *chs;
	*errpos = -1;
	if ( self->size%5 )
		return 0;
	DString_Resize( dest, self->size/5*4 );
	chs = dest->chars;
	for ( i = 0, j = 0; i < self->size; i += 5, j += 4 ){
		uint_t val = 0;
		int ind, k = 1;
		for ( ind = 4; ind >= 0; ind--, k *= 85 ){
			uint_t num = z85_nums[self->chars[i + ind]];
			if ( !num-- ){
				*errpos = i + ind;
				return 0;
			}
			val += num*k;
		}
		chs[j] = ( val & 0xFF000000 ) >> 24;
		chs[j + 1] = ( val & 0x00FF0000 ) >> 16;
		chs[j + 2] = ( val & 0x0000FF00 ) >> 8;
		chs[j + 3] = val & 0x000000FF;
	}
	return 1;
}

void DString_EncodeHex( DString *self, DString *dest )
{
	const char *hex_chars = "0123456789ABCDEF";
	daoint i;
	DString_Resize( dest, self->size*2 );
	for ( i = 0; i < self->size; i++ ){
		uint8_t ch = self->chars[i];
		dest->chars[2*i] = hex_chars[ch >> 4];
		dest->chars[2*i + 1] = hex_chars[ch & 0x0F];
	}
}

int DString_DecodeHex( DString *self, DString *dest, daoint *errpos )
{
	const uint8_t hex_nums[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 0, 0, 0, 0, 0,
		0, 11, 12, 13, 14, 15, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 11, 12, 13, 14, 15, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	daoint i;
	*errpos = -1;
	if ( self->size%2 )
		return 0;
	DString_Resize( dest, self->size/2 );
	for ( i = 0; i < dest->size; i++ ){
		uint8_t high = hex_nums[self->chars[2*i]], low = hex_nums[self->chars[2*i + 1]];
		if ( !high-- ){
			*errpos = 2*i;
			return 0;
		}
		if ( !low-- ){
			*errpos = 2*i + 1;
			return 0;
		}
		dest->chars[i] = ( high << 4 ) | low;
	}
	return 1;
}

static void DaoBinary_Encode( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *src = p[0]->xString.value;
	DString *dest = DaoProcess_PutChars( proc, "" );
	if ( p[1]->xEnum.value == 0 )
		DString_EncodeBase64( src, dest );
	else if ( p[1]->xEnum.value == 1 ) {
		if ( src->size%4 ){
			DaoProcess_RaiseError( proc, "Bin", "To be encoded with Z85, binary data size must be a multiple of 4" );
			return;
		}
		DString_EncodeZ85( src, dest );
	}
	else
		DString_EncodeHex( src, dest );
}

static void DaoBinary_Decode( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *src = p[0]->xString.value;
	DString *dest = DaoProcess_PutChars( proc, "" );
	daoint errpos;
	if ( p[1]->xEnum.value == 0 ){
		if ( !DString_DecodeBase64( src, dest, &errpos ) ){
			if ( errpos < 0 )
				DaoProcess_RaiseError( proc, "Bin", "Not a valid Base64-encoded string" );
			else {
				char buf[50];
				snprintf( buf, sizeof(buf), "Non-Base64 character at index %" DAO_INT, errpos );
				DaoProcess_RaiseError( proc, "Bin", buf );
			}
		}
	}
	else if ( p[1]->xEnum.value == 1 ) {
		if ( !DString_DecodeZ85( src, dest, &errpos ) ){
			if ( errpos < 0 )
				DaoProcess_RaiseError( proc, "Bin", "Not a valid Z85-encoded string" );
			else {
				char buf[50];
				snprintf( buf, sizeof(buf), "Non-Z85 character at index %" DAO_INT, errpos );
				DaoProcess_RaiseError( proc, "Bin", buf );
			}
		}
	}
	else if ( !DString_DecodeHex( src, dest, &errpos ) ){
		if ( errpos < 0 )
			DaoProcess_RaiseError( proc, "Bin", "Not a valid hex-encoded string" );
		else {
			char buf[50];
			snprintf( buf, sizeof(buf), "Non-hex character at index %" DAO_INT, errpos );
			DaoProcess_RaiseError( proc, "Bin", buf );
		}
	}
}

static void DaoBinary_Read( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[0]->xStream;
	DaoArray *arr = &p[1]->xArray;
	dao_integer count =  p[2]->xInteger.value;
	size_t size = 0;
	DaoArray_Sliced( arr );
	if( !DaoStream_IsReadable( stream ) ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not readable" );
		return;
	}
	switch( arr->etype ){
	case DAO_INTEGER:	size = sizeof(dao_integer); break;
	case DAO_FLOAT:		size = sizeof(dao_float); break;
	case DAO_COMPLEX:	size = sizeof(dao_complex); break;
	}
	if( count == 0 || count > arr->size )
		count = arr->size;
	DaoProcess_PutInteger( proc, DaoStream_ReadBytes( stream, arr->data.p, size*count ) );
}

static void DaoBinary_Unpack( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[0]->xStream;
	DaoArray *arr = &p[1]->xArray;
	size_t sizes[] = {1, 2, 4};
	size_t size =  sizes[p[2]->xEnum.value];
	dao_integer count = p[3]->xInteger.value;
	DaoArray_Sliced( arr );
	if( !DaoStream_IsReadable( stream ) ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not readable" );
		return;
	}
	if( count <= 0 || count > arr->size*sizeof(dao_integer)/size )
		count = arr->size*sizeof(dao_integer)/size;
	count = DaoStream_ReadBytes( stream, arr->data.p, size*count );
	if( size != sizeof(dao_integer) ){
		size_t i;
		char *bytes = (char*)arr->data.p;
		for( i = 0; i < count; i++ ){
			size_t j = count - i - 1;
			arr->data.i[j] = bytes[j];
		}
	}
	DaoProcess_PutInteger( proc, count );
}

static void DaoBinary_Pack( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[1]->xStream;
	DaoArray *arr = &p[0]->xArray;
	size_t sizes[] = {1, 2, 4};
	size_t size =  sizes[p[2]->xEnum.value];
	dao_integer count = p[3]->xInteger.value, res = 0;
	size_t i;
	DaoArray_Sliced( arr );
	if( !DaoStream_IsWritable( stream ) ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not writable" );
		return;
	}
	if( count <= 0 || count > arr->size )
		count = arr->size;
	if( size != sizeof(dao_integer) ){
		void *data = dao_malloc( size*count );
		switch( size ){
		case 1:
			if( 1 ){
				int8_t *bytes = (int8_t*)data;
				for( i = 0; i < count; i++ )
					bytes[i] = arr->data.i[i];
			}
			break;
		case 2:
			if( 1 ){
				int16_t *words = (int16_t*)data;
				for( i = 0; i < count; i++ )
					words[i] = arr->data.i[i];
			}
			break;
		case 4:
			if( 1 ){
				int32_t *dwords = (int32_t*)data;
				for( i = 0; i < count; i++ )
					dwords[i] = arr->data.i[i];
			}
			break;
		}
		res = DaoStream_WriteBytes( stream, data, size*count );
		dao_free( data );
	}
	else
		res = DaoStream_WriteBytes( stream, arr->data.i, size*count );
	DaoProcess_PutInteger( proc, res );
}

static void DaoBinary_Write( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[1]->xStream;
	DaoArray *arr = &p[0]->xArray;
	size_t size = 0;
	dao_integer count = p[2]->xInteger.value;
	DaoArray_Sliced( arr );
	if( !DaoStream_IsWritable( stream ) ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not writable" );
		return;
	}
	switch( arr->etype ){
	case DAO_INTEGER:	size = sizeof(dao_integer); break;
	case DAO_FLOAT:		size = sizeof(dao_float); break;
	case DAO_COMPLEX:	size = sizeof(dao_complex); break;
	}
	if( count <= 0 || count > arr->size )
		count = arr->size;
	DaoProcess_PutInteger( proc, DaoStream_WriteBytes( stream, arr->data.p, size*count ) );
}

enum {
	Binary_Byte = 0,
	Binary_UByte,
	Binary_Word,
	Binary_UWord,
	Binary_DWord,
	Binary_UDWord,
	Binary_QWord,
	Binary_UQWord
};

typedef int bin_data;

static void DaoBinary_GetItem( DaoProcess *proc, DaoValue *p[], int N )
{
	bin_data type = p[1]->xEnum.value;
	dao_integer offset = p[2]->xInteger.value;
	char *tpname = DaoProcess_GetReturnType( proc )->name->chars;
	daoint size;
	char *data;
	if ( p[0]->type == DAO_ARRAY ){
		DaoArray *arr = &p[0]->xArray;
		DaoArray_Sliced( arr );
		data = (char*)arr->data.p + offset;
		size = arr->size*sizeof(dao_integer);
	}
	else {
		data = p[0]->xString.value->chars;
		size = p[0]->xString.value->size;
	}
	if ( offset < 0 )
		DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
	else if ( strcmp( tpname, "float" ) == 0 ){
		if ( offset + sizeof(dao_float) > size ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		DaoProcess_PutFloat( proc, *(dao_float*)data  );
	}
	else if ( N == 4 ){ // bit range
		dao_integer count = p[3]->xInteger.value;
		uint_t val = 0;
		int i;
		if ( count < 0 ){
			DaoProcess_RaiseError( proc, "Param", "Invalid count" );
			return;
		}
		if ( offset + count > size*CHAR_BIT ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		if ( count > 4*CHAR_BIT ){
			DaoProcess_RaiseError( proc, "Param", "Bit range too large" );
			return;
		}
		for ( i = 0; i < count; i++ ){
			int index = offset%CHAR_BIT + i;
			uint8_t *ptr = data + offset/CHAR_BIT + index/CHAR_BIT;
			val |= (uint8_t)( *ptr << ( index%CHAR_BIT ) ) >> ( CHAR_BIT - 1 ) << ( count - i - 1 );
		}
		DaoProcess_PutInteger( proc, val );
	}
	else {
		size_t sizes[] = {1, 1, 2, 2, 4, 4, 8, 8};
		dao_integer num;
		if ( offset + sizes[type] > size ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		switch( type ){
		case Binary_UByte:	num = *(uint8_t*)data; break;
		case Binary_Byte:	num = *(int8_t*)data; break;
		case Binary_UWord:  num = *(uint16_t*)data; break;
		case Binary_Word:	num = *(int16_t*)data; break;
		case Binary_UDWord: num = *(uint32_t*)data; break;
		case Binary_DWord:	num = *(int32_t*)data; break;
		case Binary_UQWord: num = *(dao_integer*)data; break;
		case Binary_QWord:	num = *(dao_integer*)data; break;
		}
		DaoProcess_PutInteger( proc, num );
	}
}

static void DaoBinary_SetItem( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	bin_data type = p[1]->xEnum.value;
	dao_integer offset = p[2]->xInteger.value;
	char *data;
	DaoArray_Sliced( arr );
	data = (char*)arr->data.p + ( N == 5? 0 : offset );
	if ( offset < 0 )
		DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
	else if ( p[3]->type == DAO_FLOAT ){
		if ( offset + sizeof(dao_float) > arr->size*sizeof(dao_integer) ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		*(dao_float*)data  = p[3]->xFloat.value;
	}
	else if ( N == 5 ){ // bit range
		dao_integer count = p[3]->xInteger.value;
		uint_t value = p[4]->xInteger.value;
		int i;
		if ( count < 0 ){
			DaoProcess_RaiseError( proc, "Param", "Invalid count" );
			return;
		}
		if ( offset + count > arr->size*sizeof(dao_integer)*CHAR_BIT ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		if ( count > 4*CHAR_BIT ){
			DaoProcess_RaiseError( proc, "Param", "Bit range too large" );
			return;
		}
		for ( i = 0; i < count; i++ ){
			int index = offset%CHAR_BIT + i;
			uint8_t *ptr = data + offset/CHAR_BIT + index/CHAR_BIT;
			int pos = CHAR_BIT - index%CHAR_BIT - 1;
			uint8_t byteval = (uint_t)( value << ( 4*CHAR_BIT - count + i ) ) >> ( 4*CHAR_BIT - 1 ) << pos;
			*ptr = ( *ptr & ~( (uint8_t)1 << pos ) ) | byteval;
		}
	}
	else {
		size_t sizes[] = {1, 1, 2, 2, 4, 4, 8, 8};
		size_t size = sizes[type];
		dao_integer num = p[3]->xInteger.value;
		if ( offset + size > arr->size*sizeof(dao_integer) ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		switch( type ){
		case Binary_UByte:	*(uint8_t*)data = num; break;
		case Binary_Byte:	*(int8_t*)data = num; break;
		case Binary_UWord:  *(uint16_t*)data = num; break;
		case Binary_Word:	*(int16_t*)data = num; break;
		case Binary_UDWord: *(uint32_t*)data = num; break;
		case Binary_DWord:	*(int32_t*)data = num; break;
		case Binary_UQWord: *(dao_integer*)data = num; break;
		case Binary_QWord:	*(dao_integer*)data = num; break;
		}
	}
}

static DaoFuncItem binMeths[] =
{
	/*! Reads \a count elements from \a source to \a dest. If \a count is zero, or greater than \a dest size,
	 * \a dest size is assumed. Returns the number of elements actually read */
	{ DaoBinary_Read,		"read(source: io::Stream, dest: array<@T<int|float|complex>>, count = 0) => int" },

	/*! Reads \a count chunks of size \a size from \a source into \a dest so that each chunk corresponds to a single
	 * \a dest element (with possible widening). If \a count is zero, or greater than \a dest element size, \a dest
	 * element size is assumed. Returns the number of chunks actually read */
	{ DaoBinary_Unpack,		"unpack(source: io::Stream, dest: array<int>, size: enum<byte,word,dword>, count = 0) => int" },

	/*! Writes \a count chunks of size \a size to \a dest so that each \a source element corresponds to a single chunk
	 * (with possible narrowing). Returns the number of chunks actually written */
	{ DaoBinary_Pack,		"pack(invar source: array<int>, dest: io::Stream, size: enum<byte,word,dword>, count = 0) => int" },

	/*! Writes \a count elements from \a source to \a dest. If \a count is zero, or greater than \a dest size, all \a dest data is
	 * written. Returns the number of elements actually written */
	{ DaoBinary_Write,		"write(invar source: array<@T<int|float|complex>>, dest: io::Stream, count = 0) => int" },

	/*! Reads value described by \a what from \a source at the given byte \a offset */
	{ DaoBinary_GetItem,	"get(invar source: array<int>|string, what: enum<i8,u8,i16,u16,i32,u32,i64,u64>,"
								"offset: int) => int" },
	{ DaoBinary_GetItem,	"get(invar source: array<int>|string, what: enum<float>, offset: int) => float" },

	/*! Reads \a count bits from \a source at the given bit \a offset */
	{ DaoBinary_GetItem,	"get(invar source: array<int>|string, what: enum<bits>, offset: int, count: int) => int" },

	/*! Writes \a value described by \a what to \a dest at the given byte \a offset */
	{ DaoBinary_SetItem,	"set(dest: array<int>, what: enum<i8,u8,i16,u16,i32,u32,i64,u64>,"
								"offset: int, value: int)" },
	{ DaoBinary_SetItem,	"set(dest: array<int>, what: enum<float>, offset: int, value: float)" },

	/*! Writes \a count lower bits of \a value to \a dest at the given \a offset */
	{ DaoBinary_SetItem,	"set(dest: array<int>, what: enum<bits>, offset: int, count: int, value: int)" },

	/*! Returns \a str encoded with the given \a codec.
	 *
	 * \note For Z85 codec, \a str size must be a multiple of 4 */
	{ DaoBinary_Encode,		"encode(str: string, codec: enum<base64,z85,hex>) => string" },

	/*! Returns \a str decoded with the given \a codec */
	{ DaoBinary_Decode,		"decode(str: string, codec: enum<base64,z85,hex>) => string" },
	{ NULL, NULL }
};

static DaoType *daox_type_encoder;
static DaoType *daox_type_decoder;

void DaoXCoder_Delete( DaoXCoder *self )
{
	DaoGC_DecRC( (DaoValue*)self->stream );
	dao_free( self );
}

static void DaoEncoder_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *res = (DaoXCoder*)dao_malloc( sizeof(DaoXCoder) );
	res->counter = 0;
	if ( N == 0 ){
		res->stream = DaoStream_New();
		DaoStream_SetStringMode( res->stream );
		DaoGC_IncRC( (DaoValue*) res->stream );
	}
	else {
		DaoStream *stream = &p[0]->xStream;
		if ( DaoStream_IsWritable( stream ) ){
			DaoGC_IncRC( p[0] );
			res->stream = &p[0]->xStream;
		}
		else {
			DaoProcess_RaiseError( proc, "Param", "Stream not writable" );
			dao_free( res );
			return;
		}
	}
	DaoProcess_PutCdata( proc, res, daox_type_encoder );
}

static void DaoXCoder_Stream( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutValue( proc, (DaoValue*)self->stream );
}

static void DaoXCoder_Counter( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->counter );
}

int CheckStream( DaoProcess *proc, DaoStream *stream )
{
	if ( !DaoStream_IsOpen( stream ) ){
		DaoProcess_RaiseError( proc, "Value", "Stream was closed" );
		return 0;
	}
	return 1;
}

void DaoEncoder_WriteInteger( DaoXCoder *self, uint64_t value, int size )
{
	DaoStream *stream = self->stream;
	char buf[9] = {0};
	int i;
	for ( i = size; i > 0; i-- ){
		buf[i - 1] = value & 0xFF;
		value >>= CHAR_BIT;
	}
	self->counter += DaoStream_WriteBytes( stream, buf, size );
}

static void DaoEncoder_WriteI8( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	DaoStream_WriteChar( self->stream, (int8_t)p[1]->xInteger.value );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoEncoder_WriteU8( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	DaoStream_WriteChar( self->stream, (uint8_t)p[1]->xInteger.value );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoEncoder_WriteI16( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	DaoEncoder_WriteInteger( self, (int16_t)p[1]->xInteger.value, 2 );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoEncoder_WriteU16( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	DaoEncoder_WriteInteger( self, (uint16_t)p[1]->xInteger.value, 2 );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoEncoder_WriteI32( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	DaoEncoder_WriteInteger( self, (int32_t)p[1]->xInteger.value, 4 );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoEncoder_WriteU32( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	DaoEncoder_WriteInteger( self, (uint32_t)p[1]->xInteger.value, 4 );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoEncoder_WriteI64( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	DaoEncoder_WriteInteger( self, p[1]->xInteger.value, 8 );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoEncoder_WriteBytes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	DString *str = p[1]->xString.value;
	if ( !CheckStream( proc, self->stream ) )
		return;
	DaoStream_WriteString( self->stream, str );
	self->counter += str->size;
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoEncoder_WriteF32( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	char buf[sizeof(float)];
	DString str = DString_WrapBytes( buf, sizeof(buf) );
	*(float*)buf = p[1]->xFloat.value;
	if ( !CheckStream( proc, self->stream ) )
		return;
	if ( sizeof(float) != 4 )
		DaoProcess_RaiseWarning( proc, NULL, "The size of C float type is not 4 on this platform" );
	DaoStream_WriteString( self->stream, &str );
	self->counter += sizeof(float);
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoEncoder_WriteF64( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	char buf[sizeof(dao_float)];
	DString str = DString_WrapBytes( buf, sizeof(buf) );
	*(dao_float*)buf = p[1]->xFloat.value;
	if ( !CheckStream( proc, self->stream ) )
		return;
	if ( sizeof(dao_float) != 8 )
		DaoProcess_RaiseWarning( proc, NULL, "The size of C double type is not 8 on this platform" );
	DaoStream_WriteString( self->stream, &str );
	self->counter += sizeof(dao_float);
	DaoProcess_PutValue( proc, p[0] );
}

static DaoFuncItem encoderMeths[] =
{
	//! Creates an encoder writing to \a sink (if omitted, new string stream is created)
	{ DaoEncoder_Create,	"Encoder()" },
	{ DaoEncoder_Create,	"Encoder(sink: io::Stream)" },

	//! Sink stream
	{ DaoXCoder_Stream,		".stream(invar self: Encoder) => io::Stream" },

	//! Number of bytes successfully written to the sink
	{ DaoXCoder_Counter,	".bytesWritten(invar self: Encoder) => int" },

	/*! Writes \a value and returns self.
	 *
	 * \note Multibyte integer values are written with big-endian byte order
	 * \warning Floating point value representation may vary on different platforms */
	{ DaoEncoder_WriteI8,	"i8(self: Encoder, value: int) => Encoder" },
	{ DaoEncoder_WriteU8,	"u8(self: Encoder, value: int) => Encoder" },
	{ DaoEncoder_WriteI16,	"i16(self: Encoder, value: int) => Encoder" },
	{ DaoEncoder_WriteI16,	"u16(self: Encoder, value: int) => Encoder" },
	{ DaoEncoder_WriteI32,	"i32(self: Encoder, value: int) => Encoder" },
	{ DaoEncoder_WriteI32,	"u32(self: Encoder, value: int) => Encoder" },
	{ DaoEncoder_WriteI64,	"i64(self: Encoder, value: int) => Encoder" },
	{ DaoEncoder_WriteI64,	"u64(self: Encoder, value: int) => Encoder" },
	{ DaoEncoder_WriteF32,	"f32(self: Encoder, value: float) => Encoder" },
	{ DaoEncoder_WriteF64,	"f64(self: Encoder, value: float) => Encoder" },

	//! Writes \c bytes to the sink and returns self
	{ DaoEncoder_WriteBytes,"bytes(self: Encoder, bytes: string) => Encoder" },
	{ NULL, NULL }
};

//! Stateful binary encoder
DaoTypeBase encoderTyper = {
	"Encoder", NULL, NULL, encoderMeths, {NULL}, {0},
	(FuncPtrDel)DaoXCoder_Delete, NULL
};

static void DaoDecoder_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *res = (DaoXCoder*)dao_malloc( sizeof(DaoXCoder) );
	DaoStream *stream = &p[0]->xStream;
	res->counter = 0;
	if ( DaoStream_IsReadable( stream ) ){
		DaoGC_IncRC( p[0] );
		res->stream = &p[0]->xStream;
	}
	else {
		DaoProcess_RaiseError( proc, "Param", "Stream not readable" );
		dao_free( res );
		return;
	}
	DaoProcess_PutCdata( proc, res, daox_type_decoder );
}

uint64_t DaoDecoder_ReadInteger( DaoXCoder *self, int size )
{
	DaoStream *stream = self->stream;
	uint8_t buf[9] = {0};
	uint64_t value = 0;
	int i, count;

	count = DaoStream_ReadBytes( stream, buf, size );
	if( count != size ) return 0;

	for ( i = size; i > 0; i-- ){
		value <<= CHAR_BIT;
		value |= buf[size - i];
	}
	return value;
}

dao_float DaoDecoder_ReadFloat( DaoXCoder *self, int size )
{
	DaoStream *stream = self->stream;
	uint8_t buf[9] = {0};
	int count;

	count = DaoStream_ReadBytes( stream, buf, size );
	if( count != size ) return 0;

	return size == sizeof(float)? *(float*)buf : *(dao_float*)buf;
}

static void DaoDecoder_ReadI8( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	int8_t value = DaoDecoder_ReadInteger( self, 1 );
	DaoProcess_PutInteger( proc, value );
}

static void DaoDecoder_ReadU8( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	uint8_t value = DaoDecoder_ReadInteger( self, 1 );
	DaoProcess_PutInteger( proc, value );
}

static void DaoDecoder_ReadI16( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	int16_t value = DaoDecoder_ReadInteger( self, 2 );
	DaoProcess_PutInteger( proc, value );
}

static void DaoDecoder_ReadU16( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	uint16_t value = DaoDecoder_ReadInteger( self, 2 );
	DaoProcess_PutInteger( proc, value );
}

static void DaoDecoder_ReadI32( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	int32_t value = DaoDecoder_ReadInteger( self, 4 );
	DaoProcess_PutInteger( proc, value );
}

static void DaoDecoder_ReadU32( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	uint32_t value = DaoDecoder_ReadInteger( self, 4 );
	DaoProcess_PutInteger( proc, value );
}

static void DaoDecoder_ReadI64( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	int64_t value = DaoDecoder_ReadInteger( self, 8 );
	DaoProcess_PutInteger( proc, value );
}

static void DaoDecoder_ReadBytes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	daoint size = p[1]->xInteger.value;
	DaoStream *stream = self->stream;
	DString *buf = DaoProcess_PutChars( proc, "" );
	if ( !CheckStream( proc, self->stream ) )
		return;
	size = DaoStream_Read( stream, buf, size );
	if( size > 0 ) self->counter += size;
}
static void DaoDecoder_ReadF32( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	float value = DaoDecoder_ReadFloat( self, sizeof(float) );
	if ( sizeof(float) != 4 )
		DaoProcess_RaiseWarning( proc, NULL, "The size of C float type is not 4 on this platform" );
	DaoProcess_PutFloat( proc, value );
}

static void DaoDecoder_ReadF64( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXCoder *self = (DaoXCoder*)DaoValue_TryGetCdata( p[0] );
	if ( !CheckStream( proc, self->stream ) )
		return;
	dao_float value = DaoDecoder_ReadFloat( self, sizeof(dao_float) );
	if ( sizeof(dao_float) != 8 )
		DaoProcess_RaiseWarning( proc, NULL, "The size of C double type is not 8 on this platform" );
	DaoProcess_PutFloat( proc, value );
}

static DaoFuncItem decoderMeths[] =
{
	//! Creates a decoder reading from \a source
	{ DaoDecoder_Create,	"Decoder(source: io::Stream)" },

	//! Source stream
	{ DaoXCoder_Stream,		".stream(invar self: Decoder) => io::Stream" },

	//! Number of bytes successfully read
	{ DaoXCoder_Counter,	".bytesRead(invar self: Decoder) => int" },

	/*! Reads a value from the source
	 *
	 * \note Multibyte integer values are assumed to be in big-endian byte order
	 * \warning Floating point value representation may vary on different platforms */
	{ DaoDecoder_ReadI8,	"i8(self: Decoder) => int" },
	{ DaoDecoder_ReadU8,	"u8(self: Decoder) => int" },
	{ DaoDecoder_ReadI16,	"i16(self: Decoder) => int" },
	{ DaoDecoder_ReadU16,	"u16(self: Decoder) => int" },
	{ DaoDecoder_ReadI32,	"i32(self: Decoder) => int" },
	{ DaoDecoder_ReadU32,	"u32(self: Decoder) => int" },
	{ DaoDecoder_ReadI64,	"i64(self: Decoder) => int" },
	{ DaoDecoder_ReadI64,	"u64(self: Decoder) => int" },
	{ DaoDecoder_ReadF32,	"f32(self: Decoder) => float" },
	{ DaoDecoder_ReadF64,	"f64(self: Decoder) => float" },

	//! Reads at most \a count bytes from the source
	{ DaoDecoder_ReadBytes,	"bytes(self: Decoder, count: int) => string" },
	{ NULL, NULL }
};

//! Stateful binary decoder
DaoTypeBase decoderTyper = {
	"Decoder", NULL, NULL, decoderMeths, {NULL}, {0},
	(FuncPtrDel)DaoXCoder_Delete, NULL
};

static DaoFuncItem encodableMeths[] =
{
	//! Serializes self using the provided \a encoder
	{ NULL,	"encode(invar self: Encodable, encoder: Encoder)" },
	{ NULL, NULL }
};

//! A type which can be encoded to binary form. Use it to define conversions
//! to specific serialization formats
static DaoTypeBase encodableTyper = {
	"Encodable", NULL, NULL, encodableMeths, {NULL}, {0},
	(FuncPtrDel)NULL, NULL
};

static DaoFuncItem decodableMeths[] =
{
	//! Deserializes self using the provided \a decoder
	{ NULL,	"decode(decoder: Decoder) => Decodable" },
	{ NULL, NULL }
};

//! A type which can be decoded from binary form. Use it to define conversions
//! from specific serialization formats
static DaoTypeBase decodableTyper = {
	"Decodable", NULL, NULL, decodableMeths, {NULL}, {0},
	(FuncPtrDel)NULL, NULL
};

DAO_DLL int DaoBinary_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *binns;
	if ( sizeof(dao_integer) != 8 ){
		DaoStream* stream = DaoVmSpace_ErrorStream( vmSpace );
		DaoStream_WriteChars( stream,
				"This module is not compatible with Dao compiled with 32-bit int type!\n" );
		return 1;
	}
	binns = DaoNamespace_GetNamespace( ns, "bin" );
	daox_type_encoder = DaoNamespace_WrapType( binns, &encoderTyper, DAO_CDATA, 0 );
	daox_type_decoder = DaoNamespace_WrapType( binns, &decoderTyper, DAO_CDATA, 0 );
	DaoNamespace_WrapInterface( binns, &encodableTyper );
	DaoNamespace_WrapInterface( binns, &decodableTyper );
	DaoNamespace_WrapFunctions( binns, binMeths );
	return 0;
}
