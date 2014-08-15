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

// 2011-01: Danilov Aleksey, implementation of state and queue types.

#include"dao.h"
#include"daoStdtype.h"
#include"daoValue.h"
#include"daoThread.h"
#include"daoGC.h"

#ifdef DAO_WITH_THREAD


#ifdef UNIX
#define dao_sema_t     sem_t
#elif WIN32
#define dao_sema_t  HANDLE
#endif

typedef struct DSema       DSema;
typedef struct DaoMutex    DaoMutex;
typedef struct DaoCondVar  DaoCondVar;
typedef struct DaoSema     DaoSema;
typedef struct DaoState    DaoState;
typedef struct QueueItem   QueueItem;
typedef struct DaoQueue    DaoQueue;
typedef struct DaoGuard    DaoGuard;

struct DSema
{
	dao_sema_t  mySema;
	int         count;
};
DAO_DLL void DSema_Init( DSema *self, int n );
DAO_DLL void DSema_Destroy( DSema *self );
DAO_DLL void DSema_Wait( DSema *self );
DAO_DLL void DSema_Post( DSema *self );



/* Dao threading types: */
struct DaoMutex
{
	DAO_CSTRUCT_COMMON;

	DMutex  myMutex;
};

DAO_DLL DaoMutex* DaoMutex_New();
DAO_DLL void DaoMutex_Lock( DaoMutex *self );
DAO_DLL void DaoMutex_Unlock( DaoMutex *self );
DAO_DLL int DaoMutex_TryLock( DaoMutex *self );

struct DaoCondVar
{
	DAO_CSTRUCT_COMMON;

	DCondVar  myCondVar;
};

DAO_DLL DaoCondVar* DaoCondVar_New();
DAO_DLL void DaoCondVar_Delete( DaoCondVar *self );

DAO_DLL void DaoCondVar_Wait( DaoCondVar *self, DaoMutex *mutex );
DAO_DLL int  DaoCondVar_TimedWait( DaoCondVar *self, DaoMutex *mutex, double seconds );
/* return true if time out. */

DAO_DLL void DaoCondVar_Signal( DaoCondVar *self );
DAO_DLL void DaoCondVar_BroadCast( DaoCondVar *self );

struct DaoSema
{
	DAO_CSTRUCT_COMMON;

	DSema  mySema;
};

DAO_DLL DaoSema* DaoSema_New( int n );
DAO_DLL void DaoSema_Delete( DaoSema *self );

DAO_DLL void DaoSema_Wait( DaoSema *self );
DAO_DLL void DaoSema_Post( DaoSema *self );

DAO_DLL void DaoSema_SetValue( DaoSema *self, int n );
DAO_DLL int  DaoSema_GetValue( DaoSema *self );


struct DaoState
{
	DAO_CSTRUCT_COMMON;

	DaoValue *state;
	DaoMutex *lock;
	DaoMutex *defmtx;
	DaoMap *demands;
};



struct QueueItem
{
	DaoValue *value;
	struct QueueItem *next;
	struct QueueItem *previous;
};


struct DaoQueue
{
	DAO_CSTRUCT_COMMON;

	QueueItem *head;
	QueueItem *tail;
	volatile int size;
	int capacity;
	DaoMutex *mtx;
	DaoCondVar *pushvar;
	DaoCondVar *popvar;
	DaoCondVar *joinvar;
};

struct DaoGuard {
	DAO_CSTRUCT_COMMON;

	DaoValue *value;
	DaoMutex *lock;
	DaoCondVar *writevar;
	volatile int read;
	volatile int write;
};

#endif
