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

#define DAO_IMAGE

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "dao_image.h"
#include "daoValue.h"
#include "lode_png.h"
#include "micro_jpeg.h"


DaoType *daox_type_image = NULL;

DaoType* DaoImage_Type()
{
	return daox_type_image;
}


DaoImage* DaoImage_New()
{
	DaoImage *self = (DaoImage*) dao_calloc( 1, sizeof(DaoImage) );
	DaoCstruct_Init( (DaoCstruct*)self, daox_type_image );
	self->depth = DAOX_IMAGE_BIT32;
	return self;
}
void DaoImage_Delete( DaoImage *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DArray_Clear( & self->buffer );
	dao_free( self );
}

void DaoImage_Resize( DaoImage *self, int width, int height )
{
	int pixelBytes = 1 + self->depth;
	int widthStep = width * pixelBytes;

	assert( self->depth <= DAOX_IMAGE_BIT32 );

	if( pixelBytes != self->buffer.stride ){
		self->buffer.size = (self->buffer.size * self->buffer.stride) / pixelBytes;
		self->buffer.capacity = (self->buffer.capacity * self->buffer.stride) / pixelBytes;
		self->buffer.stride = pixelBytes;
	}
	if( widthStep % 4 ) widthStep += 4 - (widthStep % 4);
	self->width = width;
	self->height = height;
	self->stride = widthStep;
	DArray_Resize( & self->buffer, widthStep * height );
	memset( self->buffer.data.base, 0, widthStep * height );
}
int DaoImage_Convert( DaoImage *self, int dep )
{
	if( self->depth == dep ) return 1;
	return 0;
}


int DaoImage_DecodeBMP( DaoImage *self, DString *data );

int DaoImage_LoadBMP( DaoImage *self, const char *file )
{
	DString *mbs = DString_New();
	FILE *fin = fopen( file, "r+" );

	if( fin == NULL ) goto Failed;
	DaoFile_ReadAll( fin, mbs, 1 );

	if( DaoImage_DecodeBMP( self, mbs ) == 0 ) goto Failed;

Done:
	DString_Delete( mbs );
	return 1;
Failed:
	DString_Delete( mbs );
	return 0;
}

int dao_read_int( uchar_t *data )
{
	return data[0] + (data[1]<<8) + (data[2]<<16) + (data[3]<<24);
}
short dao_read_short( uchar_t *data )
{
	return data[0] + (data[1]<<8);
}

int DaoImage_DecodeBMP( DaoImage *self, DString *buffer )
{
	uchar_t *data = (uchar_t*) buffer->chars;
	int fileSize, pixelArray, pixelBytes, numBytes;
	int i, j, width, height, pixelBits;

	if( data[0x0] != 'B' || data[0x1] != 'M' ) return 0; /* Not a BMP image; */
	if( dao_read_int( data + 0xE ) != 40 ) return 0;     /* Format not supported; */
	if( dao_read_int( data + 0x2E ) ) return 0;          /* Palette not supported; */

	fileSize   = dao_read_int( data + 0x2 );
	pixelArray = dao_read_int( data + 0xA );
	width      = dao_read_int( data + 0x12 );
	height     = dao_read_int( data + 0x16 );
	pixelBits  = dao_read_short( data + 0x1C );
	numBytes   = dao_read_int( data + 0x22 );

	printf( "pixelBits : %i\n", pixelBits );

	/*
	// Supported pixel format:
	// 24: 8.8.8.0.0
	// 32: 8.8.8.8.0
	*/
	switch( pixelBits ){
	case 24 : self->depth = DAOX_IMAGE_BIT24; break;
	case 32 : self->depth = DAOX_IMAGE_BIT32; break;
	default: return 0;
	}
	DaoImage_Resize( self, width, height );

	pixelBytes = self->buffer.stride;
	for(i=0; i<height; ++i){
		uchar_t *dest = self->buffer.data.uchars + i * self->stride;
		uchar_t *src = data + pixelArray + i * self->stride;
		for(j=0;  j<width;  ++j, dest += pixelBytes, src += pixelBytes){
			dest[0] = src[2];
			dest[1] = src[1];
			dest[2] = src[0];
			if( pixelBits == 32 ) dest[3] = src[3];
		}
	}
	return 1;
}

