#ifndef __DAO_CBLAS_H__
#define __DAO_CBLAS_H__
#include<stdlib.h>
#include<assert.h>
#include<string.h>
#include<dao.h>


#ifdef __cplusplus
extern "C"{
#endif

#include<modules/auxlib/dao_aux.h>
#include<modules/stream/dao_stream.h>
#include<daoList.h>


#ifdef __cplusplus
}
#endif

#include"cblas.h"


#ifndef DAO_CBLAS_STATIC
#ifndef DAO_CBLAS_DLL
#define DAO_CBLAS_DLL DAO_DLL_EXPORT
#endif
#else
#define DAO_CBLAS_DLL
#endif

#ifdef WIN32
#define DAO_CBLAS_DLLT DAO_CBLAS_DLL
#else
#define DAO_CBLAS_DLLT
#endif


#ifdef __cplusplus
extern "C"{
#endif


#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C"{
#endif

DAO_CBLAS_DLL int DaoCBLAS_OnLoad( DaoVmSpace *vms, DaoNamespace *ns );


#ifdef __cplusplus
}
#endif

#endif
