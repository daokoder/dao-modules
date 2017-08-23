#include"dao_cblas.h"
static DaoRoutine* Dao_Get_Object_Method( DaoCdata *cd, DaoObject **obj, const char *name )
{
  DaoRoutine *meth;
  if( cd == NULL ) return NULL;
  *obj = DaoCdata_GetObject( cd );
  if( *obj == NULL ) return NULL;
  meth = DaoObject_GetMethod( *obj, name );
  if( meth == NULL ) return NULL;
  if( DaoRoutine_IsWrapper( meth ) ) return NULL; /*do not call C function*/
  return meth;
}

static DaoVmSpace* Dao_Get_Object_VmSpace( DaoObject *obj )
{
  DaoClass *klass = obj != NULL ? DaoObject_GetClass( obj ) : NULL;
  DaoNamespace *ns = klass != NULL ? DaoClass_GetNamespace( klass ) : NULL;
  return ns != NULL ? DaoNamespace_GetVmSpace( ns ) : NULL;
}

