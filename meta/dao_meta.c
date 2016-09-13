/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2014, Limin Fu
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

#include<stdlib.h>
#include<string.h>
#include"daoString.h"
#include"daoValue.h"
#include"daoParser.h"
#include"daoNamespace.h"
#include"daoGC.h"


#if 0

int DaoClass_CopyField( DaoClass *self, DaoClass *other, DMap *deftypes )
{
	DaoNamespace *ns = other->initRoutine->nameSpace;
	DaoType *tp;
	DList *offsets = DList_New(0);
	DList *routines = DList_New(0);
	DNode *it;
	int i, k, st, up, id;

	for(i=0; i<other->superClass->size; i++){
		DaoValue *sup = other->superClass->items.pValue[i];
		if( sup->type == DAO_CLASS && sup->xClass.typeHolders ){
			tp = DaoType_DefineTypes( sup->xClass.objType, ns, deftypes );
			DList_Append( self->superClass, tp->aux );
		}else if( sup->type == DAO_CTYPE && sup->xCtype.ctype->typer->core->kernel->sptree ){
			tp = DaoType_DefineTypes( sup->xCtype.ctype, ns, deftypes );
			DList_Append( self->superClass, tp->aux );
		}else{
			DList_Append( self->superClass, sup );
		}
	}

	DaoRoutine_CopyFields( self->initRoutine, other->initRoutine, 1, 1 );
	for(it=DMap_First(other->lookupTable);it;it=DMap_Next(other->lookupTable,it)){
		st = LOOKUP_ST( it->value.pInt );
		up = LOOKUP_UP( it->value.pInt );
		id = LOOKUP_ID( it->value.pInt );
		if( up ==0 ){
			if( st == DAO_CLASS_CONSTANT && id <self->constants->size ) continue;
			if( st == DAO_CLASS_VARIABLE && id <self->variables->size ) continue;
			if( st == DAO_OBJECT_VARIABLE && id <self->instvars->size ) continue;
		}
		DMap_Insert( self->lookupTable, it->key.pVoid, it->value.pVoid );
	}
	for(i=self->objDataName->size; i<other->objDataName->size; i++)
		DList_Append( self->objDataName, other->objDataName->items.pString[i] );
	for(i=self->cstDataName->size; i<other->cstDataName->size; i++)
		DList_Append( self->cstDataName, other->cstDataName->items.pString[i] );
	for(i=self->glbDataName->size; i<other->glbDataName->size; i++)
		DList_Append( self->glbDataName, other->glbDataName->items.pString[i] );
	for(i=self->variables->size; i<other->variables->size; i++){
		DaoVariable *var = other->variables->items.pVar[i];
		var = DaoVariable_New( var->value, DaoType_DefineTypes( var->dtype, ns, deftypes ) );
		DList_Append( self->variables, var );
	}
	for(i=self->instvars->size; i<other->instvars->size; i++){
		DaoVariable *var = other->instvars->items.pVar[i];
		var = DaoVariable_New( var->value, DaoType_DefineTypes( var->dtype, ns, deftypes ) );
		DList_Append( self->instvars, var );
		/* TODO fail checking */
	}
	for(i=self->constants->size; i<other->constants->size; i++){
		DaoValue *value = other->constants->items.pConst[i]->value;
		DaoRoutine *rout = & value->xRoutine;
		if( value->type != DAO_ROUTINE || rout->routHost != other->objType ){
			DList_Append( self->constants, DaoConstant_New( value ) );
			DaoValue_Update( & self->constants->items.pConst[i]->value, ns, deftypes );
			continue;
		}
		if( rout->overloads == NULL ){
			DString *name = rout->routName;
			rout = DaoRoutine_Copy( rout, 1, 1 );
			value = (DaoValue*) rout;
			k = DaoRoutine_Finalize( rout, self->objType, deftypes );
#if 0
			printf( "%i %p:  %s  %s\n", i, rout, rout->routName->chars, rout->routType->name->chars );
#endif
			if( rout->attribs & DAO_ROUT_INITOR ){
				DRoutines_Add( self->initRoutines->overloads, rout );
			}else if( (it = DMap_Find( other->lookupTable, name )) ){
				st = LOOKUP_ST( it->value.pInt );
				up = LOOKUP_UP( it->value.pInt );
				id = LOOKUP_ID( it->value.pInt );
				if( st == DAO_CLASS_CONSTANT && up ==0 && id < i ){
					DaoValue *v = self->constants->items.pConst[id]->value;
					if( v->type == DAO_ROUTINE && v->xRoutine.overloads )
						DRoutines_Add( v->xRoutine.overloads, rout );
				}
			}
			DList_Append( self->constants, DaoConstant_New( value ) );
			DList_Append( routines, rout );
			if( k == 0 ) goto Failed;
			continue;
		}else{
			/* No need to added the overloaded routines now; */
			/* Each of them has an entry in constants, and will be handled later: */
			DaoRoutine *routs = DaoRoutines_New( ns, self->objType, NULL );
			DList_Append( self->constants, DaoConstant_New( (DaoValue*) routs ) );
			continue;
		}
		DList_Append( self->constants, DaoConstant_New( value ) );
		DaoValue_Update( & self->constants->items.pConst[i]->value, ns, deftypes );
	}
	for(i=0; i<routines->size; i++){
		if( DaoRoutine_DoTypeInference( routines->items.pRoutine[i], 0 ) == 0 ) goto Failed;
	}
	DList_Delete( offsets );
	DList_Delete( routines );
	DaoRoutine_Finalize( self->initRoutine, self->objType, deftypes );
	return DaoRoutine_DoTypeInference( self->initRoutine, 0 );
Failed:
	DList_Delete( offsets );
	DList_Delete( routines );
	return 0;
}
#endif
#if 0
DaoClass* DaoClass_Instantiate( DaoClass *self, DList *types )
{
	DaoClass *klass = NULL;
	DaoType *type;
	DString *name;
	DNode *node;
	DMap *deftypes;
	daoint lt = DString_FindChar( self->className, '<', 0 );
	daoint i, holders = 0;
	if( self->typeHolders == NULL || self->typeHolders->size ==0 ) return self;
	while( types->size < self->typeHolders->size ){
		type = self->typeDefaults->items.pType[ types->size ];
		if( type == NULL ) type = self->typeHolders->items.pType[ types->size ];
		DList_Append( types, type );
	}
	name = DString_New();
	DString_Append( name, self->className );
	if( lt != MAXSIZE ) DString_Erase( name, lt, MAXSIZE );
	DString_AppendChar( name, '<' );
	for(i=0; i<types->size; i++){
		type = types->items.pType[i];
		holders += type->tid == DAO_THT;
		if( i ) DString_AppendChar( name, ',' );
		DString_Append( name, type->name );
	}
	DString_AppendChar( name, '>' );
	while( self->templateClass ) self = self->templateClass;
	node = DMap_Find( self->instanceClasses, name );
	if( node ){
		klass = node->value.pClass;
	}else{
		deftypes = DMap_New(0,0);
		klass = DaoClass_New();
		if( holders ) klass->templateClass = self;
		DMap_Insert( self->instanceClasses, name, klass );
		DaoClass_AddReference( self, klass ); /* No need for cleanup of klass; */
		DaoClass_SetName( klass, name, self->initRoutine->nameSpace );
		for(i=0; i<types->size; i++){
			type = types->items.pType[i];
			if( DaoType_MatchTo( type, self->typeHolders->items.pType[i], deftypes ) ==0 ){
				DString_Delete( name );
				return NULL;
			}
			MAP_Insert( deftypes, self->typeHolders->items.pVoid[i], type );
		}
		klass->objType->args = DList_New(DAO_DATA_VALUE);
		DList_Assign( klass->objType->args, types );
		if( DaoClass_CopyField( klass, self, deftypes ) == 0 ){
			DString_Delete( name );
			return NULL;
		}
		DaoClass_DeriveClassData( klass );
		DaoClass_DeriveObjectData( klass );
		DaoClass_ResetAttributes( klass );
		DMap_Delete( deftypes );
		if( holders ){
			klass->typeHolders = DList_New(0);
			klass->typeDefaults = DList_New(0);
			klass->instanceClasses = DMap_New(DAO_DATA_STRING,0);
			DMap_Insert( klass->instanceClasses, klass->className, klass );
			for(i=0; i<types->size; i++){
				DList_Append( klass->typeHolders, types->items.pType[i] );
				DList_Append( klass->typeDefaults, NULL );
			}
			for(i=0; i<klass->typeHolders->size; i++){
				DaoClass_AddReference( klass, klass->typeHolders->items.pType[i] );
				DaoClass_AddReference( klass, klass->typeDefaults->items.pType[i] );
			}
		}
	}
	DString_Delete( name );
	return klass;
}
#endif
#if 0


