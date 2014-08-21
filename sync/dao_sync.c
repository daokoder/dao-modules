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

// 2011-01: Danilov Aleksey, implementation of state and queue types.

#include"dao_sync.h"


#ifdef DAO_WITH_THREAD

static DaoType *dao_type_mutex   = NULL;
static DaoType *dao_type_condvar = NULL;
static DaoType *dao_type_sema    = NULL;
static DaoType *daox_type_DaoState = NULL;
static DaoType *daox_type_DaoQueue = NULL;
static DaoType *daox_type_DaoGuard = NULL;


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
	DaoProcess_PutEnum( proc, DaoMutex_TryLock( self )? "true" : "false" );
}
static void DaoMutex_Lib_Protect( DaoProcess *proc, DaoValue *p[], int n )
{
	DaoMutex *self = (DaoMutex*) p[0];
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 0 );
	if( sect == NULL ) return;
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
	{ DaoMutex_Lib_TryLock,   "trylock( self : mutex )=>bool" },
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
	DaoProcess_PutEnum( proc,
			DCondVar_TimedWait( & self->myCondVar, & mutex->myMutex, par[2]->xFloat.value )? "false" : "true" );
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
	{ DaoCondV_Lib_TimedWait, "timedwait( self : condition, mtx : mutex, seconds :float )=>bool" },
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
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 0 );
	if( sect == NULL ) return;
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
static void DaoState_GetGCFields( void *p, DList *values, DList *arrays, DList *maps, int remove )
{
	DaoState *self = (DaoState*)p;
	if( self->state ){
		DList_Append( values, self->state );
		if( remove ) self->state = NULL;
	}
}

extern DaoTypeBase stateTyper;

static void DaoState_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoType *type = DaoProcess_GetReturnType( proc );
	DaoState *res = DaoState_New( type, p[0] );
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
	DaoProcess_PutEnum( proc, set? "true" : "false" );
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
	DaoProcess_PutEnum( proc, res? "true" : "false" );
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
	/*! Constructs state object containing the given \a value */
	{ DaoState_Create,   "state<@T>( value: @T )" },

	/*! Reads the value and returns it */
	{ DaoState_Value,    "value( self: state<@T> ) => @T" },

	/*! Set the value to \a value and returns the old value */
	{ DaoState_Set,	     "set( self: state<@T>, value: @T ) => @T" },

	/*! If the current value is equal to \a from, sets the value to \a into and returns \c true. Otherwise the value is not changed
	 * and \c false is returned */
	{ DaoState_TestSet,  "alter( self: state<@T>, from: @T, into: @T ) => bool" },

	/*! Adds the give \a value to the current value */
	{ DaoState_FetchAdd, "add( self: state<@T<int|float|double|complex>>, value: @T ) => @T" },

	/*! Substitutes the given \a value from the current value */
	{ DaoState_FetchSub, "sub( self: state<@T<int|float|double|complex>>, value: @T ) => @T" },

	/*! Blocks the current thread until the specified \a value is set, or until the end of \a timeout given in seconds (if \a timeout is positive)
	 * Returns \c true if not timed out */
	{ DaoState_WaitFor,  "wait( self: state<@T>, value: @T, timeout: float = -1 ) => bool" },

	/*! Returns the list of all values currently awaited from the state by all threads */
	{ DaoState_Waitlist, "waitlist( self: state<@T> ) => list<@T>" },
	{ NULL, NULL }
};

/*! Represents state of an object or process in multithreaded environment. Uses the semantics of atomic operations to concurrently access and modify
 * the underlying data. Provides the ability to wait until a specific value is set by another thread, abstracting over conditional variables */
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
	res->joinvar = DaoCondVar_New();
	DaoGC_IncRC( (DaoValue*)res->mtx );
	DaoGC_IncRC( (DaoValue*)res->pushvar );
	DaoGC_IncRC( (DaoValue*)res->popvar );
	DaoGC_IncRC( (DaoValue*)res->joinvar );
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
	DaoGC_DecRC( (DaoValue*)self->joinvar );
	DaoCstruct_Free( (DaoCstruct*)self );
	dao_free( self );
}

