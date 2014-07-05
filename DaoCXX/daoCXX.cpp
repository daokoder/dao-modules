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

#include <llvm/Module.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/LLVMContext.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Lex/Token.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include "dao.h"
#include "daoConst.h"
#include "daoString.h"
#include "daoList.h"
#include "daoValue.h"
#include "daoRoutine.h"
}


using namespace std;
using namespace llvm;
using namespace clang;

CompilerInstance compiler;
EmitLLVMOnlyAction action;
ExecutionEngine *engine;

static DaoType *dao_type_stream2 = NULL;

extern "C"{
static void DaoCXX_Default( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoProcess_RaiseError( proc, NULL, "Unsuccessully wrapped function" );
}
}

static void DaoCXX_AddVirtualFile( const char *name, const char *source )
{
	MemoryBuffer* Buffer = llvm::MemoryBuffer::getMemBufferCopy( source, name );
	const FileEntry* FE = compiler.getFileManager().getVirtualFile( name, 
			strlen(Buffer->getBufferStart()), time(NULL) );
	compiler.getSourceManager().overrideFileContents( FE, Buffer );
	compiler.getFrontendOpts().Inputs.clear();
	compiler.getFrontendOpts().Inputs.push_back( FrontendInputFile( name, IK_CXX ) );
}

const char *source_caption_pattern = "^ @{1,2} %[ %s* %w+ %s* %( %s* (|%w+ (|%. %w+)) %s* %)";
const char *header_suffix_pattern = "%. (h | hxx | hpp)";

DaoRegex *source_caption_regex = NULL;
DaoRegex *header_suffix_regex = NULL;

static void dao_cxx_parse( DString *VT, DString *source, DList *markers )
{
	DString *marker = NULL;
	daoint start = 0, rb = DString_FindChar( VT, ']', 0 );
	DString_SetBytes( source, VT->chars + rb + 1, VT->size - 2*(rb + 1) );
	while( start < source->size && isspace( source->chars[start] ) ) start += 1;
	while( start < source->size && source->chars[start] == '@' ){
		daoint end = DString_FindChar( source, '\n', start );
		if( end == -1 ) break;
		if( marker == NULL ) marker = DString_New();
		DString_SetBytes( marker, source->chars + start, end - start );
		DList_Append( markers, marker );
		start = end + 1;
		while( start < source->size && isspace( source->chars[start] ) ) start += 1;
	}
	if( marker ) DString_Delete( marker );
	DString_Erase( source, 0, start );
}
static daoint dao_string_find_paired( DString *mbs, char lb, char rb )
{
	daoint i, count = 0, N = mbs->size;
	for(i=0; i<N; i++){
		if( mbs->chars[i] == lb ){
			count -= 1;
		}else if( mbs->chars[i] == rb ){
			count += 1;
			if( count == 0 ) return i;
		}
	}
	return -1;
}
static int dao_markers_get( DList *markers, const char *name, DString *one, DList *all )
{
	daoint i, m = 0, npos = -1;
	for(i=0; i<markers->size; i++){
		DString *marker = markers->items.pString[i];
		daoint lb = DString_FindChar( marker, '(', 0 );
		daoint rb = dao_string_find_paired( marker, '(', ')' );
		if( lb == npos || rb == npos ) continue;
		DString_SetBytes( one, marker->chars + 1, lb - 1 );
		DString_Trim( one, 1, 1, 0 );
		if( strcmp( one->chars, name ) ) continue;
		DString_SetBytes( one, marker->chars + lb + 1, rb - lb - 1 );
		DString_Trim( one, 1, 1, 0 );
		DList_Erase( markers, i, 1 );
		i -= 1;
		m += 1;
		if( all == NULL ) break;
		DList_Append( all, one );
	}
	return m;
}
const char *dao_wrapper = "( DaoProcess *_proc, DaoValue *_p[], int _n )";
const char *dao_cxx_default_includes =
"#include<stdio.h>\n"
"#include<stdlib.h>\n"
"#include<math.h>\n"
"#include<dao.h>\n";

