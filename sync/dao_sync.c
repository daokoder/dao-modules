/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2013, Limin Fu
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

#include"dao_sync.h"


#ifdef DAO_WITH_THREAD

static DaoType *dao_type_mutex   = NULL;
static DaoType *dao_type_condvar = NULL;
static DaoType *dao_type_sema    = NULL;
static DaoType *daox_type_DaoState = NULL;
static DaoType *daox_type_DaoQueue = NULL;


static void DSema_SetValue( DSema *self, int n );
static int  DSema_GetValue( DSema *self );

#ifdef UNIX
void DSema_Init( DSema *self, int n )
{
	sem_init( & self->mySema, 0, n );
}
void DSema_Destroy( DSema *self )
{
	sem_destroy( & self->mySema );
}
void DSema_Wait( DSema *self )
{
	sem_wait( & self->mySema );
}
void DSema_Post( DSema *self )
{
	sem_post( & self->mySema );
}

void DSema_SetValue( DSema *self, int n )
{
	sem_init( & self->mySema, 0, n );
}
int  DSema_GetValue( DSema *self )
{
	int n;
	sem_getvalue( & self->mySema, & n );
	return n;
}

#elif WIN32

void DSema_Init( DSema *self, int n )
{
	self->mySema = CreateSemaphore( NULL, n, n, NULL );
	self->count = n;
}
void DSema_Destroy( DSema *self )
{
	CloseHandle( self->mySema );
}
void DSema_Wait( DSema *self )
{
	WaitForSingleObject ( self->mySema, INFINITE );
	self->count --;
}
void DSema_Post( DSema *self )
{
	ReleaseSemaphore( self->mySema, 1, NULL );
	self->count ++;
}
int DSema_GetValue( DSema *self )
{
	return self->count;
}
void DSema_SetValue( DSema *self, int n )
{
	CloseHandle( self->mySema );
	self->mySema = CreateSemaphore( NULL, 0, n, NULL );
	self->count = n;
}
#endif





static int DaoMT_PushSectionFrame( DaoProcess *proc )
{
	if( DaoProcess_PushSectionFrame( proc ) == NULL ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "code section not found!" );
		return 0;
	}
	return 1;
}

static void DaoMutex_Lib_Mutex( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoMutex *mutex = DaoMutex_New();
	DaoProcess_PutValue( proc, (DaoValue*) mutex );
}
static void DaoMutex_Lib_Lock( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoMutex *self = (DaoMutex*) par[0];
	DaoMutex_Lock( self );
}
static void DaoMutex_Lib_Unlock( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoMutex *self = (DaoMutex*) par[0];
	DaoMutex_Unlock( self );
}
static void DaoMutex_Lib_TryLock( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoMutex *self = (DaoMutex*) par[0];
	DaoProcess_PutInteger( proc, DaoMutex_TryLock( self ) );
}
static void DaoMutex_Lib_Protect( DaoProcess *proc, DaoValue *p[], int n )
{
	DaoMutex *self = (DaoMutex*) p[0];
	DaoVmCode *sect = DaoGetSectionCode( proc->activeCode );
	if( sect == NULL || DaoMT_PushSectionFrame( proc ) == 0 ) return;
	DaoMutex_Lock( self );
	DaoProcess_Execute( proc );
	DaoMutex_Unlock( self );
	DaoProcess_PopFrame( proc );
}
static DaoFuncItem mutexMeths[] =
{
	{ DaoMutex_Lib_Mutex,     "mutex()=>mutex" },
	{ DaoMutex_Lib_Lock,      "lock( self : mutex )" },
	{ DaoMutex_Lib_Unlock,    "unlock( self : mutex )" },
	{ DaoMutex_Lib_TryLock,   "trylock( self : mutex )=>int" },
	{ DaoMutex_Lib_Protect,   "protect( self : mutex )[]" },
	{ NULL, NULL }
};
static void DaoMutex_Delete( DaoMutex *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DMutex_Destroy( & self->myMutex );
	dao_free( self );
}

DaoTypeBase mutexTyper =
{
	"mutex", NULL, NULL, (DaoFuncItem*) mutexMeths, {0}, {0},
	(FuncPtrDel) DaoMutex_Delete, NULL
};

