/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2015,2016, Limin Fu
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

// 2015-06, Aleksey Danilov: initial implementation

#include "dao_set.h"
#include "daoVmcode.h"

static DaoType *daox_type_set = NULL;

DaoSet* DaoSet_New( DaoType *type, int hashing )
{
	DaoSet *res = (DaoSet*)dao_malloc( sizeof(DaoSet) );
	DaoCstruct_Init( (DaoCstruct*)res, type );
	res->map = hashing? DHash_New( DAO_DATA_VALUE, DAO_DATA_NULL ) : DMap_New( DAO_DATA_VALUE, DAO_DATA_NULL );
	res->modcount = 0;
	return res;
}

void DaoSet_Delete( DaoSet *self )
{
	DMap_Delete( self->map );
	DaoCstruct_Free( (DaoCstruct*)self );
	dao_free( self );
}

void DaoSet_Modify( DaoSet *self ){
	self->modcount++;
}

static void DaoSet_HandleGC( DaoValue *p, DList *values, DList *arrays, DList *maps, int remove )
{
	DaoSet *self = (DaoSet*)p;
	DNode *node;
	for ( node = DMap_First( self->map ); node; node = DMap_Next( self->map, node ) )
		if ( node->key.pValue ){
			DList_Append( values, node->key.pValue );
			if ( remove )
				node->key.pValue = NULL;
		}
	if ( remove )
		DMap_Clear( self->map );
}

static void DaoSet_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = DaoSet_New( DaoProcess_GetReturnType( proc ), p[0]->xEnum.value == 1 );
	DaoProcess_PutValue( proc, (DaoValue*)self );
}

static void DaoSet_Size( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoProcess_PutInteger( proc, self->map->size );
}

static void DaoSet_Insert( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DMap_Insert( self->map, p[1], NULL );
	DaoSet_Modify( self );
}

static void DaoSet_Contains( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoProcess_PutBoolean( proc, DMap_Find( self->map, p[1] ) != NULL );
}

static void DaoSet_Contains2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	int res = 0;
	if ( p[1]->type == DAO_LIST ){
		DaoList *other = &p[1]->xList;
		daoint i;
		for ( i = 0; i < other->value->size; i++ )
			if ( !DMap_Find( self->map, DaoList_GetItem( other, i ) ) )
				goto End;
		res = 1;
	}
	else {
		DaoSet *other = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		DNode *node;
		for ( node = DMap_First( other->map ); node; node = DMap_Next( other->map, node ) )
			if ( !DMap_Find( self->map, node->key.pValue ) )
				goto End;
		res = 1;
	}
End:
	DaoProcess_PutBoolean( proc, res );
}

static void DaoSet_Erase( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DMap_Erase( self->map, p[1] );
	DaoSet_Modify( self );
}

static void DaoSet_Clone( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *res = DaoSet_New( DaoProcess_GetReturnType( proc ), self->map->hashing );
	DNode *node;
	for ( node = DMap_First( self->map ); node; node = DMap_Next( self->map, node ) )
		DMap_Insert( res->map, node->key.pValue, NULL );
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

void DaoSet_Union( DaoSet *a, DaoSet *b, DaoSet *res )
{
	DNode *node;
	for ( node = DMap_First( a->map ); node; node = DMap_Next( a->map, node ) )
		DMap_Insert( res->map, node->key.pValue, NULL );
	for ( node = DMap_First( b->map ); node; node = DMap_Next( b->map, node ) )
		DMap_Insert( res->map, node->key.pValue, NULL );
}

void DaoSet_UnionList( DaoSet *a, DaoList *b, DaoSet *res )
{
	DNode *node;
	daoint i;
	for ( node = DMap_First( a->map ); node; node = DMap_Next( a->map, node ) )
		DMap_Insert( res->map, node->key.pValue, NULL );
	for ( i = 0; i < b->value->size; i++ )
		DMap_Insert( res->map, DaoList_GetItem( b, i ), NULL );
}

static void DaoSet_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *res;
	if ( p[1]->type == DAO_LIST ){
		res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_UnionList( a, &p[1]->xList, res );
	}
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_Union( a, b, res );
	}
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