static int dao_make_wrapper( DString *name, DaoType *routype, DString *cproto, DString *wrapper, DString *cc )
{
	DString *pname;
	DaoType *type, **partypes = routype->nested->items.pType;
	int i, parcount = routype->nested->size;
	char sindex[10];

	DString_Clear( cc );
	DString_Clear( cproto );

	DString_Append( cc, name );
	DString_Append( cproto, name );
	DString_AppendChars( cc, "( " );
	DString_AppendChars( cproto, "( " );
	DString_AppendChars( wrapper, "void dao_" );
	DString_Append( wrapper, name );
	DString_AppendChars( wrapper, dao_wrapper );
	DString_AppendChars( wrapper, "\n{\n" );
	for(i=0; i<parcount; i++){
		type = partypes[i];
		if( type->tid != DAO_PAR_NAMED && type->tid != DAO_PAR_DEFAULT ) return 1;
		pname = type->fname;
		type = & type->aux->xType;
		if( i ){
			DString_AppendChars( cc, ", " );
			DString_AppendChars( cproto, ", " );
		}
		sprintf( sindex, "%i", i );
		DString_AppendChar( wrapper, '\t' );
		switch( type->tid ){
		case DAO_INTEGER :
		case DAO_FLOAT :
		case DAO_DOUBLE :
			DString_Append( cc, pname );
			DString_Append( cproto, type->name );
			DString_AppendChar( cproto, ' ' );
			DString_Append( cproto, pname );
			DString_Append( wrapper, type->name );
			DString_AppendChar( wrapper, ' ' );
			DString_Append( wrapper, pname );
			switch( type->tid ){
			case DAO_INTEGER : DString_AppendChars( wrapper, " = DaoValue_TryGetInteger( _p[" ); break;
			case DAO_FLOAT   : DString_AppendChars( wrapper, " = DaoValue_TryGetFloat( _p[" ); break;
			case DAO_DOUBLE  : DString_AppendChars( wrapper, " = DaoValue_TryGetDouble( _p[" ); break;
			}
			DString_AppendChars( wrapper, sindex );
			DString_AppendChars( wrapper, "] );\n" );
			break;
		case DAO_COMPLEX :
			DString_Append( cc, pname );
			DString_AppendChars( cproto, "complex16 " );
			DString_Append( cproto, pname );
			DString_AppendChars( wrapper, "complex16 " );
			DString_Append( wrapper, pname );
			DString_AppendChars( wrapper, " = DaoValue_TryGetComplex( _p[" );
			DString_AppendChars( wrapper, sindex );
			DString_AppendChars( wrapper, "] );\n" );
			break;
		case DAO_STRING :
			DString_Append( cc, pname );
			DString_AppendChars( cproto, "const char *" );
			DString_Append( cproto, pname );
			DString_AppendChars( wrapper, "const char *" );
			DString_Append( wrapper, pname );
			DString_AppendChars( wrapper, " = DaoValue_TryGetChars( _p[" );
			DString_AppendChars( wrapper, sindex );
			DString_AppendChars( wrapper, "] );\n" );
			break;
		case DAO_ENUM :
			DString_Append( cc, pname );
			DString_AppendChars( cproto, "int " );
			DString_Append( cproto, pname );
			DString_AppendChars( wrapper, "int " );
			DString_Append( wrapper, pname );
			DString_AppendChars( wrapper, " = DaoValue_TryGetEnum( _p[" );
			DString_AppendChars( wrapper, sindex );
			DString_AppendChars( wrapper, "] );\n" );
			break;
		case DAO_ARRAY :
			DString_Append( cc, pname );
			DString_AppendChars( cproto, "DaoArray *" );
			DString_Append( cproto, pname );
			DString_AppendChars( wrapper, "DaoArray *" );
			DString_Append( wrapper, pname );
			DString_AppendChars( wrapper, " = DaoValue_CastArray( _p[" );
			DString_AppendChars( wrapper, sindex );
			DString_AppendChars( wrapper, "] );\n" );
			break;
		case DAO_LIST :
			DString_Append( cc, pname );
			DString_AppendChars( cproto, "DaoList *" );
			DString_Append( cproto, pname );
			DString_AppendChars( wrapper, "DaoList *" );
			DString_Append( wrapper, pname );
			DString_AppendChars( wrapper, " = DaoValue_CastList( _p[" );
			DString_AppendChars( wrapper, sindex );
			DString_AppendChars( wrapper, "] );\n" );
			break;
		case DAO_TUPLE :
			DString_Append( cc, pname );
			DString_AppendChars( cproto, "DaoTuple *" );
			DString_Append( cproto, pname );
			DString_AppendChars( wrapper, "DaoTuple *" );
			DString_Append( wrapper, pname );
			DString_AppendChars( wrapper, " = DaoValue_CastTuple( _p[" );
			DString_AppendChars( wrapper, sindex );
			DString_AppendChars( wrapper, "] );\n" );
			break;
		case DAO_MAP :
			DString_Append( cc, pname );
			DString_AppendChars( cproto, "DaoMap *" );
			DString_Append( cproto, pname );
			DString_AppendChars( wrapper, "DaoMap *" );
			DString_Append( wrapper, pname );
			DString_AppendChars( wrapper, " = DaoValue_CastMap( _p[" );
			DString_AppendChars( wrapper, sindex );
			DString_AppendChars( wrapper, "] );\n" );
			break;
		case DAO_CDATA :
			if( strcmp( type->name->chars, "cdata" ) == 0 ){
				DString_Append( cc, pname );
				DString_AppendChars( cproto, "void *" );
				DString_Append( cproto, pname );
				DString_AppendChars( wrapper, "void *" );
				DString_Append( wrapper, pname );
				DString_AppendChars( wrapper, " = DaoValue_TryGetCdata( _p[" );
				DString_AppendChars( wrapper, sindex );
				DString_AppendChars( wrapper, "] );\n" );
			}else if( DaoType_MatchTo( type, dao_type_stream2, NULL ) ){
				DString_Append( cc, pname );
				DString_AppendChars( cproto, "FILE *" );
				DString_Append( cproto, pname );
				DString_AppendChars( wrapper, "FILE *" );
				DString_Append( wrapper, pname );
				DString_AppendChars( wrapper, " = DaoStream_GetFile( DaoValue_CastStream( _p[" );
				DString_AppendChars( wrapper, sindex );
				DString_AppendChars( wrapper, "] ) );\n" );
			}else{
				return 1;
			}
			break;
		default : return 1;
		}
	}
	DString_AppendChars( cc, " );\n" );
	DString_AppendChars( cproto, " )" );
	DString_AppendChar( wrapper, '\t' );

	type = & routype->aux->xType;
	if( type == NULL || type->tid == DAO_NONE ){
		DString_InsertChars( cproto, "void ", 0, 0, 0 );
		DString_Append( wrapper, cc );
	}else{
		switch( type->tid ){
		case DAO_INTEGER :
		case DAO_FLOAT :
		case DAO_DOUBLE :
			DString_InsertChars( cproto, " ", 0, 0, 0 );
			DString_Insert( cproto, type->name, 0, 0, 0 );
			DString_Append( wrapper, type->name );
			DString_AppendChars( wrapper, " _res = " );
			DString_Append( wrapper, cc );
			switch( type->tid ){
			case DAO_INTEGER :
				DString_AppendChars( wrapper, "DaoProcess_PutInteger( _proc, _res );\n" );
				break;
			case DAO_FLOAT :
				DString_AppendChars( wrapper, "DaoProcess_PutFloat( _proc, _res );\n" );
				break;
			case DAO_DOUBLE :
				DString_AppendChars( wrapper, "DaoProcess_PutDouble( _proc, _res );\n" );
				break;
			}
			break;
		case DAO_COMPLEX :
			DString_InsertChars( cproto, "complex16 ", 0, 0, 0 );
			DString_AppendChars( wrapper, "complex16 " );
			DString_AppendChars( wrapper, " _res = " );
			DString_Append( wrapper, cc );
			DString_AppendChars( wrapper, "DaoProcess_PutComplex( _proc, _res );\n" );
			break;
		case DAO_STRING :
			DString_InsertChars( cproto, "const char* ", 0, 0, 0 );
			DString_AppendChars( wrapper, "const char* " );
			DString_AppendChars( wrapper, " _res = " );
			DString_Append( wrapper, cc );
			DString_AppendChars( wrapper, "DaoProcess_PutChars( _proc, _res );\n" );
			break;
		case DAO_CDATA :
			if( strcmp( type->name->chars, "cdata" ) == 0 ){
				DString_InsertChars( cproto, "void* ", 0, 0, 0 );
				DString_AppendChars( wrapper, "void* " );
				DString_AppendChars( wrapper, " _res = " );
				DString_Append( wrapper, cc );
				DString_AppendChars( wrapper, "DaoProcess_PutCdata( _proc, _res, NULL );\n" );
			}else if( DaoType_MatchTo( type, dao_type_stream2, NULL ) ){
				DString_InsertChars( cproto, "FILE* ", 0, 0, 0 );
				DString_AppendChars( wrapper, "FILE* " );
				DString_AppendChars( wrapper, " _res = " );
				DString_Append( wrapper, cc );
				DString_AppendChars( wrapper, "DaoProcess_PutFile( _proc, _res );\n" );
			}else{
				return 1;
			}
			break;
		case DAO_NONE :
			DString_InsertChars( cproto, "void ", 0, 0, 0 );
			DString_Append( wrapper, cc );
			break;
		default : return 1;
		}
	}
	DString_AppendChars( wrapper, "\n}\n" );
	//printf( "\n%s\n%s\n", cproto->chars, wrapper->chars );
	return 0;
}

