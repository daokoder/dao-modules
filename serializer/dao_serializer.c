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
// THIS SOFTWARE IS PROVIDED  BY THE COPYRIGHT HOLDERS AND  CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED  WARRANTIES,  INCLUDING,  BUT NOT LIMITED TO,  THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR CONTRIBUTORS  BE LIABLE FOR ANY DIRECT,
// INDIRECT,  INCIDENTAL, SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL  DAMAGES (INCLUDING,
// BUT NOT LIMITED TO,  PROCUREMENT OF  SUBSTITUTE  GOODS OR  SERVICES;  LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY OF
// LIABILITY,  WHETHER IN CONTRACT,  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

#define DAO_SERIAL
#include"dao_serializer.h"
#include"daoString.h"
#include"daoValue.h"
#include"daoParser.h"
#include"daoNamespace.h"
#include"daoVmspace.h"
#include"daoGC.h"


int DaoParser_FindPairToken( DaoParser *self,  uchar_t lw, uchar_t rw, int start, int stop );
DaoType* DaoParser_ParseType( DaoParser *self, int start, int end, int *newpos, DList *types );


#define RADIX 32
static const char *hex_digits = "ABCDEFGHIJKLMNOP";
static const char *mydigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVW";

void DaoEncodeInteger( char *buffer, daoint value )
{
	int m;
	if( value < 0 ){
		*(buffer++) = '-';
		value = - value;
	}
	*(buffer++) = 'X';
	*buffer = 0;
	if( value == 0 ){
		*(buffer++) = '0';
		*buffer = 0;
		return;
	}
	while( value ){
		m = value % RADIX;
		value /= RADIX;
		*(buffer++) = mydigits[m];
	}
	*buffer = 0;
}
daoint DaoDecodeInteger( char *buffer )
{
	daoint value = 0;
	daoint power = 1;
	int sign = 1;
	if( *buffer == '-' ){
		sign = -1;
		buffer ++;
	}
	if( *buffer == 'X' ) buffer ++;
	while( *buffer ){
		int digit = *buffer;
		digit -= digit >= 'A' ? 'A' - 10 : '0';
		value += digit * power;
		power *= RADIX;
		buffer ++;
	}
	return value * sign;
}

void DaoEncodeDouble( char *buffer, double value )
{
	int expon, digit;
	double prod, frac;
	if( value <0.0 ){
		*(buffer++) = '-';
		value = -value;
	}
	*(buffer++) = 'X';
	*buffer = 0;
	frac = frexp( value, & expon );
	while(1){
		prod = frac * RADIX;
		digit = (int) prod;
		frac = prod - digit;
		*(buffer++) = mydigits[ digit ];
		if( frac <= 0 ) break;
	}
	*(buffer++) = '_';
	if( expon < 0 ) *(buffer++) = '_';
	DaoEncodeInteger( buffer, abs( expon ) );
	return;
}
double DaoDecodeDouble( char *buffer )
{
	double frac = 0;
	int expon, sign = 1, sign2 = 1;
	double factor = 1.0 / RADIX;
	double accum = factor;
	if( buffer[0] == '-' ){
		buffer ++;
		sign = -1;
	}
	if( *buffer == 'X' ) buffer ++;
	while( *buffer && *buffer != '_' ){
		int digit = *buffer;
		digit -= digit >= 'A' ? 'A' - 10 : '0';
		frac += accum * digit;
		accum *= factor;
		buffer ++;
	}
	if( buffer[1] == '_' ){
		sign2 = -1;
		buffer ++;
	}
	expon = sign2 * DaoDecodeInteger( buffer + 1 );
	return ldexp( frac, expon ) * sign;
}


static void DaoSerializeInteger( daoint value, DString *serial )
{
	char buf[100];
	DaoEncodeInteger( buf, value );
	DString_AppendChars( serial, buf );
}
static void DaoSerializeDouble( double value, DString *serial )
{
	char buf[100];
	DaoEncodeDouble( buf, value );
	DString_AppendChars( serial, buf );
}
static void DaoSerializeComplex( dao_complex value, DString *serial )
{
	DaoSerializeDouble( value.real, serial );
	DString_AppendChar( serial, ' ' );
	DaoSerializeDouble( value.imag, serial );
}
static void DaoSerializeString( DString *source, DString *serial )
{
	unsigned char *chars = (unsigned char*) source->chars;
	int i;
	for(i=0; i<source->size; i++){
		DString_AppendChar( serial, hex_digits[ chars[i] / 16 ] );
		DString_AppendChar( serial, hex_digits[ chars[i] % 16 ] );
	}
}
static void DaoDeserializeString( DString *serial, DString *source )
{
	unsigned char *chars = (unsigned char*) serial->chars;
	int i, size = serial->size;
	for(i=0; i<size; i+=2){
		char c1 = chars[i];
		char c2 = chars[i+1];
		if( c1 < 'A' || c1 > 'P' ) continue; // TODO: Error;
		if( c2 < 'A' || c2 > 'P' ) continue; // TODO: Error;
		DString_AppendChar( source, (char)((c1-'A')*16 + (c2-'A')) );
	}
}



DaoType *daox_type_serializer = NULL;


DaoSerializer* DaoSerializer_New( )
{
	DaoSerializer *self = (DaoSerializer*) dao_calloc( 1, sizeof(DaoSerializer) );
	DaoCstruct_Init( (DaoCstruct*)self, daox_type_serializer );
	self->error = 0;
	self->nspace = NULL;
	self->parser = NULL;
	self->value = NULL;
	self->values = DList_New(DAO_DATA_VALUE);
	self->types = DList_New(0);
	self->cdata = DHash_New(0,0);
	self->objects = DHash_New(0,0);
	self->serial = DString_New();
	self->buffer = DString_New();
	return self;
}

void DaoSerializer_Delete( DaoSerializer *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DList_Delete( self->values );
	DList_Delete( self->types );
	DMap_Delete( self->cdata );
	DMap_Delete( self->objects );
	DString_Delete( self->serial );
	DString_Delete( self->buffer );
	dao_free( self );
}

void DaoSerializer_Reset( DaoSerializer *self, DaoNamespace *ns )
{
	DaoGC_DecRC( self->value );
	DString_Reset( self->serial, 0 );
	DList_Clear( self->types );
	DList_Clear( self->values );
	DMap_Reset( self->objects );
	DMap_Reset( self->cdata );

	self->nspace = ns;
	self->parser = NULL;
	self->process = NULL;
	self->value = NULL;
	self->error = 0;
}


static void DaoSerializer_EncodeInteger( DaoSerializer *self, daoint value )
{
	char buf[100];
	DaoEncodeInteger( buf, value );
	DString_AppendChars( self->serial, buf );
}
static void DaoSerializer_EncodeDouble( DaoSerializer *self, double value )
{
	char buf[100];
	DaoEncodeDouble( buf, value );
	DString_AppendChars( self->serial, buf );
}
static void DaoSerializer_EncodeComplex( DaoSerializer *self, dao_complex value )
{
	DaoSerializer_EncodeDouble( self, value.real );
	DString_AppendChar( self->serial, ' ' );
	DaoSerializer_EncodeDouble( self, value.imag );
}

static void DaoSerializer_EncodeString( DaoSerializer *self, DString *value )
{
	DaoSerializeString( value, self->serial );
}

static void DaoSerializer_EncodeArray( DaoSerializer *self, DaoArray *value )
{
	DString *serial = self->serial;
	int i;

	DString_AppendChar( serial, '[' );
	for(i=0; i<value->ndim; i++){
		if( i ) DString_AppendChar( serial, ',' );
		DaoSerializeInteger( value->dims[i], serial );
	}
	DString_AppendChar( serial, ']' );
	for(i=0; i<value->size; i++){
		if( i ) DString_AppendChar( serial, ',' );
		switch( value->etype ){
		case DAO_BOOLEAN : DaoSerializeInteger( value->data.b[i], serial ); break;
		case DAO_INTEGER : DaoSerializeInteger( value->data.i[i], serial ); break;
		case DAO_FLOAT   : DaoSerializeDouble( value->data.f[i], serial ); break;
		case DAO_COMPLEX : DaoSerializeComplex( value->data.c[i], serial ); break;
		}
	}
}

static int DaoSerializer_EncodeValue( DaoSerializer *self, DaoValue *value );

static void DaoSerializer_EncodeList( DaoSerializer *self, DaoList *value )
{
	DString *serial = self->serial;
	DaoNamespace *ns = self->nspace;
	int i;

	for(i=0; i<value->value->size; i++){
		DaoValue *item = value->value->items.pValue[i];
		if( i ) DString_AppendChar( serial, ',' );
		//if( item->type >= DAO_ARRAY ) DString_AppendChar( serial, '{' );
		DaoSerializer_EncodeValue( self, item );
		//if( item->type >= DAO_ARRAY ) DString_AppendChar( serial, '}' );
	}
}

