/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2014-2016, Limin Fu
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

#include "dao_zip.h"
#include "daoString.h"
#include "daoValue.h"
#include "bzlib.h"

void DaoZip_Compress( DString *input, DString *output )
{
	unsigned int outputLen = input->size;
	int block = 100000;
	daoint size = input->size;

	DString_Reserve( output, size );
	size = 1 + size / block;
	if( size > 9 ) size = 9;
	BZ2_bzBuffToBuffCompress( output->chars, & outputLen, input->chars, input->size, size, 0, 30 );
	DString_Reset( output, outputLen );
}
void DaoZip_Decompress( DString *input, DString *output )
{
	unsigned int outputLen;

	/* TODO: get decompressed size from the compressed data? */
	DString_Reserve( output, input->size );
	outputLen = output->bufSize;
	while( outputLen == output->bufSize ){
		DString_Reserve( output, 2*outputLen );
		outputLen = output->bufSize;
		BZ2_bzBuffToBuffDecompress( output->chars, & outputLen, input->chars, input->size, 0, 0 );
	}
	DString_Reset( output, outputLen );
}

static void ZIP_Compress( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *source = p[0]->xString.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	DaoZip_Compress( source, res );
}
static void ZIP_Decompress( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *source = p[0]->xString.value;
	DString *res = DaoProcess_PutChars( proc, "" );
}


DaoType *daox_type_zipstream = NULL;

DaoZipStream* DaoZipStream_New()
{
	DaoZipStream *res = (DaoZipStream*)dao_malloc( sizeof(DaoZipStream) );
	res->file = NULL;
	res->bzfile = NULL;
	res->read = res->end = 0;
	return res;
}

void DaoZipStream_Close( DaoZipStream *self )
{
	if ( self->bzfile ){
		int bzerr;
		if ( self->read )
			BZ2_bzReadClose( &bzerr, self->bzfile );
		else
			BZ2_bzWriteClose( &bzerr, self->bzfile, 0, NULL, NULL );
		self->bzfile = NULL;
	}
	if ( self->file ){
		fclose( self->file );
		self->file = NULL;
	}
}

void DaoZipStream_Delete( DaoZipStream *self )
{
	DaoZipStream_Close( self );
	dao_free( self );
}

int DaoZipStream_Open( DaoZipStream *self, FILE *file, int read )
{
	int bzerr;
	if ( !file )
		return 0;
	self->file = file;
	if ( read )
		self->bzfile = BZ2_bzReadOpen( &bzerr, self->file, 0, 0, NULL, 0 );
	else
		self->bzfile = BZ2_bzWriteOpen( &bzerr, self->file, 9, 0, 30 );
	self->read = read;
	self->end = 0;
	return bzerr == BZ_OK;
}

int DaoZipStream_Read( DaoZipStream *self, DString *buffer, int count )
{
	int bzerr;
	if ( count < 0 ){
		struct stat info;
		fstat( fileno( self->file ), &info );
		count = info.st_size;
	}
	DString_Reserve( buffer, count );
	DString_Reset( buffer, BZ2_bzRead( &bzerr, self->bzfile, buffer->chars, count ) );
	self->end = ( bzerr == BZ_STREAM_END );
	return bzerr;
}

int DaoZipStream_Write( DaoZipStream *self, DString *data )
{
	int bzerr;
	BZ2_bzWrite( &bzerr, self->bzfile, data->chars, data->size );
	return bzerr;
}

static void ZIP_Open( DaoProcess *proc, DaoValue *p[], int N )
{
	char *mode = p[1]->xString.value->chars;
	if ( ( *mode != 'r' && *mode != 'w' ) || *( mode + 1 ) != '\0' )
		DaoProcess_RaiseError( proc, "Param", "Only 'r' and 'w' modes are supported" );
	else {
		DaoZipStream *res = DaoZipStream_New();
		if ( p[0]->type == DAO_STRING ){
			DString *file = p[0]->xString.value;
			if ( DaoZipStream_Open( res, fopen( file->chars, mode ), *mode == 'r' ) )
				DaoProcess_PutCdata( proc, res, daox_type_zipstream );
			else {
				char errbuf[512];
				snprintf( errbuf, sizeof(errbuf), "Failed to open file '%s'", file->chars );
				DaoProcess_RaiseError( proc, NULL, errbuf );
				DaoZipStream_Delete( res );
			}
		}
		else {
			if ( DaoZipStream_Open( res, fdopen( p[0]->xInteger.value, mode ), *mode == 'r' ) )
				DaoProcess_PutCdata( proc, res, daox_type_zipstream );
			else {
				char errbuf[100];
				int fn = p[0]->xInteger.value;
				snprintf( errbuf, sizeof(errbuf), "Failed to open file descriptor %i", fn );
				DaoProcess_RaiseError( proc, NULL, errbuf );
				DaoZipStream_Delete( res );
			}
		}
	}
}

static void ZIP_Read( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoZipStream *self = (DaoZipStream*)DaoValue_TryGetCdata( p[0] );
	int bzerr;
	dao_integer count = p[1]->xInteger.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	if ( !self->bzfile ){
		DaoProcess_RaiseError( proc, NULL, "Reading from a closed zip stream" );
		return;
	}
	DaoZipStream_Read( self, res, count );
}

