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
#include "string.h"
#include "errno.h"

#include"dao.h"
#include"daoValue.h"
#include"daoNamespace.h"

static const char jsonerr[] = "JSON";
static DaoType *booltype;

void JSON_Indent( DString *text, int indent )
{
	int i;
	for( i = 0; i < indent; i++ )
		DString_AppendChar( text, '\t' );
}

int JSON_SerializeValue( DaoValue *value, DString *text, int indent )
{
	daoint i, res;
	char buf[100];
	char *sep = indent >= 0? ",\n" : ",";
	DaoList *list;
	DaoMap *map;
	DNode *node;
	DString *str;
	switch( value->type ){
	case DAO_INTEGER:
		snprintf( buf, sizeof(buf), "%"DAO_INT_FORMAT, DaoValue_TryGetInteger( value ) );
		DString_AppendChars( text, buf );
		break;
	case DAO_FLOAT:
		snprintf( buf, sizeof(buf), "%f", DaoValue_TryGetFloat( value ) );
		DString_AppendChars( text, buf );
		break;
	case DAO_DOUBLE:
		snprintf( buf, sizeof(buf), "%f", DaoValue_TryGetDouble( value ) );
		DString_AppendChars( text, buf );
		break;
	case DAO_ENUM:
		if ( value->xEnum.subtype == DAO_ENUM_BOOL )
			DString_AppendChars( text, value->xEnum.value? "true" : "false" );
		else
			return value->type;
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
			if( ( res = JSON_SerializeValue( DaoList_GetItem( list, i ) , text, indent ) ) != 0 )
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
		if( indent >= 0 ){
			DString_AppendChars( text, "{\n" );
			indent++;
		}
		else
			DString_AppendChars( text, "{" );
		map = DaoValue_CastMap( value );
		node = DaoMap_First( map );
		while( node != NULL ){
			JSON_Indent( text, indent );
			if( DaoValue_Type( DNode_Key( node ) ) != DAO_STRING )
				return -1;
			if( ( res = JSON_SerializeValue( DNode_Key( node ), text, indent ) ) != 0 )
				return res;
			DString_AppendChars( text, ": " );
			if( ( res = JSON_SerializeValue( DNode_Value( node ), text, indent ) ) != 0 )
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
		return value->type;
	}
	return 0;
}

static void JSON_Serialize( DaoProcess *proc, DaoValue *p[], int N )
{
	char buf[100];
	int res = DaoValue_TryGetEnum( p[1] );
	if( ( res = JSON_SerializeValue( p[0], DaoProcess_PutChars( proc, "" ), res? -1 : 0 ) ) != 0 ){
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
		DaoProcess_RaiseError( proc, "Type", buf );
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
		return (DaoValue*) DaoProcess_NewDouble( process, dres );
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
		return (DaoValue*) DaoProcess_NewEnum( process, booltype, 0 );
	}
	buf[4] = '\0';
	if( strcmp( buf, "true" ) == 0 ){
		*text += 4;
		return (DaoValue*) DaoProcess_NewEnum( process, booltype, 1 );
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
	 * - int, float, double => number
	 * - none => null
	 * - enum<false:true> => bool
	 */
	{ JSON_Serialize,	"serialize(invar data: map<string,@V>|list<@V>, style: enum<pretty,compact> = $pretty) => string" },

	/*! Parses JSON in \a str and returns the corresponding map or list.
	 *
	 * Deserialization of values (JSON => Dao):
	 * - array  => list
	 * - object => map
	 * - number => int or double (depending on the presence of decimal separator)
	 * - null   => none
	 * - bool   => enum<false:true>
	 */
	{ JSON_Deserialize,	"parse(str: string) => map<string,any>|list<any>" },
	{ NULL, NULL }
};

DAO_DLL int DaoJSON_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *jsonns;
	DString *bname = DString_NewChars( "bool" );
	booltype = DaoNamespace_FindType( ns, bname );
	DString_Delete( bname );
	jsonns = DaoNamespace_GetNamespace( ns, "json" );
	DaoNamespace_WrapFunctions( jsonns, jsonMeths );
	return 0;
}
