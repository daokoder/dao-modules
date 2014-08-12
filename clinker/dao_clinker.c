
/* DaoClinker:
 * Running time wrapping of C functions loaded from dynamic linking library.
 * Copyright (C) 2008-2013, Limin Fu (daokoder@gmail.com).
 */

#include"string.h"
#include"daoType.h"
#include"daoValue.h"
#include"daoClass.h"
#include"daoObject.h"
#include"daoRoutine.h"
#include"daoNumtype.h"
#include"daoProcess.h"
#include"daoVmspace.h"
#include"daoNamespace.h"
#include"daoParser.h"
#include"daoPlatforms.h"
#include"daoGC.h"

#include"ffi.h"


const char *const ctype[] =
{ "uint8", "sint8", "uint16", "sint16", "uint32", "sint32", "uint64", "sint64" };

const char *const alias[] =
{ "uchar", "char", "ushort", "short", "uint", "int", "uint64", "sint64" };

typedef void* (*bare_func)();
typedef struct DaoFFI DaoFFI;

enum DaoFFITypes
{
	DAO_FFI_INT ,
	DAO_FFI_UINT8 ,
	DAO_FFI_SINT8 ,
	DAO_FFI_UINT16 ,
	DAO_FFI_SINT16 ,
	DAO_FFI_UINT32 ,
	DAO_FFI_SINT32 ,
	DAO_FFI_UINT64 ,
	DAO_FFI_SINT64
};
DaoType *daox_ffi_int_types[ DAO_FFI_SINT64 + 1 ] = { NULL };
DaoType *daox_ffi_stream_type = NULL;

struct DaoFFI
{
	DAO_CSTRUCT_COMMON;

	void      *fptr;
	ffi_cif    cif;
	ffi_type  *args[ DAO_MAX_PARAM + 1 ];
	ffi_type  *retype;
};
DaoType *daox_ffi_type = NULL;

DaoFFI* DaoFFI_New()
{
	DaoFFI *self = (DaoFFI*) dao_calloc( 1, sizeof(DaoFFI) );
	DaoCstruct_Init( (DaoCstruct*) self, daox_ffi_type );
	return self;
}
void DaoFFI_Delete( DaoFFI *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	// TODO:
	dao_free( self );
}

typedef union IntArgument IntArgument;
union IntArgument
{
	unsigned char   m_uint8;
	signed   char   m_sint8;
	unsigned short  m_uint16;
	signed   short  m_sint16;
	unsigned int    m_uint32;
	signed   int    m_sint32;
};