static void DaoSerializer_EncodeMap( DaoSerializer *self, DaoMap *value )
{
	DNode *it;
	DString *serial = self->serial;
	DaoNamespace *ns = self->nspace;
	char *sep = value->value->hashing ? "->" : "=>";
	int i = 0;

	for(it=DMap_First(value->value); it; it=DMap_Next(value->value,it)){
		if( (i++) ) DString_AppendChar( serial, ',' );
		DaoSerializer_EncodeValue( self, it->key.pValue );
		DString_AppendChars( serial, sep );
		DaoSerializer_EncodeValue( self, it->value.pValue );
	}
}

static void DaoSerializer_EncodeTuple( DaoSerializer *self, DaoTuple *value )
{
	DString *serial = self->serial;
	DaoNamespace *ns = self->nspace;
	DList *types  = value->ctype ? value->ctype->nested : NULL;
	int i;

	for(i=0; i<value->size; i++){
		DaoType *type = NULL;
		if( types && types->size > i ) type = types->items.pType[i];
		if( type && type->tid == DAO_PAR_NAMED ) type = & type->aux->xType;
		if( type && (type->tid == 0 || type->tid >= DAO_ENUM)) type = NULL;
		if( i ) DString_AppendChar( serial, ',' );
		DaoSerializer_EncodeValue( self, value->values[i] );
	}
}

static int DaoSerializer_EncodeObject( DaoSerializer *self, DaoObject *value )
{
	DString *tname = NULL;
	DaoValue *ret = NULL;
	DaoValue *value1 = NULL;
	DaoValue *value2 = NULL;
	DaoValue *selfpar = (DaoValue*) value;
	DString name1 = DString_WrapChars( "typename" );
	DString name2 = DString_WrapChars( "serialize" );
	DString *serial = self->serial;
	DaoNamespace *ns = self->nspace;
	DaoProcess *process = self->process;
	int errcode1 = DaoObject_GetData( value, & name1, & value1, NULL );
	int errcode2 = DaoObject_GetData( value, & name2, & value2, NULL );
	char chs[64];

	if( errcode2 || value2 == NULL || value2->type != DAO_ROUTINE ) return 0;
	if( errcode1 == 0 && value1 != NULL && value1->type == DAO_ROUTINE ){
		if( DaoProcess_Call( process, (DaoRoutine*)value1, selfpar, NULL, 0 ) ) return 0;
		ret = process->stackValues[0];
		if( ret->type == DAO_STRING ) tname = DString_Copy( ret->xString.value );
	}
	if( DaoProcess_Call( process, (DaoRoutine*)value2, selfpar, NULL, 0 ) ){
		if( tname ) DString_Delete( tname );
		return 0;
	}
	if( tname == NULL ) tname = value->defClass->className;

	DString_Append( serial, tname );
	DString_AppendChar( serial, '{' );
	DMap_Insert( self->objects, value, value );
	sprintf( chs, "(%p)", value );
	DString_AppendChars( serial, chs );

	DaoSerializer_EncodeValue( self, process->stackValues[0] );
	DString_AppendChar( serial, '}' );
	if( tname != NULL && tname != value->defClass->className ) DString_Delete( tname );
	return 1;
}

static int DaoSerializer_EncodeCstruct( DaoSerializer *self, DaoCstruct *value )
{
	DString *tname = NULL;
	DaoType *type = NULL;
	DaoValue *ret = NULL;
	DaoRoutine *meth1 = DaoType_FindFunctionChars( value->ctype, "typename" );
	DaoRoutine *meth2 = DaoType_FindFunctionChars( value->ctype, "serialize" );
	DaoNamespace *ns = self->nspace;
	DaoProcess *process = self->process;
	DString *serial = self->serial;
	int encode_return = 0;
	char chs[64];

	if( value->type == DAO_CDATA ){
		DaoCdata *cdata = (DaoCdata*) value;
		DMap_Insert( self->cdata, cdata->data, value );
	}

	if( meth2 == NULL ) goto Encode;
	if( meth1 != NULL ){
		if( DaoProcess_Call( process, meth1, (DaoValue*)value, NULL, 0 ) ) return 0;
		ret = process->stackValues[0];
		if( ret->type == DAO_STRING ) tname = DString_Copy( ret->xString.value );
	}
	if( DaoProcess_Call( process, meth2, (DaoValue*)value, NULL, 0 ) ){
		if( tname ) DString_Delete( tname );
		return 0;
	}
	encode_return = 1;

Encode:

	if( tname == NULL ) tname = value->ctype->name;
	DString_Append( serial, tname );
	DString_AppendChar( serial, '{' );
	DMap_Insert( self->objects, value, value );
	sprintf( chs, "(%p)", value );
	DString_AppendChars( serial, chs );

	if( value->type == DAO_CDATA ){
		DaoCdata *cdata = (DaoCdata*) value;
		sprintf( chs, "[%p]", cdata->data );
		DString_AppendChars( serial, chs );
	}

	if( encode_return ) DaoSerializer_EncodeValue( self, process->stackValues[0] );
	DString_AppendChar( serial, '}' );
	if( tname != NULL && tname != value->ctype->name ) DString_Delete( tname );
	return 1;
}

static int DaoSerializer_EncodeUserData( DaoSerializer *self, DaoValue *value )
{
	if( self->process == NULL ) return 0;
	switch( value->type ){
	case DAO_OBJECT  : return DaoSerializer_EncodeObject( self, (DaoObject*) value );
	case DAO_CDATA   :
	case DAO_CSTRUCT : return DaoSerializer_EncodeCstruct( self, (DaoCstruct*) value );
	}
	return 0;
}

static void DaoSerializer_EncodeNamespace( DaoSerializer *self, DaoNamespace *value )
{
	DaoSerializer_EncodeString( self, value->name );
}

static int DaoSerializer_EncodeValue( DaoSerializer *self, DaoValue *value )
{
	DString *serial = self->serial;
	DaoNamespace *ns = self->nspace;
	DaoProcess *process = self->process;
	DaoType *type = DaoNamespace_GetType( ns, value );
	char chs[64];
	int modtype;
	int rc = 1;

	if( DMap_Find( self->objects, value ) ){
		sprintf( chs, "@{%p}", value );
		DString_AppendChars( serial, chs );
		return 1;
	}
	switch( value->type ){
	case DAO_OBJECT :
	case DAO_CDATA :
	case DAO_CSTRUCT :
		return DaoSerializer_EncodeUserData( self, value );
	case DAO_CTYPE :
		DString_AppendChars( serial, "class<" );
		DString_Append( serial, type->name );
		DString_AppendChars( serial, ">{}" );
		return 1;
	}
	if( type ){
		DString_Append( serial, type->name );
		DString_AppendChar( serial, '{' );
	}
	switch( value->type ){
	case DAO_NONE :
		break;
	case DAO_INTEGER :
		DaoSerializer_EncodeInteger( self, value->xInteger.value );
		break;
	case DAO_FLOAT :
		DaoSerializer_EncodeDouble( self, value->xFloat.value );
		break;
	case DAO_COMPLEX :
		DaoSerializer_EncodeComplex( self, value->xComplex.value );
		break;
	case DAO_STRING :
		DaoSerializer_EncodeString( self, value->xString.value );
		break;
	case DAO_ENUM :
		DaoSerializer_EncodeInteger( self, value->xEnum.value );
		break;
	case DAO_ARRAY :
		DaoSerializer_EncodeArray( self, (DaoArray*) value );
		break;
	case DAO_LIST :
	case DAO_MAP :
	case DAO_TUPLE :
		DMap_Insert( self->objects, value, value );
		sprintf( chs, "(%p)", value );
		DString_AppendChars( serial, chs );
		switch( value->type ){
		case DAO_LIST :
			DaoSerializer_EncodeList( self, (DaoList*) value );
			break;
		case DAO_MAP :
			DaoSerializer_EncodeMap( self, (DaoMap*) value );
			break;
		case DAO_TUPLE :
			DaoSerializer_EncodeTuple( self, (DaoTuple*) value );
			break;
		}
		break;
	case DAO_NAMESPACE :
		DString_SetChars( self->buffer, value->xNamespace.name->chars );
		modtype = DaoVmSpace_CompleteModuleName( ns->vmSpace, self->buffer, 0 );
		/* Do not serialize module of unknown type: */
		if( modtype == DAO_MODULE_NONE || modtype == DAO_MODULE_ANY ) return 0;
		DaoSerializer_EncodeNamespace( self, (DaoNamespace*) value );
		break;
	default :
		DString_AppendChar( serial, '?' );
		rc = 0;
		break;
	}
	if( type ) DString_AppendChar( serial, '}' );
	return rc;
}