static void DaoQueue_GetGCFields( void *p, DList *values, DList *arrays, DList *maps, int remove )
{
	DaoQueue *self = (DaoQueue*)p;
	while( self->tail != NULL ){
		QueueItem *item = self->tail;
		self->tail = item->previous;
		if( item->value ){
			DList_Append( values, item->value );
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
		if ( other->size )
			DaoCondVar_BroadCast( other->joinvar );
		other->size = 0;
		other->head = other->tail = NULL;
		merged = 1;
	}
	DaoMutex_Unlock( self->mtx );
	DaoMutex_Unlock( other->mtx );
	if( !merged )
		DaoProcess_RaiseError( proc, NULL, "Merging exceeds the queue capacity" );
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
	DaoProcess_PutEnum( proc, pushable? "true" : "false" );
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
	if ( self->size == 1 )
		DaoCondVar_BroadCast( self->joinvar );
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
		if ( self->size == 1 )
			DaoCondVar_BroadCast( self->joinvar );
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

static void DaoQueue_Join( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoQueue *self = (DaoQueue*)DaoValue_CastCstruct( p[0], NULL );
	DaoMutex_Lock( self->mtx );
	while ( self->size )
		DaoCondVar_Wait( self->joinvar, self->mtx );
	DaoMutex_Unlock( self->mtx );
}

static DaoFuncItem queueMeths[] =
{
	/*! Constructs the queue given the maximum \a capacity */
	{ DaoQueue_Create,   "queue<@T>( capacity = 0 )" },

	/*! Returns queue size */
	{ DaoQueue_Size,     "size( self: queue<@T> ) => int" },

	/*! Returns queue capacity */
	{ DaoQueue_Capacity, "capacity( self: queue<@T> ) => int" },

	/*! Pushes \a value to the queue, blocks if queue size equals its capacity */
	{ DaoQueue_Push,     "push( self: queue<@T>, value: @T )" },

	/*! Tries to push \a value to the queue within the given \a timeout interval (in case of negative value, waits indefinitely).
	 * Returns \c true if \a value was successfully pushed */
	{ DaoQueue_TryPush,  "trypush( self: queue<@T>, value: @T, timeout: float = 0 ) => bool" },

	/*! Pops \a value from the queue, blocks if queue size is zero */
	{ DaoQueue_Pop,      "pop( self: queue<@T> ) => @T" },

	/*! Tries to pop \a value from the queue within the given \a timeout interval (in case of negative value, waits indefinitely). On success,
	 * returns the popped value */
	{ DaoQueue_TryPop,   "trypop( self: queue<@T>, timeout: float = 0 ) => @T|none" },

	/*! Moves all elements of \a other to this queue, leaving \a other empty */
	{ DaoQueue_Merge,    "merge( self: queue<@T>, other: queue<@T> )" },

	/*! Blocks until the queue is emptied */
	{ DaoQueue_Join,     "wait_empty( self: queue<@T> )" },
	{ NULL, NULL }
};

/*! Synchronized queue. Unlike mt::channel, does not deep-copies the data and has no constraints on the type of the elements */
DaoTypeBase queueTyper = {
	"queue<@T>", NULL, NULL, queueMeths, {NULL}, {0},
	(FuncPtrDel)DaoQueue_Delete, DaoQueue_GetGCFields
};

DaoGuard* DaoGuard_New( DaoType *type, DaoValue *value )
{
	DaoGuard *res = dao_malloc( sizeof(DaoGuard) );
	DaoCstruct_Init( (DaoCstruct*)res, type );
	res->value = NULL;
	DaoValue_Copy( value, &res->value );
	res->read = res->write = 0;
	res->lock = DaoMutex_New();
	res->writevar = DaoCondVar_New();
	DaoGC_IncRC( (DaoValue*)res->lock );
	DaoGC_IncRC( (DaoValue*)res->writevar );
	return res;
}

void DaoGuard_Delete( DaoGuard *self )
{
	DaoGC_DecRC( self->value );
	DaoGC_DecRC( (DaoValue*)self->lock );
	DaoGC_DecRC( (DaoValue*)self->writevar );
	DaoCstruct_Free( (DaoCstruct*)self );
	dao_free( self );
}
static void DaoGuard_GetGCFields( void *p, DList *values, DList *arrays, DList *maps, int remove )
{
	DaoGuard *self = (DaoGuard*)p;
	if ( self->value ){
		DList_Append( values, self->value );
		if ( remove ) self->value = NULL;
	}
}

extern DaoTypeBase guardTyper;

static void DaoGuard_Create( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoType *type = DaoProcess_GetReturnType( proc );
	DaoGuard *res = DaoGuard_New( type, p[0] );
	DaoProcess_PutValue( proc, (DaoValue*)res );
}

DaoValue* DaoGuard_ExecSection( DaoGuard *self, DaoProcess *proc )
{
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 1 );
	DaoValue *res = NULL;
	if ( sect ){
		daoint entry = proc->topFrame->entry;
		if ( sect->b > 0 ) DaoProcess_SetValue( proc, sect->a, self->value );
		proc->topFrame->entry = entry;
		if ( DaoProcess_Execute( proc ) )
			res = proc->stackValues[0];
		DaoProcess_PopFrame( proc );
		DaoProcess_SetActiveFrame( proc, proc->topFrame );
	}
	return res;
}

static void DaoGuard_Peek( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoGuard *self = (DaoGuard*)DaoValue_CastCstruct( p[0], NULL );
	DaoValue *res;
	DaoMutex_Lock( self->lock );
	self->read++;
	DaoMutex_Unlock( self->lock );
	res = DaoGuard_ExecSection( self, proc );
	DaoMutex_Lock( self->lock );
	if ( --self->read == 0 && self->write )
		DaoCondVar_BroadCast( self->writevar );
	DaoMutex_Unlock( self->lock );
	if ( res )
		DaoProcess_PutValue( proc, res );
}

static void DaoGuard_Acquire( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoGuard *self = (DaoGuard*)DaoValue_CastCstruct( p[0], NULL );
	DaoValue *res;
	DaoMutex_Lock( self->lock );
	self->write++;
	while ( self->read )
		DaoCondVar_Wait( self->writevar, self->lock );
	res = DaoGuard_ExecSection( self, proc );
	if ( res && res->type != DAO_NONE )
		DaoValue_Copy( res, &self->value );
	self->write--;
	DaoMutex_Unlock( self->lock );
}

static DaoFuncItem guardMeths[] =
{
	/*! Constructs guard object with the given \a value */
	{ DaoGuard_Create,	"guard<@T>(value: @T)" },

	/*! Grants non-exclusive read-only access to the guarded \a value. Returns the result of the code section.
	 * \note Multiple tasks are allowed to concurrently peek the data */
	{ DaoGuard_Peek,	"peek(invar self: guard<@T>)[value: invar<@T> => @V] => @V" },

	/*! Grants exclusive read/write access to the guarded \a value. Sets the value to the result of the code section if it is not \c none.
	 * \note Only a single task may acquire the data; no other task is allowed to peek the data in the meantime */
	{ DaoGuard_Acquire,	"acquire(self: guard<@T>)[value: @T => @T|none]" },
	{ DaoGuard_Acquire,	"acquire(self: guard<invar<@T>>)[value: invar<@T>]" },
	{ NULL, NULL }
};

DaoTypeBase guardTyper = {
	"guard<@T>", NULL, NULL, guardMeths, {NULL}, {0},
	(FuncPtrDel)DaoGuard_Delete, DaoGuard_GetGCFields
};

DAO_DLL int DaoSync_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *syncns = DaoNamespace_GetNamespace( ns, "sync" );
	DaoNamespace_AddConstValue( ns, "sync", (DaoValue*)syncns );
	dao_type_mutex   = DaoNamespace_WrapType( syncns, & mutexTyper, 0 );
	dao_type_condvar = DaoNamespace_WrapType( syncns, & condvTyper, 0 );
	dao_type_sema    = DaoNamespace_WrapType( syncns, & semaTyper, 0 );
	daox_type_DaoState = DaoNamespace_WrapType( syncns, &stateTyper, 0 );
	daox_type_DaoQueue = DaoNamespace_WrapType( syncns, &queueTyper, 0 );
	daox_type_DaoGuard = DaoNamespace_WrapType( syncns, &guardTyper, 0 );
	return 0;
}

#else

DAO_DLL int DaoSync_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	return 1;
}

#endif
