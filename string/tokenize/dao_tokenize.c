/*
// This file is a part of Dao standard modules.
// Copyright (C) 2006-2012, Limin Fu. Email: daokoder@gmail.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this 
// software and associated documentation files (the "Software"), to deal in the Software 
// without restriction, including without limitation the rights to use, copy, modify, merge, 
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons 
// to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or 
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include<stdlib.h>
#include<string.h>
#include"daoString.h"
#include"daoValue.h"


static void DaoSTR_Tokenize( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *self = p[0]->xString.data;
	DString *delms = p[1]->xString.data;
	DString *quotes = p[2]->xString.data;
	int bkslash = (int)p[3]->xInteger.value;
	int simplify = (int)p[4]->xInteger.value;
	DaoList *list = DaoProcess_PutList( proc );
	DaoValue *value = (DaoValue*) DaoString_New(1);
	DString *str = value->xString.data;
	if( self->mbs ){
		char *s = self->mbs;
		DString_ToMBS( str );
		DString_ToMBS( delms );
		DString_ToMBS( quotes );
		while( *s ){
			if( bkslash && *s == '\\' ){
				DString_AppendChar( str, *s );
				DString_AppendChar( str, *(s+1) );
				s += 2;
				continue;
			}
			if( ( bkslash == 0 || s == self->mbs || *(s-1) !='\\' )
					&& DString_FindChar( quotes, *s, 0 ) != MAXSIZE ){
				DString_AppendChar( str, *s );
				s ++;
				while( *s ){
					if( bkslash && *s == '\\' ){
						DString_AppendChar( str, *s );
						DString_AppendChar( str, *(s+1) );
						s += 2;
					}
					if( ( bkslash == 0 || *(s-1) !='\\' )
							&& DString_FindChar( quotes, *s, 0 ) != MAXSIZE )
						break;
					DString_AppendChar( str, *s );
					s ++;
				}
				DString_AppendChar( str, *s );
				s ++;
				continue;
			}
			if( DString_FindChar( delms, *s, 0 ) != MAXSIZE ){
				if( s != self->mbs && *(s-1)=='\\' ){
					DString_AppendChar( str, *s );
					s ++;
					continue;
				}else{
					if( simplify ) DString_Trim( str );
					if( str->size > 0 ){
						DArray_Append( & list->items, value );
						DString_Clear( str );
					}
					DString_AppendChar( str, *s );
					s ++;
					if( simplify ) DString_Trim( str );
					if( str->size > 0 ) DArray_Append( & list->items, value );
					DString_Clear( str );
					continue;
				}
			}
			DString_AppendChar( str, *s );
			s ++;
		}
		if( simplify ) DString_Trim( str );
		if( str->size > 0 ) DArray_Append( & list->items, value );
	}else{
		wchar_t *s = self->wcs;
		DString_ToWCS( str );
		DString_ToWCS( delms );
		DString_ToWCS( quotes );
		while( *s ){
			if( ( s == self->wcs || bkslash ==0 || *(s-1)!=L'\\' )
					&& DString_FindWChar( quotes, *s, 0 ) != MAXSIZE ){
				DString_AppendChar( str, *s );
				s ++;
				while( *s ){
					if( ( bkslash ==0 || *(s-1)!=L'\\' )
							&& DString_FindWChar( quotes, *s, 0 ) != MAXSIZE ) break;
					DString_AppendChar( str, *s );
					s ++;
				}
				DString_AppendChar( str, *s );
				s ++;
				continue;
			}
			if( DString_FindWChar( delms, *s, 0 ) != MAXSIZE ){
				if( s != self->wcs && ( bkslash && *(s-1)==L'\\' ) ){
					DString_AppendChar( str, *s );
					s ++;
					continue;
				}else{
					if( simplify ) DString_Trim( str );
					if( str->size > 0 ){
						DArray_Append( & list->items, value );
						DString_Clear( str );
					}
					DString_AppendChar( str, *s );
					s ++;
					if( simplify ) DString_Trim( str );
					if( str->size > 0 ) DArray_Append( & list->items, value );
					DString_Clear( str );
					continue;
				}
			}
			DString_AppendChar( str, *s );
			s ++;
		}
		if( simplify ) DString_Trim( str );
		if( str->size > 0 ) DArray_Append( & list->items, value );
	}
	DaoString_Delete( (DaoString*) value );
}
const char *p = "tokenize( self :string, seps :string, quotes='', backslash=0, simplify=0 )=>list<string>";
int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace_WrapFunction( ns, DaoSTR_Tokenize, p );
	return 0;
}
