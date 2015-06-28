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

#include "dao_regex.h"

static DaoType *daox_type_regex = NULL;
static DaoType *daox_type_match = NULL;
static DaoType *daox_type_iter = NULL;

static int Onig_New(OnigRegex*rgx, const char* pattern, const char* pattern_end, OnigOptionType option, OnigEncoding enc, OnigSyntaxType* syntax, OnigErrorInfo* einfo)
{
	return onig_new( rgx, (OnigUChar*)pattern, (OnigUChar*)pattern_end, option, enc, syntax, einfo );
}
static OnigPosition Onig_Search (OnigRegex rgx, const char* str, const char* end, const char* start, const char* range, OnigRegion* region, OnigOptionType option)
{
	return onig_search( rgx, (OnigUChar*)str, (OnigUChar*)end, (OnigUChar*)start, (OnigUChar*)range, region, option );
}

DaoOnigRegex* DaoOnigRegex_New( DString *pt, int icase, OnigErrorInfo *error, int *errcode )
{
	DaoOnigRegex *res = (DaoOnigRegex*)dao_malloc( sizeof(DaoOnigRegex) );
	res->icase = icase;
	res->regex = NULL;
	res->pattern = DString_Copy( pt );
	*errcode = Onig_New( &res->regex, pt->chars, pt->chars + pt->size,
						 icase? ONIG_OPTION_IGNORECASE : ONIG_OPTION_NONE, ONIG_ENCODING_UTF8, ONIG_SYNTAX_DEFAULT, error );
	return res;
}

void DaoOnigRegex_Delete( DaoOnigRegex *self )
{
	DString_Delete( self->pattern );
	if ( self->regex )
		onig_free( self->regex );
}

static void DaoOnigRegex_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *res;
	DString* dpt = p[0]->xString.value;
	DString* pt = DString_New();
	OnigErrorInfo error;
	int ws = Regws_Free;
	int opts = ( N > 1 )? p[1]->xEnum.value : 0;
	int icase = opts & 4, comms = opts & 8;
	int count = 0, errcode;
	char cchar = opts & 16? '\\' : '%';
	if ( opts & 1 && opts & 2 ){
		DaoProcess_RaiseError( proc, "Param", "Conflicting spacing options" );
		DString_Delete( pt );
		return;
	}
	if ( opts & 1 )
		ws = Regws_Strict;
	else if ( opts & 2 )
		ws = Regws_Implied;
	if ( ws != Regws_Strict || cchar != '\\' || comms ){
		daoint i;
		for ( i = 0; i < dpt->size; i++ )
			if ( dpt->chars[i] == cchar ){
				i++;
				if ( comms && dpt->chars[i] == '#' )
					DString_AppendChar( pt, '#' );
				else if ( cchar == '%' && dpt->chars[i] == '%' )
					DString_AppendChar( pt, '%' );
				else {
					DString_AppendChar( pt, '\\' );
					if ( i < dpt->size )
						DString_AppendChar( pt, dpt->chars[i] );
				}
			}
			else if ( dpt->chars[i] == '\\' )
				DString_AppendChars( pt, "\\\\" );
			else {
				if ( comms && dpt->chars[i] == '#' ){
					i = DString_FindChar( dpt, '\n', i + 1 );
					if ( i > 0 )
						continue;
					else
						break;
				}
				else if ( dpt->chars[i] == '[' )
					count++;
				else if ( dpt->chars[i] == ']' )
					count--;
				if ( ws != Regws_Strict && isspace( dpt->chars[i] ) && !count ){
					if ( ws == Regws_Implied ){
						if ( dpt->chars[i + 1] == dpt->chars[i] ){
							DString_AppendChars( pt, "\\s+" );
							i++;
						}
						else {
							DString_AppendChars( pt, "\\s*" );
							if ( dpt->chars[i] == '\r' && dpt->chars[i + 1] == '\n' )
								i++;
						}
					}
				}
				else
					DString_AppendChar( pt, dpt->chars[i] );
			}
	}
	else
		DString_Assign( pt, dpt );
	res = DaoOnigRegex_New( pt, icase, &error, &errcode );
	if ( errcode != ONIG_NORMAL ){
		DString *msg = DString_New();
		DString_Resize( msg, ONIG_MAX_ERROR_MESSAGE_LEN );
		DString_Resize( msg, onig_error_code_to_str( (OnigUChar*)msg->chars, errcode, error ) );
		DString_AppendChars( msg, ": " );
		DString_Append( msg, pt );
		DaoProcess_RaiseError( proc, "Regex", msg->chars );
		DString_Delete( pt );
		DString_Delete( msg );
		DaoOnigRegex_Delete( res );
		return;
	}
	DString_Delete( pt );
	DaoProcess_PutCdata( proc, res, daox_type_regex );
}

