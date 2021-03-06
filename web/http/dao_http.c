/*
// Webdao: A Lightweight Web Application Framework for Dao
//
// Copyright (c) 2013-2017, Limin Fu
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

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "dao.h"
#include "daoStdtype.h"
#include "daoValue.h"
#include "daoVmspace.h"
#include "daoThread.h"
#include "daoGC.h"

#define DAO_HAS_STREAM
#include"dao_api.h"

#include "Marten/marten.h"

#ifndef DAO_WITH_THREAD
#define DMutex_Init(x)
#define DMutex_Destroy(x)
//#define DMutex_Lock(x)
//#define DMutex_Unlock(x)
#endif

typedef struct mg_callbacks     mg_callbacks;
typedef struct mg_request_info  mg_request_info;
typedef struct mg_connection    mg_connection;
typedef struct mg_context       mg_context;

typedef struct DaoxRequest   DaoxRequest;
typedef struct DaoxResponse  DaoxResponse;
typedef struct DaoxSession   DaoxSession;
typedef struct DaoxCache     DaoxCache;
typedef struct DaoxHandler   DaoxHandler;
typedef struct DaoxServer    DaoxServer;

extern DaoTypeCore daoRequestCore;
extern DaoTypeCore daoResponseCore;
extern DaoTypeCore daoSessionCore;
extern DaoTypeCore daoCacheCore;
extern DaoTypeCore daoHandlerCore;
extern DaoTypeCore daoServerCore;
extern DaoTypeCore daoClientCore;


struct DaoxServer
{
	DAO_CSTRUCT_COMMON;

	mg_context  *context;

	DaoVmSpace *vmspace;
	DaoProcess *process;
	DaoVmCode  *sectCode;
	int         entryIndex;

	DString  *docroot;
	DString  *logfile;

	DList  *indexFiles;
	DMap   *staticMimes;

	DMap   *cookieToSessions;
	DMap   *timestampToSessions;

	DList  *freeSessions;
	DList  *freeHandlers;
	DList  *allSessions;
	DList  *allHandlers;

	DaoType *typeRequest;
	DaoType *typeResponse;
	DaoType *typeSession;
	DaoType *typeCache;
	DaoType *typeHandler;
	DaoType *typeClient;

	DaoType *typeKeyValue;
	DaoType *typeNameStream;
	DaoType *typeFileMap;
	DaoType *typeStringList;
	DaoType *typeFileCache;

	DMutex   mutex;
	daoint   sessionIndex;
};


/*==============================================================
// HTTP Request:
//============================================================*/
struct DaoxRequest
{
	DAO_CSTRUCT_COMMON;

	DaoxServer  *server;
	DaoxHandler *handler;

	DaoMap   *http_get;    // <string,string>;
	DaoMap   *http_post;   // <string,string>;
	DaoMap   *http_cookie; // <string,string>;
	DaoMap   *http_file;   // <string,stream>;
	DaoList  *http_uri;    // <string>

	DaoString  *key;    // Buffer string;
	DaoString  *value;  // Buffer string;

	DString  *uri;
	DString  *host;
	DString  *method;
	daoint    remote_ip;
	daoint    remote_port;
};

DaoxRequest* DaoxRequest_New( DaoxServer *server, DaoxHandler *handler )
{
	DaoxRequest *self = (DaoxRequest*) dao_calloc( 1, sizeof(DaoxRequest) );
	DaoCstruct_Init( (DaoCstruct*) self, server->typeRequest );
	self->server = server;
	self->handler = handler;
	self->uri = DString_New();
	self->host = DString_New();
	self->method = DString_New();
	self->key = DaoString_New(1);
	self->value = DaoString_New(1);
	self->http_get    = DaoMap_New( 1 + (size_t)rand() );  /* Random hash seed; */
	self->http_post   = DaoMap_New( 1 + (size_t)rand() );  /* To prevent hash DoS attacks; */
	self->http_cookie = DaoMap_New( 1 + (size_t)rand() );
	self->http_file   = DaoMap_New( 1 + (size_t)rand() );
	self->http_uri    = DaoList_New();
	DString_SetSharing( self->key->value, 0 );
	DString_SetSharing( self->value->value, 0 );
	DaoGC_IncRC( (DaoValue*) self->key );
	DaoGC_IncRC( (DaoValue*) self->value );
	DaoGC_IncRC( (DaoValue*) self->http_get );
	DaoGC_IncRC( (DaoValue*) self->http_post );
	DaoGC_IncRC( (DaoValue*) self->http_cookie );
	DaoGC_IncRC( (DaoValue*) self->http_file );
	DaoGC_IncRC( (DaoValue*) self->http_uri );
	DaoMap_SetType( self->http_get, server->typeKeyValue );
	DaoMap_SetType( self->http_post, server->typeKeyValue );
	DaoMap_SetType( self->http_cookie, server->typeKeyValue );
	DaoMap_SetType( self->http_file, server->typeFileMap );
	DaoList_SetType( self->http_uri, server->typeStringList );
	return self;
}

static void DaoxRequest_Delete( DaoxRequest *self )
{
	DString_Delete( self->uri );
	DString_Delete( self->host );
	DString_Delete( self->method );
	DaoGC_DecRC( (DaoValue*) self->key );
	DaoGC_DecRC( (DaoValue*) self->value );
	DaoGC_DecRC( (DaoValue*) self->http_get );
	DaoGC_DecRC( (DaoValue*) self->http_post );
	DaoGC_DecRC( (DaoValue*) self->http_cookie );
	DaoGC_DecRC( (DaoValue*) self->http_file );
	DaoGC_DecRC( (DaoValue*) self->http_uri );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}

static void DaoxRequest_Reset( DaoxRequest *self )
{
	DString_Reset( self->uri, 0 );
	DString_Reset( self->host, 0 );
	DString_Reset( self->method, 0 );
	DaoMap_Reset( self->http_get, 1 + (size_t)rand() );
	DaoMap_Reset( self->http_post, 1 + (size_t)rand() );
	DaoMap_Reset( self->http_cookie, 1 + (size_t)rand() );
	DaoMap_Reset( self->http_file, 1 + (size_t)rand() );
	DaoList_Clear( self->http_uri );
}

DString* DaoxRequest_FindVariable( DaoxRequest *self, const char *name, DaoMap *vars )
{
	DaoValue *value = DaoMap_GetValueChars( vars, name );
	if( value ) return value->xString.value;
	return NULL;
}
DString* DaoxRequest_FindCookie( DaoxRequest *self, const char *name )
{
	return DaoxRequest_FindVariable( self, name, self->http_cookie );
}

void DaoxRequest_SetURI( DaoxRequest *self, const char *uri )
{
	DString *part = DaoString_Get( self->key );
	daoint offset = 0;

	DString_Reset( self->uri, 0 );
	DString_AppendChars( self->uri, uri );

	DaoList_Clear( self->http_uri );
	while( offset < self->uri->size ){
		daoint pos = DString_FindChar( self->uri, '/', offset + 1 );
		if( pos == DAO_NULLPOS ) pos = self->uri->size;
		DString_SetBytes( part, self->uri->chars + offset + 1, pos - offset - 1 );
		DaoList_Append( self->http_uri, (DaoValue*) self->key );
		offset = pos;
	}
} 

#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

