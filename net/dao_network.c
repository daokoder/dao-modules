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

/* Contribution logs: */
/* 2011-02-11, Aleksey Danilov: added socket type with OOP-style methods; */
/* 2011-02-13, Aleksey Danilov: added socket error handling; */

#include"stdio.h"
#include"stdlib.h"
#include"string.h"
#include"time.h"
#include"math.h"
#include"errno.h"

#include"daoThread.h"

#ifdef UNIX

// gethostby* is not reentrant on Unix, synchronization is desirable
static DMutex net_mtx;
#define NET_TRANS( st ) DMutex_Lock( &net_mtx ); st; DMutex_Unlock( &net_mtx )
#define NET_INIT() DMutex_Init( &net_mtx )

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>

#else

#define NET_TRANS( st ) st
#define NET_INIT()

#include"winsock2.h"
#include"winsock.h"

#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))
#endif

typedef size_t socklen_t;
#define fileno _fileno
#define fdopen _fdopen

#endif

#include"dao.h"
#include"daoValue.h"
#include"daoStream.h"


#define BACKLOG 1000 /*  how many pending connections queue will hold */
#define MAX_DATA 512 /*  max number of bytes we can get at once */


/* types */
enum DaoProxyProtocol
{
	DPP_TRANS_DATA = 0,
	DPP_TRANS_END  = 1,
	DPP_NULL
};

typedef struct DaoDataPacket
{
	char  type;
	char  tag; /* for string and numarray */
	short size;
	int   dataI1; /* for string and numarray */
	int   dataI2; /* for numarray */
	char  data[ MAX_DATA + 100 ];
} DaoDataPacket;

static int offset = (char*) ( & ((DaoDataPacket*)0)->data ) - (char*) ( & ((DaoDataPacket*)0)->type );
static const char neterr[] = "Network";

static void GetErrorMessage( char *buffer, int code )
{
	switch( code ){
#ifdef WIN32
	case WSAENETDOWN:            strcpy( buffer, "Network subsystem failure (WSAENETDOWN)" ); break;
	case WSAEAFNOSUPPORT:        strcpy( buffer, "Address family is not supported (WSAEAFNOSUPPORT)" ); break;
	case WSAEINPROGRESS:         strcpy( buffer, "Service provider is processing another call (WSAEINPROGRESS)" ); break;
	case WSAEMFILE:              strcpy( buffer, "No more file descriptors available (WSAEMFILE)" ); break;
	case WSAENOBUFS:             strcpy( buffer, "Insufficient buffer space (WSAENOBUFS)" ); break;
	case WSAEPROTONOSUPPORT:     strcpy( buffer, "Protocol not supported (WSAEPROTONOSUPPORT)" ); break;
	case WSAEPROVIDERFAILEDINIT: strcpy( buffer, "Service provider failed to initialize (WSAEPROVIDERFAILEDINIT)" ); break;
	case WSAECONNRESET:          strcpy( buffer, "Connection was terminated by the remote side (WSAECONNRESET)" ); break;
	case WSAEADDRNOTAVAIL:       strcpy( buffer, "Address not available (WSAEADDRNOTAVAIL)" ); break;
	case WSAECONNREFUSED:        strcpy( buffer, "Connection was refused (WSAECONNREFUSED)" ); break;
	case WSAENETUNREACH:         strcpy( buffer, "Network not reachable (WSAENETUNREACH)" ); break;
	case WSAEHOSTUNREACH:        strcpy( buffer, "Host not reachable (WSAEHOSTUNREACH)" ); break;
	case WSAETIMEDOUT:           strcpy( buffer, "Attempt to establish connection timed out (WSAETIMEDOUT)" ); break;
	case WSAENETRESET:           strcpy( buffer, "Connection has timed out when $keepAlive option is set (WSAENETRESET)" ); break;
	case WSAENOTCONN:            strcpy( buffer, "Connection has been reset when $keepAlive option is set (WSAENOTCONN)" ); break;
	case WSAEMSGSIZE:            strcpy( buffer, "Message too large (WSAEMSGSIZE)" ); break;
	case WSAECONNABORTED:        strcpy( buffer, "Connection was terminated due to a time-out or other failure (WSAECONNABORTED)" ); break;
	case WSAEINVAL:              strcpy( buffer, "Invalid parameter (WSAEINVAL)" ); break;
	case WSAHOST_NOT_FOUND:      strcpy( buffer, "Authoritative answer host not found (WSAHOST_NOT_FOUND)" ); break;
	case WSATRY_AGAIN:           strcpy( buffer, "Nonauthoritative host not found, or no response from the server (WSATRY_AGAIN)" ); break;
	case WSANO_RECOVERY:         strcpy( buffer, "Non-recoverable error (WSANO_RECOVERY)" ); break;
	case WSANO_DATA:             strcpy( buffer, "Address not found (WSANO_DATA)" ); break;
	case WSASYSNOTREADY:         strcpy( buffer, "Network subsystem not ready (WSASYSNOTREADY)" ); break;
	case WSAVERNOTSUPPORTED:     strcpy( buffer, "Windows Sockets version not supported (WSAVERNOTSUPPORTED)" ); break;
	case WSAEPROCLIM:            strcpy( buffer, "Limit on the number of tasks has been reached (WSAEPROCLIM)" ); break;
	case WSAEFAULT:				 strcpy( buffer, "Failed to allocate necessary resources (WSAEFAULT)" ); break;
	case WSAEACCES:				 strcpy( buffer, "Insufficient privileges (WSAEACCES)" ); break;
	case WSAEADDRINUSE:			 strcpy( buffer, "Address already in use (WSAEADDRINUSE)" ); break;
#else
	case EAFNOSUPPORT:    strcpy( buffer, "Address family not supported (EAFNOSUPPORT)" ); break;
	case EMFILE:
	case ENFILE:          strcpy( buffer, "No more file descriptors available (EMFILE/ENFILE)" ); break;
	case EPROTONOSUPPORT: strcpy( buffer, "Protocol not supported (EPROTONOSUPPORT)" ); break;
	case EACCES:          strcpy( buffer, "Insufficient privileges (EACCES)" ); break;
	case ENOBUFS:         strcpy( buffer, "Insufficient buffer space (ENOBUFS)" ); break;
	case EADDRNOTAVAIL:   strcpy( buffer, "Address not available (EADDRNOTAVAIL)" ); break;
	case ETIMEDOUT:       strcpy( buffer, "Attempt to establish connection timed out (ETIMEDOUT)" ); break;
	case ECONNREFUSED:    strcpy( buffer, "Connection was refused (ECONNREFUSED)" ); break;
	case ENETUNREACH:     strcpy( buffer, "Network not reachable (ENETUNREACH)" ); break;
	case EADDRINUSE:      strcpy( buffer, "Address already in use (EADDRINUSE)" ); break;
	case EINTR:           strcpy( buffer, "Data sending was interrupted by a signal (EINTR)" ); break;
	case EMSGSIZE:        strcpy( buffer, "Message too large (EMSGSIZE)" ); break;
	case EPIPE:           strcpy( buffer, "Connection was broken (EPIPE)" ); break;
	case EINVAL:          strcpy( buffer, "Invalid parameter (EINVAL)" ); break;
	case ENOMEM:          strcpy( buffer, "Insufficient space (ENOMEM)" ); break;
	case ENOTCONN:        strcpy( buffer, "Socket not connected (ENOTCONN)" ); break;
#endif
	default: sprintf( buffer, "Unknown system error (%x)", code );
	}
}

