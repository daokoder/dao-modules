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

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>

#ifdef DAO_WITH_THREAD
#include <pthread.h>
#endif

#include"dao.h"
#include"daoValue.h"

typedef struct DaoSignalPipe DaoSignalPipe;

struct DaoSignalPipe {
	int readfd, writefd;
};

static DaoSignalPipe signal_pipe = {-1, -1};

void SignalHanlder( int sig )
{
	char num  = sig;
	write( signal_pipe.writefd, &num, 1 );
}

void DaoSignalPipe_Delete( DaoSignalPipe *self )
{
	// not freed -- singleton
}

enum {
	Signal_Int = 1,
	Signal_Term = 2,
	Signal_Quit = 4,
	Signal_Hup = 8,
	Signal_Chld = 16,
	Signal_Usr1 = 32,
	Signal_Usr2 = 64,
	Signal_Pipe = 128
};

static void UNIX_Trap( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoType *retype = DaoProcess_GetReturnType( proc );
	int sig = p[0]->xEnum.value;
	struct sigaction action;

	action.sa_handler = SignalHanlder;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	if ( sig & Signal_Int )
		sigaction( SIGINT, &action, NULL );
	if ( sig & Signal_Term )
		sigaction( SIGTERM, &action, NULL );
	if ( sig & Signal_Quit )
		sigaction( SIGQUIT, &action, NULL );
	if ( sig & Signal_Hup )
		sigaction( SIGHUP, &action, NULL );
	if ( sig & Signal_Chld )
		sigaction( SIGCHLD, &action, NULL );
	if ( sig & Signal_Usr1 )
		sigaction( SIGUSR1, &action, NULL );
	if ( sig & Signal_Usr2 )
		sigaction( SIGUSR2, &action, NULL );
	if ( sig & Signal_Pipe )
		sigaction( SIGPIPE, &action, NULL );
	DaoProcess_PutCdata( proc, &signal_pipe, retype );
}

static void UNIX_Pid( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoProcess_PutInteger( proc, getpid() );
}


static void UNIX_Kill( DaoProcess *proc, DaoValue *p[], int N )
{
	int sig = 0;
	switch ( p[1]->xEnum.value ){
	case 0:		sig = SIGINT; break;
	case 1:		sig = SIGTERM; break;
	case 2:		sig = SIGQUIT; break;
	case 3:		sig = SIGHUP; break;
	case 4:		sig = SIGCHLD; break;
	case 5:		sig = SIGUSR1; break;
	case 6:		sig = SIGUSR2; break;
	case 7:		sig = SIGPIPE; break;
	case 8:		sig = SIGKILL; break;
	case 9:		sig = SIGSTOP; break;
	case 10:	sig = SIGCONT; break;
	}
	if ( kill( p[0]->xInteger.value, sig ) == -1 )
		DaoProcess_RaiseError( proc, "System", errno == EPERM? "Insufficient permissions (EPERM)" : "Process or group does not exit (ESRCH)" );
}

enum {
	Poll_In = 1,
	Poll_Out = 2,
	Poll_Error = 4,
	Poll_Hup = 8,
	Poll_None = 16
};