static void DaoOnigRegex_Pattern( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, self->pattern );
}

static void DaoOnigRegex_ICase( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutBoolean( proc, self->icase );
}

static void DaoOnigRegex_CaptureCount( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, onig_number_of_captures( self->regex ) );
}

DaoOnigMatch* DaoOnigMatch_New( DString *str, DaoValue *regval, OnigRegion *match )
{
	DaoOnigMatch* res = (DaoOnigMatch*)dao_malloc( sizeof(DaoOnigMatch) );
	res->target = DString_Copy( str );
	res->match = onig_region_new();
	onig_region_copy( res->match, match );
	res->regex = (DaoOnigRegex*)DaoValue_TryGetCdata( regval );
	res->regval = regval;
	DaoGC_IncRC( regval );
	return res;
}

void DaoOnigMatch_Delete( DaoOnigMatch *self )
{
	DString_Delete( self->target );
	onig_region_free( self->match, 1 );
	DaoGC_DecRC( self->regval );
}

int GetGroupNumber( DaoProcess *proc, regex_t *regex, OnigRegion *match, DaoValue *group )
{
	int num = -1;
	if ( group->type == DAO_STRING ){
		DString *name = group->xString.value;
		num = onig_name_to_backref_number( regex, (OnigUChar*)name->chars, (OnigUChar*)(name->chars + name->size), match );
		if ( num < 0 )
			DaoProcess_RaiseError( proc, "Param", "Invalid group name" );
	}
	else {
		num = group->xInteger.value;
		if ( num < 0 || num >= match->num_regs ){
			DaoProcess_RaiseError( proc, "Param", "Invalid group number" );
			num = -1;
		}
	}
	return num;
}

static void DaoOnigMatch_String( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigMatch *self = (DaoOnigMatch*)DaoValue_TryGetCdata( p[0] );
	DString *str = DaoProcess_PutChars( proc, "" );
	int group;
	if ( ( group = GetGroupNumber( proc, self->regex->regex, self->match, p[1] ) ) >= 0 )
		DString_AppendBytes( str, self->target->chars + self->match->beg[group], self->match->end[group] - self->match->beg[group] );
}

static void DaoOnigMatch_Start( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigMatch *self = (DaoOnigMatch*)DaoValue_TryGetCdata( p[0] );
	int group;
	if ( ( group = GetGroupNumber( proc, self->regex->regex, self->match, p[1] ) ) >= 0 )
		DaoProcess_PutInteger( proc, self->match->beg[group] );
}

static void DaoOnigMatch_End( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigMatch *self = (DaoOnigMatch*)DaoValue_TryGetCdata( p[0] );
	int group;
	if ( ( group = GetGroupNumber( proc, self->regex->regex, self->match, p[1] ) ) >= 0 )
		DaoProcess_PutInteger( proc, self->match->end[group] - 1 );
}

static void DaoOnigMatch_Size( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigMatch *self = (DaoOnigMatch*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->match->num_regs - 1 );
}

static void DaoOnigMatch_Length( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigMatch *self = (DaoOnigMatch*)DaoValue_TryGetCdata( p[0] );
	int group;
	if ( ( group = GetGroupNumber( proc, self->regex->regex, self->match, p[1] ) ) >= 0 )
		DaoProcess_PutInteger( proc, self->match->end[group] - self->match->beg[group] );
}