static void DaoSet_AddTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *res = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
	DMap_Clear( res->map );
	if ( p[2]->type == DAO_LIST )
		DaoSet_UnionList( a, &p[2]->xList, res );
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[2], NULL );
		DaoSet_Union( a, b, res );
	}
	DaoSet_Modify( res );
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

void DaoSet_Difference( DaoSet *a, DaoSet *b, DaoSet *res )
{
	DNode *node;
	for ( node = DMap_First( a->map ); node; node = DMap_Next( a->map, node ) )
		if ( !DMap_Find( b->map, node->key.pValue ) )
			DMap_Insert( res->map, node->key.pValue, NULL );
}

void DaoSet_DifferenceList( DaoSet *a, DaoList *b, DaoSet *res )
{
	DNode *node;
	for ( node = DMap_First( a->map ); node; node = DMap_Next( a->map, node ) ){
		daoint i;
		int found = 0;
		for ( i = 0; i < b->value->size; i++ )
			if ( DaoValue_Compare( DaoList_GetItem( b, i ), node->key.pValue ) == 0 ){
				found = 1;
				break;
			}

		if ( !found )
			DMap_Insert( res->map, node->key.pValue, NULL );
	}
}

static void DaoSet_Sub( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *res;
	if ( p[1]->type == DAO_LIST ){
		res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_DifferenceList( a, &p[1]->xList, res );
	}
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_Difference( a, b, res );
	}
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

static void DaoSet_SubTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *res = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
	DMap_Clear( res->map );
	if ( p[2]->type == DAO_LIST )
		DaoSet_DifferenceList( a, &p[2]->xList, res );
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[2], NULL );
		DaoSet_Difference( a, b, res );
	}
	DaoSet_Modify( res );
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

void DaoSet_Intersection( DaoSet *a, DaoSet *b, DaoSet *res )
{
	DNode *node;
	for ( node = DMap_First( a->map ); node; node = DMap_Next( a->map, node ) )
		if ( DMap_Find( b->map, node->key.pValue ) )
			DMap_Insert( res->map, node->key.pValue, NULL );
}

void DaoSet_IntersectionList( DaoSet *a, DaoList *b, DaoSet *res )
{
	DNode *node;
	for ( node = DMap_First( a->map ); node; node = DMap_Next( a->map, node ) ){
		daoint i;
		int found = 0;
		for ( i = 0; i < b->value->size; i++ )
			if ( DaoValue_Compare( DaoList_GetItem( b, i ), node->key.pValue ) == 0 ){
				found = 1;
				break;
			}

		if ( found )
			DMap_Insert( res->map, node->key.pValue, NULL );
	}
}

static void DaoSet_BitAnd( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *res;
	if ( p[1]->type == DAO_LIST ){
		res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_IntersectionList( a, &p[1]->xList, res );
	}
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		DaoSet *res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_Intersection( a, b, res );
	}
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

static void DaoSet_BitAndTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *res = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
	DMap_Clear( res->map );
	if ( p[2]->type == DAO_LIST )
		DaoSet_IntersectionList( a, &p[2]->xList, res );
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[2], NULL );
		DaoSet_Intersection( a, b, res );
	}
	DaoSet_Modify( res );
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

void DaoSet_SymDifference( DaoSet *a, DaoSet *b, DaoSet *res )
{
	DNode *node;
	for ( node = DMap_First( a->map ); node; node = DMap_Next( a->map, node ) )
		if ( !DMap_Find( b->map, node->key.pValue ) )
			DMap_Insert( res->map, node->key.pValue, NULL );
	for ( node = DMap_First( b->map ); node; node = DMap_Next( b->map, node ) )
		if ( !DMap_Find( a->map, node->key.pValue ) )
			DMap_Insert( res->map, node->key.pValue, NULL );
}

void DaoSet_SymDifferenceList( DaoSet *a, DaoList *b, DaoSet *res )
{
	DNode *node;
	daoint i;
	for ( node = DMap_First( a->map ); node; node = DMap_Next( a->map, node ) ){
		int found = 0;
		for ( i = 0; i < b->value->size; i++ )
			if ( DaoValue_Compare( DaoList_GetItem( b, i ), node->key.pValue ) == 0 ){
				found = 1;
				break;
			}

		if ( !found )
			DMap_Insert( res->map, node->key.pValue, NULL );
	}
	for ( i = 0; i < b->value->size; i++ ){
		DaoValue *item = DaoList_GetItem( b, i );
		if ( !DMap_Find( a->map, item ) )
			DMap_Insert( res->map, item, NULL );
	}
}

