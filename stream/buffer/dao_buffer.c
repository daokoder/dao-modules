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

DAO_DLL int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoVmSpace_LinkModule( vmSpace, ns, "sys" );
	DaoNamespace_WrapFunction( ns, (DaoCFunction)DaoBufferRead,   "fillbuf( self: stream, buf: buffer, count = 0 )=>int" );
	DaoNamespace_WrapFunction( ns, (DaoCFunction)DaoBufferReadAll,"readbuf( self: stream, count = 0 )=>buffer" );
	DaoNamespace_WrapFunction( ns, (DaoCFunction)DaoBufferReadAll2,
							   "readbuf( self: stream, file: string, count = 0 )=>buffer" );
	DaoNamespace_WrapFunction( ns, (DaoCFunction)DaoBufferWrite,   "writebuf( self: stream, buf: buffer, count = 0 )" );
	return 0;
}
