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

#include"dao_scanner.h"

static DaoType *daox_type_scanner = NULL;

DaoScanner* DaoScanner_New()
{
	DaoScanner *self = (DaoScanner*)dao_malloc( sizeof(DaoScanner) );
	self->regex = NULL;
	self->context = NULL;
	self->pos = 0;
	self->start = -1;
	self->end = -1;
	return self;
}

void DaoScanner_Delete( DaoScanner *self )
{
	if ( self->context )
		DString_Delete( self->context );
	dao_free( self );
}

static void DaoScanner_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = DaoScanner_New();
	DString *str = p[0]->xString.data;
	daoint pos = p[1]->xInteger.value;
	if ( pos < 0 )
		pos = str->size + pos;
	self->pos = ( pos > str->size )? str->size : pos;
	self->context = DString_New( str->mbs != NULL );
	DString_Assign( self->context, str );
	DaoProcess_PutCdata( proc, self, daox_type_scanner );
}

static void DaoScanner_Context( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, self->context );
}

static void DaoScanner_Position( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->pos );
}

static void DaoScanner_SetPos( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	daoint pos = p[1]->xInteger.value;
	if ( pos < 0 )
		pos = self->context->size + pos;
	self->pos = ( pos > self->context->size )? self->context->size : pos;
}

static void DaoScanner_AtEnd( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->pos == self->context->size );
}

static void DaoScanner_Append( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DString_Append( self->context, p[1]->xString.data );
}

static void DaoScanner_Fetch( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	daoint count = p[1]->xInteger.value;
	DString *sub;
	if ( count < 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "Invalid number of characters" );
		return;
	}
	sub = DString_New( 1 );
	if ( self->pos < self->context->size ){
		if ( self->pos + count > self->context->size )
			count = self->context->size - self->pos;
		DString_SubString( self->context, sub, self->pos, count );
		self->pos += count;
	}
	DaoProcess_PutString( proc, sub );
	DString_Delete( sub );
}

static void DaoScanner_ScanSeek( DaoProcess *proc, DaoValue *p[], int seek )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DString *pt = p[1]->xString.data;
	int del = 0;
	daoint res = 0;
	if ( !seek && pt->size ){
		int i;
		if ( pt->mbs )
			for ( i = 0; i < pt->size && isspace( pt->mbs[i] ); i++ );
		else
			for ( i = 0; i < pt->size && iswspace( pt->wcs[i] ); i++ );
		if ( i >= pt->size || pt->mbs? ( pt->mbs[i] != '^' ) : ( pt->wcs[i] != L'^' ) ){
			pt = pt->mbs? DString_NewMBS( "^" ) : DString_NewWCS( L"^" );
			DString_Append( pt, p[1]->xString.data );
			del = 1;
		}
	}
	self->regex = DaoProcess_MakeRegex( proc, pt, self->context->mbs != NULL );
	if ( del )
		DString_Delete( pt );
	if ( self->regex == NULL )
		return;
	if ( self->pos < self->context->size ){
		self->start = self->pos;
		self->end = self->context->size - 1;
		if ( DaoRegex_Match( self->regex, self->context, &self->start, &self->end ) ){
			res = self->end - self->pos + 1;
			self->pos = self->end + 1;
		}
	}
	else {
		self->start = -1;
		self->end = -1;
	}
	DaoProcess_PutInteger( proc, res );
}

static void DaoScanner_Scan( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner_ScanSeek( proc, p, 0 );
}

static void DaoScanner_Seek( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner_ScanSeek( proc, p, 1 );
}

static void DaoScanner_Matched( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	daoint group = p[1]->xInteger.value;
	DString *res = DString_New( self->context->mbs != NULL );
	if ( self->regex && self->start >= 0 ){
		if ( group ){
			daoint start = self->start, end = self->end;
			if ( DaoRegex_SubMatch( self->regex, group, &start, &end ) )
				DString_SubString( self->context, res, start, end - start + 1 );
		}
		else
			DString_SubString( self->context, res, self->start, self->end - self->start + 1 );
	}
	DaoProcess_PutString( proc, res );
	DString_Delete( res );
}

static void DaoScanner_MatchedAt( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	daoint group = p[1]->xInteger.value;
	DaoTuple *res = DaoProcess_PutTuple( proc, 2 );
	res->items[0]->xInteger.value = -1;
	res->items[1]->xInteger.value = self->context->size;
	if ( self->regex && self->start >= 0 ){
		if ( group ){
			daoint start = self->start, end = self->end;
			if ( DaoRegex_SubMatch( self->regex, group, &start, &end ) ){
				res->items[0]->xInteger.value = start;
				res->items[1]->xInteger.value = end;
			}
		}
		else {
			res->items[0]->xInteger.value = self->start;
			res->items[1]->xInteger.value = self->end;
		}
	}
}