static void DaoSet_BitXor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *res;
	if ( p[1]->type == DAO_LIST ){
		res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_SymDifferenceList( a, &p[1]->xList, res );
	}
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_SymDifference( a, b, res );
	}
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

static void DaoSet_BitXorTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *res = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
	DMap_Clear( res->map );
	if ( p[2]->type == DAO_LIST )
		DaoSet_SymDifferenceList( a, &p[2]->xList, res );
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[2], NULL );
		DaoSet_SymDifference( a, b, res );
	}
	DaoSet_Modify( res );
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

static void DaoSet_AddAssign( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	if ( p[1]->type == DAO_LIST ){
		DaoList *list = &p[1]->xList;
		daoint i;
		for ( i = 0; i < list->value->size; i++ )
			DMap_Insert( self->map, DaoList_GetItem( list, i ), NULL );
	}
	else {
		DaoSet *other = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		DNode *node;
		for ( node = DMap_First( other->map ); node; node = DMap_Next( other->map, node ) )
			DMap_Insert( self->map, node->key.pValue, NULL );
	}
	DaoSet_Modify( self );
	DaoProcess_PutValue( proc, (DaoValue*)self );
}

static void DaoSet_SubAssign( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	if ( p[1]->type == DAO_LIST ){
		DaoList *list = &p[1]->xList;
		daoint i;
		for ( i = 0; i < list->value->size; i++ )
			DMap_Erase( self->map, DaoList_GetItem( list, i ) );
	}
	else {
		DaoSet *other = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		DNode *node;
		for ( node = DMap_First( other->map ); node; node = DMap_Next( other->map, node ) )
			DMap_Erase( self->map, node->key.pValue );
	}
	DaoSet_Modify( self );
	DaoProcess_PutValue( proc, (DaoValue*)self );
}

void DaoSet_Cartesian( DaoSet *a, DaoSet *b, DaoSet *res )
{
	DNode *node1, *node2;
	if ( !a->map->size || !b->map->size )
		return;
	for ( node1 = DMap_First( a->map ); node1; node1 = DMap_Next( a->map, node1 ) )
		for ( node2 = DMap_First( b->map ); node2; node2 = DMap_Next( b->map, node2 ) ){
			DaoTuple *tup = DaoTuple_Create( res->ctype->args->items.pType[0], 2, 1 );
			DaoTuple_SetItem( tup, node1->key.pValue, 0 );
			DaoTuple_SetItem( tup, node2->key.pValue, 1 );
			DMap_Insert( res->map, tup, NULL );
		}
}

void DaoSet_CartesianList( DaoSet *a, DaoList *b, DaoSet *res )
{
	DNode *node1;
	daoint i;
	if ( !a->map->size || !b->value->size )
		return;
	for ( node1 = DMap_First( a->map ); node1; node1 = DMap_Next( a->map, node1 ) )
		for ( i = 0; i < b->value->size; i++ ){
			DaoTuple *tup = DaoTuple_Create( res->ctype->args->items.pType[0], 2, 1 );
			DaoTuple_SetItem( tup, node1->key.pValue, 0 );
			DaoTuple_SetItem( tup, DaoList_GetItem( b, i ), 1 );
			DMap_Insert( res->map, tup, NULL );
		}
}

static void DaoSet_Mul( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *res;
	if ( p[1]->type == DAO_LIST ){
		res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_CartesianList( a, &p[1]->xList, res );
	}
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		res = DaoSet_New( DaoProcess_GetReturnType( proc ), a->map->hashing );
		DaoSet_Cartesian( a, b, res );
	}
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

static void DaoSet_MulTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *res = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
	DMap_Clear( res->map );
	if ( p[2]->type == DAO_LIST )
		DaoSet_CartesianList( a, &p[2]->xList, res );
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[2], NULL );
		DaoSet_Cartesian( a, b, res );
	}
	DaoSet_Modify( res );
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

