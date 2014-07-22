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

// 2011-01: Danilov Aleksey, initial implementation.

#include"dao.h"
#include"daoValue.h"
#include"daoStdtype.h"
#include"daoNumtype.h"
#include"daoStream.h"

static void DaoBinary_Read( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[0]->xStream;
	FILE* file = stream->file;
	DaoArray *arr = &p[1]->xArray;
	size_t count =  p[2]->xInteger.value;
	size_t size = 0;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_READABLE ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not readable" );
		return;
	}
	switch( arr->etype ){
	case DAO_INTEGER:	size = sizeof(daoint); break;
	case DAO_FLOAT:		size = sizeof(float); break;
	case DAO_DOUBLE:	size = sizeof(double); break;
	case DAO_COMPLEX:	size = sizeof(complex16); break;
	}
	if( count == 0 || count > arr->size )
		count = arr->size;
	DaoProcess_PutInteger( proc, fread( arr->data.p, size, count, file ) );
}

static void DaoBinary_Unpack( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[0]->xStream;
	FILE* file = stream->file;
	DaoArray *arr = &p[1]->xArray;
	size_t sizes[] = {1, 2, 4};
	size_t size =  sizes[p[2]->xEnum.value];
	size_t count = p[3]->xInteger.value;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_READABLE ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not readable" );
		return;
	}
	if( count <= 0 || count > arr->size*sizeof(daoint)/size )
		count = arr->size*sizeof(daoint)/size;
	count = fread( arr->data.p, size, count, file );
	if( size != sizeof(daoint) ){
		size_t i;
		char *bytes = (char*)arr->data.p;
		for( i = 0; i < count; i++ ){
			size_t j = count - i - 1;
			arr->data.i[j] = bytes[j];
		}
	}
	DaoProcess_PutInteger( proc, count );
}

static void DaoBinary_Pack( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[1]->xStream;
	FILE* file = stream->file;
	DaoArray *arr = &p[0]->xArray;
	size_t sizes[] = {1, 2, 4};
	size_t size =  sizes[p[2]->xEnum.value];
	size_t count = p[3]->xInteger.value, res = 0;
	size_t i;
	DaoArray_Sliced( arr );
	count = arr->size;
	if( !file ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_WRITABLE ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not writable" );
		return;
	}
	if( count <= 0 || count > arr->size )
		count = arr->size;
	if( size != sizeof(daoint) ){
		void *data = dao_malloc( size*count );
		switch( size ){
		case 1:
			if( 1 ){
				char *bytes = (char*)data;
				for( i = 0; i < count; i++ )
					bytes[i] = arr->data.i[i];
			}
			break;
		case 2:
			if( 1 ){
				short *words = (short*)data;
				for( i = 0; i < count; i++ )
					words[i] = arr->data.i[i];
			}
			break;
		case 4:
			if( 1 ){
				int *dwords = (int*)data;
				for( i = 0; i < count; i++ )
					dwords[i] = arr->data.i[i];
			}
			break;
		}
		res = fwrite( data, size, count, file );
		dao_free( data );
	}
	else
		res = fwrite( arr->data.i, size, count, file );
	DaoProcess_PutInteger( proc, res );
}

static void DaoBinary_Write( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[1]->xStream;
	FILE* file = stream->file;
	DaoArray *arr = &p[0]->xArray;
	size_t size = 0;
	size_t count = p[2]->xInteger.value;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_WRITABLE ) == 0 ){
		DaoProcess_RaiseError( proc, "Param", "The stream is not writable" );
		return;
	}
	switch( arr->etype ){
	case DAO_INTEGER:	size = sizeof(daoint); break;
	case DAO_FLOAT:		size = sizeof(float); break;
	case DAO_DOUBLE:	size = sizeof(double); break;
	case DAO_COMPLEX:	size = sizeof(complex16); break;
	}
	if( !count || count > arr->size )
		count = arr->size;
	DaoProcess_PutInteger( proc, fwrite( arr->data.p, size, count, file ) );
}

enum {
	Binary_Byte = 0,
	Binary_UByte,
	Binary_Word,
	Binary_UWord,
	Binary_DWord,
	Binary_UDWord
};

typedef int bin_data;

