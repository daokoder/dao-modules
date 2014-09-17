/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2006-2013, Limin Fu
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

#include"time.h"
#include"math.h"
#include"string.h"
#include"locale.h"
#include"stdlib.h"

#ifdef UNIX
#include<unistd.h>
#include<sys/time.h>
#include<pwd.h>
#include<sys/types.h>
#include<sys/utsname.h>
#endif

#ifdef WIN32
#include<windows.h>
#include<lmcons.h>
#define putenv _putenv
#endif

#ifdef MAC_OSX
#  include <crt_externs.h>
#  define environ (*_NSGetEnviron())
#else
extern char ** environ;
#endif

#include"daoString.h"
#include"daoValue.h"
#include"daoThread.h"
#include"dao_sys.h"

static void SYS_Sleep( DaoProcess *proc, DaoValue *p[], int N )
{
#ifdef DAO_WITH_THREAD
	DMutex    mutex;
	DCondVar  condv;
#endif

	double s = p[0]->xFloat.value;
	if( s < 0 ){
		DaoProcess_RaiseWarning( proc, "Param", "expecting positive value" );
		return;
	}
#ifdef DAO_WITH_THREAD
	/* sleep only the current thread: */
	DMutex_Init( & mutex );
	DCondVar_Init( & condv );
	DMutex_Lock( & mutex );
	DCondVar_TimedWait( & condv, & mutex, s );
	DMutex_Unlock( & mutex );
	DMutex_Destroy( & mutex );
	DCondVar_Destroy( & condv );
#elif UNIX
	sleep( (int)s ); /* This may cause the whole process to sleep. */
#else
	Sleep( s * 1000 );
#endif
}
static void SYS_Exit( DaoProcess *proc, DaoValue *p[], int N )
{
	exit( (int)p[0]->xInteger.value );
}
static void SYS_Shell( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoProcess_PutInteger( proc, system( DString_GetData( p[0]->xString.value ) ) );
}
static void SYS_Popen( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = NULL;
	DString *fname = p[0]->xString.value;
	char *mode;

	stream = DaoStream_New();
	stream->mode |= DAO_STREAM_PIPE;
	DaoProcess_PutValue( proc, (DaoValue*)stream );
	if( DString_Size( fname ) == 0 ){
		DaoProcess_RaiseError( proc, "Param", "empty command line" );
		return;
	}
	mode = DString_GetData( p[1]->xString.value );
	stream->file = popen( DString_GetData( fname ), mode );
	if( stream->file == NULL ){
		DaoProcess_RaiseError( proc, "Sys", "error opening pipe" );
		return;
	}
	if( strstr( mode, "+" ) ){
		stream->mode |= DAO_STREAM_WRITABLE | DAO_STREAM_READABLE;
	}else{
		if( strstr( mode, "r" ) ) stream->mode |= DAO_STREAM_READABLE;
		if( strstr( mode, "w" ) || strstr( mode, "a" ) ) stream->mode |= DAO_STREAM_WRITABLE;
	}
}
static void SYS_SetLocale( DaoProcess *proc, DaoValue *p[], int N )
{
	int category = 0;
	char* old;
	switch( p[0]->xEnum.value ){
	case 0: category = LC_ALL; break;
	case 1: category = LC_COLLATE; break;
	case 2: category = LC_CTYPE; break;
	case 3: category = LC_MONETARY; break;
	case 4: category = LC_NUMERIC; break;
	case 5: category = LC_TIME; break;
	}
	old = setlocale( category, N == 1 ? NULL : DString_GetData( p[1]->xString.value ) );
	if ( old )
		DaoProcess_PutChars( proc, old );
	else
		DaoProcess_RaiseError( proc, "Sys", "invalid locale" );
}
static void SYS_Clock( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoProcess_PutFloat( proc, ((float)clock())/CLOCKS_PER_SEC );
}
static void SYS_GetEnv( DaoProcess *proc, DaoValue *p[], int N )
{
	char *evar = getenv( DString_GetData( p[0]->xString.value ) );
	DaoProcess_PutChars( proc, evar? evar : "" );
}
static void SYS_PutEnv( DaoProcess *proc, DaoValue *p[], int N )
{
	char *name = DString_GetData( p[0]->xString.value );
	char *value = DString_GetData( p[1]->xString.value );
	char *buf = malloc( strlen( name ) + strlen( value ) + 2 );
	if( !buf ){
		DaoProcess_RaiseError( proc, "Sys", "memory allocation failed" );
		return;
	}
	sprintf( buf, "%s=%s", name, value );
	if( putenv( buf ) ){
		DaoProcess_RaiseError( proc, "Sys", "error putting environment variable" );
		free( buf );
	}
}
static void SYS_EnvVars( DaoProcess *proc, DaoValue *p[], int N )
{
#define LOCAL_BUF_SIZE 256
	DaoMap *map = DaoProcess_PutMap( proc, 0 );
	DaoValue *vk = (DaoValue*) DaoProcess_NewString( proc, NULL, 0 );
	DaoValue *vv = (DaoValue*) DaoProcess_NewString( proc, NULL, 0 );
	DString *key = DaoString_Get( DaoValue_CastString( vk ) );
	DString *value = DaoString_Get( DaoValue_CastString( vv ) );
	char **envs = environ;
	char buffer[ LOCAL_BUF_SIZE + 1 ];
	int nc = 0;

	while( *envs != NULL ){
		char *c = *envs;
		nc = 0;
		while( *c != '=' ){
			if( nc >= LOCAL_BUF_SIZE ){
				buffer[ nc ] = 0;
				DString_AppendChars( key, buffer );
				nc = 0;
			}
			buffer[ nc ] = *c;
			nc ++;
			c ++;
		}
		buffer[ nc ] = 0;
		DString_AppendChars( key, buffer );
		c ++;
		DString_AppendChars( value, c );
		DaoMap_Insert( map, vk, vv );
		DString_Clear( key );
		DString_Clear( value );
		envs ++;
	}
}

