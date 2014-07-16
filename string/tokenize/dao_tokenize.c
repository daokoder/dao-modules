/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011,2012, Limin Fu
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

#include<stdlib.h>
#include<string.h>
#include"daoString.h"
#include"daoValue.h"


static void DaoSTR_Tokenize( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *self = p[0]->xString.value;
	DString *delms = p[1]->xString.value;
	DString *quotes = p[2]->xString.value;
	int bkslash = (int)p[3]->xInteger.value;
	int simplify = (int)p[4]->xInteger.value;
	DaoList *list = DaoProcess_PutList( proc );
	DaoValue *value = (DaoValue*) DaoString_New();
	DString *str = value->xString.value;
	char *s = self->chars;
	while( *s ){
		if( bkslash && *s == '\\' ){
			DString_AppendChar( str, *s );
			DString_AppendChar( str, *(s+1) );
			s += 2;
			continue;
		}
		if( ( bkslash == 0 || s == self->chars || *(s-1) !='\\' )
				&& DString_FindChar( quotes, *s, 0 ) != DAO_NULLPOS ){
			DString_AppendChar( str, *s );
			s ++;
			while( *s ){
				if( bkslash && *s == '\\' ){
					DString_AppendChar( str, *s );
					DString_AppendChar( str, *(s+1) );
					s += 2;
				}
				if( ( bkslash == 0 || *(s-1) !='\\' )
						&& DString_FindChar( quotes, *s, 0 ) != DAO_NULLPOS )
					break;
				DString_AppendChar( str, *s );
				s ++;
			}
			DString_AppendChar( str, *s );
			s ++;
			continue;
		}
		if( DString_FindChar( delms, *s, 0 ) != DAO_NULLPOS ){
			if( s != self->chars && *(s-1)=='\\' ){
				DString_AppendChar( str, *s );
				s ++;
				continue;
			}else{
				if( simplify ) DString_Trim( str, 1, 1, 0 );
				if( str->size > 0 ){
					DList_Append( list->value, value );
					DString_Clear( str );
				}
				DString_AppendChar( str, *s );
				s ++;
				if( simplify ) DString_Trim( str, 1, 1, 0 );
				if( str->size > 0 ) DList_Append( list->value, value );
				DString_Clear( str );
				continue;
			}
		}
		DString_AppendChar( str, *s );
		s ++;
	}
	if( simplify ) DString_Trim( str, 1, 1, 0 );
	if( str->size > 0 ) DList_Append( list->value, value );
	DaoString_Delete( (DaoString*) value );
}
const char *p = "tokenize( source :string, seps :string, quotes='', backslash=0, simplify=0 )=>list<string>";
DAO_DLL int DaoTokenize_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace_WrapFunction( ns, DaoSTR_Tokenize, p );
	return 0;
}