static int error_compile_failed( DString *out )
{
	DString_SetChars( out, "Compiling failed on the inlined codes" );
	return 1;
}
static int error_function_notfound( DString *out, const char *name )
{
	DString_SetChars( out, "Function not found in the inlined codes: " );
	DString_AppendChars( out, name );
	return 1;
}
static int error_function_notwrapped( DString *out, const char *name )
{
	DString_SetChars( out, "Function wrapping failed: " );
	DString_AppendChars( out, name );
	return 1;
}
static uint_t DaoRotatingHash( DString *text )
{
	int i, len = text->size;
	uint_t hash = text->size;
	for(i=0; i<len; ++i) hash = ((hash<<4)^(hash>>28)^text->chars[i])&0x7fffffff;
	return hash;
}
static void dao_make_anonymous_name( char *name, DaoNamespace *NS, DString *VT, const char *prefix, const char *suffix )
{
	uint_t hash1, hash2;
	DString *path = DString_Copy( NS->name );
	DString_Erase( path, path->size - NS->lang->size - 1, -1 );
	hash1 = DaoRotatingHash( path );
	hash2 = DaoRotatingHash( VT );
	sprintf( name, "%sanonymous_%x_%x%s", prefix, hash1, hash2, suffix );
}

static int dao_cxx_block( DaoNamespace *NS, DString *VT, DList *markers, DString *source, DString *out )
{
	char bytes[200];
	char name[50];

	dao_make_anonymous_name( name, NS, VT, "", "" );
	sprintf( bytes, "void %s(){\n", name );
	DString_InsertChars( source, bytes, 0, 0, 0 );
	DString_InsertChars( source, dao_cxx_default_includes, 0, 0, 0 );

	sprintf( bytes, "\n}\nextern \"C\"{\nvoid dao_%s%s{\n\t%s();\n}\n}", name, dao_wrapper, name );
	DString_AppendChars( source, bytes );

	dao_make_anonymous_name( name, NS, VT, "", ".cxx" );
	//printf( "%s:\n%s\n", name, source->chars );
	DaoCXX_AddVirtualFile( name, source->chars );

	//action.BeginSourceFile( compiler, FrontendInputFile( name, IK_CXX ) );
	if( ! compiler.ExecuteAction( action ) ) return error_compile_failed( out );

	llvm::Module *module = action.takeModule();
	if( module == NULL ) return error_compile_failed( out );

	dao_make_anonymous_name( name, NS, VT, "dao_", "" );
	Function *Func = module->getFunction( name );
	if( Func == NULL ) return error_function_notfound( out, name );

	void *fp = engine->getPointerToFunction( Func );
	if( fp == NULL ) return error_function_notfound( out, name );

	DString_SetChars( out, name );
	DString_AppendChars( out, "()" );
	DaoNamespace_WrapFunction( NS, (DaoCFunction)fp, out->chars );
	DString_AppendChar( out, ';' );
	return 0;
}
static int dao_cxx_function( DaoNamespace *NS, DString *VT, DList *markers, DString *source, DString *out )
{
	int retc;
	char file[50];
	char proto2[200];
	char *proto = proto2;
	DString *mbs = DString_New();

	dao_make_anonymous_name( file, NS, VT, "", ".cxx" );
	dao_make_anonymous_name( proto2, NS, VT, "", "()" );
	if( dao_markers_get( markers, "define", mbs, NULL ) ) proto = mbs->chars;

	DaoRoutine *func = DaoNamespace_WrapFunction( NS, DaoCXX_Default, proto );
	if( func == NULL ){
		error_function_notwrapped( out, proto );
		DString_Delete( mbs );
		return 1;
	}
	
	DString *cproto = DString_New();
	DString *call = DString_New();

	DString_AppendChars( source, "}\nextern \"C\"{\n" );
	retc = dao_make_wrapper( func->routName, func->routType, cproto, source, call );

	DString_AppendChars( cproto, "{\n" );
	DString_Insert( source, cproto, 0, 0, 0 );
	DString_InsertChars( source, dao_cxx_default_includes, 0, 0, 0 );
	DString_AppendChars( source, "}\n" );

	DString_Delete( mbs );
	DString_Delete( cproto );
	DString_Delete( call );

	if( retc ) return error_function_notwrapped( out, func->routName->chars );

	//printf( "\n%s\n%s\n", file, source->chars );
	DaoCXX_AddVirtualFile( file, source->chars );

	if( ! compiler.ExecuteAction( action ) ) return error_compile_failed( out );

	llvm::Module *module = action.takeModule();
	if( module == NULL ) return error_compile_failed( out );

	sprintf( proto2, "dao_%s", func->routName->chars ); //XXX buffer size
	Function *Func = module->getFunction( proto2 );
	if( Func == NULL ) return error_function_notfound( out, proto2 );

	void *fp = engine->getPointerToFunction( Func );
	if( fp == NULL ) return error_function_notfound( out, proto2 );

	func->pFunc = (DaoCFunction)fp;
	return 0;
}
static int dao_cxx_header( DaoNamespace *NS, DString *VT, DList *markers, DString *source, DString *out )
{
	DString *mbs = DString_New();
	char name[50];
	char *file = name;
	dao_make_anonymous_name( name, NS, VT, "", ".h" );
	if( dao_markers_get( markers, "file", mbs, NULL ) ){
		// TODO: better handling of suffix?
		if( isalnum( mbs->chars[0] ) ) DString_InsertChars( mbs, "./", 0, 0, 0 );
		file = mbs->chars;
	}
	DaoCXX_AddVirtualFile( file, source->chars );
	DString_Delete( mbs );
	return 0;
}
static int dao_cxx_source( DaoNamespace *NS, DString *VT, DList *markers, DString *source, DString *out )
{
	DString *mbs = DString_New();
	DString *call = DString_New();
	DString *cproto = DString_New();
	DList *wraps = DList_New(DAO_DATA_STRING);
	DList *funcs = DList_New(0);
	InputKind kind = IK_CXX;
	char name[200];
	char *file = name;
	daoint i, failed = 0;

	dao_make_anonymous_name( name, NS, VT, "", ".cxx" );
	dao_markers_get( markers, "wrap", mbs, wraps );
	if( dao_markers_get( markers, "file", mbs, NULL ) ){
		file = mbs->chars;
		// TODO: better handling of suffix?
		if( DString_FindChars( mbs, ".c", 0 ) == mbs->size - 2 ) kind = IK_C;
		if( DString_FindChars( mbs, ".C", 0 ) == mbs->size - 2 ) kind = IK_C;
	}

	DString_Clear( out );
	DString_InsertChars( source, dao_cxx_default_includes, 0, 0, 0 );
	DString_AppendChars( source, "\nextern \"C\"{\n" );
	for(i=0; i<wraps->size; i++){
		DString *daoproto = wraps->items.pString[i];
		DaoRoutine *func = DaoNamespace_WrapFunction( NS, DaoCXX_Default, daoproto->chars );
		if( func == NULL || dao_make_wrapper( func->routName, func->routType, cproto, source, call ) ){
			DString_AppendChars( out, daoproto->chars );
			DString_AppendChars( out, "\n" );
			failed += 1;
			continue;
		}
		DList_Append( funcs, func );
	}
	DString_AppendChars( source, "}\n" );

	//printf( "\n%s\n%s\n", file, source->chars );
	if( failed == 0 ) DaoCXX_AddVirtualFile( file, source->chars );
	DString_Delete( mbs );
	DString_Delete( call );
	DString_Delete( cproto );

	if( failed ){
		DString_InsertChars( out, "Function wrapping failed:\n", 0, 0, 0 );
		return 1;
	}

	if( ! compiler.ExecuteAction( action ) ) return error_compile_failed( out );

	llvm::Module *module = action.takeModule();
	if( module == NULL ) return error_compile_failed( out );

	for(i=0; i<funcs->size; i++){
		DaoRoutine *func = funcs->items.pRoutine[i];
		sprintf( name, "dao_%s", func->routName->chars ); //XXX buffer size

		Function *Func = module->getFunction( name );
		if( Func == NULL ) return error_function_notfound( out, name );

		void *fp = engine->getPointerToFunction( Func );
		if( fp == NULL ) return error_function_notfound( out, name );
		func->pFunc = (DaoCFunction)fp;
	}
	return 0;
}

