
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


const char *const ctype[] = { "uint8","sint8","uint16","sint16","uint32","sint32" };
const char *const alias[] = { "uchar","char","ushort","short","uint","int" };

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
	DAO_FFI_SINT32
};
DaoType *daox_ffi_int_types[ DAO_FFI_SINT32 + 1 ] = { NULL };
DaoType *daox_ffi_wstring_type = NULL;

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


static void DaoCLoader_Execute( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoRoutine *func = proc->topFrame->routine;
	DaoType *routype, *tp, *itp, **nested;
	DaoArray *array;
	DaoCdata *cdata;
	DaoFFI *ffi;
	DString *str = NULL;
	complex16 com = { 0.0, 0.0 };
	char *chs;
	void **args;
	double dummy;
	void *ret = & dummy;
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
	args = (void*)alloca( func->parCount * sizeof(void*) );
	memset( args, 0, func->parCount * sizeof(void*) );
	for(i=0; i<func->parCount; i++){
		if( i >= routype->nested->size ) continue;
		tp = nested[i];
		if( tp->tid == DAO_PAR_NAMED || tp->tid == DAO_PAR_DEFAULT ) tp = (DaoType*) tp->aux;
		//printf( "%i  %i  %s\n", i, tp->tid, tp->name->mbs );
		switch( tp->tid ){
		case DAO_INTEGER :
			args[i] = & p[i]->xInteger.value;
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
			if( p[i]->xString.data->wcs == NULL && tp == daox_ffi_wstring_type ){
				DString_ToWCS( p[i]->xString.data );
			}else if( p[i]->xString.data->mbs == NULL && tp != daox_ffi_wstring_type ){
				DString_ToMBS( p[i]->xString.data );
			}
			if( p[i]->xString.data->mbs ){
				args[i] = & p[i]->xString.data->mbs;
			}else{
				args[i] = & p[i]->xString.data->wcs;
			}
			break;
		case DAO_ARRAY :
			itp = tp->nested->size ? tp->nested->items.pType[0] : NULL;
			array = (DaoArray*) p[i];
			args[i] = & array->data.p;
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
			break;
		case DAO_CDATA :
			args[i] = & p[i]->xCdata.data;
			break;
		default :
			args[i] = & p[i];
			break;
		}
	}
	//printf( "debug 1\n" );
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
			str = DaoProcess_PutMBString( proc, "" );
			DString_Detach( str, str->size );
			ret = (void*) & str->mbs;
			break;
		case DAO_CDATA :
			cdata = DaoProcess_PutCdata( proc, NULL, tp );
			ret = & cdata->data;
			break;
		default : break;
		}
	}
	ffi_call( & ffi->cif, ffi->fptr, ret, args );
	if( tp ){
		switch( tp->tid ){
		case DAO_STRING :
			if( ret ){
				str->size = strlen( str->mbs );
				str->bufSize = str->size + 1;
			}
			break;
		default : break;
		}
	}
	for(i=0; i<func->parCount; i++){
		if( i >= routype->nested->size ) continue;
		tp = nested[i];
		switch( tp->tid ){
		case DAO_STRING :
			if( p[i]->xString.data->mbs ){
				p[i]->xString.data->size = strlen( p[i]->xString.data->mbs );
				p[i]->xString.data->bufSize = p[i]->xString.data->size + 1;
			}else{
				p[i]->xString.data->size = wcslen( p[i]->xString.data->wcs );
				p[i]->xString.data->bufSize = p[i]->xString.data->size + 1;
			}
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
	& ffi_type_sint32
};
static ffi_type* ConvertType( DaoType *tp )
{
	int i;
	ffi_type *ffitype = & ffi_type_pointer;
	if( tp == NULL ) return ffitype;
	switch( tp->tid ){
	case DAO_INTEGER :
		ffitype = & ffi_type_sint32;
		for(i=DAO_FFI_INT; i<=DAO_FFI_SINT32; ++i){
			if( tp == daox_ffi_int_types[i] ){
				ffitype = intps[i];
				break;
			}
		}
		break;
	case DAO_FLOAT :
		ffitype = & ffi_type_float;
		break;
	case DAO_DOUBLE :
		ffitype = & ffi_type_double;
		break;
	default : break;
	}
	return ffitype;
}
static void DaoCLoader_Load( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *str;
	DString *lib = p[0]->xString.data;
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
	int i, j;
	int ok = 1;
	DString_ToMBS( lib );
	DaoVmSpace_CompleteModuleName( vms, lib );
	handle = Dao_OpenDLL( lib->mbs );
	if( handle == NULL ){
		DaoProcess_PutNone( proc );
		return;
	}
	ns = DaoNamespace_New( vms, lib->mbs );
	DaoProcess_PutValue( proc, (DaoValue*) ns );
	for(i=0; i<DAO_FFI_SINT32; i++){
		DString mbs = DString_WrapMBS( alias[i] );
		DaoNamespace_AddType( ns, daox_ffi_int_types[i]->name, daox_ffi_int_types[i] );
		if( i != DAO_FFI_SINT32 ) DaoNamespace_AddType( ns, & mbs, daox_ffi_int_types[i] );
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
	for(i=0; i<tpnames->items.size; i++){
		str = DaoValue_TryGetString( DaoList_GetItem( tpnames, i ) );
		DString_ToMBS( str );
		value = DaoNamespace_GetData( ns, str );
		if( value != NULL ) continue; /* warn XXX */
		typer = calloc( 1, sizeof(DaoTypeBase) );
		typer->name = str->mbs;
		DaoNamespace_WrapType( ns, typer, 1 );
	}
	for(i=0; i<funames->items.size; i++){
		str = DaoValue_TryGetString( DaoList_GetItem( funames, i ) );
		DString_ToMBS( str );
		if( str->size == 0 ) continue;
		func = DaoNamespace_MakeFunction( ns, str->mbs, parser );
		if( func == NULL ) continue;

		routype = func->routType;
		nested = routype->nested->items.pType;
		func->pFunc = DaoCLoader_Execute;
		fptr = Dao_GetSymbolAddress( handle, func->routName->mbs );
		if( fptr == NULL ){
			ok = 0;
			printf( "WARNING: symbol %s is not found!\n", func->routName->mbs );
			continue;
		}
		ffi = DaoFFI_New();
		ffi->fptr = fptr;
		for(j=0; j<func->parCount; j++){
			tp = nested[j];
			if( tp->tid ==DAO_PAR_NAMED || tp->tid ==DAO_PAR_DEFAULT ) tp = (DaoType*) tp;
			ffi->args[j] = ConvertType( tp );
		}
		ffi->retype = ConvertType( (DaoType*) routype->aux );
		if( ffi_prep_cif( & ffi->cif, FFI_DEFAULT_ABI, func->parCount, ffi->retype, ffi->args ) != FFI_OK ){
			DaoFFI_Delete( ffi );
			continue;
		}
		tp = DaoType_New( "FFI_Function", DAO_NONE, (DaoValue*) ffi, NULL );
		GC_ShiftRC( tp, func->routHost );
		func->routHost = tp;
	}
	GC_DecRC( dummy );
	DaoVmSpace_ReleaseParser( vms, parser );
	DaoVmSpace_ReleaseParser( vms, defparser );
	if( ok ==0 ) DaoProcess_RaiseException( proc, DAO_ERROR, "loading failed" );
}


DaoTypeBase DaoFFI_Typer =
{ "FFI", NULL, NULL, NULL, {0}, {0}, (FuncPtrDel) DaoFFI_Delete, NULL };

int DaoOnLoad( DaoVmSpace *vms, DaoNamespace *ns )
{
	int i;

	daox_ffi_wstring_type = DaoNamespace_TypeDefine( ns, "string", "wstring" );
	daox_ffi_type = DaoNamespace_WrapType( ns, & DaoFFI_Typer, 0 );
	for(i=0; i<DAO_FFI_SINT32; i++){
		DString mbs = DString_WrapMBS( alias[i] );
		daox_ffi_int_types[i] = DaoNamespace_TypeDefine( ns, "int", ctype[i] );
		if( i != DAO_FFI_SINT32 ) DaoNamespace_AddType( ns, & mbs, daox_ffi_int_types[i] );
	}

	DaoNamespace_WrapFunction( ns, DaoCLoader_Load,
			"link( lib : string, funcs : list<string>, ctypes : list<string> ={} ) => any" );
	return 0;
}