/* storage enum<const,global,var> */
static int storages[3] = { DAO_CLASS_CONSTANT, DAO_CLASS_VARIABLE, DAO_OBJECT_VARIABLE };

/* access enum<private,protected,public> */
static int permissions[3] = { DAO_PERM_PRIVATE, DAO_PERM_PROTECTED, DAO_PERM_PUBLIC };

/* a = class( name, parents, fields, methods ){ proto_class_body }
 * (1) parents: optional, list<class> or map<string,class>
 * (2) fields: optional, tuple<name:string,value:any,storage:enum<>,access:enum<>>
 * (3) methods: optional, tuple<name:string,method:routine,access:enum<>>
 * (4) default storage: $var, default access: $public.
 */
void DaoProcess_MakeClass( DaoProcess *self, DaoVmCode *vmc )
{
	DaoType *tp;
	DaoValue **values = self->activeValues;
	DaoRoutine *routine = self->activeRoutine;
	DaoNamespace *ns = self->activeNamespace;
	DaoNamespace *ns2 = self->activeNamespace;
	DaoTuple *tuple = (DaoTuple*) values[vmc->a];
	DaoClass *klass = DaoClass_New();
	DaoClass *proto = NULL;
	DaoList *parents = NULL;
	DaoMap *parents2 = NULL;
	DaoList *fields = NULL;
	DaoList *methods = NULL;
	DString *name = NULL;
	DaoValue **data = tuple->values;
	DMap *keys = tuple->ctype->mapNames;
	DMap *deftypes = DMap_New(0,0);
	DMap *pm_map = DMap_New(DAO_DATA_STRING,0);
	DMap *st_map = DMap_New(DAO_DATA_STRING,0);
	DList *routines = DList_New(0);
	DNode *it, *node;
	DaoEnum pmEnum = {DAO_ENUM,0,0,0,0,0,0,NULL};
	DaoEnum stEnum = {DAO_ENUM,0,0,0,0,0,0,NULL};
	int iclass = values[vmc->a+1]->xInteger.value;
	int i, n, st, pm, up, id, size;
	char buf[50];

	pmEnum.etype = dao_access_enum;
	stEnum.etype = dao_storage_enum;

	DaoProcess_SetValue( self, vmc->c, (DaoValue*) klass );
	//printf( "%s\n", tuple->ctype->name->chars );
	if( iclass && routine->routConsts->value->items.pValue[iclass-1]->type == DAO_CLASS ){
		proto = & routine->routConsts->value->items.pValue[iclass-1]->xClass;
		ns2 = proto->initRoutine->nameSpace;
	}

	/* extract parameters */
	if( tuple->size && data[0]->type == DAO_STRING ) name = data[0]->xString.value;
	if( parents ==NULL && parents2 == NULL && tuple->size >1 ){
		if( data[1]->type == DAO_LIST ) parents = & data[1]->xList;
		if( data[1]->type == DAO_MAP ) parents2 = & data[1]->xMap;
	}
	if( fields ==NULL && tuple->size >2 && data[2]->type == DAO_LIST ) fields = & data[2]->xList;
	if( methods ==NULL && tuple->size >3 && data[3]->type == DAO_LIST ) methods = & data[3]->xList;

	if( name == NULL || name->size ==0 ){
		sprintf( buf, "AnonymousClass%p", klass );
		DString_SetChars( klass->className, buf );
		DaoClass_SetName( klass, klass->className, ns2 );
	}else{
		DaoClass_SetName( klass, name, ns2 );
	}
	for(i=0; i<routine->parCount; i++){
		DaoType *type = routine->routType->args->items.pType[i];
		DaoValue *value = self->activeValues[i];
		/* type<@T<int|float>> kind of types may be specialized to type<float>
		 * the type holder is only available from the original routine parameters: */
		if( routine->original ) type = routine->original->routType->args->items.pType[i];
		if( type->tid == DAO_PAR_NAMED || type->tid == DAO_PAR_DEFAULT ) type = (DaoType*) type->aux;
		if( type->tid != DAO_TYPE ) continue;
		type = type->args->items.pType[0];
		if( type->tid == DAO_VARIANT && type->aux ) type = (DaoType*) type->aux;
		if( type->tid == DAO_THT && value->type == DAO_TYPE ){
			MAP_Insert( deftypes, type, value );
		}
	}
	tp = DaoNamespace_MakeType( ns, "@class", DAO_THT, NULL,NULL,0 );
	if( tp ) MAP_Insert( deftypes, tp, klass->objType );
	DaoProcess_MapTypes( self, deftypes );

	/* copy data from the proto class */
	if( proto ) DaoClass_CopyField( klass, proto, deftypes );

	/* update class members with running time data */
	for(i=2; i<=vmc->b; i+=3){
		DaoValue *value;
		st = values[vmc->a+i+1]->xInteger.value;
		id = values[vmc->a+i+2]->xInteger.value;
		value = self->activeValues[vmc->a+i];
		if( st == DAO_CLASS_CONSTANT ){
			DaoRoutine *newRout = NULL;
			DaoConstant *dest2 = klass->constants->items.pConst[id];
			DaoValue *dest = dest2->value;
			if( value->type == DAO_ROUTINE && value->xRoutine.routHost == proto->objType ){
				newRout = & value->xRoutine;
				if( DaoRoutine_Finalize( newRout, klass->objType, deftypes ) == 0){
					DaoProcess_RaiseError( self, NULL, "method creation failed" );
					continue;
				}
				DList_Append( routines, newRout );
				if( strcmp( newRout->routName->chars, "@class" ) ==0 ){
					node = DMap_Find( proto->lookupTable, newRout->routName );
					DString_Assign( newRout->routName, klass->className );
					st = LOOKUP_ST( node->value.pInt );
					up = LOOKUP_UP( node->value.pInt );
					if( st == DAO_CLASS_CONSTANT && up ==0 ){
						id = LOOKUP_ID( node->value.pInt );
						dest2 = klass->constants->items.pConst[id];
					}
					DRoutines_Add( klass->initRoutines->overloads, newRout );
				}
			}
			dest = dest2->value;
			if( dest->type == DAO_ROUTINE ){
				DaoRoutine *rout = & dest->xRoutine;
				if( rout->routHost != klass->objType ) DaoValue_Clear( & dest2->value );
			}
			if( dest->type == DAO_ROUTINE && dest->xRoutine.overloads ){
				DRoutines_Add( dest->xRoutine.overloads, newRout );
			}else{
				DaoValue_Copy( value, & dest2->value );
			}
		}else if( st == DAO_CLASS_VARIABLE ){
			DaoVariable *var = klass->variables->items.pVar[id];
			DaoValue_Move( value, & var->value, var->dtype );
		}else if( st == DAO_OBJECT_VARIABLE ){
			DaoVariable *var = klass->instvars->items.pVar[id];
			DaoValue_Move( value, & var->value, var->dtype );
		}
	}

	/* add parents from parameters */
	if( parents ){
		for(i=0,n=parents->value->size; i<n; i++){
			DaoClass_AddSuperClass( klass, parents->value->items.pValue[i] );
		}
	}else if( parents2 ){
		for(it=DMap_First(parents2->items);it;it=DMap_Next(parents2->items,it)){
			int type = it->value.pValue->type;
			if( it->key.pValue->type == DAO_STRING && (type == DAO_CLASS || type == DAO_CTYPE) ){
				DaoClass_AddSuperClass( klass, it->value.pValue );
			}//XXX error handling
		}
	}
	DaoClass_DeriveClassData( klass );
	if( fields ){ /* add fields from parameters */
		for(i=0,n=fields->value->size; i<n; i++){
			DaoValue *fieldv = fields->value->items.pValue[i];
			DaoType *type = NULL;
			DaoValue *value = NULL;

			if( DaoType_MatchValue( dao_dynclass_field, fieldv, NULL ) == 0) continue;//XXX
			data = fieldv->xTuple.items;
			size = fieldv->xTuple.size;
			st = DAO_OBJECT_VARIABLE;
			pm = DAO_PERM_PUBLIC;

			name = NULL;
			if( size && data[0]->type == DAO_STRING ) name = data[0]->xString.value;
			if( size > 1 && data[1]->type ){
				value = data[1];
				type = fieldv->xTuple.ctype->args->items.pType[1];
			}
			if( name == NULL || value == NULL ) continue;
			if( MAP_Find( klass->lookupTable, name ) ) continue;

			if( size > 2 && data[2]->type == DAO_ENUM ){
				if( DaoEnum_SetValue( & stEnum, & data[2]->xEnum, NULL ) ==0) goto InvalidField;
				st = storages[ stEnum.value ];
			}
			if( size > 3 && data[3]->type == DAO_ENUM ){
				if( DaoEnum_SetValue( & pmEnum, & data[3]->xEnum, NULL ) ==0) goto InvalidField;
				pm = permissions[ pmEnum.value ];
			}
			/* printf( "%s %i %i\n", name->chars, st, pm ); */
			switch( st ){
			case DAO_OBJECT_VARIABLE: DaoClass_AddObjectVar( klass, name, value, type, pm ); break;
			case DAO_CLASS_VARIABLE : DaoClass_AddGlobalVar( klass, name, value, type, pm ); break;
			case DAO_CLASS_CONSTANT : DaoClass_AddConst( klass, name, value, pm ); break;
			default : break;
			}
			continue;
InvalidField:
			DaoProcess_RaiseError( self, "Param", "" );
		}
	}
	if( methods ){ /* add methods from parameters */
		for(i=0,n=methods->value->size; i<n; i++){
			DaoValue *methodv = methods->value->items.pValue[i];
			DaoRoutine *newRout;
			DaoValue *method = NULL;
			DaoValue *dest;

			if( DaoType_MatchValue( dao_dynclass_method, methodv, NULL ) == 0) continue;//XXX
			data = methodv->xTuple.items;
			size = methodv->xTuple.size;
			pm = DAO_PERM_PUBLIC;

			name = NULL;
			if( size && data[0]->type == DAO_STRING ) name = data[0]->xString.value;
			if( size > 1 && data[1]->type == DAO_ROUTINE ) method = data[1];
			if( name == NULL || method == NULL ) continue;
			if( size > 2 && data[2]->type == DAO_ENUM ){
				if( DaoEnum_SetValue( & pmEnum, & data[2]->xEnum, NULL ) ==0) goto InvalidMethod;
				pm = permissions[ pmEnum.value ];
			}

			newRout = & method->xRoutine;
			if( ROUT_HOST_TID( newRout ) !=0 ) continue;
			if( DaoRoutine_Finalize( newRout, klass->objType, deftypes ) == 0){
				DaoProcess_RaiseError( self, NULL, "method creation failed" );
				continue; // XXX
			}
			DList_Append( routines, newRout );
			DString_Assign( newRout->routName, name );
			if( DString_EQ( newRout->routName, klass->className ) ){
				DRoutines_Add( klass->initRoutines->overloads, newRout );
			}

			node = DMap_Find( proto->lookupTable, name );
			if( node == NULL ){
				DaoClass_AddConst( klass, name, method, pm );
				continue;
			}
			if( LOOKUP_UP( node->value.pInt ) ) continue;
			if( LOOKUP_ST( node->value.pInt ) != DAO_CLASS_CONSTANT ) continue;
			id = LOOKUP_ID( node->value.pInt );
			dest = klass->constants->items.pConst[id]->value;
			if( dest->type == DAO_ROUTINE && dest->xRoutine.overloads ){
				DRoutines_Add( dest->xRoutine.overloads, newRout );
			}
			continue;
InvalidMethod:
			DaoProcess_RaiseError( self, "Param", "" );
		}
	}
	for(i=0,n=routines->size; i<n; i++){
		if( DaoRoutine_DoTypeInference( routines->items.pRoutine[i], 0 ) == 0 ){
			DaoProcess_RaiseError( self, NULL, "method creation failed" );
			// XXX
		}
	}
	DaoClass_DeriveObjectData( klass );
	DaoClass_ResetAttributes( klass );
	DList_Delete( routines );
	DMap_Delete( deftypes );
	DMap_Delete( pm_map );
	DMap_Delete( st_map );
}
#endif



