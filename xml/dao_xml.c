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
#include"dao_xml.h"

typedef int xml_error;

enum {
	XML_InvalidChar = 1,
	XML_InvalidRefChar,
	XML_CommentNotClosed,
	XML_CdataNotClosed,
	XML_LiteralNotClosed,
	XML_InstructionNotClosed,
	XML_InvalidDeclEnding,
	XML_HyphensInComment,
	XML_InvalidNameChar,
	XML_MisplacedLT,
	XML_InvalidReference,
	XML_UnknownEntity,
	XML_InvalidTagEnding,
	XML_InvalidAttrDefinition,
	XML_DuplicateAttrDefinition,
	XML_ReservedInstructionName,
	XML_NoVersionInfo,
	XML_InvalidVersionInfo,
	XML_InvalidEncodingInfo,
	XML_InvalidDeclAttributes,
	XML_InvalidStandaloneInfo,
	XML_CdataEndInCharData,
	XML_NoElement,
	XML_InvalidSyntax,
	XML_MisplacedDoctype,
	XML_MultipleRootElements,
	XML_InvalidEndTag,
	XML_DoctypeNotClosed,
	XML_MisplacedDeclaration,
	XML_TagNotClosed
};

int IsXMLChar( size_t ch )
{
	return ch == 0x9 || ch == 0xA || ch == 0xD || ( ch >= 0x20 && ch <= 0xD7FF ) || ( ch >= 0xE000 && ch <= 0xFFFD ) ||
			( ch >= 0x10000 && ch <= 0x10FFFF );
}

int IsXMLNameStartChar( size_t ch )
{
	return ch == ':' || ch == '_' || ( ch >= 'A' && ch <= 'Z' ) || ( ch >= 'a' && ch <= 'z' )
			|| ( ch >= 0xC0 && ch <= 0xD6 ) || ( ch >= 0xD8 && ch <= 0xF6 ) || ( ch >= 0xF8 && ch <= 0x2FF ) || ( ch >= 0x370 && ch <= 0x37D )
			|| ( ch >= 0x37F && ch <= 0x1FFF ) || ch == 0x200C || ch == 0x200D || ( ch >= 0x2070 && ch <= 0x218F )
			|| ( ch >= 0x2C00 && ch <= 0x2FEF ) || ( ch >= 0x3001 && ch <= 0xD7FF ) || ( ch >= 0xF900 && ch <= 0xFDCF )
			|| ( ch >= 0xFDF0 && ch <= 0xFFFD )	|| ( ch >= 0x10000 && ch <= 0xEFFFF );
}

int IsXMLNameChar( size_t ch )
{
	return IsXMLNameStartChar( ch ) || ch == '-' || ch == '.' || ( ch >= '0' && ch <= '9' ) || ch == 0xB7 || ( ch >= 0x300 && ch <= 0x36F )
			|| ( ch >= 0x203F && ch <= 0x2040 );
}

size_t GetChar( XMLContext *ctx )
{
	DCharState state = DString_DecodeChar( ctx->pos, ctx->pos + 4 );
	if ( !state.type ){
		ctx->len = 0;
		return 0xFFFF;
	}
	ctx->pos += state.width;
	ctx->len = state.width;
	return state.value;
}

char* UngetChar( XMLContext *ctx )
{
	ctx->pos -= ctx->len;
	ctx->len = 0;
	return ctx->pos;
}

xml_error ParseXMLReference( XMLContext *ctx, DString *dest )
{
	char *pos = ctx->pos;
	ctx->len = 0;
	/* char reference */
	if ( *pos == '#' ){
		size_t ch;
		char *end = pos + 1;
		int base = 10, offset = 1;
		if ( *end == 'x' ){
			base = 16;
			offset = 2;
			end++;
			for ( ; isxdigit( *end ); end++ );
		}
		else
			for ( ; isdigit( *end ); end++ );
		if ( *end != ';' || end == pos + offset ){
			ctx->pos = pos;
			return XML_InvalidReference;
		}
		ch = strtoul( pos + offset, NULL, base );
		if ( ( ch != ULONG_MAX || errno != ERANGE ) && IsXMLChar( ch ) )
			DString_AppendWChar( dest, ch );
		else {
			ctx->pos = pos;
			return XML_InvalidRefChar;
		}
		ctx->pos = end + 1;
	}
	/* entity reference */
	else {
		const char *ents[] = { "amp", "lt", "gt", "apos", "quot" };
		const size_t chars[] = { '&', '<', '>', '\'', '"' };
		int i;
		char *end = strchr( pos, ';' );
		if ( !end || end == pos ){
			ctx->pos = pos;
			return XML_InvalidReference;
		}
		for ( i = 0; i < 5; i++ )
			if ( strncmp( pos, ents[i], end - pos ) == 0 ){
				DString_AppendChar( dest, chars[i] );
				break;
			}
		if ( i == 5 ){
			ctx->pos = pos;
			return XML_UnknownEntity;
		}
		ctx->pos = end + 1;
	}
	return 0;
}

xml_error ParseXMLLiteral( XMLContext *ctx, DString *dest  )
{
	size_t delim = GetChar( ctx ), ch;
	xml_error res;
	while ( ( ch = GetChar( ctx ) ) != delim ){
		switch ( ch ){
		case '\r':
			if ( *ctx->pos != '\n' )
				DString_AppendChar( dest, ' ' );
			break;
		case '\n':
		case ' ':
		case '\t':
			DString_AppendChar( dest, ' ' );
			break;
		case '&':
			res = ParseXMLReference( ctx, dest );
			if ( res )
				return res;
			break;
		case '<':		return XML_MisplacedLT;
		case '\0':		return XML_LiteralNotClosed;
		default:
			if ( !IsXMLChar( ch ) )
				return XML_InvalidChar;
			DString_AppendWChar( dest, ch );
		}
	}
	return 0;
}

xml_error ParseXMLName( XMLContext *ctx, DString *dest, DMap *namepool )
{
	char *start = ctx->pos;
	if ( !IsXMLNameStartChar( GetChar( ctx ) ) )
		return XML_InvalidNameChar;
	while ( IsXMLNameChar( GetChar( ctx ) ) );
	UngetChar( ctx );
	if ( namepool ){
		DString name = { NULL, 0, 0, 0, 0, NULL };
		DNode *node;
		name.chars = start;
		name.size = name.bufSize = ctx->pos - start;
		node = DMap_Find( namepool, &name );
		if ( node )
			DString_Assign( dest, node->key.pString );
		else {
			DString_Assign( dest, &name );
			DString_Detach( dest, 0 );
			DMap_Insert( namepool, dest, NULL );
		}
	}
	else if ( dest )
		DString_SetBytes( dest, start, ctx->pos - start );
	return 0;
}

xml_error ParseXMLComment( XMLContext *ctx, DString *dest )
{
	char *start = ctx->pos;
	while ( 1 ) {
		size_t ch = GetChar( ctx );
		if ( ch == '\0' )
			return XML_CommentNotClosed;
		if ( ch == '-' && *ctx->pos == '-' ){
			if ( *( ctx->pos + 1 ) == '>' ){
				ctx->pos += 2;
				break;
			}
			return XML_HyphensInComment;
		}
		if ( !IsXMLChar( ch ) )
			return XML_InvalidChar;
	}
	if ( dest )
		DString_SetBytes( dest, start, ctx->pos - start - 3 );
	return 0;
}

xml_error ParseXMLCdata( XMLContext *ctx, DString *dest )
{
	char *start = ctx->pos;
	while ( 1 ){
		size_t ch = GetChar( ctx );
		if ( ch == '\0' )
			return XML_CdataNotClosed;
		if ( ch == ']' && *ctx->pos == ']' && *( ctx->pos + 1 ) == '>' ){
			ctx->pos += 2;
			break;
		}
		if ( !IsXMLChar( ch ) )
			return XML_InvalidChar;
	}
	DString_SetBytes( dest, start, ctx->pos - start - 3 );
	return 0;
}

daoint PassXMLWhitespace( XMLContext *ctx )
{
	char *start = ctx->pos;
	while ( 1 )
		switch ( *ctx->pos ){
		case '\r':
		case '\n':
		case ' ':
		case '\t':
			ctx->pos++;
			break;
		default:
			ctx->len = 0;
			return ctx->pos - start;
		}
	return 0;
}

xml_error ParseXMLDoctype( XMLContext *ctx, DString *dest )
{
	char *start = ctx->pos;
	daoint count = 1;
	while ( 1 ){
		size_t ch = GetChar( ctx ), delim;
		switch ( ch ){
		case '<':
			count++;
			break;
		case '>':
			count--;
			if ( count == 0 ){
				if ( dest )
					DString_AppendBytes( dest, start, ctx->pos - start );
				return 0;
			}
			break;
		case '\'':
		case '"':
			for ( delim = ch; ( ch = GetChar( ctx ) ) != delim; ){
				if ( ch == '\0' )
					return XML_LiteralNotClosed;
				if ( !IsXMLChar( ch ) )
					return XML_InvalidChar;
			}
			break;
		case '\0':
			return XML_DoctypeNotClosed;
		default:
			if ( !IsXMLChar( ch ) )
				return XML_InvalidChar;
		}
	}
	return 0;
}

xml_error ParseXMLInstruction( XMLContext *ctx, DString *name, DString *data, DMap *namepool )
{
	char *end = strstr( ctx->pos, "?>" );
	xml_error res;
	daoint count;
	if ( !end )
		return XML_InstructionNotClosed;
	res = ParseXMLName( ctx, name, namepool );
	if ( res )
		return res;
	if ( strcmp( name->chars, "xml") == 0 )
		return XML_ReservedInstructionName;
	switch ( *ctx->pos ){
	case '\r':
	case '\n':
	case ' ':
	case '\t':
		ctx->pos++;
		break;
	default:
		ctx->len = 0;
		return XML_InvalidSyntax;
	}
	if ( ctx->pos != end ){
		if ( !count )
			return XML_InvalidNameChar;
		else {
			char *start = ctx->pos;
			while ( ctx->pos < end )
				if ( !IsXMLChar( GetChar( ctx ) ) )
					return XML_InvalidChar;
			DString_SetBytes( data, start, end - start );
		}
	}
	ctx->pos = end + 2;
	return 0;
}

xml_error ParseXMLTagAttributes( XMLContext *ctx, int *empty, DMap *attribs, int xmldecl, DMap *namepool )
{
	DString *att, *val;
	xml_error res;
	daoint count;
	size_t ch;
	att = DString_New();
	val = DString_New();
	while ( 1 ){
		char *start = ctx->pos;
		res = ParseXMLName( ctx, att, namepool );
		if ( res )
			break;
		PassXMLWhitespace( ctx );
		if ( ( ch = GetChar( ctx ) ) != '=' ){
			res = XML_InvalidAttrDefinition;
			break;
		}
		PassXMLWhitespace( ctx );
		if ( *ctx->pos != '\'' && *ctx->pos != '"' ){
			ctx->len = 0;
			res = XML_InvalidAttrDefinition;
			break;
		}
		DString_Clear( val );
		res = ParseXMLLiteral( ctx, val );
		if ( res )
			break;
		if ( DMap_Find( attribs, att ) ){
			ctx->pos = start;
			ctx->len = 0;
			res = XML_DuplicateAttrDefinition;
			break;
		}
		DMap_Insert( attribs, att, val );
		count = PassXMLWhitespace( ctx );
		if ( xmldecl ){
			if ( *ctx->pos == '?' ){
				ctx->pos++;
				if ( ( ch = GetChar( ctx ) ) == '>' ){
					res = 0;
					break;
				}
				else {
					res = XML_InvalidDeclEnding;
					break;
				}
			}
		}
		else {
			if ( ( ch = GetChar( ctx ) ) == '>' ){
				*empty = 0;
				res = 0;
				break;
			}
			else if ( ch == '/' ){
				if ( ( ch = GetChar( ctx ) ) == '>' ){
					*empty = 1;
					res = 0;
					break;
				}
				else {
					res = XML_InvalidTagEnding;
					break;
				}
			}
			UngetChar( ctx );
		}
		if ( !count ){
			res = XML_InvalidSyntax;
			break;
		}
	}
	DString_Delete( att );
	DString_Delete( val );
	return res;
}

xml_error ParseXMLStartTag( XMLContext *ctx, DString *name, int *empty, DMap **attribs, DMap *namepool )
{
	xml_error res = ParseXMLName( ctx, name, namepool );
	daoint count;
	size_t ch;
	if ( res )
		return res;
	count = PassXMLWhitespace( ctx );
	if ( ( ch = GetChar( ctx ) ) == '>' ){
		*empty = 0;
		return 0;
	}
	else if ( ch == '/' ){
		if ( ( ch = GetChar( ctx ) ) == '>' ){
			*empty = 1;
			return 0;
		}
		else
			return XML_InvalidTagEnding;
	}
	if ( !count )
		return XML_InvalidNameChar;
	UngetChar( ctx );
	if ( *attribs == NULL )
		*attribs = DMap_New( DAO_DATA_STRING, DAO_DATA_STRING );
	return ParseXMLTagAttributes( ctx, empty, *attribs, 0, namepool );
}

xml_error ParseXMLEndTag( XMLContext *ctx, char *name )
{
	size_t ch;
	daoint len = strlen( name ), count;
	if ( strncmp( ctx->pos, name, len ) )
		return XML_InvalidEndTag;
	ctx->pos += len;
	count = PassXMLWhitespace( ctx );
	if ( ( ch = GetChar( ctx ) ) != '>' )
		return !count && IsXMLNameChar( ch )? XML_InvalidEndTag : XML_InvalidTagEnding;
	return 0;
}