DaoMutex* DaoMutex_New()
{
	DaoMutex* self = (DaoMutex*) dao_calloc( 1, sizeof(DaoMutex) );
	DaoCstruct_Init( (DaoCstruct*) self, dao_type_mutex );
	DMutex_Init( & self->myMutex );
	return self;
}
void DaoMutex_Lock( DaoMutex *self )
{
	DMutex_Lock( & self->myMutex );
}
void DaoMutex_Unlock( DaoMutex *self )
{
	DMutex_Unlock( & self->myMutex );
}
int DaoMutex_TryLock( DaoMutex *self )
{
	return DMutex_TryLock( & self->myMutex );
}
/* Condition variable */
static void DaoCondV_Lib_CondVar( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoProcess_PutValue( proc, (DaoValue*)DaoCondVar_New() );
}
static void DaoCondV_Lib_Wait( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoCondVar *self = (DaoCondVar*) par[0];
	DaoMutex *mutex = (DaoMutex*) par[1];
	DCondVar_Wait( & self->myCondVar, & mutex->myMutex );
}
static void DaoCondV_Lib_TimedWait( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoCondVar *self = (DaoCondVar*) par[0];
	DaoMutex *mutex = (DaoMutex*) par[1];
	DaoProcess_PutInteger( proc,
			DCondVar_TimedWait( & self->myCondVar, & mutex->myMutex, par[2]->xFloat.value ) );
}
static void DaoCondV_Lib_Signal( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoCondVar *self = (DaoCondVar*) par[0];
	DCondVar_Signal( & self->myCondVar );
}
static void DaoCondV_Lib_BroadCast( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoCondVar *self = (DaoCondVar*) par[0];
	DCondVar_BroadCast( & self->myCondVar );
}
static DaoFuncItem condvMeths[] =
{
	{ DaoCondV_Lib_CondVar,   "condition()=>condition" },
	{ DaoCondV_Lib_Wait,      "wait( self : condition, mtx : mutex )" },
	{ DaoCondV_Lib_TimedWait, "timedwait( self : condition, mtx : mutex, seconds :float )=>int" },
	{ DaoCondV_Lib_Signal,    "signal( self : condition )" },
	{ DaoCondV_Lib_BroadCast, "broadcast( self : condition )" },
	{ NULL, NULL }
};

DaoTypeBase condvTyper =
{
	"condition", NULL, NULL, (DaoFuncItem*) condvMeths, {0}, {0},
	(FuncPtrDel) DaoCondVar_Delete, NULL
};
DaoCondVar* DaoCondVar_New()
{
	DaoCondVar* self = (DaoCondVar*) dao_calloc( 1, sizeof(DaoCondVar) );
	DaoCstruct_Init( (DaoCstruct*) self, dao_type_condvar );
	DCondVar_Init( & self->myCondVar );
	return self;
}
void DaoCondVar_Delete( DaoCondVar *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DCondVar_Destroy( & self->myCondVar );
	dao_free( self );
}

void DaoCondVar_Wait( DaoCondVar *self, DaoMutex *mutex )
{
	DCondVar_Wait( & self->myCondVar, & mutex->myMutex );
}
int  DaoCondVar_TimedWait( DaoCondVar *self, DaoMutex *mutex, double seconds )
{
	return DCondVar_TimedWait( & self->myCondVar, & mutex->myMutex, seconds );
}

