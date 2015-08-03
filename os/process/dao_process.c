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

#include <string.h>

#include"dao.h"
#include"daoValue.h"
#include"daoThread.h"
#include"daoTasklet.h"

#define DAO_HAS_STREAM
#include"dao_api.h"

#ifdef WIN32
#include<io.h>
#include<stdlib.h>
#include<fcntl.h>
#include<windows.h>
#include<wctype.h>
#include<sys/stat.h>

#define pid_t HANDLE
#define fd_t HANDLE
#define IS_PATH_SEP( x ) ( ( x ) == '/' || ( x ) == '\\' || ( x ) == ':' )

HANDLE exec_event; // interrupts WaitFroMultipleObjects() to add new child

typedef wchar_t char_t;

char_t* CharsToTChars( char *chs, size_t len )
{
	char_t *res = (char_t*)dao_malloc( sizeof(char_t)*( len + 1 ) );
	size_t i;
	char *end = chs + len;
	for ( i = 0; i < len; i++ ){
		DCharState st = DString_DecodeChar( chs, end );
		if ( st.type == 0 )
			break;
		if ( st.value <= 0xFFFF )
			res[i] = st.value;
		else {
			st.value -= 0x10000;
			res[i++] = ( st.value >> 10 ) + 0xD800;
			if ( i < len )
				res[i] = ( st.value & 0x3FF ) + 0xDC00;
		}
		chs += st.width;
	}
	res[( i < len )? i : len] = L'\0';
	return res;
}
#endif

#ifdef UNIX
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>
#include<time.h>

#define IS_PATH_SEP( x ) ( ( x ) == '/' )
#define fd_t int

static pid_t fetched_pid = 0; // extra safety measure against kill()'ing of innocents
#endif

#ifdef MAC_OSX
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#else
extern char **environ;
#endif

DMutex proc_mtx;
DMap *proc_map; // PID => Process

typedef struct DaoPipe DaoPipe;
typedef struct DaoOSProcess DaoOSProcess;

struct DaoPipe
{
	int autoclose;
#ifdef WIN32
	HANDLE rpipe, wpipe;
#else
	int fdrpipe, fdwpipe;
	FILE *rpipe, *wpipe;
	DString *fifo;
#endif
};

typedef uchar_t proc_state;

enum {
	Process_Active = 1,
	Process_Finished,
	Process_Signalled
};

struct DaoOSProcess
{
	pid_t id;
	volatile dao_integer pid;
	DString *file, *dir;
	DaoList *args, *env;
	volatile int excode;
	volatile proc_state state;
#ifdef DAO_WITH_THREAD
	DCondVar cvar, *pvar;
#endif
	DaoValue *inpipe, *outpipe, *errpipe;
	int detached;
};

static DaoType *daox_type_process = NULL;
static DaoType *daox_type_pipe = NULL;


static void GetError( char *buf, size_t size )
{
#ifdef WIN32
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ), buf, size, NULL );
#else
	switch ( errno ){
	case EMFILE:
	case ENFILE:		snprintf( buf, size, "Too many files open (EMFILE/ENFILE)" ); break;
	case ENOMEM:		snprintf( buf, size, "Not enough memory (ENOMEM)" ); break;
	case EAGAIN:		snprintf( buf, size, "Too many processes running (EAGAIN)" ); break;
	case ENOENT:		snprintf( buf, size, "Directory path does not exist (ENOENT)" ); break;
	case EACCES:		snprintf( buf, size, "Directory access not permitted (EACCES)"); break;
	case ENAMETOOLONG:	snprintf( buf, size, "Directory path is too long (ENAMETOOLONG)" ); break;
	case ENOTDIR:		snprintf( buf, size, "Specified path is not a directory (ENOTDIR)" ); break;
	case ELOOP:			snprintf( buf, size, "Too many symbolic links encountered while resolving directory path (ELOOP)" ); break;
	case EINVAL:		snprintf( buf, size, "Invalid timeout value (EINVAL)" ); break;
	case EINTR:			snprintf( buf, size, "Interrupted by a signal (EINTR)" ); break;
	case EEXIST:		snprintf( buf, size, "File already exists (EEXIST)" ); break;
	case ENOSPC:		snprintf( buf, size, "Not enough free space in the file system (ENOSPC)" ); break;
	case EROFS:			snprintf( buf, size, "Writing to a read-only file system (EROFS)" ); break;
	default:			snprintf( buf, size, "Unknown system error (%x)", (int)errno );
	}
#endif
}

DaoPipe* DaoPipe_New()
{
	DaoPipe *res = (DaoPipe*)dao_malloc( sizeof(DaoPipe) );
	res->autoclose = 1;
#ifdef WIN32
	res->rpipe = res->wpipe = INVALID_HANDLE_VALUE;
#else
	res->fdrpipe = -1;
	res->fdwpipe = -1;
	res->rpipe = NULL;
	res->wpipe = NULL;
	res->fifo = NULL;
#endif
	return res;
}

enum {
	Pipe_Read,
	Pipe_Write
};

typedef int pipe_end;

void DaoPipe_Close( DaoPipe *self, pipe_end end )
{
#ifdef WIN32
	if ( end == Pipe_Read && self->rpipe != INVALID_HANDLE_VALUE ){
		CloseHandle( self->rpipe );
		self->rpipe = INVALID_HANDLE_VALUE;
	}
	else if ( self->wpipe != INVALID_HANDLE_VALUE ){
		CloseHandle( self->wpipe );
		self->wpipe = INVALID_HANDLE_VALUE;
	}
#else
	if ( end == Pipe_Read ){
		if ( self->rpipe ){
			fclose( self->rpipe );
			self->rpipe = NULL;
		}
		if ( self->fdrpipe >= 0 )
			close( self->fdrpipe );
		self->fdrpipe = -1;
	}
	else {
		if ( self->wpipe ){
			fclose( self->wpipe );
			self->wpipe = NULL;
		}
		if ( self->fdwpipe >= 0 )
			close( self->fdwpipe );
		self->fdwpipe = -1;
	}
	// delete FIFO file if owner
	if ( self->fifo && self->fdrpipe == -1 && self->fdwpipe == -1 ){
		unlink( self->fifo->chars );
		DString_Delete( self->fifo );
		self->fifo = NULL;
	}
#endif
}

void DaoPipe_Delete( DaoPipe *self )
{
	DaoPipe_Close( self, Pipe_Read );
	DaoPipe_Close( self, Pipe_Write );
	dao_free( self );
}