static void DaoBinary_GetItem( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	bin_data type = p[1]->xEnum.value;
	daoint offset = p[2]->xInteger.value;
	char *tpname = DaoProcess_GetReturnType( proc )->name->chars;
	char *data;
	DaoArray_Sliced( arr );
	data = (char*)arr->data.p + offset;
	if ( offset < 0 )
		DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
	else if ( strcmp( tpname, "double" ) == 0 ){
		if ( offset + sizeof(double) > arr->size*sizeof(daoint) ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		DaoProcess_PutDouble( proc, *(double*)data );
		return;
	}
	else if ( strcmp( tpname, "float" ) == 0 ){
		if ( offset + sizeof(float) > arr->size*sizeof(daoint) ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		DaoProcess_PutFloat( proc, *(float*)data  );
	}
	else {
		size_t sizes[] = {1, 1, 2, 2, 4, 4};
		size_t size = sizes[type];
		daoint num;
		if ( offset + size > arr->size*sizeof(daoint) ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		switch( type ){
		case Binary_UByte:	num = *(uchar_t*)data; break;
		case Binary_Byte:	num = *(char*)data; break;
		case Binary_UWord:  num = *(ushort_t*)data; break;
		case Binary_Word:	num = *(short*)data; break;
		case Binary_UDWord: num = *(uint_t*)data; break;
		case Binary_DWord:	num = *(int*)data; break;
		}
		DaoProcess_PutInteger( proc, num );
	}
}

static void DaoBinary_SetItem( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	bin_data type = p[1]->xEnum.value;
	daoint offset = p[2]->xInteger.value;
	char *data;
	DaoArray_Sliced( arr );
	data = (char*)arr->data.p + offset;
	if ( offset < 0 )
		DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
	else if ( p[3]->type == DAO_DOUBLE ){
		if ( offset + sizeof(double) > arr->size*sizeof(daoint) ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		*(double*)data = p[3]->xDouble.value;
		return;
	}
	else if ( p[3]->type == DAO_FLOAT ){
		if ( offset + sizeof(float) > arr->size*sizeof(daoint) ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		*(float*)data  = p[3]->xFloat.value;
	}
	else {
		size_t sizes[] = {1, 1, 2, 2, 4, 4};
		size_t size = sizes[type];
		daoint num = p[3]->xInteger.value;
		if ( offset + size > arr->size*sizeof(daoint) ){
			DaoProcess_RaiseError( proc, "Index::Range", "Invalid offset" );
			return;
		}
		switch( type ){
		case Binary_UByte:	*(uchar_t*)data = num; break;
		case Binary_Byte:	*(char*)data = num; break;
		case Binary_UWord:  *(ushort_t*)data = num; break;
		case Binary_Word:	*(short*)data = num; break;
		case Binary_UDWord: *(uint_t*)data = num; break;
		case Binary_DWord:	*(int*)data = num; break;
		}
	}
}

/*! \note All the routines below accepting \c io::stream expect a file stream only */
static DaoFuncItem binMeths[] =
{
	/*! Reads \a count elements from \a source to \a dest. If \a count is zero, or greater than \a dest size,
	 * \a dest size is assumed. Returns the number of elements actually read */
	{ DaoBinary_Read,		"read(source: io::stream, dest: array<@T>, count = 0) => int" },

	/*! Reads \a count chunks of size \a size from \a source into \a dest so that each chunk corresponds to a single
	 * \a dest element (with possible widening). If \a count is zero, or greater than \a dest element size, \a dest
	 * element size is assumed. Returns the number of chunks actually read */
	{ DaoBinary_Unpack,		"unpack(source: io::stream, dest: array<int>, size: enum<byte,word,dword>, count = 0) => int" },

	/*! Writes \a count chunks of size \a size to \a dest so that each \a source element corresponds to a single chunk
	 * (with possible narrowing). Returns the number of chunks actually written */
	{ DaoBinary_Pack,		"pack(invar source: array<int>, dest: io::stream, size: enum<byte,word,dword>, count = 0) => int" },

	/*! Writes \a count elements from \a source to \a dest. If \c count is zero, or greater than \a dest size, all \a dest data is
	 * written. Returns the number of elements actually written */
	{ DaoBinary_Write,		"write(invar source: array<@T>, dest: io::stream, count = 0) => int" },

	/*! Reads value described by \a what from \a source at the given \a offset */
	{ DaoBinary_GetItem,	"get(invar source: array<int>, what: enum<byte,ubyte,word,uword,dword,udword>, offset: int) => int" },
	{ DaoBinary_GetItem,	"get(invar source: array<int>, what: enum<float>, offset: int) => float" },
	{ DaoBinary_GetItem,	"get(invar source: array<int>, what: enum<double>, offset: int) => double" },

	/*! Writes \a value described by \a what to \a dest at the given \a offset */
	{ DaoBinary_SetItem,	"set(dest: array<int>, what: enum<byte,ubyte,word,uword,dword,udword>, offset: int, value: int)" },
	{ DaoBinary_SetItem,	"set(dest: array<int>, what: enum<float>, offset: int, value: float)" },
	{ DaoBinary_SetItem,	"set(dest: array<int>, what: enum<double>, offset: int, value: double)" },
	{ NULL, NULL }
};

DAO_DLL int DaoBinary_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *binns;
	binns = DaoVmSpace_GetNamespace( vmSpace, "bin" );
	DaoNamespace_AddConstValue( ns, "bin", (DaoValue*)binns );
	DaoNamespace_WrapFunctions( binns, binMeths );
	return 0;
}
