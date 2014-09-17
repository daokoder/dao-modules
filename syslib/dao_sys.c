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
#endif

#ifdef _MSC_VER
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
		DaoProcess_RaiseWarning( proc, "Value", "expecting positive value" );
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
		DaoProcess_RaiseError( proc, NULL, "empty command line" );
		return;
	}
	mode = DString_GetData( p[1]->xString.value );
	stream->file = popen( DString_GetData( fname ), mode );
	if( stream->file == NULL ){
		DaoProcess_RaiseError( proc, NULL, "error opening pipe" );
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
		DaoProcess_RaiseError( proc, NULL, "invalid locale" );
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
		DaoProcess_RaiseError( proc, NULL, "memory allocation failed" );
		return;
	}
	sprintf( buf, "%s=%s", name, value );
	if( putenv( buf ) ){
		DaoProcess_RaiseError( proc, NULL, "error putting environment variable" );
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
	{ NULL, NULL }
};



DAO_DLL int DaoSys_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	ns = DaoNamespace_GetNamespace( ns, "sys" );
	DaoNamespace_WrapFunctions( ns, sysMeths );
	return 0;
}