static void DaoSet_ToString( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoStream *stream = DaoStream_New();
	DNode *node;
	int first = 1;
	DaoStream_SetStringMode( stream );
	DaoStream_WriteChars( stream, "{ " );
	for ( node = DMap_First( self->map ); node; node = DMap_Next( self->map, node ) ){
		if ( first )
			first = 0;
		else
			DaoStream_WriteChars( stream, ", " );
		DaoValue_Print( node->key.pValue, stream, NULL, proc );
	}
	DaoStream_WriteChars( stream, " }" );
	DaoProcess_PutString( proc, stream->buffer );
	DaoStream_Delete( stream );
}

static void DaoSet_ToList( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoList *list = DaoProcess_PutList( proc );
	DNode *node;
	for ( node = DMap_First( self->map ); node; node = DMap_Next( self->map, node ) )
		DaoList_Append( list, node->key.pValue );
}

int DaoSet_Equal( DaoSet *a, DaoSet *b )
{
	if ( a->map->size != b->map->size )
		return 0;
	if ( !a->map->hashing && !b->map->hashing ){
		// ordered maps can be compared sequentially
		DNode *node1, *node2;
		for ( node1 = DMap_First( a->map ), node2 = DMap_First( b->map ); node1 && node2;
			  node1 = DMap_Next( a->map, node1 ), node2 = DMap_Next( b->map, node2 ))
			if ( DaoValue_Compare( node1->key.pValue, node2->key.pValue ) != 0 )
				return 0;
	}
	else {
		DNode *node;
		for ( node = DMap_First( a->map ); node; node = DMap_Next( a->map, node ) )
			if ( !DMap_Find( b->map, node->key.pValue ) )
				return 0;
	}
	return 1;
}

int DaoSet_EqualList( DaoSet *set, DaoList *list )
{
	daoint i;
	if ( set->map->size != list->value->size )
		return 0;
	for ( i = 0; i < list->value->size; i++ )
		if ( !DMap_Find( set->map, DaoList_GetItem( list, i ) ) )
			return 0;
	return 1;
}

static void DaoSet_Eq( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	if ( p[1]->type == DAO_LIST )
		DaoProcess_PutBoolean( proc, DaoSet_EqualList( a, &p[1]->xList ) );
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		DaoProcess_PutBoolean( proc, DaoSet_Equal( a, b ) );
	}
}

static void DaoSet_NotEq( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *a = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	if ( p[1]->type == DAO_LIST )
		DaoProcess_PutBoolean( proc, !DaoSet_EqualList( a, &p[1]->xList ) );
	else {
		DaoSet *b = (DaoSet*)DaoValue_CastCstruct( p[1], NULL );
		DaoProcess_PutBoolean( proc, !DaoSet_Equal( a, b ) );
	}
}

static void DaoSet_Clear( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DMap_Clear( self->map );
	DaoSet_Modify( self );
}

static void DaoSet_For( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoTuple *iter = &p[1]->xTuple;
	self->modcount = 0;
	DaoTuple_SetItem( iter, (DaoValue*)DaoInteger_New( self->map->size > 0 ), 0 );
	DaoTuple_SetItem( iter, (DaoValue*)DaoInteger_New( (daoint)DMap_First( self->map ) ), 1 );
}

static void DaoSet_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoSet *self = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoTuple *iter = &p[1]->xTuple;
	DaoInteger *ptr = &iter->values[1]->xInteger;
	DNode *node = (DNode*)(daoint)ptr->value;
	if ( self->modcount )
		DaoProcess_RaiseError( proc, "Error", "Set was modified while being iterated" );
	else if ( node ){
		DaoProcess_PutValue( proc, node->key.pValue );
		ptr->value = (daoint)DMap_Next( self->map, node );
		iter->values[0]->xInteger.value = ptr->value != 0;
	}
}