static int dao_cxx_inliner( DaoNamespace *NS, DString *mode, DString *verbatim, DString *out, int line )
{
	DString *source = DString_New();
	DList *markers = DList_New(DAO_DATA_STRING);
	int retc = 1;

	dao_cxx_parse( verbatim, source, markers );

	if( mode->size == 0 || strcmp( mode->chars, "block" ) ==0 ){
		retc = dao_cxx_block( NS, verbatim, markers, source, out );
	}else if( strcmp( mode->chars, "function" ) ==0 ){
		retc = dao_cxx_function( NS, verbatim, markers, source, out );
	}else if( strcmp( mode->chars, "header" ) ==0 ){
		retc = dao_cxx_header( NS, verbatim, markers, source, out );
	}else if( strcmp( mode->chars, "source" ) ==0 ){
		retc = dao_cxx_source( NS, verbatim, markers, source, out );
	}else{
		DString_SetChars( out, "Invalid inline mode" );
	}
	if( markers->size ){
		DString_AppendChars( out, "Invalid specifiers for the mode: \n" );
		for(daoint i=0; i<markers->size; i++){
			DString *marker = markers->items.pString[i];
			DString_Append( out, marker );
			DString_AppendChar( out, '\n' );
		}
		retc = 1;
	}
	DString_Delete( source );
	DList_Delete( markers );
	return retc;
}
static int dao_cxx_loader( DaoNamespace *NS, DString *file, DString *emsg )
{
	DList *markers;
	DString *source, *marker = NULL;
	daoint start = 0;
	int retc = 0;

	source = DString_New();
	if( DaoFile_ReadAll( fopen( file->chars, "r" ), source, 1 ) == 0 ){
		DString_Delete( source );
		return 1;
	}
	marker = DString_New();
	markers = DList_New(DAO_DATA_STRING);
	while( start < source->size && source->chars[start] == '@' ){
		daoint end = DString_FindChar( source, '\n', start );
		if( end == -1 ) break;
		DString_SetBytes( marker, source->chars + start, end - start );
		DList_Append( markers, marker );
		start = end + 1;
		while( start < source->size && isspace( source->chars[start] ) ) start += 1;
	}
	DString_Erase( source, 0, start );
	retc = dao_cxx_source( NS, file, markers, source, emsg );
	if( markers->size ){
		DString_AppendChars( emsg, "Invalid specifiers in the module file: \n" );
		for(daoint i=0; i<markers->size; i++){
			DString *marker = markers->items.pString[i];
			DString_Append( emsg, marker );
			DString_AppendChar( emsg, '\n' );
		}
		retc = 1;
	}
	DString_Delete( source );
	DString_Delete( marker );
	DList_Delete( markers );
	return retc;
}