static void GetHostErrorMessage( char *buffer, int code )
{
#ifdef WIN32
	GetErrorMessage( buffer, code );
#else
	switch( code ){
	case HOST_NOT_FOUND: strcpy( buffer, "Host not found (HOST_NOT_FOUND)" ); break;
	case TRY_AGAIN:      strcpy( buffer, "No response from the server (TRY_AGAIN)" ); break;
	case NO_RECOVERY:    strcpy( buffer, "A non-recoverable error occurred (NO_RECOVERY)" ); break;
	case NO_ADDRESS:     strcpy( buffer, "Address not found (NO_ADDRESS)" ); break;
	default:             sprintf( buffer, "Unknown system error (%x)", code );
	}
#endif
}

static int GetError()
{
#ifdef WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

static int GetHostError()
{
#ifdef WIN32
	return WSAGetLastError();
#else
	return h_errno;
#endif
}

enum {
	Socket_SharedAddress = 1,
	Socket_ExclusiveAddress = 2,
	Socket_ReusableAddress = 4,
};

typedef int socket_opts;

int DaoNetwork_SetSocketOptions( int sock, socket_opts opts )
{
	int flag = 1;
#ifdef WIN32
	if ( opts & Socket_ExclusiveAddress ){
		if( setsockopt( sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (void*)&flag, sizeof(int) ) == -1 ) return -1;
	}
	if ( opts & Socket_ReusableAddress ){
		if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (void*)&flag, sizeof(int) ) == -1 ) return -1;
	}
#else
	if ( opts & Socket_SharedAddress ){
		if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (void*)&flag, sizeof(int) ) == -1 ) return -1;
	}
#endif
	return 0;
}

