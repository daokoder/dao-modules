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

// 2011-10: Danilov Aleksey, initial implementation.

#include "stdio.h"
#include "ctype.h"
#include "string.h"
#include "errno.h"

#include"dao.h"
#include"daoValue.h"
#include"daoNamespace.h"
#include"daoProcess.h"

static const char jsonerr[] = "JSON";
static DaoType *booltype;
static DaoType *json_list_type;
static DaoType *json_map_type;

typedef enum {
	InvalidKeyType = -1,
	CodeSectionFailed = -2,
	RecursionTooDeep = -3
} json_error_t;

void JSON_Indent( DString *text, int indent )
{
	int i;
	for( i = 0; i < indent; i++ )
		DString_AppendChar( text, '\t' );
}

int JSON_SerializeValue( DaoProcess *proc, DaoVmCode *sect, int entry, DaoValue *value, DString *text, int indent, int level )
{
	daoint i, res;
	char buf[100];
	char *sep = indent >= 0? ",\n" : ",";
	DaoList *list;
	DaoMap *map;
	DNode *node;
	DString *str;
	if (level >= 100)
		return RecursionTooDeep;
	switch( value->type ){
	case DAO_INTEGER:
		snprintf( buf, sizeof(buf), "%lli", DaoValue_TryGetInteger( value ) );
		DString_AppendChars( text, buf );
		break;
	case DAO_FLOAT:
		snprintf( buf, sizeof(buf), "%f", DaoValue_TryGetFloat( value ) );
		DString_AppendChars( text, buf );
		break;
	case DAO_BOOLEAN:
		DString_AppendChars( text, value->xBoolean.value? "true" : "false" );
		break;
	case DAO_STRING:
		DString_AppendChar( text, '"' );
		str = DString_Copy( value->xString.value );
		for ( i = 0; i < str->size; i++ )
			switch ( str->chars[i] ){
			case '\"': DString_AppendChars( text, "\\\"" ); break;
			case '\\': DString_AppendChars( text, "\\\\" ); break;
			case '/':  DString_AppendChars( text, "\\/" ); break;
			case '\b': DString_AppendChars( text, "\\b" ); break;
			case '\f': DString_AppendChars( text, "\\f" ); break;
			case '\n': DString_AppendChars( text, "\\n" ); break;
			case '\r': DString_AppendChars( text, "\\r" ); break;
			case '\t': DString_AppendChars( text, "\\t" ); break;
			default:   DString_AppendChar( text, str->chars[i] );
			}
		DString_AppendChar( text, '"' );
		DString_Delete( str );
		break;
	case DAO_LIST:
		if( indent >= 0 ){
			DString_AppendChars( text, "[\n" );
			indent++;
		}
		else
			DString_AppendChars( text, "[" );
		list = DaoValue_CastList( value );
		for( i = 0; i < DaoList_Size( list ); i++ ){
			JSON_Indent( text, indent );
			if( ( res = JSON_SerializeValue( proc, sect, entry, DaoList_GetItem( list, i ) , text, indent, level + 1 ) ) != 0 )
				return res;
			if( i != DaoList_Size( list ) - 1 )
				DString_AppendChars( text, sep );
			else if( indent >= 0 )
				DString_AppendChars( text, "\n" );
		}
		if( indent > 0 )
			indent--;
		JSON_Indent( text, indent );
		DString_AppendChars( text, "]");
		break;
	case DAO_MAP:
		map = DaoValue_CastMap( value );
		if( indent >= 0 ){
			DString_AppendChars( text, "{\n" );
			indent++;
		}
		else
			DString_AppendChars( text, "{" );
		node = DaoMap_First( map );
		while( node != NULL ){
			DaoValue *key = DNode_Key( node );
			JSON_Indent( text, indent );
			if( DaoValue_Type( key ) != DAO_STRING ){
				if ( sect ){
					if ( sect->b > 0 )
						DaoProcess_SetValue( proc, sect->a, key );
					proc->topFrame->entry = entry;
					if ( !DaoProcess_Execute( proc ) )
						return CodeSectionFailed;
					key = proc->stackValues[0];
					if( DaoValue_Type( key ) != DAO_STRING )
						return InvalidKeyType;
				}
				else
					return InvalidKeyType;
			}
			if( ( res = JSON_SerializeValue( proc, sect, entry, key, text, indent, level + 1 ) ) != 0 )
				return res;
			DString_AppendChars( text, ": " );
			if( ( res = JSON_SerializeValue( proc, sect, entry, DNode_Value( node ), text, indent, level + 1 ) ) != 0 )
				return res;
			node = DaoMap_Next( map, node );
			if( node != NULL )
				DString_AppendChars( text, sep );
			else if( indent >= 0 )
				DString_AppendChars( text, "\n" );
		}
		if( indent > 0 )
			indent--;
		JSON_Indent( text, indent );
		DString_AppendChars( text, "}");
		break;
	case DAO_NONE:
		DString_AppendChars( text, "null" );
		break;
	default:
	CallSection:
		if ( sect ){
			if ( sect->b > 0 )
				DaoProcess_SetValue( proc, sect->a, value );
			proc->topFrame->entry = entry;
			if ( !DaoProcess_Execute( proc ) )
				return CodeSectionFailed;
			res = JSON_SerializeValue( proc, sect, entry, proc->stackValues[0], text, indent, level + 1 );
			if ( res != 0 )
				return res;
		}
		else
			return value->type;
	}
	return 0;
}