void DaoxRequest_ParseQueryString( DaoxRequest *self, DaoMap *vars, const char *data )
{
	const char *end = data + strlen( data );
	DString *key = DaoString_Get( self->key );
	DString *value = DaoString_Get( self->value );
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
			DaoMap_Insert( vars, (DaoValue*) self->key, (DaoValue*) self->value );
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
	if( key->size ) DaoMap_Insert( vars, (DaoValue*) self->key, (DaoValue*) self->value );
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

void DaoxRequest_ParsePostData( DaoxRequest *self, mg_connection *conn )
{
	DString *fname;
	DString *buffer = DString_New();
	DString *key = DaoString_Get( self->key );
	DString *value = DaoString_Get( self->value );
	const char *content_type = mg_get_header( conn, "Content-Type" );
	const char *boundary;
	char postbuf[1024];
	daoint postlen, boundarylen;
	daoint pos, pos2, pos_rnrn, offset;

	DString_SetSharing( buffer, 0 );
	if( content_type == NULL || strstr( content_type, "multipart/form-data" ) == NULL ){
		postlen = mg_read( conn, postbuf, sizeof(postbuf) );
		while( postlen > 0 ){
			DString_AppendBytes( buffer, postbuf, postlen );
			postlen = mg_read( conn, postbuf, sizeof(postbuf) );
		}
		DaoxRequest_ParseQueryString( self, self->http_post, buffer->chars );
		DString_Delete( buffer );
		return;
	}
	boundary = strstr( content_type, "boundary=" );
	if( boundary == NULL ){
		DString_Delete( buffer );
		return;
	}
	boundary += strlen( "boundary=" );
	boundarylen = strlen( boundary );

	fname = DString_New();
	buffer->size = 0;
	for(;;){
		postlen = mg_read( conn, postbuf, sizeof(postbuf) );
		if( postlen <= 0 && buffer->size < boundarylen ) break;

		DString_AppendBytes( buffer, postbuf, postlen );
		while( strstr( buffer->chars, "\r\n\r\n" ) == 0 && postlen > 0 ){
			postlen = mg_read( conn, postbuf, sizeof(postbuf) );
			if( postlen > 0 ) DString_AppendBytes( buffer, postbuf, postlen );
		}
		//printf( "###############\n%s\n", buffer->chars );

		key->size = 0;
		fname->size = 0;
		pos = DString_FindChars( buffer, "name=", 20 ); /* Skip: Content-Disposition: ; */
		if( pos != DAO_NULLPOS ){
			pos2 = DString_FindChar( buffer, '\"', pos+6 );
			if( pos2 == DAO_NULLPOS ){
				pos2 = pos + 5;
			}else{
				DString_SubString( buffer, key, pos + 6, pos2 - pos - 6 );
			}

			pos_rnrn = DString_FindChars( buffer, "\r\n\r\n", pos2 );
			pos = DString_FindChars( buffer, "filename=", pos2 );
			if( pos != DAO_NULLPOS && pos < pos_rnrn ){
				daoint pos3 = DString_FindChar( buffer, '\"', pos+10 );
				DString_SubString( buffer, fname, pos + 10, pos3 - pos - 10 );
			}

			buffer->size -= pos_rnrn + 4;
			memmove( buffer->chars, buffer->chars + pos_rnrn + 4, buffer->size );
		}
		if( fname->size == 0 ){
			offset = 0;
			while( (pos2 = DString_FindChars( buffer, boundary, offset )) == DAO_NULLPOS ){
				offset = buffer->size - boundarylen;
				if( offset < 0 ) offset = 0;
				postlen = mg_read( conn, postbuf, sizeof(postbuf) );
				if( postlen > 0 ) DString_AppendBytes( buffer, postbuf, postlen );
			}
			DString_SubString( buffer, value, 0, pos2 - 4 ); /* \r\n-- */
			DaoMap_Insert( self->http_post, (DaoValue*) self->key, (DaoValue*) self->value );
			buffer->size -= pos2 + boundarylen;
			memmove( buffer->chars, buffer->chars + pos2 + boundarylen, buffer->size );
		}else{
			DaoInteger isize = {DAO_INTEGER,0,0,0,0,0};
			DaoFileStream *stream = _DaoFileStream_New( self->server->vmspace );
			DaoTuple *tuple = DaoTuple_New(3);
			FILE *file = tmpfile();

			DaoString_Set( self->value, fname );
			stream->file = file;
			stream->base.mode |= DAO_STREAM_READABLE|DAO_STREAM_WRITABLE;
			_DaoFileStream_InitCallbacks( stream );
			DaoTuple_SetType( tuple, self->server->typeNameStream );
			DaoTuple_SetItem( tuple, (DaoValue*) self->value, 0 );
			DaoTuple_SetItem( tuple, (DaoValue*) stream, 2 );
			DaoMap_Insert( self->http_file, (DaoValue*) self->key, (DaoValue*) tuple );

			offset = 0;
			while( (pos2 = DString_FindChars( buffer, boundary, 0 )) == DAO_NULLPOS ){
				offset = buffer->size - boundarylen;
				if( offset > 0 ){
					isize.value += offset;
					fwrite( buffer->chars, 1, offset, file );
					buffer->size -= offset;
					memmove( buffer->chars, buffer->chars + offset, buffer->size );
				}
				postlen = mg_read( conn, postbuf, sizeof(postbuf) );
				if( postlen > 0 ) DString_AppendBytes( buffer, postbuf, postlen );
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



/*==============================================================
// HTTP Response:
//============================================================*/
struct DaoxResponse
{
	DAO_CSTRUCT_COMMON;

	DaoxHandler *handler;

	DMap  *cookies;
	int    responded;

	mg_connection *connection;
};

DaoxResponse* DaoxResponse_New( DaoxServer *server, DaoxHandler *handler )
{
	DaoxResponse *self = (DaoxResponse*) dao_calloc( 1, sizeof(DaoxResponse) );
	DaoCstruct_Init( (DaoCstruct*) self, server->typeResponse );
	self->cookies = DHash_New( DAO_DATA_STRING, DAO_DATA_STRING );
	self->handler = handler;
	self->responded = 0;
	return self;
}

static void DaoxResponse_Delete( DaoxResponse *self )
{
	DMap_Delete( self->cookies );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}

static void DaoxResponse_Reset( DaoxResponse *self )
{
	self->responded = 0;
	self->cookies->hashing = 1 + (size_t) rand();
	DMap_Reset( self->cookies );
}


const char *const week_days[] =
{ "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" };

const char *const months[] =
{ "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

static void DString_CookieTime( DString *self, time_t expire, int append )
{
	char buffer[128];
	struct tm *ctime;
	ctime = gmtime( & expire );
	sprintf( buffer, "%s,%i-%s-%i %02i:%02i:%02i GMT",
		   week_days[ctime->tm_wday],
		   ctime->tm_mday,
		   months[ctime->tm_mon],
		   ctime->tm_year+1900,
		   ctime->tm_hour,
		   ctime->tm_min,
		   ctime->tm_sec );
	if( append == 0 ) DString_Reset( self, 0 );
	DString_AppendChars( self, buffer );
}

const char alnumChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static void DString_Random( DString *self, int len, int alnum, int append )
{
	if( append == 0 ) DString_Reset( self, 0 );
	if( alnum ){
		while( len-- )
			DString_AppendChar( self, alnumChars[ (int)(62 * (rand()/(RAND_MAX+1.0)) ) ] );
	}else{
		while( len-- )
			DString_AppendChar( self, (char)(255 * (rand()/(RAND_MAX+1.0))) );
	}
}

void DaoxResponse_SetCookie( DaoxResponse *self, const char *name, const char *value, const char *path, int expire )
{
	DString *cookie;
	DString sname = DString_WrapChars( name );
	DNode *it = DMap_Insert( self->cookies, & sname, & sname );

	cookie = it->value.pString;
	DString_AppendChars( cookie, "=" );
	DString_AppendChars( cookie, value );
	DString_AppendChars( cookie, ";expires=" );
	DString_CookieTime( cookie, time(NULL) + expire, 1 );
	DString_AppendChars( cookie, ";path=" );
	DString_AppendChars( cookie, path );
}




/*==============================================================
// HTTP Session:
//============================================================*/
struct DaoxSession
{
	DAO_CSTRUCT_COMMON;

	DMap       *variables;
	DString    *cookie;
	DaoComplex  timestamp;  /* (time,index); */
	DMutex      mutex;
	daoint      expire;
};

DaoxSession* DaoxSession_New( DaoxServer *server )
{
	DaoComplex com = {DAO_COMPLEX,0,0,0,1,{0.0,0.0}};
	DaoxSession *self = (DaoxSession*) dao_calloc( 1, sizeof(DaoxSession) );
	DaoCstruct_Init( (DaoCstruct*) self, server->typeSession );
	self->variables = DHash_New( DAO_DATA_STRING, DAO_DATA_VALUE );
	self->cookie = DString_New();
	self->timestamp = com;
	self->expire = 5;
	DString_Random( self->cookie, 32, 1, 0 );
	DMutex_Init( & self->mutex );
	return self;
}

static void DaoxSession_Delete( DaoxSession *self )
{
	DMutex_Destroy( & self->mutex );
	DString_Delete( self->cookie );
	DMap_Delete( self->variables );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
static void DaoxSession_HandleGC( DaoValue *p, DList *vs, DList *as, DList *maps, int rm )
{
	DaoxSession *self = (DaoxSession*) p;
	DList_Append( maps, self->variables );
}
static void DaoxSession_Reset( DaoxSession *self )
{
	self->expire = 5;
	DString_Random( self->cookie, 32, 1, 0 );
	DMap_Clear( self->variables );
}




/*==============================================================
// HTTP Cache:
//============================================================*/
struct DaoxCache
{
	DAO_CSTRUCT_COMMON;

	DMap  *variables;
};

DaoxCache* DaoxCache_New( DaoxServer *server )
{
	DaoxCache *self = (DaoxCache*) dao_calloc( 1, sizeof(DaoxCache) );
	DaoCstruct_Init( (DaoCstruct*) self, server->typeCache );
	self->variables = DHash_New( DAO_DATA_STRING, DAO_DATA_VALUE );
	return self;
}

static void DaoxCache_Delete( DaoxCache *self )
{
	DMap_Delete( self->variables );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
static void DaoxCache_HandleGC( DaoValue *p, DList *vs, DList *as, DList *maps, int rm )
{
	DaoxCache *self = (DaoxCache*) p;
	DList_Append( maps, self->variables );
}
static void DaoxCache_Reset( DaoxCache *self )
{
	DMap_Clear( self->variables );
}




/*==============================================================
// HTTP Handler:
//============================================================*/
struct DaoxHandler
{
	DAO_CSTRUCT_COMMON;

	DaoxRequest   *request;
	DaoxResponse  *response;
	DaoxCache     *cache;
	DaoProcess    *process;
	DHash         *paths;
	DHash         *files;
	DString       *buffer;
	DString       *urlBuffer;
	DString       *mimeBuffer;
	DString       *pathBuffer;
};

DaoxHandler* DaoxHandler_New( DaoxServer *server )
{
	DaoxHandler *self = (DaoxHandler*) dao_calloc( 1, sizeof(DaoxHandler) );
	DaoCstruct_Init( (DaoCstruct*) self, server->typeHandler );
	self->request = DaoxRequest_New( server, self );
	self->response = DaoxResponse_New( server, self );
	self->cache = DaoxCache_New( server );
	self->process = DaoProcess_New( server->vmspace );
	self->paths = DHash_New(DAO_DATA_STRING,DAO_DATA_STRING);
	self->files = DHash_New(DAO_DATA_STRING,DAO_DATA_VALUE);
	self->buffer = DString_New();
	self->urlBuffer = DString_New();
	self->mimeBuffer = DString_New();
	self->pathBuffer = DString_New();
	GC_IncRC( self->request );
	GC_IncRC( self->response );
	GC_IncRC( self->cache );
	return self;
}

static void DaoxHandler_Delete( DaoxHandler *self )
{
	GC_DecRC( self->request );
	GC_DecRC( self->response );
	GC_DecRC( self->cache );
	GC_DecRC( self->process );
	DMap_Delete( self->paths );
	DMap_Delete( self->files );
	DString_Delete( self->buffer );
	DString_Delete( self->urlBuffer );
	DString_Delete( self->mimeBuffer );
	DString_Delete( self->pathBuffer );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}

static void DaoxHandler_HandleGC( DaoValue *p, DList *vs, DList *ls, DList *ms, int rm )
{
	DaoxHandler *self = (DaoxHandler*) p;
	DList_Append( vs, self->request );
	DList_Append( vs, self->response );
	DList_Append( vs, self->cache );
	DList_Append( vs, self->process );
	DList_Append( ms, self->files );
	if( rm ){
		self->request = NULL;
		self->response = NULL;
		self->cache = NULL;
		self->process = NULL;
	}
}

static void DaoxHandler_Reset( DaoxHandler *self )
{
	DaoxRequest_Reset( self->request );
	DaoxResponse_Reset( self->response );
	DaoProcess_Reset( self->process );
}





/*==============================================================
// HTTP Server:
//============================================================*/

static void DaoxServer_InitMimeTypes( DaoxServer *self );
static void DaoxServer_InitIndexFiles( DaoxServer *self );

DaoxServer* DaoxServer_New( DaoVmSpace *vmspace )
{
	DaoNamespace *httpns = DaoVmSpace_GetNamespace( vmspace, "http" );
	DaoType *type = DaoVmSpace_GetType( vmspace, & daoServerCore );

	DaoxServer *self = (DaoxServer*) dao_calloc( 1, sizeof(DaoxServer) );
	DaoCstruct_Init( (DaoCstruct*) self, type );
	self->vmspace = vmspace;
	self->docroot = DString_New();
	self->logfile = DString_New();
	self->indexFiles = DList_New( DAO_DATA_STRING );
	self->staticMimes = DHash_New( DAO_DATA_STRING, DAO_DATA_STRING );
	self->cookieToSessions = DHash_New( DAO_DATA_STRING, 0 );   // <DString*,DaoxSession*>
	self->timestampToSessions = DMap_New( DAO_DATA_VALUE, 0 ); // <DaoComplex,DaoxSession*>
	self->freeSessions = DList_New(0); // <DaoxSessions*>
	self->freeHandlers = DList_New(0); // <DaoxHandlers*>
	self->allSessions  = DList_New( DAO_DATA_VALUE ); // <DaoxSession*>
	self->allHandlers  = DList_New( DAO_DATA_VALUE ); // <DaoxHandler*>
	DString_SetChars( self->docroot, vmspace->startPath->chars );
	DaoxServer_InitMimeTypes( self );
	DaoxServer_InitIndexFiles( self );
	DMutex_Init( & self->mutex );

	self->typeRequest = DaoVmSpace_GetType( vmspace, & daoRequestCore );
	self->typeResponse = DaoVmSpace_GetType( vmspace, & daoResponseCore );
	self->typeSession = DaoVmSpace_GetType( vmspace, & daoSessionCore );
	self->typeCache = DaoVmSpace_GetType( vmspace, & daoCacheCore );
	self->typeHandler = DaoVmSpace_GetType( vmspace, & daoHandlerCore );
	self->typeClient = DaoVmSpace_GetType( vmspace, & daoClientCore );

	self->typeKeyValue = DaoNamespace_ParseType( httpns, "map<string,string>" );
	self->typeNameStream = DaoNamespace_FindTypeChars( httpns, "HttpUpload" );
	self->typeFileMap = DaoNamespace_ParseType( httpns, "map<string,HttpUpload>" );
	self->typeStringList = DaoNamespace_ParseType( httpns, "list<string>" );
	self->typeFileCache = DaoNamespace_ParseType( httpns, "tuple<file:string,time:float>");
	return self;
}

static void DaoxServer_Delete( DaoxServer *self )
{
	DString_Delete( self->docroot );
	DString_Delete( self->logfile );
	DList_Delete( self->indexFiles );
	DMap_Delete( self->staticMimes );
	DMap_Delete( self->cookieToSessions );
	DMap_Delete( self->timestampToSessions );
	DList_Delete( self->freeSessions );
	DList_Delete( self->freeHandlers );
	DList_Delete( self->allSessions );
	DList_Delete( self->allHandlers );
	DMutex_Destroy( & self->mutex );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
static void DaoxServer_HandleGC( DaoValue *p, DList *vs, DList *arrays, DList *maps, int rm )
{
	DaoxServer *self = (DaoxServer*) p;
	DList_Append( arrays, self->allSessions );
	DList_Append( arrays, self->allHandlers );
}

#if 0
/* In order to keep process auxiliary data such as states of random number generator: */
void DaoxServer_ReleaseProcess( DaoxServer *self, DaoProcess *proc )
{
	DMutex_Lock( & self->mutex2 );
	if( proc->factory ) DList_Clear( proc->factory );
	GC_DecRC( proc->future );
	proc->future = NULL;
	DaoProcess_PopFrames( proc, proc->firstFrame );
	DList_Clear( proc->exceptions );
	DList_Append( self->processes, proc );
	DMutex_Unlock( & self->mutex2 );
}
#endif


static int reused = 0;
DaoxSession* DaoxServer_MakeSession( DaoxServer *self, mg_connection *conn, DaoxRequest *request )
{
	DNode *it;
	DaoxSession *session = NULL;
	DString *session_cookie = DaoxRequest_FindCookie( request, "WebdaoSession" );
	DaoComplex com = {DAO_COMPLEX,0,0,0,1,{0.0,0.0}};
	time_t now = time(NULL);
	daoint i, offset;

	DMutex_Lock( & self->mutex );
	offset = self->freeSessions->size;
	it = DMap_First( self->timestampToSessions );
	for(; it; it=DMap_Next( self->timestampToSessions, it )){
		DaoxSession *ss = (DaoxSession*) it->value.pValue;
		if( ss->timestamp.value.real > now ) break;
		DMap_Erase( self->cookieToSessions, ss->cookie );
		DList_Append( self->freeSessions, ss );
	}
	for(i=offset; i<self->freeSessions->size; ++i){
		DaoxSession *ss = (DaoxSession*) self->freeSessions->items.pVoid[i];
		DMap_Erase( self->timestampToSessions, & ss->timestamp );
	}
	DMutex_Unlock( & self->mutex );

	if( session_cookie != NULL ){
		DMutex_Lock( & self->mutex );
		it = DMap_Find( self->cookieToSessions, session_cookie );
		if( it ){
			session = (DaoxSession*) it->value.pValue;
			DMap_Erase( self->timestampToSessions, (void*) & session->timestamp );
			if( session->timestamp.value.real < now ){
				DMap_Erase( self->cookieToSessions, session_cookie );
				DaoxSession_Reset( session );
				DMap_Insert( self->cookieToSessions, session->cookie, session );
			}
		}
		DMutex_Unlock( & self->mutex );
	}
	if( session == NULL && self->freeSessions->size ){
		/*
		// Try to reuse expired session object:
		*/
		DMutex_Lock( & self->mutex );
		session = (DaoxSession*) DList_PopBack( self->freeSessions );
		DaoxSession_Reset( session );
		DMap_Insert( self->cookieToSessions, session->cookie, session );
		reused += 1;
		DMutex_Unlock( & self->mutex );
	}
	if( session == NULL ){
		session = DaoxSession_New( self );
		DMutex_Lock( & self->mutex );
		DList_Append( self->allSessions, session );
		DMap_Insert( self->cookieToSessions, session->cookie, session );
		DMutex_Unlock( & self->mutex );
	}
	session->timestamp.value.real = now + session->expire;

#ifdef DEBUG
	if( self->sessionIndex % 100 == 0 ){
		int n1 = self->allSessions->size;
		int n2 = self->cookieToSessions->size;
		int n3 = self->timestampToSessions->size;
		int n4 = self->freeSessions->size;
		printf( "reused = %i %i %i %i %i\n", reused, n1, n2, n3, n4 );
		//if( self->sessionIndex > 0 && self->sessionIndex % 1000 == 0 ) free(123);
	}
#endif
	return session;
}

DaoxHandler* DaoxServer_MakeHandler( DaoxServer *self, mg_connection *conn )
{
    char host[1025];
	const mg_request_info *ri = mg_get_request_info( conn );
	const char *cookie = mg_get_header( conn, "Cookie" );
	const char *query = ri->query_string;
	DaoxHandler *handler = NULL;
	DaoxRequest *request = NULL;
	DaoxResponse *response = NULL;
	DaoxCache *cache = NULL;

	if( self->freeHandlers->size ){
		DMutex_Lock( & self->mutex );
		if( self->freeHandlers->size ){
			handler = (DaoxHandler*) DList_PopFront( self->freeHandlers );
			DaoxHandler_Reset( handler );
		}
		DMutex_Unlock( & self->mutex );
	}

	if( handler == NULL ){
		handler = DaoxHandler_New( self );
		DMutex_Lock( & self->mutex );
		DList_Append( self->allHandlers, handler );
		DMutex_Unlock( & self->mutex );
		//printf( "DaoxServer_MakeHandler: %ti\n", self->allHandlers->size );
	}

	request = handler->request;
	response = handler->response;
	cache = handler->cache;

	response->connection = conn;

	request->remote_ip = ri->remote_ip;
	request->remote_port = ri->remote_port;

	mg_get_hostname( conn, host, sizeof(host) );
	DString_SetChars( request->host, host );
	DString_SetChars( request->method, ri->request_method );

	DaoxRequest_SetURI( request, ri->uri );
	if( cookie ) DaoxRequest_ParseQueryString( request, request->http_cookie, cookie );
	if( query ) DaoxRequest_ParseQueryString( request, request->http_get, query );
	if( !strcmp( ri->request_method, "POST" ) ) DaoxRequest_ParsePostData( request, conn );
	return handler;
}

static void DaoxServer_CacheHandler( DaoxServer *self, DaoxHandler *handler, DaoxSession *session )
{
	DMutex_Lock( & self->mutex );
	if( handler != NULL ) DList_Append( self->freeHandlers, handler );
	if( session != NULL ){
		session->timestamp.value.imag = ++ self->sessionIndex;
		DMap_Insert( self->timestampToSessions, (void*) & session->timestamp, session );
	}
	DMutex_Unlock( & self->mutex );
}



typedef struct DaoxStaticMime DaoxStaticMime;

struct DaoxStaticMime
{
	const char *extension;
	const char *mime_type;
};

static const DaoxStaticMime daox_static_mimes[] =
{
	{".html", "text/html"},
	{".htm", "text/html"},
	{".shtm", "text/html"},
	{".shtml", "text/html"},
	{".css", "text/css"},
	{".js", "application/x-javascript"},
	{".ico", "image/x-icon"},
	{".gif", "image/gif"},
	{".jpg", "image/jpeg"},
	{".jpeg", "image/jpeg"},
	{".png", "image/png"},
	{".svg", "image/svg+xml"},
	{".txt", "text/plain"},
	{".torrent", "application/x-bittorrent"},
	{".wav", "audio/x-wav"},
	{".mp3", "audio/x-mp3"},
	{".mid", "audio/mid"},
	{".m3u", "audio/x-mpegurl"},
	{".ogg", "audio/ogg"},
	{".ram", "audio/x-pn-realaudio"},
	{".xml", "text/xml"},
	{".json", "text/json"},
	{".xslt", "application/xml"},
	{".xsl", "application/xml"},
	{".ra", "audio/x-pn-realaudio"},
	{".doc", "application/msword"},
	{".exe", "application/octet-stream"},
	{".zip", "application/x-zip-compressed"},
	{".xls", "application/excel"},
	{".tgz", "application/x-tar-gz"},
	{".tar", "application/x-tar"},
	{".gz", "application/x-gunzip"},
	{".arj", "application/x-arj-compressed"},
	{".rar", "application/x-arj-compressed"},
	{".rtf", "application/rtf"},
	{".pdf", "application/pdf"},
	{".swf", "application/x-shockwave-flash"},
	{".mpg", "video/mpeg"},
	{".webm", "video/webm"},
	{".mpeg", "video/mpeg"},
	{".mov", "video/quicktime"},
	{".mp4", "video/mp4"},
	{".m4v", "video/x-m4v"},
	{".asf", "video/x-ms-asf"},
	{".avi", "video/x-msvideo"},
	{".bmp", "image/bmp"},
	{".ttf", "application/x-font-ttf"},
	{NULL,  NULL}
};

static void DaoxServer_InitMimeTypes( DaoxServer *self )
{
	int i = -1;
	while( daox_static_mimes[++i].extension != NULL ){
		DString key = DString_WrapChars( daox_static_mimes[i].extension );
		DString value = DString_WrapChars( daox_static_mimes[i].mime_type );
		DMap_Insert( self->staticMimes, & key, & value );
	}
}

static const DaoxStaticMime daox_index_mimes[] =
{
	{"index.html", "text/html"},
	{"index.htm", "text/html"},
	{"index.shtm", "text/html"},
	{"index.shtml", "text/html"},
	{NULL,  NULL}
};

static void DaoxServer_InitIndexFiles( DaoxServer *self )
{
	int i = -1;
	while( daox_index_mimes[++i].extension != NULL ){
		DString key = DString_WrapChars( daox_index_mimes[i].extension );
		DString value = DString_WrapChars( daox_static_mimes[i].mime_type );
		DList_Append( self->indexFiles, & key );
		DList_Append( self->indexFiles, & value );
	}
}



/*==============================================================
// Dao Interfaces:
//============================================================*/
static void REQ_GETF_Host( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	DaoProcess_PutString( proc, request->host );
}
static void REQ_GETF_Method( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	DaoProcess_PutString( proc, request->method );
}
static void REQ_GETF_URI( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	DaoProcess_PutString( proc, request->uri );
}
static void REQ_SETF_URI( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	DString *uri = DaoValue_TryGetString( p[1] );
	DaoxRequest_SetURI( request, uri->chars );
}
static void REQ_GETF_HttpURI( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	void *pp = DaoProcess_PutValue( proc, (DaoValue*) request->http_uri );
}
static void REQ_GETF_HttpGet( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	DaoProcess_PutValue( proc, (DaoValue*) request->http_get );
}
static void REQ_SETF_HttpGet( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	GC_Assign( & request->http_get, p[1] );
}
static void REQ_GETF_HttpPost( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	DaoProcess_PutValue( proc, (DaoValue*) request->http_post );
}
static void REQ_SETF_HttpPost( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	GC_Assign( & request->http_post, p[1] );
}
static void REQ_GETF_HttpCookie( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	DaoProcess_PutValue( proc, (DaoValue*) request->http_cookie );
}
static void REQ_SETF_HttpCookie( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	GC_Assign( & request->http_cookie, p[1] );
}
static void REQ_GETF_HttpFile( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	void *pp = DaoProcess_PutValue( proc, (DaoValue*) request->http_file );
}
static void REQ_SETF_HttpFile( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	GC_Assign( & request->http_file, p[1] );
}

static DaoFunctionEntry daoRequestMeths[] =
{
	{ REQ_GETF_Host,       ".Host( self: Request ) => string" },
	{ REQ_GETF_Method,     ".Method( self: Request ) => string" },
	{ REQ_GETF_URI,        ".URI( self: Request ) => string" },
	{ REQ_SETF_URI,        ".URI=( self: Request, uri: string )" },
	{ REQ_GETF_HttpURI,    ".HttpURI( self: Request ) => list<string>" },
	{ REQ_GETF_HttpGet,    ".HttpGet( self: Request ) => map<string,string>" },
	{ REQ_GETF_HttpPost,   ".HttpPost( self: Request ) => map<string,string>" },
	{ REQ_GETF_HttpCookie, ".HttpCookie( self: Request ) => map<string,string>" },
	{ REQ_GETF_HttpFile,   ".HttpFile( self: Request ) => map<string,HttpUpload>" },
	{ REQ_SETF_HttpGet,    ".HttpGet=( self: Request, value: map<string,string> )" },
	{ REQ_SETF_HttpPost,   ".HttpPost=( self: Request, value: map<string,string> )" },
	{ REQ_SETF_HttpCookie, ".HttpCookie=( self: Request, value: map<string,string> )" },
	{ REQ_SETF_HttpFile,   ".HttpFile=( self: Request, value: map<string,HttpUpload> )" },
	{ NULL, NULL }
};


DaoTypeCore daoRequestCore =
{
	"Request",                                         /* name */
	sizeof(DaoxRequest),                               /* size */
	{ NULL },                                          /* bases */
	{ NULL },                                          /* casts */
	NULL,                                              /* numbers */
	daoRequestMeths,                                   /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	DaoCstruct_CheckSetField,  DaoCstruct_DoSetField,  /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxRequest_Delete,            /* Delete */
	NULL                                               /* HandleGC */
};



static void RES_SetCookie( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxResponse *response = (DaoxResponse*) p[0];
	DString *name = p[1]->xString.value;
	DString *value = p[2]->xString.value;
	DString *path = p[3]->xString.value;
	int expire = p[4]->xInteger.value;
	DaoxResponse_SetCookie( response, name->chars, value->chars, path->chars, expire );
}
static void RES_WriteHeader( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxResponse *response = (DaoxResponse*) p[0];
	DaoMap *headers = (DaoMap*) p[2];
	int status = p[1]->xInteger.value;
	DNode *it;

	response->responded = 1;
	mg_printf( response->connection, "HTTP/1.1 %i %s\r\n", status, "OK" /* TODO */ );
	for(it=DaoMap_First(headers); it; it=DaoMap_Next(headers,it)){
		const char *key = DaoString_GetChars( (DaoString*) it->key.pValue );
		const char *value = DaoString_GetChars( (DaoString*) it->value.pValue );
		mg_printf( response->connection, "%s: %s\r\n", key, value );
	}
	for(it=DMap_First(response->cookies); it; it=DMap_Next(response->cookies,it)){
		const char *value = DString_GetData( it->value.pString );
		mg_printf( response->connection, "Set-Cookie: %s\r\n", value );
	}
	mg_printf( response->connection, "\r\n" );
}
static void RES_Write( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxResponse *response = (DaoxResponse*) p[0];
	const char *text = DaoString_GetChars( (DaoString*) p[1] );
	mg_printf( response->connection, "%s", text );
}
static void RES_SendFile( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxResponse *response = (DaoxResponse*) p[0];
	const char *path = DaoString_GetChars( (DaoString*) p[1] );
	dao_boolean cache = p[2]->xBoolean.value;
	mg_send_file( response->connection, path );
	response->responded = 1;
	if( cache ){
		const mg_request_info *ri = mg_get_request_info( response->connection );
		DString *url = response->handler->urlBuffer;
		DString_SetChars( url, response->handler->request->host->chars );
		DString_AppendChars( url, ri->uri );
		//printf( "RES_SendFile: %s\n", url->chars );
		DMap_Insert( response->handler->paths, url, p[1]->xString.value );
	}
}
static void RES_RespondError( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxResponse *response = (DaoxResponse*) p[0];
	DaoString *message = (DaoString*) p[2];
	int code = p[1]->xInteger.value;
	DNode *it;

	response->responded = 1;
	mg_printf( response->connection, "HTTP/1.1 %i %s\r\n", code, "Internal Server Error" );
	mg_printf( response->connection, "Content-Type: text/html\r\n" );
	for(it=DMap_First(response->cookies); it; it=DMap_Next(response->cookies,it)){
		const char *value = DString_GetData( it->value.pString );
		mg_printf( response->connection, "Set-Cookie: %s\r\n", value );
	}
	mg_printf( response->connection, "\r\n" );
	mg_printf( response->connection, "<html><body><h3>Server Error:</h3><pre>\n" );
	mg_printf( response->connection, "%s\n", message->value->chars );
	mg_printf( response->connection, "</pre></body></html>" );
}

static DaoFunctionEntry daoResponseMeths[] =
{
	{ RES_SetCookie, "SetCookie( self: Response, name: string, value: string, path='', expire=3600 )" },
	{ RES_WriteHeader, "WriteHeader( self: Response, status = 200, headers: map<string,string> = {=>} )" },
	{ RES_Write,        "Write( self: Response, content: string )" },
	{ RES_SendFile,     "SendFile( self: Response, path: string, cachePath = true )" },
	{ RES_RespondError, "RespondError( self: Response, code: int, message: string )" },
	{ NULL, NULL }
};


DaoTypeCore daoResponseCore =
{
	"Response",                                        /* name */
	sizeof(DaoxResponse),                              /* size */
	{ NULL },                                          /* bases */
	{ NULL },                                          /* casts */
	NULL,                                              /* numbers */
	daoResponseMeths,                                  /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxResponse_Delete,           /* Delete */
	NULL                                               /* HandleGC */
};




static void SES_GetVar( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxSession *session = (DaoxSession*) p[0];
	DString *key = DaoValue_TryGetString( p[1] );
	DNode *it;

	DMutex_Lock( & session->mutex );
	it = DMap_Find( session->variables, key );
	if( it != NULL ) DaoProcess_PutValue( proc, it->value.pValue );
	DMutex_Unlock( & session->mutex );

	if( it == NULL ) DaoProcess_PutNone( proc );
}
static void SES_SetVar( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxSession *session = (DaoxSession*) p[0];
	DString *key = DaoValue_TryGetString( p[1] );
	DMutex_Lock( & session->mutex );
	DMap_Insert( session->variables, key, p[2] );
	DMutex_Unlock( & session->mutex );
}
static void SES_SetTimeout( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxSession *session = (DaoxSession*) p[0];
	session->expire = p[1]->xInteger.value;
	session->timestamp.value.real = time(NULL) + session->expire;
}

static DaoFunctionEntry daoSessionMeths[] =
{
	{ SES_GetVar,     "GetVariable( self: Session, name: string ) => any" },
	{ SES_SetVar,     "SetVariable( self: Session, name: string, value: any )" },
	{ SES_SetTimeout, "SetTimeout( self: Session, seconds: int )" },
	{ NULL, NULL }
};


DaoTypeCore daoSessionCore =
{
	"Session",                                         /* name */
	sizeof(DaoxSession),                               /* size */
	{ NULL },                                          /* bases */
	{ NULL },                                          /* casts */
	NULL,                                              /* numbers */
	daoSessionMeths,                                   /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxSession_Delete,            /* Delete */
	DaoxSession_HandleGC                               /* HandleGC */
};




static void CACHE_GetVar( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCache *cache = (DaoxCache*) p[0];
	DString *key = DaoValue_TryGetString( p[1] );
	DNode *it = DMap_Find( cache->variables, key );
	if( it != NULL ){
		DaoProcess_PutValue( proc, it->value.pValue );
	}else{
		DaoProcess_PutNone( proc );
	}
}
static void CACHE_SetVar( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCache *cache = (DaoxCache*) p[0];
	DString *key = DaoValue_TryGetString( p[1] );
	DMap_Insert( cache->variables, key, p[2] );
}

static DaoFunctionEntry daoCacheMeths[] =
{
	{ CACHE_GetVar,     "GetVariable( self: Cache, name: string ) => any" },
	{ CACHE_SetVar,     "SetVariable( self: Cache, name: string, value: any )" },
	{ NULL, NULL }
};


DaoTypeCore daoCacheCore =
{
	"Cache",                                           /* name */
	sizeof(DaoxCache),                                 /* size */
	{ NULL },                                          /* bases */
	{ NULL },                                          /* casts */
	NULL,                                              /* numbers */
	daoCacheMeths,                                     /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCache_Delete,              /* Delete */
	DaoxCache_HandleGC                                 /* HandleGC */
};





DaoTypeCore daoHandlerCore =
{
	"Handler",                                         /* name */
	sizeof(DaoxHandler),                               /* size */
	{ NULL },                                          /* bases */
	{ NULL },                                          /* casts */
	NULL,                                              /* numbers */
	NULL,                                              /* methods */
	NULL,                      NULL,                   /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxHandler_Delete,            /* Delete */
	DaoxHandler_HandleGC                               /* HandleGC */
};




static int DaoxHttp_TrySendFile( DaoxHandler *handler, mg_connection *conn )
{
	int sendfile = 0;
	const mg_request_info *ri = mg_get_request_info( conn );
	DaoxServer *server = (DaoxServer*) ri->user_data;
	DaoxRequest *request = handler->request;
	DaoxResponse *response = handler->response;
	DaoxSession *session = NULL;
	DString *url = handler->urlBuffer;
	DString *path = handler->pathBuffer;
	DString *mime = handler->mimeBuffer;
	DNode *it;

	DString_SetChars( url, request->host->chars );
	DString_AppendChars( url, ri->uri );

	it = DMap_Find( handler->paths, url );
	//printf( "DaoxHttp_TrySendFile: %s %p\n", url->chars, it );
	if( it != NULL ){
		//printf( "DaoxHttp_TrySendFile: %s %s\n", url->chars, it->value.pString->chars );
		if( it->value.pString->size ){
			mg_send_file( conn, it->value.pString->chars );
			return 1;
		}
		return 0;
	}

	DString_SetChars( path, ri->uri + 1 );
	DString_MakePath( server->docroot, path );
	if( Dao_IsFile( path->chars ) ){
		daoint pos = DString_RFindChar( path, '.', -1 );
		if( pos != DAO_NULLPOS ){
			DString ext = DString_WrapChars( path->chars + pos );
			it = DMap_Find( server->staticMimes, & ext );
			if( it != NULL ){
				sendfile = 1;
				DString_SetChars( mime, it->value.pString->chars );
			}
		}
	}else if( Dao_IsDir( path->chars ) && path->size && path->chars[path->size-1] != '/' ){
		mg_printf(conn, "HTTP/1.1 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", ri->uri);
		return 1;
	}else if( Dao_IsDir( path->chars ) ){
		DString *path2 = handler->buffer;
		daoint i;
		for(i=0; i<server->indexFiles->size; i+=2){
			DString_Assign( path2, server->indexFiles->items.pString[i] );
			DString_MakePath( path, path2 );
			if( Dao_IsFile( path2->chars ) ){
				DString_Assign( path, path2 );
				DString_Assign( mime, server->indexFiles->items.pString[i+1] );
				sendfile = 1;
				break;
			}
		}
	}
	if( sendfile == 0 ) DString_Clear( path );
	DMap_Insert( handler->paths, url, path );
	if( sendfile == 0 ) return 0;
	//printf( ">>>>>>>>>>>> path: %i %s\n", sendfile, path->chars );

	mg_send_file( conn, path->chars );

	return 1;
}

static int DaoxHttp_HandleRequest( mg_connection *conn )
{
	const mg_request_info *ri = mg_get_request_info( conn );
	DaoxServer *server = (DaoxServer*) ri->user_data;
	DaoxHandler *handler = DaoxServer_MakeHandler( server, conn );
	DaoVmCode *sectCode = server->sectCode;
	DaoProcess *caller = server->process;
	DaoProcess *process = handler->process;
	DaoxRequest *request = handler->request;
	DaoxResponse *response = handler->response;
	DaoxCache *cache = handler->cache;
	DaoxSession *session = NULL;
	DaoVmCode *sect = NULL;
	int ret;

	//printf( "DaoxHttp_HandleRequest: %p %s\n", handler, ri->uri );

	if( DaoxHttp_TrySendFile( handler, conn ) ){
		if( server->sectCode->b > 2 ){ /* Update session timestamp: */
			session = DaoxServer_MakeSession( server, conn, request );
		}
		DaoxServer_CacheHandler( server, handler, session );
		return 1;
	}

	if( sectCode->b > 2 ){
		session = DaoxServer_MakeSession( server, conn, request );
		DaoxResponse_SetCookie( response, "WebdaoSession", session->cookie->chars, "/", 12*3600 );
	}

	DaoProcess_PushRoutine( process, caller->activeRoutine, caller->activeObject );
	process->activeCode = caller->activeCode;
	DaoProcess_PushFunction( process, caller->topFrame->routine );
	DaoProcess_SetActiveFrame( process, process->topFrame->prev );
	sect = DaoProcess_InitCodeSection( process, 4 );
	if( sect == NULL ){
		DaoProcess_PrintException( process, NULL, 1 );
		DaoxServer_CacheHandler( server, handler, session );
		return 0;
	}

	process->topFrame->outer = caller;
	process->topFrame->host = caller->topFrame->prev;
	process->topFrame->returning = -1;
	process->topFrame->entry = server->entryIndex;

	if( sectCode->b >0 ) DaoProcess_SetValue( process, sectCode->a, (DaoValue*) request );
	if( sectCode->b >1 ) DaoProcess_SetValue( process, sectCode->a+1, (DaoValue*) response );
	if( sectCode->b >2 ) DaoProcess_SetValue( process, sectCode->a+2, (DaoValue*) session );
	if( sectCode->b >3 ) DaoProcess_SetValue( process, sectCode->a+3, (DaoValue*) cache );
	DaoProcess_Execute( process );

	ret = response->responded;

	DaoxServer_CacheHandler( server, handler, session );
	return ret;
}




static void SERVER_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxServer *self = DaoxServer_New( proc->vmSpace );
	DString *docroot = DaoValue_TryGetString( p[0] );
	if( docroot->size ) DString_Assign( self->docroot, docroot );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}

static void SERVER_GetDocRoot( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxServer *self = (DaoxServer*) p[0];
	DaoProcess_PutString( proc, self->docroot );
}

static void SERVER_SetDocRoot( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxServer *self = (DaoxServer*) p[0];
	DString *docroot = DaoValue_TryGetString( p[1] );
	DString_Assign( self->docroot, docroot );
	if( self->docroot->size && self->docroot->chars[self->docroot->size-1] != '/' ){
		DString_AppendChar( self->docroot, '/' );
	}
}

static void SERVER_GetLogFile( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxServer *self = (DaoxServer*) p[0];
	DaoProcess_PutString( proc, self->logfile );
}

static void SERVER_SetLogFile( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxServer *self = (DaoxServer*) p[0];
	DString *logfile = DaoValue_TryGetString( p[1] );
	DString_Assign( self->logfile, logfile );
}


#ifdef UNIX
#include <execinfo.h>
#endif

static DString *traceFile = NULL;

static void DaoStackTrace( FILE *fout )
{
#ifdef UNIX
	void *array[128];
	int size = backtrace(array, 128);
	char **strings = backtrace_symbols(array, size);
	time_t tm = time(NULL);
	int i;

	fprintf( fout, "\n\n>> %s\n", ctime( & tm ) );
	fprintf( fout, "ERROR: program failed with segmentation error!\n" );
	fprintf( fout, "The calling stack for the error:\n" );

	for(i = 0; i < size; ++i) fprintf( fout, "%s\n", strings[i] );
	free(strings);
#else
	fprintf( fout, "ERROR: program failed with segmentation error!\n" );
#endif
	fflush( fout );
}

static void DaoSignalHandler( int sig )
{
	FILE *fout = stdout;
	if( traceFile ) fout = fopen( traceFile->chars, "a+" );
	DaoStackTrace( fout );
	if( fout != stdout ) fclose( fout );
	exit( sig );
}

static void SERVER_Start( DaoProcess *proc, DaoValue *p[], int N )
{
	mg_context *ctx;
	mg_callbacks callbacks;
	DString *buffer = NULL;
	DaoxServer *self = (DaoxServer*) p[0];
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 4 );
	char port[16], numthd[16];
	const char *options[11] = {
		"document_root", ".",
		"listening_ports", "8080",
		"num_threads", "8",
		"access_log_file", "",
		"error_log_file", "",
		NULL
	};

	sprintf( port, "%i", (int) p[1]->xInteger.value );
	sprintf( numthd, "%i", (int) p[2]->xInteger.value );
	options[1] = self->docroot->chars;
	options[3] = port;
	options[5] = numthd;
	if( self->logfile->size ){
		options[7] = self->logfile->chars;
		options[9] = self->logfile->chars;
	}
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.begin_request = DaoxHttp_HandleRequest;

	if( sect == NULL ) return;

	DaoCGC_Start();

	self->process = proc;
	self->sectCode = sect;
	self->entryIndex = proc->topFrame->entry;
	DaoProcess_PopFrame( proc );

	ctx = mg_start( & callbacks, self, options );
	if( ctx == NULL ){
		DaoProcess_RaiseError( proc, NULL, "failed to start the server" );
		return;
	}

	if( self->logfile->size ){
		buffer = DString_Copy( self->logfile );
		DString_AppendChars( buffer, ".error" );
		traceFile = buffer;
	}
	signal( SIGSEGV, DaoSignalHandler );

	self->context = ctx;
	mg_wait( ctx );
	self->context = NULL;
	mg_quit( ctx );
	if( buffer ) DString_Delete( buffer );
	traceFile = NULL;
}
static void SERVER_Stop( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxServer *self = (DaoxServer*) p[0];
	if( self->context ) mg_stop( self->context );
}

static DaoFunctionEntry daoServerMeths[] =
{
	{ SERVER_New,        "Server( docroot = '' )" },
	{ SERVER_GetDocRoot, ".DocumentRoot( self: Server ) => string" },
	{ SERVER_SetDocRoot, ".DocumentRoot=( self: Server, docroot: string )" },
	{ SERVER_GetLogFile, ".LogFile( self: Server ) => string" },
	{ SERVER_SetLogFile, ".LogFile=( self: Server, logfile: string )" },
	{ SERVER_Start,
		"Start( self: Server, port = 80, numthread = 8 )"
			"[request: Request, response: Response, session: Session, cache: Cache]"
	},
	{ SERVER_Stop, "Stop( self: Server )" },
	{ NULL, NULL }
};


DaoTypeCore daoServerCore =
{
	"Server",                                          /* name */
	sizeof(DaoxServer),                                /* size */
	{ NULL },                                          /* bases */
	{ NULL },                                          /* casts */
	NULL,                                              /* numbers */
	daoServerMeths,                                    /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	DaoCstruct_CheckSetField,  DaoCstruct_DoSetField,  /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxServer_Delete,             /* Delete */
	DaoxServer_HandleGC                                /* HandleGC */
};


static void CLIENT_SendRequest( DaoProcess *proc, DaoValue *p[], DString *content, DString *contType )
{
	DaoString *data;
	DaoVmCode *sect = NULL;
	DaoStackFrame *frame = DaoProcess_FindSectionFrame( proc );
	DString *res = frame == NULL ? DaoProcess_PutChars( proc, "" ) : NULL;
	DString *url = p[0]->xString.value;
	DString *host = DString_Copy( url );
	DString *uri = DString_NewChars( "/" );
	DString *ports = DString_NewChars( "80" );
	long port;
	int entry, use_ssl = 0;

	struct mg_connection *conn;
	char postbuf[4*1024];
	char ebuf[1000];
	int postlen;
	int pos;
	char *end;

	if( DString_FindChars( host, "http://", 0 ) == 0 ){
		DString_Erase( host, 0, 7 );
	}else if( DString_FindChars( host, "https://", 0 ) == 0 ){
		DString_Erase( host, 0, 8 );
		use_ssl = 1;
	}
	pos = DString_FindChar( host, '/', 0 );
	if( pos != DAO_NULLPOS ){
		DString_SubString( host, uri, pos, -1 );
		DString_Erase( host, pos, -1 );
	}
	pos = DString_RFindChar( host, ':', -1 );
	if( pos != DAO_NULLPOS ){
		DString_SubString( host, ports, pos + 1, -1 );
		DString_Erase( host, pos, -1 );
	}
	port = strtol( ports->chars, &end, 10 );
	if ( *end != '\0' || port < 0 || port > 0xFFFF ){
		DaoProcess_RaiseError( proc, "Param", "Invalid port number" );
		return;
	}

	if ( content )
		conn = mg_download( host->chars, port, use_ssl, ebuf, sizeof(ebuf),
							"POST %s HTTP/1.1\r\nHost: %s\r\nContent-type:%s\r\nContent-length: %"DAO_INT"\r\n\r\n%s",
							uri->chars, host->chars, contType->chars, (ptrdiff_t)content->size, content->chars );
	else
		conn = mg_download( host->chars, port, use_ssl, ebuf, sizeof(ebuf),
							"GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", uri->chars, host->chars );

	DString_Delete( host );
	DString_Delete( uri );
	DString_Delete( ports );

	if( conn == NULL ){
		DaoProcess_RaiseError( proc, "Http", ebuf );
		return;
	}

	if( frame ){
		sect = DaoProcess_InitCodeSection( proc, 1 );
		if( sect == NULL ){
			mg_close_connection( conn );
			return;
		}
	}
	data = DaoProcess_NewString( proc, "", 0 );
	entry = proc->topFrame->entry;
	while(1){
		postlen = mg_read( conn, postbuf, sizeof(postbuf)-1 );
		if( postlen <= 0 ) break; /* TODO: handle for postlen < 0? */
		if( sect != NULL ){
			if( sect->b > 0 ){
				DString_SetBytes( data->value, postbuf, postlen );
				DaoProcess_SetValue( proc, sect->a, (DaoValue*) data );
			}
			proc->topFrame->entry = entry;
			DaoProcess_Execute( proc );
			if( proc->status == DAO_PROCESS_ABORTED ) break;
		}else{
			DString_AppendBytes( res, postbuf, postlen );
		}
	}
	mg_close_connection( conn );
}

static void CLIENT_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	CLIENT_SendRequest( proc, p, NULL, NULL );
}

static void CLIENT_Post( DaoProcess *proc, DaoValue *p[], int N )
{
	CLIENT_SendRequest( proc, p, p[1]->xString.value, p[2]->xString.value );
}

static DaoFunctionEntry daoClientMeths[] =
{
	{ CLIENT_Get,	"Get( url: string ) => string" },
	{ CLIENT_Get,	"Get( url: string )[data: string]" },
	{ CLIENT_Post,	"Post( url: string, content: string, contentType: string ) => string" },
	{ CLIENT_Post,	"Post( url: string, content: string, contentType: string )[data: string]" },
	{ NULL, NULL }
};


DaoTypeCore daoClientCore =
{
	"Client",                                          /* name */
	0,                                                 /* size */
	{ NULL },                                          /* bases */
	{ NULL },                                          /* casts */
	NULL,                                              /* numbers */
	daoClientMeths,                                    /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	NULL,                                              /* Delete */
	NULL                                               /* HandleGC */
};


DAO_DLL int DaoHttp_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *streamns = DaoVmSpace_LinkModule( vmSpace, ns, "stream" );
	DaoNamespace *httpns = DaoVmSpace_GetNamespace( vmSpace, "http" );

	DaoNamespace_AddConstValue( ns, "http", (DaoValue*) httpns );

	DaoNamespace_WrapType( httpns, & daoRequestCore, DAO_CSTRUCT, 0 );
	DaoNamespace_WrapType( httpns, & daoResponseCore, DAO_CSTRUCT, 0 );
	DaoNamespace_WrapType( httpns, & daoSessionCore, DAO_CSTRUCT, 0 );
	DaoNamespace_WrapType( httpns, & daoCacheCore, DAO_CSTRUCT, 0 );
	DaoNamespace_WrapType( httpns, & daoHandlerCore, DAO_CSTRUCT, 0 );
	DaoNamespace_WrapType( httpns, & daoServerCore, DAO_CSTRUCT, 0 );
	DaoNamespace_WrapType( httpns, & daoClientCore, DAO_CSTRUCT, 0 );

	DaoNamespace_ParseType( httpns, "map<string,string>" );
	DaoNamespace_ParseType( httpns, "tuple<file:string,time:float>" );
	DaoNamespace_DefineType( httpns, "tuple<file:string,size:int,data:io::FileStream>", "HttpUpload" );
	DaoNamespace_ParseType( httpns, "map<string,HttpUpload>" );
	DaoNamespace_ParseType( httpns, "list<string>" );
	mg_init();
	return 0;
}