static DaoFuncItem matchMeths[]=
{
	/*! Sub-string captured by \a group */
	{ DaoOnigMatch_String,	"string(self: Match, group: int|string = 0) => string" },

	/*! Size of the sub-string captured by \a group */
	{ DaoOnigMatch_Length,	"size(self: Match, group: int|string = 0) => int" },

	/*! Start position of the sub-string captured by \a group */
	{ DaoOnigMatch_Start,	"start(self: Match, group: int|string = 0) => int" },

	/*! End position of the sub-string captured by \a group */
	{ DaoOnigMatch_End, 	"end(self: Match, group: int|string = 0) => int" },

	/*! Number of captured groups */
	{ DaoOnigMatch_Size,	".groupCount(self: Match) => int" },
	{ NULL, NULL }
};

/*! Single regular expression match providing information on matched sub-string and individual captured groups.
 *
 * \c group parameter in \c Match methods may either be a group number or its name.
 *
 * Group number is interpreted the following way:
 * -\c group == 0 -- entire matched sub-string
 * -\c group > 0 and \c group <= groupCount() -- corresponding sub-match
 * -\c group < 0 or \c group > groupCount() -- not permitted
 *
 * If \c group is a name, the last group in the pattern with this name is assumed (at least one such group must exist).
 */
DaoTypeBase matchTyper =
{
	"Match", NULL, NULL, (DaoFuncItem*) matchMeths, {0}, {0},
	(FuncPtrDel)DaoOnigMatch_Delete, NULL
};

int NormBounds( DaoProcess *proc, DString *str, daoint *start, daoint *end )
{
	if ( *start < 0 )
		*start += str->size;
	if ( *end < 0 )
		*end += str->size;
	if ( *start < 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid start index" );
		return 0;
	}
	if ( *end < 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid end index" );
		return 0;
	}
	if ( *start > *end ){
		DaoProcess_RaiseError( proc, "Param", "Invalid matching range" );
		return 0;
	}
	if ( *end >= str->size )
		*end = str->size - 1;
	return 1;
}

static void DaoOnigRegex_Fetch( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DString *str = p[1]->xString.value;
	daoint start = p[3]->xInteger.value, end = p[4]->xInteger.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	if ( NormBounds( proc, str, &start, &end ) && start < str->size ){
		OnigRegion *match = onig_region_new();
		int group, pos = Onig_Search( self->regex, str->chars, str->chars + str->size, str->chars + start, str->chars + end + 1,
									  match, ONIG_OPTION_NONE );
		if ( pos >= 0 && ( group = GetGroupNumber( proc, self->regex, match, p[2] ) ) >= 0 )
			DString_AppendBytes( res, str->chars + match->beg[group], match->end[group] - match->beg[group] );
		onig_region_free( match, 1 );
	}
}

static void DaoOnigRegex_Matches( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DString *str = p[1]->xString.value;
	OnigRegion *match = onig_region_new();
	int res = onig_match( self->regex, (OnigUChar*)str->chars, (OnigUChar*)(str->chars + str->size), (OnigUChar*)str->chars, match, ONIG_OPTION_NONE );
	DaoProcess_PutBoolean( proc, res == str->size );
	onig_region_free( match, 1 );
}

static void DaoOnigRegex_Extract( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DString *str = p[1]->xString.value;
	int matched = p[2]->xEnum.value != 2;
	int unmatched = p[2]->xEnum.value != 1;
	DaoList *lst = DaoProcess_PutList( proc );
	OnigRegion *match = onig_region_new();
	int pos, start = 0;
	do {
		pos = Onig_Search( self->regex, str->chars, str->chars + str->size, str->chars + start, str->chars + str->size, match,
						   ONIG_OPTION_NONE );
		if ( unmatched && pos != start ){
			DaoString *item = DaoString_New();
			DString_AppendBytes( item->value, str->chars + start, ( ( pos >= 0 )? pos : str->size ) - start );
			DaoList_Append( lst, (DaoValue*)item );
		}
		if ( matched && pos >= 0 ){
			DaoString *item = DaoString_New();
			DString_AppendBytes( item->value, str->chars + pos, match->end[0] - match->beg[0] );
			DaoList_Append( lst, (DaoValue*)item );
		}
		if ( pos >= 0 )
			start = match->end[0];
	}
	while ( pos >= 0 && start < str->size );
	onig_region_free( match, 1 );
}