void JSON_SerializeData( DaoProcess *proc, DaoValue *p[], int custom )
{
	char buf[100];
	int res = DaoValue_TryGetEnum( p[1] );
	DaoVmCode *sect = custom? DaoProcess_InitCodeSection( proc, 0 ) : NULL;
	res = JSON_SerializeValue( sect? proc : NULL, sect, proc->topFrame->entry, p[0], DaoProcess_PutChars( proc, "" ), res? -1 : 0, 0 );
	if ( sect )
		DaoProcess_PopFrame( proc );
	if( res != 0 && res != CodeSectionFailed ){
		if( res == InvalidKeyType )
			strcpy( buf, "Non-string key in map/object" );
		else if ( res == RecursionTooDeep )
			strcpy( buf, "Recursion level too high" );
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
		DaoProcess_RaiseError( proc, "Type", buf );
	}
}

static void JSON_Serialize( DaoProcess *proc, DaoValue *p[], int N )
{
	JSON_SerializeData( proc, p, 0 );
}

static void JSON_Serialize2( DaoProcess *proc, DaoValue *p[], int N )
{
	JSON_SerializeData( proc, p, 1 );
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

DaoValue* JSON_ParseString( DaoProcess *process, char* *text )
{
	char* end = *text + 1;
	DaoValue *value;
	for( ; *end != '\0'; end++ )
		if( *end == '\\' && *(end + 1) != '\0' )
			end++;
		else if( *end == '"' ){
			DString *str;
			daoint i;
			value = (DaoValue*) DaoProcess_NewString( process, *text + 1, end - *text - 1 );
			*text = end + 1;
			str = value->xString.value;
			for ( i = 0; i < str->size - 1; i++ )
				if ( str->chars[i] == '\\' )
					switch ( str->chars[i + 1] ){
					case '\"': DString_InsertChars( str, "\"", i, 2, 1 ); break;
					case '\\': DString_InsertChars( str, "\\", i, 2, 1 ); break;
					case '/':  DString_InsertChars( str, "/", i, 2, 1 ); break;
					case 'b':  DString_InsertChars( str, "\b", i, 2, 1 ); break;
					case 'f':  DString_InsertChars( str, "\f", i, 2, 1 ); break;
					case 'n':  DString_InsertChars( str, "\n", i, 2, 1 ); break;
					case 'r':  DString_InsertChars( str, "\r", i, 2, 1 ); break;
					case 't':  DString_InsertChars( str, "\t", i, 2, 1 ); break;
					case 'u':
						if ( i < str->size - 5 ){
							int j;
							for ( j = 1; j < 5 && isxdigit( str->chars[i + j + 1] ); j++ );
							if ( j == 5 ){
								DString *buf = DString_New();
								DString_AppendWChar( buf, strtoul( str->chars + i + 2, NULL, 16 ) );
								DString_Insert( str, buf, i, 6, buf->size );
								DString_Delete( buf );
							}
						}
						break;
					}
			return value;
		}
	return NULL;
}

DaoValue* JSON_ParseNumber( DaoProcess *process, char* *text )
{
	char* stop;
	double dres;
	int ires;
	errno = 0;
	ires = strtol( *text, &stop, 10 );
	if( errno == ERANGE || ( *stop != '\0' && strchr( "eE.", *stop ) != NULL && stop != *text ) ){
		dres = strtod( *text, &stop );
		*text = stop;
		return (DaoValue*) DaoProcess_NewFloat( process, dres );
	}
	else if( stop != *text ){
		*text = stop;
		return (DaoValue*) DaoProcess_NewInteger( process, ires );
	}
	return NULL;
}

DaoValue* JSON_ParseSpecialLiteral( DaoProcess *process, char* *text )
{
	char buf[6];
	strncpy( buf, *text, 5 );
	buf[5] = '\0';
	if( strcmp( buf, "false" ) == 0 ){
		*text += 5;
		return (DaoValue*) DaoProcess_NewBoolean( process, 0 );
	}
	buf[4] = '\0';
	if( strcmp( buf, "true" ) == 0 ){
		*text += 4;
		return (DaoValue*) DaoProcess_NewBoolean( process, 1 );
	}
	else if( strcmp( buf, "null" ) == 0 ){
		*text += 4;
		return (DaoValue*) DaoProcess_NewNone( process );
	}
	return NULL;
}

char* JSON_FindData( char* text, int *line )
{
	for( ; *text != '\0'; text++ ){
		if( *text == '/' && *( text + 1 ) == '/' )
			for( text += 2; ; text++ ){
				if( *text == '\0' )
					return NULL;
				else if( *text == '\n' ){
					(*line)++;
					break;
				}
			}
		else if( *text == '/' && *( text + 1 ) == '*' )
			for( text += 2; ; text++ ){
				if( *text == '\0' )
					return NULL;
				else if( *text == '*' && *( ++text ) == '/' )
					break;
				else if( *text == '\n' )
					(*line)++;
			}
		else if( *text == '\n' )
			(*line)++;
		else if( !strchr( " \t\r", *text ) )
			return text;
	}
	return NULL;
}

DaoValue* JSON_ParseObject( DaoProcess *process, DaoValue *object, char* *text, int *error, int *line );

DaoValue* JSON_ParseArray( DaoProcess *process, DaoValue *exlist, char* *text, int *error, int *line )
{
	char* data;
	DaoList *list = exlist? (DaoList*)exlist : DaoProcess_NewList( process );
	DaoValue *value;
	int coma = 0;
	(*text)++;
	DaoList_SetType( list, json_list_type );
	for( ;; ){
		data = JSON_FindData( *text, line );
		if( data == NULL ){
			*error = JSON_ArrayNotClosed;
			return NULL;
		}
		*text = data;
		if( *data == ']' ){
			if( coma || DaoList_Size( list ) == 0 ){
				(*text)++;
				return (DaoValue*) list;
			}
			else{
				*error = JSON_UnexpectedRSB;
				return NULL;
			}
		}
		else if( *data == ',' ){
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
		else if( *data == '"' )
			value = JSON_ParseString( process, text );
		else if( *data == '[' )
			value = JSON_ParseArray( process, NULL, text, error, line );
		else if( *data == '{' )
			value = JSON_ParseObject( process, NULL, text, error, line );
		else if( strchr( "0123456789-", *data ) != NULL )
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

DaoValue* JSON_ParseObject( DaoProcess *process, DaoValue *exmap, char* *text, int *error, int *line )
{
	char* data;
	DaoMap *map = exmap? (DaoMap*)exmap : DaoProcess_NewMap( process, 0 );
	DaoValue *key, *value;
	DaoValue **val = &key;
	int coma = 0, colon = 0;
	(*text)++;
	DaoMap_SetType( map, json_map_type );
	for( ;; ){
		data = JSON_FindData( *text, line );
		if( data == NULL ){
			*error = JSON_ObjectNotClosed;
			return NULL;
		}
		*text = data;
		if( *data == '}' ){
			if( coma || DaoMap_Size( map ) == 0 ){
				(*text)++;
				return (DaoValue*) map;
			}
			else{
				*error = JSON_UnexpectedRCB;
				return NULL;
			}
		}
		else if( *data == ',' ){
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
		else if( *data == ':' ){
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
		else if( *data == '"' )
			*val = JSON_ParseString( process, text );
		else if( val == &key ){
			*error = JSON_NonStringKey;
			return NULL;
		}
		else if( *data == '[' )
			*val = JSON_ParseArray( process, NULL, text, error, line );
		else if( *data == '{' )
			*val = JSON_ParseObject( process, NULL, text, error, line );
		else if( strchr( "0123456789-", *data ) != NULL )
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
	char *text = JSON_FindData( DaoValue_TryGetChars( p[0] ), &line );
	DaoValue *value;

	if( text == NULL ){
		DaoProcess_RaiseError( proc, jsonerr, "JSON data not found" );
		return;
	}
	if( *text == '{' )
		value = JSON_ParseObject( proc, (DaoValue*)DaoProcess_PutMap( proc, 1 ), &text, &error, &line );
	else if( *text == '[' )
		value = JSON_ParseArray( proc, (DaoValue*)DaoProcess_PutList( proc ), &text, &error, &line );
	else{
		DaoProcess_RaiseError( proc, jsonerr, "JSON data is not an object or array" );
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
		DaoProcess_RaiseError( proc, jsonerr, buf );
		return;
	}
	if( JSON_FindData( text, &line ) != NULL )
		DaoProcess_RaiseError( proc, jsonerr, "JSON data does not form a single structure" );
}

static DaoFuncItem jsonMeths[] =
{
	/*! Serializes \a data to JSON and returns the resulting string. When \a style is \c $pretty, the output includes
	 * newlines and	indentation for readability, otherwise the result is put on single line.
	 *
	 * Serialization of values (Dao => JSON):
	 * - list => array
	 * - map  => object
	 * - int, float => number
	 * - string => string
	 * - none => null
	 * - bool => bool
	 */
	{ JSON_Serialize,	"serialize(invar data: Object|Array, style: enum<pretty,compact> = $pretty) => string" },

	/*! Similar to the above, but accepts arbitrary data as input. Each item of type other then \c int, \c float, \c bool,
	 * \c none, \c string, \c map<string,@T> or \c list found in \a data is passed to the specified code section, which should implement
	 * its conversion to \c Data, \c list or \c map<string,any>. The serialization then proceeds recursively */
	{ JSON_Serialize2,	"serialize(invar data: any, style: enum<pretty,compact> = $pretty)"
								 "[invar item: any => Data|list<any>|map<string,any>] => string" },

	/*! Parses JSON in \a str and returns the corresponding map or list.
	 *
	 * Deserialization of values (JSON => Dao):
	 * - array  => list
	 * - object => map
	 * - number => int or double (depending on the presence of decimal separator)
	 * - string => string
	 * - null   => none
	 * - bool   => bool
	 */
	{ JSON_Deserialize,	"parse(str: string) => Object|Array" },
	{ NULL, NULL }
};

static DaoFuncItem encodableMeths[] =
{
	//! Serializes self to JSON data
	{ NULL,	"encode(invar self: Encodable) => Data" },
	{ NULL, NULL }
};

//! A type which can be encoded to JSON data. Use it in conjunction with \c Marshallable
//! to define serialization of custom data structures to JSON
DaoTypeBase encodableTyper = {
	"Encodable", NULL, NULL, encodableMeths, {NULL}, {0},
	(FuncPtrDel)NULL, NULL
};

static DaoFuncItem decodableMeths[] =
{
	//! Deserializes self from the provided JSON \a data
	{ NULL,	"decode(invar data: Data) => Decodable" },
	{ NULL, NULL }
};

//! A type which can be decoded from JSON data. Use it in conjunction with \c Unmarshallable
//! to define deserialization of custom data structures from JSON
DaoTypeBase decodableTyper = {
	"Decodable", NULL, NULL, decodableMeths, {NULL}, {0},
	(FuncPtrDel)NULL, NULL
};

static DaoFuncItem marshallableMeths[] =
{
	//! Serializes self to JSON document
	{ NULL,	"marshal(invar self: Marshallable) => Object|Array" },
	{ NULL, NULL }
};

//! A type which can be marshalled to a JSON document
DaoTypeBase marshallableTyper = {
	"Marshallable", NULL, NULL, marshallableMeths, {NULL}, {0},
	(FuncPtrDel)NULL, NULL
};

static DaoFuncItem unmarshallableMeths[] =
{
	//! Deserializes self from the given JSON \a document
	{ NULL,	"unmarshal(invar document: Object|Array) => Unmarshallable" },
	{ NULL, NULL }
};

//! A type which can be unmarshalled from a JSON document
DaoTypeBase unmarshallableTyper = {
	"Unmarshallable", NULL, NULL, unmarshallableMeths, {NULL}, {0},
	(FuncPtrDel)NULL, NULL
};

DAO_DLL int DaoJson_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *jsonns;
	jsonns = DaoNamespace_GetNamespace( ns, "json" );
	DaoNamespace_DefineType( jsonns, "none|bool|int|float|string|list<Data>|map<string,Data>", "Data" );
	json_list_type = DaoNamespace_DefineType( jsonns, "list<Data>", "Array" );
	json_map_type = DaoNamespace_DefineType( jsonns, "map<string,Data>", "Object" );
	DaoNamespace_WrapInterface( jsonns, &encodableTyper );
	DaoNamespace_WrapInterface( jsonns, &decodableTyper );
	DaoNamespace_WrapInterface( jsonns, &marshallableTyper );
	DaoNamespace_WrapInterface( jsonns, &unmarshallableTyper );
	DaoNamespace_WrapFunctions( jsonns, jsonMeths );
	return 0;
}