static void DaoScanner_Line( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	daoint res = 1, i;
	if ( self->context->mbs ){
		for ( i = 0; i < self->pos; i++ )
			if ( self->context->mbs[i] == '\n' )
				res++;
	}
	else
		for ( i = 0; i < self->pos; i++ )
			if ( self->context->wcs[i] == L'\n' )
				res++;
	DaoProcess_PutInteger( proc, res );
}

static void DaoScanner_Follows( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DaoRegex *reg;
	DString *pt = p[1]->xString.data;
	int del = 0;
	daoint res = 0;
	if ( pt->size ){
		int i;
		if ( pt->mbs )
			for ( i = pt->size - 1; i >= 0 && isspace( pt->mbs[i] ); i-- );
		else
			for ( i = pt->size - 1; i >= 0 && iswspace( pt->wcs[i] ); i-- );
		if ( i < 0 || pt->mbs? ( pt->mbs[i] != '$' ) : ( pt->wcs[i] != L'$' ) ){
			pt = DString_Copy( pt );
			if ( pt->mbs )
				DString_AppendChar( pt, '$' );
			else
				DString_AppendWChar( pt, L'$' );
			del = 1;
		}
	}
	reg = DaoProcess_MakeRegex( proc, pt, self->context->mbs != NULL );
	if ( del )
		DString_Delete( pt );
	if ( self->regex == NULL )
		return;
	if ( self->pos > 0 || self->start > 0 ){
		daoint start = 0, end = ( self->start <= 0 ? self->pos : self->start ) - 1;
		if ( DaoRegex_Match( reg, self->context, &start, &end ) )
			res = 1;
	}
	DaoProcess_PutInteger( proc, res );
}

static void DaoScanner_Precedes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DaoRegex *reg;
	DString *pt = p[1]->xString.data;
	int del = 0;
	daoint res = 0;
	if ( pt->size ){
		int i;
		if ( pt->mbs )
			for ( i = 0; i < pt->size && isspace( pt->mbs[i] ); i++ );
		else
			for ( i = 0; i < pt->size && iswspace( pt->wcs[i] ); i++ );
		if ( i >= pt->size || pt->mbs? ( pt->mbs[i] != '^' ) : ( pt->wcs[i] != L'^' ) ){
			pt = pt->mbs? DString_NewMBS( "^" ) : DString_NewWCS( L"^" );
			DString_Append( pt, p[1]->xString.data );
			del = 1;
		}
	}
	reg = DaoProcess_MakeRegex( proc, pt, self->context->mbs != NULL );
	if ( del )
		DString_Delete( pt );
	if ( self->regex == NULL )
		return;
	if ( self->end >= 0 || self->pos < self->context->size ){
		daoint start = self->end >= 0? self->end + 1 : self->pos, end = self->context->size - 1;
		if ( DaoRegex_Match( reg, self->context, &start, &end ) )
			res = 1;
	}
	DaoProcess_PutInteger( proc, res );
}

static DaoFuncItem scannerMeths[] =
{
	{ DaoScanner_Create,	"scanner(context: string, pos = 0) => scanner" },
	{ DaoScanner_Context,	"context(self: scanner) => string" },
	{ DaoScanner_Position,	"pos(self: scanner) => int" },
	{ DaoScanner_SetPos,	"pos(self: scanner, pos: int)" },
	{ DaoScanner_AtEnd,		"at_end(self: scanner) => int" },
	{ DaoScanner_Append,	"append(self: scanner, context: string)" },
	{ DaoScanner_Fetch,		"fetch(self: scanner, count: int) => string" },
	{ DaoScanner_Scan,		"scan(self: scanner, pt: string) => int" },
	{ DaoScanner_Seek,		"seek(self: scanner, pt: string) => int" },
	{ DaoScanner_Matched,	"matched(self: scanner, group = 0) => string" },
	{ DaoScanner_MatchedAt,	"matched_at(self: scanner, group = 0) => tuple<start: int, end: int>" },
	{ DaoScanner_Line,		"line(self: scanner) => int" },
	{ DaoScanner_Follows,	"follows(self: scanner, pt: string) => int" },
	{ DaoScanner_Precedes,	"precedes(self: scanner, pt: string) => int" },
	{ NULL, NULL }
};

DaoTypeBase scannerTyper = {
	"scanner", NULL, NULL, scannerMeths, {NULL}, {0},
	(FuncPtrDel)DaoScanner_Delete, NULL
};

DAO_DLL int DaoScanner_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_scanner = DaoNamespace_WrapType( ns, &scannerTyper, 1 );
	return 0;
}