static void DaoOnigRegex_Replace( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DString *str = p[1]->xString.value;
	DString *fmt = p[2]->xString.value;
	daoint start = p[3]->xInteger.value, end = p[4]->xInteger.value;
	if ( !NormBounds( proc, str, &start, &end ) )
		return;
	if ( start >= str->size )
		DaoProcess_PutString( proc, str );
	else {
		DString *res = DaoProcess_PutChars( proc, "" );
		DString_Reserve( res, str->size );
		DString_AppendBytes( res, str->chars, start );
		OnigRegion *match = onig_region_new();
		int pos;
		do {
			pos = Onig_Search( self->regex, str->chars, str->chars + str->size, str->chars + start, str->chars + end + 1, match,
							   ONIG_OPTION_NONE );
			DString_AppendBytes( res, str->chars + start, ( ( pos >= 0 )? pos : end + 1 ) - start );
			if ( pos >= 0 ){
				daoint i;
				for ( i = 0; i < fmt->size; i++ )
					if ( fmt->chars[i] == '$' ){
						i++;
						if ( fmt->chars[i] == '$' )
							DString_AppendChar( res, '$' );
						else if ( fmt->chars[i] >= '0' && fmt->chars[i] <= '9' ){
							int group = fmt->chars[i] - '0';
							if ( group < match->num_regs )
								DString_AppendBytes( res, str->chars + match->beg[group], match->end[group] - match->beg[group] );
						}
						else if ( fmt->chars[i] == '(' ){
							daoint end = DString_FindChar( fmt, ')', ++i );
							int group;
							if ( end < 0 )
								goto Error;
							group = onig_name_to_backref_number( self->regex, (OnigUChar*)(fmt->chars + i), (OnigUChar*)(fmt->chars + end), match );
							if ( group < 0 )
								goto Error;
							DString_AppendBytes( res, str->chars + match->beg[group], match->end[group] - match->beg[group] );
							i = end;
						}
						else
							goto Error;
						continue;
					Error:
						if ( 1 ){
							char errbuf[50];
							snprintf( errbuf, sizeof(errbuf), "Invalid group identifier at index %"DAO_INT, i );
							DaoProcess_RaiseError( proc, "Regex", errbuf );
							return;
						}
					}
					else
						DString_AppendChar( res, fmt->chars[i] );
				start = match->end[0];
			}
		}
		while ( pos >= 0 && start <= end );
		DString_AppendBytes( res, str->chars + end + 1, str->size - end - 1 );
		onig_region_free( match, 1 );
	}
}

static void DaoOnigRegex_Search( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DString *str = p[1]->xString.value;
	daoint start = p[2]->xInteger.value, end = p[3]->xInteger.value;
	if ( !NormBounds( proc, str, &start, &end ) )
		return;
	if ( start >= str->size )
		DaoProcess_PutNone( proc );
	else {
		OnigRegion *match = onig_region_new();
		int pos = Onig_Search( self->regex, str->chars, str->chars + str->size, str->chars + start, str->chars + end + 1, match,
							   ONIG_OPTION_NONE );
		if ( pos >= 0 )
			DaoProcess_PutCdata( proc, DaoOnigMatch_New( str, p[0], match ), daox_type_match );
		else
			DaoProcess_PutNone( proc );
		onig_region_free( match, 1 );
	}
}

