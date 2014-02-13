/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2013, Limin Fu
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

#include "stdio.h"
#include "string.h"
#include "errno.h"

#include"dao.h"
#include"daoValue.h"

#define DAO_INT_LFORMAT  L"%" DAO_INT_FORMAT

void JSON_Indent( DString *text, int indent )
{
	int i;
	for( i = 0; i < indent; i++ )
		DString_AppendWChar( text, L'\t' );
}

int JSON_SerializeValue( DaoValue *value, DString *text, int indent )
{
	daoint i, res;
	wchar_t buf[100];
	wchar_t *sep = indent >= 0? L",\n" : L",";
	DaoList *list;
	DaoMap *map;
	DNode *node;
	DString *str;
	switch( DaoValue_Type( value ) ){
	case DAO_INTEGER:
		swprintf( buf, sizeof(buf), DAO_INT_LFORMAT, DaoValue_TryGetInteger( value ) );
		DString_AppendWCS( text, buf );
		break;
	case DAO_FLOAT:
		swprintf( buf, sizeof(buf), L"%f", DaoValue_TryGetFloat( value ) );
		DString_AppendWCS( text, buf );
		break;
	case DAO_DOUBLE:
		swprintf( buf, sizeof(buf), L"%f", DaoValue_TryGetDouble( value ) );
		DString_AppendWCS( text, buf );
		break;
	case DAO_STRING:
		DString_AppendWChar( text, L'"' );
		str = DString_Copy( value->xString.data );
		DString_ToWCS( str );
		for ( i = 0; i < str->size; i++ )
			switch ( str->wcs[i] ){
			case L'\"': DString_AppendWCS( text, L"\\\"" ); break;
			case L'\\': DString_AppendWCS( text, L"\\\\" ); break;
			case L'/':  DString_AppendWCS( text, L"\\/" ); break;
			case L'\b': DString_AppendWCS( text, L"\\b" ); break;
			case L'\f': DString_AppendWCS( text, L"\\f" ); break;
			case L'\n': DString_AppendWCS( text, L"\\n" ); break;
			case L'\r': DString_AppendWCS( text, L"\\r" ); break;
			case L'\t': DString_AppendWCS( text, L"\\t" ); break;
			default:	DString_AppendWChar( text, str->wcs[i] );
			}
		DString_AppendWChar( text, L'"' );
		DString_Delete( str );
		break;
	case DAO_LIST:
		if( indent >= 0 ){
			DString_AppendWCS( text, L"[\n" );
			indent++;
		}
		else
			DString_AppendWCS( text, L"[" );
		list = DaoValue_CastList( value );
		for( i = 0; i < DaoList_Size( list ); i++ ){
			JSON_Indent( text, indent );
			if( ( res = JSON_SerializeValue( DaoList_GetItem( list, i ) , text, indent ) ) != 0 )
				return res;
			if( i != DaoList_Size( list ) - 1 )
				DString_AppendWCS( text, sep );
			else if( indent >= 0 )
				DString_AppendWCS( text, L"\n" );
		}
		if( indent > 0 )
			indent--;
		JSON_Indent( text, indent );
		DString_AppendWCS( text, L"]");
		break;
	case DAO_MAP:
		if( indent >= 0 ){
			DString_AppendWCS( text, L"{\n" );
			indent++;
		}
		else
			DString_AppendWCS( text, L"{" );
		map = DaoValue_CastMap( value );
		node = DaoMap_First( map );
		while( node != NULL ){
			JSON_Indent( text, indent );
			if( DaoValue_Type( DNode_Key( node ) ) != DAO_STRING )
				return -1;
			if( ( res = JSON_SerializeValue( DNode_Key( node ), text, indent ) ) != 0 )
				return res;
			DString_AppendWCS( text, L": " );
			if( ( res = JSON_SerializeValue( DNode_Value( node ), text, indent ) ) != 0 )
				return res;
			node = DaoMap_Next( map, node );
			if( node != NULL )
				DString_AppendWCS( text, sep );
			else if( indent >= 0 )
				DString_AppendWCS( text, L"\n" );
		}
		if( indent > 0 )
			indent--;
		JSON_Indent( text, indent );
		DString_AppendWCS( text, L"}");
		break;
	case DAO_NONE:
		DString_AppendWCS( text, L"null" );
		break;
	default:
		return DaoValue_Type( value );
	}
	return 0;
}