DString* DaoSerializer_Encode( DaoSerializer *self, DaoValue *values[], int count )
{
	int i;
	self->parser = DaoVmSpace_AcquireParser( self->nspace->vmSpace );
	self->process = DaoVmSpace_AcquireProcess( self->nspace->vmSpace );
	for(i=0; i<count; ++i){
		DaoValue *value = values[i];
		if( value == NULL ) break;
		DaoSerializer_EncodeValue( self, value );
	}
	DaoVmSpace_ReleaseParser( self->nspace->vmSpace, self->parser );
	DaoVmSpace_ReleaseProcess( self->nspace->vmSpace, self->process );
	return self->serial;
}




static DaoObject* DaoSerializer_MakeObject( DaoSerializer *self, DaoClass *klass, DaoValue *param )
{
	DaoProcess *proc = self->process;
	DaoRoutine *routines = klass->initRoutines;
	DaoObject *object = DaoObject_New( klass );

	DaoProcess_CacheValue( proc, (DaoValue*) object );
	if( DaoProcess_PushCallable( proc, routines, (DaoValue*)object, & param, 1 ) ==0 ){
		GC_Assign( & proc->topFrame->object, object );
		if( DaoProcess_Execute( proc ) ) return object;
	}
	return NULL;
}

static DaoCstruct* DaoSerializer_MakeCstruct( DaoSerializer *self, DaoCtype *ctype, DaoValue *param )
{
	DaoValue *ret;
	DaoProcess *proc = self->process;
	DaoRoutine *routine = DaoType_FindFunction( ctype->cdtype, ctype->name );

	if( DaoProcess_PushCallable( proc, routine, NULL, & param, 1 ) ) return NULL;
	proc->topFrame->active = proc->firstFrame;
	DaoProcess_SetActiveFrame( proc, proc->firstFrame ); /* return value in stackValues[0] */
	if( DaoProcess_Execute( proc ) == 0 ) return NULL;
	ret = proc->stackValues[0];
	if( ret && (ret->type == DAO_CDATA || ret->type == DAO_CSTRUCT) ) return (DaoCstruct*) ret;
	return NULL;
}

static int DaoSerializer_DecodeValue( DaoSerializer *self, int start, int end, DaoValue **value2 )
{
	DString *serial = self->serial;
	DList *types = self->types;
	DaoNamespace *ns = self->nspace;
	DaoProcess *process = self->process;
	DaoParser *parser = self->parser;
	DaoToken **tokens = parser->tokens->items.pToken;
	DaoType *it1 = NULL, *it2 = NULL, *type = NULL;
	DaoValue *tmp = NULL;
	DaoValue *tmp2 = NULL;
	DaoValue *value = *value2;
	DaoObject *object;
	DaoCstruct *cstruct;
	DaoArray *array;
	DaoTuple *tuple;
	DaoList *list;
	DaoMap *map;
	DList *dims;
	DNode *node;
	void *key = NULL;
	char *str;
	int i, j, k, n;
	int minus = 0;
	int next = start + 1;
	int tok2 = start < end ? tokens[start+1]->type : 0;
	int maybetype = tok2 == DTOK_COLON2 || tok2 == DTOK_LT || tok2 == DTOK_LCB;

	if( tokens[start]->type == DTOK_AT && tok2 == DTOK_LCB ){
		int rb = DaoParser_FindPairToken( parser, DTOK_LCB, DTOK_RCB, start, end );
		if( rb < 0 ) return next;
		sscanf( tokens[start+2]->string.chars, "%p", & key );
		node = DMap_Find( self->objects, key );
		if( node ) DaoValue_Copy( node->value.pValue, value2 );
		return rb + 1;
	}
	if( tokens[start]->name == DTOK_ID_SYMBOL ){
		DString *tname = DString_New();
		while( tokens[start]->name == DTOK_ID_SYMBOL ){
			DString_Append( tname, & tokens[start]->string );
			start += 1;
		}
		type = DaoNamespace_MakeType( ns, tname->chars, DAO_ENUM, NULL, NULL, 0 );
		DString_Delete( tname );
		if( type == NULL ) return start;
		if( tokens[start]->name != DTOK_LCB ) return start;
		end = DaoParser_FindPairToken( parser, DTOK_LCB, DTOK_RCB, start, end );
		if( end < 0 ) return start;
		next = end + 1;
		start += 1;
		end -= 1;
	}else if( tokens[start]->type == DTOK_IDENTIFIER && maybetype ){
		type = DaoParser_ParseType( parser, start, end, & start, NULL );
		if( type == NULL ) return next;
		if( tokens[start]->name != DTOK_LCB ) return start;
		end = DaoParser_FindPairToken( parser, DTOK_LCB, DTOK_RCB, start, end );
		if( end < 0 ) return start;
		next = end + 1;
		start += 1;
		end -= 1;
	}
	if( type == NULL ){
		type = types->items.pType[0];
		if( type && type->tid >= DAO_ARRAY ){
			if( tokens[start]->name != DTOK_LCB ) return start;
			end = DaoParser_FindPairToken( parser, DTOK_LCB, DTOK_RCB, start, end );
			if( end < 0 ) return start;
			next = end + 1;
			start += 1;
			end -= 1;
		}
	}
	if( type == NULL ) return next;
	DaoValue_Copy( type->value, value2 );
	if( type->tid == DAO_CTYPE ){
		DaoValue_Copy( type->aux, value2 );
		return next;
	}
	if( start > end ) return next;
	if( tokens[start]->name == DTOK_SUB ){
		minus = 1;
		start += 1;
		if( start > end ) return next;
	}
	if( type->nested && type->nested->size >0 ) it1 = type->nested->items.pType[0];
	if( type->nested && type->nested->size >1 ) it2 = type->nested->items.pType[1];
	if( tokens[start]->name == DTOK_LB ){
		int rb = DaoParser_FindPairToken( parser, DTOK_LB, DTOK_RB, start, end );
		if( rb < 0 ) return next;
		sscanf( tokens[start+1]->string.chars, "%p", & key );
		DMap_Insert( self->objects, key, *value2 );
		start = rb + 1;
	}
	str = tokens[start]->string.chars;
#if 0
	printf( "type: %s %s\n", type->name->chars, str );
	for(i=start; i<=end; i++) printf( "%s ", tokens[i]->string.chars ); printf( "\n" );
#endif
	value = *value2;
	switch( type->tid ){
	case DAO_NONE :
		break;
	case DAO_BOOLEAN :
	case DAO_INTEGER :
		value->xInteger.value = DaoDecodeInteger( str );
		if( minus ) value->xInteger.value = - value->xInteger.value;
		break;
	case DAO_FLOAT :
		value->xFloat.value = DaoDecodeDouble( str );
		if( minus ) value->xFloat.value = - value->xFloat.value;
		break;
	case DAO_COMPLEX :
		value->xComplex.value.real = DaoDecodeDouble( str );
		if( minus ) value->xComplex.value.real = - value->xComplex.value.real;
		if( start + 1 > end ) return start+1;
		minus = 0;
		if( tokens[start + 1]->name == DTOK_SUB ){
			minus = 1;
			start += 1;
			if( start + 1 > end ) return start+1;
		}
		value->xComplex.value.imag = DaoDecodeDouble( tokens[start+1]->string.chars );
		if( minus ) value->xComplex.value.imag = - value->xComplex.value.imag;
		next = start + 2;
		break;
	case DAO_STRING :
		DaoDeserializeString( & tokens[start]->string, value->xString.value );
		break;
	case DAO_ENUM :
		value->xEnum.value = DaoDecodeInteger( str );
		break;
	case DAO_ARRAY :
#ifdef DAO_WITH_NUMARRAY
		if( tokens[start]->name != DTOK_LSB ) return next;
		k = DaoParser_FindPairToken( parser, DTOK_LSB, DTOK_RSB, start, end );
		if( k < 0 ) return next;
		n = 1;
		for(i=start+1; i<k; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			n *= DaoDecodeInteger( tokens[i]->string.chars );
		}
		if( n < 0 ) return next;
		if( it1 == NULL || it1->tid == 0 || it1->tid > DAO_COMPLEX ) return next;
		array = & value->xArray;
		dims = DList_New(0);
		for(i=start+1; i<k; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			j = DaoDecodeInteger( tokens[i]->string.chars );
			DList_Append( dims, (size_t) j );
		}
		n = dims->size;
		DaoArray_ResizeArray( array, dims->items.pInt, n );
		DList_PushFront( types, it1 );
		DList_Delete( dims );
		n = 0;
		for(i=k+1; i<=end; i++){
			j = i + 1;
			while( j <= end && tokens[j]->name != DTOK_COMMA ) j += 1;
			DaoSerializer_DecodeValue( self, i, j-1, & tmp );
			if( tmp != NULL ){
				switch( it1->tid ){
				case DAO_BOOLEAN : array->data.b[n] = tmp->xBoolean.value; break;
				case DAO_INTEGER : array->data.i[n] = tmp->xInteger.value; break;
				case DAO_FLOAT   : array->data.f[n] = tmp->xFloat.value; break;
				case DAO_COMPLEX : array->data.c[n] = tmp->xComplex.value; break;
				}
			}
			i = j;
			n += 1;
		}
		DList_PopFront( types );
#endif
		break;
	case DAO_LIST :
		list = (DaoList*) value;
		DList_PushFront( types, it1 );
		n = 0;
		for(i=start; i<=end; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			DList_Append( list->value, NULL );
			k = DaoSerializer_DecodeValue( self, i, end, list->value->items.pValue + n );
			i = k - 1;
			n += 1;
		}
		DList_PopFront( types );
		break;
	case DAO_MAP :
		map = (DaoMap*) value;
		n = 0;
		for(i=start; i<=end; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			DaoValue_Clear( & tmp );
			DaoValue_Clear( & tmp2 );
			DList_PushFront( types, it1 );
			i = DaoSerializer_DecodeValue( self, i, end, & tmp );
			DList_PopFront( types );
			if( tokens[i]->name == DTOK_COMMA ) continue;
			if( map->value->size == 0 ){
				if( tokens[i]->name == DTOK_ARROW ){
					DMap_Delete( map->value );
					map->value = DHash_New( DAO_DATA_VALUE, DAO_DATA_VALUE );
				}
			}
			if( tokens[i]->name == DTOK_ARROW || tokens[i]->name == DTOK_FIELD ) i += 1;
			DList_PushFront( types, it2 );
			i = DaoSerializer_DecodeValue( self, i, end, & tmp2 );
			DList_PopFront( types );
			node = DMap_Insert( map->value, (void*) tmp, (void*) tmp2 );
			i -= 1;
			n += 1;
		}
		break;
	case DAO_TUPLE :
		tuple = (DaoTuple*) value;
		n = 0;
		for(i=start; i<=end; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			it1 = NULL;
			if( type->nested && type->nested->size > n ){
				it1 = type->nested->items.pType[n];
				if( it1 && it1->tid == DAO_PAR_NAMED ) it1 = & it1->aux->xType;
			}
			DList_PushFront( types, it1 );
			i = DaoSerializer_DecodeValue( self, i, end, tuple->values + n );
			DList_PopFront( types );
			i -= 1;
			n += 1;
		}
		break;
	case DAO_OBJECT :
		DList_PushFront( types, NULL );
		DaoSerializer_DecodeValue( self, start, end, & tmp );
		DList_PopFront( types );
		if( tmp == NULL ) break;
		object = DaoSerializer_MakeObject( self, (DaoClass*) type->aux, tmp );
		if( object ) DaoValue_Copy( (DaoValue*) object, value2 );
		break;
	case DAO_CDATA :
	case DAO_CSTRUCT :
		if( type->tid == DAO_CDATA && tokens[start]->name == DTOK_LSB ){
			int rb = DaoParser_FindPairToken( parser, DTOK_LSB, DTOK_RSB, start, end );
			if( rb < 0 ) return next;
			sscanf( tokens[start+1]->string.chars, "%p", & key );
			start = rb + 1;
		}
		DList_PushFront( types, NULL );
		DaoSerializer_DecodeValue( self, start, end, & tmp );
		DList_PopFront( types );
		if( type->tid == DAO_CDATA ){
			DNode *it = DMap_Find( self->cdata, key );
			if( it != NULL ){
				DaoValue_Copy( it->value.pValue, value2 );
				break;
			}else if( tmp == NULL ){
				DaoCdata *cd = DaoCdata_Wrap( type, key );
				DaoValue_Copy( (DaoValue*) cd, value2 );
				break;
			}
		}
		if( tmp == NULL ) break;
		cstruct = DaoSerializer_MakeCstruct( self, (DaoCtype*) type->aux, tmp );
		if( cstruct ) DaoValue_Copy( (DaoValue*) cstruct, value2 );
		break;
	case DAO_CTYPE :
		DaoValue_Copy( type->aux, value2 );
		break;
	case DAO_NAMESPACE :
		DString_Reset( self->buffer, 0 );
		DaoDeserializeString( & tokens[start]->string, self->buffer );
		ns = DaoVmSpace_Load( self->nspace->vmSpace, self->buffer->chars );
		if( ns ) DaoValue_Copy( (DaoValue*) ns, value2 );
		break;
	}
	DaoValue_Clear( & tmp );
	DaoValue_Clear( & tmp2 );
	return next;
}

