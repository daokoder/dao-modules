/*
// Webdao: A Lightweight Web Application Framework for Dao
//
// Copyright (c) 2013, Limin Fu
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

#include "dao.h"
#include "daoStdtype.h"
#include "daoValue.h"
#include "daoVmspace.h"
#include "daoThread.h"
#include "daoGC.h"

#include "Marten/marten.h"

#ifndef DAO_WITH_THREAD
#define DMutex_Init(x)
#define DMutex_Destroy(x)
//#define DMutex_Lock(x)
//#define DMutex_Unlock(x)
#endif

typedef struct mg_callbacks mg_callbacks;
typedef struct mg_request_info mg_request_info;
typedef struct mg_connection mg_connection;
typedef struct mg_context mg_context;

typedef struct DaoxRequest DaoxRequest;
typedef struct DaoxResponse DaoxResponse;
typedef struct DaoxSession DaoxSession;
typedef struct DaoxServer DaoxServer;


DAO_DLL DaoType *daox_type_request;
DAO_DLL DaoType *daox_type_response;
DAO_DLL DaoType *daox_type_session;
DAO_DLL DaoType *daox_type_server;
DAO_DLL DaoType *daox_type_client;

DaoType *daox_type_request = NULL;
DaoType *daox_type_response = NULL;
DaoType *daox_type_session = NULL;
DaoType *daox_type_server = NULL;
DaoType *daox_type_client = NULL;

DaoType *daox_type_keyvalue = NULL;
DaoType *daox_type_namestream = NULL;
DaoType *daox_type_filemap = NULL;

static DaoVmSpace *dao_vmspace = NULL;
static DaoProcess *dao_process = NULL;
static DaoVmCode *dao_code_section = NULL;
static int dao_code_entry = 0;

static DaoxServer *daox_webdao_server = NULL;


/*==============================================================
// HTTP Request:
//============================================================*/
struct DaoxRequest
{
	DAO_CSTRUCT_COMMON;

	DaoMap   *http_get;    // <string,string>;
	DaoMap   *http_post;   // <string,string>;
	DaoMap   *http_cookie; // <string,string>;
	DaoMap   *http_file;   // <string,stream>;

	DaoString  *key;    // Buffer string;
	DaoString  *value;  // Buffer string;

	DString  *uri;
	daoint    remote_ip;
	daoint    remote_port;
};

DaoxRequest* DaoxRequest_New()
{
	DaoxRequest *self = (DaoxRequest*) dao_calloc( 1, sizeof(DaoxRequest) );
	DaoCstruct_Init( (DaoCstruct*) self, daox_type_request );
	self->uri = DString_New(1);
	self->key = DaoString_New(1);
	self->value = DaoString_New(1);
	self->http_get    = DaoMap_New( 1 + rand() );  /* Random hash seed; */
	self->http_post   = DaoMap_New( 1 + rand() );  /* To prevent hash DoS attacks; */
	self->http_cookie = DaoMap_New( 1 + rand() );
	self->http_file   = DaoMap_New( 1 + rand() );
	DString_SetSharing( self->key->value, 0 );
	DString_SetSharing( self->value->value, 0 );
	DaoGC_IncRC( (DaoValue*) self->key );
	DaoGC_IncRC( (DaoValue*) self->value );
	DaoGC_IncRC( (DaoValue*) self->http_get );
	DaoGC_IncRC( (DaoValue*) self->http_post );
	DaoGC_IncRC( (DaoValue*) self->http_cookie );
	DaoGC_IncRC( (DaoValue*) self->http_file );
	DaoGC_Assign( (DaoValue**) & self->http_get->ctype, (DaoValue*) daox_type_keyvalue );
	DaoGC_Assign( (DaoValue**) & self->http_post->ctype, (DaoValue*) daox_type_keyvalue );
	DaoGC_Assign( (DaoValue**) & self->http_cookie->ctype, (DaoValue*) daox_type_keyvalue );
	DaoGC_Assign( (DaoValue**) & self->http_file->ctype, (DaoValue*) daox_type_filemap );
	return self;
}

static void DaoxRequest_Delete( DaoxRequest *self )
{
	DString_Delete( self->uri );
	DaoGC_DecRC( (DaoValue*) self->key );
	DaoGC_DecRC( (DaoValue*) self->value );
	DaoGC_DecRC( (DaoValue*) self->http_get );
	DaoGC_DecRC( (DaoValue*) self->http_post );
	DaoGC_DecRC( (DaoValue*) self->http_cookie );
	DaoGC_DecRC( (DaoValue*) self->http_file );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}