static void JSON_Serialize( DaoProcess *proc, DaoValue *p[], int N )
{
	char buf[100];
	int res = DaoValue_TryGetEnum( p[1] );
	if( ( res = JSON_SerializeValue( p[0], DaoProcess_PutWCString( proc, L"" ), res? -1 : 0 ) ) != 0 ){
		if( res == -1 )
			strcpy( buf, "Non-string key in map/object" );
		else{
			strcpy( buf, "Unsupported value type: " );
			switch( res ){
				case DAO_COMPLEX:   strcat( buf, "complex" ); break;
				case DAO_ENUM:      strcat( buf, "enum" ); break;
				case DAO_ARRAY:	    strcat( buf, "array" );	break;
				case DAO_TUPLE:	    strcat( buf, "tuple" );	break;
				case DAO_OBJECT:    strcat( buf, "object" ); break;
				case DAO_CLASS:
				case DAO_NAMESPACE:	strcat( buf, "class/namespace" ); break;
				case DAO_CDATA:
				case DAO_CTYPE:	    strcat( buf, "cdata/ctype" ); break;
				case DAO_INTERFACE:	strcat( buf, "interface" );	break;
				case DAO_ROUTINE:   strcat( buf, "routine" ); break;
				case DAO_TYPE:      strcat( buf, "type" ); break;
				default:            strcat( buf, "[type not recognized]" );
			}
		}
		DaoProcess_RaiseException( proc, DAO_ERROR, buf );
	}
}

enum
{
	JSON_ArrayNotClosed = 1,
	JSON_ObjectNotClosed,
	JSON_UnexpectedRSB,
	JSON_UnexpectedRCB,
	JSON_UnexpectedComa,
	JSON_MissingComa,
	JSON_InvalidToken,
	JSON_UnexpectedColon,
	JSON_MissingColon,
	JSON_NonStringKey
};

DaoValue* JSON_ParseString( DaoProcess *process, wchar_t* *text )
{
	wchar_t* end = *text + 1;
	DaoValue *value;
	for( ; *end != L'\0'; end++ )
		if( *end == L'\\' && *(end + 1) != L'\0' )
			end++;
		else if( *end == L'"' ){
			DString *str;
			daoint i;
			value = (DaoValue*) DaoProcess_NewWCString( process, *text + 1, end - *text - 1 );
			*text = end + 1;
			str = value->xString.data;
			for ( i = 0; i < str->size - 1; i++ )
				if ( str->wcs[i] == L'\\' )
					switch ( str->wcs[i + 1] ){
					case L'\"': DString_InsertWCS( str, L"\"", i, 2, 1 ); break;
					case L'\\': DString_InsertWCS( str, L"\\", i, 2, 1 ); break;
					case L'/':  DString_InsertWCS( str, L"/", i, 2, 1 ); break;
					case L'b':  DString_InsertWCS( str, L"\b", i, 2, 1 ); break;
					case L'f':  DString_InsertWCS( str, L"\f", i, 2, 1 ); break;
					case L'n':  DString_InsertWCS( str, L"\n", i, 2, 1 ); break;
					case L'r':  DString_InsertWCS( str, L"\r", i, 2, 1 ); break;
					case L't':  DString_InsertWCS( str, L"\t", i, 2, 1 ); break;
					case L'u':
						if ( i < str->size - 5 ){
							DString_Erase( str, i + 1, 5 );
							str->wcs[i] = (wchar_t)wcstol( str->wcs, NULL, 16 );
						}
						break;
					}
			return value;
		}
	return NULL;
}

DaoValue* JSON_ParseNumber( DaoProcess *process, wchar_t* *text )
{
	wchar_t* stop;
	double dres;
	int ires;
	errno = 0;
	ires = wcstol( *text, &stop, 10 );
	if( errno == ERANGE || ( *stop != L'\0' && wcschr( L"eE.", *stop ) != NULL && stop != *text ) ){
		dres = wcstod( *text, &stop );
		*text = stop;
		return (DaoValue*) DaoProcess_NewDouble( process, dres );
	}
	else if( stop != *text ){
		*text = stop;
		return (DaoValue*) DaoProcess_NewInteger( process, ires );
	}
	return NULL;
}