DList* DaoSerializer_Decode( DaoSerializer *self, DString *input )
{
	DaoRoutine *routine = NULL;
	DaoNamespace *ns = self->nspace;
	DaoParser *parser = self->parser;
	DList *types = self->types;
	int next = 0;

	self->parser = DaoVmSpace_AcquireParser( self->nspace->vmSpace );
	self->process = DaoVmSpace_AcquireProcess( self->nspace->vmSpace );

	DString_SetBytes( self->serial, input->chars, input->size );

	parser = self->parser;
	parser->nameSpace = ns;
	parser->vmSpace = ns->vmSpace;
	DaoParser_LexCode( parser, self->serial->chars, 0 );
	if( parser->tokens->size == 0 ) goto FailedReturn;

	DList_PushFront( types, NULL );
	routine = DaoRoutine_New( ns, NULL, 1 );
	GC_IncRC( routine );
	parser->routine = routine;
	while( next < parser->tokens->size ){
		DaoValue_Clear( & self->value );
		next = DaoSerializer_DecodeValue( self, next, parser->tokens->size-1, & self->value );
		if( self->value == NULL ) break; /* TODO: handle error; */
		DList_Append( self->values, self->value );
	}
	parser->routine = NULL;
	GC_DecRC( routine );
	DaoVmSpace_ReleaseParser( self->nspace->vmSpace, self->parser );
	DaoVmSpace_ReleaseProcess( self->nspace->vmSpace, self->process );
	return self->values;

FailedReturn:
	DaoVmSpace_ReleaseParser( self->nspace->vmSpace, self->parser );
	DaoVmSpace_ReleaseProcess( self->nspace->vmSpace, self->process );
	return self->values;
}




int DaoValue_Serialize2( DaoValue *self, DString *serial, DaoNamespace *ns, DaoProcess *proc, DString *tname, DString *buf, DMap *omap );

