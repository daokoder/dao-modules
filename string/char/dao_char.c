/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2015, Limin Fu
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

// 2014-05: Danilov Aleksey, initial implementation.

#include <ctype.h>
#include <string.h>

#include"dao.h"
#include"daoValue.h"

static int AppendWChar( DString *str, wchar_t ch )
{
	char buf[] = {0, 0, 0, 0, 0};
	mbstate_t state;
	int len;
	memset( &state, 0, sizeof(state) );
	len = wcrtomb( buf, ch, &state );
	DString_AppendChars( str, buf );
	return len;
}

static void DaoChar_CPToStr( DaoProcess *proc, DaoValue *p[], int N )
{
	uint_t cp = p[0]->xInteger.value;
	int local = p[1]->xEnum.value == 1;
	int len;
	DString *out = DaoProcess_PutChars( proc, "" );
	if ( local ){
		if ( sizeof(wchar_t) == 4 ) // utf-32
			len = AppendWChar( out, (wchar_t)cp );
		else { // utf-16
			if ( cp <= 0xFFFF ) // bmp
				len = AppendWChar( out, (wchar_t)cp );
			else { // surrogates
				cp -= 0x10000;
				len = AppendWChar( out, (wchar_t)( ( cp >> 10 ) + 0xD800 ) );
				if ( len > 0 )
					len = AppendWChar( out, (wchar_t)( ( cp & 0x3FF ) + 0xDC00 ) );
			}
		}
	}
	else {
		DString_AppendWChar( out, cp );
		len = ( !out->size || out->chars[0] == 0xFFFD || out->chars[0] == 0 )? -1 : 1;
	}
	if ( len <= 0 )
		DString_Clear( out );
}

static void DaoChar_StrToCP( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *ch = p[0]->xString.value;
	int local = p[1]->xEnum.value == 1;
	uint_t cp;
	int len;
	if ( !ch->size ){
		DaoProcess_PutInteger( proc, 0 );
		return;
	}
	if ( local ){
		wchar_t buf[2];
		mbstate_t state;
		memset( &state, 0, sizeof(state) );
		len = mbrtowc( buf, ch->chars, MB_CUR_MAX, &state );
		if ( sizeof(wchar_t) == 2 && buf[0] >= 0xD800 && buf[0] <= 0xDBFF ) // UTF-16 surrogate pairs
			cp = ( ( (size_t)buf[0] - 0xD800 ) << 10 ) + ( (size_t)buf[1] - 0xDC00 );
		else
			cp = (size_t)buf[0];
	}
	else {
		DCharState st = DString_DecodeChar( ch->chars, ch->chars + ch->size );
		cp = st.value;
		len = !st.type? -1 : st.width;
	}
	if ( len < 0 || len < ch->size )
		cp = 0;
	DaoProcess_PutInteger( proc, cp );
}

static void DaoChar_ByteNum( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *str = p[0]->xString.value;
	daoint index = p[1]->xInteger.value;
	uchar_t byte;
	daoint res = -1;
	if ( index < 0 )
		index += str->size;
	if ( index < str->size && index >= 0 ){
		byte = str->chars[index];
		if ( ( byte & 0x80 ) == 0 || byte >> 5 == 0x6 || byte >> 4 == 0xE || byte >> 3 == 0x1E )
			res = 0;
		else if ( byte >> 6 == 0x2 ){
			int i;
			for ( i = 1; i < 4; i++ )
				if ( --index < 0 )
					break;
				else {
					byte = str->chars[index];
					if ( byte >> 5 == 0x6 || byte >> 4 == 0xE || byte >> 3 == 0x1E ){
						res = i;
						break;
					}
					if ( byte >> 6 != 0x2 )
						break;
				}
		}
	}
	DaoProcess_PutInteger( proc, res );
}

#define CheckTrail( chs, i ) ( *( chs + i ) >> 6 == 0x2 )