void daox_write_int( FILE *fout, int i )
{
	fprintf( fout, "%c%c%c%c", i&0xFF, (i>>8)&0xFF, (i>>16)&0xFF, (i>>24)&0xFF );
}
void daox_write_short( FILE *fout, short i )
{
	fprintf( fout, "%c%c", i&0xFF, (i>>8)&0xFF );
}
int DaoImage_SaveBMP( DaoImage *self, const char *file )
{
	FILE *fout = fopen( file, "w+" );
	int i, j, pixelBytes = self->buffer.stride;

	if( fout == NULL ) return 0;

	fprintf( fout, "BM" );
	daox_write_int( fout, 14 + 40 + self->buffer.size * self->buffer.stride );
	daox_write_short( fout, 0 );
	daox_write_short( fout, 0 );
	daox_write_int( fout, 14 + 40 );
	daox_write_int( fout, 40 );
	daox_write_int( fout, self->width );
	daox_write_int( fout, self->height );
	daox_write_short( fout, 1 );
	daox_write_short( fout, 8*(1+self->depth) );
	daox_write_int( fout, 0 );
	daox_write_int( fout, self->buffer.size * self->buffer.stride );
	daox_write_int( fout, 2835  );
	daox_write_int( fout, 2835  );
	daox_write_int( fout, 0 );
	daox_write_int( fout, 0 );
	for(i=0; i<self->height; ++i){
		uchar_t *pix = self->buffer.data.uchars + i * self->stride;
		for(j=0;  j<self->width;  ++j, pix += pixelBytes){
			fprintf( fout, "%c%c%c", pix[2], pix[1], pix[0] );
			if( self->depth == DAOX_IMAGE_BIT32 ) fprintf( fout, "%c", pix[3] );
		}
	}
	fclose( fout );
	return 1;
}

void daox_encode_int( DString *buffer, int i )
{
	DString_AppendChar( buffer, i&0xFF );
	DString_AppendChar( buffer, (i>>8)&0xFF );
	DString_AppendChar( buffer, (i>>16)&0xFF );
	DString_AppendChar( buffer, (i>>24)&0xFF );
}
void daox_encode_short( DString *buffer, short i )
{
	DString_AppendChar( buffer, i&0xFF );
	DString_AppendChar( buffer, (i>>8)&0xFF );
}

int DaoImage_EncodeBMP( DaoImage *self, DString *buffer )
{
	int i, j, pixelBytes = self->buffer.stride;

	DString_AppendChars( buffer, "BM" );
	daox_encode_int( buffer, 14 + 40 + self->buffer.size * self->buffer.stride );
	daox_encode_short( buffer, 0 );
	daox_encode_short( buffer, 0 );
	daox_encode_int( buffer, 14 + 40 );
	daox_encode_int( buffer, 40 );
	daox_encode_int( buffer, self->width );
	daox_encode_int( buffer, self->height );
	daox_encode_short( buffer, 1 );
	daox_encode_short( buffer, 8*(1+self->depth) );
	daox_encode_int( buffer, 0 );
	daox_encode_int( buffer, self->buffer.size * self->buffer.stride );
	daox_encode_int( buffer, 2835  );
	daox_encode_int( buffer, 2835  );
	daox_encode_int( buffer, 0 );
	daox_encode_int( buffer, 0 );
	for(i=0; i<self->height; ++i){
		uchar_t *pix = self->buffer.data.uchars + i * self->stride;
		for(j=0;  j<self->width;  ++j, pix += pixelBytes){
			DString_AppendChar( buffer, pix[2] );
			DString_AppendChar( buffer, pix[1] );
			DString_AppendChar( buffer, pix[0] );
			if( self->depth == DAOX_IMAGE_BIT32 ){
				DString_AppendChar( buffer, pix[3] );
			}
		}
	}
	return 1;
}