xml_error ParseXMLDeclaration( XMLContext *ctx, DString *version, DString *encoding, int *standalone )
{
	char *start = ctx->pos, *end = strstr( ctx->pos, "?>" );
	DMap *attribs;
	xml_error res = 0;
	int empty;
	DNode *node;
	DString *key = DString_New(), *val;
	daoint count = 1;
	if ( !end )
		return XML_InvalidDeclEnding;
	*standalone = 0;
	if ( ctx->pos == end )
		return XML_NoVersionInfo;
	if ( !PassXMLWhitespace( ctx ) )
		return XML_InvalidNameChar;
	attribs = DMap_New( DAO_DATA_STRING, DAO_DATA_STRING );
	res = ParseXMLTagAttributes( ctx, &empty, attribs, 1, NULL );
	if ( res )
		goto Exit;
	/* version */
	DString_SetChars( key, "version" );
	node = DMap_Find( attribs, key );
	if ( !node ){
		res = XML_NoVersionInfo;
		goto Exit;
	}
	val = node->value.pString;
	if ( val->size > 2 && val->chars[0] == '1' && val->chars[1] == '.' ){
		char *pos;
		for ( pos = val->chars + 2; isdigit( *pos ); pos++ );
		if ( *pos != '\0' ){
			res = XML_InvalidVersionInfo;
			goto Exit;
		}
		if ( version )
			DString_Assign( version, val );
	}
	else {
		res = XML_InvalidVersionInfo;
		goto Exit;
	}
	/* encoding */
	DString_SetChars( key, "encoding" );
	node = DMap_Find( attribs, key );
	if ( node ){
		char *pos;
		count++;
		if ( strstr( start, "encoding" ) < strstr( start, "version" ) ){
			res = XML_InvalidDeclAttributes;
			goto Exit;
		}
		val = node->value.pString;
		pos = val->chars;
		if ( !( ( *pos >= 'A' && *pos <= 'Z' ) || ( *pos >= 'a' && *pos <= 'z' ) ) ){
			res = XML_InvalidEncodingInfo;
			goto Exit;
		}
		for ( pos++; *pos != '\0' && ( ( *pos >= 'A' && *pos <= 'Z' ) || ( *pos >= 'a' && *pos <= 'z' ) || isdigit( *pos ) ||
									   strchr( "._-", *pos ) ); pos++ );
		if ( *pos != '\0' ){
			res = XML_InvalidEncodingInfo;
			goto Exit;
		}
		if ( encoding )
			DString_Assign( encoding, val );
	}
	/* standalone */
	DString_SetChars( key, "standalone" );
	node = DMap_Find( attribs, key );
	if ( node ){
		count++;
		if ( ( count == 2 && strstr( start, "standalone" ) < strstr( start, "version" ) ) ||
			 ( count == 3 && strstr( start, "standalone" ) < strstr( start, "encoding" ) ) ){
			res = XML_InvalidDeclAttributes;
			goto Exit;
		}
		val = node->value.pString;
		if ( strcmp( val->chars, "yes" ) == 0 )
			*standalone = 1;
		else if ( strcmp( val->chars, "no" ) == 0 )
			*standalone = 0;
		else
			res = XML_InvalidStandaloneInfo;
	}
	if ( attribs->size != count )
		res = XML_InvalidDeclAttributes;
Exit:
	DString_Delete( key );
	DMap_Delete( attribs );
	return res;
}

xml_error ParseXMLCharData( XMLContext *ctx, DString *dest )
{
	daoint i;
	char *str, *start = ctx->pos;
	xml_error res;
	while ( 1 ){
		size_t ch = GetChar( ctx );
		switch ( ch ){
		case '&':
			DString_AppendBytes( dest, start, ctx->pos - start - 1 );
			res = ParseXMLReference( ctx, dest );
			if ( res )
				return res;
			start = ctx->pos;
			break;
		case '\0':
		case '<':
			DString_AppendBytes( dest, start, ctx->pos - start - 1 );
			DString_Trim( dest, 1, 1, 1 );
			str = dest->chars;
			for ( i = 0; i < dest->size; i++ )
				if ( str[i] == '\r' ){
					if ( str[i + 1] == '\n' )
						DString_Erase( dest, i, 1 );
					else
						str[i] = '\n';
				}
			ctx->pos--;
			return 0;
		case ']':
			if ( *ctx->pos == ']' ){
				ctx->pos++;
				if ( *ctx->pos == '>' ){
					ctx->len = 2;
					return XML_CdataEndInCharData;
				}
			}
			break;
		default:
			if ( !IsXMLChar( ch ) )
				return XML_InvalidChar;
		}
	}
	return 0;
}

static const char xmlerr[] = "XML";
static DaoType *daox_type_xmlelem = NULL;
static DaoType *daox_type_xmlcdata = NULL;
static DaoType *daox_type_xmlinst = NULL;
static DaoType *daox_type_xmldoc = NULL;
static DaoType *daox_type_xmlwriter = NULL;
static DaoType *daox_type_xmlmod = NULL;
static DMutex xmlmtx;

void DaoXMLNode_Init( DaoXMLNode *self, xml_item kind )
{
	self->kind = kind;
	self->refs = 1;
	self->parent = NULL;
}

DaoXMLInstruction* DaoXMLInstruction_New()
{
	DaoXMLInstruction *res = (DaoXMLInstruction*)dao_malloc( sizeof(DaoXMLInstruction) );
	DaoXMLNode_Init( (DaoXMLNode*)res, XMLInstruction );
	DString_Init( &res->name );
	DString_Init( &res->data );
	return res;
}

void DaoXMLInstruction_Delete( DaoXMLInstruction *self )
{
	DString_DeleteData( &self->name );
	DString_DeleteData( &self->data );
	dao_free( self );
}

DaoXMLCharData* DaoXMLCharData_New( xml_item kind )
{
	DaoXMLCharData *res = (DaoXMLCharData*)dao_malloc( sizeof(DaoXMLCharData) );
	DaoXMLNode_Init( (DaoXMLNode*)res, kind );
	DString_Init( &res->data );
	return res;
}

void DaoXMLCharData_Delete( DaoXMLCharData *self )
{
	DString_DeleteData( &self->data );
	dao_free( self );
}

DaoXMLElement* DaoXMLElement_New()
{
	DaoXMLElement *res = (DaoXMLElement*)dao_malloc( sizeof(DaoXMLElement) );
	DaoXMLNode_Init( (DaoXMLNode*)res, XMLElement );
	res->attribs = NULL;
	DString_Init( &res->tag );
	res->c.children = NULL;
	return res;
}

daoint NodesSize( DArray *nodes )
{
	return nodes? nodes->size : 0;
}

DaoXMLNode* GetNode( DArray *nodes, daoint i )
{
	if ( i < 0 )
		i += NodesSize( nodes );
	return ( NodesSize( nodes ) > i && i >= 0 )? (DaoXMLNode*)nodes->data.daoints[i] : NULL;
}

void AppendToNodes( DArray **nodes, DaoXMLNode *node )
{
	if ( !*nodes )
		*nodes = DArray_New( sizeof(daoint) );
	DArray_PushDaoInt( *nodes, (daoint)node );
}

int InsertToNodes( DArray *nodes, daoint i, DaoXMLNode *node )
{
	daoint *pos;
	pos = DArray_Insert( nodes, i, 1 );
	if ( pos ){
		*pos = (daoint)node;
		return 1;
	}
	else
		return 0;
}

void DaoXMLNode_Delete( DaoXMLNode *self );
void DaoXMLNode_DeleteAsChld( DaoXMLNode *self );

void EraseFromNodes( DArray *nodes, daoint i, daoint count )
{
	if ( nodes ){
		daoint j;
		for ( j = 0; j < count; j++ ){
			DaoXMLNode *node = GetNode( nodes, i + j );
			if ( node )
				DaoXMLNode_DeleteAsChld( node );
		}
		DArray_Erase( nodes, i, count );
	}
}

void DeleteNodes( DArray **nodes )
{
	DArray *vt = *nodes;
	if ( vt ){
		daoint i;
		for ( i = 0; i < NodesSize( vt ); i++ ){
			DaoXMLNode *node = (DaoXMLNode*)vt->data.daoints[i];
			DaoXMLNode_DeleteAsChld( node );
		}
		DArray_Delete( vt );
		*nodes = NULL;
	}
}

void DaoXMLElement_Delete( DaoXMLElement *self ){
	if ( self->kind == XMLTextElement )
		DString_Delete( self->c.text );
	else {
		DeleteNodes( &self->c.children );
		if ( self->refs ) /* rollback in case one of the children called DaoXMLNode_AcquireParent() */
			return;
	}
	if ( self->attribs )
		DMap_Delete( self->attribs );
	DString_DeleteData( &self->tag );
	dao_free( self );
}

void DaoXMLNode_Delete( DaoXMLNode *self )
{
	DMutex_Lock( &xmlmtx );
	self->refs--;
	DMutex_Unlock( &xmlmtx );
	if ( !self->refs ){
		switch ( self->kind ){
		case XMLElement:
		case XMLTextElement:
		case XMLEmptyElement:	DaoXMLElement_Delete( (DaoXMLElement*)self ); break;
		case XMLText:
		case XMLCdata:			DaoXMLCharData_Delete( (DaoXMLCharData*)self ); break;
		case XMLInstruction:	DaoXMLInstruction_Delete( (DaoXMLInstruction*)self ); break;
		}
	}
}

void DaoXMLNode_DeleteAsChld( DaoXMLNode *self )
{
	DMutex_Lock( &xmlmtx );
	self->refs--;
	self->parent = NULL;
	DMutex_Unlock( &xmlmtx );
	if ( !self->refs ){
		switch ( self->kind ){
		case XMLElement:
		case XMLTextElement:
		case XMLEmptyElement:	DaoXMLElement_Delete( (DaoXMLElement*)self ); break;
		case XMLText:
		case XMLCdata:			DaoXMLCharData_Delete( (DaoXMLCharData*)self ); break;
		case XMLInstruction:	DaoXMLInstruction_Delete( (DaoXMLInstruction*)self ); break;
		}
	}
}

DaoXMLElement* DaoXMLNode_AcquireParent( DaoXMLNode *self )
{
	DaoXMLElement *parent;
	DMutex_Lock( &xmlmtx );
	parent = self->parent;
	if ( self->parent )
		parent->refs++;
	DMutex_Unlock( &xmlmtx );
	return parent;
}

void DaoXMLNode_AddRef( DaoXMLNode *self )
{
	DMutex_Lock( &xmlmtx );
	self->refs++;
	DMutex_Unlock( &xmlmtx );
}

int DaoXMLNode_SetParent( DaoXMLNode *self, DaoXMLElement *parent, int inc_refs )
{
	int res = 1;
	DMutex_Lock( &xmlmtx );
	if ( self->parent )
		res = 0;
	else if ( self == (DaoXMLNode*)parent )
		res = -1;
	else {
		self->parent = parent;
		if ( inc_refs )
			self->refs++;
	}
	DMutex_Unlock( &xmlmtx );
	return res;
}

DaoXMLDocument* DaoXMLDocument_New()
{
	DaoXMLDocument *res = (DaoXMLDocument*)dao_malloc( sizeof(DaoXMLDocument) );
	res->version = DString_NewChars( "1.0" );
	res->standalone = 0;
	res->encoding = DString_New();
	res->doctype = DString_New();
	res->instructions = DArray_New( sizeof(daoint) );
	res->root = NULL;
	res->namepool = DMap_New( DAO_DATA_STRING, DAO_DATA_NULL );
	return res;
}

void DaoXMLDocument_Delete( DaoXMLDocument *self )
{
	DString_Delete( self->encoding );
	DString_Delete( self->doctype );
	DString_Delete( self->version );
	DeleteNodes( &self->instructions );
	if ( self->root )
		DaoXMLNode_Delete( (DaoXMLNode*)self->root );
	DMap_Delete( self->namepool );
	dao_free( self );
}

void DaoXMLElement_Normalize( DaoXMLElement *self )
{
	if ( self->kind == XMLTextElement ){
		DaoXMLCharData *cd = DaoXMLCharData_New( XMLText );
		DString_Assign( &cd->data, self->c.text );
		DString_Delete( self->c.text );
		self->c.children = NULL;
		AppendToNodes( &self->c.children, (DaoXMLNode*)cd );
		cd->parent = self;
		self->kind = XMLElement;
	}
	else if ( !self->c.children )
		self->c.children = DArray_New( sizeof(daoint) );
}

int DaoXMLElement_AddChild( DaoXMLElement *self, DaoXMLNode *child, int inc_refs )
{
	int res = DaoXMLNode_SetParent( child, self, inc_refs );
	if ( res != 1 )
		return res;
	if ( self->kind == XMLEmptyElement )
		self->kind = XMLElement;
	else
		DaoXMLElement_Normalize( self );
	AppendToNodes( &self->c.children, child );
	return 1;
}

int DaoXMLElement_InsertChild( DaoXMLElement *self, DaoXMLNode *child, int inc_refs, daoint index )
{
	int res = DaoXMLNode_SetParent( child, self, inc_refs );
	if ( res != 1 )
		return res;
	if ( self->kind == XMLEmptyElement )
		self->kind = XMLElement;
	else
		DaoXMLElement_Normalize( self );
	if ( index == -1 || ( index == 0 && NodesSize( self->c.children ) == 0 ) )
		AppendToNodes( &self->c.children, child );
	else if ( !self->c.children || !InsertToNodes( self->c.children, index, child ) )
		return -2;
	return 1;
}

xml_error DaoXMLElement_ParseContent( DaoXMLElement *self, XMLContext *ctx, DMap *namepool )
{
	DaoXMLInstruction *in;
	DaoXMLElement *el;
	xml_error res;
	int empty;
	while ( 1 ){
		size_t ch;
		PassXMLWhitespace( ctx );
		if ( ( ch = GetChar( ctx ) ) == '\0' )
			return XML_TagNotClosed;
		if ( ch == '<' ){
			switch ( GetChar( ctx ) ){
			/* invalid markup */
			case '\0':	return XML_InvalidSyntax;
			/* end tag */
			case '/':	return ParseXMLEndTag( ctx, self->tag.chars );
			/* processing instruction */
			case '?':
				in = DaoXMLInstruction_New();
				res = ParseXMLInstruction( ctx, &in->name, &in->data, namepool );
				if ( res ){
					DaoXMLInstruction_Delete( in );
					return res;
				}
				DaoXMLElement_AddChild( self, (DaoXMLNode*)in, 0 );
				break;
			case '!':
				/* comment */
				if ( *ctx->pos == '-' && *( ctx->pos + 1 ) == '-' ){
					ctx->pos += 2;
					res = ParseXMLComment( ctx, NULL );
					if ( res )
						return res;
				}
				/* cdata section */
				else if ( strncmp( ctx->pos, "[CDATA[", 7 ) == 0 ){
					DaoXMLCharData *cd = DaoXMLCharData_New( XMLCdata );
					ctx->pos += 7;
					res = ParseXMLCdata( ctx, &cd->data );
					if ( res ){
						DaoXMLCharData_Delete( cd );
						return res;
					}
					DaoXMLElement_AddChild( self, (DaoXMLNode*)cd, 0 );
				}
				else {
					ctx->len = 0;
					return XML_InvalidSyntax;
				}
				break;
			/* start tag */
			default:
				UngetChar( ctx );
				el = DaoXMLElement_New();
				res = ParseXMLStartTag( ctx, &el->tag, &empty, &el->attribs, namepool );
				if ( res ){
					DaoXMLElement_Delete( el );
					return res;
				}
				DaoXMLElement_AddChild( self, (DaoXMLNode*)el, 0 );
				if ( empty )
					el->kind = XMLEmptyElement;
				else {
					res = DaoXMLElement_ParseContent( el, ctx, namepool );
					if ( res )
						return res;
				}
			}
		}
		/* text */
		else {
			UngetChar( ctx );
			if ( self->kind == XMLElement && !self->c.children ){
				self->kind = XMLTextElement;
				self->c.text = DString_New();
				res = ParseXMLCharData( ctx, self->c.text );
				if ( res )
					return res;
			}
			else {
				DaoXMLCharData *cd = DaoXMLCharData_New( XMLText );
				res = ParseXMLCharData( ctx, &cd->data );
				if ( res ){
					DaoXMLCharData_Delete( cd );
					return res;
				}
				DaoXMLElement_AddChild( self, (DaoXMLNode*)cd, 0 );
			}
		}
	}
}

