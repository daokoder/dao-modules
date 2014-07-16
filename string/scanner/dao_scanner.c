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

// 2014-01: Danilov Aleksey, initial implementation.

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
	DString *str = p[0]->xString.value;
	daoint pos = p[1]->xInteger.value;
	if ( pos < 0 ) pos = str->size + pos;
	self->pos = ( pos > str->size )? str->size : pos;
	self->context = DString_New();
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

static void DaoScanner_Rest( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->context->size - self->pos );
}

static void DaoScanner_Append( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DString_Append( self->context, p[1]->xString.value );
}

static void DaoScanner_FetchPeek( DaoProcess *proc, DaoValue *p[], int fetch )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	daoint count = p[1]->xInteger.value;
	DString *sub;
	if ( count < 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid number of bytes" );
		return;
	}
	sub = DString_New();
	if ( self->pos < self->context->size ){
		if ( self->pos + count > self->context->size )
			count = self->context->size - self->pos;
		DString_SubString( self->context, sub, self->pos, count );
		if ( fetch )
			self->pos += count;
	}
	DaoProcess_PutString( proc, sub );
	DString_Delete( sub );
}

static void DaoScanner_Fetch( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner_FetchPeek( proc, p, 1 );
}

static void DaoScanner_Peek( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner_FetchPeek( proc, p, 0 );
}

static void DaoScanner_ScanSeek( DaoProcess *proc, DaoValue *p[], int seek )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DString *pt = p[1]->xString.value;
	int del = 0;
	daoint res = 0;
	if ( !seek && pt->size ){
		int i;
		for ( i = 0; i < pt->size && isspace( pt->chars[i] ); i++ );
		if ( i >= pt->size || pt->chars[i] != '^' ){
			pt = DString_NewChars( "^" );
			DString_Append( pt, p[1]->xString.value );
			del = 1;
		}
	}
	self->regex = DaoProcess_MakeRegex( proc, pt );
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
	DString *res = DString_New();
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
	if ( self->regex && self->start >= 0 ){
		DaoTuple *res = DaoProcess_PutTuple( proc, 2 );
		res->values[0]->xInteger.value = -1;
		res->values[1]->xInteger.value = self->context->size;
		if ( group ){
			daoint start = self->start, end = self->end;
			if ( DaoRegex_SubMatch( self->regex, group, &start, &end ) ){
				res->values[0]->xInteger.value = start;
				res->values[1]->xInteger.value = end;
			}
		}
		else {
			res->values[0]->xInteger.value = self->start;
			res->values[1]->xInteger.value = self->end;
		}
	}
	else
		DaoProcess_PutNone( proc );
}

static void DaoScanner_Line( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	daoint res = 1, i;
	for ( i = 0; i < self->pos; i++ )
		if ( self->context->chars[i] == '\n' )
			res++;
	DaoProcess_PutInteger( proc, res );
}

static void DaoScanner_Follows( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DaoRegex *reg;
	DString *pt = p[1]->xString.value;
	int del = 0;
	daoint res = 0;
	if ( pt->size ){
		int i;
		for ( i = pt->size - 1; i >= 0 && isspace( pt->chars[i] ); i-- );
		if ( i < 0 || pt->chars[i] != '$' ){
			pt = DString_Copy( pt );
			DString_AppendChar( pt, '$' );
			del = 1;
		}
	}
	reg = DaoProcess_MakeRegex( proc, pt );
	if ( del )
		DString_Delete( pt );
	if ( self->regex == NULL )
		return;
	if ( self->pos > 0 || self->start > 0 ){
		daoint start = 0, end = ( self->start <= 0 ? self->pos : self->start ) - 1;
		if ( DaoRegex_Match( reg, self->context, &start, &end ) )
			res = 1;
	}
	DaoProcess_PutEnum( proc, res? "true" : "false" );
}