static void DString_Serialize( DString *self, DString *serial, DString *buf )
{
	int i;
	unsigned char *mbs;

	DString_Clear( buf );
	DString_Append( buf, self );
	mbs = (unsigned char*) buf->chars;
	DString_AppendChar( serial, self->chars ? '\'' : '\"' );
	for(i=0; i<buf->size; i++){
		DString_AppendChar( serial, hex_digits[ mbs[i] / 16 ] );
		DString_AppendChar( serial, hex_digits[ mbs[i] % 16 ] );
	}
	DString_AppendChar( serial, self->chars ? '\'' : '\"' );
}
static void DaoArray_Serialize( DaoArray *self, DString *serial, DString *buf )
{
	DaoInteger intmp = {DAO_INTEGER,0,0,0,0,0};
	DaoValue *value = (DaoValue*) & intmp;
	int i;
	DString_AppendChar( serial, '[' );
	for(i=0; i<self->ndim; i++){
		value->xInteger.value = self->dims[i];
		if( i ) DString_AppendChar( serial, ',' );
		DaoValue_GetString( value, buf );
		DString_Append( serial, buf );
	}
	DString_AppendChar( serial, ']' );
	for(i=0; i<self->size; i++){
		if( i ) DString_AppendChar( serial, ',' );
		switch( self->etype ){
		case DAO_BOOLEAN : DaoSerializeInteger( self->data.b[i], serial ); break;
		case DAO_INTEGER : DaoSerializeInteger( self->data.i[i], serial ); break;
		case DAO_FLOAT   : DaoSerializeDouble( self->data.f[i], serial ); break;
		case DAO_COMPLEX : DaoSerializeComplex( self->data.c[i], serial ); break;
		}
	}
}
static int DaoList_Serialize( DaoList *self, DString *serial, DaoNamespace *ns, DaoProcess *proc, DString *buf, DMap *omap )
{
	DaoType *type = self->ctype;
	int i, rc = 1;
	if( type->nested && type->nested->size ) type = type->nested->items.pType[0];
	if( type && type->noncyclic == 0 && (type->tid == 0 || type->tid >= DAO_ENUM)) type = NULL;
	for(i=0; i<self->value->size; i++){
		DaoType *it = NULL;
		if( type == NULL ) it = DaoNamespace_GetType( ns, self->value->items.pValue[i] );
		if( i ) DString_AppendChar( serial, ',' );
		if( it == NULL && type && type->tid >= DAO_ARRAY ) DString_AppendChar( serial, '{' );
		rc &= DaoValue_Serialize2( self->value->items.pValue[i], serial, ns, proc, it?it->name:NULL, buf, omap );
		if( it == NULL && type && type->tid >= DAO_ARRAY ) DString_AppendChar( serial, '}' );
	}
	return rc;
}
static int DaoMap_Serialize( DaoMap *self, DString *serial, DaoNamespace *ns, DaoProcess *proc, DString *buf, DMap *omap )
{
	DaoType *type = self->ctype;
	DaoType *keytype = NULL;
	DaoType *valtype = NULL;
	DNode *node;
	char *sep = self->value->hashing ? ":" : "=>";
	int i = 0, rc = 1;
	if( type->nested && type->nested->size >0 ) keytype = type->nested->items.pType[0];
	if( type->nested && type->nested->size >1 ) valtype = type->nested->items.pType[1];
	if( keytype && (keytype->tid == 0 || keytype->tid >= DAO_ENUM)) keytype = NULL;
	if( valtype && (valtype->tid == 0 || valtype->tid >= DAO_ENUM)) valtype = NULL;
	for(node=DMap_First(self->value); node; node=DMap_Next(self->value,node)){
		DaoType *kt = NULL, *vt = NULL;
		if( keytype == NULL ) kt = DaoNamespace_GetType( ns, node->key.pValue );
		if( valtype == NULL ) vt = DaoNamespace_GetType( ns, node->value.pValue );
		if( (i++) ) DString_AppendChar( serial, ',' );
		rc &= DaoValue_Serialize2( node->key.pValue, serial, ns, proc, kt?kt->name:NULL, buf, omap );
		DString_AppendChars( serial, sep );
		rc &= DaoValue_Serialize2( node->value.pValue, serial, ns, proc, vt?vt->name:NULL, buf, omap );
	}
	return rc;
}
static int DaoTuple_Serialize( DaoTuple *self, DString *serial, DaoNamespace *ns, DaoProcess *proc, DString *buf, DMap *omap )
{
	DList *nested = self->ctype ? self->ctype->nested : NULL;
	int i, rc = 1;
	for(i=0; i<self->size; i++){
		DaoType *type = NULL;
		DaoType *it = NULL;
		if( nested && nested->size > i ) type = nested->items.pType[i];
		if( type && type->tid == DAO_PAR_NAMED ) type = & type->aux->xType;
		if( type && (type->tid == 0 || type->tid >= DAO_ENUM)) type = NULL;
		if( type == NULL ) it = DaoNamespace_GetType( ns, self->values[i] );
		if( i ) DString_AppendChar( serial, ',' );
		rc &= DaoValue_Serialize2( self->values[i], serial, ns, proc, it?it->name:NULL, buf, omap );
	}
	return rc;
}
static int DaoObject_Serialize( DaoObject *self, DString *serial, DaoNamespace *ns, DaoProcess *proc, DString *buf, DMap *omap )
{
	DString *tname = NULL;
	DaoType *type = NULL;
	DaoValue *ret = NULL;
	DaoValue *value1 = NULL;
	DaoValue *value2 = NULL;
	DaoValue *selfpar = (DaoValue*) self;
	DString name1 = DString_WrapChars( "typename" );
	DString name2 = DString_WrapChars( "serialize" );
	int errcode1 = DaoObject_GetData( self, & name1, & value1, NULL );
	int errcode2 = DaoObject_GetData( self, & name2, & value2, NULL );
	char chs[64];
	if( errcode2 || value2 == NULL || value2->type != DAO_ROUTINE ) return 0;
	if( errcode1 == 0 && value1 != NULL && value1->type == DAO_ROUTINE ){
		if( DaoProcess_Call( proc, (DaoRoutine*)value1, selfpar, NULL, 0 ) ) return 0;
		ret = proc->stackValues[0];
		if( ret->type == DAO_STRING ) tname = DString_Copy( ret->xString.value );
	}
	if( DaoProcess_Call( proc, (DaoRoutine*)value2, selfpar, NULL, 0 ) ){
		if( tname ) DString_Delete( tname );
		return 0;
	}
	if( tname == NULL ) tname = self->defClass->className;
	type = DaoNamespace_GetType( ns, proc->stackValues[0] );

	DString_Append( serial, tname );
	DString_AppendChar( serial, '{' );
	DMap_Insert( omap, self, self );
	sprintf( chs, "(%p)", self );
	DString_AppendChars( serial, chs );

	DaoValue_Serialize2( proc->stackValues[0], serial, ns, proc, type?type->name:NULL, buf, omap );
	DString_AppendChar( serial, '}' );
	if( tname != NULL && tname != self->defClass->className ) DString_Delete( tname );
	return 1;
}
static int DaoCdata_Serialize( DaoCdata *self, DString *serial, DaoNamespace *ns, DaoProcess *proc, DString *buf, DMap *omap )
{
	DString *tname = NULL;
	DaoType *type = NULL;
	DaoValue *ret = NULL;
	DaoRoutine *meth1 = DaoType_FindFunctionChars( self->ctype, "typename" );
	DaoRoutine *meth2 = DaoType_FindFunctionChars( self->ctype, "serialize" );
	char chs[64];
	if( meth2 == NULL ) return 0;
	if( meth1 != NULL ){
		if( DaoProcess_Call( proc, meth1, (DaoValue*)self, NULL, 0 ) ) return 0;
		ret = proc->stackValues[0];
		if( ret->type == DAO_STRING ) tname = DString_Copy( ret->xString.value );
	}
	if( DaoProcess_Call( proc, meth2, (DaoValue*)self, NULL, 0 ) ){
		if( tname ) DString_Delete( tname );
		return 0;
	}
	if( tname == NULL ) tname = self->ctype->name;
	type = DaoNamespace_GetType( ns, proc->stackValues[0] );

	DString_Append( serial, tname );
	DString_AppendChar( serial, '{' );
	DMap_Insert( omap, self, self );
	sprintf( chs, "(%p)", self );
	DString_AppendChars( serial, chs );

	DaoValue_Serialize2( proc->stackValues[0], serial, ns, proc, type?type->name:NULL, buf, omap );
	DString_AppendChar( serial, '}' );
	if( tname != NULL && tname != self->ctype->name ) DString_Delete( tname );
	return 1;
}
int DaoValue_Serialize3( DaoValue *self, DString *serial, DaoNamespace *ns, DaoProcess *proc, DString *tname, DString *buf, DMap *omap )
{
	switch( self->type ){
	case DAO_OBJECT :
		if( proc == NULL ) break;
		return DaoObject_Serialize( & self->xObject, serial, ns, proc, buf, omap );
	case DAO_CDATA :
	case DAO_CSTRUCT :
		if( proc == NULL ) break;
		return DaoCdata_Serialize( & self->xCdata, serial, ns, proc, buf, omap );
	}
	return 0;
}
int DaoValue_Serialize2( DaoValue *self, DString *serial, DaoNamespace *ns, DaoProcess *proc, DString *tname, DString *buf, DMap *omap )
{
	int rc = 1;
	char chs[64];
	if( DMap_Find( omap, self ) ){
		sprintf( chs, "@{%p}", self );
		DString_AppendChars( serial, chs );
		return 1;
	}
	switch( self->type ){
	case DAO_OBJECT :
	case DAO_CDATA :
	case DAO_CSTRUCT :
		return DaoValue_Serialize3( self, serial, ns, proc, tname, buf, omap );
	}
	if( tname ){
		DString_Append( serial, tname );
		DString_AppendChar( serial, '{' );
	}
	switch( self->type ){
	case DAO_NONE :
		break;
	case DAO_INTEGER :
		DaoSerializeInteger( self->xInteger.value, serial );
		break;
	case DAO_FLOAT :
		DaoSerializeDouble( self->xFloat.value, serial );
		break;
	case DAO_COMPLEX :
		DaoSerializeComplex( self->xComplex.value, serial );
		break;
	case DAO_STRING :
		DString_Serialize( self->xString.value, serial, buf );
		break;
	case DAO_ENUM :
		DaoSerializeInteger( self->xEnum.value, serial );
		break;
	case DAO_ARRAY :
		DaoArray_Serialize( & self->xArray, serial, buf );
		break;
	case DAO_LIST :
	case DAO_MAP :
	case DAO_TUPLE :
		DMap_Insert( omap, self, self );
		sprintf( chs, "(%p)", self );
		DString_AppendChars( serial, chs );
		switch( self->type ){
		case DAO_LIST :
			rc = DaoList_Serialize( & self->xList, serial, ns, proc, buf, omap );
			break;
		case DAO_MAP :
			rc = DaoMap_Serialize( & self->xMap, serial, ns, proc, buf, omap );
			break;
		case DAO_TUPLE :
			rc = DaoTuple_Serialize( & self->xTuple, serial, ns, proc, buf, omap );
			break;
		}
		break;
	default :
		DString_AppendChar( serial, '?' );
		rc = 0;
		break;
	}
	if( tname ) DString_AppendChar( serial, '}' );
	return rc;
}
int DaoValue_Serialize( DaoValue *self, DString *serial, DaoNamespace *ns, DaoProcess *proc )
{
	DaoType *type = DaoNamespace_GetType( ns, self );
	DString *buf = DString_New();
	DMap *omap = DMap_New(0,0);
	int rc;
	DString_Clear( serial );
	rc = DaoValue_Serialize2( self, serial, ns, proc, type?type->name:NULL, buf, omap );
	DString_Delete( buf );
	DMap_Delete( omap );
	return rc;
}