void DaoCondVar_Signal( DaoCondVar *self )
{
	DCondVar_Signal( & self->myCondVar );
}
void DaoCondVar_BroadCast( DaoCondVar *self )
{
	DCondVar_BroadCast( & self->myCondVar );
}
/* Semaphore */
static void DaoSema_Lib_Sema( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoProcess_PutValue( proc, (DaoValue*)DaoSema_New( par[0]->xInteger.value ) );
}
static void DaoSema_Lib_Wait( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoSema *self = (DaoSema*) par[0];
	DSema_Wait( & self->mySema );
}
static void DaoSema_Lib_Post( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoSema *self = (DaoSema*) par[0];
	DSema_Post( & self->mySema );
}
static void DaoSema_Lib_SetValue( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoSema *self = (DaoSema*) par[0];
	DSema_SetValue( & self->mySema, par[1]->xInteger.value );
}
static void DaoSema_Lib_GetValue( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoSema *self = (DaoSema*) par[0];
	DaoProcess_PutInteger( proc, DSema_GetValue( & self->mySema ) );
}
static void DaoSema_Lib_Protect( DaoProcess *proc, DaoValue *p[], int n )
{
	DaoSema *self = (DaoSema*) p[0];
	DaoVmCode *sect = DaoGetSectionCode( proc->activeCode );
	if( sect == NULL || DaoMT_PushSectionFrame( proc ) == 0 ) return;
	DSema_Wait( & self->mySema );
	DaoProcess_Execute( proc );
	DSema_Post( & self->mySema );
	DaoProcess_PopFrame( proc );
}
static DaoFuncItem semaMeths[] =
{
	{ DaoSema_Lib_Sema,      "semaphore( value = 0 )=>semaphore" },
	{ DaoSema_Lib_Wait,      "wait( self : semaphore )" },
	{ DaoSema_Lib_Post,      "post( self : semaphore )" },
	{ DaoSema_Lib_SetValue,  "setvalue( self : semaphore, n :int )" },
	{ DaoSema_Lib_GetValue,  "getvalue( self : semaphore )=>int" },
	{ DaoSema_Lib_Protect,   "protect( self : semaphore )[]" },
	{ NULL, NULL }
};
DaoTypeBase semaTyper =
{
	"semaphore", NULL, NULL, (DaoFuncItem*) semaMeths, {0}, {0},
	(FuncPtrDel) DaoSema_Delete, NULL
};
DaoSema* DaoSema_New( int n )
{
	DaoSema* self = (DaoSema*) dao_calloc( 1, sizeof(DaoSema) );
	DaoCstruct_Init( (DaoCstruct*) self, dao_type_sema );
	DSema_Init( & self->mySema, ( n < 0 )? 0 : n );
	return self;
}
void DaoSema_Delete( DaoSema *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DSema_Destroy( & self->mySema );
	dao_free( self );
}

void DaoSema_Wait( DaoSema *self )
{
	DSema_Wait( & self->mySema );
}
void DaoSema_Post( DaoSema *self )
{
	DSema_Post( & self->mySema );
}

void DaoSema_SetValue( DaoSema *self, int n )
{
	DSema_SetValue( & self->mySema, ( n < 0 )? 0 : n );
}
int  DaoSema_GetValue( DaoSema *self )
{
	return DSema_GetValue( & self->mySema );
}




extern DaoTypeBase stateTyper;

DaoState* DaoState_New( DaoType *type, DaoValue *state )
{
	DaoState *res = dao_malloc( sizeof(DaoState) );
	DaoCstruct_Init( (DaoCstruct*)res, type );
	res->state = 0;
	DaoValue_Copy( state, &res->state );
	res->lock = DaoMutex_New();
	res->defmtx = DaoMutex_New();
	res->demands = DaoMap_New( 0 );
	DaoGC_IncRC( (DaoValue*)res->lock );
	DaoGC_IncRC( (DaoValue*)res->defmtx );
	DaoGC_IncRC( (DaoValue*)res->demands );
	return res;
}

void DaoState_Delete( DaoState *self )
{
	DaoGC_DecRC( self->state );
	DaoGC_DecRC( (DaoValue*)self->lock );
	DaoGC_DecRC( (DaoValue*)self->defmtx );
	DaoGC_DecRC( (DaoValue*)self->demands );
	DaoCstruct_Free( (DaoCstruct*)self );
	dao_free( self );
}
static void DaoState_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	DaoState *self = (DaoState*)p;
	if( self->state ){
		DArray_Append( values, self->state );
		if( remove ) self->state = NULL;
	}
}

extern DaoTypeBase stateTyper;

static void DaoState_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoType *type = DaoProcess_GetReturnType( proc );
	DaoState *res;
	switch ( p[0]->type ){
	case DAO_INTEGER:
	case DAO_FLOAT:
	case DAO_DOUBLE:
	case DAO_COMPLEX:
	case DAO_ENUM:
		break;
	default:
		DaoProcess_RaiseException( proc, DAO_ERROR, "type not supported" );
		return;
	}
	res = DaoState_New( type, p[0] );
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