static void META_Name( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *str = DaoProcess_PutChars( proc, "" );
	switch( p[0]->type ){
	case DAO_ROUTINE :
		DString_Assign( str, p[0]->xRoutine.routName );
		break;
	case DAO_CLASS :
		DString_Assign( str, p[0]->xClass.className );
		break;
	case DAO_INTERFACE :
		DString_Assign( str, p[0]->xInterface.abtype->name );
		break;
	case DAO_NAMESPACE :
		DString_Assign( str, p[0]->xNamespace.name );
		break;
	case DAO_TYPE :
		DString_Assign( str, p[0]->xType.name );
		break;
	default : break;
	}
}
static void META_NS( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoNamespace *res = proc->activeNamespace;
	if( p[0]->type == DAO_ENUM ){
		res = proc->activeNamespace;
	}else if( p[0]->type == DAO_CLASS ){
		res = p[0]->xClass.initRoutine->nameSpace;
	}else if( p[0]->type == DAO_ROUTINE ){
		res = p[0]->xRoutine.nameSpace;
	}
	DaoProcess_PutValue( proc, (DaoValue*) res );
}
static void META_Parent( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoList *ls = DaoProcess_PutList( proc );
	int i;
	if( p[0]->type == DAO_CLASS ){
		DaoClass *k = & p[0]->xClass;
		DaoList_Append( ls, k->parent );
	}else if( p[0]->type == DAO_OBJECT ){
		DaoObject *k = & p[0]->xObject;
		DaoList_Append( ls, k->parent );
	}else if( p[0]->type == DAO_TYPE) {
		DaoType *type = (DaoType*) p[0];
		for(i=0; type->bases && i < type->bases->size; ++i){
			DaoList_Append( ls, type->bases->items.pValue[i] );
		}
	}
}
static void META_Type( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoType *tp = DaoNamespace_GetType( proc->activeNamespace, p[0] );
	DaoProcess_PutValue( proc, (DaoValue*) tp );
}