void DaoImage_SetData( DaoImage *self, unsigned char *buffer, int width, int height, int dep )
{
	unsigned i, j, pixelBytes;

	self->depth = dep;
	DaoImage_Resize( self, width, height );

	pixelBytes = self->buffer.stride;
	for(i=0; i<height; ++i){
		uchar_t *dest = self->buffer.data.uchars + (height - i - 1) * self->stride;
		uchar_t *src = buffer + i * width * pixelBytes;
		memcpy( dest, src, width*pixelBytes*sizeof(uchar_t) );
	}
}
int DaoImage_DecodePNG( DaoImage *self, DString *data )
{
	unsigned char *buffer = NULL;
	unsigned char *bytes = (unsigned char*) data->chars;
	unsigned width = 0, height = 0;
	unsigned ret = lodepng_decode32( & buffer, & width, & height, bytes, data->size );

	if( ret ){
		if( buffer ) dao_free( buffer );
		return 0;
	}
	DaoImage_SetData( self, buffer, width, height, DAOX_IMAGE_BIT32 );
	dao_free( buffer );
	return 1;
}
int DaoImage_LoadPNG( DaoImage *self, const char *file )
{
	unsigned char *buffer = NULL;
	unsigned width = 0, height = 0;
	unsigned ret = lodepng_decode32_file( & buffer, & width, & height, file );

	if( ret ){
		if( buffer ) dao_free( buffer );
		return 0;
	}
	DaoImage_SetData( self, buffer, width, height, DAOX_IMAGE_BIT32 );
	dao_free( buffer );
	return 1;
}
int DaoImage_SavePNG( DaoImage *self, const char *file )
{
	unsigned i, pixelBytes = self->buffer.stride;
	unsigned char *buffer = dao_malloc( self->width * self->height * pixelBytes );

	for(i=0; i<self->height; ++i){
		uchar_t *src = self->buffer.data.uchars + (self->height - i - 1) * self->stride;
		uchar_t *dest = buffer + i * self->width * pixelBytes;
		memcpy( dest, src, self->width*pixelBytes*sizeof(uchar_t) );
	}

	if( self->depth == DAOX_IMAGE_BIT32 ){
		lodepng_encode32_file( file, buffer, self->width, self->height );
	}else if( self->depth == DAOX_IMAGE_BIT24 ){
		lodepng_encode24_file( file, buffer, self->width, self->height );
	}else{
		dao_free( buffer );
		return 0;
	}
	dao_free( buffer );
	return 1;
}
int DaoImage_DecodeJPEG( DaoImage *self, DString *data )
{
	unsigned char *buffer = NULL;
	ujImage im = ujDecode( NULL, data->chars, data->size );

	if( im == NULL ) return 0;

	buffer = ujGetImage( im, NULL );
	DaoImage_SetData( self, buffer, ujGetWidth(im), ujGetWidth(im), DAOX_IMAGE_BIT24 );

	ujDestroy( im );
	return 1;
}
int DaoImage_LoadJPEG( DaoImage *self, const char *file )
{
	unsigned char *buffer = NULL;
	ujImage im = ujDecodeFile( NULL, file );

	if( im == NULL ) return 0;

	buffer = ujGetImage( im, NULL );
	DaoImage_SetData( self, buffer, ujGetWidth(im), ujGetWidth(im), DAOX_IMAGE_BIT24 );

	ujDestroy( im );
	return 1;
}
int DaoImage_Decode( DaoImage *self, DString *data )
{
	if( DaoImage_DecodeBMP( self, data ) ) return 1;
	if( DaoImage_DecodePNG( self, data ) ) return 1;
	return DaoImage_DecodeJPEG( self, data );
}
int DaoImage_EncodePNG( DaoImage *self, DString *data )
{
	unsigned i, pixelBytes = self->buffer.stride;
	unsigned char *buffer = dao_malloc( self->width * self->height * pixelBytes );
	unsigned char *out = NULL;
	size_t outsize = 0;

	for(i=0; i<self->height; ++i){
		uchar_t *src = self->buffer.data.uchars + (self->height - i - 1) * self->stride;
		uchar_t *dest = buffer + i * self->width * pixelBytes;
		memcpy( dest, src, self->width*pixelBytes*sizeof(uchar_t) );
	}

	if( self->depth == DAOX_IMAGE_BIT32 ){
		lodepng_encode32( & out, & outsize, buffer, self->width, self->height );
	}else if( self->depth == DAOX_IMAGE_BIT24 ){
		lodepng_encode24( & out, & outsize, buffer, self->width, self->height );
	}else{
		dao_free( buffer );
		return 0;
	}
	if( out ){
		DString_SetBytes( data, (char*) out, outsize );
		free( out );
	}
	dao_free( buffer );
	return 1;
}
int DaoImage_Encode( DaoImage *self, DString *data, int format )
{
	if( format == 0 ) return DaoImage_EncodeBMP( self, data );
	return DaoImage_EncodePNG( self, data );
}

void DaoImage_Export( DaoImage *self, DaoArray *matrix, float factor )
{
	int i, j, pixelBytes = self->buffer.stride;
	daoint dims[2];

	dims[0] = self->height;
	dims[1] = self->width;
	DaoArray_ResizeArray( matrix, dims, 2 );
	for(i=0; i<self->height; ++i){
		uchar_t *pix = self->buffer.data.uchars + i * self->stride;
		for(j=0;  j<self->width;  ++j, pix += pixelBytes){
			daoint k = i * self->width + j;
			switch( matrix->etype ){
			case DAO_INTEGER : matrix->data.i[k] = factor * (*pix); break;
			case DAO_FLOAT   : matrix->data.f[k] = factor * (*pix); break;
			}
		}
	}
}