void DaoNetwork_Close( int sockfd );
int DaoNetwork_Bind( int port, socket_opts opts )
{
	struct sockaddr_in myaddr;    /*  my address information */
	int sockfd = socket( AF_INET, SOCK_STREAM, 0);
	if( sockfd == -1 ) return -1;
	if( opts && DaoNetwork_SetSocketOptions( sockfd, opts ) == -1 ) return -1;

	myaddr.sin_family = AF_INET;         /*  host byte order */
	myaddr.sin_port = htons( port );     /*  short, network byte order */
	myaddr.sin_addr.s_addr = INADDR_ANY; /*  automatically fill with my IP */
	memset( &( myaddr.sin_zero ), '\0', 8); /*  zero the rest of the struct */

	if( bind( sockfd, (struct sockaddr *) & myaddr, sizeof(struct sockaddr) ) == -1){
		DaoNetwork_Close(sockfd);
		return -1;
	}
	return sockfd;
}
int DaoNetwork_Listen( int sockfd, int backlog )
{
	return listen( sockfd, backlog );
}
int DaoNetwork_Accept( int sockfd )
{
	return accept( sockfd, NULL, NULL );
}
int DaoNetwork_Connect( const char *host, unsigned short port )
{
	int sockfd;
	struct sockaddr_in addr;
	struct hostent *he;
	NET_TRANS( he = gethostbyname( host ) ); /*  get the host info */
	if( he == NULL) return -1;
	if( ( sockfd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1) return -1;

	addr.sin_family = AF_INET;    /*  host byte order */
	addr.sin_port = htons( port );  /*  short, network byte order */
	addr.sin_addr = *(struct in_addr *)he->h_addr;

	/* printf( "DaoNetwork_Connect() : %s, %i %i\n", host, port, sockfd ); */
	if( connect( sockfd, (struct sockaddr *)& addr, sizeof(struct sockaddr)) == -1){
		DaoNetwork_Close( sockfd );
		/* printf( "DaoNetwork_Connect() failed: %s, %i %i\n", host, port, sockfd ); */
		/* perror("connect"); */
		return -1;
	}
	return sockfd;
}

#ifdef WIN32
#define INTERRUPTED WSAEINTR
#else
#define INTERRUPTED EINTR
#endif

int LoopSend( int sockfd, char *buf, int size, int flags )
{
	int left = size;
	int numbytes;
	do{
		numbytes = send( sockfd, buf, left, flags );
		if(numbytes != -1){
			left -= numbytes;
			buf += numbytes;
		}
		else if( GetError() != INTERRUPTED )
			return -1;
	}
	while( left > 0 );
	return size;
}
int LoopReceive( int sockfd, char *buf, int size, int flags )
{
	int numbytes;
	do
		numbytes = recv( sockfd, buf, size, flags );
	while( numbytes == -1 && GetError() == INTERRUPTED );
	return numbytes;
}
int DaoNetwork_Send( int sockfd, DString *buf )
{
	return LoopSend( sockfd, DString_GetData( buf ), DString_Size( buf ), 0);
}
int DaoNetwork_Receive( int sockfd, DString *buf, int max )
{
	int numbytes;
	if( max <=0 || max >= 1E4 ) max = 1E4;
	DString_Resize( buf, max );
	numbytes = LoopReceive( sockfd, (char*)DString_GetData( buf ), max, 0 );
	if( numbytes >=0 ) DString_Resize( buf, numbytes );
	/* if( numbytes >=0 ) buf->size = numbytes; */
	return numbytes;
}

enum {
	Shutdown_Receive = 0,
	Shutdown_Send = 1,
	Shutdown_Both = 2
};

typedef int shutdown_flag;

int DaoNetwork_Shutdown( int sockfd, shutdown_flag flag )
{
	return shutdown( sockfd, flag );
}

void DaoNetwork_Close( int sockfd )
{
#ifdef UNIX
	close( sockfd );
#elif WIN32
	closesocket( sockfd );
#endif
}

#define RADIX 32
static const char *mydigits = "0123456789abcdefghijklmnopqrstuvw";

void DaoPrintNumber( char *buf, long double value )
{
	int expon, digit;
	long double prod, frac;
	char *p = buf;
	if( value <0.0 ){
		p[0] = '-';
		p ++;
		value = -value;
	}
	frac = frexp( value, & expon );
	/* printf( "DaoPrintNumber: frac = %Lf %Lf\n", frac, value ); */
	while(1){
		prod = frac * RADIX;
		digit = (int) prod;
		frac = prod - digit;
		sprintf( p, "%c", mydigits[ digit ] );
		p ++;
		if( frac <= 0 ) break;
	}
	sprintf( p, "#%i", expon );
	/* printf( "DaoPrintNumber: %s, %Lg\n", buf, value ); */
	return;
}
long double DaoParseNumber( char *buf )
{
	long double frac = 0;
	int expon, sign = 1;
	char *p = buf;
	long double factor = 1.0 / RADIX;
	long double accum = factor;
	if( buf[0] == '-' ){
		p ++;
		sign = -1;
	}
	/* printf( "DaoParseNumber: %s\n", buf ); */
	while( *p && *p != '#' ){
		int digit = *p;
		digit -= digit >= 'a' ? 'a' - 10 : '0';
		frac += accum * digit;
		accum *= factor;
		p ++;
	}
	expon = strtol( p+1, NULL, 10 );
	/* printf( "DaoParseNumber: %Lf %Lf %Lf %s\n", frac, accum, ldexp( frac, expon ), p+1 ); */
	return ldexp( frac, expon ) * sign;
}

static int DaoNetwork_SendTag( int sockfd, char tag )
{
	DaoDataPacket packet;
	int length;
	length = offset + 1;
	packet.type = 0;
	packet.tag = tag;
	packet.size = htons( length );
	packet.dataI1 = packet.dataI2 = 0;
	return LoopSend( sockfd, (char*)&packet, length, 0);
}
static int DaoNetwork_SendNil( int sockfd )
{
	return DaoNetwork_SendTag( sockfd, DPP_TRANS_END );
}
static int DaoNetwork_SendInteger( int sockfd, int value )
{
	DaoDataPacket packet;
	int length;
	packet.tag = 0;
	packet.dataI1 = packet.dataI2 = 0;
	DaoPrintNumber( packet.data, value );
	length = offset + strlen( packet.data ) + 1;
	/* printf( "send number: %i, %s\n", length, packet.data ); */
	packet.type = DAO_INTEGER;
	packet.size = htons( length );
	return LoopSend( sockfd, (char*)&packet, length, 0);
}
static int DaoNetwork_SendFloat( int sockfd, float value )
{
	DaoDataPacket packet;
	int length;
	packet.tag = 0;
	packet.dataI1 = packet.dataI2 = 0;
	DaoPrintNumber( packet.data, value );
	length = offset + strlen( packet.data ) + 1;
	packet.type = DAO_FLOAT;
	packet.size = htons( length );
	return LoopSend( sockfd, (char*)&packet, length, 0);
}
static int DaoNetwork_SendNumber( int sockfd, DaoValue *data )
{
	if( DaoValue_Type( data ) == DAO_INTEGER )
		return DaoNetwork_SendInteger( sockfd, DaoValue_TryGetInteger( data ) );
	else if(  DaoValue_Type( data ) == DAO_FLOAT )
		return DaoNetwork_SendFloat( sockfd, DaoValue_TryGetFloat( data ) );
	else
		return DaoNetwork_SendFloat( sockfd, DaoValue_TryGetDouble( data ) );
}
static int DaoNetwork_SendChars( int sockfd, const char *mbs, int len )
{
	DaoDataPacket packet;
	int length = 0;
	int shift = 0;

	packet.type = DAO_STRING;
	packet.dataI1 = packet.dataI2 = 0;
	while( shift < len ){
		int copy = ( len - shift ) >= MAX_DATA ? MAX_DATA : ( len - shift );
		/* tag: 0, new string; 1, cat string; 2, finish string: */
		packet.tag = shift == 0 ? 0 : ( ( len - shift ) > MAX_DATA ? 1 : 2 );
		length = copy + offset + 1;
		strncpy( packet.data, mbs + shift, copy );
		packet.data[copy] = 0;
		packet.size = htons( length );
		if( LoopSend( sockfd, (char*) &packet, length, 0) == -1 )
			return -1;
		shift += copy;
	}
	return 0;
}
static int DaoNetwork_SendString( int sockfd, DString *str )
{
	return DaoNetwork_SendChars( sockfd, DString_GetData( str ), DString_Size( str ) );
}
static int DaoNetwork_SendComplex( int sockfd, complex16 data )
{
	DaoDataPacket packet;
	int len, length = offset;
	char *buf2 = packet.data;
	packet.tag = 0;
	packet.dataI1 = packet.dataI2 = 0;
	packet.type = DAO_COMPLEX;
	DaoPrintNumber( buf2, data.real );
	len = strlen( buf2 ) + 1;
	length += len;
	buf2 += len;
	DaoPrintNumber( buf2, data.imag );
	length += strlen( buf2 );
	packet.size = htons( length );
	return LoopSend( sockfd, (char*) &packet, length, 0);
}
static int DaoNetwork_SendArray( int sockfd, DaoArray *data )
{
	DaoDataPacket packet;
	char *buf2 = packet.data;
	int length = 0;
	int numtype = DaoArray_NumType( data );
	int M = DaoArray_Size( data );
	int j, len;

	packet.type = DAO_ARRAY;
	packet.tag = numtype;
	packet.dataI1 = htonl( M );
	packet.dataI2 = htonl( 0 );
	if( numtype == DAO_INTEGER ){
		daoint *vec = DaoArray_ToInteger( data );
		for(j=0; j<M; j++){
			DaoPrintNumber( buf2, vec[j] );
			len = strlen( buf2 ) + 1;
			length += len;
			buf2 += len;
			if( length >= MAX_DATA ){
				packet.size = htons( offset + length );
				if( LoopSend( sockfd, (char*) &packet, offset + length, 0) == -1 )
					return -1;
				length = 0;
				buf2 = packet.data;
				packet.dataI2 = htonl( j+1 );
			}
		}
	}else if( numtype == DAO_FLOAT ){
		float *vec = DaoArray_ToFloat( data );
		for(j=0; j<M; j++){
			DaoPrintNumber( buf2, vec[j] );
			len = strlen( buf2 ) + 1;
			length += len;
			buf2 += len;
			if( length >= MAX_DATA ){
				packet.size = htons( offset + length );
				if( LoopSend( sockfd, (char*) &packet, offset + length, 0) == -1 )
					return -1;
				length = 0;
				buf2 = packet.data;
				packet.dataI2 = htonl( j+1 );
			}
		}
	}else{
		double *vec = DaoArray_ToDouble( data );
		if( numtype == DAO_COMPLEX ) M += M;
		for(j=0; j<M; j++){
			DaoPrintNumber( buf2, vec[j] );
			len = strlen( buf2 ) + 1;
			length += len;
			buf2 += len;
			if( length >= MAX_DATA ){
				packet.size = htons( offset + length );
				if( LoopSend( sockfd, (char*) &packet, offset + length, 0) == -1 )
					return -1;
				length = 0;
				buf2 = packet.data;
				packet.dataI2 = htonl( j+1 );
			}
		}
	}
	packet.size = htons( offset + length );
	return LoopSend( sockfd, (char*) &packet, offset + length, 0);
}
static int DaoNetwork_SendExt( DaoProcess *proc, int sockfd, DaoValue *data[], int size )
{
	int i;
	for( i=0; i<size; i++ ){
		DaoValue *item = data[i];
		int res = 0;
		switch( DaoValue_Type( item ) ){
		case DAO_INTEGER :
		case DAO_FLOAT :
		case DAO_DOUBLE :
			res = DaoNetwork_SendNumber( sockfd, item );
			break;
		case DAO_STRING :
			res = DaoNetwork_SendString( sockfd, DaoString_Get( DaoValue_CastString( item ) ) );
			break;
		case DAO_COMPLEX :
			res = DaoNetwork_SendComplex( sockfd, DaoComplex_Get( DaoValue_CastComplex( item ) ) );
			break;
		case DAO_ARRAY :
			res = DaoNetwork_SendArray( sockfd, DaoValue_CastArray( item ) );
			break;
		default : break;
		}
		if( res == -1 )
			return -1;
	}
	return 1;
}
int DaoNetwork_ReceiveExt( DaoProcess *proc, int sockfd, DaoList *data )
{
	int i, j, numbytes, start, size, M;
	char buf[ MAX_DATA + MAX_DATA + 100];
	char *buf2 = buf;
	short numtype;
	complex16  com;
	DaoArray *arr = NULL;
	DaoValue *item;
	DString  *str = DString_New();
	float *fv = NULL;
	double *dv = NULL;
	DaoDataPacket *inpack;
	int dpp = 0, count = 0;
	char bufin[ MAX_DATA + MAX_DATA + 2 ];

	/* printf( "receive3 : offset = %i\n", offset ); */
	numbytes = LoopReceive( sockfd, bufin, MAX_DATA, 0 );
	if( numbytes == -1 ){
		DString_Delete( str );
		return -1;
	}
	while( numbytes >0 ){
		bufin[ numbytes ] = '\0';
		start = 0;
		count = 1;
		/* printf( "DaoProxy_Receive: %i\n", bufin[0] ); */
		while( start < numbytes ){
			inpack = (DaoDataPacket*) (bufin+start);
			size = ntohs( inpack->size );
			/* printf( "chunk: %i, %i, %i, %i\n", inpack->type, numbytes, start, size ); */
			if( size > ( numbytes - start ) /* if the packet is not received completely: */
					|| ( start + offset >= numbytes ) /* ensure valid header */ ){
				numbytes = numbytes - start;
				memmove( bufin, bufin + start, numbytes );
				/* printf( "1 count = %i\n", count ); */
				count = LoopReceive( sockfd, bufin + numbytes, MAX_DATA, 0 );
				/* printf( "count = %i\n", count ); */
				if( count < 0 ){
					DString_Delete( str );
					return -1;
				}
				numbytes += count;
				bufin[ numbytes ] = '\0';
				start = 0;
				continue;
			}
			if( inpack->type == 0 && inpack->tag == DPP_TRANS_END ) break;
			if( inpack->type == 0 ){
				dpp = inpack->tag;
				start += size;
				continue;
			}
			buf2 = inpack->data;
			/* printf( "type = %i\n", inpack->type ); */
			switch( inpack->type ){
			case DAO_INTEGER :
				item = (DaoValue*) DaoProcess_NewInteger( proc, DaoParseNumber( inpack->data ) );
				DaoList_PushBack( data, item );
				break;
			case DAO_FLOAT :
				item = (DaoValue*) DaoProcess_NewFloat( proc, DaoParseNumber( inpack->data ) );
				/* printf( "number: %s %g\n", inpack->data, num->item.f ); */
				DaoList_PushBack( data, item );
				break;
			case DAO_DOUBLE :
				item = (DaoValue*) DaoProcess_NewDouble( proc, DaoParseNumber( inpack->data ) );
				DaoList_PushBack( data, item );
				break;
			case DAO_STRING :
				if( inpack->tag == 0 ) DString_Clear( str );
				DString_AppendChars( str, inpack->data );
				item = (DaoValue*) DaoProcess_NewString( proc, NULL, 0);
				DaoString_Set( DaoValue_CastString( item ), str );
				/* printf( "string: %s\n", inpack->data ); */
				if( inpack->tag ==2 || size <= MAX_DATA ) DaoList_PushBack( data, item );
				break;
			case DAO_COMPLEX :
				com.real = DaoParseNumber( buf2 );
				while( *buf2 ) buf2 ++;
				com.imag = DaoParseNumber( buf2+1 );
				item = (DaoValue*) DaoProcess_NewComplex( proc, com );
				DaoList_PushBack( data, item );
				break;
			case DAO_ARRAY :
				numtype = inpack->tag;
				M = ntohl( inpack->dataI1 );
				j = ntohl( inpack->dataI2 );
				/* printf( "ARRAY: M=%i; j=%i\n", M, j ); */
				if( j == 0 ){
					item = (DaoValue*) DaoProcess_NewArray( proc, DAO_INTEGER );
					arr = DaoValue_CastArray( item );
					DaoArray_SetNumType( arr, numtype );
					DaoArray_ResizeVector( arr, M );
					DaoList_PushBack( data, item );
				}
				if( numtype == DAO_INTEGER ){
					daoint *iv = DaoArray_ToInteger( arr );
					for(i=j; i<M; i++){
						iv[i] = DaoParseNumber( buf2 );
						while( *buf2 ) buf2 ++;
						buf2 ++;
						if( ( buf2 - inpack->data ) >= MAX_DATA ) break;
					}
				}else if( numtype == DAO_FLOAT ){
					fv = DaoArray_ToFloat( arr );
					for(i=j; i<M; i++){
						fv[i] = DaoParseNumber( buf2 );
						while( *buf2 ) buf2 ++;
						buf2 ++;
						if( ( buf2 - inpack->data ) >= MAX_DATA ) break;
					}
				}else{
					dv = DaoArray_ToDouble( arr );
					if( numtype == DAO_COMPLEX ) M += M;
					for(i=j; i<M; i++){
						dv[i] = DaoParseNumber( buf2 );
						while( *buf2 ) buf2 ++;
						buf2 ++;
						if( ( buf2 - inpack->data ) >= MAX_DATA ) break;
					}
				}
				break;
			default : break;
			}
			start += size;
		}
		if( inpack->type ==0 && inpack->tag == DPP_TRANS_END ) break;
		if( count <= 0 ) break;
		/* printf( "before %i %i\n", daoProxyPort, sockfd ); */
		numbytes = LoopReceive( sockfd, bufin, MAX_DATA, 0 );
		/* printf( "after %i %i\n", daoProxyPort, sockfd ); */
		if( numbytes == -1 ){
			DString_Delete( str );
			return -1;
		}
	}
	DString_Delete( str );
	return dpp;
}

#define MAX_ERRMSG 100

enum {
	Socket_Closed = 0,
	Socket_Bound,
	Socket_Connected,
	Socket_Listening
};

typedef int socket_state;
typedef struct DaoSocket DaoSocket;

struct DaoSocket
{
	int id;
	socket_state state;
};

extern DaoTypeBase socketTyper;
DaoType *daox_type_socket = NULL;

static DaoSocket* DaoSocket_New(  )
{
	DaoSocket *self = dao_malloc( sizeof(DaoSocket) );
	self->id = -1;
	self->state = Socket_Closed;
	return self;
}

static int DaoSocket_Shutdown( DaoSocket *self, shutdown_flag flag )
{
	if( self->id != -1 ){
		if ( DaoNetwork_Shutdown( self->id, flag ) != 0 )
			return -1;
	}
	return 0;
}

static void DaoSocket_Close( DaoSocket *self )
{
	if( self->id != -1 ){
		DaoNetwork_Close( self->id );
		self->id = -1;
		self->state = Socket_Closed;
	}
}

static void DaoSocket_Delete( DaoSocket *self )
{
	DaoSocket_Close( self );
	dao_free( self );
}

static int DaoSocket_Bind( DaoSocket *self, int port, socket_opts opts )
{
	DaoSocket_Close( self );
	self->id = DaoNetwork_Bind( port, opts );
	if( self->id != -1 )
		self->state = Socket_Bound;
	return self->id;
}

static int DaoSocket_Connect( DaoSocket *self, DString *host, int port )
{
	DaoSocket_Close( self );
	self->id = DaoNetwork_Connect( DString_GetData( host ), port );
	if( self->id != -1 )
		self->state = Socket_Connected;
	return self->id;
}

static void DaoSocket_Lib_Shutdown( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket* self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	if ( self->state != Socket_Connected )
		DaoProcess_RaiseError( proc, neterr, "Socket not connected" );
	else if ( DaoSocket_Shutdown( self, par[1]->xEnum.value ) != 0 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_Close( DaoProcess *proc, DaoValue *par[], int N  )
{
	DaoSocket_Close( (DaoSocket*)DaoValue_TryGetCdata( par[0] ) );
}

static void DaoSocket_Lib_Bind( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	if( DaoSocket_Bind( self, par[1]->xInteger.value, N == 3? par[2]->xEnum.value : 0 ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_Listen( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	if( self->state != Socket_Bound ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not bound" );
		return;
	}
	if( DaoNetwork_Listen( self->id, DaoValue_TryGetInteger( par[1] ) ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	self->state = Socket_Listening;
}

static void DaoSocket_Lib_Accept( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	DaoSocket *sock;
	if( self->state != Socket_Listening ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not in the listening state" );
		return;
	}
	sock = DaoSocket_New(  );
	sock->id = DaoNetwork_Accept( self->id );
	if( sock->id == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	sock->state = Socket_Connected;
	DaoProcess_PutCdata( proc, (void*)sock, daox_type_socket );
}

static void DaoSocket_Lib_Connect( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	if( DaoSocket_Connect( self, par[1]->xString.value, par[2]->xInteger.value ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_Send( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	int n;
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	n = DaoNetwork_Send( self->id, DaoString_Get( DaoValue_CastString( par[1] ) ) );
	if( n == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_Receive( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	DString *mbs = DaoProcess_PutChars( proc, "" );
	if( DaoNetwork_Receive( self->id, mbs, DaoValue_TryGetInteger( par[1] ) ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_SendDao( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	int n;
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	n = DaoNetwork_SendExt( proc, self->id, par+1, N-1 );
	if( n == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
}

static void DaoSocket_Lib_ReceiveDao( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	DaoList *res = DaoProcess_PutList( proc );
	if( DaoNetwork_ReceiveExt( proc, self->id, res ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_Id( DaoProcess *proc, DaoValue *par[], int N  )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	DaoProcess_PutInteger( proc, self->id );
}

static void DaoSocket_Lib_State( DaoProcess *proc, DaoValue *par[], int N  )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	char buffer[10];
	switch( self->state ){
	case Socket_Closed:    strcpy( buffer, "closed" ); break;
	case Socket_Bound:     strcpy( buffer, "bound" ); break;
	case Socket_Listening: strcpy( buffer, "listening" ); break;
	case Socket_Connected: strcpy( buffer, "connected" ); break;
	}
	DaoProcess_PutEnum( proc, buffer );
}

static void DaoSocket_Lib_GetPeerName( DaoProcess *proc, DaoValue *par[], int N  )
{
	DString *res;
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	char errbuf[MAX_ERRMSG];
	struct sockaddr_in addr;
#ifdef WIN32
	int size = sizeof( struct sockaddr_in );
#else
	socklen_t size = sizeof( struct sockaddr_in );
#endif
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	if( getpeername( self->id, (struct sockaddr *) & addr, & size ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	res = DaoProcess_PutChars( proc, "" );
	DString_SetChars( res, inet_ntoa( addr.sin_addr ) );
}

static void DaoSocket_Lib_GetStream( DaoProcess *proc, DaoValue *par[], int N  )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	const char *mode = DString_GetData( DaoValue_TryGetString( par[1] ) );
	DaoStream *stream;
#ifdef WIN32
	DaoProcess_RaiseError( proc, NULL, "Creating stream from a socket is not supported on the current platform" );
	return;
#endif
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	stream = DaoStream_New();
	stream->mode |= DAO_STREAM_FILE;
	stream->file = fdopen( self->id, mode );
	if ( !stream->file ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		DaoStream_Delete( stream );
		return;
	}
	if( strstr( mode, "+" ) ){
		stream->mode = DAO_STREAM_WRITABLE | DAO_STREAM_READABLE;
	}else{
		if( strstr( mode, "r" ) ) stream->mode |= DAO_STREAM_READABLE;
		if( strstr( mode, "w" ) || strstr( mode, "a" ) ) stream->mode |= DAO_STREAM_WRITABLE;
	}
	DaoProcess_PutValue( proc, (DaoValue*)stream );
}

static DaoFuncItem socketMeths[] =
{
	/*! Binds the socket to \a port using \a address options if specified. For the description of \a address, see \c net.bind() */
	{  DaoSocket_Lib_Bind,          "bind( self: socket, port: int )" },
	{  DaoSocket_Lib_Bind,          "bind( self: socket, port: int, address: enum<shared;exclusive;reused> )" },

	/*! Listens the socket using \a backLog as the maximum size of the queue of pending connections */
	{  DaoSocket_Lib_Listen,        "listen( self: socket, backLog = 10 )" },

	/*! Accepts connection */
	{  DaoSocket_Lib_Accept,        "accept( self: socket ) => socket" },

	/*! Connects to \a host : \a port */
	{  DaoSocket_Lib_Connect,       "connect( self: socket, host: string, port: int )" },

	/*! Sends data \a data */
	{  DaoSocket_Lib_Send,          "send( self: socket, data: string )" },

	/*! Receives at most \a limit bytes and returnes the received data */
	{  DaoSocket_Lib_Receive,       "receive( self: socket, limit = 512 ) => string" },

	/*! Sends data via the internal serialization protocol */
	{  DaoSocket_Lib_SendDao,       "send_dao( self: socket, ... )" },

	/*! Receives data via the internal serialization protocol */
	{  DaoSocket_Lib_ReceiveDao,    "receive_dao( self: socket ) => list<int|float|double|complex|string|array>" },

	/*! Peer name */
	{  DaoSocket_Lib_GetPeerName,   ".peername( invar self: socket ) => string" },

	/*! Socket file descriptor */
	{  DaoSocket_Lib_Id,            ".id( invar self: socket ) => int" },

	/*! Current socket state */
	{  DaoSocket_Lib_State,         ".state( invar self: socket ) => enum<closed,bound,listening,connected>" },

	/*! Shuts down the connection, stopping further operations specified by \a what */
	{  DaoSocket_Lib_Shutdown,      "shutdown( self: socket, what: enum<send,receive,both> )" },

	/*! Closes the socket */
	{  DaoSocket_Lib_Close,         "close( self: socket )" },

	/*! Returns stream opened with \a mode bound to the socket
	 * \warning Not supported on Windows */
	{  DaoSocket_Lib_GetStream,     "open( invar self: socket, mode: string ) => io::stream" },
	{ NULL, NULL }
};

DaoTypeBase socketTyper = {
	"socket", NULL, NULL, socketMeths, {0}, {0}, (FuncPtrDel)DaoSocket_Delete, NULL
};

static void DaoNetLib_Bind( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *sock = DaoSocket_New(  );
	if( DaoSocket_Bind( sock, par[0]->xInteger.value, N == 2? par[1]->xEnum.value : 0 ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	DaoProcess_PutCdata( proc, (void*)sock, daox_type_socket );
}
static void DaoNetLib_Connect( DaoProcess *proc, DaoValue *p[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *sock = DaoSocket_New(  );
	if( DaoSocket_Connect( sock, p[0]->xString.value, p[1]->xInteger.value ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	DaoProcess_PutCdata( proc, (void*)sock, daox_type_socket );
}
static void DaoNetLib_GetHost( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	struct hostent *hent;
	struct sockaddr_in addr;
	struct in_addr id;
	size_t size = sizeof( struct sockaddr_in );
	const char *host = DaoString_GetChars( DaoValue_CastString( par[0] ) );
	DaoTuple *tup = DaoProcess_PutTuple( proc, 3 );
	DaoTuple_SetItem( tup, (DaoValue*)DaoProcess_NewList( proc ), 1 );
	DaoTuple_SetItem( tup, (DaoValue*)DaoProcess_NewList( proc ), 2 );
	if( DaoString_Size( DaoValue_CastString( par[0] ) ) ==0 ) return;
	if( host[0] >= '0' && host[0] <= '9' ){
#ifdef UNIX
		if( inet_aton( host, & id ) == 0 ) return;
#elif WIN32
		id.s_addr = inet_addr( host );
#endif
		addr.sin_family = AF_INET; 
		addr.sin_addr = id;
		memset( &( addr.sin_zero ), '\0', 8);
		NET_TRANS( hent = gethostbyaddr( (void*) & addr, size, AF_INET ) );
	}else{
		NET_TRANS( hent = gethostbyname( host ) );
	}
	if( hent == NULL ){
		GetHostErrorMessage( errbuf, GetHostError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	DString_SetChars( tup->values[0]->xString.value, hent->h_name );
	if( hent->h_addrtype == AF_INET ){
		char **p = hent->h_aliases;
		char **q = hent->h_addr_list;
		while( *p ){
			DaoList_Append( &tup->values[1]->xList, (DaoValue*)DaoString_NewChars( *p ) );
			p ++;
		}
		while ( *q ){
			DaoList_Append( &tup->values[2]->xList, (DaoValue*)DaoString_NewChars( inet_ntoa( *(struct in_addr*) (*q) ) ) );
			q ++;
		}
	}else{ /* AF_INET6 */
	}
}

static void DaoNetLib_GetService( DaoProcess *proc, DaoValue *par[], int N  )
{
	struct servent *srvent;
	DaoTuple *tup = DaoProcess_PutTuple( proc, 3 );
	char **p;
	DaoTuple_SetItem( tup, (DaoValue*)DaoProcess_NewList( proc ), 2 );
	if ( par[0]->type == DAO_STRING ){
		if ( !par[0]->xString.value->size )
			return;
		srvent = getservbyname( par[0]->xString.value->chars, "tcp" );
	}
	else
		srvent = getservbyport( par[0]->xInteger.value, "tcp" );
	if( srvent == NULL )
		return;
	DString_SetChars( tup->values[0]->xString.value, srvent->s_name );
	tup->values[1]->xInteger.value = srvent->s_port;
	p = srvent->s_aliases;
	while( *p ){
		DaoList_Append( &tup->values[2]->xList, (DaoValue*)DaoString_NewChars( *p ) );
		p ++;
	}
}

static void DaoNetLib_Select( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	struct timeval tv;
	int i;
	fd_set set1, set2;
	DaoTuple *tuple = DaoProcess_PutTuple( proc, 0 );
	DaoList *list1 = DaoValue_CastList( par[0] );
	DaoList *list2 = DaoValue_CastList( par[1] );
	DaoList *reslist;
	DaoValue *value;
	DaoSocket *socket;
	FILE *file;
	if( DaoList_Size( list1 ) == 0 && DaoList_Size( list2 ) == 0 ){
		DaoProcess_RaiseError( proc, neterr, "Both read and write parameters are empty lists" );
		return;
	}
	FD_ZERO( &set1 );
	FD_ZERO( &set2 );
	for( i = 0; i < DaoList_Size( list1 ); i++ ){
		value = DaoList_GetItem( list1, i );
		if( DaoValue_CastStream( value ) ){
#ifdef WIN32
			DaoProcess_RaiseError( proc, neterr, "Selecting streams not supported on the current platform" );
			return;
#endif
			file = DaoStream_GetFile( DaoValue_CastStream( value ) );
			if( file == NULL ){
				DaoProcess_RaiseError( proc, neterr, "The read list contains a stream not associated with a file" );
				return;
			}
			FD_SET( fileno( file ), &set1 );
		}else{
			socket = (DaoSocket*)DaoValue_TryGetCdata( value );
			if( socket->id == -1 ){
				DaoProcess_RaiseError( proc, neterr, "The read list contains a closed socket" );
				return;
			}
			FD_SET( socket->id, &set1 );
		}
	}
	for( i = 0; i < DaoList_Size( list2 ); i++ ){
		value = DaoList_GetItem( list2, i );
		if( DaoValue_CastStream( value ) ){
#ifdef WIN32
			DaoProcess_RaiseError( proc, neterr, "Selecting streams not supported on the current platform" );
			return;
#endif
			file = DaoStream_GetFile( DaoValue_CastStream( value ) );
			if( file == NULL ){
				DaoProcess_RaiseError( proc, neterr, "The write list contains a stream not associated with a file" );
				return;
			}
			FD_SET( fileno( file ), &set2 );
		}else{
			socket = (DaoSocket*)DaoValue_TryGetCdata( value );
			if( socket->id == -1 ){
				DaoProcess_RaiseError( proc, neterr, "The write list contains a closed socket" );
				return;
			}
			FD_SET( socket->id, &set2 );
		}
	}
	tv.tv_sec = (int)DaoValue_TryGetFloat( par[2] );
	tv.tv_usec = ( DaoValue_TryGetFloat( par[2] )- tv.tv_sec ) * 1E6;
	if( select( FD_SETSIZE, &set1, &set2, NULL, & tv ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	value = (DaoValue*) DaoProcess_NewList( proc );
	DaoTuple_SetItem( tuple, value, 0 );
	reslist = DaoValue_CastList( DaoTuple_GetItem( tuple, 0 ) );
	for( i = 0; i < DaoList_Size( list1 ); i++ ){
		value = DaoList_GetItem( list1, i );
		if( DaoValue_CastStream( value ) ){
			if( FD_ISSET( fileno( DaoStream_GetFile( DaoValue_CastStream( value ) ) ), &set1 ) )
				DaoList_PushBack( reslist, value );
		}else if( FD_ISSET( ((DaoSocket*)DaoValue_TryGetCdata( value ))->id, &set1 ) )
			DaoList_PushBack( reslist, value );
	}
	value = (DaoValue*) DaoProcess_NewList( proc );
	DaoTuple_SetItem( tuple, value, 1 );
	reslist = DaoValue_CastList( DaoTuple_GetItem( tuple, 1 ) );
	for( i = 0; i < DaoList_Size( list2 ); i++ ){
		value = DaoList_GetItem( list2, i );
		if( DaoValue_CastStream( value ) ){
			if( FD_ISSET( fileno( DaoStream_GetFile( DaoValue_CastStream( value ) ) ), &set2 ) )
				DaoList_PushBack( reslist, value );
		}else if( FD_ISSET( ((DaoSocket*)DaoValue_TryGetCdata( value ))->id, &set2 ) )
			DaoList_PushBack( reslist, value );
	}
}

static DaoFuncItem netMeths[] =
{
	/*! Returns socket bound to \a port using \a address options if specified.
	 *
	 * Meaning of \a address values:
	 * -\c shared -- non-exclusive binding of the address and port, other sockets will be able to bind to the same address and port
	 * (SO_REUSEADDR on Unix, ignored on Windows)
	 * -\c exclusive -- exclusive binding of the address and port, no other sockets are allowed to rebind
	 * (SO_EXCLUSIVEADDRUSE on Windows, ignored on Unix)
	 * -\c reused -- rebinds the socket even if the address and port are already bound by another socket (non-exclusively)
	 * (SO_REUSEADDR on Windows, ignored on Unix)
	*/
	{  DaoNetLib_Bind,          "bind( port: int ) => socket" },
	{  DaoNetLib_Bind,          "bind( port: int, address: enum<shared;exclusive;reused> ) => socket" },

	/*! Returns socket connected to \a host : \a port */
	{  DaoNetLib_Connect,       "connect( host: string, port: int ) => socket" },
	{  DaoNetLib_Connect,       "connect( host: string, port: int ) => socket" },

	/*! Returns information for host with the given \a id, which may be either name or address */
	{  DaoNetLib_GetHost,       "host( id: string ) => tuple<name: string, aliases: list<string>, addresses: list<string>>" },

	/*! Returns information for TCP service with the given \a id, which may be either name or port */
	{  DaoNetLib_GetService,    "service( id: string|int ) => tuple<name: string, port: int, aliases: list<string>>" },

	/*! Waits \a timeout seconds for any object in \a read or \a write list to become available for reading or writing accordingly.
	 * Returns sub-lists of \a read and \a write containing available objects
	 * \warning On Windows, selecting streams is not supported	*/
	{  DaoNetLib_Select,
		"select( invar read: list<@X<io::stream|socket>>, invar write: list<@Y<io::stream|socket>>,"
				"timeout: float )=>tuple<read: list<@X>, write: list<@Y>>" },
	{ NULL, NULL }
};

void DaoNetwork_Init( DaoVmSpace *vms, DaoNamespace *ns )
{
#ifdef WIN32
	char errbuf[MAX_ERRMSG];
	WSADATA wsaData;   /*  if this doesn't work */
	/* WSAData wsaData; // then try this instead */

	if(WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
		strcpy( errbuf, "WSAStartup failed: " );
		GetErrorMessage( errbuf + strlen( errbuf ), GetError() );
		strcat( errbuf, "\n" );
		fprintf(stderr, errbuf );
		exit(1);
	} 
#endif
	NET_INIT();
}

DAO_DLL int DaoNet_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *netns = DaoNamespace_GetNamespace( ns, "net" );
	DaoNamespace_AddConstValue( ns, "net", (DaoValue*)netns );
	daox_type_socket = DaoNamespace_WrapType( netns, & socketTyper, 1 );
	DaoNamespace_WrapFunctions( netns, netMeths );
	DaoNetwork_Init( vmSpace, ns );
	return 0;
}