static void UNIX_Poll( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoList *fdlist = &p[0]->xList;
	if ( fdlist->value->size ){
		struct pollfd fds[fdlist->value->size];
		daoint i;
		int res, timeout = p[1]->xFloat.value*1E3;
		if ( timeout < 0 ){
			DaoProcess_RaiseError( proc, "Param", "Invalid timeout" );
			return;
		}
		for ( i = 0; i < fdlist->value->size; ++i ){
			DaoTuple *otup = &DaoList_GetItem( fdlist, i )->xTuple;
			DaoTuple *itup = &otup->values[1]->xTuple;
			int events = itup->values[0]->xEnum.value;
			itup->values[1]->xEnum.value = Poll_None;
			fds[i].fd = otup->values[0]->xInteger.value;
			fds[i].events = 0;
			if ( events & Poll_In )
				fds[i].events |= POLLIN;
			if ( events & Poll_Out )
				fds[i].events |= POLLOUT;
			if ( events & Poll_Error )
				fds[i].events |= POLLERR;
			if ( events & Poll_Hup )
				fds[i].events |= POLLHUP;
		}
		res = poll( fds, (nfds_t)fdlist->value->size, timeout );
		if ( res == -1 ){
			if ( errno == EINTR )
				DaoProcess_PutEnum( proc, "$interrupted" );
			else
				DaoProcess_RaiseError( proc, "System", errno == EINVAL? "Too many file descriptors given (EINVAL)" : "Insufficient memory (ENOMEM)" );
		}
		else if ( res == 0 )
			DaoProcess_PutEnum( proc, "$timeouted" );
		else {
			DaoProcess_PutEnum( proc, "$polled" );
			for ( i = 0; i < fdlist->value->size; ++i ){
				DaoTuple *tup = &DaoList_GetItem( fdlist, i )->xTuple;
				DaoEnum *en = &tup->values[1]->xTuple.values[1]->xEnum;
				if ( !fds[i].revents )
					en->value = Poll_None;
				else {
					if ( fds[i].revents & POLLIN )
						en->value |= Poll_In;
					if ( fds[i].revents & POLLOUT )
						en->value |= Poll_Out;
					if ( fds[i].revents & POLLERR )
						en->value |= Poll_Error;
					if ( fds[i].revents & POLLHUP )
						en->value |= Poll_Hup;
				}
			}
		}
	}
}

static void UNIX_Umask( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTuple *res = DaoProcess_PutTuple( proc, 3 );
	mode_t perm = 0;
	DString *pmode;
	const char *mode = p[0]->xString.value->chars;
	const char *error = "Invalid user mode";

	for ( ; *mode; ++mode )
		switch ( *mode ){
		case 'r':	if ( perm & S_IRUSR ) goto FormatError; perm |= S_IRUSR; break;
		case 'w':	if ( perm & S_IWUSR ) goto FormatError; perm |= S_IWUSR; break;
		case 'x':	if ( perm & S_IXUSR ) goto FormatError; perm |= S_IXUSR; break;
		default:	goto FormatError;
		}
	mode = p[1]->xString.value->chars;
	error = "Invalid group mode";
	for ( ; *mode; ++mode )
		switch ( *mode ){
		case 'r':	if ( perm & S_IRGRP ) goto FormatError; perm |= S_IRGRP; break;
		case 'w':	if ( perm & S_IWGRP ) goto FormatError; perm |= S_IWGRP; break;
		case 'x':	if ( perm & S_IXGRP ) goto FormatError; perm |= S_IXGRP; break;
		default:	goto FormatError;
		}
	mode = p[2]->xString.value->chars;
	error = "Invalid other mode";
	for ( ; *mode; ++mode )
		switch ( *mode ){
		case 'r':	if ( perm & S_IROTH ) goto FormatError; perm |= S_IROTH; break;
		case 'w':	if ( perm & S_IWOTH ) goto FormatError; perm |= S_IWOTH; break;
		case 'x':	if ( perm & S_IXOTH ) goto FormatError; perm |= S_IXOTH; break;
		default:	goto FormatError;
		}

	perm = umask( perm );
	pmode = res->values[0]->xString.value;
	pmode->size = 0;
	if ( perm & S_IRUSR ) DString_AppendChar( pmode, 'r' );
	if ( perm & S_IWUSR ) DString_AppendChar( pmode, 'w' );
	if ( perm & S_IXUSR ) DString_AppendChar( pmode, 'x' );
	pmode = res->values[1]->xString.value;
	pmode->size = 0;
	if ( perm & S_IRGRP ) DString_AppendChar( pmode, 'r' );
	if ( perm & S_IWGRP ) DString_AppendChar( pmode, 'w' );
	if ( perm & S_IXGRP ) DString_AppendChar( pmode, 'x' );
	pmode = res->values[2]->xString.value;
	pmode->size = 0;
	if ( perm & S_IROTH ) DString_AppendChar( pmode, 'r' );
	if ( perm & S_IWOTH ) DString_AppendChar( pmode, 'w' );
	if ( perm & S_IXOTH ) DString_AppendChar( pmode, 'x' );
	return;
FormatError:
	DaoProcess_RaiseError( proc, "Param", error );
}