int DaoPipe_Init( DaoPipe *self )
{
#ifdef WIN32
	SECURITY_ATTRIBUTES attrs;
	attrs.nLength = sizeof(attrs);
	attrs.bInheritHandle = TRUE;
	attrs.lpSecurityDescriptor = NULL;
	if ( !CreatePipe( &self->rpipe, &self->wpipe, &attrs, 0 ) )
		return 0;
	// setting handle inheritance
	SetHandleInformation( self->rpipe, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
	SetHandleInformation( self->wpipe, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
	return 1;
#else
	int fd[2];
	if ( pipe( fd ) >= 0 ){
		self->fdrpipe = fd[0];
		self->fdwpipe = fd[1];
		return 1;
	}
	return 0;
#endif
}

int DaoPipe_GetBlocking( DaoPipe *self, pipe_end end )
{
#ifdef WIN32
	DWORD mode = 0;
	HANDLE hpipe = ( end == Pipe_Read )? self->rpipe : self->wpipe;
	GetNamedPipeHandleState( hpipe, &mode, NULL, NULL, NULL, NULL, 0 );
	return !( mode & PIPE_NOWAIT );
#else
	int fdpipe = ( end == Pipe_Read )? self->fdrpipe : self->fdwpipe;
	return !( fcntl( fdpipe, F_GETFL, 0 ) & O_NONBLOCK );
#endif
}

int DaoPipe_SetBlocking( DaoPipe *self, pipe_end end, int blocking )
{
#ifdef WIN32
	DWORD mode = blocking? PIPE_WAIT : PIPE_NOWAIT;
	HANDLE hpipe = ( end == Pipe_Read )? self->rpipe : self->wpipe;
	return SetNamedPipeHandleState( hpipe, &mode, NULL, NULL );
#else
	int fdpipe = ( end == Pipe_Read )? self->fdrpipe : self->fdwpipe;
	int flag = blocking? ( fcntl( fdpipe, F_GETFL, 0 ) & ~( O_NONBLOCK ) ) : ( fcntl( fdpipe, F_GETFL, 0 ) | O_NONBLOCK );
	return fcntl( fdpipe, F_SETFL, flag ) >= 0;
#endif
}

int DaoPipe_IsReadable( DaoPipe *self )
{
#ifdef WIN32
	return self->rpipe != INVALID_HANDLE_VALUE;
#else
	return self->fdrpipe != -1;
#endif
}

int DaoPipe_IsWritable( DaoPipe *self )
{
#ifdef WIN32
	return self->wpipe != INVALID_HANDLE_VALUE;
#else
	return self->fdwpipe != -1;
#endif
}

int DaoPipe_PrepareForRead( DaoPipe *self )
{
	if ( !DaoPipe_IsReadable( self ) )
		return 0;
#ifdef UNIX
	// lazily opened stream, required for feof() check
	if ( !self->rpipe ){
		self->rpipe = fdopen( self->fdrpipe, "r" );
		if ( !self->rpipe )
			return 0;
	}
#endif
	return 1;
}

int DaoPipe_PrepareForWrite( DaoPipe *self )
{
	if ( !DaoPipe_IsWritable( self ) )
		return 0;
#ifdef UNIX
	// lazily opened stream
	if ( !self->wpipe ){
		self->wpipe = fdopen( self->fdwpipe, "w" );
		if ( !self->wpipe )
			return 0;
	}
#endif
	return 1;
}

int DaoPipe_AtEof( DaoPipe *self )
{
	if ( !DaoPipe_PrepareForRead( self ) )
		return 0;
	else {
#ifdef WIN32
		DWORD avail = 0;
		return !PeekNamedPipe( self->rpipe, NULL, 0, NULL, &avail, NULL ) && GetLastError() == 109; // pipe was closed
#else
		return feof( self->rpipe );
#endif
	}
}

int DaoPipe_Read( DaoPipe *self, char *buf, int count )
{
#ifdef WIN32
	DWORD len = 0;
	ReadFile( self->rpipe, buf, count, &len, NULL ); // required for non-blocking read
	return len;
#else
	return fread( buf, 1, count, self->rpipe );
#endif
}

int DaoPipe_Write( DaoPipe *self, char *buf, int count )
{
#ifdef WIN32
	DWORD len = 0;
	WriteFile( self->wpipe, buf, count, &len, NULL ); // required for non-blocking write
	FlushFileBuffers( self->wpipe );
	return len;
#else
	int len = fwrite( buf, 1, count, self->wpipe );
	fflush( self->wpipe );
	return len;
#endif
}

int DaoPipe_WaitRead( DaoPipe *self, dao_float timeout )
{
	if ( !DaoPipe_IsReadable( self ) )
		return 0;
#ifdef UNIX
	if ( 1 ){
		// using select()
		struct timeval tv;
		fd_set set;
		int res;
		FD_ZERO( &set );
		FD_SET( self->fdrpipe, &set );
		if ( timeout >= 0 ){
			tv.tv_sec = timeout;
			tv.tv_usec = ( timeout - tv.tv_sec ) * 1E6;
		}
		res = select( FD_SETSIZE, &set, NULL, NULL, timeout < 0? NULL : &tv );
		return res < 0? -1 : ( res > 0 );
	}
#else
	// looping, peeking, waiting
	if ( 1 ){
		dao_integer total = timeout*1000, elapsed = 0, tick = 15;
		DWORD avail = 0;
		if ( total == 0 )
			return PeekNamedPipe( self->rpipe, NULL, 0, NULL, &avail, NULL )? ( avail > 0 ) : ( GetLastError() == 109? 1 : -1 );
		else
			while ( timeout < 0? 1 : ( elapsed < total ) ){
				if ( !PeekNamedPipe( self->rpipe, NULL, 0, NULL, &avail, NULL ) )
					return ( GetLastError() == 109? 1 : -1 );
				if ( avail )
					return 1;
				Sleep( tick );
				elapsed += tick;
			}
	}
#endif
	return 1;
}

static void DaoPipe_LibClose( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	int mask = ( N < 2 )? 3 : p[1]->xEnum.value + 1;
	if ( mask & 1 )
		DaoPipe_Close( self, Pipe_Read );
	if ( mask & 2 )
		DaoPipe_Close( self, Pipe_Write );
}

static void DaoPipe_LibRead( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	if ( !DaoPipe_IsReadable( self ) )
		DaoProcess_RaiseError( proc, "Pipe", "Reading from a non-readable pipe" );
	else {
		dao_integer count = p[1]->xInteger.value;
		DString *res = DaoProcess_PutChars( proc, "" );
		if ( !DaoPipe_PrepareForRead( self ) ){
			DaoProcess_RaiseError( proc, "Pipe", "Failed to open pipe for reading" );
			return;
		}
		if ( count > 0 ){
			DString_Reserve( res, count );
			DString_Reset( res, DaoPipe_Read( self, res->chars, count ) );
		}
		else if ( count < 0 ){
			char buf[4096];
			int len = 0;
			do {
				len = DaoPipe_Read( self, buf, sizeof(buf) );
				DString_AppendBytes( res, buf, len );
			}
			while ( len );
		}
	}
}

static void DaoPipe_LibWrite( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	if ( !DaoPipe_IsWritable( self ) )
		DaoProcess_RaiseError( proc, "Pipe", "Writing to a non-writable pipe" );
	else {
		if ( !DaoPipe_PrepareForWrite( self ) )
			DaoProcess_RaiseError( proc, "Pipe", "Failed to open pipe for writing" );
		else {
			DString *data = p[1]->xString.value;
			int len = DaoPipe_Write( self, data->chars, data->size );
			if ( len < data->size )
				DaoProcess_RaiseException( proc, "Error::Pipe::Buffer", "Pipe buffer is full, some data were not written",
										   (DaoValue*)DaoInteger_New( data->size - len ) );
		}
	}
}

static void DaoPipe_Check( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	int res = 0;
	switch ( p[1]->xEnum.value ){
	case 0:	res = DaoPipe_IsReadable( self ); break;
	case 1:	res = DaoPipe_IsWritable( self ); break;
	case 2:	res = DaoPipe_IsReadable( self ) || DaoPipe_IsWritable( self ); break;
	case 3: res = DaoPipe_AtEof( self ); break;
	}
	DaoProcess_PutBoolean( proc, res );
}

static void DaoPipe_Id( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	DaoTuple *tup = DaoProcess_PutTuple( proc, 2 );
#ifdef WIN32
	tup->values[0]->xInteger.value = (daoint)self->rpipe;
	tup->values[1]->xInteger.value = (daoint)self->wpipe;
#else
	tup->values[0]->xInteger.value = self->fdrpipe;
	tup->values[1]->xInteger.value = self->fdwpipe;
#endif
}

static void DaoPipe_Wait( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	dao_float timeout = p[1]->xFloat.value;
	int res = DaoPipe_WaitRead( self, timeout );
	if ( res < 0 ){
		char buf[512];
		GetError( buf, sizeof(buf) );
		DaoProcess_RaiseError( proc, "Pipe", buf );
	}
	else
		DaoProcess_PutBoolean( proc, res );
}

static void DaoPipe_GetRMode( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutBoolean( proc, DaoPipe_GetBlocking( self, Pipe_Read ) );
}

static void DaoPipe_SetRMode( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	int res = DaoPipe_SetBlocking( self, Pipe_Read, p[1]->xBoolean.value );
	if ( !res ){
		char buf[512];
		GetError( buf, sizeof(buf) );
		DaoProcess_RaiseError( proc, "Pipe", buf );
	}
}

static void DaoPipe_GetWMode( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutBoolean( proc, DaoPipe_GetBlocking( self, Pipe_Write ) );
}

static void DaoPipe_SetWMode( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	int res = DaoPipe_SetBlocking( self, Pipe_Write, p[1]->xBoolean.value );
	if ( !res ){
		char buf[512];
		GetError( buf, sizeof(buf) );
		DaoProcess_RaiseError( proc, "Pipe", buf );
	}
}

static void DaoPipe_GetClose( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *self = (DaoPipe*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutBoolean( proc, self->autoclose );
}

static DaoFuncItem pipeMeths[] =
{
	/*! ID of the read and write ends of the pipe (file descriptors on Unix, handles on Windows) */
	{ DaoPipe_Id,		".fd(invar self: Pipe) => tuple<read: int, write: int>" },

	/*! Determines if synchronous (blocking) mode is used for reading (`true` by default) */
	{ DaoPipe_GetRMode,	".syncRead(invar self: Pipe) => bool" },
	{ DaoPipe_SetRMode,	".syncRead=(self: Pipe, value: bool)" },

	/*! Determines if synchronous (blocking) mode is used for writing (`true` by default) */
	{ DaoPipe_GetWMode,	".syncWrite(invar self: Pipe) => bool" },
	{ DaoPipe_SetWMode,	".syncWrite=(self: Pipe, value: bool)" },

	/*! Indicates that unused pipe end is automatically closed when the pipe is passed to a process (`true` by default) */
	{ DaoPipe_GetClose,".autoClose(invar self: Pipe) => bool" },

	/*! Checks if the pipe is in the state specified by \a what */
	{ DaoPipe_Check,	"check(self: Pipe, what: enum<readable,writable,open,eof>) => bool" },

	/*! Reads *at most* \a count bytes from the pipe, or all available data if \a count is less then 0 */
	{ DaoPipe_LibRead,	"read(self: Pipe, count = -1) => string" },

	/*! Writes \a data to the input stream of the process. If \a data were not fully written, the method will raise `Pipe::Buffer`
	 * error containing the number of bytes which were not written. */
	{ DaoPipe_LibWrite,	"write(self: Pipe, data: string)" },

	/*! Waits \a timeout seconds for the pipe to become available for reading. If \a timeout is less then 0, waits indefinitely.
	 * Returns `true` if not timeouted
	 *
	 * \warning On Windows, `wait()` uses system clock for short-term sleeping, thus the accuracy of waiting with timeout may
	 * vary depending on the current clock resolution */
	{ DaoPipe_Wait,		"wait(invar self: Pipe, timeout = -1.0) => bool" },

	/*! Closes the specified \a end of the pipe, or both ends if \a end is not given */
	{ DaoPipe_LibClose,	"close(self: Pipe)" },
	{ DaoPipe_LibClose,	"close(self: Pipe, end: enum<read,write,both>)" },
	{ NULL, NULL }
};

/*! Represents pipe to be used for inter-process communication, implements `io::Device` */
static DaoTypeBase pipeTyper = {
	"Pipe", NULL, NULL, pipeMeths, {NULL}, {0},
	(FuncPtrDel)DaoPipe_Delete, NULL
};

#ifdef DAO_WITH_THREAD

// background tasklet tracking child processes until all of them are terminated
void WaitForChildren( void *p )
{
	int done = 0;
	do {
		pid_t id;
#ifdef WIN32
		DWORD code = -1;
		HANDLE waitlist[MAXIMUM_WAIT_OBJECTS];
		DWORD count = 1;
		DNode *pnode;
		int res;
		DMutex_Lock( &proc_mtx );
		ResetEvent( exec_event );
		waitlist[0] = exec_event;
		// get all available children
		for ( pnode = DMap_First( proc_map ); pnode && count < MAXIMUM_WAIT_OBJECTS; pnode = DMap_Next( proc_map, pnode ) )
			waitlist[count++] = ( (DaoOSProcess*)DaoValue_TryGetCdata( pnode->value.pValue ) )->id;
		DMutex_Unlock( &proc_mtx );
		if ( count == 1 )
			break;
		// wait for any child to exit, or for addition of a new child
		res = WaitForMultipleObjects( count, waitlist, FALSE, INFINITE );
		if ( res - WAIT_OBJECT_0 >= count )
			break;
		id = waitlist[res - WAIT_OBJECT_0];
		if ( id == exec_event ) // a child was added, repeat the procedure
			continue;
#else
		int status;
		// wait for any child to exit
		id = fetched_pid = wait( &status );
		if ( id < 0 )
			break;
#endif
		if ( 1 ) {
			char key[sizeof(void*)*2];
			DNode *node;
			memset( &key, 0, sizeof(key) );
			*(pid_t*)( &key ) = id;
			DMutex_Lock( &proc_mtx );
#ifdef UNIX
			fetched_pid = 0;
#endif
			// update Process object
			node = DMap_Find( proc_map, &key );
			if ( node ){
				DaoOSProcess *proc = (DaoOSProcess*)DaoValue_TryGetCdata( node->value.pValue );
#ifdef WIN32
				GetExitCodeProcess( proc->id, &code );
				proc->excode = code;
				proc->state = ( code >= 0 )? Process_Finished : Process_Signalled;
#else
				proc->excode = WEXITSTATUS( status );
				proc->state = WIFEXITED( status )? Process_Finished : Process_Signalled;
#endif
#ifdef WIN32
				CloseHandle( proc->id );
#endif
				proc->pid = 0;
				DCondVar_Signal( &proc->cvar );
				if ( proc->pvar )
					DCondVar_Signal( proc->pvar );
				DMap_EraseNode( proc_map, node );
			}
			done = !proc_map->size;
			DMutex_Unlock( &proc_mtx );
		}
	}
	while ( !done );
}

DaoOSProcess* DaoOSProcess_New()
{
	DaoOSProcess *res = (DaoOSProcess*)dao_malloc( sizeof(DaoOSProcess) );
	res->id = 0;
	res->pid = 0;
	res->file = DString_New();
	res->dir = DString_NewChars( "." );
	res->excode = 1;
	res->args = DaoList_New();
	res->env = NULL;
	DCondVar_Init( &res->cvar );
	res->pvar = NULL;
	res->state = 0;
	res->inpipe = NULL;
	res->outpipe = NULL;
	res->errpipe = NULL;
	res->detached = 0;
	return res;
}

void DaoOSProcess_Delete( DaoOSProcess *self )
{
	if ( self->inpipe )
		DaoGC_DecRC( self->inpipe );
	if ( self->outpipe )
		DaoGC_DecRC( self->outpipe );
	if ( self->errpipe )
		DaoGC_DecRC( self->errpipe );
	DaoList_Delete( self->args );
	if ( self->env )
		DaoList_Delete( self->env );
	DString_Delete( self->file );
	DString_Delete( self->dir );
	DCondVar_Destroy( &self->cvar );
	dao_free( self );
}

proc_state DaoOSProcess_GetState( DaoOSProcess *self )
{
	return self->state;
}

dao_integer DaoOSProcess_GetPID( DaoOSProcess *self )
{
	dao_integer res;
	if ( self->state != Process_Active )
		return self->pid; // no syncing required
	DMutex_Lock( &proc_mtx );
	res = self->pid;
	DMutex_Unlock( &proc_mtx );
	return res;
}

int DaoOSProcess_GetExitCode( DaoOSProcess *self, int *dest )
{
	if ( self->state != Process_Active ){
		*dest = self->excode; // no syncing required
		return 1;
	}
	return 0;
}

void DaoOSProcess_Kill( DaoOSProcess *self, int force )
{
	if ( self->state != Process_Active )
		return;
	DMutex_Lock( &proc_mtx );
	if ( self->state == Process_Active ){
#ifdef WIN32
		TerminateProcess( self->id, -1 );
#else
		if ( self->id != fetched_pid ) // decrease the possibility of killing unrelated process in marginal circumstances
			kill( self->id, force? SIGKILL : SIGTERM );
#endif
	}
	DMutex_Unlock( &proc_mtx );
}

int GetFile( DaoFileStream *stream, fd_t *fd )
{
#ifdef WIN32
	*fd = INVALID_HANDLE_VALUE;
	if ( stream->file )
		DuplicateHandle( GetCurrentProcess(), (HANDLE)_get_osfhandle( _fileno( stream->file ) ), GetCurrentProcess(), fd, 0,
						 TRUE, DUPLICATE_SAME_ACCESS );
	return *fd != INVALID_HANDLE_VALUE;
#else
	*fd = stream->file? dup( fileno( stream->file ) ) : -1;
	return *fd != -1;
#endif
}

int GetStdStream( DaoProcess *proc, DaoValue *value, DaoValue **dest, DaoPipe **pipe, fd_t *fd, pipe_end end )
{
	if ( value ){
		if ( value->xCdata.ctype == daox_type_pipe )
			DaoValue_Copy( value, dest );
		else
			return GetFile( (DaoFileStream*) value, fd );
	}
	else
		DaoGC_Assign( dest, (DaoValue*) DaoProcess_NewCdata( proc, daox_type_pipe, DaoPipe_New(), 1 ) );
	*pipe = (DaoPipe*)DaoValue_TryGetCdata( *dest );
	if ( !value && ( !DaoPipe_Init( *pipe ) || !DaoPipe_SetBlocking( *pipe, end, 0 ) ) )
		return 0;
#ifdef WIN32
	*fd = ( end == Pipe_Read )? ( *pipe )->wpipe : ( *pipe )->rpipe;
#else
	*fd = ( end == Pipe_Read )? ( *pipe )->fdwpipe : ( *pipe )->fdrpipe;
#endif
	return 1;
}

void CloseUnused( DaoPipe *pipe, pipe_end end, fd_t fd )
{
	if ( pipe ){
		if ( pipe->autoclose )
			DaoPipe_Close( pipe, end );
	}
	else
#ifdef WIN32
		CloseHandle( fd );
#else
		close( fd );
#endif
}

DaoValue* DaoOSProcess_Start( DaoOSProcess *self, DaoProcess *proc, DString *cmd, DaoList *args, DString *dir, DaoList *env,
							  DaoValue *inpipe, DaoValue *outpipe, DaoValue *errpipe, int detached )
{
	DaoPipe *pin = NULL, *pout = NULL, *perr = NULL;
	fd_t fin, fout, ferr;
	DaoValue *value = NULL;
	daoint i;
	char *pos = NULL;
	int shell = 0;
	if ( !cmd->size ){
	// shell command
#ifdef WIN32
		DString_SetChars( cmd, "cmd" );
		DaoList_Insert( args, (DaoValue*)DaoString_NewChars( "/C" ), 0 );
#else
		DString_SetChars( cmd, "/bin/sh" );
		DaoList_Insert( args, (DaoValue*)DaoString_NewChars( "-c" ), 0 );
#endif
		shell = 1;
	}
	// copying data to self
	for ( i = cmd->size - 1; i >= 0; i-- )
		if ( IS_PATH_SEP( cmd->chars[i] ) ){
			pos = cmd->chars + i;
			break;
		}
	if ( pos )
		DString_SetChars( self->file, pos + 1 );
	else
		DString_Assign( self->file, cmd );
	for ( i = 0; i < args->value->size; i++ )
		DaoList_Append( self->args, (DaoValue*)DaoString_NewChars( DaoList_GetItem( args, i )->xString.value->chars ) );
	if ( env ){
		self->env = DaoList_New();
		for ( i = 0; i < env->value->size; i++ )
			DaoList_Append( self->env, (DaoValue*)DaoString_NewChars( DaoList_GetItem( env, i )->xString.value->chars ) );
	}
	if ( dir ){
		DString_Assign( self->dir, dir );
		if ( dir->size == 1 && dir->chars[0] == '.' )
			dir = NULL;
	}
	// setting file descriptors
	if ( !GetStdStream( proc, inpipe, &self->inpipe, &pin, &fin, Pipe_Write ) )
		return NULL;
	if ( !GetStdStream( proc, outpipe, &self->outpipe, &pout, &fout, Pipe_Read ) )
		return NULL;
	if ( !GetStdStream( proc, errpipe, &self->errpipe, &perr, &ferr, Pipe_Read ) )
		return NULL;
#ifdef WIN32
	if ( 1 ){
		PROCESS_INFORMATION pinfo;
		STARTUPINFOW sinfo;
		DString *cmdline, *envblock = NULL;
		int res;
		char_t *tcmd, tdir[MAX_PATH + 1];
		memset( &pinfo, 0, sizeof(pinfo) );
		memset( &sinfo, 0, sizeof(sinfo) );
		sinfo.cb = sizeof(sinfo);
		// setting standard IO redirection
		sinfo.hStdError = ferr;
		sinfo.hStdOutput = fout;
		sinfo.hStdInput = fin;
		sinfo.dwFlags = STARTF_USESTDHANDLES;
		// setting command line
		if ( shell )
			cmdline = DString_Copy( cmd );
		else {
			cmdline = DString_NewChars( "\"" );
			DString_Append( cmdline, cmd );
			DString_AppendChar( cmdline, '"' );
		}
		for ( i = 0; i < args->value->size; i++ ){
			DString *arg = DaoList_GetItem( args, i )->xString.value;
			daoint j;
			if ( shell ){
				DString_AppendChar( cmdline, ' ' );
				DString_Append( cmdline, arg );
			}
			else {
				DString_AppendChars( cmdline, " \"" );
				for ( j = 0; j < arg->size; j++ ){
					if ( arg->chars[j] == '"' )
						DString_AppendChar( cmdline, '\\' );
					DString_AppendChar( cmdline, arg->chars[j] );
				}
				DString_AppendChar( cmdline, '"' );
			}
		}
		tcmd = CharsToTChars( cmdline->chars, cmdline->size );
		// setting environment
		if ( env ){
			envblock = DString_New();
			for ( i = 0; i < env->value->size; i++ ){
				DString_Append( envblock, DaoList_GetItem( env, i )->xString.value );
				DString_AppendChar( envblock, '\0' );
			}
			DString_AppendChar( envblock, '\0' );
		}
		// setting working directory
		if ( dir ){
			char_t *rldir = CharsToTChars( dir->chars, dir->size );
			res = GetFullPathNameW( rldir, MAX_PATH + 1, tdir, NULL ) != 0;
			dao_free( rldir );
			if ( !res )
				goto Exit;
		}
		// spawning process
		res = CreateProcessW( NULL, tcmd, NULL, NULL, TRUE, detached? DETACHED_PROCESS : 0, envblock? envblock->chars : NULL,
							  dir? tdir : NULL, &sinfo, &pinfo );
	Exit:
		dao_free( tcmd );
		DString_Delete( cmdline );
		if ( envblock )
			DString_Delete( envblock );
		if ( !res )
			return NULL;
		CloseHandle( pinfo.hThread );
		self->id = pinfo.hProcess;
		self->pid = pinfo.dwProcessId;
	}
#else
	if ( 1 ){
		pid_t id;
		struct stat st;
		char *argv[args->value->size + 2];
		// setting argv
		argv[0] = self->file->chars;
		for ( i = 0; i < args->value->size; i++ )
			argv[i + 1] = DaoList_GetItem( args, i )->xString.value->chars;
		argv[args->value->size + 1] = NULL;
		// checking working directory
		if ( dir && stat( dir->chars, &st ) != 0 )
			return NULL;
		// forking
		id = fork();
		if ( id == 0 ){ // child
			// closing unused pipe ends
			if ( perr )
				DaoPipe_Close( perr, Pipe_Read );
			if ( pout )
				DaoPipe_Close( pout, Pipe_Read );
			if ( pin )
				DaoPipe_Close( pin, Pipe_Write );
			// redirecting standard IO
			dup2( ferr, fileno( stderr ) );
			dup2( fout, fileno( stdout ) );
			dup2( fin, fileno( stdin ) );
			// setting environment
			if ( env ){
				int i;
				char **envs;
				// clearing existing environment
				for ( i = 0; environ[i]; i++ );
				if ( i ){
					envs = dao_malloc( sizeof(char*)*i );
					for ( i = 0; environ[i]; i++ ){
						char *pos = strchr( environ[i], '=' );
						if ( pos ){
							size_t len = pos - environ[i];
							envs[i] = dao_malloc( sizeof(char)*( len + 1 ) );
							strncpy( envs[i], environ[i], len );
							envs[i][len] = '\0';
						}
						else
							envs[i] = NULL;
					}
					for ( i = 0; environ[i]; i++ )
						if ( envs[i] ){
							unsetenv( envs[i] );
							dao_free( envs[i] );
						}
					dao_free( envs );
				}
				for ( i = 0; i < env->value->size; i++ ){
					DString *str = env->value->items.pValue[i]->xString.value;
					size_t len = str->size;
					char *envs = malloc( sizeof(char)*( len + 1 ) );
					if ( envs ){
						memcpy( envs, str->chars, len + 1 );
						putenv( envs );
					}
				}
			}
			// setting working directory
			if ( dir && chdir( dir->chars ) != 0 )
				_exit( 1 );
			// detaching
			if ( detached )
				setsid();
			// executing file
			execvp( self->file->chars, argv );
			_exit( 1 );
			return NULL;
		}
		if ( id < 0 )
			return NULL;
		self->id = id;
		self->pid = id;
	}
#endif
	// closing unused file descriptors
	CloseUnused( perr, Pipe_Write, ferr );
	CloseUnused( pout, Pipe_Write, fout );
	CloseUnused( pin, Pipe_Read, fin );
	self->state = Process_Active;
	self->detached = detached;
	value = (DaoValue*)DaoProcess_NewCdata( proc, daox_type_process, self, 1 );
	// introducing process to the child tracker
	if ( !detached ){
		char key[sizeof(void*)*2];
		memset( &key, 0, sizeof(key) );
		*(pid_t*)( &key ) = self->id;
		DMutex_Lock( &proc_mtx );
		DMap_Insert( proc_map, key, value );
		if ( proc_map->size == 1 )
			DaoCallServer_AddTask( WaitForChildren, NULL, 1 ); // spawning child tracking tasklet
#ifdef WIN32
		SetEvent( exec_event ); // rise and shine, new process has been added
#endif
		DMutex_Unlock( &proc_mtx );
	}
	return value;
}

int DaoOSProcess_WaitExit( DaoOSProcess *self, dao_float timeout )
{
	int res = 1;
	if ( self->detached )
		return -1;
	if ( self->state == Process_Active ){
		DMutex_Lock( &proc_mtx );
		while ( res && self->state == Process_Active ){ // catching spurious wakeups
			if ( timeout < 0 )
				DCondVar_Wait( &self->cvar, &proc_mtx );
			else
				res = DCondVar_TimedWait( &self->cvar, &proc_mtx, timeout ) == 0;
		}
		DMutex_Unlock( &proc_mtx );
	}
	return res;
}

int CheckEnv( DaoList *env, DaoProcess *proc )
{
	if ( env ){
		daoint i;
		for ( i = 0; i < env->value->size; i++ ){
			DString *envs = DaoList_GetItem( env, i )->xString.value;
			if ( DString_FindChar( envs, '=', 0 ) < 0 ){
				DString *msg = DString_NewChars( "Invalid environment variable definition: " );
				DString_Append( msg, envs );
				DaoProcess_RaiseError( proc, "Value", msg->chars );
				DString_Delete( msg );
				return 0;
			}
		}
	}
	return 1;
}

int CheckStream( DaoProcess *proc, DaoValue *value, int read )
{
	if ( value->xCdata.ctype != daox_type_pipe ){
		DaoFileStream *stream = (DaoFileStream*) value;
		if ( !stream->file ){
			DaoProcess_RaiseError( proc, "Value", "Not an open file stream" );
			return 0;
		}
		if ( read && ! DaoStream_IsReadable( (DaoStream*) stream ) ){
			DaoProcess_RaiseError( proc, "Value", "Stream not readable" );
			return 0;
		}
		if ( !read && ! DaoStream_IsWritable( (DaoStream*) stream ) ){
			DaoProcess_RaiseError( proc, "Value", "Stream not writable" );
			return 0;
		}
	}
	return 1;
}

static void OS_Exec( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *res = DaoOSProcess_New();
	DString *path = p[0]->xString.value;
	DaoList *args = &p[1]->xList;
	DString *dir = NULL;
	DaoList *env = NULL;
	DaoValue *inpipe = NULL, *outpipe = NULL, *errpipe = NULL;
	daoint i, detached = 0;
	DString *name = DString_New();
	for ( i = 2; i < N; i++ ){
		DaoTuple *param = &p[i]->xTuple;
		DaoEnum_MakeName( &param->values[0]->xEnum, name );
		if ( strcmp( name->chars, "$dir" ) == 0 )
			dir = param->values[1]->xString.value;
		else if ( strcmp( name->chars, "$environ" ) == 0 )
			env = &param->values[1]->xList;
		else if ( strcmp( name->chars, "$stdin" ) == 0 )
			inpipe = param->values[1];
		else if ( strcmp( name->chars, "$stdout" ) == 0 )
			outpipe = param->values[1];
		else if ( strcmp( name->chars, "$stderr" ) == 0 )
			errpipe = param->values[1];
		else if ( strcmp( name->chars, "$detached" ) == 0 )
			detached = param->values[1]->xBoolean.value;
	}
	DString_Delete( name );
	if ( ( inpipe && !CheckStream( proc, inpipe, 1 ) ) || ( outpipe && !CheckStream( proc, outpipe, 0 ) ) ||
		 ( errpipe && !CheckStream( proc, errpipe, 0 ) ) )
		return;
	if ( CheckEnv( env, proc ) ){
		DaoValue *value = DaoOSProcess_Start( res, proc, path, args, dir, env, inpipe, outpipe, errpipe, detached );
		if ( N == 1 )
			DaoList_Delete( args );
		if ( value )
			DaoProcess_PutValue( proc, value );
		else {
			char buf[512];
			GetError( buf, sizeof(buf) );
			DaoProcess_RaiseError( proc, "Process", buf );
			DaoOSProcess_Delete( res );
		}
	}
}

static void OS_Shell( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *res = DaoOSProcess_New();
	DString *cmd = p[0]->xString.value;
	DString *dir = NULL;
	DaoList *env = NULL;
	daoint i, detached = 0;
	DString *name = DString_New();
	DaoValue *inpipe = NULL, *outpipe = NULL, *errpipe = NULL;
	for ( i = 1; i < N; i++ ){
		DaoTuple *param = &p[i]->xTuple;
		DaoEnum_MakeName( &param->values[0]->xEnum, name );
		if ( strcmp( name->chars, "$dir" ) == 0 )
			dir = param->values[1]->xString.value;
		else if ( strcmp( name->chars, "$environ" ) == 0 )
			env = &param->values[1]->xList;
		else if ( strcmp( name->chars, "$stdin" ) == 0 )
			inpipe = param->values[1];
		else if ( strcmp( name->chars, "$stdout" ) == 0 )
			outpipe = param->values[1];
		else if ( strcmp( name->chars, "$stderr" ) == 0 )
			errpipe = param->values[1];
		else if ( strcmp( name->chars, "$detached" ) == 0 )
			detached = param->values[1]->xBoolean.value;
	}
	DString_Delete( name );
	if ( ( inpipe && !CheckStream( proc, inpipe, 1 ) ) || ( outpipe && !CheckStream( proc, outpipe, 0 ) ) ||
		 ( errpipe && !CheckStream( proc, errpipe, 0 ) ) )
		return;
	if ( CheckEnv( env, proc ) ){
		DaoList *args = DaoList_New();
		DString *str = DString_New();
		DaoValue *value;
		DaoList_Append( args, (DaoValue*)DaoString_NewChars( cmd->chars ) );
		value = DaoOSProcess_Start( res, proc, str, args, dir, env, inpipe, outpipe, errpipe, detached );
		if ( value )
			DaoProcess_PutValue( proc, value );
		else {
			char buf[512];
			GetError( buf, sizeof(buf) );
			DaoProcess_RaiseError( proc, "Process", buf );
			DaoOSProcess_Delete( res );
		}
		DaoList_Delete( args );
		DString_Delete( str );
	}
}

static void DaoOSProcess_Id( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, DaoOSProcess_GetPID( self ) );
}

static void DaoOSProcess_File( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, self->file );
}

static void DaoOSProcess_Dir( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutString( proc, self->dir );
}

static void DaoOSProcess_Args( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	DaoList *lst = DaoProcess_PutList( proc );
	int i;
	for ( i = 0; i < self->args->value->size; i++ )
		DaoList_Append( lst, (DaoValue*)DaoString_NewChars( DaoList_GetItem( self->args, i )->xString.value->chars ) );
}

static void DaoOSProcess_Env( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	if ( !self->env )
		DaoProcess_PutNone( proc );
	else {
		DaoList *lst = DaoProcess_PutList( proc );
		int i;
		for ( i = 0; i < self->env->value->size; i++ )
			DaoList_Append( lst, (DaoValue*)DaoString_NewChars( DaoList_GetItem( self->env, i )->xString.value->chars ) );
	}
}

static void DaoOSProcess_State( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	proc_state state = DaoOSProcess_GetState( self );
	if ( state == Process_Active )
		DaoProcess_PutEnum( proc, self->detached? "detached" : "running" );
	else
		DaoProcess_PutEnum( proc, state == Process_Finished? "finished" : "terminated" );
}

static void DaoOSProcess_Wait( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	dao_float timeout = p[1]->xFloat.value;
	if ( self->detached )
		DaoProcess_RaiseError( proc, "Process", "Attempting to wait for detached process" );
	else
		DaoProcess_PutBoolean( proc, DaoOSProcess_WaitExit( self, timeout ) > 0 );
}

static void DaoOSProcess_LibKill( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	DaoOSProcess_Kill( self, p[1]->xEnum.value == 1 );
}

static void DaoOSProcess_ExitCode( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	int code;
	if ( DaoOSProcess_GetExitCode( self, &code ) )
		DaoProcess_PutInteger( proc, code );
	else
		DaoProcess_PutNone( proc );
}

static void DaoOSProcess_Stdin( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	if ( !self->inpipe )
		DaoProcess_PutNone( proc );
	else
		DaoProcess_PutValue( proc, self->inpipe );
}

static void DaoOSProcess_Stdout( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	if ( !self->outpipe )
		DaoProcess_PutNone( proc );
	else
		DaoProcess_PutValue( proc, self->outpipe );
}

static void DaoOSProcess_Stderr( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	if ( !self->errpipe )
		DaoProcess_PutNone( proc );
	else
		DaoProcess_PutValue( proc, self->errpipe );
}

static DaoFuncItem procMeths[] =
{
	/*! PID (0 if the process exited) */
	{ DaoOSProcess_Id,		".id(invar self: Process) => int" },

	/*! Name of the program being executed */
	{ DaoOSProcess_File,	".program(invar self: Process) => string" },

	/*! Program arguments */
	{ DaoOSProcess_Args,	".arguments(invar self: Process) => list<string>" },

	/*! Working directory (may be relative to the current working directory at the time of process creation) */
	{ DaoOSProcess_Dir,		".workDir(invar self: Process) => string" },

	/*! Process environment in the form of a list of 'name=value' strings (`none` if the process environment is inherited) */
	{ DaoOSProcess_Env,		".environment(invar self: Process) => list<string>|none" },

	/*! Current process status (detached processes always have `$detached` status).
	 *
	 * \note On Windows, a process is deemed terminated if its exit code is less then 0. On Unix, it means that the process exited
	 * because it received a signal */
	{ DaoOSProcess_State,	".status(invar self: Process) => enum<running,finished,terminated,detached>" },

	/*! Process exit code, or `none` if the process is still running or is detached.
	 *
	 * \note On Unix, exit code consists of the lowest 8 bits of exit status when the process exited normally. On Windows, it is
	 * complete 32-bit value returned by the process */
	{ DaoOSProcess_ExitCode,".exitCode(invar self: Process) => int|none" },

	/*! Waits \a timeout seconds for process to exit.  If \a timeout is less then 0, waits indefinitely. Returns `true` if not
	 * timeouted
	 *
	 * \note Cannot be used on detached processes */
	{ DaoOSProcess_Wait,	"wait(invar self: Process, timeout = -1.0) => bool" },

	/*! Attempts to terminate the process the way specified by \a how. On Unix, sends SIGTERM (\a how is `$gracefully`) or
	 * SIGKILL (\a how is `$forcibly`). On Windows, terminates the process forcibly (regardless of \a how) and sets its exit code
	 * to -1 */
	{ DaoOSProcess_LibKill,	"terminate(self: Process, how: enum<gracefully,forcibly> = $forcibly)" },

	/*! Writable input pipe (stdin stream of the process), or `none` if input stream was redirected to a file */
	{ DaoOSProcess_Stdin,	".input(invar self: Process) => Pipe|none" },

	/*! Readable output pipe (stdout stream of the process), or `none` if output stream was redirected to a file */
	{ DaoOSProcess_Stdout,	".output(invar self: Process) => Pipe|none" },

	/*! Readable error pipe (stderr stream of the process), or `none` if error stream was redirected to a file */
	{ DaoOSProcess_Stderr,	".errors(invar self: Process) => Pipe|none" },
	{ NULL, NULL }
};

/*! Represents child process. All child processes are automatically tracked by single background tasklet, which automatically
 * updates `Process` objects associated with the sub-processes */
static DaoTypeBase procTyper = {
	"Process", NULL, NULL, procMeths, {NULL}, {0},
	(FuncPtrDel)DaoOSProcess_Delete, NULL
};

static void OS_Wait( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoList *lst = &p[0]->xList;
	dao_float timeout = p[1]->xFloat.value;
	DaoValue *value = NULL;
	daoint i;
	if ( !lst->value->size ){
		DaoProcess_PutNone( proc );
		return;
	}
	DMutex_Lock( &proc_mtx );
	if ( 1 ){
		daoint count = lst->value->size;
		DaoOSProcess *cprocs[count];
		// check if there already is an exited process
		for ( i = 0; i < count; i++ ){
			DaoValue *item = DaoList_GetItem( lst, i );
			DaoOSProcess *child = (DaoOSProcess*)DaoValue_TryGetCdata( item );
			if ( child->state != Process_Active ){
				value = item;
				break;
			}
			cprocs[i] = child;
		}
		if ( !value && timeout != 0 ){
			int set = 0;
			DCondVar svar;
			DCondVar_Init( &svar );
			// set hook for the process tracker
			for ( i = 0; i < count; i++ )
				if ( !cprocs[i]->detached && !cprocs[i]->pvar ){
					cprocs[i]->pvar = &svar;
					set = 1;
				}
			if ( set ){
				while ( 1 ){ // catching spurious wakeups
					if ( timeout < 0 )
						DCondVar_Wait( &svar, &proc_mtx );
					else if ( DCondVar_TimedWait( &svar, &proc_mtx, timeout ) != 0 )
						break;
					// find which one exited
					for ( i = 0; i < count; i++ )
						if ( cprocs[i]->state != Process_Active ){
							value = DaoList_GetItem( lst, i );
							break;
						}
					if ( value || timeout < 0 )
						break;
				}
				// unset hook
				for ( i = 0; i < count; i++ )
					if ( cprocs[i]->pvar == &svar )
						cprocs[i]->pvar = NULL;
			}
			DCondVar_Destroy( &svar );
		}
	}
	DMutex_Unlock( &proc_mtx );
	if ( value )
		DaoProcess_PutValue( proc, value );
	else
		DaoProcess_PutNone( proc );
}
#endif

static void OS_Select( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoList *lst = &p[0]->xList;
	dao_float timeout = p[1]->xFloat.value;
	DaoValue *value = NULL;
	daoint i = 0;
	int res = 0, found = 0;
	if ( !lst->value->size ){
		DaoProcess_PutNone( proc );
		return;
	}
	if ( 1 ){
		daoint count = lst->value->size;
		DaoPipe *pipes[count];
		for ( i = 0; i < count; i++ ){
			DaoValue *item = DaoList_GetItem( lst, i );
			pipes[i] = (DaoPipe*)DaoValue_TryGetCdata( item );
			if ( DaoPipe_IsReadable( pipes[i] ) )
				found = 1;
		}
#ifdef UNIX
		if ( found ){
			// using select()
			struct timeval tv;
			fd_set set;
			FD_ZERO( &set );
			for ( i = 0; i < count; i++ )
				if ( pipes[i]->fdrpipe != -1 )
					FD_SET( pipes[i]->fdrpipe, &set );
			if ( timeout >= 0 ){
				tv.tv_sec = timeout;
				tv.tv_usec = ( timeout - tv.tv_sec ) * 1E6;
			}
			res = select( FD_SETSIZE, &set, NULL, NULL, timeout < 0? NULL : &tv );
			res =  res < 0? -1 : ( res > 0 );
			if ( res > 0 )
				for ( i = 0; i < count; i++ )
					if ( FD_ISSET( pipes[i]->fdrpipe, &set ) ){
						value = DaoList_GetItem( lst, i );
						break;
					}
		}
#else
		// looping, peeking, waiting
		if ( found ){
			dao_integer total = timeout*1000, elapsed = 0, tick = 15;
			DWORD avail = 0;
			if ( total == 0 ){
				for ( i = 0; i < count; i++ ){
					if ( pipes[i]->rpipe == INVALID_HANDLE_VALUE )
						continue;
					res = PeekNamedPipe( pipes[i]->rpipe, NULL, 0, NULL, &avail, NULL )? ( avail > 0 ) :
																						 ( GetLastError() == 109? 1 : -1 );
					if ( res > 0 )
						value = DaoList_GetItem( lst, i );
					if ( res )
						break;
				}
			}
			else
				while ( total < 0? 1 : ( elapsed < total ) ){
					for ( i = 0; i < count; i++ ){
						if ( pipes[i]->rpipe == INVALID_HANDLE_VALUE )
							continue;
						res = PeekNamedPipe( pipes[i]->rpipe, NULL, 0, NULL, &avail, NULL )? ( avail > 0 ) :
																							 ( GetLastError() == 109? 1 : -1 );
						if ( res > 0 )
							value = DaoList_GetItem( lst, i );
						if ( res )
							goto End;
					}
					Sleep( tick );
					elapsed += tick;
				}
		}
#endif
	}
End:
	if ( value )
		DaoProcess_PutValue( proc, value );
	else if ( res < 0 ){
		char buf[512];
		GetError( buf, sizeof(buf) );
		DaoProcess_RaiseError( proc, "Pipe", buf );
	}
	else
		DaoProcess_PutNone( proc );
}

static void OS_Pipe( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *res = DaoPipe_New();
	res->autoclose = p[0]->xBoolean.value;
	if ( !DaoPipe_Init( res ) ){
		DaoPipe_Delete( res );
		DaoProcess_RaiseError( proc, "Pipe", "Failed to create pipe" );
	}
	else
		DaoProcess_PutCdata( proc, res, daox_type_pipe );
}

static void OS_Mkfifo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *pipe = DaoPipe_New();
	DString *name = p[0]->xString.value;
	DString *mode = p[1]->xString.value;
	int rd = 0, wr = 0;
	int res;
	if ( strcmp( mode->chars, "rw" ) == 0 )
		rd = wr = 1;
	else if ( strcmp( mode->chars, "r" ) == 0 )
		rd = 1;
	else if ( strcmp( mode->chars, "w" ) == 0 )
		wr = 1;
	else {
		DaoProcess_RaiseError( proc, "Param", "Invalid mode" );
		DaoPipe_Delete( pipe );
		return;
	}
#ifdef WIN32
	SECURITY_ATTRIBUTES attrs;
	HANDLE handle;
	DWORD opmode = 0;
	DString *pname = DString_NewChars( "\\\\.\\pipe\\" );
	char_t *tname = NULL;
	attrs.nLength = sizeof(attrs);
	attrs.bInheritHandle = TRUE;
	attrs.lpSecurityDescriptor = NULL;
	DString_Append( pname, name );
	tname = CharsToTChars( pname->chars, pname->size );
	DString_Delete( pname );
	if ( rd )
		opmode |= PIPE_ACCESS_INBOUND;
	if ( wr )
		opmode |= PIPE_ACCESS_OUTBOUND;
	// create named pipe with specific open mode
	handle = CreateNamedPipeW( tname, opmode, 0, PIPE_UNLIMITED_INSTANCES, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, &attrs );
	dao_free( tname );
	res = handle != INVALID_HANDLE_VALUE;
	if ( res ){
		if ( rd )
			pipe->rpipe = handle;
		// duplicate pipe handle to allow closing of separate pipe ends
		if ( rd && wr )
			res = DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(), &pipe->wpipe, 0, TRUE, DUPLICATE_SAME_ACCESS );
		else if ( wr )
			pipe->wpipe = handle;
	}
