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

#include"dao.h"

#ifndef DAO_SERIAL_DLL
#ifdef DAO_SERIAL
#  define DAO_SERIAL_DLL DAO_DLL_EXPORT
#else
#  define DAO_SERIAL_DLL DAO_DLL_IMPORT
#endif
#endif

#ifndef __DAO_SERIALIZER_H__
#define __DAO_SERIALIZER_H__

#define DAO_HAS_STREAM
#include"dao_api.h"

typedef struct DaoSerializer    DaoSerializer;
typedef struct DaoDeserializer  DaoDeserializer;

struct DaoSerializer
{
	DAO_CSTRUCT_COMMON;

	DaoStream  *stream;
};

struct DaoDeserializer
{
	DAO_CSTRUCT_COMMON;

	DaoStream  *stream;
};


#endif

DAO_API( DAO_SERIAL_DLL, DaoSerializer*, DaoSerializer_New, () );
DAO_API( DAO_SERIAL_DLL, DaoDeserializer*, DaoDeserializer_New, () );

DAO_API( DAO_SERIAL_DLL, void, DaoSerializer_Delete, (DaoSerializer *self) );
DAO_API( DAO_SERIAL_DLL, void, DaoDeserializer_Delete, (DaoSerializer *self) );

DAO_API( DAO_SERIAL_DLL, void, DaoSerializer_SetStream, (DaoSerializer *self, DaoStream *stream) );
DAO_API( DAO_SERIAL_DLL, void, DaoDeserializer_SetStream, (DaoSerializer *self, DaoStream *stream) );

DAO_API( DAO_SERIAL_DLL, void, DaoSerializer_Write, (DaoSerializer *self, DaoValue *value) );
DAO_API( DAO_SERIAL_DLL, DaoValue*, DaoDeserializer_Read, (DaoSerializer *self) );

DAO_API( DAO_SERIAL_DLL, void, DaoSerializer_Write2,
		(DaoSerializer *self, DaoValue *value, DaoNamespace *ns, DaoProcess *proc) );

DAO_API( DAO_SERIAL_DLL, DaoValue*, DaoDeserializer_Read2,
		(DaoSerializer *self, DaoNamespace *ns, DaoProcess *proc) );


DAO_SERIAL_DLL int DaoValue_Serialize( DaoValue *self, DString *serial, DaoNamespace *ns, DaoProcess *proc );
DAO_SERIAL_DLL int DaoValue_Deserialize( DaoValue **self, DString *serial, DaoNamespace *ns, DaoProcess *proc );

DAO_SERIAL_DLL void DaoNamespace_Backup( DaoNamespace *self, DaoProcess *proc, FILE *fout, int limit );
DAO_SERIAL_DLL void DaoNamespace_Restore( DaoNamespace *self, DaoProcess *proc, FILE *fin );

