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
#include"dao_sys.h"

#ifdef WIN32
#define fstat _fstat
#define stat _stat
#define fileno _fileno
#endif

static void DaoBinary_FillBuf( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[0]->xStream;
	FILE* file = stream->file;
	Dao_Buffer *buffer = Dao_Buffer_CastFromValue( p[1] );
	size_t count = p[2]->xInteger.value;
	if( !file ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_READABLE ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not readable" );
		return;
	}
	if( count == 0 || count > buffer->size )
		count = buffer->size;
	DaoProcess_PutInteger( proc, fread( buffer->buffer.pVoid, 1, count, file ) );
}

static void DaoBinary_ReadArr( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[0]->xStream;
	FILE* file = stream->file;
	DaoArray *arr = &p[1]->xArray;
	size_t count =  p[2]->xInteger.value;
	size_t size;
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
	size_t count =  p[3]->xInteger.value;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_READABLE ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not readable" );
		return;
	}
	if( count == 0 || count > arr->size )
		count = arr->size;
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

static void DaoBinary_ReadBuf( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[0]->xStream;
	FILE* file = stream->file;
	Dao_Buffer *buffer;
	size_t count = p[1]->xInteger.value;
	if( !file ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_READABLE ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not readable" );
		return;
	}
	/* max of int is too big, also SIZE_MAX not available on Mac: */
	if( count == 0 ) count = 1<<24;
	buffer = Dao_Buffer_New( count );
	count = fread( buffer->buffer.pVoid, 1, count, file );
	Dao_Buffer_Resize( buffer, count );
	DaoProcess_PutValue( proc, (DaoValue*)buffer );
}

static void DaoBinary_WriteBuf( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[1]->xStream;
	FILE* file = stream->file;
	Dao_Buffer *buffer = Dao_Buffer_CastFromValue( p[0] );
	size_t count = p[2]->xInteger.value;
	if( !file ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_WRITABLE ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not writable" );
		return;
	}
	if( count == 0 || count > buffer->size )
		count = buffer->size;
	fwrite( buffer->buffer.pVoid, 1, count, file );
}

static void DaoBinary_Pack( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[1]->xStream;
	FILE* file = stream->file;
	DaoArray *arr = &p[0]->xArray;
	size_t sizes[] = {1, 2, 4};
	size_t size =  sizes[p[2]->xEnum.value];
	size_t count =  p[3]->xInteger.value;
	size_t i;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_WRITABLE ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not writable" );
		return;
	}
	if( count == 0 || count > arr->size )
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
		fwrite( data, size, count, file );
		dao_free( data );
	}
	else
		fwrite( arr->data.i, size, count, file );
}

static void DaoBinary_WriteArr( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = &p[1]->xStream;
	FILE* file = stream->file;
	DaoArray *arr = &p[0]->xArray;
	size_t count =  p[2]->xInteger.value;
	size_t size = 0;
	DaoArray_Sliced( arr );
	if( !file ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not a file" );
		return;
	}
	if( ( stream->mode & DAO_STREAM_WRITABLE ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "The stream is not writable" );
		return;
	}
	if( count == 0 || count > arr->size )
		count = arr->size;
	switch( arr->etype ){
	case DAO_INTEGER:	size = sizeof(daoint); break;
	case DAO_FLOAT:		size = sizeof(float); break;
	case DAO_DOUBLE:	size = sizeof(double); break;
	case DAO_COMPLEX:	size = sizeof(complex16); break;
	}
	fwrite( arr->data.p, size, count, file );
}

static DaoFuncItem binMeths[] =
{
	/*! Fills \a dest with \a count bytes read from \a source. If \a count is zero, or greater than \a dest size,
	 * \a dest size is assumed. Returns the number of bytes read */
	{ DaoBinary_FillBuf,	"fill(source: io::stream, dest: buffer, count = 0) => int" },

	/*! Fills \a dest with \a count elements read from \a source. If \a count iszero, or greater than \a dest size,
	 * \a dest size is assumed. Returns the number of elements read */
	{ DaoBinary_ReadArr,	"fill(source: io::stream, dest: array<@T>, count = 0) => int" },

	/*! Reads \a count chunks of size \a size from \a source into \a dest so that each chunk corresponds to a single
	 * \a dest element. If \a count is zero, or greater than \a dest size, \a dest size is assumed.
	 * Returns the number of chunks read */
	{ DaoBinary_Unpack,		"unpack(source: io::stream, dest: array<int>, size: enum<byte,word,dword>, count = 0) => int" },

	/*! Writes \a count chunks of size \a size to \a dest so that each \a source element corresponds to a single
	 * chunk. If \a count is zero, or greater than \a dest size, \a dest size is assumed */
	{ DaoBinary_Pack,		"pack(invar source: array<int>, dest: io::stream, size: enum<byte,word,dword>, count = 0)" },

	/*! Reads \a count bytes from \a source and returns them as a buffer. If \a count is zero, 2^24 is assumed */
	{ DaoBinary_ReadBuf,	"read(source: io::stream, count = 0) => buffer" },

	/*! Writes \a count bytes from \a source into \a dest.  If \a count is zero, or greater than \a source size,
	 * \a source size is assumed */
	{ DaoBinary_WriteBuf,	"write(invar source: buffer, dest: io::stream, count = 0)" },

	/*! Writes \a count elements from \a source into \a dest.  If \a count is zero, or greater than \a source size,
	 * \a source size is assumed */
	{ DaoBinary_WriteArr,	"write(invar source: array<@T>, dest: io::stream, count = 0)" },
	{ NULL, NULL }
};

DAO_DLL int DaoBinary_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *binns;
	DaoVmSpace_LinkModule( vmSpace, ns, "sys" );
	binns = DaoVmSpace_GetNamespace( vmSpace, "bin" );
	DaoNamespace_AddConstValue( ns, "bin", (DaoValue*)binns );
	DaoNamespace_WrapFunctions( binns, binMeths );
	return 0;
}
