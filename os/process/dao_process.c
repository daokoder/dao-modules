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

#include"dao.h"
#include"daoValue.h"
#include"daoThread.h"
#include"daoTasklet.h"

#ifdef WIN32
#include<io.h>
#include<fcntl.h>
#include<windows.h>
#include<wctype.h>
#include<sys/stat.h>

#define pid_t HANDLE
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

static pid_t fetched_pid = 0; // extra safety measure against kill()'ing of innocents
#endif

DMutex proc_mtx;
DMap *proc_map; // PID => Process

typedef struct DaoOSProcess DaoOSProcess;
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
	DCondVar cvar, *pvar;
#ifdef WIN32
	HANDLE rpipe, wpipe;
#else
	int fdrpipe, fdwpipe;
	FILE *rpipe, *wpipe;
#endif
};

static DaoType *daox_type_process = NULL;

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
	res->dir = DString_New();
	res->excode = 1;
	res->args = DaoList_New();
	res->env = NULL;
	DCondVar_Init( &res->cvar );
	res->pvar = NULL;
	res->state = 0;
#ifdef WIN32
	res->rpipe = res->wpipe = INVALID_HANDLE_VALUE;
#else
	res->fdrpipe = -1;
	res->fdwpipe = -1;
	res->rpipe = NULL;
	res->wpipe = NULL;
#endif
	return res;
}

void DaoOSProcess_Close( DaoOSProcess *self, int mask )
{
#ifdef WIN32
	if ( ( mask & 1 ) && self->rpipe != INVALID_HANDLE_VALUE ){
		CloseHandle( self->rpipe );
		self->rpipe = INVALID_HANDLE_VALUE;
	}
	if ( ( mask & 2 ) && self->wpipe != INVALID_HANDLE_VALUE ){
		CloseHandle( self->wpipe );
		self->wpipe = INVALID_HANDLE_VALUE;
	}
#else
	if ( mask & 1 ){
		if ( self->rpipe ){
			fclose( self->rpipe );
			self->rpipe = NULL;
		}
		if ( self->fdrpipe >= 0 )
			close( self->fdrpipe );
		self->fdrpipe = -1;
	}
	if ( mask & 2 ){
		if ( self->wpipe ){
			fclose( self->wpipe );
			self->wpipe = NULL;
		}
		if ( self->fdwpipe >= 0 )
			close( self->fdwpipe );
		self->fdwpipe = -1;
	}
#endif
}

