
#include "dao.h"
#include "daoString.h"
#include "daoValue.h"
#include "bzlib.h"

static void ZIP_Compress( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *source = p[0]->xString.value;
	DString *res = DaoProcess_PutChars( proc, "" );
	unsigned int resLen = source->size;

	DString_Reserve( res, source->size );
	BZ2_bzBuffToBuffCompress( res->chars, & resLen, source->chars, source->size, 9, 0, 30 );
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

static DaoFuncItem zipMeths[]=
{
	{ ZIP_Compress,     "compress( source: string ) => string" },
	{ ZIP_Decompress,   "decompress( source: string ) => string" },
	{ NULL, NULL }
};

DAO_DLL int DaoZip_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	ns = DaoNamespace_GetNamespace( ns, "zip" );
	DaoNamespace_WrapFunctions( ns, zipMeths );
	return 0;
}