DaoValue* JSON_ParseSpecialLiteral( DaoProcess *process, wchar_t* *text )
{
	wchar_t buf[6];
	wcsncpy( buf, *text, 5 );
	buf[5] = L'\0';
	if( wcscmp( buf, L"false" ) == 0 ){
		*text += 5;
		return (DaoValue*) DaoProcess_NewInteger( process, 0 );
	}
	buf[4] = L'\0';
	if( wcscmp( buf, L"true" ) == 0 ){
		*text += 4;
		return (DaoValue*) DaoProcess_NewInteger( process, 1 );
	}
	else if( wcscmp( buf, L"null" ) == 0 ){
		*text += 4;
		return (DaoValue*) DaoProcess_NewNone( process );
	}
	return NULL;
}

wchar_t* JSON_FindData( wchar_t* text, int *line )
{
	for( ; *text != L'\0'; text++ ){
		if( *text == L'/' && *( text + 1 ) == L'/' )
			for( text += 2; ; text++ ){
				if( *text == L'\0' )
					return NULL;
				else if( *text == L'\n' ){
					(*line)++;
					break;
				}
			}
		else if( *text == L'/' && *( text + 1 ) == L'*' )
			for( text += 2; ; text++ ){
				if( *text == L'\0' )
					return NULL;
				else if( *text == L'*' && *( ++text ) == L'/' )
					break;
				else if( *text == L'\n' )
					(*line)++;
			}
		else if( *text == L'\n' )
			(*line)++;
		else if( !wcschr( L" \t\r", *text ) )
			return text;
	}
	return NULL;
}

DaoValue* JSON_ParseObject( DaoProcess *process, DaoValue *object, wchar_t* *text, int *error, int *line );

DaoValue* JSON_ParseArray( DaoProcess *process, DaoValue *exlist, wchar_t* *text, int *error, int *line )
{
	wchar_t* data;
	DaoList *list = exlist? (DaoList*)exlist : DaoProcess_NewList( process );
	DaoValue *value;
	int coma = 0;
	(*text)++;
	for( ;; ){
		data = JSON_FindData( *text, line );
		if( data == NULL ){
			*error = JSON_ArrayNotClosed;
			return NULL;
		}
		*text = data;
		if( *data == L']' ){
			if( coma || DaoList_Size( list ) == 0 ){
				(*text)++;
				return (DaoValue*) list;
			}
			else{
				*error = JSON_UnexpectedRSB;
				return NULL;
			}
		}
		else if( *data == L',' ){
			if( !coma ){
				*error = JSON_UnexpectedComa;
				return NULL;
			}
			coma = 0;
			(*text)++;
			continue;
		}
		else if( coma ){
			*error = JSON_MissingComa;
			return NULL;
		}
		else if( *data == L'"' )
			value = JSON_ParseString( process, text );
		else if( *data == L'[' )
			value = JSON_ParseArray( process, NULL, text, error, line );
		else if( *data == L'{' )
			value = JSON_ParseObject( process, NULL, text, error, line );
		else if( wcschr( L"0123456789-", *data ) != NULL )
			value = JSON_ParseNumber( process, text );
		else
			value = JSON_ParseSpecialLiteral( process, text );
		if( value == NULL ){
			if( !*error )
				*error = JSON_InvalidToken;
			return NULL;
		}
		DaoList_PushBack( list, value );
		coma = 1;
	}
}