void DaoOSProcess_Delete( DaoOSProcess *self )
{
	DaoOSProcess_Close( self, 3 );
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

int DaoOSProcess_IsReadable( DaoOSProcess *self )
{
#ifdef WIN32
	return self->rpipe != INVALID_HANDLE_VALUE;
#else
	return self->fdrpipe != -1;
#endif
}

int DaoOSProcess_IsWritable( DaoOSProcess *self )
{
#ifdef WIN32
	return self->wpipe != INVALID_HANDLE_VALUE;
#else
	return self->fdwpipe != -1;
#endif
}

int DaoOSProcess_PrepareForRead( DaoOSProcess *self )
{
	if ( !DaoOSProcess_IsReadable( self ) )
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

int DaoOSProcess_PrepareForWrite( DaoOSProcess *self )
{
	if ( !DaoOSProcess_IsWritable( self ) )
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

int DaoOSProcess_AtEof( DaoOSProcess *self )
{
	if ( !DaoOSProcess_PrepareForRead( self ) )
		return 0;
	else {
#ifdef WIN32
		if ( self->state != Process_Active ){
			DWORD avail = 0;
			return ( PeekNamedPipe( self->rpipe, NULL, 0, NULL, &avail, NULL ) && !avail ) || GetLastError() == 109; // end of pipe
		}
		return 0;
#else
		return feof( self->rpipe );
#endif
	}
}

int DaoOSProcess_Read( DaoOSProcess *self, char *buf, int count )
{
#ifdef WIN32
	DWORD len = 0;
	ReadFile( self->rpipe, buf, count, &len, NULL ); // required for non-blocking read
	return len;
#else
	return fread( buf, 1, count, self->rpipe );
#endif
}

int DaoOSProcess_Write( DaoOSProcess *self, char *buf, int count )
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

DaoValue* DaoOSProcess_Start( DaoOSProcess *self, DaoProcess *proc, DString *cmd, DaoList *args, DString *dir, DaoList *env )
{
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
	DString_Assign( self->dir, dir );
	if ( dir->size == 1 && dir->chars[0] == '.' )
		dir = NULL;
#ifdef WIN32
	if ( 1 ){
		SECURITY_ATTRIBUTES attrs;
		PROCESS_INFORMATION pinfo;
		STARTUPINFOW sinfo;
		HANDLE crpipe, cwpipe;
		DWORD mode = PIPE_NOWAIT;
		DString *cmdline, *envblock = NULL;
		int res;
		char_t *tcmd, tdir[MAX_PATH + 1];
		attrs.nLength = sizeof(attrs);
		attrs.bInheritHandle = TRUE;
		attrs.lpSecurityDescriptor = NULL;
		// creating pipes
		res = CreatePipe( &self->rpipe, &cwpipe, &attrs, 0 );
		if ( res ){
			res = CreatePipe( &crpipe, &self->wpipe, &attrs, 0 );
			if ( !res )
				CloseHandle( crpipe );
		}
		if ( !res )
			return NULL;
		// setting handle inheritance
		SetHandleInformation( self->rpipe, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
		SetHandleInformation( self->wpipe, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
		// setting non-blocking mode
		SetNamedPipeHandleState( self->rpipe, &mode, NULL, NULL );
		SetNamedPipeHandleState( self->wpipe, &mode, NULL, NULL );
		memset( &pinfo, 0, sizeof(pinfo) );
		memset( &sinfo, 0, sizeof(sinfo) );
		sinfo.cb = sizeof(sinfo);
		// setting standard IO redirection
		sinfo.hStdError = cwpipe;
		sinfo.hStdOutput = cwpipe;
		sinfo.hStdInput = crpipe;
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
		res = CreateProcessW( NULL, tcmd, NULL, NULL, TRUE, 0, envblock? envblock->chars : NULL, dir? tdir : NULL, &sinfo, &pinfo );
	Exit:
		dao_free( tcmd );
		DString_Delete( cmdline );
		if ( envblock )
			DString_Delete( envblock );
		CloseHandle( crpipe );
		CloseHandle( cwpipe );
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
		int fdr[2], fdw[2], res;
		// setting argv
		argv[0] = self->file->chars;
		for ( i = 0; i < args->value->size; i++ )
			argv[i + 1] = DaoList_GetItem( args, i )->xString.value->chars;
		argv[args->value->size + 1] = NULL;
		// creating pipes
		res = pipe( fdr ) >= 0;
		if ( res ){
			res = pipe( fdw ) >= 0;
			if ( !res ){
				close( fdr[0] );
				close( fdr[1] );
			}
		}
		if ( !res )
			return NULL;
		// setting non-blocking mode
		fcntl( fdr[0], F_SETFL, fcntl( fdr[0], F_GETFL, 0 ) | O_NONBLOCK );
		fcntl( fdw[1], F_SETFL, fcntl( fdw[1], F_GETFL, 0 ) | O_NONBLOCK );
		// checking working directory
		if ( dir && stat( dir->chars, &st ) != 0 )
			return NULL;
		// forking
		id = fork();
		if ( id == 0 ){ // child
			int fdout = fileno( stdout );
			int fdin = fileno( stdin );
			int fderr = fileno( stderr );
			close( fdr[0] );
			close( fdw[1] );
			// redirecting standard IO
			dup2( fdr[1], fdout );
			dup2( fdr[1], fderr );
			dup2( fdw[0], fdin );
			// setting environment
			if ( env ){
				int i;
				clearenv();
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
			// executing file
			execvp( self->file->chars, argv );
			_exit( 1 );
			return NULL;
		}
		if ( id < 0 )
			return NULL;
		close( fdr[1] );
		close( fdw[0] );
		self->fdrpipe = fdr[0];
		self->fdwpipe = fdw[1];
		self->id = id;
		self->pid = id;
	}
#endif
	self->state = Process_Active;
	// introducing process to the child tracker
	if ( 1 ){
		char key[sizeof(void*)*2];
		memset( &key, 0, sizeof(key) );
		*(pid_t*)( &key ) = self->id;
		value = (DaoValue*)DaoProcess_NewCdata( proc, daox_type_process, self, 1 );
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

int DaoOSProcess_WaitRead( DaoOSProcess *self, dao_float timeout )
{
	if ( self->state == Process_Active ){
		if ( !DaoOSProcess_PrepareForRead( self ) )
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
				while ( total < 0? 1 : ( elapsed < total ) ){
					if ( !PeekNamedPipe( self->rpipe, NULL, 0, NULL, &avail, NULL ) )
						return ( GetLastError() == 109? 1 : -1 );
					if ( avail )
						return 1;
					Sleep( tick );
					elapsed += tick;
				}
		}
#endif
		return 0;
	}
	return 1;
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

void GetError( char *buf, size_t size )
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
	default:			snprintf( buf, size, "Unknown system error (%x)", (int)errno );
	}
#endif
}

static void OS_Exec( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *res = DaoOSProcess_New();
	DString *path = p[0]->xString.value;
	DaoList *args = &p[1]->xList;
	DString *dir = p[2]->xString.value;
	DaoList *env = ( p[3]->type == DAO_LIST )? &p[3]->xList : NULL;
	if ( CheckEnv( env, proc ) ){
		DaoValue *value = DaoOSProcess_Start( res, proc, path, args, dir, env );
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
	DString *dir = p[1]->xString.value;
	DaoList *env = ( p[2]->type == DAO_LIST )? &p[2]->xList : NULL;
	if ( CheckEnv( env, proc ) ){
		DaoList *args = DaoList_New();
		DString *str = DString_New();
		DaoValue *value;
		DaoList_Append( args, (DaoValue*)DaoString_NewChars( cmd->chars ) );
		value = DaoOSProcess_Start( res, proc, str, args, dir, env );
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
	DaoProcess_PutEnum( proc, state == Process_Active? "running" : ( state == Process_Finished? "finished" : "terminated" ) );
}

static void DaoOSProcess_Wait( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	dao_float timeout = p[2]->xFloat.value;
	daoint op = p[1]->xEnum.value;
	int res = 1;
	if ( op == 0 )
		res = DaoOSProcess_WaitExit( self, timeout );
	else {
		if ( !DaoOSProcess_IsReadable( self ) ){
			DaoProcess_RaiseError( proc, "Process", "Waiting for output on a non-readable process" );
			return;
		}
		res = DaoOSProcess_WaitRead( self, timeout );
		if ( res < 0 ){
			char buf[512];
			GetError( buf, sizeof(buf) );
			DaoProcess_RaiseError( proc, "Process", buf );
			return;
		}
	}
	DaoProcess_PutBoolean( proc, res );
}

static void DaoOSProcess_LibKill( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	DaoOSProcess_Kill( self, p[1]->xEnum.value == 1 );
}

static void DaoOSProcess_LibClose( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	int mask = ( N < 2 )? 2 : p[1]->xEnum.value + 1;
	DaoOSProcess_Close( self, mask );
}

static void DaoOSProcess_LibRead( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	if ( !DaoOSProcess_IsReadable( self ) )
		DaoProcess_RaiseError( proc, "Process", "Reading from a non-readable process" );
	else {
		dao_integer count = p[1]->xInteger.value;
		DString *res = DaoProcess_PutChars( proc, "" );
		if ( !DaoOSProcess_PrepareForRead( self ) ){
			DaoProcess_RaiseError( proc, "Process", "Failed to open process for reading" );
			return;
		}
		if ( count > 0 ){
			DString_Reserve( res, count );
			DString_Reset( res, DaoOSProcess_Read( self, res->chars, count ) );
		}
		else if ( count < 0 ){
			char buf[4096];
			int len = 0;
			do {
				len = DaoOSProcess_Read( self, buf, sizeof(buf) );
				DString_AppendBytes( res, buf, len );
			}
			while ( len );
		}
	}
}

static void DaoOSProcess_LibWrite( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	if ( !DaoOSProcess_IsWritable( self ) )
		DaoProcess_RaiseError( proc, "Process", "Writing to a non-writable process" );
	else {
		if ( !DaoOSProcess_PrepareForWrite( self ) )
			DaoProcess_RaiseError( proc, "Process", "Failed to open process for writing" );
		else {
			DString *data = p[1]->xString.value;
			int len = DaoOSProcess_Write( self, data->chars, data->size );
			if ( len < data->size )
				DaoProcess_RaiseException( proc, "Error::Buffer", "Process input buffer is full, some data were not written",
										   (DaoValue*)DaoInteger_New( data->size - len ) );
		}
	}
}

static void DaoOSProcess_Check( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoOSProcess *self = (DaoOSProcess*)DaoValue_TryGetCdata( p[0] );
	int res = 0;
	switch ( p[1]->xEnum.value ){
	case 0:	res = DaoOSProcess_IsReadable( self ); break;
	case 1:	res = DaoOSProcess_IsWritable( self ); break;
	case 2:	res = DaoOSProcess_IsReadable( self ) || DaoOSProcess_IsWritable( self ); break;
	case 3: res = DaoOSProcess_AtEof( self ); break;
	}
	DaoProcess_PutBoolean( proc, res );
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
				if ( !cprocs[i]->pvar ){
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

static void OS_Select( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoList *lst = &p[0]->xList;
	dao_float timeout = p[1]->xFloat.value;
	DaoValue *value = NULL;
	daoint i;
	int res = 0;
	if ( !lst->value->size ){
		DaoProcess_PutNone( proc );
		return;
	}
	if ( 1 ){
		daoint count = lst->value->size;
		DaoOSProcess *cprocs[count];
		for ( i = 0; i < count; i++ ){
			DaoValue *item = DaoList_GetItem( lst, i );
			DaoOSProcess *child = (DaoOSProcess*)DaoValue_TryGetCdata( item );
			if ( child->state != Process_Active ){
				value = item;
				goto End;
			}
			cprocs[i] = child;
		}
#ifdef UNIX
		if ( 1 ){
			// using select()
			struct timeval tv;
			fd_set set;
			FD_ZERO( &set );
			for ( i = 0; i < count; i++ )
				FD_SET( cprocs[i]->fdrpipe, &set );
			if ( timeout >= 0 ){
				tv.tv_sec = timeout;
				tv.tv_usec = ( timeout - tv.tv_sec ) * 1E6;
			}
			res = select( FD_SETSIZE, &set, NULL, NULL, timeout < 0? NULL : &tv );
			res =  res < 0? -1 : ( res > 0 );
			if ( res > 0 )
				for ( i = 0; i < count; i++ )
					if ( FD_ISSET( cprocs[i]->fdrpipe, &set ) ){
						value = DaoList_GetItem( lst, i );
						break;
					}
		}
#else
		// looping, peeking, waiting
		if ( 1 ){
			dao_integer total = timeout*1000, elapsed = 0, tick = 15;
			DWORD avail = 0;
			if ( total == 0 ){
				for ( i = 0; i < count; i++ ){
					res = PeekNamedPipe( cprocs[i]->rpipe, NULL, 0, NULL, &avail, NULL )? ( avail > 0 ) :
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
						res = PeekNamedPipe( cprocs[i]->rpipe, NULL, 0, NULL, &avail, NULL )? ( avail > 0 ) :
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
		DaoProcess_RaiseError( proc, "Process", buf );
	}
	else
		DaoProcess_PutNone( proc );
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
	{ DaoOSProcess_Dir,		".workdir(invar self: Process) => string" },

	/*! Process environment in the form of a list of 'name=value' strings (`none` if the process environment is inherited) */
	{ DaoOSProcess_Env,		".environment(invar self: Process) => list<string>|none" },

	/*! Current process status.
	 *
	 * \note On Windows, a process is deemed terminated if its exit code is less then 0. On Unix, it means that the process exited
	 * because it received a signal */
	{ DaoOSProcess_State,	".status(invar self: Process) => enum<running,finished,terminated>" },

	/*! Process exit code, or `none` if the process is still running.
	 *
	 * \note On Unix, exit code consists of the lowest 8 bits of exit status when the process exited normally. On Windows, it is
	 * complete 32-bit value returned by the process */
	{ DaoOSProcess_ExitCode,".exitcode(invar self: Process) => int|none" },

	/*! Waits \a timeout seconds for process to exit or become available for reading (output buffer is not empty) depending on
	 * \a what. If \a timeout is less then 0, waits indefinitely. Returns `true` if not timeouted.
	 *
	 * \note `wait($output)` will return immediately if the process exited.
	 *
	 * \warning On Windows, `wait($output)` uses system clock for short-term sleeping, thus the accuracy of waiting with timeout
	 * may vary depending on the current clock resolution. */
	{ DaoOSProcess_Wait,	"wait(invar self: Process, what: enum<exit,output>, timeout = -1.0) => bool" },

	/*! Attempts to terminate the process the way specified by \a how. On Unix, sends SIGTERM (\a how is `$gracefully`) or
	 * SIGKILL (\a how is `$forcibly`). On Windows, terminates the process forcibly (regardless of \a how) and sets its exit code
	 * to -1 */
	{ DaoOSProcess_LibKill,	"terminate(self: Process, how: enum<gracefully,forcibly> = $forcibly)" },

	/*! Reads *at most* \a count bytes from the output and error stream of the process, or all available data if \a count is
	 * less then 0.
	 *
	 * \note Non-blocking mode is used for reading and writing */
	{ DaoOSProcess_LibRead,	"read(self: Process, count = -1) => string" },

	/*! Writes \a data to the input stream of the process. If the pipe buffer is full, the method will raise `Buffer` error
	 * containing the number of bytes which were not written.
	 *
	 * \note Non-blocking mode is used for reading and writing */
	{ DaoOSProcess_LibWrite,"write(self: Process, data: string)" },

	/*! Checks if the process (with regard to its I/O streams) is in the state specified by \a what.
	 *
	 * \note `check($eof)` checks the output (readable) stream of the process. On Windows, it will always return `false` if
	 * the process is still running */
	{ DaoOSProcess_Check,	"check(self: Process, what: enum<readable,writable,open,eof>) => bool" },

	/*! Closes output (`$read`), input (`$write`) or both process streams depending on \a what. If \a what is not specified,
	 * input stream is assumed. Closing input or output stream forbids further writing or reading accordingly */
	{ DaoOSProcess_LibClose,"close(self: Process, what: enum<read,write,both>)" },
	{ DaoOSProcess_LibClose,"close(self: Process)" },
	{ NULL, NULL }
};

/*! Represents child process, satisfies `io::Device` interface with non-blocking read and write operations. All child processes
 * are automatically tracked by single background tasklet, which automatically updates `Process` objects associated with the
 * sub-processes */
static DaoTypeBase procTyper = {
	"Process", NULL, NULL, procMeths, {NULL}, {0},
	(FuncPtrDel)DaoOSProcess_Delete, NULL
};

static DaoFuncItem osMeths[] =
{
	/*! Creates new child process executing the file specified by \a path with the \a arguments (if given) and \a environment
	 * (if `none`, inherited from the current process). \a path may omit the full path to the file, in that case
	 * its resolution is system-dependent. Returns the corresponding `Process` object.
	 *
	 * \note On Windows, the given \a path and \a arguments are concatenated into command line, where all component are wrapped
	 * in quotes (existing quotes are escaped) and separated by space characters. If such behavior is undesirable for some case,
	 * use `shell()` instead.
	 *
	 * \note The routine will fail with error on Windows if the execution cannot be started (for instance, \a path is not valid);
	 * on Unix, you may need to examine the process status or exit code (which will be set to 1) to detect execution failure.
	 *
	 * \warning On Windows, \a environment is assumed to be in local or ASCII encoding */
	{ OS_Exec,	"exec(path: string, arguments: list<string> = {}, directory = '.', environment: list<string>|none = none) => Process" },

	/*! Similar to `exec()`, but calls system shell ('/bin/sh -c' on Unix, 'cmd /C' on Windows) with the given \a command.
	 * \a command is passed to the shell 'as-is', so you need to ensure that it is properly escaped */
	{ OS_Shell,	"shell(command: string, directory = '.', environment: list<string>|none = none) => Process" },

	/*! Waits for one of child processes in \a children to exit for \a timeout seconds (waits indefinitely if \a timeout is less
	 * then 0). Returns the first found exited process, or `none` if timeouted or if \a children is empty
	 *
	 * \warning If the function is called concurrently from multiple threads, it will ignore the processes which are currently
	 * tracked by its other invocations. */
	{ OS_Wait,	"wait(children: list<Process>, timeout = -1.0) => Process|none" },

	/*! Waits \a timeout seconds for one of child processes in \a children to exit or become available for reading (output buffer
	 * is not empty). If \a timeout is less then 0, waits indefinitely. Returns the first found process matching the criterion,
	 * or `none` if timeouted or if \a children is empty.
	 *
	 * \warning On Windows, `select()` uses system clock for short-term sleeping, thus the accuracy of waiting with timeout may
	 * vary depending on the current clock resolution. */
	{ OS_Select,"select(children: list<Process>, timeout = -1.0) => Process|none" },
	{ NULL, NULL }
};

DAO_DLL int DaoProcess_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *osns = DaoVmSpace_GetNamespace( vmSpace, "os" );
	DaoNamespace_AddConstValue( ns, "os", (DaoValue*)osns );
	daox_type_process = DaoNamespace_WrapType( osns, &procTyper, 1 );
	DaoNamespace_WrapFunctions( osns, osMeths );
	DMutex_Init( &proc_mtx );
#ifdef WIN32
	exec_event = CreateEvent( NULL, TRUE, FALSE, NULL );
#endif
	proc_map = DHash_New( DAO_DATA_VOID2, DAO_DATA_VALUE );
	return 0;
}