static void DaoState_Value( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoState *self = (DaoState*)DaoValue_CastCstruct( p[0], NULL );
	DaoMutex_Lock( self->lock );
	DaoProcess_PutValue( proc, self->state );
	DaoMutex_Unlock( self->lock );
}

static void DaoState_TestSet( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoState *self = (DaoState*)DaoValue_CastCstruct( p[0], NULL );
	int set = 0;
	DNode *node;
	DaoMutex_Lock( self->lock );
	if( !DaoValue_Compare( self->state, p[1] ) ){
		DaoValue_Copy( p[2], &self->state );
		set = 1;
		node = DaoMap_First( self->demands );
		while( node && DaoValue_Compare( DNode_Key( node ), self->state ) )
			node = DaoMap_Next( self->demands, node );
		if( node )
			DaoCondVar_BroadCast( (DaoCondVar*)DNode_Value( node ) );
	}
	DaoMutex_Unlock( self->lock );
	DaoProcess_PutInteger( proc, set );
}

static void DaoState_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoState *self = (DaoState*)DaoValue_CastCstruct( p[0], NULL );
	DNode *node;
	DaoValue *old = 0;
	DaoMutex_Lock( self->lock );
	DaoValue_Copy( self->state, &old );
	DaoValue_Copy( p[1], &self->state );
	node = DaoMap_First( self->demands );
	while( node && DaoValue_Compare( DNode_Key( node ), self->state ) )
		node = DaoMap_Next( self->demands, node );
	if( node ){
		DaoCondVar_BroadCast( (DaoCondVar*)DNode_Value( node ) );
		DaoMap_Erase( self->demands, DNode_Key( node ) );
	}
	DaoMutex_Unlock( self->lock );
	DaoProcess_PutValue( proc, old );
}

static void DaoState_FetchAdd( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoState *self = (DaoState*)DaoValue_CastCstruct( p[0], NULL );
	DNode *node;
	DaoValue *old = 0;
	DaoMutex_Lock( self->lock );
	if ( p[1]->type != self->state->type ){
		DaoMutex_Unlock( self->lock );
		DaoProcess_RaiseException( proc, DAO_ERROR, "types do not match" );
		return;
	}
	DaoValue_Copy( self->state, &old );
	switch ( self->state->type ){
	case DAO_INTEGER:
		self->state->xInteger.value += p[1]->xInteger.value;
		break;
	case DAO_FLOAT:
		self->state->xFloat.value += p[1]->xFloat.value;
		break;
	case DAO_DOUBLE:
		self->state->xDouble.value += p[1]->xDouble.value;
		break;
	case DAO_COMPLEX:
		self->state->xComplex.value.real += p[1]->xComplex.value.real;
		self->state->xComplex.value.imag += p[1]->xComplex.value.imag;
		break;
	case DAO_ENUM:
		DaoEnum_AddValue( &self->state->xEnum, &p[1]->xEnum, 0 ); //enames ignored?
		break;
	}
	node = DaoMap_First( self->demands );
	while( node && DaoValue_Compare( DNode_Key( node ), self->state ) )
		node = DaoMap_Next( self->demands, node );
	if( node ){
		DaoCondVar_BroadCast( (DaoCondVar*)DNode_Value( node ) );
		DaoMap_Erase( self->demands, DNode_Key( node ) );
	}
	DaoMutex_Unlock( self->lock );
	DaoProcess_PutValue( proc, old );
}

static void DaoState_FetchSub( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoState *self = (DaoState*)DaoValue_CastCstruct( p[0], NULL );
	DNode *node;
	DaoValue *old = 0;
	DaoMutex_Lock( self->lock );
	if ( p[1]->type != self->state->type ){
		DaoMutex_Unlock( self->lock );
		DaoProcess_RaiseException( proc, DAO_ERROR, "types do not match" );
		return;
	}
	DaoValue_Copy( self->state, &old );
	switch ( self->state->type ){
	case DAO_INTEGER:
		self->state->xInteger.value -= p[1]->xInteger.value;
		break;
	case DAO_FLOAT:
		self->state->xFloat.value -= p[1]->xFloat.value;
		break;
	case DAO_DOUBLE:
		self->state->xDouble.value -= p[1]->xDouble.value;
		break;
	case DAO_COMPLEX:
		self->state->xComplex.value.real -= p[1]->xComplex.value.real;
		self->state->xComplex.value.imag -= p[1]->xComplex.value.imag;
		break;
	case DAO_ENUM:
		DaoEnum_RemoveValue( &self->state->xEnum, &p[1]->xEnum, 0 ); //enames ignored?
		break;
	}
	node = DaoMap_First( self->demands );
	while( node && DaoValue_Compare( DNode_Key( node ), self->state ) )
		node = DaoMap_Next( self->demands, node );
	if( node ){
		DaoCondVar_BroadCast( (DaoCondVar*)DNode_Value( node ) );
		DaoMap_Erase( self->demands, DNode_Key( node ) );
	}
	DaoMutex_Unlock( self->lock );
	DaoProcess_PutValue( proc, old );
}