int DaoParser_FindPairToken( DaoParser *self,  uchar_t lw, uchar_t rw, int start, int stop );
DaoType* DaoParser_ParseType( DaoParser *self, int start, int end, int *newpos, DList *types );

static DaoObject* DaoClass_MakeObject( DaoClass *self, DaoValue *param, DaoProcess *proc )
{
	DaoObject *object = DaoObject_New( self );
	DaoProcess_CacheValue( proc, (DaoValue*) object );
	if( DaoProcess_PushCallable( proc, self->initRoutines, (DaoValue*)object, & param, 1 ) ==0 ){
		GC_Assign( & proc->topFrame->object, object );
		proc->topFrame->returning = -1;
		if( DaoProcess_Execute( proc ) ) return object;
	}
	return NULL;
}
static DaoCdata* DaoCdata_MakeObject( DaoCdata *self, DaoValue *param, DaoProcess *proc )
{
	DaoValue *value;
	DaoRoutine *routine = DaoType_FindFunction( self->ctype, self->ctype->name );
	printf( "%p %s\n", routine, self->ctype->name->chars );
	if( DaoProcess_PushCallable( proc, routine, NULL, & param, 1 ) ) return NULL;
	proc->topFrame->active = proc->firstFrame;
	DaoProcess_SetActiveFrame( proc, proc->firstFrame ); /* return value in stackValues[0] */
	if( DaoProcess_Execute( proc ) == 0 ) return NULL;
	value = proc->stackValues[0];
	if( value && (value->type == DAO_CDATA || value->type == DAO_CSTRUCT) ) return & value->xCdata;
	return NULL;
}