xml_error DaoXMLDocument_ParseXML( DaoXMLDocument *self, DString *xml, daoint *errpos )
{
	char *start;
	int empty, xmldecl = 0, dtddecl = 0, misc = 0, ws = 0;
	xml_error res;
	XMLContext ctx = {xml->chars, 0,};
	memset( &ctx.state, 0, sizeof(ctx.state) );
	start = ctx.pos;
	while ( 1 ){
		size_t ch;
		daoint count = PassXMLWhitespace( &ctx ) > 0;
		if ( !ws )
			ws = count > 0;
		if ( ( ch = GetChar( &ctx ) ) == '\0' ){
			res = self->root? 0 : XML_NoElement;
			goto Exit;
		}
		if ( ch != '<' ){
			res = XML_InvalidSyntax;
			goto Exit;
		}
		switch ( GetChar( &ctx ) ){
		/* processing instruction or XML header */
		case '?':
			if ( strncmp( ctx.pos, "xml", 3 ) == 0 && !IsXMLNameChar( *( ctx.pos + 3 ) ) ){
				if ( xmldecl || dtddecl || misc || ws ){
					res = XML_MisplacedDeclaration;
					goto Exit;
				}
				ctx.pos += 3;
				res = ParseXMLDeclaration( &ctx, self->version, self->encoding, &self->standalone );
				xmldecl = 1;
				if ( res )
					goto Exit;
			}
			else {
				DaoXMLInstruction *in = DaoXMLInstruction_New();
				misc = 1;
				res = ParseXMLInstruction( &ctx, &in->name, &in->data, self->namepool );
				if ( res ){
					DaoXMLInstruction_Delete( in );
					goto Exit;
				}
				AppendToNodes( &self->instructions, (DaoXMLNode*)in );
			}
			break;
		case '!':
			/* comment */
			if ( *ctx.pos == '-' && *( ctx.pos + 1 ) == '-' ){
				misc = 1;
				ctx.pos += 2;
				res = ParseXMLComment( &ctx, NULL );
				if ( res )
					goto Exit;
			}
			/* DTD section */
			else if ( strncmp( ctx.pos, "DOCTYPE", 7 ) == 0 ){
				size_t ch;
				ctx.pos += 7;
				switch ( ( ch = GetChar( &ctx ) ) ){
				case '\r':
				case '\n':
				case ' ':
				case '\t':
					break;
				default:
					res = XML_InvalidSyntax;
					goto Exit;
				}
				if ( dtddecl || self->root ){
					res = XML_MisplacedDoctype;
					ctx.pos -= 8;
					ctx.len = 0;
					goto Exit;
				}
				dtddecl = 1;
				DString_SetChars( self->doctype, "<!DOCTYPE" );
				DString_AppendChar( self->doctype, ch );
				res = ParseXMLDoctype( &ctx, self->doctype );
				if ( res )
					goto Exit;
			}
			else {
				ctx.len = 0;
				res = XML_InvalidSyntax;
				goto Exit;
			}
			break;
		/* invalid markup */
		case '/':
		case '\0':
			res = XML_InvalidSyntax;
			goto Exit;
		/* element */
		default:
			UngetChar( &ctx );
			if ( self->root ){
				res = XML_MultipleRootElements;
				goto Exit;
			}
			else {
				DaoXMLElement *el = DaoXMLElement_New();
				res = ParseXMLStartTag( &ctx, &el->tag, &empty, &el->attribs, self->namepool );
				if ( res ){
					DaoXMLElement_Delete( el );
					goto Exit;
				}
				self->root = el;
				if ( empty )
					el->kind = XMLEmptyElement;
				else {
					res = DaoXMLElement_ParseContent( el, &ctx, self->namepool );
					if ( res )
						goto Exit;
				}
			}
		}
	}
Exit:
	*errpos = res? UngetChar( &ctx ) - start : 0;
	return res;
}

int GetXMLLineNumber( char *str, daoint pos )
{
	daoint i;
	int line = 1;
	for ( i = 0; i < pos; i++ )
		switch ( str[i] ){
		case '\0':	break;
		case '\n':	line++; break;
		case '\r':
			if ( str[i + 1] != '\n' )
				line++;
			break;
		}
	return line;
}

int GetXMLColumnNumber( char *str, daoint pos )
{
	daoint i;
	if ( pos > strlen( str ) )
		return 0;
	for ( i = pos; i >= 0 && str[i] != '\n' && str[i] != '\r'; i-- );
	return pos - i;
}

#define ERRSIZE 200

void GetXMLErrorMessage( xml_error error, char *buf )
{
	switch ( error ){
	case XML_InvalidChar:				strcpy( buf, "Invalid character" ); break;
	case XML_InvalidRefChar:			strcpy( buf, "Reference to invalid character" ); break;
	case XML_CommentNotClosed:			strcpy( buf, "Comment section not closed" ); break;
	case XML_CdataNotClosed:			strcpy( buf, "CDATA section not closed" ); break;
	case XML_LiteralNotClosed:			strcpy( buf, "Literal not closed" ); break;
	case XML_InstructionNotClosed:		strcpy( buf, "Invalid processing instruction ending" ); break;
	case XML_HyphensInComment:			strcpy( buf, "'--' in comment" ); break;
	case XML_InvalidNameChar:			strcpy( buf, "Invalid name character" ); break;
	case XML_MisplacedLT:				strcpy( buf, "Misplaced '<'" ); break;
	case XML_InvalidReference:			strcpy( buf, "Invalid reference" ); break;
	case XML_UnknownEntity:				strcpy( buf, "Reference to unknown entity" ); break;
	case XML_InvalidTagEnding:			strcpy( buf, "Invalid tag ending" ); break;
	case XML_InvalidAttrDefinition:		strcpy( buf, "Invalid attribute definition" ); break;
	case XML_DuplicateAttrDefinition:	strcpy( buf, "Duplicate attribute definition" ); break;
	case XML_ReservedInstructionName:	strcpy( buf, "Instruction name is reserved" ); break;
	case XML_NoVersionInfo:				strcpy( buf, "XML declaration without version information" ); break;
	case XML_InvalidVersionInfo:		strcpy( buf, "Invalid version information" ); break;
	case XML_InvalidEncodingInfo:		strcpy( buf, "Invalid encoding information" ); break;
	case XML_InvalidDeclEnding:			strcpy( buf, "Invalid XML declaration ending" ); break;
	case XML_InvalidDeclAttributes:		strcpy( buf, "Invalid names or order of XML declaration attributes" ); break;
	case XML_InvalidStandaloneInfo:		strcpy( buf, "Invalid standalone parameter" ); break;
	case XML_CdataEndInCharData:		strcpy( buf, "']]>' in character data" ); break;
	case XML_NoElement:					strcpy( buf, "No element in XML document" ); break;
	case XML_InvalidSyntax:				strcpy( buf, "XML syntax error" ); break;
	case XML_MultipleRootElements:		strcpy( buf, "Multiple root elements" ); break;
	case XML_InvalidEndTag:				strcpy( buf, "Invalid end tag" ); break;
	case XML_DoctypeNotClosed:			strcpy( buf, "Invalid DOCTYPE section ending" ); break;
	case XML_MisplacedDoctype:			strcpy( buf, "Misplaced DOCTYPE section" ); break;
	case XML_MisplacedDeclaration:		strcpy( buf, "Misplaced XML declaration" ); break;
	case XML_TagNotClosed:				strcpy( buf, "Tag not closed" ); break;
	default:							strcpy( buf, "Unknown error" );
	}
}

static void DaoXMLDocument_FromString( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *doc = DaoXMLDocument_New();
	xml_error res;
	DString *xml = p[0]->xString.value;
	daoint errpos = 0;
	res = DaoXMLDocument_ParseXML( doc, xml, &errpos );
	if ( res ){
		char buf[ERRSIZE];
		int len = snprintf( buf, sizeof(buf), "XML processing error (line %i, column %i): ", GetXMLLineNumber( xml->chars, errpos ),
							GetXMLColumnNumber( xml->chars, errpos ) );
		GetXMLErrorMessage( res, buf + len );
		DaoProcess_RaiseError( proc, xmlerr, buf );
		DaoXMLDocument_Delete( doc );
	}
	else
		DaoProcess_PutCdata( proc, doc, daox_type_xmldoc );
}

static void DaoXMLDocument_GetVersion( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, self->version );
}

static void DaoXMLDocument_SetVersion( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DString *version = p[1]->xString.value;
	if ( version->size > 2 && strncmp( version->chars, "1.", 2 ) == 0 ){
		daoint i;
		for ( i = 2; i < version->size && isdigit( version->chars[i] ); i++ );
		if ( i < version->size )
			DaoProcess_RaiseError( proc, xmlerr, "Invalid version information" );
		else
			DString_Assign( self->version, version );
	}
	else
		DaoProcess_RaiseError( proc, xmlerr, "Invalid version information" );
}

static void DaoXMLDocument_GetEncoding( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, self->encoding );
}

static void DaoXMLDocument_SetEncoding( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DString *encoding = p[1]->xString.value;
	char *pos;
	pos = encoding->chars;
	if ( encoding->size && ( ( *pos >= 'A' && *pos <= 'Z' ) || ( *pos >= 'a' && *pos <= 'z' ) ) ){
		for ( pos++; *pos != '\0'; pos++ )
			if ( !( ( *pos >= 'A' && *pos <= 'Z' ) || ( *pos >= 'a' && *pos <= 'z' ) || isdigit( *pos ) || strchr( "._-", *pos ) ) )
				break;
		if ( *pos != '\0' )
			DaoProcess_RaiseError( proc, xmlerr, "Invalid encoding information" );
		else
			DString_Assign( self->encoding, encoding );
	}
	else
		DaoProcess_RaiseError( proc, xmlerr, "Invalid encoding information" );
}

static void DaoXMLDocument_GetStandalone( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutEnum( proc, self->standalone? "true" : "false" );
}

static void DaoXMLDocument_SetStandalone( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	self->standalone = p[1]->xEnum.value != 0;
}

static void DaoXMLDocument_GetDoctype( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, self->doctype );
}

static void DaoXMLDocument_SetDoctype( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DString *dtd = p[1]->xString.value;
	xml_error res;
	XMLContext ctx = {NULL, 0};
	memset( &ctx.state, 0, sizeof(ctx.state) );
	if ( !dtd->size ){
		DString_Clear( self->doctype );
		return;
	}
	ctx.pos = dtd->chars;
	if ( strncmp( ctx.pos, "<!DOCTYPE", 9 ) != 0 ){
		DaoProcess_RaiseError( proc, xmlerr, "Invalid DOCTYPE section" );
		return;
	}
	ctx.pos += 9;
	switch ( GetChar( &ctx ) ){
	case '\r':
	case '\n':
	case ' ':
	case '\t':
		break;
	default:
		DaoProcess_RaiseError( proc, xmlerr, "Invalid DOCTYPE section" );
		return;
	}
	res = ParseXMLDoctype( &ctx, NULL );
	if ( res ){
		char buf[ERRSIZE];
		int len = snprintf( buf, sizeof(buf), "XML processing error (line %i, column %i): ", GetXMLLineNumber( dtd->chars, UngetChar( &ctx )
																											   - dtd->chars ),
							GetXMLColumnNumber( dtd->chars, UngetChar( &ctx ) - dtd->chars ) );
		GetXMLErrorMessage( res, buf + len );
		DaoProcess_RaiseError( proc, xmlerr, buf );
	}
	else
		DString_Assign( self->doctype, dtd );
}

static void DaoXMLDocument_GetInstructions( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DaoList *ins = DaoProcess_PutList( proc );
	daoint i;
	for ( i = 0; i < NodesSize( self->instructions ); i++ ){
		DaoXMLNode *node = GetNode( self->instructions, i );
		DaoXMLNode_AddRef( node );
		DaoList_Append( ins, (DaoValue*)DaoProcess_NewCdata( proc, daox_type_xmlinst, node, 1 ));
	}
}

int AdoptChild( DaoProcess *proc, DaoXMLNode *node, DaoXMLElement *parent )
{
	int res = parent? DaoXMLElement_AddChild( parent, node, 1 ) : DaoXMLNode_SetParent( node, parent, 1 );
	if ( res == 0 ){
		DaoProcess_RaiseError( proc, xmlerr, "Item already has parent" );
		return 0;
	}
	if ( res == -1 ){
		DaoProcess_RaiseError( proc, xmlerr, "Trying to make element a child of itself" );
		return 0;
	}
	return 1;
}

int AdoptChildAt( DaoProcess *proc, DaoXMLNode *node, DaoXMLElement *parent, daoint index )
{
	int res = DaoXMLElement_InsertChild( parent, node, 1, index );
	if ( res == 0 ){
		DaoProcess_RaiseError( proc, xmlerr, "Item already has parent" );
		return 0;
	}
	if ( res == -1 ){
		DaoProcess_RaiseError( proc, xmlerr, "Trying to make element a child of itself" );
		return 0;
	}
	if ( res == -2 ){
		DaoProcess_RaiseError( proc, "Index::Range", "" );
		return 0;
	}
	return 1;
}