static void DaoCLoader_Execute( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoRoutine *func = proc->topFrame->routine;
	DaoType *routype, *tp, *itp, **nested;
	DaoArray *array;
	DaoCdata *cdata;
	DaoFFI *ffi;
	DString *str = NULL;
	complex16 com = { 0.0, 0.0 };
	IntArgument ints[DAO_MAX_PARAM];
	void *args[DAO_MAX_PARAM];
	char *bytes = NULL;
	double dummy;
	void *ret = & dummy;
	daoint ivalue;
	int i;
	if( func == NULL || func->routHost == NULL || func->routHost->aux == NULL ){
		return;
	}
	ffi = (DaoFFI*) DaoValue_CastCstruct( func->routHost->aux, daox_ffi_type );
	if( ffi == NULL ){
		return;
	}
	//printf( "ffiData = %p\n", func->ffiData );
	routype = func->routType;
	nested = routype->nested->items.pType;
	memset( args, 0, func->parCount * sizeof(void*) );
	for(i=0; i<func->parCount; i++){
		if( i >= routype->nested->size ) continue;
		tp = nested[i];
		//printf( "%i  %i  %s\n", i, tp->tid, tp->name->chars );
		if( tp->tid == DAO_PAR_NAMED || tp->tid == DAO_PAR_DEFAULT ) tp = (DaoType*) tp->aux;
		switch( tp->tid ){
		case DAO_INTEGER :
			args[i] = & ints[i];
			ivalue = p[i]->xInteger.value;
			if( tp == daox_ffi_int_types[ DAO_FFI_UINT8 ] ){
				ints[i].m_uint8 = ivalue;
			}else if( tp == daox_ffi_int_types[ DAO_FFI_SINT8 ] ){
				ints[i].m_sint8 = ivalue;
			}else if( tp == daox_ffi_int_types[ DAO_FFI_UINT16 ] ){
				ints[i].m_uint16 = ivalue;
			}else if( tp == daox_ffi_int_types[ DAO_FFI_SINT16 ] ){
				ints[i].m_sint16 = ivalue;
			}else if( tp == daox_ffi_int_types[ DAO_FFI_UINT32 ] ){
				ints[i].m_uint32 = ivalue;
			}else{
				ints[i].m_sint32 = ivalue;
			}
			//args[i] = & p[i]->xInteger.value; // only works with little-endian;
			break;
		case DAO_FLOAT :
			args[i] = & p[i]->xFloat.value;
			break;
		case DAO_DOUBLE :
			args[i] = & p[i]->xDouble.value;
			break;
		case DAO_COMPLEX :
			args[i] = & p[i]->xComplex.value;
			break;
		case DAO_STRING :
			args[i] = & p[i]->xString.value->chars;
			break;
		case DAO_ARRAY :
			itp = tp->nested->size ? tp->nested->items.pType[0] : NULL;
			array = (DaoArray*) p[i];
			if( itp == NULL ) break;
			switch( itp->tid ){
			case DAO_INTEGER :
				if( itp == daox_ffi_int_types[ DAO_FFI_UINT8 ] ){
					DaoArray_ToUByte( array );
				}else if( itp == daox_ffi_int_types[ DAO_FFI_SINT8 ] ){
					DaoArray_ToSByte( array );
				}else if( itp == daox_ffi_int_types[ DAO_FFI_UINT16 ] ){
					DaoArray_ToUShort( array );
				}else if( itp == daox_ffi_int_types[ DAO_FFI_SINT16 ] ){
					DaoArray_ToSShort( array );
				}else if( itp == daox_ffi_int_types[ DAO_FFI_UINT32 ] ){
					DaoArray_ToUInt( array );
				}else{
					DaoArray_ToSInt( array );
				}
				break;
			case DAO_FLOAT :
				DaoArray_ToFloat( array );
				break;
			case DAO_DOUBLE :
				DaoArray_ToDouble( array );
				break;
			case DAO_COMPLEX :
				break;
			default : break;
			}
			args[i] = & array->data.p;
			break;
		case DAO_CSTRUCT :
			args[i] = & p[i];
			if( DaoValue_CastCstruct( p[i], daox_ffi_stream_type ) ){
				args[i] = & p[i]->xStream.file;
			}
			break;
		case DAO_CDATA :
			args[i] = & p[i]->xCdata.data;
			break;
		default :
			args[i] = & p[i];
			break;
		}
	}
	tp = (DaoType*) routype->aux;
	if( tp ){
		switch( tp->tid ){
		case DAO_INTEGER :
			ret = (void*) DaoProcess_PutInteger( proc, 0 );
			break;
		case DAO_FLOAT :
			ret = (void*) DaoProcess_PutFloat( proc, 0.0 );
			break;
		case DAO_DOUBLE :
			ret = (void*) DaoProcess_PutDouble( proc, 0.0 );
			break;
		case DAO_COMPLEX :
			ret = (void*) DaoProcess_PutComplex( proc, com );
			break;
		case DAO_STRING :
			ret = (void*) & bytes;
			break;
		case DAO_CDATA :
			cdata = DaoProcess_PutCdata( proc, NULL, tp );
			ret = & cdata->data;
			break;
		default : break;
		}
	}
	ffi_call( & ffi->cif, ffi->fptr, ret, args );
	DaoProcess_PutChars( proc, bytes );
	for(i=0; i<func->parCount; i++){
		if( i >= routype->nested->size ) continue;
		tp = nested[i];
		if( tp->tid == DAO_PAR_NAMED || tp->tid == DAO_PAR_DEFAULT ) tp = (DaoType*) tp->aux;
		switch( tp->tid ){
		case DAO_STRING :
			p[i]->xString.value->size = strlen( p[i]->xString.value->chars );
			p[i]->xString.value->bufSize = p[i]->xString.value->size + 1;
			break;
		case DAO_ARRAY :
			itp = tp->nested->size ? tp->nested->items.pType[0] : NULL;
			array = (DaoArray*) p[i];
			if( itp == NULL ) break;
			switch( itp->tid ){
			case DAO_INTEGER :
				if( itp == daox_ffi_int_types[ DAO_FFI_UINT8 ] ){
					DaoArray_FromUByte( array );
				}else if( itp == daox_ffi_int_types[ DAO_FFI_SINT8 ] ){
					DaoArray_FromSByte( array );
				}else if( itp == daox_ffi_int_types[ DAO_FFI_UINT16 ] ){
					DaoArray_FromUShort( array );
				}else if( itp == daox_ffi_int_types[ DAO_FFI_SINT16 ] ){
					DaoArray_FromSShort( array );
				}else if( itp == daox_ffi_int_types[ DAO_FFI_UINT32 ] ){
					DaoArray_FromUInt( array );
				}else{
					DaoArray_FromSInt( array );
				}
				break;
			case DAO_FLOAT :
				DaoArray_FromFloat( array );
				break;
			case DAO_DOUBLE :
				DaoArray_FromDouble( array );
				break;
			case DAO_COMPLEX :
				break;
			default : break;
			}
			break;
		default : break;
		}
	}
}