static void META_Fields( DaoProcess *proc, DaoValue *p[], int N, int cst )
{
	DNode *it;
	DaoList *list = DaoProcess_PutList( proc );
	DaoTuple *tuple;
	DaoClass *klass;
	DaoObject *object;
	DaoNamespace *ns, *here = proc->activeNamespace;
	DMap *index = NULL, *lookup = NULL;
	DList *constants, *variables;
	DaoValue *value;
	DaoValue *type = NULL;
	int restri = p[1]->xEnum.value;
	int i, st = 0;

	if( p[0]->type == DAO_CLASS ){
		klass = & p[0]->xClass;
		lookup = klass->lookupTable;
		constants = klass->constants;
		variables = klass->variables;
		st = cst ? DAO_CLASS_CONSTANT : DAO_CLASS_VARIABLE;
	}else if( p[0]->type == DAO_OBJECT ){
		object = & p[0]->xObject;
		klass = object->defClass;
		constants = klass->constants;
		variables = klass->instvars;
		st = cst ? DAO_CLASS_CONSTANT : DAO_OBJECT_VARIABLE;
	}else if( p[0]->type == DAO_NAMESPACE ){
		ns = & p[0]->xNamespace;
		lookup = ns->lookupTable;
		constants = ns->constants;
		variables = ns->variables;
		st = cst ? DAO_GLOBAL_CONSTANT : DAO_GLOBAL_VARIABLE;
	}else{
		DaoProcess_RaiseError( proc, NULL, "invalid parameter" );
		return;
	}

	index = DHash_New(0,0);
	for(it = DMap_First(lookup); it != NULL; it = DMap_Next(lookup, it) ){
		size_t id = it->value.pInt;
		if( restri && lookup && LOOKUP_PM( id ) != DAO_PERM_PUBLIC ) continue;
		if( lookup ) id = LOOKUP_ID( id ) | (LOOKUP_ST( id ) << 16);
		DMap_Insert( index, (void*) id, it->key.pString );
	}
	if( cst ){
		for(i=0; i<constants->size; ++i){
			tuple = DaoTuple_Create( list->ctype->args->items.pType[0], 2, 1 );
			it = DMap_Find( index, (void*)(size_t) (i|st<<16) );
			if( restri && it == NULL ) continue;

			if( it ) DString_Assign( tuple->values[0]->xString.value, it->value.pString );
			DaoTuple_SetItem( tuple, constants->items.pConst[i]->value, 1 );
			DaoList_PushBack( list, (DaoValue*) tuple );
		}
	}else{
		for(i=0; i<variables->size; ++i){
			tuple = DaoTuple_Create( list->ctype->args->items.pType[0], 3, 1 );
			it = DMap_Find( index, (void*)(size_t) (i|st<<16) );
			if( restri && it == NULL ) continue;

			if( it ) DString_Assign( tuple->values[0]->xString.value, it->value.pString );
			if( p[0]->type == DAO_OBJECT ){
				DaoTuple_SetItem( tuple, p[0]->xObject.objValues[i], 1 );
			}else{
				DaoTuple_SetItem( tuple, variables->items.pVar[i]->value, 1 );
			}
			DaoTuple_SetItem( tuple, (DaoValue*) variables->items.pVar[i]->dtype, 2 );
			DaoList_PushBack( list, (DaoValue*) tuple );
		}
	}
	DMap_Delete( index );
}
static void META_Cst1( DaoProcess *proc, DaoValue *p[], int N )
{
	META_Fields( proc, p, N, 1 );
}
static void META_Var1( DaoProcess *proc, DaoValue *p[], int N )
{
	META_Fields( proc, p, N, 0 );
}
static void META_Cst2( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *name = p[1]->xString.value;
	DaoConstant *cst = NULL;
	DNode *node;

	if( p[0]->type == DAO_CLASS || p[0]->type == DAO_OBJECT ){
		DaoClass *klass = & p[0]->xClass;
		if( p[0]->type == DAO_OBJECT ) klass = p[0]->xObject.defClass;
		node = DMap_Find( klass->lookupTable, name );
		if( node && LOOKUP_ST( node->value.pInt ) == DAO_CLASS_CONSTANT ){
			cst = klass->constants->items.pConst[ LOOKUP_ID( node->value.pInt ) ];
		}
	}else if( p[0]->type == DAO_NAMESPACE ){
		DaoNamespace *ns = & p[0]->xNamespace;
		node = DMap_Find( ns->lookupTable, name );
		if( node && LOOKUP_ST( node->value.pInt ) == DAO_GLOBAL_CONSTANT ){
			cst = ns->constants->items.pConst[ LOOKUP_ID( node->value.pInt ) ];
		}
	}else{
		DaoProcess_RaiseError( proc, NULL, "invalid parameter" );
	}
	if( cst == NULL ){
		DaoProcess_RaiseError( proc, NULL, "invalid field name" );
		return;
	}
	DaoProcess_PutValue( proc, cst->value );
	if( N > 2 ){
		DaoType *type = DaoNamespace_GetType( proc->activeNamespace, cst->value );
		if( DaoType_MatchValue( type, p[2], NULL ) < DAO_MT_EQ ){
			DaoProcess_RaiseError( proc, NULL, "invalid value" );
			return;
		}
		DaoValue_Copy( p[2], & cst->value );
	}
}
static void META_Var2( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *name = p[1]->xString.value;
	DaoTuple *tuple = DaoProcess_PutTuple( proc, 2 );
	DaoVariable *var = NULL;
	DNode *node;

	if( p[0]->type == DAO_OBJECT ){
		DaoObject *object = (DaoObject*) p[0];
		node = DMap_Find( object->defClass->lookupTable, name );
		if( node && LOOKUP_ST( node->value.pInt ) == DAO_CLASS_VARIABLE ){
			var = object->defClass->variables->items.pVar[ LOOKUP_ID( node->value.pInt ) ];
		}else if( node && LOOKUP_ST( node->value.pInt ) == DAO_OBJECT_VARIABLE ){
			int id = LOOKUP_ID( node->value.pInt );
			var = object->defClass->instvars->items.pVar[id];
			DaoTuple_SetItem( tuple, object->objValues[id], 0 );
			DaoTuple_SetItem( tuple, (DaoValue*) var->dtype, 1 );
			if( N > 2 && DaoValue_Move( p[2], & object->objValues[id], var->dtype ) == 0 ){
				DaoProcess_RaiseError( proc, NULL, "invalid value" );
			}
			return;
		}
	}else if( p[0]->type == DAO_CLASS ){
		DaoClass *klass = & p[0]->xClass;
		if( p[0]->type == DAO_OBJECT ) klass = p[0]->xObject.defClass;
		node = DMap_Find( klass->lookupTable, name );
		if( node && LOOKUP_ST( node->value.pInt ) == DAO_CLASS_VARIABLE ){
			var = klass->variables->items.pVar[ LOOKUP_ID( node->value.pInt ) ];
		}
	}else if( p[0]->type == DAO_NAMESPACE ){
		DaoNamespace *ns = & p[0]->xNamespace;
		node = DMap_Find( ns->lookupTable, name );
		if( node && LOOKUP_ST( node->value.pInt ) == DAO_GLOBAL_VARIABLE ){
			var = ns->variables->items.pVar[ LOOKUP_ID( node->value.pInt ) ];
		}
	}else{
		DaoProcess_RaiseError( proc, NULL, "invalid parameter" );
	}
	if( var == NULL ){
		DaoProcess_RaiseError( proc, NULL, "invalid field name" );
		return;
	}
	DaoTuple_SetItem( tuple, var->value, 0 );
	DaoTuple_SetItem( tuple, (DaoValue*) var->dtype, 1 );
	if( N > 2 && DaoValue_Move( p[2], & var->value, var->dtype ) == 0 ){
		DaoProcess_RaiseError( proc, NULL, "invalid value" );
	}
}
static void META_Routine( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoList *list;
	DaoValue *item;
	int i;
	if( N == 1 ){
		DaoRoutine *rout = & p[0]->xRoutine;
		list = DaoProcess_PutList( proc );
		if( p[0]->type != DAO_ROUTINE ){
			DaoProcess_RaiseError( proc, NULL, "invalid parameter" );
			return;
		}
		if( rout->overloads ){
			for(i=0; i<rout->overloads->routines->size; i++){
				item = rout->overloads->routines->items.pValue[i];
				DaoList_Append( list, item );
			}
		}else{
		}
	}else{
		DaoProcess_PutValue( proc, (DaoValue*) proc->activeRoutine );
	}
}
static void META_Class( DaoProcess *proc, DaoValue *p[], int N )
{
	if( p[0]->type == DAO_ROUTINE && p[0]->xRoutine.routHost ){
		DaoProcess_PutValue( proc, (DaoValue*) p[0]->xRoutine.routHost->aux );
	}else if( p[0]->type == DAO_OBJECT ){
		DaoProcess_PutValue( proc, (DaoValue*) p[0]->xObject.defClass );
	}else if( p[0]->type == DAO_CDATA || p[0]->type == DAO_CSTRUCT ){
		DaoProcess_PutValue( proc, (DaoValue*) p[0]->xCstruct.ctype->aux );
	}else{
		DaoProcess_PutNone( proc );
	}
}
static void META_Child( DaoProcess *proc, DaoValue *p[], int N )
{
	if( p[0]->type == DAO_OBJECT ){
		DaoProcess_PutValue( proc, (DaoValue*) p[0]->xObject.rootObject );
	}else if( p[0]->type == DAO_CDATA || p[0]->type == DAO_CSTRUCT ){
		DaoProcess_PutValue( proc, (DaoValue*) p[0]->xCstruct.object );
	}else{
		DaoProcess_PutNone( proc );
	}
}
static void META_Param( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoRoutine *routine = (DaoRoutine*) p[0];
	DaoList *routConsts = routine->routConsts;
	DaoList *list = DaoProcess_PutList( proc );
	DaoType *routype = routine->routType;
	DaoType *itp = list->ctype->args->items.pType[0];
	int i;
	for(i=0; i<routype->args->size; i++){
		DaoType *partype = routype->args->items.pType[i];
		DaoTuple *tuple = DaoTuple_Create( itp, 2 + (partype->tid == DAO_PAR_DEFAULT), 1 );

		if( partype->fname ) DString_Assign( tuple->values[0]->xString.value, partype->fname );
		DaoTuple_SetItem( tuple, partype->fname ? partype->aux : (DaoValue*) partype, 1 );
		if( tuple->size == 3 ) DaoTuple_SetItem( tuple, routConsts->value->items.pValue[i], 2 );
		DaoList_Append( list, (DaoValue*) tuple );
	}
}
static void META_Trace( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTuple *entry = NULL;
	DaoList *backtrace = DaoProcess_PutList( proc );
	DaoStackFrame *frame = proc->topFrame;
	DaoInteger line = {DAO_INTEGER,0,0,0,0,0};
	DaoInteger inst = {DAO_INTEGER,0,0,0,0,0};
	int print = p[0]->xEnum.value;
	int maxDepth = p[1]->xInteger.value;
	int depth = 1;

	if( print ){
		DaoProcess_Trace( proc, maxDepth );
		return;
	}

	frame = frame->prev; /* skip this frame for trace(); */
	for( ; frame && frame->routine; frame = frame->prev, ++depth ){
		if( depth > maxDepth && maxDepth > 0 ) break;

		inst.value = line.value = 0;
		if( frame->routine->body ){
			DaoRoutineBody *body = frame->routine->body;
			DaoVmCode *codes = body->vmCodes->data.codes;
			inst.value = (depth==1) ? (int)(proc->activeCode - codes) : frame->entry;
			line.value = body->annotCodes->items.pVmc[inst.value]->line;
		}

		entry = DaoTuple_Create( backtrace->ctype->args->items.pType[0], 3, 1 );

		DaoTuple_SetItem( entry, (DaoValue*) frame->routine, 0 );
		DaoTuple_SetItem( entry, (DaoValue*) & inst, 1 );
		DaoTuple_SetItem( entry, (DaoValue*) & line, 2 );

		DaoList_PushBack( backtrace, (DaoValue*) entry );
	}
}
static DaoFunctionEntry metaMeths[]=
{
	/* TODO: methods for types. */
	{ META_Name, /* TODO: type for cdata; */
		"nameOf( object: routine|routine[]|class|interface|namespace|type ) => string"
	},
	{ META_NS,
		"namespaceOf( object: routine|routine[]|class|enum<here> = $here ) => namespace"
	},
	{ META_Type,
		"typeOf( object: any ) => type"
	},
	{ META_Class,
		"classOf( object: any ) => any"
	},
	{ META_Parent,
		"parentOf( object: any ) => list<any>"
	},
	{ META_Child,
		"childOf( object: any ) => any"
	},
	{ META_Routine,
		"currentRoutine() => routine|routine[]"
	},
	{ META_Routine,
		"overloadsOf( rout: routine|routine[] ) => list<routine|routine[]>"
	},
	{ META_Param,
		"parametersOf( rout: routine|routine[] )"
			"=> list< tuple<name: string, partype: any, ... : any> >"
	},
	{ META_Cst1,
		"constants( object: any, restrict = false ) "
			"=> list< tuple<name: string, value: any> >"
	},
	{ META_Var1,
		"variables( object: any, restrict = false ) "
			"=> list< tuple<name: string, value: any, dectype: any> >"
	},
	{ META_Cst2,
		"constant( object: any, name: string ) => any"
	},
	{ META_Var2,
		"variable( object: any, name: string ) => tuple<value: any, dectype: any>"
	},
	{ META_Cst2,
		"constant( object: any, name: string, value: any ) => any"
	},
	{ META_Var2,
		"variable( object: any, name: string, value: any ) => tuple<value: any, dectype: any>"
	},
	{ META_Trace,
		"trace( action:enum<generate,print> = $generate, depth = 0 ) "
			"=> list< tuple<call: routine|routine[], instruction: int, line: int> >"
	},
	{ NULL, NULL }
};

DAO_DLL int DaoMeta_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *metans = DaoNamespace_GetNamespace( ns, "meta" );
	DaoNamespace_WrapFunctions( metans, metaMeths );
	return 0;
}