#else
	if ( rd && wr ){
		DaoProcess_RaiseError( proc, "Param", "Invalid mode" );
		DaoPipe_Delete( pipe );
		return;
	}
	// create FIFO file
	res = mkfifo( name->chars, S_IRWXU|S_IRGRP|S_IXGRP|S_IXOTH ) == 0;
	if ( res ){
		// open for reading or writing
		if ( rd )
			pipe->fdrpipe = open( name->chars, O_RDONLY );
		else
			pipe->fdwpipe = open( name->chars, O_WRONLY );
		pipe->fifo = DString_Copy( name );
		res = pipe->fdrpipe >= 0 || pipe->fdwpipe >= 0;
	}
#endif
	pipe->autoclose = p[3]->xBoolean.value;
	if ( !res ){
		char buf[512];
		GetError( buf, sizeof(buf) );
		DaoProcess_RaiseError( proc, "Pipe", buf );
		DaoPipe_Delete( pipe );
	}
	else
		DaoProcess_PutCdata( proc, pipe, daox_type_pipe );
}

static void OS_Open( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoPipe *pipe = DaoPipe_New();
	DString *name = p[0]->xString.value;
	DString *mode = p[1]->xString.value;
	int rd = 0, wr = 0;
	int res;
	if ( strcmp( mode->chars, "rw" ) == 0 )
		rd = wr = 1;
	else if ( strcmp( mode->chars, "r" ) == 0 )
		rd = 1;
	else if ( strcmp( mode->chars, "w" ) == 0 )
		wr = 1;
	else {
		DaoProcess_RaiseError( proc, "Param", "Invalid mode" );
		DaoPipe_Delete( pipe );
		return;
	}
	pipe->autoclose = p[3]->xBoolean.value;
#ifdef WIN32
	if ( 1 ){
		SECURITY_ATTRIBUTES attrs;
		DWORD sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		DString *pname = DString_NewChars( "\\\\.\\pipe\\" );
		char_t *tname = NULL;
		attrs.nLength = sizeof(attrs);
		attrs.bInheritHandle = TRUE;
		attrs.lpSecurityDescriptor = NULL;
		DString_Append( pname, name );
		tname = CharsToTChars( pname->chars, pname->size );
		DString_Delete( pname );
		// open named pipe for reading and/or writing
		if ( rd )
			pipe->rpipe = CreateFileW( tname, GENERIC_READ, sharing, &attrs, OPEN_EXISTING, 0, NULL );
		if ( wr )
			pipe->wpipe = CreateFileW( tname, GENERIC_WRITE, sharing, &attrs, OPEN_EXISTING, 0, NULL );
		dao_free( tname );
		res = pipe->rpipe != INVALID_HANDLE_VALUE || pipe->wpipe != INVALID_HANDLE_VALUE;
	}
#else
	if ( rd && wr ){
		DaoProcess_RaiseError( proc, "Param", "Invalid mode" );
		DaoPipe_Delete( pipe );
		return;
	}
	if ( 1 ){
		// check if FIFO file
		struct stat st;
		if ( stat( name->chars, &st ) == 0 && !S_ISFIFO( st.st_mode ) ){
			DaoProcess_RaiseError( proc, "Value", "The path does not point to FIFO file" );
			DaoPipe_Delete( pipe );
			return;
		}
	}
	// open file for reading or writing
	if ( rd )
		pipe->fdrpipe = open( name->chars, O_RDONLY );
	else
		pipe->fdwpipe = open( name->chars, O_WRONLY );
	res = pipe->fdrpipe >= 0 || pipe->fdwpipe >= 0;
#endif
	if ( !res ){
		char buf[512];
		GetError( buf, sizeof(buf) );
		DaoProcess_RaiseError( proc, "Pipe", buf );
		DaoPipe_Delete( pipe );
	}
	else
		DaoProcess_PutCdata( proc, pipe, daox_type_pipe );
}

