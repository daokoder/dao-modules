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
// THIS SOFTWARE IS PROVIDED  BY THE COPYRIGHT HOLDERS AND  CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED  WARRANTIES,  INCLUDING,  BUT NOT LIMITED TO,  THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR CONTRIBUTORS  BE LIABLE FOR ANY DIRECT,
// INDIRECT,  INCIDENTAL, SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL  DAMAGES (INCLUDING,
// BUT NOT LIMITED TO,  PROCUREMENT OF  SUBSTITUTE  GOODS OR  SERVICES;  LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY OF
// LIABILITY,  WHETHER IN CONTRACT,  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include<time.h>
#include<string.h>
#include"dao_stream.h"
#include"daoValue.h"
#include"daoVmspace.h"

#ifdef WIN32
#include<windows.h>
#include<lmcons.h>
#define putenv _putenv

#  ifndef __GNUC__
#    define fdopen _fdopen
#    define popen  _popen
#    define pclose _pclose
#  endif

#endif

static int DaoFileStream_Read( DaoStream *stream, DString *data, int count )
{
	DaoFileStream *self = (DaoFileStream*) stream;

	DString_Reset( data, 0 );
	if( count >= 0 ){
		DString_Reset( data, count );
		DString_Reset( data, fread( data->chars, 1, count, self->file ) );
	}else if( count == -1 ){
		DaoFile_ReadLine( self->file, data );
	}else{
		DaoFile_ReadAll( self->file, data, 0 );
	}
	return data->size;
}
static int DaoFileStream_Write( DaoStream *stream, const void *data, int count )
{
	DaoFileStream *self = (DaoFileStream*) stream;
	DString bytes = DString_WrapBytes( (char*) data, count );
	DaoFile_WriteString( self->file, & bytes );
	return count;
}
static int DaoFileStream_AtEnd( DaoStream *stream )
{
	DaoFileStream *self = (DaoFileStream*) stream;
	return feof( self->file );
}
static void DaoFileStream_Flush( DaoStream *stream )
{
	DaoFileStream *self = (DaoFileStream*) stream;
	fflush( self->file );
}


static int DaoStringStream_Read( DaoStream *stream, DString *data, int count )
{
	DaoStringStream *self = (DaoStringStream*) stream;

	DString_Reset( data, 0 );
	if( count >= 0 ){
		DString_SubString( self->base.buffer, data, self->offset, count );
		self->offset += data->size;
	}else if( count == -1 ){
		int delim = '\n';
		daoint pos = DString_FindChar( self->base.buffer, delim, self->offset );
		if( self->offset == 0 && (pos == DAO_NULLPOS || pos == self->base.buffer->size-1) ){
			DString_Append( data, self->base.buffer );
			self->offset = self->base.buffer->size;
		}else if( pos == DAO_NULLPOS ){
			DString_SubString( self->base.buffer, data, pos, -1 );
			self->offset = self->base.buffer->size;
		}else{
			DString_SubString( self->base.buffer, data, pos, pos - self->offset + 1 );
			self->offset = pos + 1;
		}
		if( self->base.mode & DAO_STREAM_AUTOCONV ) DString_ToUTF8( data );
		return self->offset < self->base.buffer->size;
	}else{
		if( self->offset == 0 ){ 
			DString_Assign( data, self->base.buffer );
		}else{
			DString_SubString( self->base.buffer, data, self->offset, -1 );
		}    
		self->offset += data->size;
	}
	return data->size;
}
static int DaoStringStream_Write( DaoStream *stream, const void *data, int count )
{
	DaoStringStream *self = (DaoStringStream*) stream;
	DString_AppendBytes( self->base.buffer, (char*) data, count );
	return count;
}
static int DaoStringStream_AtEnd( DaoStream *stream )
{
	DaoStringStream *self = (DaoStringStream*) stream;
	return self->offset >= self->base.buffer->size;
}



DaoFileStream* DaoFileStream_NewByType( DaoType *type )
{
	DaoFileStream *self = (DaoFileStream*) dao_calloc( 1, sizeof(DaoFileStream) );
	DaoCstruct_Init( (DaoCstruct*) self, type );
	self->base.Read = NULL;
	self->base.Write = NULL;
	self->base.AtEnd = NULL;
	self->base.Flush = NULL;
	self->base.SetColor = NULL;
	return self;
}