static void DaoState_WaitFor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoState *self = (DaoState*)DaoValue_CastCstruct( p[0], NULL );
	int eq = 0, res = 1;
	DaoValue *state = p[1];
	float timeout;
	DaoCondVar *condvar = NULL;
	DaoMutex_Lock( self->lock );
	if( !DaoValue_Compare( self->state, state ) )
		eq = 1;
	else{
		condvar = (DaoCondVar*)DaoMap_GetValue( self->demands, state );
		if( !condvar ){
			condvar = DaoCondVar_New();
			DaoMap_Insert( self->demands, state, (DaoValue*)condvar );
		}
	}
	DaoMutex_Unlock( self->lock );
	if( !eq ){
		DaoMutex_Lock( self->defmtx );
		timeout = p[2]->xFloat.value;
		if( timeout > 0 )
			do
				res = !DaoCondVar_TimedWait( condvar, self->defmtx, timeout );
			while( res && DaoValue_Compare( self->state, state ) );
		else if( timeout == 0 )
			res = 0;
		else
			do
				DaoCondVar_Wait( condvar, self->defmtx );
			while( DaoValue_Compare( self->state, state ) );
		DaoMutex_Unlock( self->defmtx );
	}
	DaoProcess_PutInteger( proc, res );
}

static void DaoState_Waitlist( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoState *self = (DaoState*)DaoValue_CastCstruct( p[0], NULL );
	DaoList *list = DaoProcess_PutList( proc );
	DNode *node;
	DaoMutex_Lock( self->lock );
	node = DaoMap_First( self->demands );
	while( node ){
		DaoList_PushBack( list, DNode_Key( node ) );
		node = DaoMap_Next( self->demands, node );
	}
	DaoMutex_Unlock( self->lock );
}

static DaoFuncItem stateMeths[] =
{
	{ DaoState_Create,   "state<@T>( value: @T )" },
	{ DaoState_Value,    "value( self: state<@T> ) => @T" },
	{ DaoState_Set,	     "set( self: state<@T>, value: @T ) => @T" },
	{ DaoState_TestSet,  "alter( self: state<@T>, from: @T, into: @T ) => int" },
	{ DaoState_FetchAdd, "add( self: state<@T>, value: @T ) => @T" },
	{ DaoState_FetchSub, "sub( self: state<@T>, value: @T ) => @T" },
	{ DaoState_WaitFor,  "wait( self: state<@T>, value: @T, timeout: float = -1 ) => int" },
	{ DaoState_Waitlist, "waitlist( self: state<@T> ) => list<@T>" },
	{ NULL, NULL }
};

DaoTypeBase stateTyper = {
	"state<@T>", NULL, NULL, stateMeths, {NULL}, {0},
	(FuncPtrDel)DaoState_Delete, DaoState_GetGCFields
};

extern DaoTypeBase queueTyper;

DaoQueue* DaoQueue_New( DaoType *type, int capacity )
{
	DaoQueue *res = (DaoQueue*)dao_malloc( sizeof(DaoQueue) );
	DaoCstruct_Init( (DaoCstruct*)res, type );
	res->head = res->tail = NULL;
	res->size = 0;
	res->capacity = ( ( capacity < 0 )? 0 : capacity );
	res->mtx = DaoMutex_New();
	res->pushvar = DaoCondVar_New();
	res->popvar = DaoCondVar_New();
	DaoGC_IncRC( (DaoValue*)res->mtx );
	DaoGC_IncRC( (DaoValue*)res->pushvar );
	DaoGC_IncRC( (DaoValue*)res->popvar );
	return res;
}