/*
// Note: reference count is handled for "value2"!
//
// Item of list/tuple etc. can be directly passed as parameter "value2",
// to avoid creating unnecessary intermediate objects.
*/
static int DaoParser_Deserialize( DaoParser *self, int start, int end, DaoValue **value2, DList *types, DaoNamespace *ns, DaoProcess *proc, DMap *omap )
{
	DaoToken **tokens = self->tokens->items.pToken;
	DaoType *it1 = NULL, *it2 = NULL, *type = NULL;
	DaoValue *value = *value2;
	DaoValue *tmp = NULL;
	DaoValue *tmp2 = NULL;
	DaoObject *object;
	DaoCdata *cdata;
	DaoArray *array;
	DaoTuple *tuple;
	DaoList *list;
	DaoMap *map;
	DList *dims;
	DNode *node;
	void *key = NULL;
	char *str;
	int i, j, k, n;
	int minus = 0;
	int next = start + 1;
	int tok2 = start < end ? tokens[start+1]->type : 0;
	int maybetype = tok2 == DTOK_COLON2 || tok2 == DTOK_LT || tok2 == DTOK_LCB;

	if( tokens[start]->type == DTOK_AT && tok2 == DTOK_LCB ){
		int rb = DaoParser_FindPairToken( self, DTOK_LCB, DTOK_RCB, start, end );
		if( rb < 0 ) return next;
		sscanf( tokens[start+2]->string.chars, "%p", & key );
		node = DMap_Find( omap, key );
		if( node ) DaoValue_Copy( node->value.pValue, value2 );
		return rb + 1;
	}
	if( tokens[start]->name == DTOK_ID_SYMBOL ){
		DString *mbs = DString_New();
		while( tokens[start]->name == DTOK_ID_SYMBOL ){
			DString_Append( mbs, & tokens[start]->string );
			start += 1;
		}
		type = DaoNamespace_MakeType( ns, mbs->chars, DAO_ENUM, NULL, NULL, 0 );
		DString_Delete( mbs );
		if( type == NULL ) return start;
		if( tokens[start]->name != DTOK_LCB ) return start;
		end = DaoParser_FindPairToken( self, DTOK_LCB, DTOK_RCB, start, end );
		if( end < 0 ) return start;
		next = end + 1;
		start += 1;
		end -= 1;
	}else if( tokens[start]->type == DTOK_IDENTIFIER && maybetype ){
		type = DaoParser_ParseType( self, start, end, & start, NULL );
		if( type == NULL ) return next;
		if( tokens[start]->name != DTOK_LCB ) return start;
		end = DaoParser_FindPairToken( self, DTOK_LCB, DTOK_RCB, start, end );
		if( end < 0 ) return start;
		next = end + 1;
		start += 1;
		end -= 1;
	}
	if( type == NULL ){
		type = types->items.pType[0];
		if( type && type->tid >= DAO_ARRAY ){
			if( tokens[start]->name != DTOK_LCB ) return start;
			end = DaoParser_FindPairToken( self, DTOK_LCB, DTOK_RCB, start, end );
			if( end < 0 ) return start;
			next = end + 1;
			start += 1;
			end -= 1;
		}
	}
	if( type == NULL ) return next;
	DaoValue_Copy( type->value, value2 );
	if( start > end ) return next;
	if( tokens[start]->name == DTOK_SUB ){
		minus = 1;
		start += 1;
		if( start > end ) return next;
	}
	if( type->nested && type->nested->size >0 ) it1 = type->nested->items.pType[0];
	if( type->nested && type->nested->size >1 ) it2 = type->nested->items.pType[1];
	if( tokens[start]->name == DTOK_LB ){
		int rb = DaoParser_FindPairToken( self, DTOK_LB, DTOK_RB, start, end );
		if( rb < 0 ) return next;
		sscanf( tokens[start+1]->string.chars, "%p", & key );
		DMap_Insert( omap, key, *value2 );
		start = rb + 1;
	}
	str = tokens[start]->string.chars;
#if 0
	printf( "type: %s %s\n", type->name->chars, str );
	for(i=start; i<=end; i++) printf( "%s ", tokens[i]->string.chars ); printf( "\n" );
#endif
	value = *value2;
	switch( type->tid ){
	case DAO_NONE :
		break;
	case DAO_BOOLEAN :
	case DAO_INTEGER :
		value->xInteger.value = DaoDecodeInteger( str );
		if( minus ) value->xInteger.value = - value->xInteger.value;
		break;
	case DAO_FLOAT :
		value->xFloat.value = DaoDecodeDouble( str );
		if( minus ) value->xFloat.value = - value->xFloat.value;
		break;
	case DAO_COMPLEX :
		value->xComplex.value.real = DaoDecodeDouble( str );
		if( minus ) value->xComplex.value.real = - value->xComplex.value.real;
		if( start + 1 > end ) return start+1;
		minus = 0;
		if( tokens[start + 1]->name == DTOK_SUB ){
			minus = 1;
			start += 1;
			if( start + 1 > end ) return start+1;
		}
		value->xComplex.value.imag = DaoDecodeDouble( tokens[start+1]->string.chars );
		if( minus ) value->xComplex.value.imag = - value->xComplex.value.imag;
		next = start + 2;
		break;
	case DAO_STRING :
		n = tokens[start]->string.size - 1;
		for(i=1; i<n; i++){
			char c1 = str[i];
			char c2 = str[i+1];
			if( c1 < 'A' || c1 > 'P' ) continue;
			DString_AppendChar( value->xString.value, (char)((c1-'A')*16 + (c2-'A')) );
			i += 1;
		}
		break;
	case DAO_ENUM :
		value->xEnum.value = DaoDecodeInteger( str );
		break;
	case DAO_ARRAY :
#ifdef DAO_WITH_NUMARRAY
		if( tokens[start]->name != DTOK_LSB ) return next;
		k = DaoParser_FindPairToken( self, DTOK_LSB, DTOK_RSB, start, end );
		if( k < 0 ) return next;
		n = 1;
		for(i=start+1; i<k; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			n *= strtol( tokens[i]->string.chars, 0, 0 );
		}
		if( n < 0 ) return next;
		if( it1 == NULL || it1->tid == 0 || it1->tid > DAO_COMPLEX ) return next;
		array = & value->xArray;
		dims = DList_New(0);
		for(i=start+1; i<k; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			j = strtol( tokens[i]->string.chars, 0, 0 );
			DList_Append( dims, (size_t) j );
		}
		n = dims->size;
		DaoArray_ResizeArray( array, dims->items.pInt, n );
		DList_PushFront( types, it1 );
		DList_Delete( dims );
		n = 0;
		for(i=k+1; i<=end; i++){
			j = i + 1;
			while( j <= end && tokens[j]->name != DTOK_COMMA ) j += 1;
			DaoParser_Deserialize( self, i, j-1, & tmp, types, ns, proc, omap );
			switch( it1->tid ){
			case DAO_BOOLEAN : array->data.b[n] = tmp->xBoolean.value; break;
			case DAO_INTEGER : array->data.i[n] = tmp->xInteger.value; break;
			case DAO_FLOAT   : array->data.f[n] = tmp->xFloat.value; break;
			case DAO_COMPLEX : array->data.c[n] = tmp->xComplex.value; break;
			}
			i = j;
			n += 1;
		}
		DList_PopFront( types );
#endif
		break;
	case DAO_LIST :
		list = & value->xList;
		DList_PushFront( types, it1 );
		n = 0;
		for(i=start; i<=end; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			DList_Append( list->value, NULL );
			k = DaoParser_Deserialize( self, i, end, list->value->items.pValue + n, types, ns, proc, omap );
			i = k - 1;
			n += 1;
		}
		DList_PopFront( types );
		break;
	case DAO_MAP :
		map = & value->xMap;
		n = 0;
		for(i=start; i<=end; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			DaoValue_Clear( & tmp );
			DaoValue_Clear( & tmp2 );
			DList_PushFront( types, it1 );
			i = DaoParser_Deserialize( self, i, end, &tmp, types, ns, proc, omap );
			DList_PopFront( types );
			if( tokens[i]->name == DTOK_COMMA ) continue;
			if( map->value->size == 0 ){
				if( tokens[i]->name == DTOK_COLON ){
					DMap_Delete( map->value );
					map->value = DHash_New( DAO_DATA_VALUE, DAO_DATA_VALUE );
				}
			}
			if( tokens[i]->name == DTOK_COLON || tokens[i]->name == DTOK_FIELD ) i += 1;
			DList_PushFront( types, it2 );
			i = DaoParser_Deserialize( self, i, end, &tmp2, types, ns, proc, omap );
			DList_PopFront( types );
			node = DMap_Insert( map->value, (void*) tmp, (void*) tmp2 );
			i -= 1;
			n += 1;
		}
		break;
	case DAO_TUPLE :
		tuple = & value->xTuple;
		n = 0;
		for(i=start; i<=end; i++){
			if( tokens[i]->name == DTOK_COMMA ) continue;
			it1 = NULL;
			if( type->nested && type->nested->size > n ){
				it1 = type->nested->items.pType[n];
				if( it1 && it1->tid == DAO_PAR_NAMED ) it1 = & it1->aux->xType;
			}
			DList_PushFront( types, it1 );
			i = DaoParser_Deserialize( self, i, end, tuple->values + n, types, ns, proc, omap );
			DList_PopFront( types );
			i -= 1;
			n += 1;
		}
		break;
	case DAO_OBJECT :
		DList_PushFront( types, NULL );
		DaoParser_Deserialize( self, start, end, & tmp, types, ns, proc, omap );
		DList_PopFront( types );
		if( tmp == NULL ) break;
		object = DaoClass_MakeObject( & type->aux->xClass, tmp, proc );
		if( object ) DaoValue_Copy( (DaoValue*) object, value2 );
		break;
	case DAO_CDATA :
	case DAO_CSTRUCT :
		DList_PushFront( types, NULL );
		DaoParser_Deserialize( self, start, end, & tmp, types, ns, proc, omap );
		DList_PopFront( types );
		if( tmp == NULL ) break;
		cdata = DaoCdata_MakeObject( & type->aux->xCdata, tmp, proc );
		if( cdata ) DaoValue_Copy( (DaoValue*) cdata, value2 );
		break;
	}
	DaoValue_Clear( & tmp );
	DaoValue_Clear( & tmp2 );
	return next;
}
/*
// Note: reference count is not handled for "self"!
// But it is cached in the DaoProcess object, so no need to handle it by user!
*/
int DaoValue_Deserialize( DaoValue **self, DString *serial, DaoNamespace *ns, DaoProcess *proc )
{
	DaoRoutine *routine = NULL;
	DaoParser *parser = DaoVmSpace_AcquireParser( ns->vmSpace );
	DList *types = DList_New(0);
	DMap *omap = DMap_New(0,0);
	int rc;

	*self = NULL;
	parser->nameSpace = ns;
	parser->vmSpace = ns->vmSpace;
	DaoParser_LexCode( parser, DString_GetData( serial ), 0 );
	if( parser->tokens->size == 0 ) goto Failed;

	DList_PushFront( types, NULL );
	routine = DaoRoutine_New( ns, NULL, 1 );
	GC_IncRC( routine );
	parser->routine = routine;
	rc = DaoParser_Deserialize( parser, 0, parser->tokens->size-1, self, types, ns, proc, omap );
	if( *self ) DaoProcess_CacheValue( proc, *self );
	parser->routine = NULL;
	GC_DecRC( routine );
	DaoVmSpace_ReleaseParser( ns->vmSpace, parser );
	DList_Delete( types );
	DMap_Delete( omap );
	return rc;
Failed:
	DaoVmSpace_ReleaseParser( ns->vmSpace, parser );
	DList_Delete( types );
	DMap_Delete( omap );
	return 0;
}


