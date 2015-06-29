
#include "dao.h"
#include "daoString.h"
#include "daoValue.h"
#include "bzlib.h"

static void ZIP_Compress( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *source = p[0]->xString.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	unsigned int resLen = source->size;
	int block = 100000;
	daoint size = source->size;

	DString_Reserve( res, size );
	size = 1 + size / block;
	if( size > 9 ) size = 9;
	BZ2_bzBuffToBuffCompress( res->chars, & resLen, source->chars, source->size, size, 0, 30 );
	DString_Reset( res, resLen );
}
static void ZIP_Decompress( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *source = p[0]->xString.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	unsigned int resLen;

	/* TODO: get decompressed size from the compressed data? */
	DString_Reserve( res, source->size );
	resLen = res->bufSize;
	while( resLen == res->bufSize ){
		DString_Reserve( res, 2*resLen );
		resLen = res->bufSize;
		BZ2_bzBuffToBuffDecompress( res->chars, & resLen, source->chars, source->size, 0, 0 );
	}
	DString_Reset( res, resLen );
}

typedef struct DaoZipStream DaoZipStream;

struct DaoZipStream
{
	FILE *file;
	void *bzfile;
	short read, end;
};

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

static void DaoZipStream_LibClose( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoZipStream *self = (DaoZipStream*)DaoValue_TryGetCdata( p[0] );
	DaoZipStream_Close( self );
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

static void DaoZipStream_Read( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoZipStream *self = (DaoZipStream*)DaoValue_TryGetCdata( p[0] );
	int bzerr;
	dao_integer count = p[1]->xInteger.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	if ( !self->bzfile )
		DaoProcess_RaiseError( proc, NULL, "Reading from a closed zip stream" );
	else {
		if ( count < 0 ){
			struct stat info;
			fstat( fileno( self->file ), &info );
			count = info.st_size;
		}
		DString_Reserve( res, count );
		DString_Reset( res, BZ2_bzRead( &bzerr, self->bzfile, res->chars, count ) );
		self->end = ( bzerr == BZ_STREAM_END );
	}
}

static void DaoZipStream_Write( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoZipStream *self = (DaoZipStream*)DaoValue_TryGetCdata( p[0] );
	DString *data = p[1]->xString.value;
	int bzerr;
	if ( !self->bzfile )
		DaoProcess_RaiseError( proc, NULL, "Writing to a closed zip stream" );
	else
		BZ2_bzWrite( &bzerr, self->bzfile, data->chars, data->size );
}

static void DaoZipStream_Check( DaoProcess *proc, DaoValue *p[], int N )
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


static DaoFuncItem zipstreamMeths[] =
{
	{ ZIP_Open,				"Stream(file: string, mode: string) => Stream" },
	{ ZIP_Open,				"Stream(fileno: int, mode: string) => Stream" },
	{ DaoZipStream_Read,	"read(self: Stream, count = -1) => string" },
	{ DaoZipStream_Write,	"write(self: Stream, data: string)" },
	{ DaoZipStream_Check,	"check(self: Stream, what: enum<readable,writable,open,eof>) => bool" },
	{ DaoZipStream_LibClose,"close(self: Stream)" },
	{ NULL, NULL }
};

DaoTypeBase zipstreamTyper =
{
	"Stream", NULL, NULL, (DaoFuncItem*) zipstreamMeths, {0}, {0},
	(FuncPtrDel) DaoZipStream_Delete, NULL
};

static DaoFuncItem zipMeths[]=
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
	daox_type_zipstream = DaoNamespace_WrapType( ns, &zipstreamTyper, 1 );
	DaoNamespace_WrapFunctions( ns, zipMeths );
	return 0;
}