static void ZIP_Write( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoZipStream *self = (DaoZipStream*)DaoValue_TryGetCdata( p[0] );
	DString *data = p[1]->xString.value;
	int bzerr;
	if ( !self->bzfile )
		DaoProcess_RaiseError( proc, NULL, "Writing to a closed zip stream" );
	else
		BZ2_bzWrite( &bzerr, self->bzfile, data->chars, data->size );
}

static void ZIP_Close( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoZipStream *self = (DaoZipStream*)DaoValue_TryGetCdata( p[0] );
	DaoZipStream_Close( self );
}

static void ZIP_Check( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoZipStream *self = (DaoZipStream*)DaoValue_TryGetCdata( p[0] );
	int res = 0;
	switch ( p[1]->xEnum.value ){
	case 0:	res = self->bzfile && self->read; break;
	case 1:	res = self->bzfile && !self->read; break;
	case 2:	res = self->bzfile? 1 : 0; break;
	case 3:
		if ( self->bzfile && self->end ){
			int bzerr, size = 0;
			void *data;
			BZ2_bzReadGetUnused( &bzerr, self->bzfile, &data, &size );
			res = size == 0;
		}
		break;
	}
	DaoProcess_PutBoolean( proc, res );
}

static void ZIP_ReadFile( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoZipStream *stream = DaoZipStream_New();
	int silent = p[1]->xBoolean.value;
	int bzerr;
	dao_integer count = p[0]->xInteger.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	if ( p[0]->type == DAO_STRING ){
		DString *file = p[0]->xString.value;
		if ( !DaoZipStream_Open( stream, fopen( file->chars, "r" ), 1 ) ){
			if ( !silent ) {
				char errbuf[512];
				snprintf( errbuf, sizeof(errbuf), "Failed to open file '%s'", file->chars );
				DaoProcess_RaiseError( proc, NULL, errbuf );
			}
			DaoZipStream_Delete( stream );
			return;
		}
	}
	else if ( !DaoZipStream_Open( stream, fdopen( p[0]->xInteger.value, "r" ), 1 ) ){
			if ( !silent ){
				char errbuf[100];
				int fn = p[0]->xInteger.value;
				snprintf( errbuf, sizeof(errbuf), "Failed to open file descriptor %i", fn );
				DaoProcess_RaiseError( proc, NULL, errbuf );
				DaoZipStream_Delete( stream );
			}
			DaoZipStream_Delete( stream );
			return;
		}
	if ( count < 0 ){
		struct stat info;
		fstat( fileno( stream->file ), &info );
		count = info.st_size;
	}
	DString_Reserve( res, count );
	DString_Reset( res, BZ2_bzRead( &bzerr, stream->bzfile, res->chars, count ) );
	DaoZipStream_Delete( stream );
}


static DaoFunctionEntry zipstreamMeths[] =
{
	{ ZIP_Open,   "Stream(file: string, mode: string) => Stream" },
	{ ZIP_Open,   "Stream(fileno: int, mode: string) => Stream" },
	{ ZIP_Read,   "read(self: Stream, count = -1) => string" },
	{ ZIP_Write,  "write(self: Stream, data: string)" },
	{ ZIP_Check,  "check(self: Stream, what: enum<readable,writable,open,eof>) => bool" },
	{ ZIP_Close,  "close(self: Stream)" },
	{ NULL, NULL }
};


static void DaoZipStream_CoreDelete( DaoValue *self )
{
	DaoZipStream_Delete( (DaoZipStream*) self->xCdata.data );
	DaoCstruct_Delete( (DaoCstruct*) self );
}

DaoTypeCore daoZipStreamCore =
{
	"Stream",                                              /* name */
	sizeof(DaoZipStream),                                  /* size */
	{ NULL },                                              /* bases */
	NULL,                                                  /* numbers */
	zipstreamMeths,                                        /* methods */
	DaoCstruct_CheckGetField,    DaoCstruct_DoGetField,    /* GetField */
	NULL,                        NULL,                     /* SetField */
	NULL,                        NULL,                     /* GetItem */
	NULL,                        NULL,                     /* SetItem */
	NULL,                        NULL,                     /* Unary */
	NULL,                        NULL,                     /* Binary */
	NULL,                        NULL,                     /* Conversion */
	NULL,                        NULL,                     /* ForEach */
	NULL,                                                  /* Print */
	NULL,                                                  /* Slice */
	NULL,                                                  /* Compare */
	NULL,                                                  /* Hash */
	NULL,                                                  /* Create */
	NULL,                                                  /* Copy */
	(DaoDeleteFunction) DaoZipStream_CoreDelete,           /* Delete */
	NULL                                                   /* HandleGC */
};

static DaoFunctionEntry zipMeths[]=
{
	{ ZIP_Compress,     "compress( source: string ) => string" },
	{ ZIP_Decompress,   "decompress( source: string ) => string" },
	{ ZIP_Open,			"open(file: string, mode: string) => Stream" },
	{ ZIP_Open,			"open(fileno: int, mode: string) => Stream" },
	{ ZIP_ReadFile,		"read(file: string, silent = false) => string" },
	{ ZIP_ReadFile,		"read(fileno: int, silent = false) => string" },
	{ NULL, NULL }
};

DAO_DLL int DaoZip_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	ns = DaoNamespace_GetNamespace( ns, "zip" );
	daox_type_zipstream = DaoNamespace_WrapType( ns, & daoZipStreamCore, DAO_CDATA, 0 );
	DaoNamespace_WrapFunctions( ns, zipMeths );
	return 0;
}
