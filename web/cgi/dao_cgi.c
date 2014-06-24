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

#include"stdio.h"
#include"stdlib.h"
#include"string.h"
#include"unistd.h"
#include"time.h"

// must be larger than the size of boundary string for multipart/form-data:
#define LOCAL_BUF_SIZE 1000

#include"dao.h"
#include"daoStdtype.h"
#include"daoNamespace.h"

#ifdef MAC_OSX
#  include <crt_externs.h>
#  define environ (*_NSGetEnviron())
#else
extern char ** environ;
#endif

DaoType *daox_type_namestream = NULL;
DaoType *daox_type_filemap = NULL;

static DaoVmSpace *vmMaster = NULL;

static void InsertKeyValue( DaoProcess *proc, DaoMap *mulmap, DaoMap *map, DaoValue *vk, DaoValue *vv )
{
	DaoValue *val, *vlist;
	DaoMap_Insert( map, vk, vv );
	if( mulmap ){
		val = DaoMap_GetValue( mulmap, vk );
		if( val == NULL ){
			vlist = (DaoValue*) DaoProcess_NewList( proc );
			DaoMap_Insert( mulmap, vk, vlist );
			val = DaoMap_GetValue( mulmap, vk );
		}
		DaoList_PushBack( DaoValue_CastList( val ), vv );
	}
}

#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

static void ParseKeyValueString( DaoProcess *proc, DaoMap *mulmap, DaoMap *map, const char *data )
{
	const char *end = data + strlen( data );
	DaoValue *vk = (DaoValue*) DaoProcess_NewString( proc, NULL, 0 );
	DaoValue *vv = (DaoValue*) DaoProcess_NewString( proc, NULL, 0 );
	DString *key = DaoString_Get( DaoValue_CastString( vk ) );
	DString *value = DaoString_Get( DaoValue_CastString( vv ) );
	DString *buffer = key;

	buffer->size = 0;
	for(; data < end; ++data){
		if( buffer->size >= buffer->bufSize ) DString_Reserve( buffer, 1.5*buffer->size + 8 );
		if( *data == '=' ){
			buffer->chars[ buffer->size ] = 0;
			buffer = value;
			buffer->size = 0;
		}else if( *data == '&' || *data == ';' ){
			buffer->chars[ buffer->size ] = 0;
			InsertKeyValue( proc, mulmap, map, vk, vv );
			DString_Reset( key, 0 );   /* also detaching shared memory; */
			DString_Reset( value, 0 ); /* also detaching shared memory; */
			buffer = key;
		}else if( *data != ' ' ){
			if( *data == '%' ){
				char a = tolower( data[1] );
				char b = tolower( data[2] );
				buffer->chars[ buffer->size ++ ] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
				data += 2;
			}else if( *data == '+' ){
				buffer->chars[ buffer->size ++ ] = ' ';
			}else{
				buffer->chars[ buffer->size ++ ] = *data;
			}
		}
	}
	if( key->size ) InsertKeyValue( proc, mulmap, map, vk, vv );
}
static void ParseKeyValueStringArray( DaoProcess *proc, DaoMap *map, char **p )
{
	int nc = 0;
	char buffer[ LOCAL_BUF_SIZE + 1 ];
	
	DaoValue *vk = (DaoValue*) DaoProcess_NewString( proc, NULL, 0 );
	DaoValue *vv = (DaoValue*) DaoProcess_NewString( proc, NULL, 0 );
	DString *key = DaoString_Get( DaoValue_CastString( vk ) );
	DString *value = DaoString_Get( DaoValue_CastString( vv ) );
	while( *p != NULL ){
		char *c = *p;
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
		p ++;
	}
}


/*
// POST /upload HTTP/1.1
// Host: 127.0.0.1:8080
// Content-Length: 123456
// Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryHPkMEn
//
// ------WebKitFormBoundaryHPkMEn
// Content-Disposition: form-data; name="first"
//
// field abc
// ------WebKitFormBoundaryHPkMEn
// Content-Disposition: form-data; name="second"
//
// field def
// ------WebKitFormBoundaryHPkMEn
// Content-Disposition: form-data; name="file"; filename="summary.txt"
// Content-Type: text/plain
//
// Test Summary:  files, 19 passed, 0 failed;  units: 169 passed, 0 failed;
//
// ------WebKitFormBoundaryHPkMEn--
*/