static void DaoXMLDocument_SetInstructions( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DaoList *ins = &p[1]->xList;
	daoint i;
	DeleteNodes( &self->instructions );
	for ( i = 0; i < DaoList_Size( ins ); i++ ){
		DaoXMLNode *node = (DaoXMLNode*)DaoValue_TryGetCdata( DaoList_GetItem( ins, i ) );
		if ( !AdoptChild( proc, node, NULL ) )
			return;
		AppendToNodes( &self->instructions, node );
	}
}

static void DaoXMLDocument_GetRoot( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DaoXMLNode_AddRef( (DaoXMLNode*)self->root );
	DaoProcess_PutCdata( proc, self->root, daox_type_xmlelem );
}

static void DaoXMLDocument_SetRoot( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DaoXMLNode_Delete( (DaoXMLNode*)self->root );
	self->root = (DaoXMLElement*)DaoValue_TryGetCdata( p[1] );
	DaoXMLNode_AddRef( (DaoXMLNode*)self->root );
}

static void DaoXMLDocument_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *root = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoXMLDocument *res = DaoXMLDocument_New();
	res->root = root;
	DaoXMLNode_AddRef( (DaoXMLNode*)root );
	DaoProcess_PutCdata( proc, res, daox_type_xmldoc );
}

daoint EscapeXMLMarkupChars(DString *str, DString *dest);

void AddIndent( int indent, DString *dest )
{
	daoint i;
	for ( i = 0; i < indent; i++ )
		DString_AppendChars( dest, "\t" );
}

void DaoXMLCharData_Serialize( DaoXMLCharData *self, int indent, DString *dest )
{
	AddIndent( indent, dest );
	if ( self->kind == XMLText )
		EscapeXMLMarkupChars( &self->data, dest );
	else {
		DString_AppendChars( dest, "<![CDATA[" );
		DString_Append( dest, &self->data );
		DString_AppendChars( dest, "]]>" );
	}
	if ( indent && ( self->kind != XMLText || ( self->data.size && self->data.chars[self->data.size - 1] != '\n' ) ) )
		DString_AppendChars( dest, "\n" );
}

void DaoXMLInstruction_Serialize( DaoXMLInstruction *self, int indent, DString *dest )
{
	AddIndent( indent, dest );
	DString_AppendChars( dest, "<?" );
	DString_Append( dest, &self->name );
	DString_AppendChars( dest, " " );
	DString_Append( dest, &self->data );
	DString_AppendChars( dest, "?>\n" );
}

void DaoXMLElement_Serialize( DaoXMLElement *self, int indent, DString *dest )
{
	AddIndent( indent, dest );
	DString_AppendChars( dest, "<" );
	DString_Append( dest, &self->tag );
	if ( self->attribs ){
		DNode *node;
		for ( node = DMap_First( self->attribs ); node; node = DMap_Next( self->attribs, node ) ){
			DString_AppendChars( dest, " " );
			DString_Append( dest, node->key.pString );
			DString_AppendChars( dest, "=\"" );
			EscapeXMLMarkupChars( node->value.pString, dest );
			DString_AppendChars( dest, "\"" );
		}
	}
	if ( self->kind == XMLEmptyElement )
		DString_AppendChars( dest, "/>" );
	else {
		DString_AppendChars( dest, ">" );
		if ( self->kind == XMLTextElement )
			EscapeXMLMarkupChars( self->c.text, dest );
		else if ( NodesSize( self->c.children ) == 1 && ( GetNode( self->c.children, 0 )->kind == XMLText ||
														  GetNode( self->c.children, 0 )->kind == XMLCdata  ) )
			DaoXMLCharData_Serialize( (DaoXMLCharData*)GetNode( self->c.children, 0 ), 0, dest );
		else {
			daoint i;
			if ( NodesSize( self->c.children ) )
				DString_AppendChars( dest, "\n" );
			for ( i = 0; i < NodesSize( self->c.children ); i++ ){
				DaoXMLNode *node = GetNode( self->c.children, i );
				if ( node->kind == XMLInstruction )
					DaoXMLInstruction_Serialize( (DaoXMLInstruction*)node, indent + 1, dest );
				else if ( node->kind == XMLText || node->kind == XMLCdata )
					DaoXMLCharData_Serialize( (DaoXMLCharData*)node, indent + 1, dest );
				else
					DaoXMLElement_Serialize( (DaoXMLElement*)node, indent + 1, dest );
			}
			if ( NodesSize( self->c.children ) )
				AddIndent( indent, dest );
		}
		DString_AppendChars( dest, "</" );
		DString_Append( dest, &self->tag );
		DString_AppendChars( dest, ">" );
	}
	DString_AppendChars( dest, "\n" );
}

static void DaoXMLDocument_Serialize( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLDocument *self = (DaoXMLDocument*)DaoValue_TryGetCdata( p[0] );
	DString *xml = DaoProcess_PutChars( proc, "" );
	daoint i;
	DString_AppendChars( xml, "<?xml version=\"" );
	DString_Append( xml, self->version );
	DString_AppendChars( xml, "\"" );
	if ( self->encoding->size ){
		DString_AppendChars( xml, " encoding=\"" );
		DString_Append( xml, self->encoding );
		DString_AppendChars( xml, "\"" );
	}
	if ( self->standalone )
		DString_AppendChars( xml, " standalone=\"yes\"" );
	DString_AppendChars( xml, "?>\n" );
	if ( self->doctype->size ){
		DString_Append( xml, self->doctype );
		DString_AppendChars( xml, "\n" );
	}
	for ( i = 0; i < NodesSize( self->instructions ); i++ )
		DaoXMLInstruction_Serialize( (DaoXMLInstruction*)GetNode( self->instructions, i ), 0, xml );
	DaoXMLElement_Serialize( self->root, 0, xml );
}

static DaoFuncItem xmlDocMeths[] =
{
	/*! Constructs new XML document with \a root as its root element */
	{ DaoXMLDocument_Create,			"document(invar root: element) => document" },

	/*! XML version */
	{ DaoXMLDocument_GetVersion,		".version(invar self: document) => string" },
	{ DaoXMLDocument_SetVersion,		".version=(self: document, value: string)" },

	/*! Encoding
	 * \note Specifying encoding has no effect on actual encoding of resulting XML document */
	{ DaoXMLDocument_GetEncoding,		".encoding(invar self: document) => string" },
	{ DaoXMLDocument_SetEncoding,		".encoding=(self: document, value: string)" },

	/*! Standalone document parameter */
	{ DaoXMLDocument_GetStandalone,		".standalone(self: document) => bool" },
	{ DaoXMLDocument_SetStandalone,		".standalone=(self: document, value: bool)" },

	/*! Internal DTD section ('<!DOCTYPE ... >')
	 * \note DTD is not interpreted and thus has no effect on treatment of elements and attributes */
	{ DaoXMLDocument_GetDoctype,		".doctype(invar self: document) => string" },
	{ DaoXMLDocument_SetDoctype,		".doctype=(self: document, value: string)" },

	/*! Processing instructions outside of root element (placed before root element during serialization)
	 * \note The list contains *references* to instructions; however, modifying the list itself has no effect on the document */
	{ DaoXMLDocument_GetInstructions,	".instructions(invar self: document) => list<instruction>" },
	{ DaoXMLDocument_SetInstructions,	".instructions=(self: document, value: list<instruction>)" },

	/*! Root element */
	{ DaoXMLDocument_GetRoot,			".root(invar self: document) => element" },
	{ DaoXMLDocument_SetRoot,			".root=(self: document, value: element)" },

	/*! Returns XML representation of the whole document */
	{ DaoXMLDocument_Serialize,			"toXML(invar self: document) => string" },
	{ NULL, NULL }
};

/*! XML document */
DaoTypeBase xmlDocTyper = {
	"document", NULL, NULL, xmlDocMeths, {NULL}, {0},
	(FuncPtrDel)DaoXMLDocument_Delete, NULL
};

void RaiseValidationError( DaoProcess *proc, xml_error error, daoint pos )
{
	char buf[100];
	GetXMLErrorMessage( error, buf );
	snprintf( buf + strlen( buf ), sizeof(buf) - strlen( buf ), " at index %" DAO_INT_FORMAT, pos );
	DaoProcess_RaiseError( proc, xmlerr, buf );
}

int ValidateName( DaoProcess *proc, char *name )
{
	XMLContext ctx = {name, 0};
	xml_error res = ParseXMLName( &ctx, NULL, NULL );
	memset( &ctx.state, 0, sizeof(ctx.state) );
	if ( !res && *ctx.pos != '\0' )
		res = XML_InvalidNameChar;
	if ( res ){
		RaiseValidationError( proc, res, UngetChar( &ctx ) - name );
		return 0;
	}
	return 1;
}

daoint FindInvalidXMLChar( DString *str );

int ValidateCharData( DaoProcess *proc, DString *str )
{
	daoint pos = FindInvalidXMLChar( str );
	if ( pos >= 0 ){
		RaiseValidationError( proc, XML_InvalidChar, pos );
		return 0;
	}
	return 1;
}

static void DaoXMLInstruction_GetName( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLInstruction *self = (DaoXMLInstruction*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, &self->name );
}

static void DaoXMLInstruction_SetName( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLInstruction *self = (DaoXMLInstruction*)DaoValue_TryGetCdata( p[0] );
	DString *name = p[1]->xString.value;
	if ( name->size ){
		if ( ValidateName( proc, name->chars ) ){
			if ( strcmp( name->chars, "xml" ) == 0 ){
				char buf[100];
				GetXMLErrorMessage( XML_ReservedInstructionName, buf );
				DaoProcess_RaiseError( proc, xmlerr, buf );
			}
			else
				DString_Assign( &self->name, name );
		}
	}
	else
		DaoProcess_RaiseError( proc, xmlerr, "Empty instruction name" );
}

static void DaoXMLInstruction_GetData( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLInstruction *self = (DaoXMLInstruction*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, &self->data);
}

static void DaoXMLInstruction_SetData( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLInstruction *self = (DaoXMLInstruction*)DaoValue_TryGetCdata( p[0] );
	DString *data = p[1]->xString.value;
	if ( ValidateCharData( proc, data ) )
		DString_Assign( &self->data, data );
}

static void DaoXMLInstruction_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLInstruction *res = DaoXMLInstruction_New();
	DString_Assign( &res->name, p[0]->xString.value );
	if ( !res->name.size ){
		DaoProcess_RaiseError( proc, xmlerr, "Empty instruction name" );
		DaoXMLInstruction_Delete( res );
		return;
	}
	if ( ValidateName( proc, res->name.chars ) ){
		if ( strcmp( res->name.chars, "xml" ) == 0 ){
			char buf[100];
			GetXMLErrorMessage( XML_ReservedInstructionName, buf );
			DaoProcess_RaiseError( proc, xmlerr, buf );
			DaoXMLInstruction_Delete( res );
		}
		else {
			DString_Assign( &res->data, p[1]->xString.value );
			if ( ValidateCharData( proc, &res->data ) )
				DaoProcess_PutCdata( proc, res, daox_type_xmlinst );
			else
				DaoXMLInstruction_Delete( res );
		}
	}
	else
		DaoXMLInstruction_Delete( res );
}

static DaoFuncItem xmlInstMeths[] =
{
	/*! Constructs XML processing instruction given its \a name and \a data */
	{ DaoXMLInstruction_Create,		"instruction(name: string, data = '') => instruction" },

	/*! Instruction target */
	{ DaoXMLInstruction_GetName,	".target(invar self: instruction) => string" },
	{ DaoXMLInstruction_SetName,	".target=(self: instruction, value: string)" },

	/*! Instruction data */
	{ DaoXMLInstruction_GetData,	".data(invar self: instruction) => string" },
	{ DaoXMLInstruction_SetData,	".data=(self: instruction, value: string)" },
	{ NULL, NULL }
};

/*! XML processing instruction */
DaoTypeBase xmlInstTyper = {
	"instruction", NULL, NULL, xmlInstMeths, {NULL}, {0},
	(FuncPtrDel)DaoXMLNode_Delete, NULL
};

static void DaoXMLElement_GetTag( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, &self->tag );
}

static void DaoXMLElement_SetTag( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DString *tag = p[1]->xString.value;
	if ( !tag->size ){
		DaoProcess_RaiseError( proc, xmlerr, "Empty tag" );
		return;
	}
	if ( ValidateName( proc, tag->chars ) )
		DString_Assign( &self->tag, tag );
}

static void DaoXMLElement_GetAttr( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DNode *node;
	DString *attr = p[1]->xString.value;
	int success = 0;
	if ( self->attribs ){
		node = DMap_Find( self->attribs, attr );
		if ( node ){
			DaoProcess_PutString( proc, node->value.pString );
			success = 1;
		}
	}
	if ( !success ) {
		char buf[ERRSIZE];
		snprintf( buf, sizeof(buf), "Element has no attribute '%s'", attr->chars );
		DaoProcess_RaiseError( proc, xmlerr, buf );
	}
}

static void DaoXMLElement_SetAttr( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DString *name = p[2]->xString.value;
	if ( !name->size )
		DaoProcess_RaiseError( proc, xmlerr, "Empty attribute name" );
	else if ( ValidateName( proc, name->chars ) ){
		if ( p[1]->type == DAO_NONE ){
			if ( self->attribs )
				DMap_Erase( self->attribs, name );
		}
		else {
			DString *val = p[1]->xString.value;
			if ( !self->attribs )
				self->attribs = DMap_New( DAO_DATA_STRING, DAO_DATA_STRING );
			DMap_Insert( self->attribs, name, val );
		}
	}
}

static void DaoXMLElement_HasAttr( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DString *attr = p[1]->xString.value;
	if ( self->attribs )
		DaoProcess_PutEnum( proc, DMap_Find( self->attribs, attr )? "true" : "false" );
	else
		DaoProcess_PutEnum( proc, "false" );
}

static void DaoXMLElement_GetText( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DString *str = NULL;
	if ( self->kind == XMLTextElement )
		str = self->c.text;
	else if ( self->kind != XMLEmptyElement ) {
		DArray *children = self->c.children;
		if ( NodesSize( children ) != 0 ){
			if ( NodesSize( children ) == 1 ){
				DaoXMLNode *node = GetNode( self->c.children, 0 );
				if ( node->kind == XMLText || node->kind == XMLCdata )
					str = &( (DaoXMLCharData*)node )->data;
			}
		}
		else {
			DaoProcess_PutChars( proc, "" );
			return;
		}
	}
	if ( str )
		DaoProcess_PutString( proc, str );
	else
		DaoProcess_RaiseError( proc, xmlerr, "Element content is not a single chardata item" );
}

