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

// 2011-01: Danilov Aleksey, initial implementation.

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
		uchar_t high = self->chars[i*3], mid = self->chars[i*3 + 1], low = self->chars[i*3 + 2];
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
		uchar_t highest = base64_nums[self->chars[i*4]], higher = base64_nums[self->chars[i*4 + 1]];
		uchar_t lower = base64_nums[self->chars[i*4 + 2]], lowest = base64_nums[self->chars[i*4 + 3]];
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
		uchar_t highest = base64_nums[self->chars[self->size - 4]], higher = base64_nums[self->chars[self->size - 3]];
		uchar_t lower = 0;
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
	uchar_t *chs = self->chars;
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
	const uchar_t z85_nums[] = {
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
	uchar_t *chs;
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
		uchar_t ch = self->chars[i];
		dest->chars[2*i] = hex_chars[ch >> 4];
		dest->chars[2*i + 1] = hex_chars[ch & 0x0F];
	}
}

int DString_DecodeHex( DString *self, DString *dest, daoint *errpos )
{
	const uchar_t hex_nums[] = {
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
		uchar_t high = hex_nums[self->chars[2*i]], low = hex_nums[self->chars[2*i + 1]];
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
	FILE* file = stream->file;
	DaoArray *arr = &p[1]->xArray;
	dao_integer count =  p[2]->xInteger.value;
	size_t size = 0;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_READABLE ) == 0 ){
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
	DaoProcess_PutInteger( proc, fread( arr->data.p, size, count, file ) );
}

static void DaoBinary_Unpack( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[0]->xStream;
	FILE* file = stream->file;
	DaoArray *arr = &p[1]->xArray;
	size_t sizes[] = {1, 2, 4};
	size_t size =  sizes[p[2]->xEnum.value];
	dao_integer count = p[3]->xInteger.value;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_READABLE ) == 0 ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not readable" );
		return;
	}
	if( count <= 0 || count > arr->size*sizeof(dao_integer)/size )
		count = arr->size*sizeof(dao_integer)/size;
	count = fread( arr->data.p, size, count, file );
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
	FILE* file = stream->file;
	DaoArray *arr = &p[0]->xArray;
	size_t sizes[] = {1, 2, 4};
	size_t size =  sizes[p[2]->xEnum.value];
	dao_integer count = p[3]->xInteger.value, res = 0;
	size_t i;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_WRITABLE ) == 0 ){
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
				char *bytes = (char*)data;
				for( i = 0; i < count; i++ )
					bytes[i] = arr->data.i[i];
			}
			break;
		case 2:
			if( 1 ){
				short *words = (short*)data;
				for( i = 0; i < count; i++ )
					words[i] = arr->data.i[i];
			}
			break;
		case 4:
			if( 1 ){
				int *dwords = (int*)data;
				for( i = 0; i < count; i++ )
					dwords[i] = arr->data.i[i];
			}
			break;
		}
		res = fwrite( data, size, count, file );
		dao_free( data );
	}
	else
		res = fwrite( arr->data.i, size, count, file );
	DaoProcess_PutInteger( proc, res );
}

static void DaoBinary_Write( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[1]->xStream;
	FILE* file = stream->file;
	DaoArray *arr = &p[0]->xArray;
	size_t size = 0;
	dao_integer count = p[2]->xInteger.value;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_WRITABLE ) == 0 ){
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
	DaoProcess_PutInteger( proc, fwrite( arr->data.p, size, count, file ) );
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
			uchar_t *ptr = data + offset/CHAR_BIT + index/CHAR_BIT;
			val |= (uchar_t)( *ptr << ( index%CHAR_BIT ) ) >> ( CHAR_BIT - 1 ) << ( count - i - 1 );
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
		case Binary_UByte:	num = *(uchar_t*)data; break;
		case Binary_Byte:	num = *(char*)data; break;
		case Binary_UWord:  num = *(ushort_t*)data; break;
		case Binary_Word:	num = *(short*)data; break;
		case Binary_UDWord: num = *(uint_t*)data; break;
		case Binary_DWord:	num = *(int*)data; break;
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
			uchar_t *ptr = data + offset/CHAR_BIT + index/CHAR_BIT;
			int pos = CHAR_BIT - index%CHAR_BIT - 1;
			uchar_t byteval = (uint_t)( value << ( 4*CHAR_BIT - count + i ) ) >> ( 4*CHAR_BIT - 1 ) << pos;
			*ptr = ( *ptr & ~( (uchar_t)1 << pos ) ) | byteval;
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
		case Binary_UByte:	*(uchar_t*)data = num; break;
		case Binary_Byte:	*(char*)data = num; break;
		case Binary_UWord:  *(ushort_t*)data = num; break;
		case Binary_Word:	*(short*)data = num; break;
		case Binary_UDWord: *(uint_t*)data = num; break;
		case Binary_DWord:	*(int*)data = num; break;
		case Binary_UQWord: *(dao_integer*)data = num; break;
		case Binary_QWord:	*(dao_integer*)data = num; break;
		}
	}
}

/*! \note All the routines below accepting \c io::Stream expect a file stream only */
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
	{ DaoBinary_GetItem,	"get(invar source: array<int>|string, what: enum<byte,ubyte,word,uword,dword,udword,qword,uqword>,"
								"offset: int) => int" },
	{ DaoBinary_GetItem,	"get(invar source: array<int>|string, what: enum<float>, offset: int) => float" },

	/*! Reads \a count bits from \a source at the given bit \a offset */
	{ DaoBinary_GetItem,	"get(invar source: array<int>|string, what: enum<bits>, offset: int, count: int) => int" },

	/*! Writes \a value described by \a what to \a dest at the given byte \a offset */
	{ DaoBinary_SetItem,	"set(dest: array<int>, what: enum<byte,ubyte,word,uword,dword,udword,qword,uqword>,"
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

DAO_DLL int DaoBinary_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *binns;
	binns = DaoNamespace_GetNamespace( ns, "bin" );
	DaoNamespace_WrapFunctions( binns, binMeths );
	return 0;
}