static void SetSignalMask( DaoProcess *proc, DaoValue *p[], int block )
{
	int sig = p[0]->xEnum.value;
	int action = block? SIG_BLOCK : SIG_UNBLOCK;
	sigset_t set;

	sigemptyset(&set);
	if ( sig & Signal_Int )
		sigaddset( &set, SIGINT);
	if ( sig & Signal_Term )
		sigaddset( &set, SIGTERM );
	if ( sig & Signal_Quit )
		sigaddset( &set, SIGQUIT );
	if ( sig & Signal_Hup )
		sigaddset( &set, SIGHUP );
	if ( sig & Signal_Chld )
		sigaddset( &set, SIGCHLD );
	if ( sig & Signal_Usr1 )
		sigaddset( &set, SIGUSR1 );
	if ( sig & Signal_Usr2 )
		sigaddset( &set, SIGUSR2 );
	if ( sig & Signal_Pipe )
		sigaddset( &set, SIGPIPE );
#ifdef DAO_WITH_THREAD
	pthread_sigmask(action, &set, NULL);
#else
	sigprocmask(action, &set, NULL);
#endif
}

static void UNIX_Block( DaoProcess *proc, DaoValue *p[], int N )
{
	SetSignalMask( proc, p, 1 );
}

static void UNIX_Unblock( DaoProcess *proc, DaoValue *p[], int N )
{
	SetSignalMask( proc, p, 0 );
}

static DaoFunctionEntry unixMeths[] =
{
	//! Sets the specified \a signals to be suppressed; when on of them is catched by the process, its ID will be written to the returned
	//! signal pipe (which exists in single instance per process)
	{ UNIX_Trap,	"trap(signals: SignalSet) => SignalPipe" },

	//! Blocks or unblocks the specified \a signals for the current thread using \c pthread_sigmask() or \c sigprocmask()
	//!
	//! \warning Be cautious when using these functions with tasklets -- newly created tasklets may reuse previously spawned threads
	{ UNIX_Block,	"block(signals: SignalSet)" },
	{ UNIX_Unblock,	"unblock(signals: SignalSet)" },

	//! Current process PID
	{ UNIX_Pid,		"pid() => int" },

	//! Sends \a signal to \a pid
	{ UNIX_Kill,	"kill(pid: int, signal: enum<sigint,sigterm,sigquit,sighup,sigchld,sigusr1,sigusr2,sigpipe,sigkill,sigstop,sigcont>)" },

	//! Waits \a timeout seconds for events on file \a descriptors. Each \c PollFd item specifies *fd* to be polled and *events.monitored*;
	//! *events.occurred* is updated according to the results (set to \c $none before polling).
	//!
	//! Possible results:
	//! - \c polled -- one or more events occurred
	//! - \c timeouted -- no events occurred within the given timeout
	//! - \c interrupted -- interrupted by a signal before any events occurred
	{ UNIX_Poll,	"poll(descriptors: list<PollFd>, timeout: float) => enum<polled,timeouted,interrupted>" },

	//! Sets the default \a user, \a group and \a other file permissions for the process and returns the previous permissions
	{ UNIX_Umask,	"umask(user: string, group: string, other: string) => tuple<user: string, group: string, other: string>" },
	{ NULL, NULL }
};

static void SIGPIPE_Fd( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSignalPipe *pipe = (DaoSignalPipe*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, pipe->readfd );
}