DaoFileStream* DaoFileStream_New()
{
	return DaoFileStream_NewByType( dao_type_file_stream );
}
void DaoFileStream_Delete( DaoFileStream *self )
{
	DaoFileStream_Close( self );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
void DaoFileStream_Close( DaoFileStream *self )
{
	self->base.mode &= ~(DAO_STREAM_WRITABLE | DAO_STREAM_READABLE);
	self->base.Read = NULL;
	self->base.Write = NULL;
	self->base.AtEnd = NULL;
	self->base.Flush = NULL;
	self->base.SetColor = NULL;
	if( self->file ){
		fflush( self->file );
		fclose( self->file );
		self->file = NULL;
	}
}
void DaoFileStream_InitCallbacks( DaoFileStream *self )
{
	self->base.AtEnd = DaoFileStream_AtEnd;
	if( self->base.mode & DAO_STREAM_READABLE ) self->base.Read = DaoFileStream_Read;
	if( self->base.mode & DAO_STREAM_WRITABLE ){
		self->base.Write = DaoFileStream_Write;
		self->base.Flush = DaoFileStream_Flush;
	}
}

DaoPipeStream* DaoPipeStream_New()
{
	return DaoFileStream_NewByType( dao_type_pipe_stream );
}
int DaoPipeStream_Close( DaoPipeStream *self );
void DaoPipeStream_Delete( DaoPipeStream *self )
{
	DaoPipeStream_Close( self );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
int DaoPipeStream_Close( DaoPipeStream *self )
{
	int ret = 0;
	self->base.mode &= ~(DAO_STREAM_WRITABLE | DAO_STREAM_READABLE);
	self->base.Read = NULL;
	self->base.Write = NULL;
	self->base.AtEnd = NULL;
	self->base.Flush = NULL;
	self->base.SetColor = NULL;
	if( self->file ){
		fflush( self->file );
		ret = pclose( self->file );
		self->file = NULL;
	}
	return ret;
}



DaoStringStream* DaoStringStream_New()
{
	DaoStringStream *self = (DaoStringStream*) dao_calloc( 1, sizeof(DaoStringStream) );
	DaoCstruct_Init( (DaoCstruct*) self, dao_type_string_stream );
	self->base.buffer = DString_New();
	self->base.Read = DaoStringStream_Read;
	self->base.Write = DaoStringStream_Write;
	self->base.AtEnd = DaoStringStream_AtEnd;
	self->base.Flush = NULL;
	self->base.SetColor = NULL;
	return self;
}
void DaoStringStream_Delete( DaoStringStream *self )
{
	DString_Delete( self->base.buffer );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}


FILE* DaoStream_GetFile( DaoStream *self )
{
	if( DaoType_ChildOf( self->ctype, dao_type_file_stream ) ){
		DaoFileStream *stream = (DaoFileStream*) self;
		return stream->file;
	}else if( self->Write == DaoStdStream_WriteStdout || self->Write == DaoStream_WriteStderr ){
		DaoStdStream *stream = (DaoStdStream*) self;
		return DaoStream_GetFile( stream->redirect );
	}else if( self->Write == DaoStream_WriteStdout ){
		return stdout;
	}else if( self->Write == DaoStream_WriteStderr ){
		return stderr;
	}
	return NULL;
}
DaoStream* DaoProcess_PutFile( DaoProcess *self, FILE *file )
{
	DaoFileStream *stream = DaoFileStream_New();
	stream->file = file;
	stream->base.mode |= DAO_STREAM_WRITABLE | DAO_STREAM_READABLE;
	DaoFileStream_InitCallbacks( stream );
	DaoProcess_PutValue( self, (DaoValue*) stream );
	return (DaoStream*) stream;
}
DaoStream* DaoProcess_NewStream( DaoProcess *self, FILE *file )
{
	DaoFileStream *stream = DaoFileStream_New();
	stream->file = file;
	stream->base.mode |= DAO_STREAM_WRITABLE | DAO_STREAM_READABLE;
	DaoFileStream_InitCallbacks( stream );
	DaoProcess_CacheValue( self, (DaoValue*) stream );
	return (DaoStream*) stream;
}


/*
// Special relative paths:
// 1. ::path, path relative to the current source code file;
// 2. :path, path relative to the current working directory;
*/
static void DaoIO_MakePath( DaoProcess *proc, DString *path )
{
	if( path->size ==0 ) return;
	if( path->chars[0] != ':' ) return;
	if( path->chars[1] == ':' ){
		DString_Erase( path, 0, 2 );
		DString_MakePath( proc->activeNamespace->path, path );
		return;
	}
	DString_Erase( path, 0, 1 );
	DString_MakePath( proc->vmSpace->pathWorking, path );
}
static FILE* DaoIO_OpenFile( DaoProcess *proc, DString *name, const char *mode, int silent )
{
	DString *fname = DString_Copy( name );
	char buf[200];
	FILE *fin;

	DaoIO_MakePath( proc, fname );
	fin = Dao_OpenFile( fname->chars, mode );
	DString_Delete( fname );
	if( fin == NULL && silent == 0 ){
		snprintf( buf, sizeof(buf), "error opening file: %s", DString_GetData( name ) );
		DaoProcess_RaiseError( proc, NULL, buf );
		return NULL;
	}
	return fin;
}
static void DaoIO_Open( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoFileStream *self = NULL;
	char *mode;
	self = DaoFileStream_New();
	DaoProcess_PutValue( proc, (DaoValue*)self );
	if( N == 0 ){
		self->file = tmpfile();
		self->base.Read = DaoFileStream_Read;
		self->base.Write = DaoFileStream_Write;
		self->base.AtEnd = DaoFileStream_AtEnd;
		self->base.Flush = DaoFileStream_Flush;
		self->base.mode |= DAO_STREAM_WRITABLE | DAO_STREAM_READABLE;
		if( self->file <= 0 ){
			DaoProcess_RaiseError( proc, NULL, "failed to create temporary file" );
			return;
		}
	}else{
		mode = DString_GetData( p[1]->xString.value );
		if( p[0]->type == DAO_INTEGER ){
			self->file = fdopen( p[0]->xInteger.value, mode );
			if( self->file == NULL ){
				DaoProcess_RaiseError( proc, NULL, "failed to open file descriptor" );
				return;
			}
		}else{
			self->file = DaoIO_OpenFile( proc, p[0]->xString.value, mode, 0 );
		}
		if( strstr( mode, "+" ) ){
			self->base.mode |= DAO_STREAM_WRITABLE | DAO_STREAM_READABLE;
		}else{
			if( strstr( mode, "r" ) ){
				self->base.mode |= DAO_STREAM_READABLE;
			}
			if( strstr( mode, "w" ) || strstr( mode, "a" ) ){
				self->base.mode |= DAO_STREAM_WRITABLE;
			}
		}
		DaoFileStream_InitCallbacks( self );
	}
}

static void DaoIO_Seek( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoFileStream *self = (DaoFileStream*) p[0];
	daoint pos = p[1]->xInteger.value;
	int options[] = { SEEK_SET, SEEK_CUR, SEEK_END };
	int where = options[ p[2]->xEnum.value ];
	if( self->file == NULL ) return;
	fseek( self->file, pos, where );
}
static void DaoIO_Tell( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoFileStream *self = (DaoFileStream*) p[0];
	dao_integer *num = DaoProcess_PutInteger( proc, 0 );
	if( self->file == NULL ) return;
	*num = ftell( self->file );
}
static void DaoIO_FileNO( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoFileStream *self = (DaoFileStream*) p[0];
	dao_integer *num = DaoProcess_PutInteger( proc, 0 );
	if( self->file == NULL ) return;
	*num = fileno( self->file );
}
static void DaoIO_Close( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoFileStream *self = (DaoFileStream*) p[0];
	DaoFileStream_Close( self );
}

static void PIPE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipeStream *stream = NULL;
	DString *fname = p[0]->xString.value;
	char *mode;

	stream = DaoPipeStream_New();
	DaoProcess_PutValue( proc, (DaoValue*)stream );
	if( DString_Size( fname ) == 0 ){
		DaoProcess_RaiseError( proc, "Param", "empty command line" );
		return;
	}
	mode = DString_GetData( p[1]->xString.value );
	stream->file = popen( DString_GetData( fname ), mode );
	if( stream->file == NULL ){
		DaoProcess_RaiseError( proc, "System", "error opening pipe" );
		return;
	}
	if( strstr( mode, "+" ) ){
		stream->base.mode |= DAO_STREAM_WRITABLE | DAO_STREAM_READABLE;
	}else{
		if( strstr( mode, "r" ) ) stream->base.mode |= DAO_STREAM_READABLE;
		if( strstr( mode, "w" ) || strstr( mode, "a" ) ) stream->base.mode |= DAO_STREAM_WRITABLE;
	}
	DaoFileStream_InitCallbacks( stream );
}
static void PIPE_Close( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipeStream *stream = (DaoPipeStream*) p[0];
	if ( stream->file ){
		DaoProcess_PutInteger( proc, DaoPipeStream_Close( stream ) );
	} else {
		DaoProcess_RaiseError( proc, "Param", "open pipe stream required" );
	}
}
static void PIPE_FileNO( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoFileStream *self = (DaoPipeStream*) p[0];
	dao_integer *num = DaoProcess_PutInteger( proc, 0 );
	if( self->file == NULL ) return;
	*num = fileno( self->file );
}


static void DaoIOS_Open( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStringStream *self = DaoStringStream_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void DaoIOS_Seek( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStringStream *self = (DaoStringStream*) p[0];
	daoint pos = p[1]->xInteger.value;
	int options[] = { SEEK_SET, SEEK_CUR, SEEK_END };
	int where = options[ p[2]->xEnum.value ];
	switch( where ){
	case SEEK_SET : self->offset  = pos; break;
	case SEEK_CUR : self->offset += pos; break;
	case SEEK_END : self->offset = self->base.buffer->size - pos; break;
	}
	if( self->offset < 0 ) self->offset = 0;
}
static void DaoIOS_Tell( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStringStream *self = (DaoStringStream*) p[0];
	DaoProcess_PutInteger( proc, self->offset );
}


static DaoFuncItem dao_io_methods[] =
{
	{ DaoIO_Open,      "tmpFile() => FileStream" },
	{ DaoIO_Open,      "open( file: string, mode: string ) => FileStream" },
	{ DaoIO_Open,      "open( fd: int, mode: string ) => FileStream" },

	/*! Spawns sub-process which executes the given shell \a command with redirected standard input or output depending on \a mode.
	 * If \a mode is 'r', returns readable stream of the process output; if \a mode is 'w', returns writable stream of the process
	 * input */
	{ PIPE_New,     "popen( command: string, mode: string ) => io::PipeStream" },
	{ NULL, NULL }
};

static DaoFuncItem fileStreamMeths[] =
{
	{ DaoIO_Open,      "FileStream() => FileStream" },
	{ DaoIO_Open,      "FileStream( file: string, mode: string ) => FileStream" },
	{ DaoIO_Open,      "FileStream( fd: int, mode: string ) => FileStream" },

	{ DaoIO_Close,     "close( self: FileStream )" },

	{ DaoIO_Seek,      "seek( self: FileStream, pos: int, from: enum<begin,current,end> ) => int" },
	{ DaoIO_Tell,      "tell( self: FileStream ) => int" },
	{ DaoIO_FileNO,    ".fd( invar self: FileStream ) => int" },
	{ NULL, NULL }
};


DaoTypeBase DaoFileStream_Typer =
{
	"FileStream", NULL, NULL, (DaoFuncItem*) fileStreamMeths, {0}, {0},
	(FuncPtrDel) DaoFileStream_Delete, NULL
};


static DaoFuncItem pipeMeths[] =
{
	{ PIPE_New,      "PipeStream( file: string, mode: string ) => PipeStream" },
	{ PIPE_FileNO,   ".fd( invar self: PipeStream ) => int" },
	{ PIPE_Close,    "close( self: PipeStream ) => int" },
	{ NULL, NULL }
};

DaoTypeBase DaoPipeStream_Typer =
{
	"PipeStream", NULL, NULL, (DaoFuncItem*) pipeMeths, {0}, {0},
	(FuncPtrDel) DaoPipeStream_Delete, NULL
};


static DaoFuncItem stringStreamMeths[] =
{
	{ DaoIOS_Open,    "StringStream() => StringStream" },
	{ DaoIOS_Seek,    "seek( self: StringStream, pos: int, from: enum<begin,current,end> ) => int" },
	{ DaoIOS_Tell,    "tell( self: StringStream ) => int" },
	{ NULL, NULL }
};


DaoTypeBase DaoStringStream_Typer =
{
	"StringStream", NULL, NULL, (DaoFuncItem*) stringStreamMeths, {0}, {0},
	(FuncPtrDel) DaoStringStream_Delete, NULL
};

static DaoFuncItem ioSeekableMeths[] =
{
	{ NULL,		"seek( self: Seekable, pos: int, from: enum<begin,current,end> ) => int" },
	{ NULL,		"tell( self: Seekable ) => int" },
	{ NULL, NULL }
};

DaoTypeBase ioSeekableTyper =
{
	"Seekable", NULL, NULL, (DaoFuncItem*) ioSeekableMeths, {0}, {0},
	(FuncPtrDel) NULL, NULL
};

DaoType *dao_type_file_stream = NULL;
DaoType *dao_type_pipe_stream = NULL;
DaoType *dao_type_string_stream = NULL;

DAO_DLL int DaoStream_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *ions = DaoVmSpace_GetNamespace( vmSpace, "io" );
	DaoFileStream_Typer.supers[0] = DaoType_GetTyper( dao_type_stream );
	DaoPipeStream_Typer.supers[0] = DaoType_GetTyper( dao_type_stream );
	DaoStringStream_Typer.supers[0] = DaoType_GetTyper( dao_type_stream );
	dao_type_file_stream = DaoNamespace_WrapType( ions, & DaoFileStream_Typer, 0 );
	dao_type_pipe_stream = DaoNamespace_WrapType( ions, & DaoPipeStream_Typer, 0 );
	dao_type_string_stream = DaoNamespace_WrapType( ions, & DaoStringStream_Typer, 0 );
	DaoNamespace_WrapInterface( ions, &ioSeekableTyper );
	DaoNamespace_WrapFunctions( ions, dao_io_methods );
	return 0;
}