static void DaoXMLElement_SetText( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DString *text = p[1]->xString.value;
	if ( ValidateCharData( proc, text ) ){
		if ( self->kind == XMLTextElement )
			DString_Assign( self->c.text, p[1]->xString.value );
		else {
			DeleteNodes( &self->c.children );
			self->kind = XMLTextElement;
			self->c.text = DString_Copy( p[1]->xString.value );
		}
	}
}

static void DaoXMLElement_Size( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->kind == XMLTextElement? 1 : NodesSize( self->c.children ) );
}

static void DaoXMLElement_GetEmpty( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutEnum( proc, self->kind == XMLEmptyElement? "true" : "false" );
}

static void DaoXMLElement_SetEmpty( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	daoint empty = p[1]->xEnum.value == 1;
	if ( empty ){
		if ( self->kind == XMLTextElement )
			DString_Delete( self->c.text );
		else
			DeleteNodes( &self->c.children );
		self->kind = XMLEmptyElement;
	}
	else if ( self->kind == XMLEmptyElement )
		self->kind = XMLElement;
}

static void DaoXMLElement_Clear( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	if ( self->attribs )
		DMap_Clear( self->attribs );
	if ( self->kind == XMLTextElement )
		DString_Clear( self->c.text );
	else
		DeleteNodes( &self->c.children );
}

static void DaoXMLElement_GetChildren( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoList *lst = DaoProcess_PutList( proc );
	if ( self->kind == XMLTextElement ){
		DaoXMLCharData *cd = DaoXMLCharData_New( XMLText );
		DString_Assign( &cd->data, self->c.text );
		DaoList_Append( lst, (DaoValue*)DaoProcess_NewCdata( proc, daox_type_xmlcdata, cd, 1 ) );
	}
	else {
		daoint i;
		for ( i = 0; i < NodesSize( self->c.children ); i++ ){
			DaoType *type = daox_type_xmlelem;
			DaoXMLNode *node = GetNode( self->c.children, i );
			if ( node->kind == XMLCdata || node->kind == XMLText )
				type = daox_type_xmlcdata;
			else if ( node->kind == XMLInstruction )
				type = daox_type_xmlinst;
			DaoXMLNode_AddRef( node );
			DaoList_Append( lst, (DaoValue*)DaoProcess_NewCdata( proc, type, node, 1 ) );
		}
	}
}

static void DaoXMLElement_SetChildren( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoList *lst = &p[1]->xList;
	daoint i;
	if ( self->kind == XMLTextElement )
		DString_Clear( self->c.text );
	else
		DeleteNodes( &self->c.children );
	self->kind = XMLElement;
	for ( i = 0; i < DaoList_Size( lst ); i++ ){
		DaoXMLNode *node = (DaoXMLNode*)DaoValue_TryGetCdata( DaoList_GetItem( lst, i ) );
		if ( !AdoptChild( proc, node, self ) )
			return;
	}
}

int CheckAttrValue( DaoProcess *proc, DString *attr, DString *value )
{
	daoint pos = FindInvalidXMLChar( value );
	if ( pos >= 0 ){
		char buf[200];
		GetXMLErrorMessage( XML_InvalidChar, buf );
		snprintf( buf + strlen( buf ), sizeof(buf) - strlen( buf ), " at index %" DAO_INT_FORMAT " in the value of attribute '%s'",
				  pos, attr->chars );
		DaoProcess_RaiseError( proc, xmlerr, buf );
		return 0;
	}
	return 1;
}

DString* DaoTuple_GetParamName( DaoTuple *self )
{
	DString *s = DString_New();
	DaoEnum_MakeName( &self->values[0]->xEnum, s );
	DString_Erase( s, 0, 1 );
	return s;
}

static void DaoXMLElement_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *res = DaoXMLElement_New();
	DString_Assign( &res->tag, p[0]->xString.value );
	if ( !res->tag.size ){
		DaoProcess_RaiseError( proc, xmlerr, "Empty tag" );
		DaoXMLElement_Delete( res );
		return;
	}
	if ( res->tag.chars[res->tag.size - 1] == '/' ){
		DString_Erase( &res->tag, res->tag.size - 1, 1 );
		res->kind = XMLEmptyElement;
	}
	if ( ValidateName( proc, res->tag.chars ) ){
		int i;
		for ( i = 1; i < N; i++ ){
			DaoTuple *tup = &p[i]->xTuple;
			DString *attr = DaoTuple_GetParamName( tup );
			DString *val = DString_Copy( tup->values[1]->xString.value );
			if ( !CheckAttrValue( proc, attr, val ) ){
				DaoXMLElement_Delete( res );
				return;
			}
			if ( !res->attribs )
				res->attribs = DMap_New( DAO_DATA_STRING, DAO_DATA_STRING );
			DMap_Insert( res->attribs, attr, val );
			DString_Delete( attr );
			DString_Delete( val );
			}
		DaoProcess_PutCdata( proc, res, daox_type_xmlelem );
	}
	else
		DaoXMLElement_Delete( res );
}

static void DaoXMLElement_Append( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoXMLNode *node = (DaoXMLNode*)DaoValue_TryGetCdata( p[1] );
	if ( !AdoptChild( proc, node, self ) )
		return;
}

static void DaoXMLElement_Insert( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoXMLNode *item = (DaoXMLNode*)DaoValue_TryGetCdata( p[1] );
	daoint index = p[2]->xInteger.value;
	DaoXMLElement_Normalize( self );
	if ( !AdoptChildAt( proc, item, self, index ) )
		return;
}

static void DaoXMLElement_Drop( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoXMLElement_Normalize( self );
	if ( p[1]->type == DAO_INTEGER ){
		daoint index = p[1]->xInteger.value;
		daoint count = p[2]->xInteger.value;
		if ( index < 0 )
			index += NodesSize( self->c.children );
		if ( index < 0 || index >= NodesSize( self->c.children ) )
			DaoProcess_RaiseError( proc, "Index::Range", "" );
		else if ( count < 0 )
			DaoProcess_RaiseError( proc, "Param", "Invalid number of elements" );
		else
			EraseFromNodes( self->c.children, index, count );
	}
	else {
		DaoXMLNode *child = (DaoXMLNode*)DaoValue_TryGetCdata( p[1] );
		daoint i;
		for ( i = 0; i < NodesSize( self->c.children ); i++ ){
			DaoXMLNode *node = GetNode( self->c.children, i );
			if ( node == child ){
				EraseFromNodes( self->c.children, i, 1 );
				break;
			}
		}
	}
}

static void DaoXMLElement_Attributes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoMap *map = DaoProcess_PutMap( proc, 0 );
	if ( self->attribs ){
		DNode *node = DMap_First( self->attribs );
		for ( ; node; node = DMap_Next( self->attribs, node ) ){
			DaoString *key = DaoString_New();
			DaoString *val = DaoString_New();
			DaoString_Set( key, node->key.pString );
			DaoString_Set( val, node->value.pString );
			DaoMap_Insert( map, (DaoValue*)key, (DaoValue*)val );
		}
	}
}

void RaiseAttrConvError( DaoProcess *proc, DString *attr, uchar_t type )
{
	char buf[200];
	snprintf( buf, sizeof(buf) - 6, "Failed to convert attribute '%s' to ", attr->chars );
	switch ( type ){
	case DAO_INTEGER:	strcat( buf, "int" ); break;
	case DAO_FLOAT:		strcat( buf, "float" ); break;
	case DAO_DOUBLE:	strcat( buf, "double" ); break;
	case DAO_STRING:	strcat( buf, "string" ); break;
	case DAO_ENUM:		strcat( buf, "enum" ); break;
	default:			strcat( buf, "?" ); break;
	}
	DaoProcess_RaiseError( proc, "Conversion", buf );
}

static void DaoXMLElement_MapAttribs( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	if ( !self->attribs || !self->attribs->size )
		DaoProcess_RaiseError( proc, xmlerr, "Element has no attributes" );
	else {
		DaoTuple *tup = DaoProcess_PutTuple( proc, p[1]->xType.nested->size );
		int i;
		for ( i = 0; i < tup->size; i++ ){
			DaoType *stp = tup->ctype->nested->items.pType[i];
			DString *key = DString_New();
			DNode *node;
			if ( tup->values[i] == NULL ){
				char buf[200];
				snprintf( buf, sizeof(buf), "Unsupported type of tuple item: %s", stp->name->chars );
				DaoProcess_RaiseError( proc, "Type", buf );
				goto Error;
			}
			if ( stp->tid != DAO_PAR_NAMED ){
				DaoProcess_RaiseError( proc, "Type", "Tuple type contains unnamed items" );
				goto Error;
			}
			DString_Assign( key, stp->fname );
			node = DMap_Find( self->attribs, key );
			if ( node ){
				DString *val = node->value.pString;
				char *end = NULL;
				uchar_t type = tup->values[i]->type;
				switch ( type ){
				case DAO_INTEGER:
					tup->values[i]->xInteger.value = strtoll( val->chars, &end, 0 );
					if( *end != '\0' ){
						RaiseAttrConvError( proc, key, type );
						goto Error;
					}
					break;
				case DAO_FLOAT:
					tup->values[i]->xFloat.value = strtod( val->chars, &end );
					if( *end != '\0' ){
						RaiseAttrConvError( proc, key, type );
						goto Error;
					}
					break;
				case DAO_DOUBLE:
					tup->values[i]->xDouble.value = strtod( val->chars, &end );
					if( *end != '\0' ){
						RaiseAttrConvError( proc, key, type );
						goto Error;
					}
					break;
				case DAO_STRING:
					DString_Assign( tup->values[i]->xString.value, val );
					break;
				case DAO_ENUM:
					val = DString_Copy( val );
					if ( !DaoEnum_SetSymbols( &tup->values[i]->xEnum, val->chars ) ){
						RaiseAttrConvError( proc, key, type );
						DString_Delete( key );
						goto Error;
					}
					DString_Delete( val );
					break;
				default:
					if ( 1 ){
						char buf[200];
						snprintf( buf, sizeof(buf), "Unsupported type for attribute '%s'", key->chars );
						DaoProcess_RaiseError( proc, "Type", buf );
						goto Error;
					}
				}
			}
			else {
				char buf[200];
				snprintf( buf, sizeof(buf), "Element has no attribute '%s'", key->chars );
				DaoProcess_RaiseError( proc, xmlerr, buf );
				goto Error;
			}
			DString_Delete( key );
			continue;
		Error:
			DString_Delete( key );
			break;
		}
	}
}

static void DaoXMLElement_GetElems( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoList *lst = DaoProcess_PutList( proc );
	if ( self->kind == XMLElement ) {
		daoint i;
		for ( i = 0; i < NodesSize( self->c.children ); i++ ){
			DaoXMLNode *node = GetNode( self->c.children, i );
			if ( node->kind == XMLElement || node->kind == XMLEmptyElement || node->kind == XMLTextElement ){
				DaoXMLNode_AddRef( node );
				DaoList_Append( lst, (DaoValue*)DaoProcess_NewCdata( proc, daox_type_xmlelem, node, 1 ) );
			}
		}
	}
}

static void DaoXMLElement_GetChild( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	daoint at = p[1]->xInteger.value;
	DaoXMLNode *node;
	DaoType *type = daox_type_xmlelem;
	DaoXMLElement_Normalize( self );
	node = GetNode( self->c.children, at );
	if ( node ){
		if ( node->kind == XMLText || node->kind == XMLCdata )
			type = daox_type_xmlcdata;
		else if ( node->kind == XMLInstruction )
			type = daox_type_xmlinst;
		DaoXMLNode_AddRef( node );
		DaoProcess_PutCdata( proc, node, type );
	}
	else
		DaoProcess_RaiseError( proc, "Index::Range", "" );
}

int MatchElement( DaoXMLNode *node, DString *tag, DaoValue *attribs[], int attr_count )
{
	if ( node->kind == XMLElement || node->kind == XMLEmptyElement || node->kind == XMLTextElement ){
		DaoXMLElement *el = (DaoXMLElement*)node;
		if ( !tag->size || DString_EQ( tag, &el->tag ) ){
			int match = 1;
			int i;
			for ( i = 0; i < attr_count; i++ ){
				DaoTuple *tup = &attribs[i]->xTuple;
				DString *attr = DaoTuple_GetParamName( tup );
				DString *val = DString_Copy( tup->values[1]->xString.value );
				DNode *node;
				node = el->attribs? DMap_Find( el->attribs, attr ) : NULL;
				if ( !node || !DString_EQ( val, node->value.pString ) )
					match = 0;
				DString_Delete( attr );
				DString_Delete( val );
				if ( !match )
					return 0;
			}
		}
		else
			return 0;
		return 1;
	}
	return 0;
}

DaoXMLElement* ResolvePath( DaoXMLElement *el, DString *path, DString *endtag )
{
	daoint prev, pos = -1;
	DString_Clear( endtag );
	while ( 1 ){
		prev = pos;
		pos = DString_FindChar( path, '/', pos + 1 );
		if ( pos >= 0 ){
			daoint i;
			DString *tag = DString_New();
			int found = 0;
			DString_SubString( path, tag, prev + 1, pos - prev - 1 );
			for ( i = 0; i < NodesSize( el->c.children ); i++ ){
				DaoXMLNode *node = GetNode( el->c.children, i );
				if ( MatchElement( node, tag, NULL, 0 ) ){
					el = (DaoXMLElement*)node;
					found = 1;
					break;
				}
			}
			DString_Delete( tag );
			if ( !found )
				return NULL;
		}
		else
			break;
	}
	DString_SubString( path, endtag, prev + 1, path->size - prev - 1 );
	return el;
}

static void DaoXMLElement_FindElem( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DString *path = p[1]->xString.value;
	daoint i;
	if ( self->kind == XMLElement ){
		daoint i;
		DString *tag = DString_New();
		DaoXMLElement *el = ResolvePath( self, path, tag );
		if ( el && el->kind == XMLElement ){
			for ( i = 0; i < NodesSize( el->c.children ); i++ ){
				DaoXMLNode *node = GetNode( el->c.children, i );
				if ( MatchElement( node, tag, p + 2, N - 2 ) ){
					DaoXMLNode_AddRef( node );
					DaoProcess_PutCdata( proc, node, daox_type_xmlelem );
					DString_Delete( tag );
					return;
				}
			}
		}
		DString_Delete( tag );
	}
	DaoProcess_PutNone( proc );
}