static DaoFuncItem osMeths[] =
{
#ifdef DAO_WITH_THREAD
	/*! Creates new child process executing the file specified by \a path with the \a arguments (if given). \a path may omit the
	 * full path to the file, in that case its resolution is system-dependent. Returns the corresponding `Process` object.
	 *
	 * Additional variadic parameters may specify the following process parameters:
	 * - `dir` -- working directory
	 * - `environ` -- environment (list of 'name=value' items specifying environment variables)
	 * - `stdin`, `stdout`, `stderr` -- pipes or files for redirection of standard input, output and error streams
	 * - `detached` -- if `true`, the process is detached from the calling process (on Unix, it also starts new session and becomes
	 * leader of new process group)
	 *
	 * If pipe for any of the standard streams is not specified, it is automatically created with non-blocking mode set. Unless the
	 * user-specified pipe is created with `autoClose` set to `false`, its unused end is automatically closed.
	 *
	 * \note On Windows, the given \a path and \a arguments are concatenated into command line, where all component are wrapped
	 * in quotes (existing quotes are escaped) and separated by space characters. If such behavior is undesirable for some case,
	 * use `shell()` instead.
	 *
	 * \note The routine will fail with error on Windows if the execution cannot be started (for instance, \a path is not valid);
	 * on Unix, you may need to examine the process status or exit code (which will be set to 1) to detect execution failure.
	 *
	 * \warning On Windows, environment variable strings are assumed to be in local or ASCII encoding */
	{ OS_Exec,	"exec(path: string, invar arguments: list<string>, ...: "
					 "tuple<enum<dir>, string> | tuple<enum<environ>, invar<list<string>>> | "
					 "tuple<enum<stdin,stdout,stderr>, Pipe|io::FileStream|io::PipeStream> | tuple<enum<detached>, bool>) => Process" },

	/*! Similar to `exec()`, but calls system shell ('/bin/sh -c' on Unix, 'cmd /C' on Windows) with the given \a command.
	 * \a command is passed to the shell 'as-is', so you need to ensure that it is properly escaped */
	{ OS_Shell,	"shell(command: string, ...: "
					  "tuple<enum<dir>, string> | tuple<enum<environ>, invar<list<string>>> | "
					  "tuple<enum<stdin,stdout,stderr>, Pipe|io::FileStream|io::PipeStream> | tuple<enum<detached>, bool>) => Process" },

	/*! Waits for one of child processes in \a children to exit for \a timeout seconds (waits indefinitely if \a timeout is less
	 * then 0). Returns the first found exited process, or `none` if timeouted or if \a children is empty
	 *
	 * \warning If the function is called concurrently from multiple threads, it will ignore the processes which are currently
	 * tracked by its other invocations. Detached processes are ignored as well. */
	{ OS_Wait,	"wait(invar children: list<Process>, timeout = -1.0) => Process|none" },
#endif

	/*! Creates new pipe and returns the corresponding `Pipe` object. If \a autoClose is `true`, unused pipe end is automatically
	 * closed when the pipe is passed to `exec()` or `shell()`; setting this parameter to `false` allows to pass single pipe to
	 * multiple processes, in which case you should manually close the unused pipe end (if any) in order to enable EOF check */
	{ OS_Pipe,	"pipe(autoClose = true) => Pipe" },

	/*! Creates named pipe with the specified \a name and access \a mode ('r', 'w' or 'rw'), returns the corresponding `Pipe`
	 * object. For the description of \a autoClose parameter, see `pipe(autoClose = true) => Pipe`.
	 *
	 * On Windows, named pipe is created with name '\\.\pipe\' + \a name, open mode corresponding to \a mode (inbound, outbound or
	 * duplex for 'r', 'w' and 'rw' accordingly) and full sharing. This pipe is removed from the system when no process has
	 * references to it (including the process which created the pipe).
	 *
	 * On Unix, FIFO file is created with the path specified by \a name, and is opened with read or write access (both cannot be
	 * specified). Depending on the system, the routine may block until another process opens this file with the opposite access
	 * type. The file is automatically unlinked from the file system when the `Pipe` object returned by this routine is fully
	 * closed, but it will remain accessible via the existing references to it */
	{ OS_Mkfifo,"pipe(name: string, mode: string, action: enum<create>, autoClose = true) => Pipe" },

	/*! Opens existing named pipe with the specified \a name using the given access \a mode ('r', 'w' or 'rw'), returns the
	 * corresponding `Pipe` object. For the description of \a autoClose parameter, see `pipe(autoClose = true) => Pipe`.
	 *
	 * On Windows, named pipe with name '\\.\pipe\' + \a name is opened. For named pipe created with duplex mode ('rw'),
	 * any of the possible \a mode values are acceptable, pipe created with inbound ('r') or outbound('w') mode can only be opened
	 * with the opposite access type.
	 *
	 * On Unix, FIFO file with path specified by \a name is opened with read or write access (both cannot be specified), the
	 * routine may block (depending on the system) until another process opens this file with the opposite access type */
	{ OS_Open,	"pipe(name: string, mode: string, action: enum<open>, autoClose = true) => Pipe" },

	/*! Waits \a timeout seconds for one of \a pipes to become available for reading. If \a timeout is less then 0, waits
	 * indefinitely. Returns the first found readable pipe, or `none` if timeouted or if \a pipes is empty.
	 *
	 * \warning On Windows, `select()` uses system clock for short-term sleeping, thus the accuracy of waiting with timeout may
	 * vary depending on the current clock resolution. */
	{ OS_Select,"select(invar pipes: list<Pipe>, timeout = -1.0) => Pipe|none" },
	{ NULL, NULL }
};

DAO_DLL int DaoProcess_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *streamns = DaoVmSpace_LinkModule( vmSpace, ns, "stream" );
	DaoNamespace *osns = DaoVmSpace_GetNamespace( vmSpace, "os" );
	DaoNamespace_AddConstValue( ns, "os", (DaoValue*)osns );
	daox_type_pipe = DaoNamespace_WrapType( osns, &pipeTyper, DAO_CDATA, 0 );

#ifdef DAO_WITH_THREAD
	DMutex_Init( &proc_mtx );
	daox_type_process = DaoNamespace_WrapType( osns, &procTyper, DAO_CDATA, 0 );
#else
	DaoStream_WriteChars( vmSpace->errorStream, "WARNING: Module \"os.process\" is incomplete without Dao threading support!\n" );
#endif

	DaoNamespace_WrapFunctions( osns, osMeths );

#ifdef WIN32
	exec_event = CreateEvent( NULL, TRUE, FALSE, NULL );
#endif
	proc_map = DHash_New( DAO_DATA_VOID2, DAO_DATA_VALUE );
	return 0;
}
