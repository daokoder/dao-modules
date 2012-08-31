/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2012, Limin Fu
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
#include "dao_image.h"


DaoType *daox_type_image = NULL;


DaoxImage* DaoxImage_New()
{
	DaoxImage *self = (DaoxImage*) calloc( 1, sizeof(DaoxImage) );
	DaoCdata_InitCommon( (DaoCdata*)self, daox_type_image );
	return self;
}
void DaoxImage_Delete( DaoxImage *self )
{
	if( self->imageData ) dao_free( self->imageData );
	DaoCdata_FreeCommon( (DaoCdata*) self );
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
	DString *mbs = DString_New(1);
	FILE *fin = fopen( file, "r+" );
	int fileSize, pixelArray, pixelBytes, numBytes;
	int i, j, width, height, pixelBits;
	uchar_t *data;

	if( fin == NULL ) goto Failed;
	DaoFile_ReadAll( fin, mbs, 1 );
	data = (uchar_t*) mbs->mbs;

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





static void IMAGE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxImage *self = DaoxImage_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void IMAGE_Load( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxImage *self = (DaoxImage*) p[0];
	char *file = DaoValue_TryGetMBString( p[1] );
	if( DaoxImage_LoadBMP( self, file ) == 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "file format not supported" );
	}
}
static void IMAGE_Save( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxImage *self = (DaoxImage*) p[0];
	char *file = DaoValue_TryGetMBString( p[1] );
	if( DaoxImage_SaveBMP( self, file ) == 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "file saving failed" );
	}
}
static DaoFuncItem DaoxImageMeths[]=
{
	{ IMAGE_New,     "Image()" },
	{ IMAGE_Load,    "Load( self: Image, file: string )" },
	{ IMAGE_Save,    "Save( self: Image, file: string )" },
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
