/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2015, Limin Fu
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

// 2015-08: Danilov Aleksey, initial implementation

#include<string.h>
#include<stdio.h>

#include"dao.h"
#include"daoValue.h"
#include"daoProcess.h"
#include"daoNamespace.h"
#include"daoStdtype.h"
#include"daoType.h"

static DMutex test_mtx;
static int test_count = 0, pass_count = 0, fail_count = 0, skip_count = 0;

#define TEST_TRANS( st ) DMutex_Lock( &test_mtx ); st; DMutex_Unlock( &test_mtx )

static void TEST_Run( DaoProcess *proc, DaoValue* p[], int N )
{
	DString *prefix = p[0]->xString.value;
	DList *funcs = proc->activeNamespace->definedRoutines;
	DaoStream *out = DaoVmSpace_StdioStream( proc->vmSpace );
	daoint i;
	char buf[100];
	for ( i = 0; i < funcs->size; ++i ){
		DaoRoutine *rout = funcs->items.pRoutine[i];
		// find global routines matching prefix
		if ( strncmp( rout->routName->chars, prefix->chars, prefix->size ) == 0 ){
			// ignore 'main()'
			if ( strcmp( rout->routName->chars, "main" ) == 0 )
				continue;
			// run
			if ( !( rout->attribs & DAO_ROUT_DECORATOR ) && rout->parCount == 0 ){
				if ( DaoProcess_Call( proc, rout, NULL, NULL, 0 ) == DAO_ERROR ){ // failed
					char buf[512];
					snprintf( buf, sizeof(buf), "unexpected error running '%s'", rout->routName->chars );
					DaoProcess_RaiseError( proc, "Error::Test", buf );
					return;
				}
			}
		}
	}
	DMutex_Lock( &test_mtx );
	snprintf( buf, sizeof(buf), "Summary: %i tests, %i passed, %i failed, %i skipped\n", test_count, pass_count, fail_count, skip_count );
	DaoStream_WriteChars( out, buf );
	test_count = pass_count = fail_count = skip_count = 0;
	DMutex_Unlock( &test_mtx );
}

int GetSourceLine( DaoProcess *proc )
{
	DaoRoutine *rout = proc->activeRoutine;
	ushort_t id = proc->activeCode - proc->topFrame->active->codes;
	return id < rout->body->vmCodes->size? rout->body->annotCodes->items.pVmc[id]->line : rout->defLine;
}

char* GetErrorMsg( DaoProcess *proc, char *buf )
{
	sprintf( buf, "line %i", GetSourceLine( proc ) );
	return buf;
}

static void TEST_Assert( DaoProcess *proc, DaoValue* p[], int N )
{
	if ( !p[0]->xBoolean.value ){
		DString *msg = p[1]->xString.value;
		char buf[1024];
		snprintf( buf, sizeof(buf), "line %i -- %s", GetSourceLine( proc ), msg->size? msg->chars : "assertion failed" );
		DaoProcess_RaiseError( proc, "Test::Assert", buf );
	}
}

static void TEST_SkipTest( DaoProcess *proc, DaoValue* p[], int N )
{
	DaoProcess_RaiseError( proc, "Test::Skip", p[0]->xString.value->chars );
}

static void TEST_AssertEq( DaoProcess *proc, DaoValue* p[], int N )
{
	if ( DaoValue_Compare( p[0], p[1] ) != 0 ){
		char buf[20];
		DaoTuple *tup = DaoTuple_New( 2 );
		DaoTuple_SetItem( tup, p[0], 0 );
		DaoTuple_SetItem( tup, p[1], 1 );
		DaoProcess_RaiseException( proc, "Error::Test::AssertEqual", GetErrorMsg( proc, buf ), (DaoValue*)tup );
	}
}

static void TEST_AssertNeq( DaoProcess *proc, DaoValue* p[], int N )
{
	if ( DaoValue_Compare( p[0], p[1] ) == 0 ){
		char buf[20];
		DaoProcess_RaiseException( proc, "Error::Test::AssertNotEqual", GetErrorMsg( proc, buf ), p[0] );
	}
}

static void TEST_AssertRange( DaoProcess *proc, DaoValue* p[], int N )
{
	if ( DaoValue_Compare( p[0], p[1]->xTuple.values[0] ) < 0 || DaoValue_Compare( p[0], p[1]->xTuple.values[1] ) > 0 ){
		char buf[20];
		DaoTuple *tup = DaoTuple_New( 2 );
		DaoTuple_SetItem( tup, p[0], 0 );
		DaoTuple_SetItem( tup, p[1], 1 );
		DaoProcess_RaiseException( proc, "Error::Test::AssertInRange", GetErrorMsg( proc, buf ), (DaoValue*)tup );
	}
}

static void TEST_AssertNRange( DaoProcess *proc, DaoValue* p[], int N )
{
	if ( DaoValue_Compare( p[0], p[1]->xTuple.values[0] ) >= 0 && DaoValue_Compare( p[0], p[1]->xTuple.values[1] ) <= 0 ){
		char buf[20];
		DaoTuple *tup = DaoTuple_New( 2 );
		DaoTuple_SetItem( tup, p[0], 0 );
		DaoTuple_SetItem( tup, p[1], 1 );
		DaoProcess_RaiseException( proc, "Error::Test::AssertNotInRange", GetErrorMsg( proc, buf ), (DaoValue*)tup );
	}
}

static void TEST_AssertNone( DaoProcess *proc, DaoValue* p[], int N )
{
	if ( p[0]->type != DAO_NONE ){
		char buf[20];
		DaoProcess_RaiseException( proc, "Error::Test::AssertNone", GetErrorMsg( proc, buf ), p[0] );
	}
}