static void DaoSet_BasicFunctional( DaoProcess *proc, DaoValue *p[], int npar, int funct )
{
	DaoSet *set = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *set2 = NULL;
	DaoValue *res;
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 2 );
	daoint entry;
	DNode *node;
	int popped = 0;
	switch( funct ){
	case DVM_FUNCT_SELECT:
	case DVM_FUNCT_COLLECT:
		set2 = DaoSet_New( DaoProcess_GetReturnType( proc ), set->map->hashing );
		DaoProcess_PutValue( proc, (DaoValue*)set2 );
		break;
	case DVM_FUNCT_FIND:
		DaoProcess_PutNone( proc );
		break;
	}
	if( sect == NULL ) return;
	entry = proc->topFrame->entry;
	set->modcount = 0;
	for ( node = DMap_First( set->map ); node; node = DMap_Next( set->map, node ) ){
		if ( sect->b > 0 )
			DaoProcess_SetValue( proc, sect->a, node->key.pValue );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if ( proc->status == DAO_PROCESS_ABORTED )
			break;
		if ( set->modcount ){
			DaoProcess_RaiseError( proc, "Error", "Set was modified while being iterated" );
			break;
		}
		res = proc->stackValues[0];
		switch ( funct ){
		case DVM_FUNCT_COLLECT:
			if ( res->type != DAO_NONE )
				DMap_Insert( set2->map, res, NULL );
			break;
		case DVM_FUNCT_SELECT:
			if ( res->xBoolean.value )
				DMap_Insert( set2->map, node->key.pValue, NULL );
			break;
		}
		if ( funct == DVM_FUNCT_FIND && res->xBoolean.value ){
			popped = 1;
			DaoProcess_PopFrame( proc );
			DaoProcess_PutValue( proc, node->key.pValue );
			break;
		}
	}
	if ( popped == 0 )
		DaoProcess_PopFrame( proc );
}

static void DaoSet_Collect( DaoProcess *proc, DaoValue *p[], int npar )
{
	DaoSet_BasicFunctional( proc, p, npar, DVM_FUNCT_COLLECT );
}

static void DaoSet_Find( DaoProcess *proc, DaoValue *p[], int npar )
{
	DaoSet_BasicFunctional( proc, p, npar, DVM_FUNCT_FIND );
}

static void DaoSet_Select( DaoProcess *proc, DaoValue *p[], int npar )
{
	DaoSet_BasicFunctional( proc, p, npar, DVM_FUNCT_SELECT );
}

static void DaoSet_Iterate( DaoProcess *proc, DaoValue *p[], int npar )
{
	DaoSet_BasicFunctional( proc, p, npar, DVM_FUNCT_ITERATE );
}

static void DaoSet_BasicReduce( DaoProcess *proc, DaoValue *p[], int npar, int which )
{
	DaoSet *set= (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoValue *res = NULL;
	daoint entry;
	DaoVmCode *sect;
	DNode *node = DMap_First( set->map );

	if ( which == 1 ){
		res = node? node->key.pValue : dao_none_value;
		node = DMap_Next( set->map, node );
	}
	else
		res = p[1];
	if ( set->map->size == 0 ){
		DaoProcess_PutValue( proc, res );
		return;
	}
	sect = DaoProcess_InitCodeSection( proc, 3 );
	if( sect == NULL ) return;
	entry = proc->topFrame->entry;
	set->modcount = 0;
	for ( ; node; node = DMap_Next( set->map, node ) ){
		if ( sect->b > 0 )
			DaoProcess_SetValue( proc, sect->a, node->key.pValue );
		if ( sect->b > 1 )
			DaoProcess_SetValue( proc, sect->a + 1, res );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if ( proc->status == DAO_PROCESS_ABORTED )
			break;
		if ( set->modcount ){
			DaoProcess_RaiseError( proc, "Error", "Set was modified while being iterated" );
			break;
		}
		res = proc->stackValues[0];
	}
	DaoProcess_PopFrame( proc );
	DaoProcess_PutValue( proc, res );
}

static void DaoSet_Reduce( DaoProcess *proc, DaoValue *p[], int npar )
{
	DaoSet_BasicReduce( proc, p, npar, 1 );
}
static void DaoSet_Reduce2( DaoProcess *proc, DaoValue *p[], int npar )
{
	DaoSet_BasicReduce( proc, p, npar, 2 );
}

static unsigned int GetHashSeed( DaoProcess *self, DaoValue *seed )
{
	if( seed->type == DAO_INTEGER ) return seed->xInteger.value;
	if( seed->type == DAO_ENUM ){
		unsigned int hashing = seed->xEnum.value;
		if( hashing == 2 ) hashing = rand();
		return hashing;
	}
	return 0;
}

static void DaoSet_Functional2( DaoProcess *proc, DaoValue *p[], int npar, int meth )
{
	DaoValue *res = NULL;
	DaoMap *map = NULL;
	DaoSet *set3 = NULL;
	DaoSet *set = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoSet *set2 = ( npar > 1 && p[1]->type == DAO_CSTRUCT )? (DaoSet*)DaoValue_CastCstruct( p[1], NULL ) : NULL;
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 3 );
	daoint entry;
	DNode *node1, *node2;
	unsigned int hashing = npar > 2? GetHashSeed( proc, p[2] ) : 0;

	switch ( meth ){
	case DVM_FUNCT_COLLECT:
		set3 = DaoSet_New( DaoProcess_GetReturnType( proc ), set->map->hashing );
		DaoProcess_PutValue( proc, (DaoValue*)set3 );
		break;
	case DVM_FUNCT_ASSOCIATE:
		map = DaoProcess_PutMap( proc, hashing );
		break;
	}

	if ( sect == NULL ) return;
	entry = proc->topFrame->entry;
	set->modcount = 0;
	if ( set2 ){
		node2 = DMap_First( set2->map );
		set2->modcount = 0;
	}
	for ( node1 = DMap_First( set->map ); node1; node1 = DMap_Next( set->map, node1 ) ){
		if ( set2 && !node2 )
			break;
		if( sect->b > 0 )
			DaoProcess_SetValue( proc, sect->a, node1->key.pValue );
		if( sect->b > 1 )
			DaoProcess_SetValue( proc, sect->a + 1, node2->key.pValue );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if ( proc->status == DAO_PROCESS_ABORTED )
			break;
		if ( set->modcount || ( set2 && set2->modcount ) ){
			DaoProcess_RaiseError( proc, "Error", "Set was modified while being iterated" );
			break;
		}
		res = proc->stackValues[0];
		if ( res->type == DAO_NONE )
			continue;
		switch ( meth ){
		case DVM_FUNCT_COLLECT:
			DMap_Insert( set3->map, res, NULL );
			break;
		case DVM_FUNCT_ASSOCIATE :
			DaoMap_Insert( map, res->xTuple.values[0], res->xTuple.values[1] );
			break;
		}
		if ( set2 )
			node2 = DMap_Next( set2->map, node2 );
	}
	DaoProcess_PopFrame( proc );
}