void DString_SetTChars( DString *str, wchar_t *tcs )
{
	DString_Clear( str );
	for ( ; *tcs != L'\0'; tcs++ )
		if ( *tcs >= 0xD800 && *tcs <= 0xDBFF ){
			size_t lead = ( (size_t)*tcs - 0xD800 ) << 10;
			tcs++;
			if ( *tcs == L'\0' )
					break;
			DString_AppendWChar( str, lead + ( (size_t)*tcs - 0xDC00 ) );
		}
		else
			DString_AppendWChar( str, *tcs );
}

static void SYS_User( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *user = DaoProcess_PutChars( proc, "" );
	int res;
#ifdef WIN32
	wchar_t buf[UNLEN + 1];
	DWORD len = sizeof(buf);
	res = GetUserNameW( buf, &len );
	if ( res )
		DString_SetTChars( user, buf );
#else
	struct passwd pwd, *ptr;
	char buf[4096];
	res = getpwuid_r( getuid(), &pwd, buf, 4096, &ptr ) == 0;
	if ( res )
		DString_SetChars( user, pwd.pw_name );
#endif
	if ( !res )
		DaoProcess_RaiseError( proc, "Sys", "Failed to get user information" );
}

static void SYS_Uname( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTuple *tup = DaoProcess_PutTuple( proc, 4 );
	int res;
#ifdef WIN32
	OSVERSIONINFOEX info;
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	res = GetVersionEx( (OSVERSIONINFO*)&info );
	if ( res ){
		wchar_t buf[512];
		DWORD len = sizeof(buf);
		char *version;
		DString_SetChars( tup->values[0]->xString.value, "Windows" );
		switch ( info.dwMajorVersion ){
		case 5:
			switch ( info.dwMinorVersion ){
			case 0:	version = "2000"; break;
			case 1:	version = "XP"; break;
			case 2:	version = GetSystemMetrics( SM_SERVERR2 )? "Server 2003 R2" : "Server 2003"; break;
			}
			break;
		case 6:
			switch ( info.dwMinorVersion ){
			case 0:	version = info.wProductType == VER_NT_WORKSTATION? "Vista" : "Server 2008"; break;
			case 1:	version = info.wProductType == VER_NT_WORKSTATION? "7" : "Server 2008 R2"; break;
			case 2:	version = info.wProductType == VER_NT_WORKSTATION? "8" : "Server 2012"; break;
			case 3:	version = info.wProductType == VER_NT_WORKSTATION? "8.1" : "Server 2012 R2"; break;
			}
			break;
		}
		DString_SetChars( tup->values[1]->xString.value, version );
		DString_AppendChar( tup->values[2]->xString.value, '0' + info.dwMajorVersion );
		DString_AppendChar( tup->values[2]->xString.value, '.' );
		DString_AppendChar( tup->values[2]->xString.value, '0' + info.dwMinorVersion );
		if ( GetComputerNameExW( ComputerNameDnsFullyQualified, buf, &len ) )
			DString_SetTChars( tup->values[3]->xString.value, buf );
	}
#else
	struct utsname info;
	res = uname( &info ) != -1;
	if ( res ){
		DString_SetChars( tup->values[0]->xString.value, info.sysname );
		DString_SetChars( tup->values[1]->xString.value, info.version );
		DString_SetChars( tup->values[2]->xString.value, info.release );
		DString_SetChars( tup->values[3]->xString.value, info.nodename );
	}
#endif
	if ( !res )
		DaoProcess_RaiseError( proc, "Sys", "Failed to get system information" );
}

static DaoFuncItem sysMeths[]=
{
	{ SYS_Shell,     "shell( command: string ) => int" },
	{ SYS_Popen,     "popen( cmd: string, mode: string ) => io::Stream" },
	{ SYS_Sleep,     "sleep( seconds: float )" },
	{ SYS_Exit,      "exit( code = 0 )" },
	{ SYS_Clock,     "clock() => float" },
	{ SYS_SetLocale, "setlocale( category: enum<all,collate,ctype,monetary,numeric,time> = $all, locale = \"\" ) => string" },
	{ SYS_EnvVars,   "getenv() => map<string,string>"},
	{ SYS_GetEnv,    "getenv( name: string ) => string" },
	{ SYS_PutEnv,    "putenv( name: string, value = \"\" )"},
	{ SYS_User,      "user() => string"},
	{ SYS_Uname,     "uname() => tuple<system: string, version: string, release: string, host: string>"},
	{ NULL, NULL }
};



DAO_DLL int DaoSys_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	ns = DaoNamespace_GetNamespace( ns, "sys" );
	DaoNamespace_WrapFunctions( ns, sysMeths );
	return 0;
}
