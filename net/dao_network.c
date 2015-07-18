/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2015, Limin Fu
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
#include"limits.h"
#include"stdint.h"
#include"ctype.h"

#ifdef UNIX

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>

// for sendfile()
#ifdef LINUX
#define _FILE_OFFSET_BITS 64
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#else

#define NET_TRANS( st ) st
#define NET_INIT()

//#include"winsock2.h"
//#include"winsock.h"
#include"ws2tcpip.h"

#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))
#endif

typedef int socklen_t;
#define fileno _fileno
#define fdopen _fdopen

#endif


#include"dao.h"
#include"daoValue.h"
#include"daoStdtype.h"
#include"daoStream.h"
#include"daoThread.h"
#include"daoParser.h"

#ifdef UNIX

// gethostby* is not reentrant on Unix, synchronization is desirable
static DMutex net_mtx;
#define NET_TRANS( st ) DMutex_Lock( &net_mtx ); st; DMutex_Unlock( &net_mtx )
#define NET_INIT() DMutex_Init( &net_mtx )

#endif

// avoid SIGPIPE on broken connection
#ifdef MSG_NOSIGNAL
#define SEND_FLAGS MSG_NOSIGNAL
#else
#define SEND_FLAGS 0
#endif

#define DAOX_PACKET_MAX 0xfff

#define DAO_INT32      (END_SUB_TYPES+1)
#define DAO_INT64      (END_SUB_TYPES+2)
#define DAO_FLOAT32    (END_SUB_TYPES+3)
#define DAO_FLOAT64    (END_SUB_TYPES+4)
#define DAO_COMPLEX32  (END_SUB_TYPES+5)
#define DAO_COMPLEX64  (END_SUB_TYPES+6)


typedef uint32_t ipv4_t;
typedef uint16_t port_t;

typedef struct sockaddr_in  sockaddr_in;

typedef struct DaoDataPacket  DaoDataPacket;

struct DaoDataPacket
{
	uint8_t   type;     /* Value type; */
	uint8_t   subtype;  /* Value subtype; */
	uint8_t   count[2]; /* Byte count in ::data; */
	uint8_t   aux1[8];  /* Aux information for the value; */
	uint8_t   aux2[8];  /* Aux information for the value; */
	uint8_t   data[ DAOX_PACKET_MAX + 128 ];
};

#define null_packet  ((DaoDataPacket*)NULL)
static int packet_core_size = (char*) &null_packet->data - (char*) &null_packet->type;