static void DaoOnigRegex_Scan( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DaoValue *rval = p[0];
	DString *str = p[1]->xString.value;
	daoint start = p[2]->xInteger.value;
	daoint end = p[3]->xInteger.value;
	DaoList *list = DaoProcess_PutList( proc );
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 1 );
	DaoValue *res;
	if ( !NormBounds( proc, str, &start, &end ) )
		return;
	if ( sect == NULL )
		return;
	if ( start < str->size ){
		OnigRegion *match = onig_region_new();
		int entry = proc->topFrame->entry;
		int pos;
		do {
			pos = Onig_Search( self->regex, str->chars, str->chars + str->size, str->chars + start, str->chars + end + 1, match,
							   ONIG_OPTION_NONE );
			if ( pos >= 0 ){
				if ( sect->b > 0 )
					DaoProcess_SetValue( proc, sect->a, (DaoValue*)DaoProcess_NewCdata( proc, daox_type_match,
																						DaoOnigMatch_New( str, rval, match ), 1 ) );
				proc->topFrame->entry = entry;
				if ( !DaoProcess_Execute( proc ) )
					break;
				res = proc->stackValues[0];
				if ( res && res->type != DAO_NONE )
					DaoList_Append( list, res );
				start = match->end[0];
			}
		}
		while ( pos >= 0 && start <= end );
		DaoProcess_PopFrame( proc );
		onig_region_free( match, 1 );
	}
}

static void DaoOnigRegex_Change( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DaoValue *rval = p[0];
	DString *str = p[1]->xString.value;
	daoint start = p[2]->xInteger.value;
	daoint end = p[3]->xInteger.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 1 );
	if ( !NormBounds( proc, str, &start, &end ) )
		return;
	if ( sect == NULL )
		return;
	if ( start >= str->size )
		DString_Assign( res, str );
	else {
		OnigRegion *match = onig_region_new();
		int entry = proc->topFrame->entry;
		int pos;
		DString_AppendBytes( res, str->chars, start );
		do {
			pos = Onig_Search( self->regex, str->chars, str->chars + str->size, str->chars + start, str->chars + end + 1, match,
							   ONIG_OPTION_NONE );
			DString_AppendBytes( res, str->chars + start, ( ( pos >= 0 )? pos : end + 1 ) - start );
			if ( pos >= 0 ){
				if ( sect->b > 0 )
					DaoProcess_SetValue( proc, sect->a, (DaoValue*)DaoProcess_NewCdata( proc, daox_type_match,
																						DaoOnigMatch_New( str, rval, match ), 1 ) );
				else
					onig_region_free( match, 1 );
				proc->topFrame->entry = entry;
				if ( !DaoProcess_Execute( proc ) )
					break;
				DString_Append( res, proc->stackValues[0]->xString.value );
				start = match->end[0];
			}
		}
		while ( pos >= 0 && start <= end );
		DString_AppendBytes( res, str->chars + end + 1, str->size - end - 1 );
		DaoProcess_PopFrame( proc );
		onig_region_free( match, 1 );
	}
}

DaoOnigIter* DaoOnigIter_New( DString *str, DaoValue *regval, daoint start, daoint end )
{
	DaoOnigIter* res = (DaoOnigIter*)dao_malloc( sizeof(DaoOnigIter) );
	res->target = DString_Copy( str );
	res->regex = (DaoOnigRegex*)DaoValue_TryGetCdata( regval );
	res->regval = regval;
	DaoGC_IncRC( regval );
	res->start = start;
	res->end = end;
	res->match = onig_region_new();
	return res;
}

void DaoOnigIter_Delete( DaoOnigIter *self )
{
	DString_Delete( self->target );
	DaoGC_DecRC( self->regval );
	onig_region_free( self->match, 1 );
}