static void DaoScanner_Precedes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoScanner *self = (DaoScanner*)DaoValue_TryGetCdata( p[0] );
	DaoRegex *reg;
	DString *pt = p[1]->xString.value;
	int del = 0;
	daoint res = 0;
	if ( pt->size ){
		int i;
		for ( i = 0; i < pt->size && isspace( pt->chars[i] ); i++ );
		if ( i >= pt->size || pt->chars[i] != '^' ){
			pt = DString_NewChars( "^" );
			DString_Append( pt, p[1]->xString.value );
			del = 1;
		}
	}
	reg = DaoProcess_MakeRegex( proc, pt );
	if ( del )
		DString_Delete( pt );
	if ( self->regex == NULL )
		return;
	if ( self->end >= 0 || self->pos < self->context->size ){
		daoint start = self->end >= 0? self->end + 1 : self->pos, end = self->context->size - 1;
		if ( DaoRegex_Match( reg, self->context, &start, &end ) )
			res = 1;
	}
	DaoProcess_PutEnum( proc, res? "true" : "false" );
}

static DaoFuncItem scannerMeths[] =
{
	/*! Constructs scanner operating on string \a context starting at position \a pos */
	{ DaoScanner_Create,	"scanner(context: string, pos = 0) => scanner" },

	/*! String being scanned */
	{ DaoScanner_Context,	".context(invar self: scanner) => string" },

	/*! Current position */
	{ DaoScanner_Position,	".pos(invar self: scanner) => int" },
	{ DaoScanner_SetPos,	".pos(self: scanner, pos: int)" },

	/*! Number of bytes left to scan (from the current position to the end of the context) */
	{ DaoScanner_Rest,		".rest(invar self: scanner) => int" },

	/*! Appends \a str to the context string */
	{ DaoScanner_Append,	"append(self: scanner, str: string)" },

	/*! Returns \a count bytes starting from the current position and advances the scanner */
	{ DaoScanner_Fetch,		"fetch(self: scanner, count: int) => string" },

	/*! Returns \a count bytes starting from the current position without advancing the scanner */
	{ DaoScanner_Peek,		"peek(invar self: scanner, count: int) => string" },

	/*! Mathes \a pattern immediately at the current position; on success, the scanner is advanced and its last match information is updated.
	 * Returns the number of bytes the scanner has advanced through */
	{ DaoScanner_Scan,		"scan(self: scanner, pattern: string) => int" },

	/*! Mathes \a pattern anywhere in the string after the current position; on success, the scanner is advanced and its last match
	 * information is updated. Returns the number of bytes the scanner has advanced through */
	{ DaoScanner_Seek,		"seek(self: scanner, pattern: string) => int" },

	/*! The last matched sub-string or its group \a group (if \a group is greater then zero) */
	{ DaoScanner_Matched,	"matched(invar self: scanner, group = 0) => string" },

	/*! Position of the last matched sub-string or its group \a group (if \a group is greater then zero) */
	{ DaoScanner_MatchedAt,	"matched_pos(invar self: scanner, group = 0) => tuple<start: int, end: int>|none" },

	/*! Line number at the current position */
	{ DaoScanner_Line,		"line(invar self: scanner) => int" },

	/*! Matches \a pattern immediately before the current position without affecting the state of the scanner */
	{ DaoScanner_Follows,	"follows(invar self: scanner, pattern: string) => bool" },

	/*! Matches \a pattern immediately after the current position without affecting the state of the scanner */
	{ DaoScanner_Precedes,	"precedes(invar self: scanner, pattern: string) => bool" },
	{ NULL, NULL }
};

/*! Provides way to successively process textual data using Dao string patterns */
DaoTypeBase scannerTyper = {
	"scanner", NULL, NULL, scannerMeths, {NULL}, {0},
	(FuncPtrDel)DaoScanner_Delete, NULL
};

DAO_DLL int DaoScanner_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_scanner = DaoNamespace_WrapType( ns, &scannerTyper, 1 );
	return 0;
}
