/*
// Dao Graphics Engine
// http://www.daovm.net
//
// Copyright (c) 2012-2014, Limin Fu
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


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "daoValue.h"
#include "dao_image.h"
#include "lode_png.h"
#include "micro_jpeg.h"


DaoType *daox_type_image = NULL;


DaoxImage* DaoxImage_New()
{
	DaoxImage *self = (DaoxImage*) dao_calloc( 1, sizeof(DaoxImage) );
	DaoCstruct_Init( (DaoCstruct*)self, daox_type_image );
	return self;
}
void DaoxImage_Delete( DaoxImage *self )
{
	if( self->imageData ) dao_free( self->imageData );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}

void DaoxImage_Resize( DaoxImage *self, int width, int height )
{
	int pixelBytes = 1 + self->depth;
	int widthStep = width * pixelBytes;

	assert( self->depth <= DAOX_IMAGE_BIT32 );

	if( widthStep % 4 ) widthStep += 4 - (widthStep % 4);
	self->width = width;
	self->height = height;
	self->widthStep = widthStep;
	self->imageSize = widthStep * height;
	self->imageData = (uchar_t*)dao_realloc( self->imageData, self->imageSize*sizeof(uchar_t) );
}
int DaoxImage_Convert( DaoxImage *self, int dep )
{
	if( self->depth == dep ) return 1;
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

int DaoxImage_LoadBMP( DaoxImage *self, const char *file )
{
	DString *mbs = DString_New();
	FILE *fin = fopen( file, "r+" );
	int fileSize, pixelArray, pixelBytes, numBytes;
	int i, j, width, height, pixelBits;
	uchar_t *data;

	if( fin == NULL ) goto Failed;
	DaoFile_ReadAll( fin, mbs, 1 );
	data = (uchar_t*) mbs->chars;

	if( data[0x0] != 'B' || data[0x1] != 'M' ) goto Failed; /* Not a BMP image; */
	if( dao_read_int( data + 0xE ) != 40 ) goto Failed; /* Format not supported; */
	if( dao_read_int( data + 0x2E ) ) goto Failed; /* Palette not supported; */

	fileSize = dao_read_int( data + 0x2 );
	pixelArray = dao_read_int( data + 0xA );
	width = dao_read_int( data + 0x12 );
	height = dao_read_int( data + 0x16 );
	pixelBits = dao_read_short( data + 0x1C );
	numBytes = dao_read_int( data + 0x22 );

	printf( "pixelBits : %i\n", pixelBits );

	/*
	// Supported pixel format:
	// 24: 8.8.8.0.0
	// 32: 8.8.8.8.0
	*/
	switch( pixelBits ){
	case 24 : self->depth = DAOX_IMAGE_BIT24; break;
	case 32 : self->depth = DAOX_IMAGE_BIT32; break;
	default: goto Failed;
	}
	DaoxImage_Resize( self, width, height );

	pixelBytes = 1 + self->depth;
	for(i=0; i<height; ++i){
		uchar_t *dest = self->imageData + i * self->widthStep;
		uchar_t *src = data + pixelArray + i * self->widthStep;
		for(j=0;  j<width;  ++j, dest += pixelBytes, src += pixelBytes){
			dest[0] = src[2];
			dest[1] = src[1];
			dest[2] = src[0];
			if( pixelBits == 32 ) dest[3] = src[3];
		}
	}

Done:
	DString_Delete( mbs );
	return 1;
Failed:
	DString_Delete( mbs );
	return 0;
}

void daox_write_int( FILE *fout, int i )
{
	fprintf( fout, "%c%c%c%c", i&0xFF, (i>>8)&0xFF, (i>>16)&0xFF, (i>>24)&0xFF );
}
void daox_write_short( FILE *fout, short i )
{
	fprintf( fout, "%c%c", i&0xFF, (i>>8)&0xFF );
}
int DaoxImage_SaveBMP( DaoxImage *self, const char *file )
{
	FILE *fout = fopen( file, "w+" );
	int i, j, pixelBytes = 1 + self->depth;

	if( fout == NULL ) return 0;

	fprintf( fout, "BM" );
	daox_write_int( fout, 14 + 40 + self->imageSize );
	daox_write_short( fout, 0 );
	daox_write_short( fout, 0 );
	daox_write_int( fout, 14 + 40 );
	daox_write_int( fout, 40 );
	daox_write_int( fout, self->width );
	daox_write_int( fout, self->height );
	daox_write_short( fout, 1 );
	daox_write_short( fout, 8*(1+self->depth) );
	daox_write_int( fout, 0 );
	daox_write_int( fout, self->imageSize );
	daox_write_int( fout, 2835  );
	daox_write_int( fout, 2835  );
	daox_write_int( fout, 0 );
	daox_write_int( fout, 0 );
	for(i=0; i<self->height; ++i){
		uchar_t *pix = self->imageData + i * self->widthStep;
		for(j=0;  j<self->width;  ++j, pix += pixelBytes){
			fprintf( fout, "%c%c%c", pix[2], pix[1], pix[0] );
			if( self->depth == DAOX_IMAGE_BIT32 ) fprintf( fout, "%c", pix[3] );
		}
	}
	fclose( fout );
	return 1;
}