void DaoQueue_Delete( DaoQueue *self )
{
	QueueItem *item;
	while( self->tail != NULL ){
		item = self->tail;
		self->tail = item->previous;
		DaoGC_DecRC( item->value );
		dao_free( item );
	}
	DaoGC_DecRC( (DaoValue*)self->mtx );
	DaoGC_DecRC( (DaoValue*)self->pushvar );
	DaoGC_DecRC( (DaoValue*)self->popvar );
	DaoCstruct_Free( (DaoCstruct*)self );
	dao_free( self );
}

static void DaoQueue_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	DaoQueue *self = (DaoQueue*)p;
	while( self->tail != NULL ){
		QueueItem *item = self->tail;
		self->tail = item->previous;
		if( item->value ){
			DArray_Append( values, item->value );
			if( remove ) item->value = NULL;
		}
	}
}

static void DaoQueue_Size( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoQueue *self = (DaoQueue*)DaoValue_CastCstruct( p[0], NULL );
	int size;
	DaoMutex_Lock( self->mtx );
	size = self->size;
	DaoMutex_Unlock( self->mtx );
	DaoProcess_PutInteger( proc, size );
}

static void DaoQueue_Capacity( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoQueue *self = (DaoQueue*)DaoValue_CastCstruct( p[0], NULL );
	DaoProcess_PutInteger( proc, self->capacity );
}

static void DaoQueue_Merge( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoQueue *self = (DaoQueue*)DaoValue_CastCstruct( p[0], NULL );
	DaoQueue *other = (DaoQueue*)DaoValue_CastCstruct( p[1], NULL );
	int merged = 0;
	DaoMutex_Lock( self->mtx );
	DaoMutex_Lock( other->mtx );
	if( !self->capacity || self->size + other->size <= self->capacity ){
		if( self->size && other->size ){
			self->tail->next = other->head;
			other->head->previous = self->tail;
		}
		else if( !self->size ){
			self->head = other->head;
			self->tail = other->tail;
			DaoCondVar_BroadCast( self->popvar );
		}
		self->size += other->size;
		if( other->capacity && other->size == other->capacity )
			DaoCondVar_BroadCast( other->pushvar );
		other->size = 0;
		other->head = other->tail = NULL;
		merged = 1;
	}
	DaoMutex_Unlock( self->mtx );
	DaoMutex_Unlock( other->mtx );
	if( !merged )
		DaoProcess_RaiseException( proc, DAO_ERROR, "Merging exceeds the queue capacity" );
}

static void DaoQueue_Push( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoQueue *self = (DaoQueue*)DaoValue_CastCstruct( p[0], NULL );
	QueueItem *item = (QueueItem*)dao_malloc( sizeof(QueueItem) );
	item->value = NULL;
	DaoValue_Copy( p[1], &item->value );
	item->next = NULL;
	DaoMutex_Lock( self->mtx );
	while( self->capacity && self->size == self->capacity )
		DaoCondVar_Wait( self->pushvar, self->mtx );
	item->previous = self->tail;
	if( self->tail )
		self->tail->next = item;
	else{
		self->head = item;
		DaoCondVar_Signal( self->popvar );
	}
	self->tail = item;
	self->size++;
	DaoMutex_Unlock( self->mtx );
}

static void DaoQueue_TryPush( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoQueue *self = (DaoQueue*)DaoValue_CastCstruct( p[0], NULL );
	QueueItem *item = (QueueItem*)dao_malloc( sizeof(QueueItem) );
	float timeout = DaoValue_TryGetFloat( p[2] );
	int pushable = 0, timed = 0;
	item->value = NULL;
	DaoValue_Copy( p[1], &item->value );
	item->next = NULL;
	DaoMutex_Lock( self->mtx );
	if( timeout == 0 )
		pushable = ( !self->capacity || self->size < self->capacity );
	else if( timeout < 0 ){
		while( self->capacity && self->size == self->capacity )
			DaoCondVar_Wait( self->pushvar, self->mtx );
		pushable = 1;
	}
	else{
		while( !timed && self->capacity && self->size == self->capacity )
			timed = DaoCondVar_TimedWait( self->pushvar, self->mtx, timeout );
		pushable = !timed;
	}
	if( pushable ){
		item->previous = self->tail;
		if( self->tail )
			self->tail->next = item;
		else{
			self->head = item;
			DaoCondVar_Signal( self->popvar );
		}
		self->tail = item;
		self->size++;
	}
	DaoMutex_Unlock( self->mtx );
	if( !pushable ){
		DaoGC_DecRC( item->value );
		dao_free( item );
	}
	DaoProcess_PutInteger( proc, pushable );
}