DaoValue* JSON_ParseObject( DaoProcess *process, DaoValue *exmap, wchar_t* *text, int *error, int *line )
{
	wchar_t* data;
	DaoMap *map = exmap? (DaoMap*)exmap : DaoProcess_NewMap( process, 0 );
	DaoValue *key, *value;
	DaoValue **val = &key;
	int coma = 0, colon = 0;
	(*text)++;
	for( ;; ){
		data = JSON_FindData( *text, line );
		if( data == NULL ){
			*error = JSON_ObjectNotClosed;
			return NULL;
		}
		*text = data;
		if( *data == L'}' ){
			if( coma || DaoMap_Size( map ) == 0 ){
				(*text)++;
				return (DaoValue*) map;
			}
			else{
				*error = JSON_UnexpectedRCB;
				return NULL;
			}
		}
		else if( *data == L',' ){
			if( !coma ){
				*error = JSON_UnexpectedComa;
				return NULL;
			}
			coma = 0;
			(*text)++;
			continue;
		}
		else if( coma ){
			*error = JSON_MissingComa;
			return NULL;
		}
		else if( *data == L':' ){
			if( !colon ){
				*error = JSON_UnexpectedColon;
				return NULL;
			}
			colon = 0;
			(*text)++;
			continue;
		}
		else if( colon ){
			*error = JSON_MissingColon;
			return NULL;
		}
		else if( *data == L'"' )
			*val = JSON_ParseString( process, text );
		else if( val == &key ){
			*error = JSON_NonStringKey;
			return NULL;
		}
		else if( *data == L'[' )
			*val = JSON_ParseArray( process, NULL, text, error, line );
		else if( *data == L'{' )
			*val = JSON_ParseObject( process, NULL, text, error, line );
		else if( wcschr( L"0123456789-", *data ) != NULL )
			*val = JSON_ParseNumber( process, text );
		else
			*val = JSON_ParseSpecialLiteral( process, text );
		if( *val == NULL ){
			if( !*error )
				*error = JSON_InvalidToken;
			return NULL;
		}
		if( val == &key ){
			val = &value;
			colon = 1;
		}
		else{
			DaoMap_Insert( map, key, value );
			val = &key;
			coma = 1;
		}
	}
}

static void JSON_Deserialize( DaoProcess *proc, DaoValue *p[], int N )
{
	char buf[100];
	int error = 0, line = 1;
	wchar_t *text = JSON_FindData( DaoValue_TryGetWCString( p[0] ), &line );
	DaoValue *value;

	if( text == NULL ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "JSON data not found" );
		return;
	}
	if( *text == L'{' )
		value = JSON_ParseObject( proc, (DaoValue*)DaoProcess_PutMap( proc, 1 ), &text, &error, &line );
	else if( *text == L'[' )
		value = JSON_ParseArray( proc, (DaoValue*)DaoProcess_PutList( proc ), &text, &error, &line );
	else{
		DaoProcess_RaiseException( proc, DAO_ERROR, "JSON data is not an object or array" );
		return;
	}
	if( value == NULL ){
		strcpy( buf, "JSON parser error at line " );
		snprintf( buf + strlen( buf ), 10, "%i", line );
		strcat( buf, ": " );
		switch( error ){
		case JSON_ArrayNotClosed:  strcat( buf, "unexpected end of data (array)" ); break;
		case JSON_ObjectNotClosed: strcat( buf, "unexpected end of data (object)" ); break;
		case JSON_UnexpectedRSB:   strcat( buf, "unexpected ']'" ); break;
		case JSON_UnexpectedRCB:   strcat( buf, "unexpected '}'" ); break;
		case JSON_UnexpectedComa:  strcat( buf, "unexpected ','" ); break;
		case JSON_MissingComa:     strcat( buf, "missing ','" ); break;
		case JSON_InvalidToken:    strcat( buf, "invalid token" ); break;
		case JSON_UnexpectedColon: strcat( buf, "unexpected ':'" ); break;
		case JSON_MissingColon:    strcat( buf, "missing ':'" ); break;
		case JSON_NonStringKey:    strcat( buf, "non-string key in object" ); break;
		default:                   strcat( buf, "[undefined error]" );
		}
		DaoProcess_RaiseException( proc, DAO_ERROR, buf );
		return;
	}
	if( JSON_FindData( text, &line ) != NULL )
		DaoProcess_RaiseException( proc, DAO_ERROR, "JSON data does not form a single structure" );
}

DAO_DLL int DaoJSON_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace_WrapFunction( ns, (DaoCFunction)JSON_Serialize,
		"toJSON( self: map<string, @V>|list<@V>, style: enum<pretty,compact>=$pretty )=>string" );
	DaoNamespace_WrapFunction( ns, (DaoCFunction)JSON_Deserialize,
		"parseJSON( self: string )=>map<string, any>|list<any>" );
	return 0;
}