static void DaoXMLElement_FindElems( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DString *path = p[1]->xString.value;
	daoint i;
	DaoList *lst = DaoProcess_PutList( proc );
	for ( i = 2; i < N; i++ )
		if ( p[i]->type != DAO_PAR_NAMED ){
			DaoProcess_RaiseError( proc, "Param", "Additional method parameters must be named" );
			return;
		}
	if ( self->kind == XMLElement ){
		daoint i;
		DString *tag = DString_New();
		DaoXMLElement *el = ResolvePath( self, path, tag );
		if ( el && el->kind == XMLElement  ){
			for ( i = 0; i < NodesSize( el->c.children ); i++ ){
				DaoXMLNode *node = GetNode( el->c.children, i );
				if ( MatchElement( node, tag, p + 2, N - 2 ) ){
					DaoXMLNode_AddRef( node );
					DaoList_PushBack( lst, (DaoValue*)DaoProcess_NewCdata( proc, daox_type_xmlelem, node, 1 ) );
				}
			}
		}
		DString_Delete( tag );
	}
}

void RaiseElemConvError( DaoProcess *proc, DString *path, DString *tag, uchar_t type )
{
	char buf[400];
	snprintf( buf, sizeof(buf) - 6, "Failed to convert element '%s.%s' to ", path->chars, tag->chars );
	switch ( type ){
	case DAO_INTEGER:	strcat( buf, "int" ); break;
	case DAO_FLOAT:		strcat( buf, "float" ); break;
	case DAO_DOUBLE:	strcat( buf, "double" ); break;
	case DAO_STRING:	strcat( buf, "string" ); break;
	case DAO_ENUM:		strcat( buf, "enum" ); break;
	case DAO_TUPLE:		strcat( buf, "tuple" ); break;
	default:			strcat( buf, "?" ); break;
	}
	DaoProcess_RaiseError( proc, "Conversion", buf );
}

int GetElementData( DaoProcess *proc, DaoXMLElement *el, DString *path, DString *data )
{
	DString_Clear( data );
	if ( el->kind == XMLTextElement )
		DString_Assign( data, el->c.text );
	else if ( el->kind == XMLEmptyElement ){
		char buf[400];
		snprintf( buf, sizeof(buf), "Element is empty: %s.%s", path->chars, el->tag.chars );
		DaoProcess_RaiseError( proc, xmlerr, buf );
		return 0;
	}
	else if ( NodesSize( el->c.children ) != 0 ){
		if ( NodesSize( el->c.children ) > 1 || ( GetNode( el->c.children, 0 )->kind != XMLText &&
												  GetNode( el->c.children, 0 )->kind != XMLCdata ) ){
			char buf[400];
			snprintf( buf, sizeof(buf), "Element content is not a single chardata item: %s.%s", path->chars, el->tag.chars );
			DaoProcess_RaiseError( proc, xmlerr, buf );
			return 0;
		}
		DString_Assign( data, &( (DaoXMLCharData*)GetNode( el->c.children, 0 ) )->data );
	}
	return 1;
}

int MapContent( DaoProcess *proc, DaoXMLElement *el, DaoTuple *tup, DString *path )
{
	int i;
	if ( path->size )
		DString_AppendChar( path, '.' );
	DString_Append( path, &el->tag );
	if ( el->kind != XMLElement || NodesSize( el->c.children ) == 0 ){
		char buf[400];
		snprintf( buf, sizeof(buf), "Element does not contain sub-elements: %s", path->chars );
		DaoProcess_RaiseError( proc, xmlerr, buf );
		return 0;
	}
	for ( i = 0; i < tup->size; i++ ){
		daoint j;
		DaoType *stp = tup->ctype->nested->items.pType[i];
		DString *tag, *data;
		if ( stp->tid != DAO_PAR_NAMED ){
			DaoProcess_RaiseError( proc, "Type", "Tuple type contains unnamed items" );
			return 0;
		}
		tag = DString_Copy( stp->fname );
		data = DString_New();
		for ( j = 0; j < NodesSize( el->c.children ); j++ ){
			DaoXMLNode *node = GetNode( el->c.children, j );
			if ( node->kind == XMLElement || node->kind == XMLEmptyElement || node->kind == XMLTextElement ){
				DaoXMLElement *elem = (DaoXMLElement*)node;
				if ( DString_EQ( &elem->tag, tag ) ){
					char *end = NULL;
					uchar_t tp = tup->values[i]->type;
					if ( tp != DAO_TUPLE && !GetElementData( proc, elem, path, data ))
						return 0;
					switch ( tp ){
					case DAO_INTEGER:
						tup->values[i]->xInteger.value = strtoll( data->chars, &end, 0 );
						if( *end != '\0' ){
							RaiseElemConvError( proc, path, tag, tp );
							goto Error;
						}
						break;
					case DAO_FLOAT:
						tup->values[i]->xFloat.value = strtod( data->chars, &end );
						if( *end != '\0' ){
							RaiseElemConvError( proc, path, tag, tp );
							goto Error;
						}
						break;
					case DAO_DOUBLE:
						tup->values[i]->xDouble.value = strtod( data->chars, &end );
						if( *end != '\0' ){
							RaiseElemConvError( proc, path, tag, tp );
							goto Error;
						}
						break;
					case DAO_STRING:
						DString_Assign( tup->values[i]->xString.value, data );
						break;
					case DAO_ENUM:
						if ( !DaoEnum_SetSymbols( &tup->values[i]->xEnum, data->chars ) ){
							RaiseElemConvError( proc, path, tag, tp );
							goto Error;
						}
						break;
					case DAO_TUPLE:
						if ( 1 ){
							DString *subpath = DString_Copy( path );
							int res;
							res = MapContent( proc, elem, &tup->values[i]->xTuple, subpath );
							DString_Delete( subpath );
							if ( !res )
								return 0;
						}
						break;
					default:
						if ( 1 ){
							char buf[400];
							snprintf( buf, sizeof(buf), "Unsupported type of tuple item: %s", stp->name->chars );
							DaoProcess_RaiseError( proc, "Type", buf );
							goto Error;
						}
					}
					break;
				}
			}
		}
		if ( j == NodesSize( el->c.children ) ){
			char buf[400];
			snprintf( buf, sizeof(buf), "Element not found: %s.%s", path->chars, tag->chars );
			DaoProcess_RaiseError( proc, xmlerr, buf );
			goto Error;
		}
		DString_Delete( tag );
		DString_Delete( data );
		continue;
	Error:
		DString_Delete( tag );
		DString_Delete( data );
		return 0;
	}
	return 1;
}

int InitTuple( DaoProcess *proc, DaoTuple *tup )
{
	int i;
	for ( i = 0; i < tup->size; i++ ){
		if ( !tup->values[i] ){
			DaoType *ntype = tup->ctype->nested->items.pType[i];
			if ( ntype->tid == DAO_PAR_NAMED && ntype->aux->xType.nested && ntype->aux->xType.tid == DAO_TUPLE ){
				tup->values[i] = (DaoValue*)DaoTuple_Create( &ntype->aux->xType, ntype->aux->xType.nested->size, 1 );
				if ( !InitTuple( proc, &tup->values[i]->xTuple ) )
					return 0;
			}
			else {
				char buf[400];
				if ( ntype->tid != DAO_PAR_NAMED )
					snprintf( buf, sizeof(buf), "Tuple type contains unnamed items" );
				else
					snprintf( buf, sizeof(buf), "Unsupported type of tuple item: %s", ntype->name->chars );
				DaoProcess_RaiseError( proc, "Type", buf );
				return 0;
			}
		}
	}
	return 1;
}

static void DaoXMLElement_MapChildren( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DaoTuple *tup = DaoProcess_PutTuple( proc, p[1]->xType.nested->size );
	if ( InitTuple( proc, tup ) ){
		DString *path = DString_New();
		MapContent( proc, self, tup, path );
		DString_Delete( path );
	}
}

void GetEnumString( DaoEnum *en, DString *dest )
{
	DMap *mapNames = en->etype->mapNames;
	DNode *node;
	for ( node = DMap_First( mapNames ); node; node = DMap_Next( mapNames, node ) ){
		if ( en->subtype == DAO_ENUM_FLAG ){
			if ( !( node->value.pInt & en->value ) )
				continue;
		}
		else if ( node->value.pInt != en->value )
			continue;
		if ( dest->size )
			DString_AppendChar( dest, ';' );
		DString_Append( dest, node->key.pString );
	}
}

int WriteContent( DaoProcess *proc, DaoXMLElement *dest, DString *tag, DaoValue *src, DString *path )
{
	DaoXMLElement *el = DaoXMLElement_New();
	DString_Assign( &el->tag, tag );
	if ( path->size )
		DString_AppendChar( path, '.' );
	DString_Append( path, tag );
	if ( src->type == DAO_TUPLE ){
		DaoTuple *tup = &src->xTuple;
		DString *subpath = DString_Copy( path );
		daoint i;
		for ( i = 0; i < tup->size; i++ ){
			DaoType *stp = tup->ctype->nested->items.pType[i];
			DString *etag;
			if ( stp->tid != DAO_PAR_NAMED ){
				char buf[200];
				snprintf( buf, sizeof(buf), "Tuple content for element '%s' contains unnamed items", path->chars );
				DaoProcess_RaiseError( proc, "Type", buf );
				DString_Delete( subpath );
				DaoXMLElement_Delete( el );
				return 0;
			}
			etag = DString_Copy( stp->fname );
			if ( !WriteContent( proc, el, etag, tup->values[i], subpath ) ){
				DString_Delete( subpath );
				DString_Delete( etag );
				DaoXMLElement_Delete( el );
				return 0;
			}
			DString_Delete( etag );
		}
		DaoXMLElement_AddChild( dest, (DaoXMLNode*)el, 0 );
		DString_Delete( subpath );
	}
	else {
		el->kind = XMLTextElement;
		el->c.text = DString_New();
		switch ( src->type ){
		case DAO_INTEGER:
		case DAO_FLOAT:
		case DAO_DOUBLE:
		case DAO_STRING:
			DaoValue_GetString( src, el->c.text );
			if ( 1 ){
				daoint pos = FindInvalidXMLChar( el->c.text );
				if ( pos >= 0 ){
					char buf[200];
					GetXMLErrorMessage( XML_InvalidChar, buf );
					snprintf( buf + strlen( buf ), sizeof(buf) - strlen( buf ), " at index %" DAO_INT_FORMAT " in the content of element '%s'",
							  pos, el->tag.chars );
					DaoProcess_RaiseError( proc, xmlerr, buf );
					DaoXMLElement_Delete( el );
					return 0;
				}
			}
			DaoXMLElement_AddChild( dest, (DaoXMLNode*)el, 0 );
			break;
		case DAO_ENUM:
			GetEnumString( &src->xEnum, el->c.text );
			DaoXMLElement_AddChild( dest, (DaoXMLNode*)el, 0 );
			break;
		default:
			if ( 1 ){
				char buf[200];
				snprintf( buf, sizeof(buf), "Unsupported type of content for element '%s'", path->chars );
				DaoProcess_RaiseError( proc, "Type", buf );
				DaoXMLElement_Delete( el );
				return 0;
			}
		}
	}
	return 1;
}

static void DaoXMLElement_AddContent( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DString *path = DString_New();
	daoint i;
	for ( i = 1; i < N; i++ ){
		DString *name = DaoTuple_GetParamName( &p[i]->xTuple );
		int res = WriteContent( proc, self, name, p[i]->xTuple.values[1], path );
		DString_Delete( name );
		if ( !res )
			break;
	}
	DString_Delete( path );
}

void FindNamespace( DaoXMLElement *el, DString *id, DString *dest )
{
	DaoXMLElement *parent;
	if ( el->attribs ){
		DNode *node = DMap_Find( el->attribs, id );
		if ( node ){
			DString_Assign( dest, node->value.pString );
			return;
		}
	}
	parent = DaoXMLNode_AcquireParent( (DaoXMLNode*)el );
	if ( parent ){
		FindNamespace( parent, id, dest );
		DaoXMLNode_Delete( (DaoXMLNode*)parent );
	}
}

void DaoXMLElement_GetNamespace( DaoXMLElement *self, DString *prefix, DString *dest )
{
	DString *id = DString_NewChars( "xmlns" );
	DString_Clear( dest );
	if ( prefix->size ){
		DString_AppendChar( id, ':' );
		DString_Append( id, prefix );
	}
	FindNamespace( self, id, dest );
	DString_Delete( id );
}

static void DaoXMLElement_Namespace( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLElement *self = (DaoXMLElement*)DaoValue_TryGetCdata( p[0] );
	DString *prefix = p[1]->xString.value;
	DString *name = DaoProcess_PutChars( proc, "" );
	DaoXMLElement_GetNamespace( self, prefix, name );
}

static void DaoXMLElement_Map( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoValue *par[2] = {p[0], p[2]};
	if ( p[1]->xEnum.value == 0 )
		DaoXMLElement_MapAttribs( proc, par, 2 );
	else
		DaoXMLElement_MapChildren( proc, par, 2 );
}