static ffi_type *intps[] =
{
	& ffi_type_sint32,
	& ffi_type_uint8,
	& ffi_type_sint8,
	& ffi_type_uint16,
	& ffi_type_sint16,
	& ffi_type_uint32,
	& ffi_type_sint32,
	& ffi_type_uint64,
	& ffi_type_sint64
};
static ffi_type* ConvertType( DaoType *tp )
{
	int i;
	ffi_type *ffitype = & ffi_type_pointer;
	if( tp == NULL ) return ffitype;
	switch( tp->tid ){
	case DAO_INTEGER :
		ffitype = & ffi_type_sint32;
		for(i=DAO_FFI_INT; i<=DAO_FFI_SINT64; ++i){
			if( tp == daox_ffi_int_types[i] ){
				ffitype = intps[i];
				break;
			}
		}
		break;
	case DAO_FLOAT  : ffitype = & ffi_type_float; break;
	case DAO_DOUBLE : ffitype = & ffi_type_double; break;
	default : break;
	}
	return ffitype;
}
static void DaoCLoader_Load( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *str;
	DString *lib = p[0]->xString.value;
	DaoList *funames = (DaoList*) p[1];
	DaoList *tpnames = (DaoList*) p[2];
	DaoVmSpace *vms = proc->vmSpace;
	DaoNamespace *ns;
	DaoType *routype, *tp, **nested;
	DaoRoutine *func, *dummy;
	DaoValue *value;
	DaoParser *parser;
	DaoParser *defparser;
	DaoTypeBase *typer;
	DaoFFI *ffi;
	void *handle, *fptr;
	char *chs;
	int i, j, ok = 1;
	DString *path = DString_New();
	DString *file = DString_New();
	daoint pos;

	pos = lib->size;
	while( pos && (lib->chars[pos-1] == '_' || isalnum( lib->chars[pos-1] )) ) pos -= 1;
	if( pos && (lib->chars[pos-1] == '/' || lib->chars[pos-1] == '\\') ){
		DString_SubString( lib, path, 0, pos );
		DString_SubString( lib, file, pos, lib->size - pos );
	}else{
		DString_Assign( file, lib );
	}

	pos = DString_FindChars( file, DAO_DLL_PREFIX, 0 );
	if( pos != 0 ) DString_InsertChars( file, DAO_DLL_PREFIX, 0, 0, strlen(DAO_DLL_PREFIX) );
	pos = DString_FindChars( file, DAO_DLL_SUFFIX, 0 );
	if( pos != (file->size - strlen(DAO_DLL_SUFFIX)) ) DString_AppendChars( file, DAO_DLL_SUFFIX );
	DString_Assign( lib, file );
	Dao_MakePath( path, lib );
	Dao_MakePath( proc->activeNamespace->path, lib );

	handle = Dao_OpenDLL( lib->chars );
	if( handle == NULL ){
		DaoProcess_PutNone( proc );
		return;
	}
	ns = DaoNamespace_New( vms, lib->chars );
	DaoProcess_PutValue( proc, (DaoValue*) ns );

	for(i=0; i<DAO_FFI_SINT64; i++){
		DString mbs = DString_WrapChars( alias[i] );
		DaoNamespace_AddType( ns, daox_ffi_int_types[i]->name, daox_ffi_int_types[i] );
		DaoNamespace_AddTypeConstant( ns, daox_ffi_int_types[i]->name, daox_ffi_int_types[i] );
		if( i < DAO_FFI_SINT32 ){
			DaoNamespace_AddType( ns, & mbs, daox_ffi_int_types[i] );
			DaoNamespace_AddTypeConstant( ns, & mbs, daox_ffi_int_types[i] );
		}
	}

	dummy = DaoRoutine_New( ns, NULL, 0 );
	GC_IncRC( dummy );
	parser = DaoVmSpace_AcquireParser( vms );
	parser->vmSpace = vms;
	parser->nameSpace = ns;
	parser->defParser = defparser = DaoVmSpace_AcquireParser( vms );
	defparser->vmSpace = vms;
	defparser->nameSpace = ns;
	defparser->routine = dummy;
	for(i=0; i<tpnames->value->size; i++){
		str = DaoValue_TryGetString( DaoList_GetItem( tpnames, i ) );
		value = DaoNamespace_GetData( ns, str );
		if( value != NULL ) continue; /* warn XXX */
		typer = calloc( 1, sizeof(DaoTypeBase) );
		typer->name = str->chars;
		DaoNamespace_WrapType( ns, typer, 1 );
	}
	for(i=0; i<funames->value->size; i++){
		str = DaoValue_TryGetString( DaoList_GetItem( funames, i ) );
		if( str->size == 0 ) continue;
		func = DaoNamespace_MakeFunction( ns, str->chars, parser );
		if( func == NULL ) continue;

		routype = func->routType;
		nested = routype->nested->items.pType;
		func->pFunc = DaoCLoader_Execute;
		fptr = Dao_GetSymbolAddress( handle, func->routName->chars );
		if( fptr == NULL ){
			ok = 0;
			printf( "WARNING: symbol %s is not found!\n", func->routName->chars );
			continue;
		}
		ffi = DaoFFI_New();
		ffi->fptr = fptr;
		for(j=0; j<func->parCount; j++){
			tp = nested[j];
			if( tp->tid ==DAO_PAR_NAMED || tp->tid ==DAO_PAR_DEFAULT ) tp = (DaoType*) tp->aux;
			ffi->args[j] = ConvertType( tp );
		}
		ffi->retype = ConvertType( (DaoType*) routype->aux );
		if( ffi_prep_cif( & ffi->cif, FFI_DEFAULT_ABI, func->parCount, ffi->retype, ffi->args ) != FFI_OK ){
			DaoFFI_Delete( ffi );
			continue;
		}
		tp = DaoType_New( "FFI_Function", DAO_NONE, (DaoValue*) ffi, NULL );
		GC_Assign( & func->routHost, tp );
	}
	GC_DecRC( dummy );
	DaoVmSpace_ReleaseParser( vms, parser );
	DaoVmSpace_ReleaseParser( vms, defparser );
	if( ok ==0 ) DaoProcess_RaiseError( proc, NULL, "loading failed" );
}