static void DaoOnigIter_Init( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigIter *self = (DaoOnigIter*)DaoValue_TryGetCdata( p[0] );
	DaoTuple *iter = &p[1]->xTuple;
	DaoTuple_SetItem( iter, (DaoValue*)DaoInteger_New( self->start < self->target->size ), 0 );
	DaoTuple_SetItem( iter, (DaoValue*)DaoInteger_New( self->start ), 1 );
	if ( 1 ){
		DaoInteger *valid = &iter->values[0]->xInteger;
		DaoInteger *start = &iter->values[1]->xInteger;
		char *chs = self->target->chars;
		if ( valid->value ){
			int pos = Onig_Search( self->regex->regex, chs, chs + self->target->size, chs + start->value, chs + self->end + 1,
								   self->match, ONIG_OPTION_NONE );
			valid->value = pos >= 0;
			if ( valid->value )
				start->value = self->match->end[0];
		}
	}
}

static void DaoOnigIter_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigIter *self = (DaoOnigIter*)DaoValue_TryGetCdata( p[0] );
	DaoTuple *iter = &p[1]->xTuple;
	if ( iter->values[1]->type == DAO_INTEGER ){
		DaoInteger *valid = &iter->values[0]->xInteger;
		DaoInteger *start = &iter->values[1]->xInteger;
		char *chs = self->target->chars;
		int pos;
		DaoProcess_PutCdata( proc, DaoOnigMatch_New( self->target, self->regval, self->match ), daox_type_match );
		pos = ( start->value < self->target->size )?
					Onig_Search( self->regex->regex, chs, chs + self->target->size, chs + start->value, chs + self->end + 1, self->match,
								 ONIG_OPTION_NONE ) : -1;
		valid->value = pos >= 0;
		if ( valid->value )
			start->value = self->match->end[0];
	}
	else
		DaoProcess_RaiseError( proc, "Param", "Invalid iterator type" );
}

static void DaoOnigRegex_Iter( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOnigRegex *self = (DaoOnigRegex*)DaoValue_TryGetCdata( p[0] );
	DaoValue *rval = p[0];
	DString *str = p[1]->xString.value;
	daoint start = p[2]->xInteger.value;
	daoint end = p[3]->xInteger.value;
	if ( NormBounds( proc, str, &start, &end ) )
		DaoProcess_PutCdata( proc, DaoOnigIter_New( str, rval, start, end ), daox_type_iter );
}

/*! \c for iterator to iterate over regular expression matches in a string */
static DaoFuncItem iterMeths[] =
{
	{ DaoOnigIter_Init,	"for(self: Iter, iterator: ForIterator)" },
	{ DaoOnigIter_Get,	"[](self: Iter, index: ForIterator) => Match" },
	{ NULL, NULL }
};

DaoTypeBase iterTyper =
{
	"Iter", NULL, NULL, (DaoFuncItem*) iterMeths, {0}, {0},
	(FuncPtrDel)DaoOnigIter_Delete, NULL
};