static DaoFuncItem xmlElemMeths[] =
{
	/*! Constructs XML element with \a tag; if \a tag ends with '/', empty element ('<tag .../>') is created.
	 * Element attributes may be provided as name-value pairs via additional named parameters */
	{ DaoXMLElement_Create,		"element(tag: string, ...: tuple<enum, string>) => element" },

	/*! Tag */
	{ DaoXMLElement_GetTag,		".tag(invar self: element) => string" },
	{ DaoXMLElement_SetTag,		".tag=(self: element, value: string)" },

	/*! Attribute \a attribute */
	{ DaoXMLElement_GetAttr,	"[](invar self: element, attrib: string) => string" },
	{ DaoXMLElement_SetAttr,	"[]=(self: element, value: string|none, attrib: string)" },

	/*! Returns map of all attributes
	 * \note Modifying the returned map has no effect on the element */
	{ DaoXMLElement_Attributes,	".attribs(invar self: element) => map<string,string>" },

	/*! Returns \c true if element has \a attribute */
	{ DaoXMLElement_HasAttr,	"has(invar self: element, attrib: string) => bool" },

	/*! Treats element as one containing character data only. Getting text succeeds if element has single child representing character data,
	 * or has no chidren at all (but is not empty). Setting text of an element clears its list of children */
	{ DaoXMLElement_GetText,	".text(invar self: element) => string" },
	{ DaoXMLElement_SetText,	".text=(self: element, value: string)" },

	/*! Number of direct children */
	{ DaoXMLElement_Size,		".size(invar self: element) => int" },

	/*! Maps either element attributes or its children depending on \a what and returns the resulting data. \a mapping must be a tuple type with
	 * named items only, each of which refers to an existing attribute or child element by its name/tag (if there are multiple elements with the
	 * given tag, the first one is taken). Leaf elements (containing character data only) and attributes can be mapped to \c int, \c float,
	 * \c double, \c string or \c enum. Non-leaf elements can be mapped to a tuple type, in which case the mapping proceeds recursively.
	 * \a mapping may omit unneeded attributes and elements
	 * \note Use \c tuple<tag: T,> to map elements containing single sub-element */
	{ DaoXMLElement_Map,		"map(invar self: element, what: enum<attribs,children>, mapping: type<@T<tuple<...>>>) => @T" },

	/*! Additional named parameters of this method are converted into elements and appended to the list of children. For each parameter
	 * in the form *name => value*, an element '<name>value</name>' is created; specifying a tuple as value continues the conversion recursively.
	 * For leaf elements (containing character data only), supported types are \c int, \c float, \c double, \c string and \c enum;
	 * \c enum flags are written separated by ';' */
	{ DaoXMLElement_AddContent,	"extend(self: element, ...: tuple<enum, any>)" },

	/*! Is \c true if element is an empty element ('<tag ... />'). Making an element empty erases its list of children */
	{ DaoXMLElement_GetEmpty,	".empty(invar self: element) => bool" },
	{ DaoXMLElement_SetEmpty,	".empty=(self: element, value: bool)" },

	/*! Removes all attributes; for a non-empty element, sets its content to empty string */
	{ DaoXMLElement_Clear,		"clear(self: element)" },

	/*! The list of direct children
	 * \note The returned list contains *references* to child items; however, modifying the list itself has no effect on the element */
	{ DaoXMLElement_GetChildren,".children(invar self: element) => list<element|instruction|chardata>" },
	{ DaoXMLElement_SetChildren,".children=(self: element, value: list<element|instruction|chardata>)" },

	/*! Returns direct child with index \a at */
	{ DaoXMLElement_GetChild,	"child(invar self: element, at: int) => element|instruction|chardata" },

	/*! Returns the list of direct child elements
	 * \note The returned list contains *references* to child items; however, modifying the list itself has no effect on the element */
	{ DaoXMLElement_GetElems,	".elements(invar self: element) => list<element>" },

	/*! Returns the first element among the children which matches specified description. \a path is a sequence of tags delimited by '/'
	 * (e.g. 'a', 'a/b', '/b', 'a/', 'a//c', '') pointing to the element; empty tag matches any element.
	 * Element attributes may be provided as name-value pairs via additional variadic parameters */
	{ DaoXMLElement_FindElem,	"find(self: element, path: string, ...: tuple<enum, string>) => element|none" },

	/*! Returns all elements among the children which match specified description. \a path is a sequence of tags delimited by '/'
	 * (e.g. 'a', 'a/b', '/b', 'a/', 'a//c', '') pointing to the elements; empty tag matches any element (for non-end tags, the first element is
	 * picked). Element attributes may be provided as name-value pairs via additional variadic parameters */
	{ DaoXMLElement_FindElems,	"select(invar self: element, path: string, ...: tuple<enum, string>) => list<element>" },

	/*! Appends \a item to the list of children */
	{ DaoXMLElement_Append,		"append(self: element, item: element|instruction|chardata)" },

	/*! Inserts \a item in the list of children at index \a at */
	{ DaoXMLElement_Insert,		"insert(self: element, item: element|instruction|chardata, at: int)" },

	/*! Removes at most \a count children at index \a at in the list of children */
	{ DaoXMLElement_Drop,		"drop(self: element, at: int, count = 1)" },

	/*! Removes \a child from the list of children */
	{ DaoXMLElement_Drop,		"drop(self: element, child: element|instruction|chardata)" },

	/*! Returns namespace name (URI) associated with \a prefix (empty string if not found). If \a prefix is empty string,
	 * default namespace is assumed
	 * \warning Obtaining inherited namespace succeeds only if parent elements are preserved */
	{ DaoXMLElement_Namespace,	"namespace(invar self: element, prefix = '') => string" },
	{ NULL, NULL }
};

/*! XML element */
DaoTypeBase xmlElemTyper = {
	"element", NULL, NULL, xmlElemMeths, {NULL}, {0},
	(FuncPtrDel)DaoXMLNode_Delete, NULL
};

static void DaoXMLCharData_GetKind( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLCharData *self = (DaoXMLCharData*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutEnum( proc, self->kind == XMLText? "text" : "cdata" );
}

static void DaoXMLCharData_SetKind( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLCharData *self = (DaoXMLCharData*)DaoValue_TryGetCdata( p[0] );
	xml_item kind = p[1]->xEnum.value == 0? XMLText : XMLCdata;
	if ( kind == XMLCdata && self->kind == XMLText && strstr( self->data.chars, "]]>" ) ){
		char buf[100];
		GetXMLErrorMessage( XML_CdataEndInCharData, buf );
		DaoProcess_RaiseError( proc, xmlerr, buf );
	}
	else
		self->kind = kind;
}

static void DaoXMLCharData_GetValue( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLCharData *self = (DaoXMLCharData*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, &self->data );
}

static void DaoXMLCharData_SetValue( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLCharData *self = (DaoXMLCharData*)DaoValue_TryGetCdata( p[0] );
	DString *data = p[1]->xString.value;
	if ( self->kind == XMLCdata && strstr( data->chars, "]]>" ) ){
		char buf[100];
		GetXMLErrorMessage( XML_CdataEndInCharData, buf );
		DaoProcess_RaiseError( proc, xmlerr, buf );
	}
	else if ( ValidateCharData( proc, data ) )
		DString_Assign( &self->data, data );
}

static void DaoXMLCharData_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLCharData *res = DaoXMLCharData_New( p[1]->xEnum.value == 0? XMLText : XMLCdata );
	DString_Assign( &res->data, p[0]->xString.value );
	if ( res->kind == XMLCdata && strstr( res->data.chars, "]]>" ) ){
		char buf[100];
		GetXMLErrorMessage( XML_CdataEndInCharData, buf );
		DaoProcess_RaiseError( proc, xmlerr, buf );
		DaoXMLCharData_Delete( res );
	}
	else if ( !ValidateCharData( proc, &res->data ) )
		DaoXMLCharData_Delete( res );
	else
		DaoProcess_PutCdata( proc, res, daox_type_xmlcdata );
}

static void DaoXMLCharData_Size( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLCharData *self = (DaoXMLCharData*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->data.size );
}

static void DaoXMLCharData_Append( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLCharData *self = (DaoXMLCharData*)DaoValue_TryGetCdata( p[0] );
	DString *data = p[1]->xString.value;
	if ( self->kind == XMLCdata && DString_FindChars( data, "]]>", 0 ) >= 0 ){
		char buf[100];
		GetXMLErrorMessage( XML_CdataEndInCharData, buf );
		DaoProcess_RaiseError( proc, xmlerr, buf );
	}
	else if ( ValidateCharData( proc, data ) )
		DString_Append( &self->data, data );
}

static DaoFuncItem xmlCdataMeths[] =
{
	/*! Constructs XML character data containing \a data. Data representation form depends on \a kind and can be either plain text
	 * or CDATA section */
	{ DaoXMLCharData_Create,	"chardata(data = '', kind: enum<text, cdata> = $text) => chardata" },

	/*! Representation form: plain text or CDATA section */
	{ DaoXMLCharData_GetKind,	".kind(invar self: chardata) => enum<text, cdata>" },
	{ DaoXMLCharData_SetKind,	".kind=(self: chardata, value: enum<text, cdata>)" },

	/*! Represented character data */
	{ DaoXMLCharData_GetValue,	".data(invar self: chardata) => string" },
	{ DaoXMLCharData_SetValue,	".data=(self: chardata, value: string)" },

	/*! Returns the size of character data */
	{ DaoXMLCharData_Size,		"size(invar self: chardata) => int" },

	/*! Appends \a value to character data */
	{ DaoXMLCharData_Append,	"append(self: chardata, value: string)" },
	{ NULL, NULL }
};

/*! XML character data */
DaoTypeBase xmlCdataTyper = {
	"chardata", NULL, NULL, xmlCdataMeths, {NULL}, {0},
	(FuncPtrDel)DaoXMLNode_Delete, NULL
};

daoint FindInvalidXMLChar( DString *str )
{
	daoint i;
	for ( i = 0; i < str->size; i++ )
		if ( !IsXMLChar( str->chars[i] ) )
			return i;
	return -1;
}

daoint EscapeXMLMarkupChars( DString *str, DString *dest ){
	daoint i;
	for ( i = 0; i < str->size; i++ )
		switch ( str->chars[i] ){
		case '&':	DString_AppendChars( dest, "&amp;" ); break;
		case '<':	DString_AppendChars( dest, "&lt;" ); break;
		case '>':	DString_AppendChars( dest, "&gt;" ); break;
		case '"':	DString_AppendChars( dest, "&quot;" ); break;
		case '\'':	DString_AppendChars( dest, "&apos;" ); break;
		default:
			if ( !IsXMLChar( str->chars[i] ) )
				return i;
			DString_AppendChar( dest, str->chars[i] );
		}
	return -1;
}

DaoXMLWriter* DaoXMLWriter_New()
{
	DaoXMLWriter *res = (DaoXMLWriter*)dao_malloc( sizeof(DaoXMLWriter) );
	res->stream = NULL;
	res->tagstack = DaoList_New();
	res->indent = 0;
	res->start = res->end = res->closed = res->decl = res->dtd = 0;
	res->del = 1;
	return res;
}

void DaoXMLWriter_Delete( DaoXMLWriter *self )
{
	if ( DaoList_Size( self->tagstack ) )
		fprintf( stderr, "\n[xml::writer] Tag not closed\n" );
	if ( self->stream )
		DaoGC_DecRC( (DaoValue*)self->stream );
	DaoList_Delete( self->tagstack );
	dao_free( self );
}

static void DaoXMLWriter_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *res = DaoXMLWriter_New();
	if ( N == 0 )
		res->stream = DaoStream_New();
	else {
		DaoStream *stream = &p[0]->xStream;
		if ( stream->mode & DAO_STREAM_WRITABLE ){
			DaoGC_IncRC( p[0] );
			res->stream = &p[0]->xStream;
		}
		else {
			DaoProcess_RaiseError( proc, xmlerr, "Stream not writable" );
			DaoXMLWriter_Delete( res );
			return;
		}
	}
	DaoProcess_PutCdata( proc, res, daox_type_xmlwriter );
}

void DaoXMLWriter_Return( DaoXMLWriter *self )
{
	int i;
	DaoStream_WriteChar( self->stream, '\n' );
	for ( i = 0; i < self->indent; i++ )
		DaoStream_WriteChar( self->stream, '\t' );
}

void DaoXMLWriter_StartContent( DaoXMLWriter *self )
{
	if ( self->start )
		self->end = 1;
	else {
		DaoXMLWriter_Return( self );
		self->end = 0;
	}
}

int DaoXMLWriter_CheckClosed( DaoXMLWriter *self, DaoProcess *proc )
{
	if ( self->closed ){
		DaoProcess_RaiseError( proc, xmlerr, "Stream is closed" );
		return 0;
	}
	return 1;
}

static void DaoXMLWriter_RawData( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	DString *data = p[1]->xString.value;
	if ( !DaoXMLWriter_CheckClosed( self, proc ) )
		return;
	if ( ValidateCharData( proc, data ) ){
		DaoXMLWriter_StartContent( self );
		DaoStream_WriteString( self->stream, data );
		self->start = 0;
		DaoProcess_PutValue( proc, p[0] );
	}
}

void WriteDouble( DaoStream *self, double value )
{
	char buf[50];
	char *pos;
	snprintf( buf, sizeof(buf), "%f", value );
	pos = strchr( buf, '.' );
	if ( pos ){
		int i, index = pos + 1 - buf;
		for ( i = strlen( buf ) - 1; i > index && buf[i] == '0'; i-- );
		if ( i >= index )
			buf[i + 1] = '\0';
	}
	DaoStream_WriteChars( self, buf );
}

int DaoXMLWriter_CheckTagOpen( DaoXMLWriter *self, DaoProcess *proc )
{
	if ( !DaoList_Size( self->tagstack ) ){
		DaoProcess_RaiseError( proc, xmlerr, "No tag open" );
		return 0;
	}
	return 1;
}