static void DaoSet_Collect2( DaoProcess *proc, DaoValue *p[], int npar )
{
	DaoSet_Functional2( proc, p, npar, DVM_FUNCT_COLLECT );
}

static void DaoSet_Associate2( DaoProcess *proc, DaoValue *p[], int npar )
{
	DaoSet_Functional2( proc, p, npar, DVM_FUNCT_ASSOCIATE );
}
static void DaoSet_Associate( DaoProcess *proc, DaoValue *p[], int npar )
{
	DaoValue *res = NULL;
	DaoSet *set = (DaoSet*)DaoValue_CastCstruct( p[0], NULL );
	DaoMap *map = DaoProcess_PutMap( proc, GetHashSeed( proc, p[1] ) );
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 2 );
	daoint entry = proc->topFrame->entry;
	DNode *node;

	if ( sect == NULL ) return;
	set->modcount = 0;
	for ( node = DMap_First( set->map ); node; node = DMap_Next( set->map, node ) ){
		if ( sect->b > 0 )
			DaoProcess_SetValue( proc, sect->a, node->key.pValue );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if ( proc->status == DAO_PROCESS_ABORTED )
			break;
		if ( set->modcount ){
			DaoProcess_RaiseError( proc, "Error", "Set was modified while being iterated" );
			break;
		}
		res = proc->stackValues[0];
		if ( res->type == DAO_NONE )
			continue;
		DaoMap_Insert( map, res->xTuple.values[0], res->xTuple.values[1] );
	}
	DaoProcess_PopFrame( proc );
}