static DaoFuncItem regexMeths[] =
{
	/*! String pattern
	 *
	 * \note The pattern is stored in canonical form, i.e. with strict spacing, '\' as escape character and without comments */
	{ DaoOnigRegex_Pattern,		".pattern(self: Regex) => string" },

	/*! Number of capture groups in the pattern */
	{ DaoOnigRegex_CaptureCount,".groupCount(self: Regex) => int" },

	/*! Case-insensitivity */
	{ DaoOnigRegex_ICase,		".ignoresCase(self: Regex) => bool" },

	/*! Finds the first match in \a target in the range [\a start; \a end] and returns sub-match specified by \a group.
	 *
	 * \note For the interpretation of group numbers, see \c Match */
	{ DaoOnigRegex_Fetch,	"fetch(self: Regex, target: string, group: int|string = 0, start = 0, end = -1) => string" },

	/*! Returns the first match in \a target in the range [\a start; \a end], or \c none if no match was found */
	{ DaoOnigRegex_Search,	"search(self: Regex, target: string, start = 0, end = -1) => Match|none" },

	/*! Checks if the entire \a target is matched by the regex */
	{ DaoOnigRegex_Matches,	"matches(self: Regex, target: string) => bool" },

	/*! Returns all matches in \a target (or unmatched, or both, depending on \a matchType) */
	{ DaoOnigRegex_Extract,	"extract(self: Regex, target: string, matchType: enum<both,matched,unmatched> = $matched)"
								" => list<string>" },

	/*! Replaces all matches in \a target in the range [\a start; \a end] with \a format string. Returns the entire resulting
	 * string. \a format may contain backreferences in the form '$<group number from 0 to 9>' or '$(<group name>)'; '$$' can be used
	 * to escape '$' */
	{ DaoOnigRegex_Replace,	"replace(self: Regex, target: string, format: string, start = 0, end = -1) => string" },

	/*! Iterates over all matches in \a target in the range [\a start; \a end], yielding each match as \a found. Returns the list of
	 * values obtained from the code section */
	{ DaoOnigRegex_Scan,	"scan(self: Regex, target: string, start = 0, end = -1)[found: Match => none|@V] => list<@V>" },

	/*! Iterates over all matches in \a target, yielding each of them as \a found. Returns the string formed by replacing each match
	 * in \a target by the corresponding string returned from the code section */
	 { DaoOnigRegex_Change,	"replace(self: Regex, target: string, start = 0, end = -1)[found: Match => string] => string" },

	/*! Returns \c for iterator to iterate over all matches in \a target in the range [\a start; \a end].
	 *
	 * \note Changing \c target has no effect on the iteration process (the iterator will still be bound to the original string) */
	{ DaoOnigRegex_Iter,	"iter(self: Regex, target: string, start = 0, end = -1) => Iter" },
	{ NULL, NULL },
};

/*! Regular expression using [Onigmo fork](https://github.com/k-takata/Onigmo) of [Oniguruma](http://www.geocities.jp/kosako3/oniguruma/) library with Ruby grammar as backend */
DaoTypeBase regexTyper =
{
	"Regex", NULL, NULL, (DaoFuncItem*) regexMeths, {0}, {0},
	(FuncPtrDel)DaoOnigRegex_Delete, NULL
};

static DaoFuncItem reMeths[] =
{
	/*! Constructs regular expression from \a pattern using specified \a options (if provided).
	 *
	 * Default options mimic Dao string patterns syntax:
	 * -free spacing -- whitespace is ignored outside of '[...]'
	 * -'%' is used as control character
	 * -the pattern is treated as case-sensitive
	 *
	 * This behavior can be overridden with the following values of \a options:
	 * -\c $strictSpacing -- whitespace characters in the pattern are treated 'as is' (canonical behavior)
	 * -\c $impliedSpacing -- outside of '[ ... ]', a standalone whitespace character or '\r\n' are interpreted as '\\s*',
	 * and a pair of equal whitespace characters is interpreted as '\\s+'
	 * -\c $ignoreCase -- the pattern is treated as case-insensitive
	 * -\c $allowComments -- all characters starting from '#' up to '\n' (or end of string) are ignored as comments ('#' can be escaped)
	 * -\c $useBackslash -- use canonical '\' as control character
	 *
	 * \note Regular expression engine presumes UTF-8-encoded patterns */
	{ DaoOnigRegex_Create,		"compile(pattern: string) => Regex" },
	{ DaoOnigRegex_Create,		"compile(pattern: string, options: enum<strictSpacing;impliedSpacing;ignoreCase;allowComments;"
																	 "useBackslash>) => Regex" },
	{ NULL, NULL },
};

DAO_DLL int DaoRegex_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *rens = DaoNamespace_GetNamespace( ns, "re" );
	daox_type_regex = DaoNamespace_WrapType( rens, &regexTyper, DAO_CTYPE_INVAR|DAO_CTYPE_OPAQUE );
	daox_type_match = DaoNamespace_WrapType( rens, &matchTyper, DAO_CTYPE_INVAR|DAO_CTYPE_OPAQUE );
	daox_type_iter = DaoNamespace_WrapType( rens, &iterTyper, DAO_CTYPE_OPAQUE );
	DaoNamespace_WrapFunctions(rens, reMeths);
	onig_init();
	return 0;
}