static void DaoXMLWriter_Text( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	if ( !DaoXMLWriter_CheckClosed( self, proc ) || !DaoXMLWriter_CheckTagOpen( self, proc ) )
		return;
	DaoXMLWriter_StartContent( self );
	switch ( p[1]->type ){
	case DAO_INTEGER:
		DaoStream_WriteInt( self->stream, p[1]->xInteger.value );
		break;
	case DAO_FLOAT:
		WriteDouble( self->stream, p[1]->xFloat.value );
		break;
	case DAO_DOUBLE:
		WriteDouble( self->stream, p[1]->xDouble.value );
		break;
	case DAO_ENUM:
		if ( 1 ){
			DString *buf = DString_New();
			GetEnumString( &p[1]->xEnum, buf );
			DaoStream_WriteString( self->stream, buf );
			DString_Delete( buf );
		}
		break;
	case DAO_STRING:
		if ( 1 ){
			DString *str = p[1]->xString.value;
			DString *text = DString_New();
			daoint pos = EscapeXMLMarkupChars( str, text );
			if ( pos >= 0 ){
				RaiseValidationError( proc, XML_InvalidChar, pos );
				return;
			}
			DaoStream_WriteString( self->stream, text );
			DString_Delete( text );
		}
		break;
	}
	self->start = 0;
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoXMLWriter_Cdata( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	DString *data = p[1]->xString.value;
	DString *cdata;
	daoint pos;
	if ( !DaoXMLWriter_CheckClosed( self, proc ) || !DaoXMLWriter_CheckTagOpen( self, proc ) )
		return;
	pos = DString_FindChars( data, "]]>", 0 );
	if ( pos >= 0 )
		RaiseValidationError( proc, XML_CdataEndInCharData, pos );
	else if ( ValidateCharData( proc, data ) ){
		cdata = DString_NewChars( "<![CDATA[" );
		DString_Append( cdata, data );
		DString_AppendChars( cdata, "]]>" );
		DaoXMLWriter_StartContent( self );
		DaoStream_WriteString( self->stream, cdata );
		self->start = 0;
		DaoProcess_PutValue( proc, p[0] );
		DString_Delete( cdata );
	}
}

static void DaoXMLWriter_Comment( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	DString *text = p[1]->xString.value;
	DString *comm;
	daoint pos;
	if ( !DaoXMLWriter_CheckClosed( self, proc ) )
		return;
	pos = DString_FindChars( text, "--", 0 );
	if ( pos >= 0 )
		RaiseValidationError( proc, XML_HyphensInComment, pos );
	else if ( ValidateCharData( proc, text ) ){
		comm = DString_NewChars( "\t<!-- " );
		DString_Append( comm, text );
		DString_AppendChars( comm, " -->" );
		DaoStream_WriteString( self->stream, comm );
		self->start = 0;
		DaoProcess_PutValue( proc, p[0] );
		DString_Delete( comm );
	}
}

static void DaoXMLWriter_Stream( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutValue( proc, (DaoValue*)self->stream );
}

static void DaoXMLWriter_Flush( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	if ( !DaoXMLWriter_CheckClosed( self, proc ) )
		return;
	DaoStream_Flush( self->stream );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoXMLWriter_Close( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	DaoStream_Close( self->stream );
	self->closed = 1;
}

static void DaoXMLWriter_Tag( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	DString *tag = p[1]->xString.value;
	DString *buf, *name, *attribs = NULL;
	xml_error res;
	XMLContext ctx = {NULL, 0};
	memset( &ctx.state, 0, sizeof(ctx.state) );
	if ( !DaoXMLWriter_CheckClosed( self, proc ) )
		return;
	DString_Trim( tag, 1, 1, 0 );
	if ( !tag->size ){
		DaoProcess_RaiseError( proc, xmlerr, "Empty tag" );
		return;
	}
	buf = DString_Copy( tag );
	name = DString_New();
	ctx.pos = buf->chars;
	res = ParseXMLName( &ctx, name, NULL );
	if ( res ){
		RaiseValidationError( proc, res, UngetChar( &ctx ) - buf->chars );
		goto Exit;
	}
	if ( *ctx.pos != '\0' ){
		if ( !PassXMLWhitespace( &ctx ) && *ctx.pos != '/' ){
			RaiseValidationError( proc, XML_InvalidNameChar, ctx.pos - buf->chars );
			goto Exit;
		}
		if ( GetChar( &ctx ) != '/' || GetChar( &ctx ) != '\0' ){
			RaiseValidationError( proc, XML_InvalidTagEnding, UngetChar( &ctx ) - buf->chars );
			goto Exit;
		}
	}
	if ( N > 2 ){
		if ( p[2]->type == DAO_MAP ){
			DaoMap *map = &p[2]->xMap;
			DNode *node;
			for ( node = DaoMap_First( map ); node; node = DaoMap_Next( map, node ) ){
				DString *attr = node->key.pValue->xString.value;
				DString *val = node->value.pValue->xString.value;
				if ( !CheckAttrValue( proc, attr, val ) )
					goto Exit;
				if ( !attribs )
					attribs = DString_New();
				DString_AppendChar( attribs, ' ' );
				DString_Append( attribs, attr );
				DString_AppendChars( attribs, "=\"" );
				EscapeXMLMarkupChars( val, attribs );
				DString_AppendChar( attribs, '"' );
			}
		}
		else {
			int i;
			attribs = DString_New();
			for ( i = 2; i < N; i++ ){
				DString *attr = DaoTuple_GetParamName( &p[i]->xTuple );
				DString *val = p[i]->xTuple.values[1]->xString.value;
				if ( !CheckAttrValue( proc, attr, val ) ){
					DString_Delete( attr );
					goto Exit;
				}
				DString_AppendChar( attribs, ' ' );
				DString_Append( attribs, attr );
				DString_AppendChars( attribs, "=\"" );
				EscapeXMLMarkupChars( val, attribs );
				DString_AppendChar( attribs, '"' );
				DString_Delete( attr );
			}
		}
	}
	self->start = 0;
	DaoXMLWriter_Return( self );
	DaoStream_WriteChar( self->stream, '<' );
	if ( attribs ){
		int empty = DString_FindChar( tag, '/', 0 ) > 0;
		if ( empty )
			DString_Resize( tag, tag->size - 1 );
		DaoStream_WriteString( self->stream, tag );
		DaoStream_WriteString( self->stream, attribs );
		if ( empty )
			DaoStream_WriteChar( self->stream, '/' );
	}
	else
		DaoStream_WriteString( self->stream, tag );
	DaoStream_WriteChar( self->stream, '>' );
	if ( buf->chars[buf->size - 1] != '/' ){
		DaoString *str = DaoString_New();
		self->indent++;
		self->start = 1;
		DaoString_Set( str, tag );
		DaoList_PushBack( self->tagstack, (DaoValue*)str );
	}
	DaoProcess_PutValue( proc, p[0] );
Exit:
	DString_Delete( buf );
	DString_Delete( name );
	if ( attribs )
		DString_Delete( attribs );
}

static void DaoXMLWriter_End( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	if ( !DaoXMLWriter_CheckClosed( self, proc ) )
		return;
	if ( !DaoList_Size( self->tagstack ) ){
		DaoProcess_RaiseError( proc, xmlerr, "No tag open" );
		return;
	}
	self->indent--;
	if ( !self->end && !self->start )
		DaoXMLWriter_Return( self );
	self->start = self->end = 0;
	DaoStream_WriteChars( self->stream, "</" );
	DaoStream_WriteString( self->stream, DaoList_Back( self->tagstack )->xString.value );
	DaoStream_WriteChar( self->stream, '>' );
	DaoList_PopBack( self->tagstack );
	DaoProcess_PutValue( proc, p[0] );
}

static void DaoXMLWriter_Header( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	DString *version = p[1]->xString.value;
	DString *encoding = p[2]->xString.value;
	DString *standalone = p[3]->xString.value;
	DString *buf;
	xml_error res;
	XMLContext ctx = {NULL, 0};
	int par;
	memset( &ctx.state, 0, sizeof(ctx.state) );
	if ( !DaoXMLWriter_CheckClosed( self, proc ) )
		return;
	if ( self->decl ){
		DaoProcess_RaiseError( proc, xmlerr, "Header was already written" );
		return;
	}
	if ( DaoList_Size( self->tagstack ) ){
		DaoProcess_RaiseError( proc, xmlerr, "Trying to write header within an element" );
		return;
	}
	self->decl = 1;
	buf = DString_NewChars( "<?xml version=\"" );
	DString_Append( buf, version );
	DString_AppendChar( buf, '"' );
	if ( encoding->size ){
		DString_AppendChars( buf, " encoding=\"" );
		DString_Append( buf, encoding );
		DString_AppendChar( buf, '"' );
	}
	if ( standalone->size ){
		DString_AppendChars( buf, " standalone=\"" );
		DString_Append( buf, standalone );
		DString_AppendChar( buf, '"' );
	}
	DString_AppendChars( buf, "?>" );
	ctx.pos = buf->chars + 5;
	res = ParseXMLDeclaration( &ctx, NULL, NULL, &par );
	if ( res ){
		char buf[100];
		GetXMLErrorMessage( res, buf );
		DaoProcess_RaiseError( proc, xmlerr, buf );
	}
	else {
		DaoStream_WriteString( self->stream, buf );
		DaoProcess_PutValue( proc, p[0] );
	}
	DString_Delete( buf );
}

static void DaoXMLWriter_PInst( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	DString *name = p[1]->xString.value;
	DString *data = p[2]->xString.value;
	DString_Trim( name, 1, 1, 0 );
	DString_Trim( data, 1, 1, 0 );
	if ( ValidateName( proc, name->chars ) && ValidateCharData( proc, data ) ){
		if ( strcmp( name->chars, "xml" ) == 0 ){
			char buf[100];
			GetXMLErrorMessage( XML_ReservedInstructionName, buf );
			DaoProcess_RaiseError( proc, xmlerr, buf );
			return;
		}
		self->start = 0;
		DaoXMLWriter_Return( self );
		DaoStream_WriteChars( self->stream, "<?" );
		DaoStream_WriteString( self->stream, name );
		DaoStream_WriteChars( self->stream, " " );
		DaoStream_WriteString( self->stream, data );
		DaoStream_WriteChars( self->stream, "?>" );
		DaoProcess_PutValue( proc, p[0] );
	}
}

static void DaoXMLWriter_Doctype( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoXMLWriter *self = (DaoXMLWriter*)DaoValue_TryGetCdata( p[0] );
	DString *dtd = p[1]->xString.value;
	XMLContext ctx = {NULL, 0};
	xml_error res;
	memset( &ctx.state, 0, sizeof(ctx.state) );
	if ( self->dtd ){
		DaoProcess_RaiseError( proc, xmlerr, "DTD section was already written" );
		return;
	}
	if ( DaoList_Size( self->tagstack ) ){
		DaoProcess_RaiseError( proc, xmlerr, "Trying to write DTD section within an element" );
		return;
	}
	if ( strncmp( dtd->chars, "<!DOCTYPE", 9 ) != 0 ){
		DaoProcess_RaiseError( proc, xmlerr, "Not a DTD section" );
		return;
	}
	ctx.pos = dtd->chars + 9;
	switch ( GetChar( &ctx ) ){
	case '\r':
	case '\n':
	case ' ':
	case '\t':
		break;
	default:
		DaoProcess_RaiseError( proc, xmlerr, "Invalid DOCTYPE section" );
		return;
	}
	res = ParseXMLDoctype( &ctx, NULL );
	if ( res ){
		RaiseValidationError( proc, res, UngetChar( &ctx ) - dtd->chars );
		return;
	}
	self->dtd = 1;
	DaoXMLWriter_Return( self );
	DaoStream_WriteString( self->stream, dtd );
	DaoProcess_PutValue( proc, p[0] );
}

static DaoFuncItem xmlWriterMeths[] =
{
	/*! Creates XML writer which writes to stream \a dest */
	{ DaoXMLWriter_Create,	"writer(dest: io::stream) => writer" },

	/*! Creates XML writer which writes to internal string stream */
	{ DaoXMLWriter_Create,	"writer() => writer" },

	/*! Output stream */
	{ DaoXMLWriter_Stream,	".stream(invar self: writer) => invar<io::stream>" },

	/*! Flushes output stream and returns \a self */
	{ DaoXMLWriter_Flush,	"flush(self: writer) => writer" },

	/*! Closes output stream */
	{ DaoXMLWriter_Close,	"close(self: writer)" },

	/*! Writes \a data as raw data (without preprocessing) and returns \a self */
	{ DaoXMLWriter_RawData,	"raw(self: writer, data: string) => writer" },

	/*! Writes \a value as text and returns \a self. Special characters in resulting text are replaced with references */
	{ DaoXMLWriter_Text,	"text(self: writer, value: int|float|double|enum|string) => writer" },

	/*! Writes CDATA section containing \a data and returns \a self */
	{ DaoXMLWriter_Cdata,	"cdata(self: writer, data: string) => writer" },

	/*! Writes comment containing \a text and returns \a self */
	{ DaoXMLWriter_Comment,	"comment(self: writer, text: string) => writer" },

	/*! Writes start tag or empty-element \a name and returns \a self. An empty element is assumed if \a name ends with '/'.
	 * Attributes may be provided as name-value pairs via additional named parameters */
	{ DaoXMLWriter_Tag,		"tag(self: writer, name: string, ...: tuple<enum, string>) => writer" },

	/*! Writes start tag or empty-element \a name with \a attributes and returns \a self.
	 * An empty element is assumed if \a name ends with '/' */
	{ DaoXMLWriter_Tag,		"tag(self: writer, name: string, attributes: map<string,string>) => writer" },

	/*! Writes end tag matching the last written start tag and returns \a self */
	{ DaoXMLWriter_End,		"end(self: writer) => writer" },

	/*! Writes XML declaration containing \a version, \a encoding and \a standalone parameters (\a encoding and \a standalone may be omitted),
	 * returns \a self
	 * \note Specifying encoding has no effect on actual encoding of resulting XML document */
	{ DaoXMLWriter_Header,	"header(self: writer, version = '1.0', encoding = '', standalone = '') => writer" },

	/*! Writes processing instruction with \a name and \a data and returns \a self */
	{ DaoXMLWriter_PInst,	"instruction(self: writer, name: string, data = '') => writer" },

	/*! Writes DTD section and returns \a self; \a dtd should be in form of '<!DOCTYPE ... >' */
	{ DaoXMLWriter_Doctype,	"doctype(self: writer, dtd: string) => writer" },
	{ NULL, NULL }
};

/*! Writable stream of XML data */
DaoTypeBase xmlWriterTyper = {
	"writer", NULL, NULL, xmlWriterMeths, {NULL}, {0},
	(FuncPtrDel)DaoXMLWriter_Delete, NULL
};

static DaoFuncItem xmlMeths[] = {
	/*! Returns XML document parsed from \a str */
	{ DaoXMLDocument_FromString,	"parse(str: string) => xml::document" },
	{ NULL, NULL }
};

DAO_DLL int DaoXML_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *xmlns;
	DMutex_Init( &xmlmtx );
	xmlns = DaoVmSpace_GetNamespace( vmSpace, "xml" );
	DaoNamespace_AddConstValue( ns, "xml", (DaoValue*)xmlns );
	daox_type_xmlinst = DaoNamespace_WrapType( xmlns, &xmlInstTyper, 1 );
	daox_type_xmlcdata = DaoNamespace_WrapType( xmlns, &xmlCdataTyper, 1 );
	daox_type_xmlelem = DaoNamespace_WrapType( xmlns, &xmlElemTyper, 1 );
	daox_type_xmldoc = DaoNamespace_WrapType( xmlns, &xmlDocTyper, 1 );
	daox_type_xmlwriter = DaoNamespace_WrapType( xmlns, &xmlWriterTyper, 1 );
	DaoNamespace_WrapFunctions( xmlns, xmlMeths );
	return 0;
}
