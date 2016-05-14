/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2016, Limin Fu
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
#include"daoStdtype.h"

#ifndef DAO_SERIAL_DLL
#ifdef DAO_SERIAL
#  define DAO_SERIAL_DLL DAO_DLL_EXPORT
#else
#  define DAO_SERIAL_DLL DAO_DLL_IMPORT
#endif
#endif

#ifndef __DAO_SERIALIZER_H__
#define __DAO_SERIALIZER_H__


typedef struct DaoSerializer  DaoSerializer;

struct DaoSerializer
{
	DAO_CSTRUCT_COMMON;

	DaoNamespace  *nspace;
	DaoProcess    *process;
	DaoParser     *parser;
	DaoValue      *value;
	DList         *types;
	DMap          *objects;
	DString       *serial;
	DString       *buffer;
	short          error;
};


#endif

DAO_API( SERIAL, DaoSerializer*, DaoSerializer_New, () );
DAO_API( SERIAL, void, DaoSerializer_Delete, (DaoSerializer *self) );
DAO_API( SERIAL, void, DaoSerializer_Reset, (DaoSerializer *self, DaoNamespace *ns) );
DAO_API( SERIAL, DString*,  DaoSerializer_Encode, (DaoSerializer *self, DaoValue *value) );
DAO_API( SERIAL, DaoValue*, DaoSerializer_Decode, (DaoSerializer *self, DString *input) );



DAO_SERIAL_DLL int DaoValue_Serialize( DaoValue *self, DString *serial, DaoNamespace *ns, DaoProcess *proc );
DAO_SERIAL_DLL int DaoValue_Deserialize( DaoValue **self, DString *serial, DaoNamespace *ns, DaoProcess *proc );

DAO_SERIAL_DLL void DaoNamespace_Backup( DaoNamespace *self, DaoProcess *proc, FILE *fout, int limit );
DAO_SERIAL_DLL void DaoNamespace_Restore( DaoNamespace *self, DaoProcess *proc, FILE *fin );