static void DaoxRequest_Reset( DaoxRequest *self )
{
	DaoMap_Reset( self->http_get );
	DaoMap_Reset( self->http_post );
	DaoMap_Reset( self->http_cookie );
	DaoMap_Reset( self->http_file );
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
	DString *buffer = DString_New(1);
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
		while( postlen ){
			DString_AppendBytes( buffer, postbuf, postlen );
			postlen = mg_read( conn, postbuf, sizeof(postbuf) );
		}
		DaoxRequest_ParseQueryString( self, self->http_post, buffer->chars );
		DString_Delete( buffer );
		return;
	}
	boundary = strstr( content_type, "boundary=" ) + strlen( "boundary=" );
	boundarylen = strlen( boundary );

	fname = DString_New(1);
	buffer->size = 0;
	for(;;){
		postlen = mg_read( conn, postbuf, sizeof(postbuf) );
		if( postlen == 0 && buffer->size < boundarylen ) break;

		DString_AppendBytes( buffer, postbuf, postlen );
		while( strstr( buffer->chars, "\r\n\r\n" ) == 0 && postlen != 0 ){
			postlen = mg_read( conn, postbuf, sizeof(postbuf) );
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
				postlen = mg_read( conn, postbuf, sizeof(postbuf) );
				DString_AppendBytes( buffer, postbuf, postlen );
			}
			DString_SubString( buffer, value, 0, pos2 - 4 ); /* \r\n-- */
			DaoMap_Insert( self->http_post, (DaoValue*) self->key, (DaoValue*) self->value );
			buffer->size -= pos2 + boundarylen;
			memmove( buffer->chars, buffer->chars + pos2 + boundarylen, buffer->size );
		}else{
			DaoInteger isize = {DAO_INTEGER,0,0,0,0,0};
			DaoStream *stream = DaoStream_New();
			DaoTuple *tuple = DaoTuple_New(3);
			FILE *file = tmpfile();

			DaoString_Set( self->value, fname );
			DaoStream_SetFile( stream, file );
			DaoTuple_SetType( tuple, daox_type_namestream );
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



/*==============================================================
// HTTP Response:
//============================================================*/
struct DaoxResponse
{
	DAO_CSTRUCT_COMMON;

	DMap  *cookies;

	mg_connection *connection;
};

DaoxResponse* DaoxResponse_New()
{
	DaoxResponse *self = (DaoxResponse*) dao_calloc( 1, sizeof(DaoxResponse) );
	DaoCstruct_Init( (DaoCstruct*) self, daox_type_response );
	self->cookies = DMap_New( DAO_DATA_STRING, DAO_DATA_STRING );
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
	DMap_Clear( self->cookies );
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

void DaoxResponse_SetCookie( DaoxResponse *self, const char *name, const char *value, const char *path, time_t expire )
{
	DString *cookie;
	DString sname = DString_WrapChars( name );
	DNode *it = DMap_Insert( self->cookies, & sname, & sname );

	cookie = it->value.pString;
	DString_AppendChars( cookie, "=" );
	DString_AppendChars( cookie, value );
	DString_AppendChars( cookie, ";expires=" );
	DString_CookieTime( cookie, expire, 1 );
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

DaoxSession* DaoxSession_New()
{
	DaoComplex com = {DAO_COMPLEX,0,0,0,1,{0.0,0.0}};
	DaoxSession *self = (DaoxSession*) dao_calloc( 1, sizeof(DaoxSession) );
	DaoCstruct_Init( (DaoCstruct*) self, daox_type_session );
	self->variables = DHash_New( DAO_DATA_STRING, DAO_DATA_VALUE );
	self->cookie = DString_New(1);
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
static void DaoxSession_GetGCFields( void *p, DList *vs, DList *as, DList *maps, int rm )
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
// HTTP Server:
//============================================================*/
struct DaoxServer
{
	DAO_CSTRUCT_COMMON;

	DString  *docroot;

	DList   *indexFiles;
	DMap     *staticMimes;

	DMap  *uriToPaths;

	DMap  *cookieToSessions;
	DMap  *timestampToSessions;

	DList  *freeRequests;
	DList  *freeResponses;
	DList  *freeSessions;

	DList  *allRequests;
	DList  *allResponses;
	DList  *allSessions;

	DMutex   mutex;
	DMutex   mutex2;
	daoint   sessionIndex;
};

static void DaoxServer_InitMimeTypes( DaoxServer *self );
static void DaoxServer_InitIndexFiles( DaoxServer *self );

DaoxServer* DaoxServer_New()
{
	DaoxServer *self = (DaoxServer*) dao_calloc( 1, sizeof(DaoxServer) );
	DaoCstruct_Init( (DaoCstruct*) self, daox_type_server );
	self->docroot = DString_New(1);
	self->indexFiles = DList_New( DAO_DATA_STRING );
	self->staticMimes = DHash_New( DAO_DATA_STRING, DAO_DATA_STRING );
	self->uriToPaths = DHash_New( DAO_DATA_STRING, DAO_DATA_STRING );
	self->cookieToSessions = DHash_New( DAO_DATA_STRING, 0 );   // <DString*,DaoxSession*>
	self->timestampToSessions = DMap_New( DAO_DATA_VALUE, 0 ); // <DaoComplex,DaoxSession*>
	self->freeRequests  = DList_New(0); // <DaoxRequest*>
	self->freeResponses = DList_New(0); // <DaoxResponses*>
	self->freeSessions = DList_New(0); // <DaoxSessions*>
	self->allRequests  = DList_New( DAO_DATA_VALUE ); // <DaoxRequest*>
	self->allResponses = DList_New( DAO_DATA_VALUE ); // <DaoxResponses*>
	self->allSessions  = DList_New( DAO_DATA_VALUE ); // <DaoxSession*>
	DString_SetChars( self->docroot, dao_vmspace->startPath->chars );
	DaoxServer_InitMimeTypes( self );
	DaoxServer_InitIndexFiles( self );
	DMutex_Init( & self->mutex );
	DMutex_Init( & self->mutex2 );
	return self;
}

static void DaoxServer_Delete( DaoxServer *self )
{
	DString_Delete( self->docroot );
	DList_Delete( self->indexFiles );
	DMap_Delete( self->staticMimes );
	DMap_Delete( self->uriToPaths );
	DMap_Delete( self->cookieToSessions );
	DMap_Delete( self->timestampToSessions );
	DList_Delete( self->freeRequests );
	DList_Delete( self->freeResponses );
	DList_Delete( self->freeSessions );
	DList_Delete( self->allRequests );
	DList_Delete( self->allResponses );
	DList_Delete( self->allSessions );
	DMutex_Destroy( & self->mutex );
	DMutex_Destroy( & self->mutex2 );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
static void DaoxServer_GetGCFields( void *p, DList *vs, DList *arrays, DList *maps, int rm )
{
	DaoxServer *self = (DaoxServer*) p;
	DList_Append( arrays, self->allRequests );
	DList_Append( arrays, self->allResponses );
	DList_Append( arrays, self->allSessions );
}


DaoxRequest* DaoxServer_MakeRequest( DaoxServer *self, mg_connection *conn )
{
	DaoxRequest *request = NULL;
	const mg_request_info *ri = mg_get_request_info( conn );
	const char *cookie = mg_get_header( conn, "Cookie" );
	const char *query_string = ri->query_string;

	if( self->freeRequests->size ){
		DMutex_Lock( & self->mutex );
		if( self->freeRequests->size ){
			request = (DaoxRequest*) DList_PopFront( self->freeRequests );
			DaoxRequest_Reset( request );
		}
		DMutex_Unlock( & self->mutex );
	}

	if( request == NULL ){
		request = DaoxRequest_New();
		DMutex_Lock( & self->mutex );
		DList_Append( self->allRequests, request );
		DMutex_Unlock( & self->mutex );
	}
	request->remote_ip = ri->remote_ip;
	request->remote_port = ri->remote_port;

	DString_SetChars( request->uri, ri->uri );
	if( cookie ) DaoxRequest_ParseQueryString( request, request->http_cookie, cookie );
	if( query_string ) DaoxRequest_ParseQueryString( request, request->http_get, query_string );
	if( !strcmp( ri->request_method, "POST" ) ) DaoxRequest_ParsePostData( request, conn );
	return request;
}
DaoxResponse* DaoxServer_MakeResponse( DaoxServer *self, mg_connection *conn )
{
	DaoxResponse *response = NULL;

	if( self->freeResponses->size ){
		DMutex_Lock( & self->mutex );
		if( self->freeResponses->size ){
			response = (DaoxResponse*) DList_PopFront( self->freeResponses );
			DaoxResponse_Reset( response );
		}
		DMutex_Unlock( & self->mutex );
	}

	if( response == NULL ){
		response = DaoxResponse_New();
		DMutex_Lock( & self->mutex );
		DList_Append( self->allResponses, response );
		DMutex_Unlock( & self->mutex );
	}

	response->connection = conn;
	return response;
}
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
		session = DaoxSession_New();
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
static void DaoxServer_CacheObjects( DaoxServer *self, DaoxRequest *request, DaoxResponse *response, DaoxSession *session )
{
	DMutex_Lock( & self->mutex );
	if( request != NULL ) DList_Append( self->freeRequests, request );
	if( response != NULL ) DList_Append( self->freeResponses, response );
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
static void REQ_GETF_URI( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	DaoProcess_PutString( proc, request->uri );
}
static void REQ_SETF_URI( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRequest *request = (DaoxRequest*) p[0];
	DString *uri = DaoValue_TryGetString( p[1] );
	DString_Reset( request->uri, 0 );
	DString_Append( request->uri, uri );
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

static DaoFuncItem RequestMeths[] =
{
	{ REQ_GETF_URI,        ".URI( self :Request ) => string" },
	{ REQ_GETF_URI,        ".URI=( self :Request, uri :string )" },
	{ REQ_GETF_HttpGet,    ".HttpGet( self :Request ) => map<string,string>" },
	{ REQ_GETF_HttpPost,   ".HttpPost( self :Request ) => map<string,string>" },
	{ REQ_GETF_HttpCookie, ".HttpCookie( self :Request ) => map<string,string>" },
	{ REQ_GETF_HttpFile,   ".HttpFile( self :Request ) => map<string,HttpUpload>" },
	{ REQ_SETF_HttpGet,    ".HttpGet=( self :Request, value :map<string,string> )" },
	{ REQ_SETF_HttpPost,   ".HttpPost=( self :Request, value :map<string,string> )" },
	{ REQ_GETF_HttpCookie, ".HttpCookie=( self :Request, value :map<string,string> )" },
	{ REQ_GETF_HttpFile,   ".HttpFile=( self :Request, value :map<string,HttpUpload> )" },
	{ NULL, NULL }
};

static DaoTypeBase RequestTyper =
{
	"Request", NULL, NULL, RequestMeths, {0}, {0},
	(FuncPtrDel) DaoxRequest_Delete, NULL
};


static void RES_WriteHeader( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxResponse *response = (DaoxResponse*) p[0];
	DaoMap *headers = (DaoMap*) p[2];
	int status = p[1]->xInteger.value;
	DString *session_cookie = DString_New(1);
	DNode *it;

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
	DString_Delete( session_cookie );
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
	mg_send_file( response->connection, path );
}

static DaoFuncItem ResponseMeths[] =
{
	{ RES_WriteHeader, "WriteHeader( self :Response, status = 200, headers :map<string,string> = {=>} )" },
	{ RES_Write,    "Write( self :Response, content : string )" },
	{ RES_SendFile, "SendFile( self :Response, path : string )" },
	{ NULL, NULL }
};

static DaoTypeBase ResponseTyper =
{
	"Response", NULL, NULL, ResponseMeths, {0}, {0},
	(FuncPtrDel) DaoxResponse_Delete, NULL
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

static DaoFuncItem SessionMeths[] =
{
	{ SES_GetVar,     "GetVariable( self :Session, name :string ) => any" },
	{ SES_SetVar,     "SetVariable( self :Session, name :string, value :any )" },
	{ SES_SetTimeout, "SetTimeout( self :Session, seconds :int )" },
	{ NULL, NULL }
};

static DaoTypeBase SessionTyper =
{
	"Session", NULL, NULL, SessionMeths, {0}, {0},
	(FuncPtrDel) DaoxSession_Delete, DaoxSession_GetGCFields
};


static int DaoxHttp_TrySendFile( mg_connection *conn )
{
	int sendfile = 0;
	int sect_b = dao_code_section->b;
	const mg_request_info *ri = mg_get_request_info( conn );
	DaoxRequest *request = NULL;
	DaoxResponse *response = NULL;
	DaoxSession *session = NULL;
	DaoxServer *server = daox_webdao_server;
	DString uri = DString_WrapChars( ri->uri );
	DString *path = DString_New(1);
	DString *mime = DString_New(1);
	DNode *it;

	DString_SetChars( path, ri->uri + 1 );
	Dao_MakePath( server->docroot, path );
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
	}else if( Dao_IsDir( path->chars ) && path->chars[path->size-1] != '/' ){
		mg_printf(conn, "HTTP/1.1 301 Moved Permanently\r\nLocation: %s/\r\n\r\n", ri->uri);
		DString_Delete( path );
		DString_Delete( mime );
		return 1;
	}else if( Dao_IsDir( path->chars ) ){
		DString *path2 = DString_New(1);
		daoint i;
		for(i=0; i<server->indexFiles->size; i+=2){
			DString_Assign( path2, server->indexFiles->items.pString[i] );
			Dao_MakePath( path, path2 );
			if( Dao_IsFile( path2->chars ) ){
				DString_Assign( path, path2 );
				DString_Assign( mime, server->indexFiles->items.pString[i+1] );
				sendfile = 1;
				break;
			}
		}
		DString_Delete( path2 );
	}
	if( sendfile == 0 ) DString_Clear( path );
	DMutex_Lock( & server->mutex2 );
	DMap_Insert( server->uriToPaths, & uri, path );
	DMutex_Unlock( & server->mutex2 );
	if( sendfile == 0 ){
		DString_Delete( path );
		DString_Delete( mime );
		return 0;
	}
	//printf( ">>>>>>>>>>>> path: %i %s\n", sendfile, path->chars );

	if( sect_b >2 ){ /* Update session timestamp: */
		request = DaoxServer_MakeRequest( server, conn );
		session = DaoxServer_MakeSession( server, conn, request );
		DaoxServer_CacheObjects( server, request, NULL, session );
	}
	mg_send_file( conn, path->chars );
	DString_Delete( path );
	DString_Delete( mime );
	return 1;
}

static int DaoxHttp_HandleRequest( mg_connection *conn )
{
	int sect_a = dao_code_section->a;
	int sect_b = dao_code_section->b;
	const mg_request_info *ri = mg_get_request_info( conn );
	DaoxServer *server = daox_webdao_server;
	DaoProcess *process = NULL;
	DaoxRequest *request = NULL;
	DaoxResponse *response = NULL;
	DaoxSession *session = NULL;
	DaoVmCode *sect = NULL;
	DString uri = DString_WrapChars( ri->uri );
	DNode *it;

	DMutex_Lock( & server->mutex2 );
	it = DMap_Find( server->uriToPaths, & uri );
	DMutex_Unlock( & server->mutex2 );

	if( it != NULL ){
		if( it->value.pString->size ){
			mg_send_file( conn, it->value.pString->chars );
			return 1;
		}
	}else if( DaoxHttp_TrySendFile( conn ) ){
		return 1;
	}

	request = DaoxServer_MakeRequest( server, conn );
	response = DaoxServer_MakeResponse( server, conn );
	if( sect_b > 2 ){
		time_t expire = time(NULL) + 12*3600;
		session = DaoxServer_MakeSession( server, conn, request );
		DaoxResponse_SetCookie( response, "WebdaoSession", session->cookie->chars, "/", expire );
	}

	process = DaoVmSpace_AcquireProcess( dao_vmspace );
	DaoProcess_PushRoutine( process, dao_process->activeRoutine, dao_process->activeObject );
	process->activeCode = dao_process->activeCode;
	DaoProcess_PushFunction( process, dao_process->topFrame->routine );
	DaoProcess_SetActiveFrame( process, process->topFrame->prev );
	sect = DaoProcess_InitCodeSection( process, 3 );
	if( sect == NULL ){
		DaoProcess_PrintException( process, NULL, 1 );
		DaoVmSpace_ReleaseProcess( dao_vmspace, process );
		DaoxServer_CacheObjects( server, request, response, session );
		return 1;
	}

	process->topFrame->outer = dao_process;
	process->topFrame->host = dao_process->topFrame->prev;
	process->topFrame->returning = -1;
	process->topFrame->entry = dao_code_entry;

	if( sect_b >0 ) DaoProcess_SetValue( process, sect_a, (DaoValue*) request );
	if( sect_b >1 ) DaoProcess_SetValue( process, sect_a+1, (DaoValue*) response );
	if( sect_b >2 ) DaoProcess_SetValue( process, sect_a+2, (DaoValue*) session );
	DaoProcess_Execute( process );

	DaoVmSpace_ReleaseProcess( dao_vmspace, process );

	DaoxServer_CacheObjects( server, request, response, session );
	return 1;
}



static void SERVER_SetDocRoot( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *docroot = DaoValue_TryGetString( p[0] );
	DString_Assign( daox_webdao_server->docroot, docroot );
}
static void SERVER_Start( DaoProcess *proc, DaoValue *p[], int N )
{
	mg_context *ctx;
	mg_callbacks callbacks;
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 3 );
	char port[16], numthd[16];
	const char *options[7] = {
		"document_root", ".",
		"listening_ports", "8080",
		"num_threads", "8", NULL
	};

	sprintf( port, "%i", (int) p[0]->xInteger.value );
	sprintf( numthd, "%i", (int) p[1]->xInteger.value );
	options[1] = daox_webdao_server->docroot->chars;
	options[3] = port;
	options[5] = numthd;
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.begin_request = DaoxHttp_HandleRequest;

	if( sect == NULL ) return;
	DaoCGC_Start();

	dao_process = proc;
	dao_code_section = sect;
	dao_code_entry = proc->topFrame->entry;
	DaoProcess_PopFrame( proc );

	ctx = mg_start( & callbacks, NULL, options );
	if( ctx == NULL ){
		DaoProcess_RaiseError( proc, NULL, "failed to start the server" );
		return;
	}
	mg_wait( ctx );
}

static DaoFuncItem ServerMeths[] =
{
	{ SERVER_SetDocRoot, "SetDocumentRoot( docroot: string )" },
	{ SERVER_Start,     "Start( port = 80, numthread = 8 )"
	                 "[request:Request,response:Response,session:Session]" },
	{ NULL, NULL }
};

static DaoTypeBase ServerTyper =
{
	"Server", NULL, NULL, ServerMeths, {0}, {0},
	(FuncPtrDel) DaoxServer_Delete, DaoxServer_GetGCFields
};


static void CLIENT_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoString *data;
	DaoVmCode *sect = NULL;
	DaoStackFrame *frame = DaoProcess_FindSectionFrame( proc );
	DString *res = frame == NULL ? DaoProcess_PutChars( proc, "" ) : NULL;
	DString *url = p[0]->xString.value;
	DString *host = DString_Copy( url );
	DString *uri = DString_NewChars( "/" );
	DString *ports = DString_NewChars( "80" );
	const char *method = p[1]->xEnum.value ? "POST" : "GET";
	int port, use_ssl = 0;
	int entry;

	struct mg_connection *conn;
	char postbuf[4*1024];
	char ebuf[1000];
	int postlen;
	int pos;

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
	port = strtol( ports->chars, NULL, 0 );

	conn = mg_download( host->chars, port, use_ssl, ebuf, sizeof(ebuf),
			"%s %s HTTP/1.1\r\nHost: %s\r\n\r\n", method, uri->chars, host->chars );

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
		if( postlen == 0 ) break;
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

static DaoFuncItem ClientMeths[] =
{
	{ CLIENT_Get, "Get( url: string, method: enum<GET,POST> = $GET ) => string" },
	{ CLIENT_Get, "Get( url: string, method: enum<GET,POST> = $GET )[data: string]" },
	{ NULL, NULL }
};

static DaoTypeBase ClientTyper =
{
	"Client", NULL, NULL, ClientMeths, {0}, {0}, (FuncPtrDel) NULL, NULL
};


DAO_DLL int DaoHTTP_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	dao_vmspace = vmSpace;
	daox_type_request = DaoNamespace_WrapType( ns, & RequestTyper, 0 );
	daox_type_response = DaoNamespace_WrapType( ns, & ResponseTyper, 0 );
	daox_type_session = DaoNamespace_WrapType( ns, & SessionTyper, 0 );
	daox_type_server = DaoNamespace_WrapType( ns, & ServerTyper, 0 );
	daox_type_client = DaoNamespace_WrapType( ns, & ClientTyper, 0 );
	daox_type_keyvalue = DaoNamespace_ParseType( ns, "map<string,string>" );
	daox_type_namestream = DaoNamespace_TypeDefine( ns, "tuple<file:string,size:int,data:io::stream>", "HttpUpload" );
	daox_type_filemap = DaoNamespace_ParseType( ns, "map<string,HttpUpload>" );
	mg_init();
	daox_webdao_server = DaoxServer_New();
	DaoNamespace_AddConstValue( ns, "__WebdaoServer__", (DaoValue*) daox_webdao_server );
	return 0;
}