static void PreparePostData( DaoProcess *proc, DaoMap *httpPOSTS, DaoMap *httpPOST, DaoMap *httpFILE )
{
	DString *fname;
	DString *buffer = DString_New();
	DaoValue *vk = (DaoValue*) DaoProcess_NewString( proc, NULL, 0 );
	DaoValue *vv = (DaoValue*) DaoProcess_NewString( proc, NULL, 0 );
	DString *key = DaoString_Get( DaoValue_CastString( vk ) );
	DString *value = DaoString_Get( DaoValue_CastString( vv ) );
	char *content_length = getenv( "CONTENT_LENGTH" );
	char *content_type = getenv( "CONTENT_TYPE" );
	const char *boundary;
	char postbuf[1024];
	daoint postlen, boundarylen, len = 0;
	daoint pos, pos2, pos_rnrn, offset;

	if( content_length != NULL ) len = strtol( content_length, NULL, 10);
	if( len == 0 ) return;
	DString_SetSharing( buffer, 0 );
	if( content_type == NULL || strstr( content_type, "multipart/form-data" ) == NULL ){
		postlen = fread( postbuf, 1, sizeof(postbuf), stdin );
		while( postlen ){
			DString_AppendBytes( buffer, postbuf, postlen );
			postlen = fread( postbuf, 1, sizeof(postbuf), stdin );
		}
		ParseKeyValueString( proc, httpPOSTS, httpPOST, buffer->chars );
		DString_Delete( buffer );
		return;
	}
	boundary = strstr( content_type, "boundary=" ) + strlen( "boundary=" );
	boundarylen = strlen( boundary );

	fname = DString_New();
	buffer->size = 0;
	for(;;){
		postlen = fread( postbuf, 1, sizeof(postbuf), stdin );
		if( postlen == 0 && buffer->size < boundarylen ) break;

		DString_AppendBytes( buffer, postbuf, postlen );
		while( strstr( buffer->chars, "\r\n\r\n" ) == 0 && postlen != 0 ){
			postlen = fread( postbuf, 1, sizeof(postbuf), stdin );
			DString_AppendBytes( buffer, postbuf, postlen );
		}
		//printf( "###############\n%s\n", buffer->chars );

		key->size = 0;
		fname->size = 0;
		pos = DString_FindChars( buffer, "name=", 20 ); /* Skip: Content-Disposition: ; */
		pos2 = DString_FindChar( buffer, '\"', pos+6 );
		DString_SubString( buffer, key, pos + 6, pos2 - pos - 6 );

		pos_rnrn = DString_FindChars( buffer, "\r\n\r\n", pos2 );
		pos = DString_FindChars( buffer, "filename=", pos2 );
		if( pos != DAO_NULLPOS && pos < pos_rnrn ){
			daoint pos3 = DString_FindChar( buffer, '\"', pos+10 );
			DString_SubString( buffer, fname, pos + 10, pos3 - pos - 10 );
		}

		buffer->size -= pos_rnrn + 4;
		memmove( buffer->chars, buffer->chars + pos_rnrn + 4, buffer->size );
		if( fname->size == 0 ){
			offset = 0;
			while( (pos2 = DString_FindChars( buffer, boundary, offset )) == DAO_NULLPOS ){
				offset = buffer->size - boundarylen;
				if( offset < 0 ) offset = 0;
				postlen = fread( postbuf, 1, sizeof(postbuf), stdin );
				DString_AppendBytes( buffer, postbuf, postlen );
			}
			DString_SubString( buffer, value, 0, pos2 - 4 ); /* \r\n-- */
			DaoMap_Insert( httpPOST, (DaoValue*) vk, (DaoValue*) vv );
			buffer->size -= pos2 + boundarylen;
			memmove( buffer->chars, buffer->chars + pos2 + boundarylen, buffer->size );
		}else{
			DaoInteger isize = {DAO_INTEGER,0,0,0,0,0};
			DaoStream *stream = DaoStream_New();
			DaoTuple *tuple = DaoTuple_New(3);
			FILE *file = tmpfile();

			DaoString_Set( (DaoString*) vv, fname );
			DaoStream_SetFile( stream, file );
			DaoTuple_SetType( tuple, daox_type_namestream );
			DaoTuple_SetItem( tuple, (DaoValue*) vv, 0 );
			DaoTuple_SetItem( tuple, (DaoValue*) stream, 2 );
			DaoMap_Insert( httpFILE, (DaoValue*) vk, (DaoValue*) tuple );

			offset = 0;
			while( (pos2 = DString_FindChars( buffer, boundary, 0 )) == DAO_NULLPOS ){
				offset = buffer->size - boundarylen;
				if( offset > 0 ){
					isize.value += offset;
					fwrite( buffer->chars, 1, offset, file );
					buffer->size -= offset;
					memmove( buffer->chars, buffer->chars + offset, buffer->size );
				}
				postlen = fread( postbuf, 1, sizeof(postbuf), stdin );
				DString_AppendBytes( buffer, postbuf, postlen );
			}
			isize.value += pos2 - 4;
			fwrite( buffer->chars, 1, pos2 - 4, file );  /* \r\n-- */
			buffer->size -= pos2 + boundarylen;
			memmove( buffer->chars, buffer->chars + pos2 + boundarylen, buffer->size );
			rewind( file );
			DaoTuple_SetItem( tuple, (DaoValue*) & isize, 1 );
		}
	}
	DString_Delete( buffer );
}