static void IMAGE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoImage *self = DaoImage_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void IMAGE_Load( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoImage *self = (DaoImage*) p[0];
	DString *codePath = proc->activeRoutine->nameSpace->path;
	DString *file = p[1]->xString.value;
	int ret = 0;
	DString_MakePath( codePath, file );
	if( DString_Match( file, "<I>%.PNG $", NULL, NULL ) ){
		ret = DaoImage_LoadPNG( self, file->chars );
	}else if( DString_Match( file, "<I>%. (JPG|JPEG) $", NULL, NULL ) ){
		ret = DaoImage_LoadJPEG( self, file->chars );
	}else if( DString_Match( file, "<I>%.BMP $", NULL, NULL ) ){
		ret = DaoImage_LoadBMP( self, file->chars );
	}
	if( ret == 0 ) DaoProcess_RaiseError( proc, NULL, "file format not supported" );
}
static void IMAGE_Save( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoImage *self = (DaoImage*) p[0];
	DString *codePath = proc->activeRoutine->nameSpace->path;
	DString *file = p[1]->xString.value;
	int ret = 0;
	DString_MakePath( codePath, file );
	if( DString_Match( file, "<I>%.PNG $", NULL, NULL ) ){
		ret = DaoImage_SavePNG( self, file->chars );
	}else if( DString_Match( file, "<I>%.BMP $", NULL, NULL ) ){
		ret = DaoImage_SaveBMP( self, file->chars );
	}
	if( ret == 0 ) DaoProcess_RaiseError( proc, NULL, "file saving failed" );
}
static void IMAGE_Encode( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoImage *self = (DaoImage*) p[0];
	DString *res = DaoProcess_PutChars( proc, "" );
	DaoImage_Encode( self, res, p[1]->xEnum.value );
}
static void IMAGE_Decode( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoImage *self = (DaoImage*) p[0];
	int ret = DaoImage_Decode( self, p[1]->xString.value );
	DaoProcess_PutBoolean( proc, ret );
}
static void IMAGE_Export( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoImage *self = (DaoImage*) p[0];
	DaoArray *matrix = (DaoArray*) p[2];
	double factor = DaoValue_GetFloat( p[3] );
	int channels = p[1]->xEnum.value; // TODO:

	DaoImage_Export( self, matrix, factor );
}
static void IMAGE_New2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoImage *self = DaoImage_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
	DaoImage_Decode( self, p[0]->xString.value );
}
static void IMAGE_Serialize( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoImage *self = (DaoImage*) p[0];
	DString *res = DaoProcess_PutChars( proc, "" );
	DaoImage_Encode( self, res, 1 );
}

static DaoFunctionEntry daoImageMeths[]=
{
	{ IMAGE_New,     "Image()" },
	{ IMAGE_Load,    "Load( self: Image, file: string )" },
	{ IMAGE_Save,    "Save( self: Image, file: string )" },
	{ IMAGE_Encode,  "Encode( self: Image, format: enum<bmp,png> ) => string" },
	{ IMAGE_Decode,  "Decode( self: Image, data: string ) => bool" },
	{ IMAGE_Export,  "Export( self: Image, channel: enum<red;grean;blue;alpha>, matrix: array<@T<int|float>>, factor: @T = 1 )" },

	{ IMAGE_New2,       "Image( data: string )" },
	{ IMAGE_Serialize,  "serialize( self: Image ) => string" },
	{ NULL, NULL }
};


DaoTypeCore daoImageCore =
{
	"Image",                                           /* name */
	sizeof(DaoImage),                                  /* size */
	{ NULL },                                          /* bases */
	NULL,                                              /* numbers */
	daoImageMeths,                                     /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* GetField */
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
	(DaoDeleteFunction) DaoImage_Delete,               /* Delete */
	NULL                                               /* HandleGC */
};

#undef DAO_IMAGE
#undef DAO_IMAGE_DLL
#define DAO_HAS_IMAGE
#include"dao_api.h"

DAO_DLL_EXPORT int DaoImage_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_image = DaoNamespace_WrapType( ns, & daoImageCore, DAO_CSTRUCT, 0 );

#define DAO_API_INIT
#include"dao_api.h"

	return 0;
}
