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

#include<stdio.h>
#include<sys/stat.h>

#include"dao.h"
#include"daoStdtype.h"
#include"daoStream.h"
#include"dao_sys.h"

#ifdef WIN32
#define fstat _fstat
#define stat _stat
#define fileno _fileno
#endif

static void DaoBufferRead( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = DaoValue_CastStream( p[0] );
	FILE* file = DaoStream_GetFile( stream );
	Dao_Buffer *buffer = (Dao_Buffer*)DaoValue_CastCdata( p[1] );
	size_t size = DaoValue_TryGetInteger( p[2] );
	if( !file )
		file = stdin;
	if( ( stream->mode & DAO_IO_READ ) == 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The stream is not readable" );
		return;
	}
	if( size == 0 || size > buffer->size )
		size = buffer->size;
	DaoProcess_PutInteger( proc, fread( buffer->buffer.pVoid, 1, size, file ) );
}

static void DaoBufferReadAll( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = DaoValue_CastStream( p[0] );
	FILE* file = DaoStream_GetFile( stream );
	Dao_Buffer *buffer;
	size_t size = DaoValue_TryGetInteger( p[1] );
	struct stat info;
	if( !file ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The stream size cannot be determined" );
		return;
	}
	if( ( stream->mode & DAO_IO_READ ) == 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The stream is not readable" );
		return;
	}
	fstat( fileno( file ), &info );
	if( size == 0 || info.st_size - ftell( file )/2 < size )
		size = info.st_size - ftell( file )/2;
	buffer = Dao_Buffer_New( size );
	size = fread( buffer->buffer.pVoid, 1, size, file );
	Dao_Buffer_Resize( buffer, size );
	DaoProcess_PutValue( proc, (DaoValue*)buffer );
}

static void DaoBufferReadAll2( DaoProcess *proc, DaoValue *p[], int N )
{
	FILE* file;
	Dao_Buffer *buffer;
	const char *fname = DaoString_GetMBS( DaoValue_CastString( p[1] ) );
	size_t size = DaoValue_TryGetInteger( p[2] );
	struct stat info;
	file = fopen( fname, "rb" );
	if( !file ){
		char errbuf[300];
		snprintf( errbuf, sizeof( errbuf ), "Error opening file: %s", fname );
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
		return;
	}
	fstat( fileno( file ), &info );
	if( size == 0 || info.st_size < size )
		size = info.st_size;
	buffer = Dao_Buffer_New( size );
	size = fread( buffer->buffer.pVoid, 1, size, file );
	Dao_Buffer_Resize( buffer, size );
	fclose( file );
	DaoProcess_PutValue( proc, (DaoValue*)buffer );
}

static void DaoBufferWrite( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = DaoValue_CastStream( p[0] );
	FILE* file = DaoStream_GetFile( stream );
	Dao_Buffer *buffer = (Dao_Buffer*)DaoValue_CastCdata( p[1] );
	size_t size = DaoValue_TryGetInteger( p[2] );
	if( !file )
		file = stdout;
	if( ( stream->mode & DAO_IO_WRITE ) == 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The stream is not writable" );
		return;
	}
	if( size == 0 || size > buffer->size )
		size = buffer->size;
	fwrite( buffer->buffer.pVoid, 1, size, file );
}

int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoVmSpace_LinkModule( vmSpace, ns, "sys" );
	DaoNamespace_WrapFunction( ns, (DaoCFunction)DaoBufferRead,   "fillbuf( self: stream, buf: buffer, count = 0 )=>int" );
	DaoNamespace_WrapFunction( ns, (DaoCFunction)DaoBufferReadAll,"readbuf( self: stream, count = 0 )=>buffer" );
	DaoNamespace_WrapFunction( ns, (DaoCFunction)DaoBufferReadAll2,
							   "readbuf( self: stream, file: string, count = 0 )=>buffer" );
	DaoNamespace_WrapFunction( ns, (DaoCFunction)DaoBufferWrite,   "writebuf( self: stream, buf: buffer, count = 0 )" );
	return 0;
}
