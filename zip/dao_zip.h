/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2014,2015, Limin Fu
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

#include "dao.h"

#ifndef DAO_ZIP_DLL
#ifdef DAO_ZIP
#  define DAO_ZIP_DLL DAO_DLL_EXPORT
#else
#  define DAO_ZIP_DLL DAO_DLL_IMPORT
#endif
#endif

#ifndef __DAO_ZIP_H__
#define __DAO_ZIP_H__

typedef struct DaoZipStream DaoZipStream;

struct DaoZipStream
{
	FILE *file;
	void *bzfile;
	short read, end;
};


#endif

DAO_API( DAO_ZIP_DLL, void, DaoZip_Compress, (DString *input, DString *output) );
DAO_API( DAO_ZIP_DLL, void, DaoZip_Decompress, (DString *input, DString *output) );


DAO_API( DAO_ZIP_DLL, DaoZipStream*, DaoZipStream_New, () );
DAO_API( DAO_ZIP_DLL, void, DaoZipStream_Close, (DaoZipStream *self) );
DAO_API( DAO_ZIP_DLL, void, DaoZipStream_Delete, (DaoZipStream *self) );
DAO_API( DAO_ZIP_DLL, int, DaoZipStream_Open, (DaoZipStream *self, FILE *file, int read) );
DAO_API( DAO_ZIP_DLL, int, DaoZipStream_Read, (DaoZipStream *self, DString *buffer, int count) );
DAO_API( DAO_ZIP_DLL, int, DaoZipStream_Write, (DaoZipStream *self, DString *data) );