static void SIGPIPE_Fetch( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSignalPipe *pipe = (DaoSignalPipe*)DaoValue_TryGetCdata( p[0] );
	char sig;
	int res;
	do
		res = read( pipe->readfd, &sig, 1 );
	while ( res != 1 && errno == EINTR );
	if ( res != 1 )
		DaoProcess_RaiseError( proc, "System", "Failed to read from the signal pipe" );
	else {
		char *symbol = "";
		switch ( sig ){
		case SIGINT:	symbol = "$sigint"; break;
		case SIGTERM:	symbol = "$sigterm"; break;
		case SIGQUIT:	symbol = "$sigquit"; break;
		case SIGHUP:	symbol = "$sighup"; break;
		case SIGCHLD:	symbol = "$sigchld"; break;
		case SIGUSR1:	symbol = "$sigusr1"; break;
		case SIGUSR2:	symbol = "$sigusr2"; break;
		case SIGPIPE:	symbol = "$sigpipe"; break;
		}
		DaoProcess_PutEnum( proc, symbol );
	}
}

static DaoFunctionEntry daoSignalPipeMeths[] =
{
	//! File descriptor for the read end
	{ SIGPIPE_Fd,	"fd(invar self: SignalPipe) => int" },

	//! Reads signal ID from the pipe; blocks until a signal occurs
	{ SIGPIPE_Fetch,"fetch(self: SignalPipe) => enum<sigint,sigterm,sigquit,sighup,sigchld,sigusr1,sigusr2,sigpipe>" },
	{ NULL, NULL }
};


static void DaoSignalPipe_CoreDelete( DaoValue *self )
{
	DaoSignalPipe_Delete( (DaoSignalPipe*) self->xCdata.data );
	DaoCstruct_Delete( (DaoCstruct*) self );
}

//! Pipe from which signals sent to the process can be fetched
DaoTypeCore daoSignalPipeCore =
{
	"SignalPipe",                                          /* name */
	sizeof(DaoSignalPipe),                                 /* size */
	{ NULL },                                              /* bases */
	{ NULL },                                              /* casts */
	NULL,                                                  /* numbers */
	daoSignalPipeMeths,                                    /* methods */
	DaoCstruct_CheckGetField,    DaoCstruct_DoGetField,    /* GetField */
	DaoCstruct_CheckSetField,    DaoCstruct_DoSetField,    /* SetField */
	DaoCstruct_CheckGetItem,     DaoCstruct_DoGetItem,     /* GetItem */
	NULL,                        NULL,                     /* SetItem */
	NULL,                        NULL,                     /* Unary */
	NULL,                        NULL,                     /* Binary */
	DaoCstruct_CheckConversion,  DaoCstruct_DoConversion,  /* Conversion */
	NULL,                        NULL,                     /* ForEach */
	NULL,                                                  /* Print */
	NULL,                                                  /* Slice */
	NULL,                                                  /* Compare */
	NULL,                                                  /* Hash */
	NULL,                                                  /* Create */
	NULL,                                                  /* Copy */
	(DaoDeleteFunction) DaoSignalPipe_CoreDelete,          /* Delete */
	NULL                                                   /* HandleGC */
};


DAO_DLL int DaoUnix_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *osns = DaoVmSpace_GetNamespace( vmSpace, "os" );
	DaoNamespace_AddConstValue( ns, "os", (DaoValue*)osns );
	DaoNamespace_WrapType( osns, &daoSignalPipeCore, DAO_CDATA, 0 );
	DaoNamespace_DefineType( osns, "enum<sigint;sigterm;sigquit;sighup;sigchld;sigusr1;sigusr2;sigpipe>", "SignalSet" );
	DaoNamespace_DefineType( osns, "enum<in;out;error;hup;none>", "PollEvent" );
	DaoNamespace_DefineType( osns, "tuple<fd: int, events: tuple<monitored: PollEvent, occurred: PollEvent>>", "PollFd" );
	DaoNamespace_WrapFunctions( osns, unixMeths );

	if ( pipe((int*)&signal_pipe) != 0 )
		DaoStream_WriteChars( DaoVmSpace_ErrorStream( vmSpace ), "WARNING: Failed to create signal pipe!\n" );
	return 0;
}