static void TEST_AssertNNone( DaoProcess *proc, DaoValue* p[], int N )
{
	if ( p[0]->type == DAO_NONE ){
		char buf[20];
		DaoProcess_RaiseException( proc, "Error::Test::AssertNotNone", GetErrorMsg( proc, buf ), p[0] );
	}
}

static void TEST_AssertEmpty( DaoProcess *proc, DaoValue* p[], int N )
{
	int res = 0;
	switch ( p[0]->type ){
	case DAO_STRING:res = p[0]->xString.value->size; break;
	case DAO_LIST:	res = p[0]->xList.value->size; break;
	case DAO_ARRAY:	res = p[0]->xArray.size; break;
	case DAO_MAP:	res = p[0]->xMap.value->size; break;
	}
	if ( res ){
		char buf[20];
		DaoProcess_RaiseException( proc, "Error::Test::AssertEmpty", GetErrorMsg( proc, buf ), p[0] );
	}
}

static void TEST_AssertNEmpty( DaoProcess *proc, DaoValue* p[], int N )
{
	int res = 0;
	switch ( p[0]->type ){
	case DAO_STRING:res = p[0]->xString.value->size; break;
	case DAO_LIST:	res = p[0]->xList.value->size; break;
	case DAO_ARRAY:	res = p[0]->xArray.size; break;
	case DAO_MAP:	res = p[0]->xMap.value->size; break;
	}
	if ( !res ){
		char buf[20];
		DaoProcess_RaiseException( proc, "Error::Test::AssertNotEmpty", GetErrorMsg( proc, buf ), p[0] );
	}
}

static void TEST_AssertError( DaoProcess *proc, DaoValue* p[], int N )
{
	DString *expected = p[0]->xString.value;
	DString *actual = NULL;
	DList *errors = proc->exceptions;
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 0 );
	int catched = 0;
	int size = errors->size;
	if( sect == NULL ) return;
	DaoProcess_Execute( proc );
	if ( proc->status == DAO_PROCESS_ABORTED && errors->size > size ){
		DaoException *e = (DaoException*)&errors->items.pValue[errors->size - 1]->xCdata;
		if ( DString_Compare( expected, e->ctype->name ) != 0 )
			actual = DString_Copy( e->ctype->name );
		else
			catched = 1;
		DList_Clear( errors );
	}
	DaoProcess_PopFrame( proc );
	if ( !catched ){
		char buf[512];
		if ( actual ){
			snprintf( buf, sizeof(buf), "line %i -- expected %s error, intercepted %s", GetSourceLine( proc ), expected->chars, actual->chars );
			DString_Delete( actual );
		}
		else
			snprintf( buf, sizeof(buf), "line %i -- expected %s error, intercepted nothing", GetSourceLine( proc ), expected->chars );
		DaoProcess_RaiseError( proc, "Test::AssertError", buf );
	}
}

static void TEST_New( DaoProcess *proc, DaoValue* p[], int N )
{
	int count;
	TEST_TRANS( count = ++test_count );
	DaoProcess_PutInteger( proc, count );
}

static void TEST_Pass( DaoProcess *proc, DaoValue* p[], int N )
{
	TEST_TRANS( ++pass_count );
}

static void TEST_Fail( DaoProcess *proc, DaoValue* p[], int N )
{
	TEST_TRANS( ++fail_count );
}

static void TEST_Skip( DaoProcess *proc, DaoValue* p[], int N )
{
	TEST_TRANS( ++skip_count );
}

static DaoFuncItem testFuncs[] =
{
	//! Runs tests -- all global routines with the given \a namePrefix -- and prints summary
	{ TEST_Run,			"runTests(namePrefix = '')" },

	//! Skips the current test due to the specified \a reason
	{ TEST_SkipTest,	"skipTest(reason = '')" },

	//! Generic assertion
	{ TEST_Assert,		"assert(condition: bool, message = '')" },

	//! Value assertions
	{ TEST_AssertEq,	"assertEqual(invar actual: @T, invar expected: @T)" },
	{ TEST_AssertNeq,	"assertNotEqual(invar actual: @T, invar expected: @T)" },
	{ TEST_AssertRange,	"assertInRange(invar actual: @T, invar range: tuple<@T,@T>)" },
	{ TEST_AssertNRange,"assertNotInRange(invar actual: @T, invar range: tuple<@T,@T>)" },
	{ TEST_AssertNone,	"assertNone(invar value: @T|none)" },
	{ TEST_AssertNNone,	"assertNotNone(invar value: @T|none)" },
	{ TEST_AssertEmpty,	"assertEmpty(invar value: string|list<@T>|array<@T>|map<@K,@V>)" },
	{ TEST_AssertNEmpty,"assertNotEmpty(invar value: string|list<@T>|array<@T>|map<@K,@V>)" },

	//! Asserts that error with the given \a name is raised by the code section
	{ TEST_AssertError,	"assertError(name: string)[]" },
	{ NULL, NULL }
};

static DaoFuncItem baseFuncs[] =
{
	// not intended for public use
	{ TEST_New,			"newTest() => int" },
	{ TEST_Pass,		"testPassed()" },
	{ TEST_Fail,		"testFailed()" },
	{ TEST_Skip,		"testSkipped()" },
	{ NULL, NULL }
};

DAO_DLL int DaoBase_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *testns = DaoNamespace_GetNamespace( ns, "test" );
	DaoNamespace *basens = DaoNamespace_GetNamespace( testns, "base" );
	DaoNamespace_WrapFunctions( testns, testFuncs );
	DaoNamespace_WrapFunctions( basens, baseFuncs );
	DMutex_Init( &test_mtx );
	return 0;
}