void DaoxImage_SetData( DaoxImage *self, unsigned char *buffer, int width, int height, int dep )
{
	unsigned i, j, pixelBytes;

	self->depth = dep;
	DaoxImage_Resize( self, width, height );

	pixelBytes = 1 + self->depth;
	for(i=0; i<height; ++i){
		uchar_t *dest = self->imageData + (height - i - 1) * self->widthStep;
		uchar_t *src = buffer + i * width * pixelBytes;
		memcpy( dest, src, width*pixelBytes*sizeof(uchar_t) );
	}
}
int DaoxImage_DecodePNG( DaoxImage *self, DString *data )
{
	unsigned char *buffer = NULL;
	unsigned char *bytes = (unsigned char*) data->chars;
	unsigned width = 0, height = 0;
	unsigned ret = lodepng_decode32( & buffer, & width, & height, bytes, data->size );

	if( ret ){
		if( buffer ) dao_free( buffer );
		return 0;
	}
	DaoxImage_SetData( self, buffer, width, height, DAOX_IMAGE_BIT32 );
	dao_free( buffer );
	return 1;
}
int DaoxImage_LoadPNG( DaoxImage *self, const char *file )
{
	unsigned char *buffer = NULL;
	unsigned width = 0, height = 0;
	unsigned ret = lodepng_decode32_file( & buffer, & width, & height, file );

	if( ret ){
		if( buffer ) dao_free( buffer );
		return 0;
	}
	DaoxImage_SetData( self, buffer, width, height, DAOX_IMAGE_BIT32 );
	dao_free( buffer );
	return 1;
}
int DaoxImage_SavePNG( DaoxImage *self, const char *file )
{
	unsigned i, j, pixelBytes = 1 + self->depth;
	unsigned char *buffer = dao_malloc( self->width * self->height * pixelBytes );

	for(i=0; i<self->height; ++i){
		uchar_t *src = self->imageData + (self->height - i - 1) * self->widthStep;
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
int DaoxImage_DecodeJPEG( DaoxImage *self, DString *data )
{
	unsigned char *buffer = NULL;
	ujImage im = ujDecode( NULL, data->chars, data->size );

	if( im == NULL ) return 0;

	buffer = ujGetImage( im, NULL );
	DaoxImage_SetData( self, buffer, ujGetWidth(im), ujGetWidth(im), DAOX_IMAGE_BIT24 );

	ujDestroy( im );
	return 1;
}
int DaoxImage_LoadJPEG( DaoxImage *self, const char *file )
{
	unsigned char *buffer = NULL;
	ujImage im = ujDecodeFile( NULL, file );

	if( im == NULL ) return 0;

	buffer = ujGetImage( im, NULL );
	DaoxImage_SetData( self, buffer, ujGetWidth(im), ujGetWidth(im), DAOX_IMAGE_BIT24 );

	ujDestroy( im );
	return 1;
}
int DaoxImage_Decode( DaoxImage *self, DString *data )
{
	if( DaoxImage_DecodePNG( self, data ) ) return 1;
	return DaoxImage_DecodeJPEG( self, data );
}

void DaoxImage_Export( DaoxImage *self, DaoArray *matrix, float factor )
{
	int i, j, pixelBytes = 1 + self->depth;
	daoint dims[2];

	dims[0] = self->height;
	dims[1] = self->width;
	DaoArray_ResizeArray( matrix, dims, 2 );
	for(i=0; i<self->height; ++i){
		uchar_t *pix = self->imageData + i * self->widthStep;
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
	DaoxImage *self = DaoxImage_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void IMAGE_Load( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxImage *self = (DaoxImage*) p[0];
	DString *codePath = proc->activeRoutine->nameSpace->path;
	DString *file = p[1]->xString.value;
	int ret = 0;
	Dao_MakePath( codePath, file );
	if( DString_Match( file, "<I>%.PNG $", NULL, NULL ) ){
		ret = DaoxImage_LoadPNG( self, file->chars );
	}else if( DString_Match( file, "<I>%. (JPG|JPEG) $", NULL, NULL ) ){
		ret = DaoxImage_LoadJPEG( self, file->chars );
	}else if( DString_Match( file, "<I>%.BMP $", NULL, NULL ) ){
		ret = DaoxImage_LoadBMP( self, file->chars );
	}
	if( ret == 0 ) DaoProcess_RaiseError( proc, NULL, "file format not supported" );
}
static void IMAGE_Save( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxImage *self = (DaoxImage*) p[0];
	DString *codePath = proc->activeRoutine->nameSpace->path;
	DString *file = p[1]->xString.value;
	int ret = 0;
	Dao_MakePath( codePath, file );
	if( DString_Match( file, "<I>%.PNG $", NULL, NULL ) ){
		ret = DaoxImage_SavePNG( self, file->chars );
	}else if( DString_Match( file, "<I>%.BMP $", NULL, NULL ) ){
		ret = DaoxImage_SaveBMP( self, file->chars );
	}
	if( ret == 0 ) DaoProcess_RaiseError( proc, NULL, "file saving failed" );
}
static void IMAGE_Export( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxImage *self = (DaoxImage*) p[0];
	DaoArray *matrix = (DaoArray*) p[2];
	double factor = DaoValue_GetFloat( p[3] );
	int channels = p[1]->xEnum.value; // TODO:

	DaoxImage_Export( self, matrix, factor );
}
static DaoFuncItem DaoxImageMeths[]=
{
	{ IMAGE_New,     "Image()" },
	{ IMAGE_Load,    "Load( self: Image, file: string )" },
	{ IMAGE_Save,    "Save( self: Image, file: string )" },
	{ IMAGE_Export,  "Export( self: Image, channel: enum<red;grean;blue;alpha>, matrix: array<@T<int|float>>, factor: @T = 1 )" },
	{ NULL, NULL }
};

DaoTypeBase DaoxImage_Typer =
{
	"Image", NULL, NULL, (DaoFuncItem*) DaoxImageMeths, { NULL }, { NULL },
	(FuncPtrDel)DaoxImage_Delete, NULL
};


DAO_DLL int DaoImage_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_image = DaoNamespace_WrapType( ns, & DaoxImage_Typer, 0 );
	return 0;
}