static DaoFunctionEntry daoSetMeths[] =
{
	//! Constructs new tree- or hash-based set depending on \a kind
	{ DaoSet_Create,	"Set<@T>(kind: enum<tree,hash> = $tree)" },

	//! Set size
	{ DaoSet_Size,		".size(invar self: Set<@T>) => int" },

	//! Inserts \a value into the set
	{ DaoSet_Insert,	"insert(self: Set<@T>, invar value: @T)" },

	//! Check if the set contains the given \a value
	{ DaoSet_Contains,	"contains(invar self: Set<@T>, invar value: @T) => bool" },
	{ DaoSet_Contains,	"[](invar self: Set<@T>, invar value: @T) => bool" },

	//! Check if the set contains \a other set
	{ DaoSet_Contains2,	"contains(invar self: Set<@T>, invar other: Set<@T>|list<@T>) => bool" },

	//! Removes \a value from the set
	{ DaoSet_Erase,		"erase(self: Set<@T>, invar value: @T)" },

	//! Returns set copy
	{ DaoSet_Clone,		"clone(invar self: Set<@T>) => Set<@T>" },

	//! Removes all items from the set
	{ DaoSet_Clear,		"clear(self: Set<@T>)" },

	//! Insert \a other items in the set and returns self
	{ DaoSet_AddAssign,	"+=(self: Set<@T>, invar other: Set<@T>|list<@T>) => Set<int>" },
	{ DaoSet_AddAssign,	"|=(self: Set<@T>, invar other: Set<@T>|list<@T>) => Set<int>" },

	//! Removes \a other items from the set and returns self
	{ DaoSet_SubAssign,	"-=(self: Set<@T>, invar other: Set<@T>|list<@T>) => Set<int>" },

	//! Union of \a a and \a b
	{ DaoSet_Add,		"+(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },
	{ DaoSet_AddTo,		"+(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },
	{ DaoSet_Add,		"|(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },
	{ DaoSet_AddTo,		"|(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },

	//! Difference of \a a and \a b
	{ DaoSet_Sub,		"-(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },
	{ DaoSet_SubTo,		"-(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },

	//! Intersection of \a a and \a b
	{ DaoSet_BitAnd,	"&(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },
	{ DaoSet_BitAndTo,	"&(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },

	//! Symmetric defference of \a a and \a b
	{ DaoSet_BitXor,	"^(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },
	{ DaoSet_BitXorTo,	"^(c: Set<@T>, invar a: Set<@T>, invar b: Set<@T>|list<@T>) => Set<@T>" },

	//! Cartesian product of \a a and \a b
	{ DaoSet_Mul,		"*(invar a: Set<@X>, invar b: Set<@Y>|list<@Y>) => Set<tuple<@X,@Y>>" },
	{ DaoSet_MulTo,		"*(c: Set<tuple<@X,@Y>>, invar a: Set<@X>, invar b: Set<@Y>|list<@Y>) => Set<tuple<@X,@Y>>" },

	//! String conversion
	{ DaoSet_ToString,	"(string)(invar self: Set<@T>)" },

	//! Set items as list
	{ DaoSet_ToList,	"items(invar self: Set<@T>) => list<@T>" },

	//! Set comparison
	{ DaoSet_Eq,		"==(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => bool" },
	{ DaoSet_NotEq,		"!=(invar a: Set<@T>, invar b: Set<@T>|list<@T>) => bool" },

	//! For-in iteration support
	{ DaoSet_For,		"for(invar self: Set<@T>, iterator: ForIterator)" },
	{ DaoSet_Get,		"[](invar self: Set<@T>, index: ForIterator) => @T" },

	//! Iterates over the set, collecting non-`none` values returned from the code section into a new set
	{ DaoSet_Collect,	"collect(invar self: Set<@T>)[invar item: @T => @V|none] => Set<@V>" },

	//! Iterates over the two sets in parallel, collecting non-`none` values returned from the code section into a new set
	{ DaoSet_Collect2,	"collect(invar self: Set<@X>, invar other: Set<@Y>)"
							"[invar item: @X, invar item2: @Y => @V|none] => Set<@V>" },

	//! Iterates over the set, forming a map out of the key-value tuples returned from the code section
	{ DaoSet_Associate,	"associate(invar self: Set<@T>, hashing: enum<none,auto,random>|int = $none)"
							"[invar item: @T => none|tuple<@K,@V>] => map<@K,@V>" },

	//! Iterates over the two sets in parallel, forming a map out of the key-value tuples returned from the code section
	{ DaoSet_Associate2,"associate(invar self: Set<@X>, invar other: Set<@Y>, hashing: enum<none,auto,random>|int = $none)"
							"[invar item: @X, invar item2: @Y => none|tuple<@K,@V>] => map<@K,@V>" },

	//! Returns the first item in the set satisfying the condition specified by the code section (`none` if not found)
	{ DaoSet_Find,		"find(invar self: Set<@T>)[invar item: @T => bool] => @T|none" },

	//! Iterates over the set, yielding each \a item and accumulated \a value (with the initial value of \a init or the first
	//! item in the set); code section result forms the new value to be passed to the next iteration. Returns the accumulated value
	{ DaoSet_Reduce,	"reduce(invar self: Set<@T>)[invar item: @T, value: @T => @T] => @T|none" },
	{ DaoSet_Reduce2,	"reduce(invar self: Set<@T>, init: @V)[invar item: @T, value: @V => @V] => @V" },

	//! Selects all items satisfying the condition specified by the code section and returns them in a new set
	{ DaoSet_Select,	"select(invar self: Set<@T>)[invar item: @T => bool] => Set<@T>" },

	//! Iterates over set items
	{ DaoSet_Iterate,	"iterate(invar self: Set<@T>)[invar item: @T]" },
	{ NULL, NULL }
};

/*! Tree- or hash-based set.
 *
 * \note For set operations involving two sets and producing a new set, the kind of the resulting set is determined by the
 * left operand */


static DaoType* DaoSet_CheckUnary( DaoType *type, DaoVmCode *op, DaoRoutine *ctx )
{
	if( op->code == DVM_SIZE ) return dao_type_int;
	return NULL;
}

static DaoValue* DaoSet_DoUnary( DaoValue *value, DaoVmCode *op, DaoProcess *proc )
{
	DaoSet *self = (DaoSet*) value;
	if( op->code == DVM_SIZE ) DaoProcess_PutInteger( proc, self->map->size );
	return NULL;
}

static DaoType* DaoSet_CheckBinary( DaoType *self, DaoVmCode *op, DaoType *args[2], DaoRoutine *ctx )
{
	return DaoCstruct_CheckBinary( self, op, args, ctx );

#if 0
	// TODO
	switch( op->code ){
	case DVM_ADD : case DVM_SUB : case DVM_MUL :
	case DVM_BITAND : case DVM_BITOR  : case DVM_BITXOR :
		if( self != args[0] ) return NULL;
		if( args[1]->tid == DAO_CSTRUCT ){
		}else if( args[1]->tid == DAO_LIST ){
		}else{
			return NULL;
		}
		break;
	case DVM_EQ : case DVM_NE :
	case DVM_IN :
		break;
	default: return NULL;
	}
	return NULL;
#endif
}

DaoValue* DaoSet_DoBinary( DaoValue *self, DaoVmCode *op, DaoValue *args[2], DaoProcess *proc )
{
	return DaoCstruct_DoBinary( self, op, args, proc );
}


DaoTypeCore daoSetCore =
{
	"Set<@T>",                                             /* name */
	sizeof(DaoSet),                                        /* size */
	{ NULL },                                              /* bases */
	NULL,                                                  /* numbers */
	daoSetMeths,                                           /* methods */
	DaoCstruct_CheckGetField,    DaoCstruct_DoGetField,    /* GetField */
	DaoCstruct_CheckSetField,    DaoCstruct_DoSetField,    /* SetField */
	DaoCstruct_CheckGetItem,     DaoCstruct_DoGetItem,     /* GetItem */
	DaoCstruct_CheckSetItem,     DaoCstruct_DoSetItem,     /* SetItem */
	DaoSet_CheckUnary,           DaoSet_DoUnary,           /* Unary */
	DaoSet_CheckBinary,          DaoSet_DoBinary,          /* Binary */
	DaoCstruct_CheckConversion,  DaoCstruct_DoConversion,  /* Conversion */
	NULL,                        NULL,                     /* ForEach */
	NULL,                                                  /* Print */
	NULL,                                                  /* Slice */
	NULL,                                                  /* Compare */
	NULL,                                                  /* Hash */
	NULL,                                                  /* Create */
	NULL,                                                  /* Copy */
	(DaoDeleteFunction) DaoSet_Delete,                     /* Delete */
	DaoSet_HandleGC                                        /* HandleGC */
};


DAO_DLL int DaoSet_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *stdns = DaoVmSpace_GetNamespace( vmSpace, "std" );
	daox_type_set = DaoNamespace_WrapType( stdns, & daoSetCore, DAO_CSTRUCT, 0 );
	return 0;
}