DaoTypeBase DaoFFI_Typer =
{ "FFI", NULL, NULL, NULL, {0}, {0}, (FuncPtrDel) DaoFFI_Delete, NULL };

DAO_DLL int DaoClinker_OnLoad( DaoVmSpace *vms, DaoNamespace *ns )
{
	DaoNamespace *io = DaoVmSpace_GetNamespace( vms, "io" );
	int i;

	daox_ffi_stream_type = DaoNamespace_FindTypeChars( io, "stream" );
	daox_ffi_type = DaoNamespace_WrapType( ns, & DaoFFI_Typer, 0 );
	for(i=0; i<DAO_FFI_SINT64; i++){
		DString mbs = DString_WrapChars( alias[i] );
		daox_ffi_int_types[i] = DaoNamespace_TypeDefine( ns, "int", ctype[i] );
		if( i < DAO_FFI_SINT32 ){
			DaoNamespace_AddType( ns, & mbs, daox_ffi_int_types[i] );
			DaoNamespace_AddTypeConstant( ns, & mbs, daox_ffi_int_types[i] );
		}
	}

	DaoNamespace_WrapFunction( ns, DaoCLoader_Load,
			"link( lib : string, funcs : list<string>, ctypes : list<string> ={} ) => any" );
	return 0;
}