static const char neterr[] = "Network";
static const char hosterr[] = "Network::Host";

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
	case ENOMEM:          strcpy( buffer, "Insufficient memory (ENOMEM)" ); break;
	case ENOTCONN:        strcpy( buffer, "Socket not connected (ENOTCONN)" ); break;
	case EIO:             strcpy( buffer, "Hardware I/O error (EIO)" ); break;
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
	Socket_DefaultAddress = 8,
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
int DaoNetwork_Bind( int type, ipv4_t ip, port_t port, socket_opts opts )
{
	struct sockaddr_in myaddr;    /*  my address information */
	int sockfd = socket( AF_INET, type, 0);
	if( sockfd == -1 ) return -1;
	if( opts && DaoNetwork_SetSocketOptions( sockfd, opts ) == -1 ) return -1;

	myaddr.sin_family = AF_INET;         /*  host byte order */
	myaddr.sin_port = htons( port );     /*  short, network byte order */
	myaddr.sin_addr.s_addr = ip;
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
int DaoNetwork_Accept( int sockfd, ipv4_t *ip, port_t *port )
{
	struct sockaddr_in addr;
	socklen_t size = sizeof(addr);
	int fd = accept( sockfd, (struct sockaddr*)&addr, &size );
	if ( fd == -1 )
		return -1;
	*port = ntohs( addr.sin_port );
	*ip = addr.sin_addr.s_addr;
	return fd;
}
int DaoNetwork_Connect( ipv4_t ip, port_t port )
{
	int sockfd;
	struct sockaddr_in addr;
	if( ( sockfd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1) return -1;

	addr.sin_family = AF_INET;    /*  host byte order */
	addr.sin_port = htons( port );  /*  short, network byte order */
	addr.sin_addr.s_addr = ip;

	if( connect( sockfd, (struct sockaddr *)& addr, sizeof(struct sockaddr)) == -1){
		DaoNetwork_Close( sockfd );
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

int LoopSend( int sockfd, char *buf, int size, sockaddr_in *addr )
{
	socklen_t addrlen = addr? sizeof(*addr) : 0;
	int left = size;
	daoint numbytes;
	do{
		numbytes = sendto( sockfd, buf, left, SEND_FLAGS, (struct sockaddr*)addr, addrlen );
		if(numbytes != -1){
			left -= numbytes;
			buf += numbytes;
		}
		else if( GetError() != INTERRUPTED )
			return -1;
	} while( left > 0 );
	return size;
}
int LoopReceive( int sockfd, char *buf, int size, int flags, sockaddr_in *addr )
{
	socklen_t addrlen = addr? sizeof(*addr) : 0;
	daoint numbytes;
	do{
		numbytes = recvfrom( sockfd, buf, size, flags, (struct sockaddr*)addr, &addrlen );
	} while( numbytes == -1 && GetError() == INTERRUPTED );
	return numbytes;
}
int DaoNetwork_Send( int sockfd, DString *buf )
{
	return LoopSend( sockfd, DString_GetData( buf ), DString_Size( buf ), NULL );
}
int DaoNetwork_SendTo( int sockfd, ipv4_t ip, port_t port, DString *buf )
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( port );
	addr.sin_addr.s_addr = ip;
	return LoopSend( sockfd, buf->chars, buf->size, &addr );
}
int DaoNetwork_Receive( int sockfd, DString *buf, int max )
{
	int numbytes;
	if ( max <= 0 )
		max = 4096;
	else if ( max > 65536 )
		max = 65536;
	DString_Resize( buf, max );
	numbytes = LoopReceive( sockfd, buf->chars, max, 0, NULL );
	if( numbytes >=0 ) DString_Resize( buf, numbytes );
	/* if( numbytes >=0 ) buf->size = numbytes; */
	return numbytes;
}
int DaoNetwork_ReceiveFrom( int sockfd, sockaddr_in *addr, DString *buf, int max )
{
	daoint numbytes;
	if ( max <= 0 )
		max = 4096;
	else if ( max > 65536 )
		max = 65536;
	DString_Resize( buf, max );
	numbytes = LoopReceive( sockfd, buf->chars, max, 0, addr );
	if( numbytes >=0 ) DString_Resize( buf, numbytes );
	/* if( numbytes >=0 ) buf->size = numbytes; */
	return numbytes;
}

static void DaoNetwork_EncodeUInt16( uint8_t data[], uint16_t value )
{
	uint8_t i, w = sizeof(uint16_t);
	for(i=0; i<w; ++i) data[i] = (value >> 8*(w-1-i)) & 0xFF;
}
static void DaoNetwork_EncodeInt32( uint8_t data[], int32_t value )
{
	uint8_t i, w = sizeof(int32_t);
	for(i=0; i<w; ++i) data[i] = (value >> 8*(w-1-i)) & 0xFF;
}
static void DaoNetwork_EncodeInt64( uint8_t data[], int64_t value )
{
	uint8_t i, w = sizeof(int64_t);
	for(i=0; i<w; ++i) data[i] = (value >> 8*(w-1-i)) & 0xFF;
}
static int DaoNetwork_BigEndianFloat()
{
	double inf = INFINITY;
	uint8_t *bytes = (uint8_t*) & inf;
	return bytes[0] != 0;
}
static void DaoNetwork_EncodeFloat32( uint8_t *data, float value )
{
	uint8_t i, w = sizeof(float);
	uint8_t *bytes = (uint8_t*) & value;
	if( DaoNetwork_BigEndianFloat() ){
		for(i=0; i<w; ++i) data[i] = bytes[i];
	}else{
		for(i=0; i<w; ++i) data[i] = bytes[w-1-i];
	}
}
static void DaoNetwork_EncodeFloat64( uint8_t *data, double value )
{
	uint8_t i, w = sizeof(double);
	uint8_t *bytes = (uint8_t*) & value;
	if( DaoNetwork_BigEndianFloat() ){
		for(i=0; i<w; ++i) data[i] = bytes[i];
	}else{
		for(i=0; i<w; ++i) data[i] = bytes[w-1-i];
	}
}
static void DaoNetwork_EncodeDaoInteger( uint8_t *data, dao_integer value )
{
	if( sizeof(dao_integer) == sizeof(int64_t) ){
		DaoNetwork_EncodeInt64( data, value );
	}else{
		DaoNetwork_EncodeInt32( data, value );
	}
}
static void DaoNetwork_EncodeDaoFloat( uint8_t *data, dao_float value )
{
	if( sizeof(dao_float) == sizeof(double) ){
		DaoNetwork_EncodeFloat64( data, value );
	}else{
		DaoNetwork_EncodeFloat32( data, value );
	}
}
static uint16_t DaoNetwork_DecodeUInt16( uint8_t *data )
{
	uint8_t i, w = sizeof(int16_t);
	uint16_t value = 0;
	for(i=0; i<w; ++i) value |= ((uint16_t)data[i]) << 8*(w-1-i);
	return (int16_t)value;
}
static int32_t DaoNetwork_DecodeInt32( uint8_t *data )
{
	uint8_t i, w = sizeof(int32_t);
	uint32_t value = 0;
	for(i=0; i<w; ++i) value |= ((uint32_t)data[i]) << 8*(w-1-i);
	return (int32_t)value;
}
static int64_t DaoNetwork_DecodeInt64( uint8_t *data )
{
	uint8_t i, w = sizeof(int64_t);
	uint64_t value = 0;
	for(i=0; i<w; ++i) value |= ((uint64_t)data[i]) << 8*(w-1-i);
	return (int64_t)value;
}
static dao_integer DaoNetwork_DecodeDaoInt( uint8_t *data, int width )
{
	if( width == sizeof(int64_t) ) return DaoNetwork_DecodeInt64( data );
	return DaoNetwork_DecodeInt32( data );
}
float DaoNetwork_DecodeFloat32( uint8_t *data )
{
	uint8_t i, w = sizeof(float);
	uint8_t bytes[sizeof(float)];
	if( DaoNetwork_BigEndianFloat() ) return *(float*) data;
	for(i=0; i<w; ++i) bytes[i] = data[w-1-i];
	return *(float*) bytes;
}
double DaoNetwork_DecodeFloat64( uint8_t *data )
{
	uint8_t i, w = sizeof(double);
	uint8_t bytes[sizeof(double)];
	if( DaoNetwork_BigEndianFloat() ) return *(double*) data;
	for(i=0; i<w; ++i) bytes[i] = data[w-1-i];
	return *(double*) bytes;
}
static dao_float DaoNetwork_DecodeDaoFloat( uint8_t *data, int width )
{
	if( width == sizeof(double) ) return DaoNetwork_DecodeFloat64( data );
	return DaoNetwork_DecodeFloat32( data );
}
static dao_complex DaoNetwork_DecodeComplex( uint8_t *data, int width )
{
	dao_complex res;
	if( width == sizeof(double) ){
		res.real = DaoNetwork_DecodeFloat64( data );
		res.imag = DaoNetwork_DecodeFloat64( data + width );
	}else{
		res.real = DaoNetwork_DecodeFloat32( data );
		res.imag = DaoNetwork_DecodeFloat32( data + width );
	}
	return res;
}
static int DaoNetwork_SendEnd( int sockfd, sockaddr_in *addr )
{
	DaoDataPacket packet = {0};
	packet.type = 0xff;
	DaoNetwork_EncodeUInt16( packet.count, 0 );
	return LoopSend( sockfd, (char*)&packet, packet_core_size, addr );
}
static int DaoNetwork_SendNone( int sockfd, sockaddr_in *addr )
{
	DaoDataPacket packet = {0};
	packet.type = DAO_NONE;
	DaoNetwork_EncodeUInt16( packet.count, 0 );
	return LoopSend( sockfd, (char*)&packet, packet_core_size, addr );
}
static int DaoNetwork_SendBoolean( int sockfd, sockaddr_in *addr, dao_boolean value )
{
	DaoDataPacket packet = {0};

	packet.type = DAO_INTEGER;
	packet.data[0] = value;
	DaoNetwork_EncodeUInt16( packet.count, 1 );
	return LoopSend( sockfd, (char*)&packet, packet_core_size + 1, addr );
}
static int DaoNetwork_SendInteger( int sockfd, sockaddr_in *addr, dao_integer value )
{
	DaoDataPacket packet = {0};

	packet.type = DAO_INTEGER;
	packet.subtype = sizeof(dao_integer);
	DaoNetwork_EncodeUInt16( packet.count, packet.subtype );
	DaoNetwork_EncodeDaoInteger( packet.data, value );
	return LoopSend( sockfd, (char*)&packet, packet_core_size + packet.subtype, addr );
}
static int DaoNetwork_SendFloat( int sockfd, sockaddr_in *addr, dao_float value )
{
	DaoDataPacket packet = {0};

	packet.type = DAO_FLOAT;
	packet.subtype = sizeof(dao_float);
	DaoNetwork_EncodeUInt16( packet.count, packet.subtype );
	DaoNetwork_EncodeDaoFloat( packet.data, value );
	return LoopSend( sockfd, (char*)&packet, packet_core_size + packet.subtype, addr );
}
static int DaoNetwork_SendComplex( int sockfd, sockaddr_in *addr, dao_complex value )
{
	DaoDataPacket packet = {0};

	packet.type = DAO_COMPLEX;
	packet.subtype = sizeof(dao_float);
	DaoNetwork_EncodeDaoFloat( packet.data, value.real );
	DaoNetwork_EncodeDaoFloat( packet.data + packet.subtype, value.imag );
	DaoNetwork_EncodeUInt16( packet.count, 2*packet.subtype );
	return LoopSend( sockfd, (char*)&packet, packet_core_size + 2*packet.subtype, addr );
}
static int DaoNetwork_SendString( int sockfd, sockaddr_in *addr, DString *value )
{
	DaoDataPacket packet = {0};
	uint32_t length = 0;
	daoint shift = 0;

	packet.type = DAO_STRING;
	while( shift < value->size ){
		int copy = (value->size - shift);
		if( copy >= DAOX_PACKET_MAX ) copy = DAOX_PACKET_MAX;
		length = packet_core_size + copy;
		DaoNetwork_EncodeUInt16( packet.count, copy );
		DaoNetwork_EncodeInt64( packet.aux1, value->size );
		DaoNetwork_EncodeInt64( packet.aux2, shift );
		memcpy( packet.data, value->chars + shift, copy );
		if( LoopSend( sockfd, (char*) &packet, length, addr ) == -1 ) return -1;
		shift += copy;
	}
	return 0;
}
static int DaoNetwork_SendArray( int sockfd, sockaddr_in *addr, DaoArray *array )
{
	DaoDataPacket packet = {0};
	uint8_t *buffer = packet.data;
	uint8_t numtype = array->etype;
	uint32_t length = 0;
	daoint j;

	packet.type = DAO_ARRAY;
	packet.subtype = numtype;
	DaoNetwork_EncodeInt64( packet.aux1, array->ndim );
	DaoNetwork_EncodeInt64( packet.aux2, sizeof(dao_integer) );

	length = array->ndim * sizeof(dao_integer);
	if( length >= DAOX_PACKET_MAX ) return -1; // TODO: error, too many dimensions;

	for(j=0; j<array->ndim; j++){
		DaoNetwork_EncodeDaoInteger( packet.data + j*sizeof(dao_integer), array->dims[j] );
	}
	DaoNetwork_EncodeUInt16( packet.count, length );
	if( LoopSend( sockfd, (char*) &packet, packet_core_size+length, addr ) == -1 ) return -1;

	length = 0;
	buffer = packet.data;
	DaoNetwork_EncodeInt64( packet.aux1, array->size );
	DaoNetwork_EncodeInt64( packet.aux2, 0 );
	if( numtype == DAO_BOOLEAN ){
		dao_boolean *vec = array->data.b;
		for(j=0; j<array->size; j++){
			buffer[0] = vec[j];
			length += sizeof(dao_boolean);
			buffer += sizeof(dao_boolean);
			if( length >= DAOX_PACKET_MAX ){
				int count = packet_core_size + length;
				DaoNetwork_EncodeUInt16( packet.count, length );
				if( LoopSend( sockfd, (char*) &packet, count, addr ) == -1 ) return -1;
				length = 0;
				buffer = packet.data;
				DaoNetwork_EncodeInt64( packet.aux2, j+1 );
			}
		}
	}else if( numtype == DAO_INTEGER ){
		dao_integer *vec = array->data.i;
		packet.subtype = sizeof(dao_integer) == 4 ? DAO_INT32 : DAO_INT64;
		for(j=0; j<array->size; j++){
			DaoNetwork_EncodeDaoInteger( buffer, vec[j] );
			length += sizeof(dao_integer);
			buffer += sizeof(dao_integer);
			if( length >= DAOX_PACKET_MAX ){
				int count = packet_core_size + length;
				DaoNetwork_EncodeUInt16( packet.count, length );
				if( LoopSend( sockfd, (char*) &packet, count, addr ) == -1 ) return -1;
				length = 0;
				buffer = packet.data;
				DaoNetwork_EncodeInt64( packet.aux2, j+1 );
			}
		}
	}else if( numtype == DAO_FLOAT ){
		dao_float *vec = array->data.f;
		packet.subtype = sizeof(dao_float) == 4 ? DAO_FLOAT32 : DAO_FLOAT64;
		for(j=0; j<array->size; j++){
			DaoNetwork_EncodeDaoFloat( buffer, vec[j] );
			length += sizeof(dao_float);
			buffer += sizeof(dao_float);
			if( length >= DAOX_PACKET_MAX ){
				int count = packet_core_size + length;
				DaoNetwork_EncodeUInt16( packet.count, length );
				if( LoopSend( sockfd, (char*) &packet, count, addr ) == -1 ) return -1;
				length = 0;
				buffer = packet.data;
				DaoNetwork_EncodeInt64( packet.aux2, j+1 );
			}
		}
	}else if( numtype == DAO_COMPLEX ){
		dao_complex *vec = array->data.c;
		packet.subtype = sizeof(dao_complex) == 8 ? DAO_COMPLEX32 : DAO_COMPLEX64;
		for(j=0; j<array->size; j++){
			DaoNetwork_EncodeDaoFloat( buffer, vec[j].real );
			DaoNetwork_EncodeDaoFloat( buffer+sizeof(dao_float), vec[j].imag );
			length += sizeof(dao_complex);
			buffer += sizeof(dao_complex);
			if( length >= DAOX_PACKET_MAX ){
				int count = packet_core_size + length;
				DaoNetwork_EncodeUInt16( packet.count, length );
				if( LoopSend( sockfd, (char*) &packet, count, addr ) == -1 ) return -1;
				length = 0;
				buffer = packet.data;
				DaoNetwork_EncodeInt64( packet.aux2, j+1 );
			}
		}
	}
	DaoNetwork_EncodeUInt16( packet.count, length );
	return LoopSend( sockfd, (char*) &packet, packet_core_size + length, addr );
}

static int DaoNetwork_SendDao( int sockfd, sockaddr_in *addr, DaoValue *value, DaoType *type );

static DaoType* DaoType_GetFieldType( DaoType *self, int i )
{
	DaoType *itype = NULL;
	if( i < self->nested->size ){
		itype = self->nested->items.pType[i];
		if( itype->tid >= DAO_PAR_NAMED && itype->tid < DAO_PAR_VALIST ){
			itype = (DaoType*) itype->aux;
		}
	}else if( self->variadic ){
		itype = (DaoType*) self->nested->items.pType[self->nested->size-1]->aux;
	}
	return itype;
}

static int DaoNetwork_SendTuple( int sockfd, sockaddr_in *addr, DaoTuple *tuple, DaoType *type )
{
	DaoDataPacket packet = {0};
	int i;

	packet.type = DAO_TUPLE;
	packet.subtype = tuple->ctype != type;
	DaoNetwork_EncodeInt64( packet.aux1, tuple->size );
	DaoNetwork_EncodeUInt16( packet.count, 0 );

	if( LoopSend( sockfd, (char*) &packet, packet_core_size, addr ) == -1 ) return -1;

	/*
	// If this tuple is an element of a container, the "type" will be the proper
	// nominative type for this tuple. If the exact type of this tuple is the same
	// as the nominative type, we can skip serializing this type name, since it
	// is already serialized.
	*/
	if( packet.subtype ){
		if( DaoNetwork_SendString( sockfd, addr, tuple->ctype->name ) == -1 ) return -1;
	}
	for(i=0; i<tuple->size; ++i){
		DaoType *itype = DaoType_GetFieldType( tuple->ctype, i );
		if( DaoNetwork_SendDao( sockfd, addr, tuple->values[i], itype ) == -1 ) return -1;
	}
	return 0;
}
static int DaoNetwork_SendList( int sockfd, sockaddr_in *addr, DaoList *list, DaoType *type )
{
	DaoDataPacket packet = {0};
	DaoType *itype;
	int i;

	packet.type = DAO_LIST;
	packet.subtype = list->ctype != type;
	DaoNetwork_EncodeInt64( packet.aux1, list->value->size );
	DaoNetwork_EncodeUInt16( packet.count, 0 );

	if( LoopSend( sockfd, (char*) &packet, packet_core_size, addr ) == -1 ) return -1;
	if( packet.subtype ){
		if( DaoNetwork_SendString( sockfd, addr, list->ctype->name ) == -1 ) return -1;
	}
	itype = list->ctype->nested->items.pType[0];
	for(i=0; i<list->value->size; ++i){
		if( DaoNetwork_SendDao( sockfd, addr, list->value->items.pValue[i], itype ) == -1 ){
			return -1;
		}
	}
	return 0;
}
static int DaoNetwork_SendMap( int sockfd, sockaddr_in *addr, DaoMap *map, DaoType *type )
{
	DaoDataPacket packet = {0};
	DaoType *keytype;
	DaoType *valtype;
	DNode *it;

	packet.type = DAO_MAP;
	packet.subtype = map->ctype != type;
	DaoNetwork_EncodeInt64( packet.aux1, map->value->size );
	DaoNetwork_EncodeInt64( packet.aux2, map->value->hashing );
	DaoNetwork_EncodeUInt16( packet.count, 0 );

	if( LoopSend( sockfd, (char*) &packet, packet_core_size, addr ) == -1 ) return -1;
	if( packet.subtype ){
		if( DaoNetwork_SendString( sockfd, addr, map->ctype->name ) == -1 ) return -1;
	}
	keytype = map->ctype->nested->items.pType[0];
	valtype = map->ctype->nested->items.pType[1];
	for(it=DMap_First(map->value); it; it=DMap_Next(map->value,it)){
		if( DaoNetwork_SendDao( sockfd, addr, it->key.pValue, keytype ) == -1 ) return -1;
		if( DaoNetwork_SendDao( sockfd, addr, it->value.pValue, valtype ) == -1 ) return -1;
	}
	return 0;
}
static int DaoNetwork_SendDao( int sockfd, sockaddr_in *addr, DaoValue *value, DaoType *type )
{
	switch( DaoValue_Type( value ) ){
	case DAO_NONE    : return DaoNetwork_SendNone( sockfd, addr );
	case DAO_BOOLEAN : return DaoNetwork_SendBoolean( sockfd, addr, value->xBoolean.value );
	case DAO_INTEGER : return DaoNetwork_SendInteger( sockfd, addr, value->xInteger.value );
	case DAO_FLOAT   : return DaoNetwork_SendFloat( sockfd, addr, value->xFloat.value );
	case DAO_COMPLEX : return DaoNetwork_SendComplex( sockfd, addr, value->xComplex.value );
	case DAO_STRING  : return DaoNetwork_SendString( sockfd, addr, value->xString.value );
	case DAO_ARRAY   : return DaoNetwork_SendArray( sockfd, addr, (DaoArray*) value );
	case DAO_TUPLE   : return DaoNetwork_SendTuple( sockfd, addr, (DaoTuple*) value, type );
	case DAO_LIST    : return DaoNetwork_SendList( sockfd, addr, (DaoList*) value, type );
	case DAO_MAP     : return DaoNetwork_SendMap( sockfd, addr, (DaoMap*) value, type );
	default : break;
	}
	return -1;
}
static int DaoNetwork_ReceivePacket( int sockfd, sockaddr_in *addr, DaoDataPacket *packet )
{
	int numbytes = LoopReceive( sockfd, (char*) packet, packet_core_size, 0, addr );

	if( numbytes == -1 ) return 0;
	if( packet->type == 0xff ) return 1;

	numbytes = DaoNetwork_DecodeUInt16( packet->count );
	numbytes = LoopReceive( sockfd, (char*) packet->data, numbytes, 0, addr );
	if( numbytes == -1 ) return 0;
	return 1;
}

typedef struct DaoNetObjBuffer DaoNetObjBuffer;
struct DaoNetObjBuffer
{
	DaoProcess    *process;
	DaoNamespace  *nameSpace;
	DaoType       *type;
	DaoBoolean    *bnumber;
	DaoInteger    *inumber;
	DaoFloat      *fnumber;
	DaoComplex    *cnumber;
	DaoString     *string;
};

static DaoValue* DaoNetwork_ReceiveDao( int sockfd, sockaddr_in *addr,  DaoNetObjBuffer *obuffer )
{
	DaoProcess *proc = obuffer->process;
	DaoNamespace *ns = obuffer->nameSpace;
	DaoDataPacket packet = {0};
	DaoValue *value;
	DaoString *string;
	DaoArray *array;
	DaoTuple *tuple;
	DaoList *list;
	DaoMap *map;
	DaoType *type;
	DaoType *itype;
	DaoType *keytype;
	DaoType *valtype;
	dao_complex cvalue;
	daoint size, offset;
	int i, count, subtype;
	int width, hashing;

	if( DaoNetwork_ReceivePacket( sockfd, addr, & packet ) == 0 ) return NULL;

	switch( packet.type ){
	case DAO_BOOLEAN :
		/*
		// Since primitive values get copied when assigned or moved,
		// so reusing a single primitive value for deserialization
		// may save a lot of memory allocations.
		*/
		if( obuffer->bnumber == NULL ) obuffer->bnumber = DaoProcess_NewBoolean( proc, 0 );
		obuffer->bnumber->value = packet.data[0];
		return (DaoValue*) obuffer->bnumber;
	case DAO_INTEGER :
		if( obuffer->inumber == NULL ) obuffer->inumber = DaoProcess_NewInteger( proc, 0 );
		obuffer->inumber->value = DaoNetwork_DecodeDaoInt( packet.data, packet.subtype );
		return (DaoValue*) obuffer->inumber;
	case DAO_FLOAT :
		if( obuffer->fnumber == NULL ) obuffer->fnumber = DaoProcess_NewFloat( proc, 0.0 );
		obuffer->fnumber->value = DaoNetwork_DecodeDaoFloat( packet.data, packet.subtype );
		return (DaoValue*) obuffer->fnumber;
	case DAO_COMPLEX :
		cvalue = DaoNetwork_DecodeComplex( packet.data, packet.subtype );
		if( obuffer->cnumber == NULL ){
			obuffer->cnumber = DaoProcess_NewComplex( proc, cvalue );
		}else{
			obuffer->cnumber->value = cvalue;
		}
		return (DaoValue*) obuffer->cnumber;
	case DAO_STRING :
		string = DaoProcess_NewString( proc, NULL, 0 );
		size = DaoNetwork_DecodeInt64( packet.aux1 );
		while( packet.type == DAO_STRING && string->value->size < size ){
			count = DaoNetwork_DecodeUInt16( packet.count );
			DString_AppendBytes( string->value, (char*)packet.data, count );
			if( string->value->size >= size ) break;
			if( DaoNetwork_ReceivePacket( sockfd, addr, & packet ) == 0 ) return NULL;
		}
		if( string->value->size != size ) return NULL; // TODO;
		return (DaoValue*) string;
	case DAO_ARRAY :
		count = DaoNetwork_DecodeInt64( packet.aux1 );
		size  = DaoNetwork_DecodeInt64( packet.aux2 );
		array = DaoProcess_NewArray( proc, packet.subtype );
		DaoArray_SetDimCount( array, count );
		for(i=0; i<count; ++i){
			array->dims[i] = DaoNetwork_DecodeDaoInt( packet.data + i*size, size );
		}
		DaoArray_ResizeArray( array, array->dims, count );

		if( DaoNetwork_ReceivePacket( sockfd, addr, & packet ) == 0 ) return NULL;
		offset = 0;
		width = 1;
		switch( packet.subtype ){
		case DAO_INT32     :
		case DAO_FLOAT32   :
		case DAO_COMPLEX32 : width = 4; break;
		case DAO_INT64     :
		case DAO_FLOAT64   :
		case DAO_COMPLEX64 : width = 8; break;
		}
		size = DaoNetwork_DecodeInt64( packet.aux1 );
		while( packet.type == DAO_ARRAY && offset < size ){
			count = DaoNetwork_DecodeUInt16( packet.count );
			offset = DaoNetwork_DecodeInt64( packet.aux2 );
			for(i=0; i<count; i+=width, offset+=1){
				switch( array->etype ){
				case DAO_BOOLEAN :
					array->data.b[offset] = packet.data[i];
					break;
				case DAO_INTEGER :
					array->data.i[offset] = DaoNetwork_DecodeDaoInt( packet.data+i, width );
					break;
				case DAO_FLOAT :
					array->data.f[offset] = DaoNetwork_DecodeDaoFloat( packet.data+i, width );
					break;
				case DAO_COMPLEX :
					array->data.c[offset] = DaoNetwork_DecodeComplex( packet.data+i, width );
					i += width;
					break;
				}
			}
			if( offset >= size ) break;
			if( DaoNetwork_ReceivePacket( sockfd, addr, & packet ) == 0 ) return NULL;
		}
		if( offset != size ) return NULL; // TODO;
		return (DaoValue*) array;
	case DAO_TUPLE :
		size = DaoNetwork_DecodeInt64( packet.aux1 );
		type = obuffer->type; /* Possible nominative type from parent container; */
		if( packet.subtype ){ /* Serialized type name is present, use it: */
			string = (DaoString*) DaoNetwork_ReceiveDao( sockfd, addr, obuffer );
			if( string == NULL || string->type != DAO_STRING ) return NULL; // TODO;
			type = DaoParser_ParseTypeName( string->value->chars, ns, NULL );
		}
		tuple = DaoTuple_Create( type, size, 0 );
		DaoProcess_CacheValue( proc, (DaoValue*) tuple );
		for(i=0; i<size; ++i){
			obuffer->type = DaoType_GetFieldType( tuple->ctype, i );
			value = DaoNetwork_ReceiveDao( sockfd, addr, obuffer );
			if( value == NULL ) return NULL; // TODO;
			DaoTuple_SetItem( tuple, value, i );
		}
		return (DaoValue*) tuple;
	case DAO_LIST :
		size = DaoNetwork_DecodeInt64( packet.aux1 );
		type = obuffer->type;
		if( packet.subtype ){
			string = (DaoString*) DaoNetwork_ReceiveDao( sockfd, addr, obuffer );
			if( string == NULL || string->type != DAO_STRING ) return NULL; // TODO;
			type = DaoParser_ParseTypeName( string->value->chars, ns, NULL );
		}
		list = DaoProcess_NewList( proc );
		DaoList_SetType( list, type );
		for(i=0; i<size; ++i){
			obuffer->type = type->nested->items.pType[0];
			value = DaoNetwork_ReceiveDao( sockfd, addr, obuffer );
			if( value == NULL ) return NULL; // TODO;
			DList_Append( list->value, value );
		}
		return (DaoValue*) list;
	case DAO_MAP :
		size    = DaoNetwork_DecodeInt64( packet.aux1 );
		hashing = DaoNetwork_DecodeInt64( packet.aux2 );
		type = obuffer->type;
		if( packet.subtype ){
			string = (DaoString*) DaoNetwork_ReceiveDao( sockfd, addr, obuffer );
			if( string == NULL || string->type != DAO_STRING ) return NULL; // TODO;
			type = DaoParser_ParseTypeName( string->value->chars, ns, NULL );
		}
		map = DaoProcess_NewMap( proc, hashing );
		DaoMap_SetType( map, type );
		for(i=0; i<size; ++i){
			DaoValue *key, *value;
			obuffer->type = type->nested->items.pType[0];
			key = DaoNetwork_ReceiveDao( sockfd, addr, obuffer );
			if( key == NULL ) return NULL; // TODO;
			obuffer->type = type->nested->items.pType[1];
			value = DaoNetwork_ReceiveDao( sockfd, addr, obuffer );
			if( value == NULL ) return NULL; // TODO;
			DMap_Insert( map->value, key, value );
		}
		return (DaoValue*) map;
	case DAO_NONE : return dao_none_value;
	}
	return NULL;
}




enum {
	Shutdown_None = 0,
	Shutdown_Receive = 1,
	Shutdown_Send = 2,
};

typedef short shutdown_flag;

int DaoNetwork_Shutdown( int sockfd, shutdown_flag flag )
{
	int sf;
	if ( ( flag & Shutdown_Receive ) && ( flag & Shutdown_Send ) )
		sf = 2;
	else if ( flag & Shutdown_Send )
		sf = 1;
	else
		sf = 0;
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


#define MAX_ERRMSG 100

enum {
	Socket_Closed = 0,
	Socket_Bound,
	Socket_Connected,
	Socket_Listening
};

enum {
	Proto_TCP = SOCK_STREAM,
	Proto_UDP = SOCK_DGRAM
};

typedef int socket_proto;
typedef short socket_state;
typedef struct DaoSocket DaoSocket;
typedef struct DaoIpv4Addr DaoIpv4Addr;
typedef struct DaoSocketAddr DaoSocketAddr;

struct DaoSocket
{
	int id;
	socket_state state;
	shutdown_flag shutflag;
};

struct DaoIpv4Addr {
	uint8_t octets[4];
};

struct DaoSocketAddr {
	uint8_t ip[4];
	port_t port;
};

extern DaoTypeBase socketTyper;
DaoType *daox_type_socket = NULL;
DaoType *daox_type_tcpstream = NULL;
DaoType *daox_type_tcplistener = NULL;
DaoType *daox_type_udpsocket = NULL;
DaoType *daox_type_ipv4addr = NULL;
DaoType *daox_type_sockaddr = NULL;

static DaoSocket* DaoSocket_New(  )
{
	DaoSocket *self = dao_malloc( sizeof(DaoSocket) );
	self->id = -1;
	self->state = Socket_Closed;
	self->shutflag = Shutdown_None;
	return self;
}

static int DaoSocket_Shutdown( DaoSocket *self, shutdown_flag flag )
{
	if( self->id != -1 ){
		if ( DaoNetwork_Shutdown( self->id, flag ) != 0 )
			return -1;
		self->shutflag = flag;
	}
	return 0;
}

static void DaoSocket_Close( DaoSocket *self )
{
	if( self->id != -1 ){
		DaoNetwork_Close( self->id );
		self->id = -1;
		self->state = Socket_Closed;
		self->shutflag = Shutdown_None;
	}
}

static void DaoSocket_Delete( DaoSocket *self )
{
	DaoSocket_Close( self );
	dao_free( self );
}

int GetIpAddr( const char *str, ipv4_t *ip )
{
#ifdef WIN32
		int value = inet_addr( str );
		if ( value == INADDR_NONE )
			return 0;
		*ip = value;
		return 1;
#else
		return inet_pton( AF_INET, str, ip );
#endif
}

int ParseAddr( DaoProcess *proc, DString *addr, ipv4_t *ip, port_t *port, int hostname )
{
	daoint pos = DString_RFindChar( addr, ':', addr->size - 1 );
	unsigned long long num;
	char buf[50] = {0};
	char *ptr;
	if ( pos < 0 || pos == addr->size - 1 ){
		DaoProcess_RaiseError( proc, "Param", "Missing port number" );
		return 0;
	}
	if ( strchr( "+-", addr->chars[pos + 1] ) || isspace( addr->chars[pos + 1] ) ){
		DaoProcess_RaiseError( proc, "Param", "Invalid port number" );
		return 0;
	}
	num = strtoull( addr->chars + pos + 1, &ptr, 10 );
	if ( *ptr != '\0' || ( num == ULLONG_MAX && errno == ERANGE ) || num > 0xFFFF ){
		DaoProcess_RaiseError( proc, "Param", "Invalid port number" );
		return 0;
	}
	*port = num;
	if ( pos > sizeof(buf) ){
		DaoProcess_RaiseError( proc, "Param", "Invalid IP address" );
		return 0;
	}
	if ( pos == 0 ){
		*ip = 0;
		return 1;
	}
	strncpy( buf, addr->chars, pos );
	if ( hostname ){
		struct hostent *he;
		NET_TRANS( he = gethostbyname( buf ) );
		if ( he == NULL){
			char errbuf[MAX_ERRMSG];
			GetHostErrorMessage( errbuf, GetHostError() );
			DaoProcess_RaiseError( proc, hosterr, errbuf );
			return 0;
		}
		*ip =  *(ipv4_t*)he->h_addr;
	}
	else if ( !GetIpAddr( buf, ip ) ){
		DaoProcess_RaiseError( proc, "Param", "Invalid IP address" );
		return 0;
	}
	return 1;
}

static int DaoSocket_Bind( DaoSocket *self, socket_proto proto, ipv4_t ip, port_t port, socket_opts opts )
{
	DaoSocket_Close( self );
	self->id = DaoNetwork_Bind( proto, ip, port, opts );
	if( self->id != -1 )
		self->state = Socket_Bound;
	return self->id;
}

static int DaoSocket_Connect( DaoSocket *self, ipv4_t ip, port_t port )
{
	DaoSocket_Close( self );
	self->id = DaoNetwork_Connect( ip, port );
	if( self->id >= 0 )
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

int CheckPort( DaoProcess *proc, dao_integer port )
{
	if ( port < 0 || port > 0xFFFF ){
		DaoProcess_RaiseError( proc, "Param", "Invalid port number" );
		return 0;
	}
	return 1;
}

int ExtractAddr( DaoProcess *proc, DaoValue *param, ipv4_t *ip, port_t *port )
{
	if ( param->type == DAO_STRING ){
		if ( !ParseAddr( proc, param->xString.value, ip, port, 1 ) )
			return 0;
	}
	else {
		DaoSocketAddr *addr = (DaoSocketAddr*)DaoValue_TryGetCdata( param );
		*ip = *(ipv4_t*)addr->ip;
		*port = addr->port;
	}
	return 1;
}

static void DaoSocket_Lib_Listen( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	ipv4_t ip;
	port_t port;
	dao_integer backlog = par[2]->xInteger.value;
	if ( !ExtractAddr( proc, par[1], &ip, &port ) )
		return;
	if ( backlog <= 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid backlog value" );
		return;
	}
	if( DaoSocket_Bind( self, Proto_TCP, ip, port, par[3]->xEnum.value ) == -1 || DaoNetwork_Listen( self->id, backlog ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
	self->state = Socket_Listening;
}

static void DaoSocket_Lib_Bind( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	ipv4_t ip;
	port_t port;
	if ( !ExtractAddr( proc, par[1], &ip, &port ) )
		return;
	if( DaoSocket_Bind( self, Proto_UDP, ip, port, par[2]->xEnum.value ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_Accept( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	DaoTuple *tup = DaoProcess_PutTuple( proc, 2 );
	DaoSocket *sock;
	DaoSocketAddr *addr;
	if( self->state != Socket_Listening ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not in the listening state" );
		return;
	}
	sock = DaoSocket_New(  );
	addr = (DaoSocketAddr*)dao_malloc( sizeof(DaoSocketAddr) );
	DaoTuple_SetItem( tup, (DaoValue*)DaoProcess_NewCdata( proc, daox_type_tcpstream, sock, 1 ), 0 );
	DaoTuple_SetItem( tup, (DaoValue*)DaoProcess_NewCdata( proc, daox_type_sockaddr, addr, 1 ), 1 );
	sock->id = DaoNetwork_Accept( self->id, (ipv4_t*)addr->ip, &addr->port );
	if( sock->id == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	sock->state = Socket_Connected;
}

static void DaoSocket_Lib_Connect( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	ipv4_t ip;
	port_t port;
	if ( !ExtractAddr( proc, par[1], &ip, &port ) )
		return;
	if ( DaoSocket_Connect( self, ip, port ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_Send( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	if( DaoNetwork_Send( self->id, DaoString_Get( DaoValue_CastString( par[1] ) ) ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

#ifndef MAX_PATH
#ifndef PATH_MAX
#define MAX_PATH 512
#else
#define MAX_PATH PATH_MAX
#endif
#endif

static void DaoSocket_Lib_SendFile( DaoProcess *proc, DaoValue *par[], int N  )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	DString *fname = par[1]->xString.value;
#ifdef LINUX
	struct stat finfo;
	int file, res;
#else
	FILE *file;
	char buf[4096];
#endif
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
#ifdef LINUX
	file = open( fname->chars, O_RDONLY, 0 );
	if ( file < 0 ){
#else
	file = fopen( fname->chars, "rb" );
	if ( !file ){
#endif
		char errbuf[MAX_ERRMSG + MAX_PATH];
		snprintf( errbuf, sizeof(errbuf), "Failed to open file: %s", fname->chars );
		DaoProcess_RaiseError( proc, "File", errbuf );
		return;
	}
#ifdef LINUX
	if ( fstat( file, &finfo ) < 0 ){
		DaoProcess_RaiseError( proc, "File", "Failed to get file size" );
		close( file );
		return;
	}
	res = sendfile( self->id, file, NULL, finfo.st_size );
	close( file );
	if ( res < 0 )
		goto Error;
#else
	while ( !feof( file ) ){
		int count = fread( buf, 1, sizeof(buf), file );
		if ( !count ){
			DaoProcess_RaiseError( proc, "File", "File read failure" );
			fclose( file );
			return;
		}
		if ( LoopSend( self->id, buf, count, NULL ) < count ){
			fclose( file );
			goto Error;
		}
	}
	fclose( file );
#endif
	return;
Error:
	if ( 1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
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

static void DaoSocket_Lib_ReceiveAll( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	daoint count = par[1]->xInteger.value;
	DString *prep = par[2]->xString.value;
	DString *res;
	char *start;
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	if ( count < 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid byte count" );
		return;
	}
	if ( count > 1048576 ){
		DaoProcess_RaiseError( proc, "Param", "Byte count too large (limited to 1048576)" );
		return;
	}
	res = DaoProcess_PutChars( proc, "" );
	if ( prep->size ){
		DString_Append( res, prep );
		count += prep->size;
		if ( count < 0 ){
			DaoProcess_RaiseError( proc, "Param", "Resulting string length too large" );
			return;
		}
	}
	DString_Resize( res, count );
	start = res->chars + prep->size;
	while ( 1 ){
		int numbytes = LoopReceive( self->id, start, count, 0, NULL );
		if ( numbytes == -1 ){
			GetErrorMessage( errbuf, GetError() );
			DaoProcess_RaiseError( proc, neterr, errbuf );
			return;
		}
		if ( !numbytes ){
			DString_Resize( res, res->size - count );
			break;
		}
		start += numbytes;
		count -= numbytes;
		if ( !count )
			break;
	}
}

static void DaoSocket_Lib_Serialize( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	int n;
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	n = DaoNetwork_SendDao( self->id, NULL, par[1], NULL );
	if( n == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
}

static void DaoSocket_Lib_Deserialize( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	DaoNetObjBuffer obuffer = {NULL};
	DaoValue *value;
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	obuffer.process = proc;
	obuffer.nameSpace = proc->topFrame->routine->nameSpace;
	value = DaoNetwork_ReceiveDao( self->id, NULL, & obuffer );
	if( value == NULL ){ // TODO:
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	DaoProcess_PutValue( proc, value );
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
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	DaoSocketAddr *saddr;
	char errbuf[MAX_ERRMSG];
	struct sockaddr_in addr;
	socklen_t size = sizeof( struct sockaddr_in );
	if( self->state != Socket_Connected ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not connected" );
		return;
	}
	if( getpeername( self->id, (struct sockaddr *) & addr, & size ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	saddr = (DaoSocketAddr*)dao_malloc( sizeof(DaoSocketAddr) );
	*(ipv4_t*)saddr->ip = addr.sin_addr.s_addr;
	saddr->port = ntohs( addr.sin_port );
	DaoProcess_PutCdata( proc, saddr, daox_type_sockaddr );
}

static void DaoSocket_Lib_GetSockName( DaoProcess *proc, DaoValue *par[], int N  )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	DaoSocketAddr *saddr;
	char errbuf[MAX_ERRMSG];
	struct sockaddr_in addr;
	socklen_t size = sizeof( struct sockaddr_in );
	if( getsockname( self->id, (struct sockaddr *) & addr, & size ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	saddr = (DaoSocketAddr*)dao_malloc( sizeof(DaoSocketAddr) );
	*(ipv4_t*)saddr->ip = addr.sin_addr.s_addr;
	saddr->port = ntohs( addr.sin_port );
	DaoProcess_PutCdata( proc, saddr, daox_type_sockaddr );
}

#if 0
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
#endif
static void DaoSocket_Lib_GetDescriptor( DaoProcess *proc, DaoValue *par[], int N  )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	DaoProcess_PutInteger( proc, self->id );
}

static void DaoSocket_Lib_Check( DaoProcess *proc, DaoValue *par[], int N  )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( par[0] );
	int res = 0, open = ( self->state == Socket_Bound || self->state == Socket_Connected );
	switch ( par[1]->xEnum.value ){
	case 0:	res = open && !( self->shutflag & Shutdown_Receive ); break;
	case 1: res = open && !( self->shutflag & Shutdown_Send ); break;
	case 2:	res = open; break;
	case 3:	res = open && !( self->shutflag & Shutdown_Receive ); break;
	}
	DaoProcess_PutBoolean( proc, res );
}

static void DaoSocket_Lib_For( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	DaoTuple *iter = &p[1]->xTuple;
	if ( self->state != Socket_Listening ){
		DaoProcess_RaiseError( proc, neterr, "The socket not in the listening state" );
		return;
	}
	DaoTuple_SetItem( iter, (DaoValue*)DaoInteger_New( 1 ), 0 );
	DaoTuple_SetItem( iter, (DaoValue*)DaoInteger_New( 0 ), 1 );
}

static void DaoSocket_Lib_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket_Lib_Accept( proc, p, 1 );
}

struct in_addr* GetAddr( DaoProcess *proc, DaoValue *param, struct hostent **he )
{
	if ( param->type == DAO_STRING ){
		NET_TRANS( *he = gethostbyname( param->xString.value->chars ) );
		if ( *he == NULL){
			char errbuf[MAX_ERRMSG];
			GetHostErrorMessage( errbuf, GetHostError() );
			DaoProcess_RaiseError( proc, hosterr, errbuf );
			return NULL;
		}
		return (struct in_addr *)(*he)->h_addr;
	}
	else {
		DaoIpv4Addr *ip = (DaoIpv4Addr*)DaoValue_TryGetCdata( param );
		return (struct in_addr*)ip->octets;
	}
}

static void DaoSocket_Lib_SendTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	ipv4_t ip;
	port_t port;
	if ( !ExtractAddr( proc, p[1], &ip, &port ) )
		return;
	if ( DaoNetwork_SendTo( self->id, ip, port, p[2]->xString.value ) == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_ReceiveFrom( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	DaoSocketAddr *saddr;
	DaoTuple *tup = DaoProcess_PutTuple( proc, 2 );
	struct sockaddr_in addr;
	if ( self->state != Socket_Bound ){
		DaoProcess_RaiseError( proc, neterr, "The socket is not bound" );
		return;
	}
	saddr = (DaoSocketAddr*)dao_malloc( sizeof(DaoSocketAddr) );
	DaoTuple_SetItem( tup, (DaoValue*)DaoProcess_NewCdata( proc, daox_type_sockaddr, saddr, 1 ), 0 );
	if ( DaoNetwork_ReceiveFrom( self->id, &addr, tup->values[1]->xString.value, p[1]->xInteger.value ) == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
	*(ipv4_t*)saddr->ip = addr.sin_addr.s_addr;
	saddr->port = ntohs( addr.sin_port );
}

int GetSockOpt( DaoProcess *proc, DaoSocket *sock, int opt, int *value )
{
	socklen_t size = sizeof(*value);
	if ( sock->state == Socket_Closed ){
		DaoProcess_RaiseError( proc, neterr, "The socket is closed" );
		return 0;
	}
	if ( getsockopt( sock->id, SOL_SOCKET, opt, (char*)value, &size ) == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return 0;
	}
	return 1;
}

int SetSockOpt( DaoProcess *proc, DaoSocket *sock, int opt, int value )
{
	if ( sock->state == Socket_Closed ){
		DaoProcess_RaiseError( proc, neterr, "The socket is closed" );
		return 0;
	}
	if ( setsockopt( sock->id, SOL_SOCKET, opt, (char*)&value, sizeof(value) ) == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return 0;
	}
	return 1;
}

static void DaoSocket_Lib_KeepAlive( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	int value;
	if ( GetSockOpt( proc, self, SO_KEEPALIVE, &value ) )
		DaoProcess_PutBoolean( proc, value );
}

static void DaoSocket_Lib_SetKeepAlive( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	int value = p[1]->xBoolean.value;
	SetSockOpt( proc, self, SO_KEEPALIVE, value );
}

static void DaoSocket_Lib_NoDelay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	int value;
	socklen_t size = sizeof(value);
	if ( self->state == Socket_Closed ){
		DaoProcess_RaiseError( proc, neterr, "The socket is closed" );
		return;
	}
	if ( getsockopt( self->id, IPPROTO_TCP, TCP_NODELAY, (char*)&value, &size ) == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return;
	}
	DaoProcess_PutBoolean( proc, value );
}

static void DaoSocket_Lib_SetNoDelay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	int value = p[1]->xBoolean.value;
	if ( self->state == Socket_Closed ){
		DaoProcess_RaiseError( proc, neterr, "The socket is closed" );
		return;
	}
	if ( setsockopt( self->id, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value) ) == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_Broadcast( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	int value;
	if ( GetSockOpt( proc, self, SO_BROADCAST, &value ) )
		DaoProcess_PutBoolean( proc, value );
}

static void DaoSocket_Lib_SetBroadcast( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	int value = p[1]->xBoolean.value;
	SetSockOpt( proc, self, SO_BROADCAST, value );
}

#ifdef WIN32
typedef DWORD numsockopt_t;
#else
typedef uint8_t numsockopt_t;
#endif

int GetIPSockOpt( DaoProcess *proc, DaoSocket *sock, int opt, numsockopt_t *value )
{
	socklen_t size = sizeof(*value);
	if ( sock->state == Socket_Closed ){
		DaoProcess_RaiseError( proc, neterr, "The socket is closed" );
		return 0;
	}
	if ( getsockopt( sock->id, IPPROTO_IP, opt, (char*)value, &size ) == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return 0;
	}
	return 1;
}

int SetIPSockOpt( DaoProcess *proc, DaoSocket *sock, int opt, numsockopt_t value )
{
	if ( sock->state == Socket_Closed ){
		DaoProcess_RaiseError( proc, neterr, "The socket is closed" );
		return 0;
	}
	if ( setsockopt( sock->id, IPPROTO_IP, opt, (char*)&value, sizeof(value) ) == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		return 0;
	}
	return 1;
}

static void DaoSocket_Lib_MultiLoop( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	numsockopt_t value;
	if ( GetIPSockOpt( proc, self, IP_MULTICAST_LOOP, &value ) )
		DaoProcess_PutBoolean( proc, value );
}

static void DaoSocket_Lib_SetMultiLoop( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	numsockopt_t value = p[1]->xBoolean.value;
	SetIPSockOpt( proc, self, IP_MULTICAST_LOOP, value );
}

void MultiGroupOperation( DaoProcess *proc, DaoValue *p[], int N, int add )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	struct ip_mreq value;
	struct in_addr *paddr;
	struct hostent *he;
	int res;
	paddr = GetAddr( proc, p[1], &he );
	if ( !paddr )
		return;
	value.imr_multiaddr = *paddr;
	value.imr_interface.s_addr = INADDR_ANY;
	res = setsockopt( self->id, IPPROTO_IP, add? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, (char*)&value, sizeof(value) );
	if ( res == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_JoinMultiGroup( DaoProcess *proc, DaoValue *p[], int N )
{
	MultiGroupOperation( proc, p, N, 1 );
}

static void DaoSocket_Lib_LeaveMultiGroup( DaoProcess *proc, DaoValue *p[], int N )
{
	MultiGroupOperation( proc, p, N, 0 );
}

static void DaoSocket_Lib_MultiTTL( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	numsockopt_t value;
	if ( GetIPSockOpt( proc, self, IP_MULTICAST_TTL, &value ) )
		DaoProcess_PutBoolean( proc, value );
}

static void DaoSocket_Lib_SetMultiTTL( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
	numsockopt_t value;
	value = p[1]->xInteger.value;
	SetIPSockOpt( proc, self, IP_MULTICAST_TTL, value );
}

static void TTLOperation( DaoProcess *proc, DaoValue *p[], int N, int get )
{
	DaoSocket *self = (DaoSocket*)DaoValue_TryGetCdata( p[0] );
#ifdef WIN32
	DWORD value;
#else
	int value;
#endif
	int res;
	socklen_t size = sizeof(value);
	value = p[1]->xInteger.value;
	if ( self->state == Socket_Closed ){
		DaoProcess_RaiseError( proc, neterr, "The socket is closed" );
		return;
	}
	res = get? getsockopt( self->id, IPPROTO_IP, IP_TTL, (char*)&value, &size ) :
			   setsockopt( self->id, IPPROTO_IP, IP_TTL, (char*)&value, size );
	if ( res == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
	}
}

static void DaoSocket_Lib_TTL( DaoProcess *proc, DaoValue *p[], int N )
{
	TTLOperation( proc, p, N, 1 );
}

static void DaoSocket_Lib_SetTTL( DaoProcess *proc, DaoValue *p[], int N )
{
	TTLOperation( proc, p, N, 0 );
}

static DaoFuncItem socketMeths[] =
{
	/*! Address to which the socket is bound */
	{  DaoSocket_Lib_GetSockName,   ".localAddr( invar self: Socket ) => SocketAddr" },

	/*! Socket file descriptor */
	{  DaoSocket_Lib_Id,            ".id( invar self: Socket ) => int" },

	/*! Current socket state */
	{  DaoSocket_Lib_State,         ".state( invar self: Socket ) => enum<closed,bound,listening,connected>" },

	/*! Closes the socket */
	{  DaoSocket_Lib_Close,         "close( self: Socket )" },
	{ NULL, NULL }
};

//! Abstract socket type
DaoTypeBase socketTyper = {
	"Socket", NULL, NULL, socketMeths, {0}, {0}, (FuncPtrDel)DaoSocket_Delete, NULL
};

static DaoFuncItem TcpStreamMeths[] =
{
	/*! Connects to address \a addr which may be either a 'host:port' string or \c SocketAddr */
	{ DaoSocket_Lib_Connect,		"connect( self: TcpStream, addr: string|SocketAddr )" },

	/*! Sends \a data */
	{ DaoSocket_Lib_Send,			"write( self: TcpStream, data: string )" },

	/*! Sends \a file */
	{ DaoSocket_Lib_SendFile,		"writeFile( self: TcpStream, file: string )" },

	/*! Receives at most \a count bytes (64Kb max, 4Kb if \a count <= 0) and returnes the received data */
	{ DaoSocket_Lib_Receive,		"read( self: TcpStream, count = -1 ) => string" },

	/* TODO: move to different wrapper: */
	/*! Sends data via the internal serialization protocol */
	{ DaoSocket_Lib_Serialize,		"serialize( self: TcpStream, invar object: Object )" },

	/*! Receives data via the internal serialization protocol */
	{ DaoSocket_Lib_Deserialize,	"deserialize( self: TcpStream ) => Object" },

	/*! Checks the property specified by \a what; required to satisfy `io::Device` interface  */

	/*! Attempts to receive exactly \a count bytes of data and return it. Will return less data only if
	 * the connection was closed.
	 *
	 * If \a prepend is given, it will be prepended to the resulting string. Use case: receiving a message
	 * with a header and a large body (e.g., HTTP response). First, the header is read, by which the body
	 * size is determined. While doing this, part of the message body might be read along with the header.
	 * Concatenating it with the rest of the body after receiving the latter would nullify the performance
	 * advantage of using \c readAll(), so that chunk should be passed as \a prepend parameter */
	{ DaoSocket_Lib_ReceiveAll,		"readAll( self: TcpStream, count: int, prepend = '' ) => string" },

	/*! Peer address of the connected socket */
	{ DaoSocket_Lib_GetPeerName,	".peerAddr( invar self: TcpStream ) => SocketAddr" },

	/*! Checks the property specified by \a what; required to satisfy `io::Device` interface  */
	{ DaoSocket_Lib_Check,			"check(self: TcpStream, what: enum<readable,writable,open,eof>) => bool" },

	/*! Shuts down the connection, stopping further operations specified by \a what */
	{ DaoSocket_Lib_Shutdown,		"shutdown( self: TcpStream, what: enum<send,receive,all> )" },

	/*! Returns \c io::Stream opened with the given \a mode bound to the socket
	 *
	 * \warning Not supported on Windows */
	//{ DaoSocket_Lib_GetStream,		"open( invar self: TcpStream, mode: string ) => io::Stream" },
	// Removed to avoid dependency on module/stream;

	{ DaoSocket_Lib_GetDescriptor,	".descriptor( invar self: TcpStream ) => int" },

	/*! TCP keep-alive option (SO_KEEPALIVE) */
	{ DaoSocket_Lib_KeepAlive,		".keepAlive( invar self: TcpStream ) => bool" },
	{ DaoSocket_Lib_SetKeepAlive,	".keepAlive=( self: TcpStream, value: bool )" },

	/*! TCP no-delay option (SO_NODELAY) */
	{ DaoSocket_Lib_NoDelay,		".noDelay( invar self: TcpStream ) => bool" },
	{ DaoSocket_Lib_SetNoDelay,		".noDelay=( self: TcpStream, value: bool )" },
	{ NULL, NULL }
};

//! TCP stream socket (a connection endpoint)
DaoTypeBase TcpStreamTyper = {
	"TcpStream", NULL, NULL, TcpStreamMeths, {&socketTyper, NULL}, {0}, (FuncPtrDel)DaoSocket_Delete, NULL
};

static DaoFuncItem TcpListenerMeths[] =
{
	/*! Binds the socket to address \a addr (either a 'host:port' string or \c SocketAddr) using \a addrOpts as the address binding options (for the description of
	 * \a addrOpts, see \c net.listen()). Sets the socket into the listening state using \a backLog as the maximum size of the queue of pending connections
	 * (use \c MAX_BACKLOG constant to assign the maximum queue size) */
	{ DaoSocket_Lib_Listen,			"listen( self: TcpListener, addr: string|SocketAddr, backLog = 10, addrOpts: enum<shared;exclusive;reused;default> = $exclusive )" },

	/*! Accepts new connection, returning the corresponding \c TcpStream and peer address */
	{ DaoSocket_Lib_Accept,			"accept( self: TcpListener ) => tuple<stream: TcpStream, addr: SocketAddr>" },

	//! Equivalient to an infinite loop calling \c accept() on each iteration
	{ DaoSocket_Lib_For,			"for( self: TcpListener, iterator: ForIterator )" },
	{ DaoSocket_Lib_Get,			"[]( self: TcpListener, index: ForIterator ) => tuple<stream: TcpStream, addr: SocketAddr>" },
	{ NULL, NULL }
};

//! TCP server-side socket
DaoTypeBase TcpListenerTyper = {
	"TcpListener", NULL, NULL, TcpListenerMeths, {&socketTyper, NULL}, {0}, (FuncPtrDel)DaoSocket_Delete, NULL
};

static DaoFuncItem UdpSocketMeths[] =
{
	/*! Binds the socket to address \a addr (either a 'host:port' string or \c SocketAddr) using \a addrOpts as the address binding options. For the description
	 * of \a addrOpts, see \c net.listen() */
	{ DaoSocket_Lib_Bind,			"bind( self: UdpSocket, addr: string|SocketAddr, addrOpts: enum<shared;exclusive;reused;default> = $exclusive )" },

	/*! Sends \a data to the receiver specified by address \a addr which is either a 'host:port' string or \c SocketAddr */
	{ DaoSocket_Lib_SendTo,			"write( self: UdpSocket, addr: string|SocketAddr, data: string )" },

	/*! Receives at most \a limit bytes and returnes the received data and the address of its sender */
	{ DaoSocket_Lib_ReceiveFrom,	"read( self: UdpSocket, limit = 4096 ) => tuple<addr: SocketAddr, data: string>" },

	/*! UDP broadcast option (SO_BROADCAST) */
	{ DaoSocket_Lib_Broadcast,		".broadcast( invar self: UdpSocket ) => bool" },
	{ DaoSocket_Lib_SetBroadcast,	".broadcast=( self: UdpSocket, value: bool )" },

	/*! Multicast loop socket option (IP_MULTICAST_LOOP) */
	{ DaoSocket_Lib_MultiLoop,		".multicastLoop( invar self: UdpSocket ) => bool" },
	{ DaoSocket_Lib_SetMultiLoop,	".multicastLoop=( self: UdpSocket, value: bool )" },

	/*! Joins multicast \a group specified by its host name or ip address */
	{ DaoSocket_Lib_JoinMultiGroup,	"joinGroup( self: UdpSocket, group: string|Ipv4Addr )" },

	/*! Leaves multicast \a group specified by its host name or ip address */
	{ DaoSocket_Lib_LeaveMultiGroup,"leaveGroup( self: UdpSocket, group: string|Ipv4Addr )" },

	/*! Multicast TTL value (IP_MULTICAST_TTL) */
	{ DaoSocket_Lib_MultiTTL,		".multicastTtl( invar self: UdpSocket ) => int" },
	{ DaoSocket_Lib_SetMultiTTL,	".multicastTtl=( self: UdpSocket, value: int )" },

	/*! TTL value (IP_TTL) */
	{ DaoSocket_Lib_TTL,			".ttl( invar self: UdpSocket ) => int" },
	{ DaoSocket_Lib_SetTTL,			".ttl=( self: UdpSocket, value: int )" },
	{ NULL, NULL }
};

//! UDP socket
DaoTypeBase UdpSocketTyper = {
	"UdpSocket", NULL, NULL, UdpSocketMeths, {&socketTyper, NULL}, {0}, (FuncPtrDel)DaoSocket_Delete, NULL
};

void DaoIpv4Addr_Delete( DaoIpv4Addr *self )
{
	dao_free( self );
}

static void DaoIpv4Addr_Create( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *res = (DaoIpv4Addr*)dao_malloc( sizeof(DaoIpv4Addr) );
	if ( p[0]->type == DAO_STRING ){
		DString *str = p[0]->xString.value;
		if ( !str->size ){
			DaoProcess_RaiseError( proc, "Value", "Empty IPv4 address string" );
			dao_free( res );
			return;
		}
		if ( !GetIpAddr( str->chars, (ipv4_t*)res->octets ) ){
			DaoProcess_RaiseError( proc, "Value", "Invalid IPv4 address string" );
			dao_free( res );
			return;
		}
	}
	else {
		daoint i;
		for ( i = 0; i < 4; i++ ){
			dao_integer octet = p[i]->xInteger.value;
			if ( octet < 0 || octet > 255 ){
				DaoProcess_RaiseError( proc, "Value", "Invalid IPv4 address octet" );
				dao_free( res );
				return;
			}
			res->octets[i] = octet;
		}
	}
	DaoProcess_PutCdata( proc, res, daox_type_ipv4addr );
}

static void DaoIpv4Addr_ToString( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *self = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
	char buf[20];
	snprintf( buf, sizeof(buf), "%i.%i.%i.%i", (int)self->octets[0], (int)self->octets[1], (int)self->octets[2], (int)self->octets[3] );
	DaoProcess_PutChars( proc, buf );
}

enum {
	IPv4Unspecified = 0,
	IPv4Multicast,
	IPv4Private,
	IPv4Global,
	IPv4Loopback,
	IPv4LinkLocal,
	IPv4Broadcast
};

static void DaoIpv4Addr_Check( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *self = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
	int res;
	uint8_t *octets = self->octets;
	switch ( p[1]->xEnum.value ){
	case IPv4Unspecified: // 0.0.0.0
		res = *(ipv4_t*)self->octets == 0;
		break;
	case IPv4Multicast: // RFC 5771
		res = ( octets[0] == 224 && ( octets[1] <= 5 || octets[1] >= 252 ) ) ||
				( octets[0] >= 225 && octets[0] <= 239 );
		break;
	case IPv4Private: // RFC 1918
	case IPv4Global:
		res = octets[0] == 10 || ( octets[0] == 172 && octets[1] >= 16 && octets[1] <= 31 ) ||
				( octets[0] == 192 && octets[1] == 168 );
		if ( p[1]->xEnum.value == IPv4Global )
			res = !res;
		break;
	case IPv4Loopback:
		res = octets[0] == 127 && octets[1] == 0 && octets[2] == 0;
		break;
	case IPv4LinkLocal: // RFC 3927
		res = octets[0] == 169 && octets[1] == 254;
		break;
	case IPv4Broadcast: // RFC 919
		res = *(ipv4_t*)self->octets == 0xFFFFFFFF;
		break;
	}
	DaoProcess_PutBoolean( proc, res );
}

static void DaoIpv4Addr_Lt( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *a = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
	DaoIpv4Addr *b = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, memcmp( a->octets, b->octets, 4 ) < 0 );
}

static void DaoIpv4Addr_Le( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *a = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
	DaoIpv4Addr *b = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, memcmp( a->octets, b->octets, 4 ) <= 0 );
}

static void DaoIpv4Addr_Eq( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *a = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
	DaoIpv4Addr *b = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, memcmp( a->octets, b->octets, 4 ) == 0 );
}

static void DaoIpv4Addr_Neq( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *a = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
	DaoIpv4Addr *b = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, memcmp( a->octets, b->octets, 4 ) != 0 );
}

void IPv4_Add( DaoIpv4Addr *self, int32_t value, DaoIpv4Addr *res )
{
	int i;
	ipv4_t bin = 0;
	for ( i = 0; i < 4; i++ )
		bin |= (ipv4_t)self->octets[i] << ( 3 - i )*CHAR_BIT;
	bin += value;
	for ( i = 0; i < 4; i++ )
		res->octets[i] = ( bin >> ( 3 - i )*CHAR_BIT ) & 0xFF;
}

static void DaoIpv4Addr_Lib_Add( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *self = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
	DaoIpv4Addr *res = (DaoIpv4Addr*)dao_malloc( sizeof(DaoIpv4Addr) );
	IPv4_Add( self, p[1]->xInteger.value, res );
	DaoProcess_PutCdata( proc, res, daox_type_ipv4addr );
}

static void DaoIpv4Addr_Lib_Sub( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *self = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
	DaoIpv4Addr *res = (DaoIpv4Addr*)dao_malloc( sizeof(DaoIpv4Addr) );
	IPv4_Add( self, -p[1]->xInteger.value, res );
	DaoProcess_PutCdata( proc, res, daox_type_ipv4addr );
}

static void DaoIpv4Addr_Octets( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoIpv4Addr *self = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
	DaoTuple *tup = DaoProcess_PutTuple( proc, 4 );
	daoint i;
	for ( i = 0; i < 4; i++ )
		tup->values[i]->xInteger.value = self->octets[i];
}

static DaoFuncItem Ipv4AddrMeths[] =
{
	//! Creates IPv4 address from dotted string \a value or given the individual octets \a a, \a b, \a c and \a d
	{ DaoIpv4Addr_Create,	"Ipv4Addr(value: string)" },
	{ DaoIpv4Addr_Create,	"Ipv4Addr(a: int, b: int, c: int, d: int)" },

	//! Dotted string value
	{ DaoIpv4Addr_ToString,	".value(self: Ipv4Addr) => string" },

	//! Individual address octets
	{ DaoIpv4Addr_Octets,	".octets(self: Ipv4Addr) => tuple<int,int,int,int>" },

	//! Same as calling value()
	{ DaoIpv4Addr_ToString,	"(string)(self: Ipv4Addr)" },

	//! Returns true if the address belongs to the kind specified by \a what
	{ DaoIpv4Addr_Check,	"is(self: Ipv4Addr, "
								"what: enum<unspecified,multicast,private,global,loopback,linkLocal,broadcast>) => bool" },

	//! Address comparison
	{ DaoIpv4Addr_Lt,		"<(a: Ipv4Addr, b: Ipv4Addr) => bool" },
	{ DaoIpv4Addr_Le,		"<=(a: Ipv4Addr, b: Ipv4Addr) => bool" },
	{ DaoIpv4Addr_Eq,		"==(a: Ipv4Addr, b: Ipv4Addr) => bool" },
	{ DaoIpv4Addr_Neq,		"!=(a: Ipv4Addr, b: Ipv4Addr) => bool" },

	//! Address arithmetic
	{ DaoIpv4Addr_Lib_Add,	"+(a: Ipv4Addr, b: int) => Ipv4Addr" },
	{ DaoIpv4Addr_Lib_Sub,	"-(a: Ipv4Addr, b: int) => Ipv4Addr" },
	{ NULL, NULL }
};

DaoTypeBase Ipv4AddrTyper = {
	"Ipv4Addr", NULL, NULL, Ipv4AddrMeths, {0}, {0}, (FuncPtrDel)DaoIpv4Addr_Delete, NULL
};

void DaoSocketAddr_Delete( DaoSocketAddr *self )
{
	dao_free( self );
}

static void DaoSocketAddr_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSocketAddr *addr = (DaoSocketAddr*)dao_malloc( sizeof(DaoSocketAddr) );
	if ( p[0]->type == DAO_STRING ){
		if ( !ParseAddr( proc, p[0]->xString.value, (ipv4_t*)addr->ip, &addr->port, 1 ) ){
			dao_free( addr );
			return;
		}
	}
	else {
		DaoIpv4Addr *ip = (DaoIpv4Addr*)DaoValue_TryGetCdata( p[0] );
		dao_integer port = p[1]->xInteger.value;
		if ( !CheckPort( proc, port ) ){
			dao_free( addr );
			return;
		}
		*(ipv4_t*)addr->ip = *(ipv4_t*)ip->octets;
		addr->port = port;
	}
	DaoProcess_PutCdata( proc, addr, daox_type_sockaddr );
}

static void DaoSocketAddr_ToString( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoSocketAddr *self = (DaoSocketAddr*)DaoValue_TryGetCdata( p[0] );
	char buf[25];
	snprintf( buf, sizeof(buf), "%i.%i.%i.%i:%i", (int)self->ip[0], (int)self->ip[1], (int)self->ip[2], (int)self->ip[3], (int)self->port );
	DaoProcess_PutChars( proc, buf );
}

static void DaoSocketAddr_Eq( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoSocketAddr *a = (DaoSocketAddr*)DaoValue_TryGetCdata( p[0] );
	DaoSocketAddr *b = (DaoSocketAddr*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, memcmp( a->ip, b->ip, 4 ) == 0 && a->port == b->port );
}

static void DaoSocketAddr_Neq( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoSocketAddr *a = (DaoSocketAddr*)DaoValue_TryGetCdata( p[0] );
	DaoSocketAddr *b = (DaoSocketAddr*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, memcmp( a->ip, b->ip, 4 ) != 0 || a->port != b->port );
}

static void DaoSocketAddr_IP( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoSocketAddr *self = (DaoSocketAddr*)DaoValue_TryGetCdata( p[0] );
	DaoIpv4Addr *ip = (DaoIpv4Addr*)dao_malloc( sizeof(DaoIpv4Addr) );
	*(ipv4_t*)ip->octets = *(ipv4_t*)self->ip;
	DaoProcess_PutCdata( proc, ip, daox_type_ipv4addr );
}

static void DaoSocketAddr_Port( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoSocketAddr *self = (DaoSocketAddr*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->port );
}

static DaoFuncItem sockaddrMeths[] =
{
	//! Creates socket address given 'host:port' \a value (if the host part is empty, '0.0.0.0' is assumed)
	{ DaoSocketAddr_Create,		"SocketAddr(value: string)" },

	//! Creates socket address given or \a ip and \a port
	{ DaoSocketAddr_Create,		"SocketAddr(ip: Ipv4Addr, port: int)" },

	{ DaoSocketAddr_IP,			".ip(self: SocketAddr) => Ipv4Addr" },
	{ DaoSocketAddr_Port,		".port(self: SocketAddr) => int" },

	//! 'ip:port' string
	{ DaoSocketAddr_ToString,	".value(self: SocketAddr) => string" },

	//! String conversion
	{ DaoSocketAddr_ToString,	"(string)(self: SocketAddr)" },

	//! Comparison
	{ DaoSocketAddr_Eq,			"==(a: SocketAddr, b: SocketAddr) => bool" },
	{ DaoSocketAddr_Neq,		"!=(a: SocketAddr, b: SocketAddr) => bool" },
	{ NULL, NULL }
};

DaoTypeBase sockaddrTyper = {
	"SocketAddr", NULL, NULL, sockaddrMeths, {0}, {0}, (FuncPtrDel)DaoSocketAddr_Delete, NULL
};

static void DaoNetLib_Listen( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *sock;
	ipv4_t ip;
	port_t port;
	dao_integer backlog = par[1]->xInteger.value;
	if ( !ExtractAddr( proc, par[0], &ip, &port ) )
		return;
	if ( backlog <= 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid backlog value" );
		return;
	}
	sock = DaoSocket_New();
	if( DaoSocket_Bind( sock, Proto_TCP, ip, port, par[2]->xEnum.value ) == -1 || DaoNetwork_Listen( sock->id, backlog ) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		DaoSocket_Delete( sock );
		return;
	}
	sock->state = Socket_Listening;
	DaoProcess_PutCdata( proc, (void*)sock, daox_type_tcplistener );
}
static void DaoNetLib_Bind( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	DaoSocket *sock;
	ipv4_t ip;
	port_t port;
	if ( !ExtractAddr( proc, par[0], &ip, &port ) )
		return;
	sock = DaoSocket_New();
	if( DaoSocket_Bind( sock, Proto_UDP, ip, port, par[1]->xEnum.value) == -1 ){
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		DaoSocket_Delete( sock );
		return;
	}
	DaoProcess_PutCdata( proc, (void*)sock, daox_type_udpsocket );
}
static void DaoNetLib_Connect( DaoProcess *proc, DaoValue *p[], int N  )
{
	DaoSocket *sock = DaoSocket_New(  );
	ipv4_t ip;
	port_t port;
	if ( !ExtractAddr( proc, p[0], &ip, &port ) )
		return;
	if ( DaoSocket_Connect( sock, ip, port ) == -1 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, GetError() );
		DaoProcess_RaiseError( proc, neterr, errbuf );
		DaoSocket_Delete( sock );
	}
	else
		DaoProcess_PutCdata( proc, sock, daox_type_tcpstream );
}
static void DaoNetLib_GetHost( DaoProcess *proc, DaoValue *par[], int N  )
{
	char errbuf[MAX_ERRMSG];
	struct hostent *hent;
	struct sockaddr_in addr;
	struct in_addr id;
	size_t size = sizeof( struct sockaddr_in );
	const char *host = DaoString_GetChars( DaoValue_CastString( par[0] ) );
	DaoTuple *tup;
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
		int error = GetHostError();
#ifdef WIN32
		if ( error == WSAHOST_NOT_FOUND || error == WSANO_DATA )
#else
		if ( error == HOST_NOT_FOUND || error == NO_ADDRESS )
#endif
			DaoProcess_PutNone( proc );
		else {
			GetHostErrorMessage( errbuf, error );
			DaoProcess_RaiseError( proc, hosterr, errbuf );
		}
		return;
	}
	tup = DaoProcess_PutTuple( proc, 3 );
	DaoTuple_SetItem( tup, (DaoValue*)DaoProcess_NewList( proc ), 1 );
	DaoTuple_SetItem( tup, (DaoValue*)DaoProcess_NewList( proc ), 2 );
	DString_SetChars( tup->values[0]->xString.value, hent->h_name );
	if( hent->h_addrtype == AF_INET ){
		char **p = hent->h_aliases;
		char **q = hent->h_addr_list;
		while( *p ){
			DaoList_Append( &tup->values[1]->xList, (DaoValue*)DaoString_NewChars( *p ) );
			p ++;
		}
		while ( *q ){
			DaoIpv4Addr *ip = (DaoIpv4Addr*)dao_malloc( sizeof(DaoIpv4Addr) );
			*(ipv4_t*)ip->octets = ( (struct in_addr*)(*q) )->s_addr;
			DaoValue *value = (DaoValue*)DaoProcess_NewCdata( proc, daox_type_ipv4addr, ip, 1 );
			DaoList_Append( &tup->values[2]->xList, value );
			q ++;
		}
	}else{ /* AF_INET6 */
	}
}

static void DaoNetLib_GetService( DaoProcess *proc, DaoValue *par[], int N  )
{
	struct servent *srvent;
	DaoTuple *tup;
	const char *proto = par[1]->xEnum.value == 0? "tcp" : "udp";
	char **p;
	if ( par[0]->type == DAO_STRING ){
		if ( !par[0]->xString.value->size )
			return;
		srvent = getservbyname( par[0]->xString.value->chars, proto );
	}
	else
		srvent = getservbyport( par[0]->xInteger.value, proto );
	if( srvent == NULL ){
		DaoProcess_PutNone( proc );
		return;
	}
	tup = DaoProcess_PutTuple( proc, 3 );
	DaoTuple_SetItem( tup, (DaoValue*)DaoProcess_NewList( proc ), 2 );
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
	int i, fd;
	fd_set set1, set2;
	DaoTuple *tuple = DaoProcess_PutTuple( proc, 2 );
	DaoList *list1 = DaoValue_CastList( par[0] );
	DaoList *list2 = DaoValue_CastList( par[1] );
	DaoList *reslist;
	DaoValue *value;
	if( DaoList_Size( list1 ) == 0 && DaoList_Size( list2 ) == 0 ){
		DaoProcess_RaiseError( proc, "Param", "Both read and write parameters are empty lists" );
		return;
	}
	FD_ZERO( &set1 );
	FD_ZERO( &set2 );
	for( i = 0; i < DaoList_Size( list1 ); i++ ){
		value = DaoList_GetItem( list1, i );
		fd = value->type == DAO_INTEGER? value->xInteger.value : ( (DaoSocket*)DaoValue_TryGetCdata( value ) )->id;
		if( fd < 0 ){
			DaoProcess_RaiseError( proc, "Value", "The read list contains a closed socket (or an invalid fd)" );
			return;
		}
		FD_SET( fd, &set1 );
	}
	for( i = 0; i < DaoList_Size( list2 ); i++ ){
		value = DaoList_GetItem( list2, i );
		fd = value->type == DAO_INTEGER? value->xInteger.value : ( (DaoSocket*)DaoValue_TryGetCdata( value ) )->id;
		if( fd < 0 ){
			DaoProcess_RaiseError( proc, "Value", "The write list contains a closed socket (or an invalid fd)" );
			return;
		}
		FD_SET( fd, &set2 );
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
		fd = value->type == DAO_INTEGER? value->xInteger.value : ( (DaoSocket*)DaoValue_TryGetCdata( value ) )->id;
		if( FD_ISSET( fd, &set1 ) )
			DaoList_PushBack( reslist, value );
	}
	value = (DaoValue*) DaoProcess_NewList( proc );
	DaoTuple_SetItem( tuple, value, 1 );
	reslist = DaoValue_CastList( DaoTuple_GetItem( tuple, 1 ) );
	for( i = 0; i < DaoList_Size( list2 ); i++ ){

		value = DaoList_GetItem( list2, i );
		fd = value->type == DAO_INTEGER? value->xInteger.value : ( (DaoSocket*)DaoValue_TryGetCdata( value ) )->id;
		if( FD_ISSET( fd, &set2 ) )
			DaoList_PushBack( reslist, value );
	}
}

static DaoFuncItem netMeths[] =
{
	/*! Returns new TCP listener bound to address \a addr (either a 'host:port' string or \c SocketAddr) with \a addrOpts options used to regulate address binding.
	 * The socket is put in the listening state using \a backLog as the maximum size of the queue of pending connections (use \c MAX_BACKLOG constant to assign
	 * the maximum queue size).
	 *
	 * Meaning of \a addrOpts values:
	 * -\c shared -- non-exclusive binding of the address and port, other sockets will be able to bind to the same address and port
	 * (SO_REUSEADDR on Unix, ignored on Windows)
	 * -\c exclusive -- exclusive binding of the address and port, no other sockets are allowed to rebind
	 * (SO_EXCLUSIVEADDRUSE on Windows, ignored on Unix)
	 * -\c reused -- rebinds the socket even if the address and port are already bound by another socket (non-exclusively)
	 * (SO_REUSEADDR on Windows, ignored on Unix)
	 * -\c default -- default mode for the current platform (\c exclusive + \c reused on Unix, \c shared on Windows)
	 *
	 * If \a addrOpts is not specified, \c exclusive address mode is used */
	{  DaoNetLib_Listen,		"listen( addr: string|SocketAddr, backLog = 10, addrOpts: enum<shared;exclusive;reused;default> = $exclusive )"
								" => TcpListener" },

	/*! Returns new UDP socket bound to address \a addr (either a 'host:port' string or \c SocketAddr) with \a addrOpts options used to regulate address binding
	 * (see \c net.listen() for the description of \a addrOpts) */
	{  DaoNetLib_Bind,			"bind( addr: string|SocketAddr, addrOpts: enum<shared;exclusive;reused;default> = $exclusive )"
								" => UdpSocket" },

	/*! Returns client-side TCP connection endpoint connected to address \a addr which may be either a 'host:port' string or \c SockAddr */
	{  DaoNetLib_Connect,		"connect( addr: string|SocketAddr ) => TcpStream" },

	/*! Returns information for host with the given \a id (which may be either a name or an IPv4 address in dotted form). Returns \c none if the host is not found
	 * or does not have an IP address */
	{  DaoNetLib_GetHost,		"host( id: string ) => tuple<name: string, aliases: list<string>, addrs: list<Ipv4Addr>>|none" },

	/*! Returns information for service with the given \a id (which may be either a name or a port number) to be used with
	 * the protocol \a proto. Returns \c none if the corresponding service was not found */
	{  DaoNetLib_GetService,	"service( id: string|int, proto: enum<tcp,udp> )"
								" => tuple<name: string, port: int, aliases: list<string>>|none" },

	/*! Waits \a timeout seconds for any \c Socket or file descriptor in \a read or \a write list to become available for
	 * reading or writing accordingly. Returns sub-lists of \a read and \a write containing available sockets/descriptors.
	 *
	 * \note On Windows, only socket descriptors can be selected */
	{  DaoNetLib_Select,		"select( invar read: list<@R<Socket|int>>, invar write: list<@W<Socket|int>>, timeout: float )"
								" => tuple<read: list<@R>, write: list<@W>>" },
	{ NULL, NULL }
};

DaoNumItem netConsts[] =
{
	/*! Largest possible backLog value for TcpListener::listen() */
	{ "MAX_BACKLOG", DAO_INTEGER, SOMAXCONN },

	/*! Maximum number of file descriptors net.select() can handle for each input list */
	{ "SELECT_CAP", DAO_INTEGER, FD_SETSIZE },
	{ NULL, 0.0, 0.0 }
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

#define SimpleTypes     "none|bool|int|float|complex|string"
#define ArrayTypes      "array<bool>|array<int>|array<float>|array<complex>"
#define ContainerTypes  "list<Object>|map<Object,Object>|tuple<...:Object>|invar<Object>"
/*
// It is necessary to include invar<Object>, because the input parameter
// for serialize() is an invariable type, which allows covariant subtyping,
// so values that can match to invar<Object> but not to Object can get
// serialized and deserialized. If invar<Object> is not present in the
// return type of deserialize(), the deserialized values cannot be returned
// successfully!
*/

DAO_DLL int DaoNet_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *netns = DaoNamespace_GetNamespace( ns, "net" );
	DaoNamespace_DefineType( netns, SimpleTypes"|"ArrayTypes"|"ContainerTypes, "Object" );
	DaoNamespace_AddConstNumbers( netns, netConsts );
	daox_type_ipv4addr = DaoNamespace_WrapType( netns, & Ipv4AddrTyper, DAO_CTYPE_INVAR | DAO_CTYPE_OPAQUE );
	daox_type_sockaddr = DaoNamespace_WrapType( netns, & sockaddrTyper, DAO_CTYPE_INVAR | DAO_CTYPE_OPAQUE );
	daox_type_socket = DaoNamespace_WrapType( netns, & socketTyper, DAO_CTYPE_OPAQUE );
	daox_type_tcpstream = DaoNamespace_WrapType( netns, & TcpStreamTyper, DAO_CTYPE_OPAQUE );
	daox_type_tcplistener = DaoNamespace_WrapType( netns, & TcpListenerTyper, DAO_CTYPE_OPAQUE );
	daox_type_udpsocket = DaoNamespace_WrapType( netns, & UdpSocketTyper, DAO_CTYPE_OPAQUE );
	DaoNamespace_WrapFunctions( netns, netMeths );
	DaoNetwork_Init( vmSpace, ns );
	return 0;
}