extern "C"{
DAO_DLL int DaoCxx_OnLoad( DaoVmSpace *vms, DaoNamespace *ns );
}

DAO_DLL int DaoCxx_OnLoad( DaoVmSpace *vms, DaoNamespace *ns )
{
	static int argc = 2;
	const char *argv[2] = { "dao", "dummy-main.cpp" };
	DString *mbs = DString_New();

	DString_SetChars( mbs, source_caption_pattern );
	source_caption_regex = DaoRegex_New( mbs );

	DString_SetChars( mbs, header_suffix_pattern );
	header_suffix_regex = DaoRegex_New( mbs );

	compiler.createDiagnostics(argc, argv);

	DiagnosticsEngine & DG = compiler.getDiagnostics();
	CompilerInvocation::CreateFromArgs( compiler.getInvocation(), argv + 1, argv + argc, DG );
	compiler.setTarget( TargetInfo::CreateTargetInfo( DG, compiler.getTargetOpts() ) );

	clang::HeaderSearchOptions & headers = compiler.getHeaderSearchOpts();
	DString_SetChars( mbs, DaoVmSpace_CurrentLoadingPath( vms ) );
	DString_AppendChars( mbs, "/../" ); // /usr/local/dao relative to /usr/local/dao/lib
	headers.AddPath( mbs->chars, clang::frontend::System, false, false, true );
#ifdef DAO_DIR
	headers.AddPath( DAO_DIR "/include", clang::frontend::System, false, false, true );
#endif
	DString_SetChars( mbs, DaoVmSpace_CurrentLoadingPath( vms ) );
	DString_AppendChars( mbs, "/../../kernel" ); // at build
	headers.AddPath( mbs->chars, clang::frontend::System, false, false, true );

	DString_Delete( mbs );

	string predefines;
#ifdef MAC_OSX
	predefines = "#define MACOSX 1\n#define UNIX 1\n";
	// needed to circumvent a bug which is supposingly fixed in clang 2.9-16
	headers.AddPath( "/Developer/SDKs/MacOSX10.5.sdk/usr/lib/gcc/i686-apple-darwin9/4.2.1/include", clang::frontend::System, false, false, true );
	// workaround for finding: stdarg.h
	headers.AddPath( "/usr/lib/clang/3.2/include", clang::frontend::System, false, false, true );
	headers.AddPath( "/usr/lib/clang/4.2/include", clang::frontend::System, false, false, true );
	headers.AddPath( "/usr/local/lib/clang/3.2/include", clang::frontend::System, false, false, true );
#elif defined(UNIX)
	predefines = "#define UNIX 1\n";
#elif defined(WIN32)
	predefines = "#define WIN32 1\n";
	headers.AddPath( "C:/MinGW/lib/gcc/mingw32/4.6.1/include", clang::frontend::System, false, false, true );
#endif

	//compiler.getHeaderSearchOpts().AddPath( "", clang::frontend::Angled, false, false, true );

	compiler.createFileManager();
	compiler.createSourceManager( compiler.getFileManager() );
	compiler.createPreprocessor();
	Preprocessor & pp = compiler.getPreprocessor();
	pp.setPredefines( pp.getPredefines() + "\n" + predefines );

	DaoCXX_AddVirtualFile( "dummy-main.cpp", "void dummy_main(){}" );

	InitializeNativeTarget();
	if( ! compiler.ExecuteAction( action ) ) return 1;

	std::string Error;
	engine = ExecutionEngine::createJIT( action.takeModule(), &Error );
	if( engine == NULL ){
		errs() << Error << "\n";
		return 1;
	}

	DaoNamespace_AddCodeInliner( ns, "cxx", dao_cxx_inliner );
	DaoNamespace_AddCodeInliner( ns, "cpp", dao_cxx_inliner );
	DaoNamespace_TypeDefine( ns, "int", "short" );
	DaoNamespace_TypeDefine( ns, "int", "size_t" );
	DaoNamespace_TypeDefine( ns, "int", "int8_t" );
	DaoNamespace_TypeDefine( ns, "int", "uint8_t" );
	DaoNamespace_TypeDefine( ns, "int", "int16_t" );
	DaoNamespace_TypeDefine( ns, "int", "uint16_t" );
	DaoNamespace_TypeDefine( ns, "int", "int32_t" );
	DaoNamespace_TypeDefine( ns, "int", "uint32_t" );
	DaoNamespace_TypeDefine( ns, "int", "int64_t" );
	DaoNamespace_TypeDefine( ns, "int", "uint64_t" );
	ns = DaoVmSpace_GetNamespace( vms, "io" );
	dao_type_stream2 = DaoNamespace_FindTypeChars( ns, "stream" );
	return 0;
}