const char alnumChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
void DaoCGI_RandomString( DaoProcess *proc, DaoValue *p[], int N )
{
	int len = DaoValue_TryGetInteger( p[0] );
	int alnum = DaoValue_TryGetInteger( p[1] );
	int i;
	DString *res = DaoProcess_PutChars( proc, "" );
	if( alnum ){
		for(i=0; i<len; i++)
			DString_AppendChar( res, alnumChars[ (int)(62 * (rand()/(RAND_MAX+1.0)) ) ] );
	}else{
		for(i=0; i<len; i++)
			DString_AppendChar( res, (char)(255 * (rand()/(RAND_MAX+1.0))) );
	}
}
#define IO_BUF_SIZE (1<<12)
void DaoCGI_SendFile( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *mbs;
	DString *file = DaoValue_TryGetString( p[0] );
	DString *mime = DaoValue_TryGetString( p[1] );
	DString *notfound = DaoValue_TryGetString( p[2] );
	char buf[IO_BUF_SIZE];
	FILE *fin = fopen( DString_GetData( file ), "r" );
	if( fin == NULL ){
		printf( "%s", DString_GetData( notfound ) );
		return;
	}
	mbs = DString_New();
	printf( "Content-Type: %s\n\n", DString_GetData( mime ) );
	while(1){
		size_t count = fread( buf, 1, IO_BUF_SIZE, fin );
		if( count ==0 ) break;
		DString_Reset( mbs, 0 );
		DString_AppendBytes( mbs, buf, count );
		DaoFile_WriteString( stdout, mbs );
	}
	fclose( fin );
	DString_Delete( mbs );
}

static DaoFuncItem cgiMeths[]=
{
	{ DaoCGI_RandomString,  "random_string( n:int, alnum=1 )=>string" },
	{ DaoCGI_SendFile,  "sendfile( file :string, mime='text/plain', notfound='' )=>string" },
	{ NULL, NULL }
};

DAO_DLL int DaoCGI_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoProcess *process = DaoVmSpace_AcquireProcess( vmSpace );
	DaoMap *httpENV, *httpGET, *httpPOST, *httpFILE, *httpCOOKIE;
	DaoMap *httpGETS, *httpPOSTS;
	srand( time(NULL) );

	vmMaster = vmSpace;

	daox_type_namestream = DaoNamespace_TypeDefine( ns, "tuple<file:string,size:int,data:io::stream>", "HttpUpload" );
	daox_type_filemap = DaoNamespace_ParseType( ns, "map<string,HttpUpload>" );

	DaoNamespace_WrapFunctions( ns, cgiMeths );

	httpENV = DaoMap_New(1+rand());
	httpGET = DaoMap_New(1+rand());
	httpPOST = DaoMap_New(1+rand());
	httpFILE = DaoMap_New(1+rand());
	httpCOOKIE = DaoMap_New(1+rand());
	httpGETS = DaoMap_New(1+rand());
	httpPOSTS = DaoMap_New(1+rand());

	DaoNamespace_AddValue( ns, "HTTP_ENV", (DaoValue*)httpENV, "map<string,string>" );
	DaoNamespace_AddValue( ns, "HTTP_GET", (DaoValue*)httpGET, "map<string,string>" );
	DaoNamespace_AddValue( ns, "HTTP_POST", (DaoValue*)httpPOST, "map<string,string>" );
	DaoNamespace_AddValue( ns, "HTTP_FILE", (DaoValue*)httpFILE, "map<string,HttpUpload>" );
	DaoNamespace_AddValue( ns, "HTTP_COOKIE", (DaoValue*)httpCOOKIE, "map<string,string>");
	DaoNamespace_AddValue( ns,"HTTP_GETS",(DaoValue*)httpGETS,"map<string,list<string>>");
	DaoNamespace_AddValue( ns,"HTTP_POSTS",(DaoValue*)httpPOSTS,"map<string,list<string>>");

	// Prepare HTTP_ENV:
	ParseKeyValueStringArray( process, httpENV, environ );

	// Prepare HTTP_GET:
	char *query = getenv( "QUERY_STRING" );
	if( query == NULL ){ /* lighttpd does not set "QUERY_STRING": */
		query = getenv( "REQUEST_URI" );
		if( query ) query = strchr( query, '?' );
		if( query ) query += 1;
	}
	if( query ) ParseKeyValueString( process, httpGETS, httpGET, query );
	query = getenv( "HTTP_COOKIE" );
	if( query ) ParseKeyValueString( process, NULL, httpCOOKIE, query );

	PreparePostData( process, httpPOSTS, httpPOST, httpFILE );

	DaoVmSpace_ReleaseProcess( vmSpace, process );
	return 0;
}