static void DaoChar_IndCount( DaoProcess *proc, DaoValue *p[], int indices )
{
	DString *str = p[0]->xString.value;
	int local = p[1]->xEnum.value == 1;
	DaoArray *arr;
	daoint count = 0;
	if ( indices )
		arr = DaoProcess_PutArray( proc );
	if ( local ){
		mbstate_t state;
		char *chs = str->chars;
		memset( &state, 0, sizeof(state) );
		while ( 1 ){
			int len;
			daoint i = chs - str->chars;
			wchar_t buf[2];
			if ( i >= str->size || *chs == '\0' )
				break;
			len = mbrtowc( buf, chs, MB_CUR_MAX, &state );
			if ( len <= 0 ){
				char errbuf[100];
				snprintf( errbuf, sizeof(errbuf), "Invalid character at index %i", i );
				DaoProcess_RaiseError( proc, "Value", errbuf );
				return;
			}
			if ( indices ){
				DaoArray_ResizeVector( arr, arr->size + 1 );
				arr->data.i[arr->size - 1] = i;
			}
			else
				count++;
			chs += len;
		}
	}
	else {
		uchar_t *chs = (uchar_t*)str->chars;
		while ( 1 ){
			uchar_t byte = *chs;
			int len = 0;
			daoint i = (char*)chs - str->chars;
			if ( i >= str->size || *chs == 0 )
				break;
			if ( ( byte & 0x80 ) == 0 )
				len = 1;
			else if ( byte >> 5 == 0x6 )
				len = ( i < str->size - 1 && CheckTrail( chs, 1 ) )? 2 : 0;
			else if ( byte >> 4 == 0xE )
				len = ( i < str->size - 2 && CheckTrail( chs, 1 ) && CheckTrail( chs, 2 ) )? 3 : 0;
			else if ( byte >> 3 == 0x1E )
				len = ( i < str->size - 3 && CheckTrail( chs, 1 ) && CheckTrail( chs, 2 ) && CheckTrail( chs, 3 ) )? 4 : 0;
			if ( !len ){
				char errbuf[100];
				snprintf( errbuf, sizeof(errbuf), "Invalid character at index %i", i );
				DaoProcess_RaiseError( proc, "Value", errbuf );
				return;
			}
			if ( indices ){
				DaoArray_ResizeVector( arr, arr->size + 1 );
				arr->data.i[arr->size - 1] = i;
			}
			else
				count++;
			chs += len;
		}
	}
	if ( !indices )
		DaoProcess_PutInteger( proc, count );
}

#undef CheckTrail

static void DaoChar_Indices( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoChar_IndCount( proc, p, 1 );
}

static void DaoChar_Count( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoChar_IndCount( proc, p, 0 );
}

enum {
	Char_Alpha = 0,
	Char_Digit,
	Char_Alnum,
	Char_Punct,
	Char_Print,
	Char_Space,
	Char_Lower,
	Char_Upper
};

static void DaoChar_Check( DaoProcess *proc, DaoValue *p[], int N )
{
	int ch = p[0]->xInteger.value;
	int res;
	switch ( p[1]->xEnum.value ){
	case Char_Alpha:	res = isalpha( ch ); break;
	case Char_Digit:	res = isdigit( ch ); break;
	case Char_Alnum:	res = isalnum( ch ); break;
	case Char_Punct:	res = ispunct( ch ); break;
	case Char_Print:	res = isprint( ch ); break;
	case Char_Space:	res = isspace( ch ); break;
	case Char_Lower:	res = islower( ch ); break;
	case Char_Upper:	res = isupper( ch ); break;
	}
	DaoProcess_PutBoolean( proc, res != 0 );
}

static DaoFunctionEntry charMeths[]=
{
	/*! Returns Unicode \a codepoint converted to string with the given \a encoding. In case of invalid code point, 0 is returned */
	{ DaoChar_CPToStr,		"decode(codepoint: int, encoding: enum<utf8,local> = $utf8) => string" },

	/*! Returns character \a char converted to Unicode code point. \a char must be a string of single Unicode character in encoding \a encoding.
	 * In case of empty string, invalid character or string of multiple characters, 0 is returned */
	{ DaoChar_StrToCP,		"encode(char: string, encoding: enum<utf8,local> = $utf8) => int" },

	/*! Examines the byte at \a index in a UTF-8 string \a str and returns its offset from the start of the character it belongs to
	 * (0, 1, 2 or 3). In case of invalid index or character, -1 is returned */
	{ DaoChar_ByteNum,		"offset(str: string, index: int) => int" },

	/*! Returns indices of all characters in \a str with the given \a encoding */
	{ DaoChar_Indices,		"indices(str: string, encoding: enum<utf8,local> = $utf8) => array<int>" },

	/*! Returns the number of characters in \a str with \a encoding
	 * \note The number of characters may not correspond to the number of visible symbols */
	{ DaoChar_Count,		"count(str: string, encoding: enum<utf8,local> = $utf8) => int" },

	/*! Checks if \a char, which is a locally-encoded 8-bit character, belongs to the given character \a category */
	{ DaoChar_Check,		"check(char: int, category: enum<alpha,digit,alnum,punct,print,space,lower,upper>) => bool" },
	{ NULL, NULL },
};

DAO_DLL int DaoChar_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *charns;
	charns = DaoVmSpace_GetNamespace( vmSpace, "char" );
	DaoNamespace_AddConstValue( ns, "char", (DaoValue*)charns );
	DaoNamespace_WrapFunctions( charns, charMeths );
	return 0;
}
