/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2015-2016, Limin Fu
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

// 2015-07: Danilov Aleksey, initial implementation.

#ifndef __DAO_HTTP__
#define __DAO_HTTP__

#include "dao.h"
#include "daoValue.h"

typedef struct DaoHttpHeader    DaoHttpHeader;
typedef struct DaoHttpRequest   DaoHttpRequest;
typedef struct DaoHttpResponse  DaoHttpResponse;
typedef struct DaoChunkDecoder  DaoChunkDecoder;

#define DAO_HTTP_HEADER_COMMON \
	DAO_CSTRUCT_COMMON; \
	DString *version; \
	DMap *headers; \
	DList *cookies; \
	daoint size

typedef enum {
	Http_Get = 0,
	Http_Post,
	Http_Head,
	Http_Connect,
	Http_Put,
	Http_Delete,
	Http_Trace,
	Http_Options,
	Http_Patch
} http_meth_t;

struct DaoHttpHeader
{
	DAO_HTTP_HEADER_COMMON;
};

struct DaoHttpRequest
{
	DAO_HTTP_HEADER_COMMON;
	DString *method, *uri;
};

struct DaoHttpResponse 
{
	DAO_HTTP_HEADER_COMMON;
	DString *reason;
	daoint code;
};

typedef enum {
	Http_IncompleteMessage = 1,
	Http_InvalidStartLine,
	Http_InvalidMethod,
	Http_InvalidHttpVersion,
	Http_InvalidUrl,
	Http_InvalidStatusCode,
	Http_InvalidHeader,
	Http_InvalidFieldName,
	Http_InvalidFieldValue,
	Http_MissingFormItemName,
	Http_MissingFormEnd
} http_err_t;

typedef enum {
	Status_Idle = 0,
	Status_IncompleteBody,
	Status_TrailExpected,
	Status_IncompleteHeader,
	Status_Finished
} chunk_status_t;

struct DaoChunkDecoder
{
	DAO_CSTRUCT_COMMON;

	chunk_status_t status;
	daoint pending;
	DString *part;
	uchar_t last;
};

#endif