static void DaoQueue_Pop( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoQueue *self = (DaoQueue*)DaoValue_CastCstruct( p[0], NULL );
	QueueItem *item = NULL;
	DaoMutex_Lock( self->mtx );
	while( !self->size )
		DaoCondVar_Wait( self->popvar, self->mtx );
	item = self->head;
	self->head = item->next;
	if( !self->head )
		self->tail = NULL;
	else
		self->head->previous = NULL;
	if( self->capacity && self->size == self->capacity )
		DaoCondVar_Signal( self->pushvar );
	self->size--;
	DaoMutex_Unlock( self->mtx );
	DaoProcess_PutValue( proc, item->value );
	DaoGC_DecRC( item->value );
	dao_free( item );
}

static void DaoQueue_TryPop( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoQueue *self = (DaoQueue*)DaoValue_CastCstruct( p[0], NULL );
	QueueItem *item = NULL;
	float timeout = DaoValue_TryGetFloat( p[1] );
	int popable = 0, timed = 0;
	DaoMutex_Lock( self->mtx );
	if( timeout == 0 )
		popable = self->size;
	else if( timeout < 0 ){
		while( !self->size )
			DaoCondVar_Wait( self->popvar, self->mtx );
		popable = 1;
	}
	else{
		while( !timed && !self->size )
			timed = DaoCondVar_TimedWait( self->popvar, self->mtx, timeout );
		popable = !timed;
	}
	if( popable ){
		item = self->head;
		self->head = item->next;
		if( !self->head )
			self->tail = NULL;
		else
			self->head->previous = NULL;
		if( self->capacity && self->size == self->capacity )
			DaoCondVar_Signal( self->pushvar );
		self->size--;
	}
	DaoMutex_Unlock( self->mtx );
	DaoProcess_PutValue( proc, item? item->value : dao_none_value );
	if( item ){
		DaoGC_DecRC( item->value );
		dao_free( item );
	}
}

static void DaoQueue_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoType *type = DaoProcess_GetReturnType( proc );
	DaoQueue *res = DaoQueue_New( type, DaoValue_TryGetInteger( p[0] ) );
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

static DaoFuncItem queueMeths[] =
{
	{ DaoQueue_Create,   "queue<@T>( capacity = 0 )" },
	{ DaoQueue_Size,     "size( self: queue<@T> ) => int" },
	{ DaoQueue_Capacity, "capacity( self: queue<@T> ) => int" },
	{ DaoQueue_Push,     "push( self: queue<@T>, value: @T )" },
	{ DaoQueue_TryPush,  "trypush( self: queue<@T>, value: @T, timeout: float = 0 ) => int" },
	{ DaoQueue_Pop,      "pop( self: queue<@T> ) => @T" },
	{ DaoQueue_TryPop,   "trypop( self: queue<@T>, timeout: float = 0 ) => @T|none" },
	{ DaoQueue_Merge,    "merge( self: queue<@T>, other: queue<@T> )" },
	{ NULL, NULL }
};

DaoTypeBase queueTyper = {
	"queue<@T>", NULL, NULL, queueMeths, {NULL}, {0},
	(FuncPtrDel)DaoQueue_Delete, DaoQueue_GetGCFields
};

DAO_DLL int DaoSync_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	dao_type_mutex   = DaoNamespace_WrapType( ns, & mutexTyper, 0 );
	dao_type_condvar = DaoNamespace_WrapType( ns, & condvTyper, 0 );
	dao_type_sema    = DaoNamespace_WrapType( ns, & semaTyper, 0 );
	daox_type_DaoState = DaoNamespace_WrapType( ns, &stateTyper, 0 );
	daox_type_DaoQueue = DaoNamespace_WrapType( ns, &queueTyper, 0 );
	return 0;
}

#else

DAO_DLL int DaoSync_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	return 1;
}

#endif