static void NS_Backup( DaoNamespace *self, DaoProcess *proc, FILE *fout, int limit, int store )
{
	DNode *node = DMap_First( self->lookupTable );
	DString *prefix = DString_New();
	DString *serial = DString_New();
	DaoValue *value = NULL;
	size_t max = limit * 1000; /* limit per object in KB */
	int id, pm, up, st;

	for( ; node !=NULL; node = DMap_Next( self->lookupTable, node ) ){
		DString *name = node->key.pString;
		id = node->value.pInt;
		up = LOOKUP_UP( id );
		st = LOOKUP_ST( id );
		pm = LOOKUP_PM( id );
		if( up ) continue;
		if( st != store ) continue;
		if( st == DAO_GLOBAL_CONSTANT ) value = DaoNamespace_GetConst( self, id );
		if( st == DAO_GLOBAL_VARIABLE ) value = DaoNamespace_GetVariable( self, id );
		if( value == NULL ) continue;
		if( DaoValue_Serialize( value, serial, self, proc ) ==0 ) continue;
		prefix->size = 0;
		switch( pm ){
		case DAO_PERM_PRIVATE   : DString_AppendChars( prefix, "private " ); break;
		case DAO_PERM_PROTECTED : DString_AppendChars( prefix, "protected " ); break;
		case DAO_PERM_PUBLIC    : DString_AppendChars( prefix, "public " ); break;
		}
		switch( st ){
		case DAO_GLOBAL_CONSTANT : DString_AppendChars( prefix, "const " ); break;
		case DAO_GLOBAL_VARIABLE : DString_AppendChars( prefix, "var " ); break;
		}
		if( max && prefix->size + name->size + serial->size + 4 > max ) continue;
		fprintf( fout, "%s%s = %s\n", prefix->chars, name->chars, serial->chars );
	}
	DString_Delete( prefix );
	DString_Delete( serial );
}
void DaoNamespace_Backup( DaoNamespace *self, DaoProcess *proc, FILE *fout, int limit )
{
	int i;
	NS_Backup( self, proc, fout, limit, DAO_GLOBAL_CONSTANT );
	if( self->inputs->size ){ /* essential statements and definitions */
		static const char *digits = "ABCDEFGHIJKLMNOP";
		unsigned char *mbs = (unsigned char*) self->inputs->chars;
		fprintf( fout, "inputs { " );
		for(i=0; i<self->inputs->size; i++){
			fprintf( fout, "%c%c", digits[ mbs[i] / 16 ], digits[ mbs[i] % 16 ] );
		}
		fprintf( fout, " }\n" );
	}
	NS_Backup( self, proc, fout, limit, DAO_GLOBAL_VARIABLE );
}
void DaoNamespace_Restore( DaoNamespace *self, DaoProcess *proc, FILE *fin )
{
	DaoParser *parser = DaoVmSpace_AcquireParser( self->vmSpace );
	DString *line = DString_New();
	DList *types = DList_New(0);
	DList *tokens = parser->tokens;
	DMap *omap = DMap_New(0,0);
	DString *name;
	DNode *node;

	parser->nameSpace = self;
	parser->vmSpace = self->vmSpace;
	while( DaoFile_ReadLine( fin, line ) ){
		DaoValue *value = NULL;
		int st = DAO_GLOBAL_VARIABLE;
		int pm = DAO_PERM_PRIVATE;
		int i, n, start = 0;
		char *mbs;

		DaoParser_LexCode( parser, line->chars, 0 );
		if( tokens->size == 0 ) continue;
		name = & tokens->items.pToken[start]->string;
		if( name->size == 6 && strcmp( name->chars, "inputs" ) == 0 ){
			if( tokens->size < 3 ) continue;
			DString_Clear( line );
			n = tokens->items.pToken[start+2]->string.size;
			mbs = tokens->items.pToken[start+2]->string.chars;
			for(i=0; i<n; i++){
				char c1 = mbs[i];
				char c2 = mbs[i+1];
				if( c1 < 'A' || c1 > 'P' ) continue;
				DString_AppendChar( line, (char)((c1-'A')*16 + (c2-'A')) );
				i += 1;
			}
			/* printf( "%s\n", line->chars ); */
			DaoProcess_Eval( proc, self, line->chars );
			continue;
		}
		switch( tokens->items.pToken[start]->name ){
		case DKEY_PRIVATE   : pm = DAO_PERM_PRIVATE;   start += 1; break;
		case DKEY_PROTECTED : pm = DAO_PERM_PROTECTED; start += 1; break;
		case DKEY_PUBLIC    : pm = DAO_PERM_PUBLIC;    start += 1; break;
		}
		if( start >= tokens->size ) continue;
		switch( tokens->items.pToken[start]->name ){
		case DKEY_CONST : st = DAO_GLOBAL_CONSTANT; start += 1; break;
		case DKEY_VAR   : st = DAO_GLOBAL_VARIABLE; start += 1; break;
		}
		if( tokens->items.pToken[start]->name != DTOK_IDENTIFIER ) continue;
		name = & tokens->items.pToken[start]->string;
		start += 1;
		if( start + 3 >= tokens->size ) continue;
		if( tokens->items.pToken[start]->name != DTOK_ASSN ) continue;
		start += 1;

		DList_Clear( parser->errors );
		DList_Clear( types );
		DList_PushFront( types, NULL );
		DaoParser_Deserialize( parser, start, tokens->size-1, &value, types, self, proc, omap );
		if( value == NULL ) continue;
		node = DMap_Find( self->lookupTable, name );
		if( node ) continue;
		if( st == DAO_GLOBAL_CONSTANT ){
			DaoNamespace_AddConst( self, name, value, pm );
		}else{
			DaoNamespace_AddVariable( self, name, value, NULL, pm );
		}
	}
	DMap_Delete( omap );
	DString_Delete( line );
	DList_Delete( types );
	DaoVmSpace_ReleaseParser( self->vmSpace, parser );
}





static void AUX_Serialize( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *mbs = DaoProcess_PutChars( proc, "" );
	DaoValue_Serialize( p[0], mbs, proc->activeNamespace, proc );
}
static void AUX_Deserialize( DaoProcess *proc, DaoValue *p[], int N )
{
	int top = proc->factory->size;
	DaoValue *value = NULL;
	DaoValue_Deserialize( & value, p[0]->xString.value, proc->activeNamespace, proc );
	if( value == NULL ) DaoProcess_RaiseError( proc, "Deserialize", "deserialization failed" );
	DaoProcess_PutValue( proc, value );
	DaoProcess_PopValues( proc, proc->factory->size - top );
	GC_DecRC( value );
}
static void AUX_Backup( DaoProcess *proc, DaoValue *p[], int N )
{
	FILE *fout = fopen( DString_GetData( p[0]->xString.value ), "w+" );
	if( fout == NULL ){
		DaoProcess_RaiseError( proc, "File", DString_GetData( p[0]->xString.value ) );
		return;
	}
	DaoNamespace_Backup( proc->activeNamespace, proc, fout, p[1]->xInteger.value );
	fclose( fout );
}
static void AUX_Restore( DaoProcess *proc, DaoValue *p[], int N )
{
	FILE *fin = fopen( DString_GetData( p[0]->xString.value ), "r" );
	if( fin == NULL ){
		DaoProcess_RaiseError( proc, "File", DString_GetData( p[0]->xString.value ) );
		return;
	}
	DaoNamespace_Restore( proc->activeNamespace, proc, fin );
	fclose( fin );
}

static DaoFuncItem serializerMeths[]=
{
	/*! Serializes \a value to text. For a class instance to be serializeable,
	  its class should define `serialize()` method returning instance-related
	  data to be serialized */
	{ AUX_Serialize,   "serialize( invar value : any )=>string" },

	/*! Deserializes value from \a text. For a class instance to be deserializable,
	  its class should provide constructor compatible with the data returned by the
	  `serialize()` method; if the latter returns a tuple, the constructor should
	  accept parameters corresponding to the individual fields of that tuple */
	{ AUX_Deserialize, "deserialize( text : string )=>any" },

	/*! Saves the current state of the program to the file specified by \a dest;
	  if \a limit is greater then 0, objects whose serialized size exceeds
	  \a limit * 1000 bytes are not	included in the backup */
	{ AUX_Backup,      "backup( dest = \"backup.sdo\", limit=0 )" },

	/*! Restores previously saved program state from the file specified by \a source */
	{ AUX_Restore,     "restore( source = \"backup.sdo\" )" },
	{ NULL, NULL }
};


DAO_SERIAL_DLL int DaoSerializer_OnLoad2( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	ns = DaoVmSpace_GetNamespace( vmSpace, "std" );
	DaoNamespace_WrapFunctions( ns, serializerMeths );
	return 0;
}



static void SERIAL_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSerializer *self = DaoSerializer_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SERIAL_Encode( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *serial;
	DaoSerializer *self = (DaoSerializer*) p[0];

	DaoSerializer_Reset( self, proc->activeNamespace );
	serial = DaoSerializer_Encode( self, p + 1, N-1 );
	DaoProcess_PutString( proc, serial );
	DaoSerializer_Reset( self, NULL );
}
static void SERIAL_Decode( DaoProcess *proc, DaoValue *p[], int N )
{
	DList *values;
	DaoList *result = DaoProcess_PutList( proc );
	DaoSerializer *self = (DaoSerializer*) p[0];

	DaoSerializer_Reset( self, proc->activeNamespace );
	values = DaoSerializer_Decode( self, p[1]->xString.value );
	//if( value == NULL ) DaoProcess_RaiseError( proc, "Serializer", "deserialization failed" );
	DList_AppendList( result->value, values );
	DaoSerializer_Reset( self, NULL );
}

static DaoFuncItem DaoSerializerMeths[]=
{
	{ SERIAL_New,  "Serializer()" },

	/*! Serializes \a value to text. For a class instance to be serializeable,
	  its class should define `serialize()` method returning instance-related
	  data to be serialized */
	{ SERIAL_Encode,  "encode( self: Serializer, invar value: any, ... : any ) => string" },

	/*! Deserializes value from \a text. For a class instance to be deserializable,
	  its class should provide constructor compatible with the data returned by the
	  `serialize()` method; if the latter returns a tuple, the constructor should
	  accept parameters corresponding to the individual fields of that tuple */
	{ SERIAL_Decode,  "decode( self: Serializer, text: string ) => list<any>" },

	{ NULL, NULL }
};

static void DaoSerializer_GetGCFields( void *p, DList *values, DList *lists, DList *maps, int remove )
{
	DaoSerializer *self = (DaoSerializer*) p;
	DList_Append( lists, self->values );
	if( self->value ) DList_Append( values, self->value );
	if( remove ){
		self->value = NULL;
	}
}

DaoTypeBase DaoSerializer_Typer =
{
	"Serializer", NULL, NULL, (DaoFuncItem*) DaoSerializerMeths, { NULL }, { NULL },
	(FuncPtrDel)DaoSerializer_Delete, DaoSerializer_GetGCFields
};

#undef DAO_SERIAL
#undef DAO_SERIAL_DLL
#define DAO_HAS_SERIAL
#include"dao_api.h"

DAO_DLL_EXPORT int DaoSerializer_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_serializer = DaoNamespace_WrapType( ns, & DaoSerializer_Typer, DAO_CSTRUCT, 0 );

#define DAO_API_INIT
#include"dao_api.h"

	return 0;
}

