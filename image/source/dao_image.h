/*
// Dao Graphics Engine
// http://www.daovm.net
//
// Copyright (c) 2012-2015, Limin Fu
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

#ifndef DAO_IMAGE_DLL
#ifdef DAO_IMAGE
#  define DAO_IMAGE_DLL DAO_DLL_EXPORT
#else
#  define DAO_IMAGE_DLL DAO_DLL_IMPORT
#endif
#endif


#ifndef __DAO_IMAGE_H__
#define __DAO_IMAGE_H__

#include "daoStdtype.h"

typedef struct DaoImage  DaoImage;


/*
// Image depths including the alpha channel:
*/
enum DaoImageDepth
{
	DAOX_IMAGE_BIT8 ,
	DAOX_IMAGE_BIT16 ,
	DAOX_IMAGE_BIT24 ,
	DAOX_IMAGE_BIT32
};


/*
// DaoImage supports only RGBA with different depth.
// Each channel is encoded in the same number of bits.
*/
struct DaoImage
{
	DAO_CSTRUCT_COMMON;

	DArray  buffer;  /* Data buffer; */
	uint_t  stride;  /* Number of pixels per row in the buffer; */
	uint_t  width;   /* Number of pixels per row in the image; */
	uint_t  height;  /* Number of pixels per column in the image; */
	uint_t  depth;   /* Color depth type; */
};
#endif

DAO_API( IMAGE, DaoType*, DaoImage_Type, (DaoVmSpace *vmspace) );

DAO_API( IMAGE, DaoImage*, DaoImage_New, (DaoType *type) );
DAO_API( IMAGE, void, DaoImage_Delete, (DaoImage *self) );

DAO_API( IMAGE, void, DaoImage_Resize, (DaoImage *self, int width, int height) );

DAO_API( IMAGE, int, DaoImage_Convert, (DaoImage *self, int dep) );

DAO_API( IMAGE, int, DaoImage_Decode, (DaoImage *self, DString *data) );
DAO_API( IMAGE, int, DaoImage_Encode, (DaoImage *self, DString *data, int format) );

DAO_API( IMAGE, int, DaoImage_LoadBMP, (DaoImage *self, const char *file) );
DAO_API( IMAGE, int, DaoImage_LoadPNG, (DaoImage *self, const char *file) );
DAO_API( IMAGE, int, DaoImage_SaveBMP, (DaoImage *self, const char *file) );
DAO_API( IMAGE, int, DaoImage_SavePNG, (DaoImage *self, const char *file) );

DAO_API( IMAGE, void, DaoImage_Export, (DaoImage *self, DaoArray *matrix, float factor) );

