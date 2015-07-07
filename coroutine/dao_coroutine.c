/*
// Dao Coroutine Module
// http://www.daovm.net
//
// Copyright (c) 2013, Limin Fu
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "daoValue.h"
#include "daoGC.h"


typedef struct DaoxCoroutine DaoxCoroutine;


struct DaoxCoroutine
{
	DAO_CSTRUCT_COMMON;

	DaoProcess  *process;
};

DaoType *daox_type_coroutine = NULL;

DaoxCoroutine* DaoxCoroutine_New( DaoType *type )
{
	DaoxCoroutine *self = (DaoxCoroutine*) dao_calloc( 1, sizeof(DaoxCoroutine) );
	DaoCstruct_Init( (DaoCstruct*) self, type );
	self->process = NULL;
	return self;
}
void DaoxCoroutine_Delete( DaoxCoroutine *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	GC_DecRC( self->process );
	dao_free( self );
}




int DaoProcess_Resume( DaoProcess *self, DaoValue *par[], int N, DaoProcess *ret )
{
	DaoType *tp;
	DaoVmCode *vmc;
	DaoTuple *tuple;
	if( self->status != DAO_PROCESS_SUSPENDED ) return 0;
	if( self->activeCode && self->activeCode->code == DVM_MCALL ){
		tp = self->activeTypes[ self->activeCode->c ];
		if( N == 1 ){
			DaoProcess_PutValue( self, par[0] );
		}else if( N ){ /* TODO */
			tuple = DaoTuple_New( N );
			tuple->ctype = tp;
			GC_IncRC( tuple->ctype );
			DaoProcess_MakeTuple( self, tuple, par, N );
			DaoProcess_PutValue( self, (DaoValue*) tuple );
		}
	}else if( N ){ /* TODO */
		DaoRoutine *rout = self->topFrame->routine;
		self->paramValues = self->stackValues + self->topFrame->stackBase;
		if( rout ) rout = DaoProcess_PassParams( self, rout, NULL, NULL, par, NULL, N, DVM_CALL );
		self->paramValues = self->stackValues + 1;
		if( rout == NULL ){
			DaoProcess_RaiseError( ret, NULL, "invalid parameters." );
			return 0;
		}
	}
	DaoProcess_Start( self );
	DaoProcess_PutValue( ret, self->stackValues[0] );
	return 1;
}




static void COROUT_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoType *retype = DaoProcess_GetReturnType( proc );
	DaoxCoroutine *self = DaoxCoroutine_New( retype );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void COROUT_Start( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoxCoroutine *self = (DaoxCoroutine*) par[0];
	DaoValue *params[DAO_MAX_PARAM];
	DaoValue *val = par[1];
	DaoProcess *vmProc;
	DaoRoutine *rout;
	int i, passed = 0;
	if( val == NULL || val->type != DAO_ROUTINE ){
		DaoProcess_RaiseError( proc, "Type", NULL );
		return;
	}
	params[0] = par[0];
	memcpy( params + 1, par + 2, (N-2)*sizeof(DaoValue*) );
	rout = DaoRoutine_Resolve( (DaoRoutine*)val, NULL, NULL, params, NULL, N-1, DVM_CALL );
	if( rout ) rout = DaoProcess_PassParams( proc, rout, NULL, NULL, params, NULL, N-1, DVM_CALL );
	if( rout == NULL || rout->body == NULL ){
		DaoProcess_RaiseError( proc, "Param", "not matched" );
		return;
	}
	if( self->process == NULL ){
		self->process = DaoProcess_New( proc->vmSpace );
		GC_IncRC( self->process );
	}
	vmProc = self->process;
	DaoProcess_PushRoutine( vmProc, rout, NULL );
	vmProc->activeValues = vmProc->stackValues + vmProc->topFrame->stackBase;
	for(i=0; i<rout->parCount; i++){
		vmProc->activeValues[i] = proc->paramValues[i];
		GC_IncRC( vmProc->activeValues[i] );
	}
	vmProc->status = DAO_PROCESS_SUSPENDED;
	vmProc->pauseType = DAO_PAUSE_COROUTINE_YIELD;
	DaoProcess_Start( vmProc );
	DaoProcess_PutValue( proc, vmProc->stackValues[0] );
	if( vmProc->status == DAO_PROCESS_ABORTED )
		DaoProcess_RaiseError( proc, NULL, "coroutine execution is aborted." );
}
static void COROUT_Resume( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCoroutine *self = (DaoxCoroutine*) p[0];
	DaoProcess *sp = self->process;
	if( self->process == proc ){
		DaoProcess_RaiseError( proc, NULL, "coroutine can only resume in alien process." );
		return;
	}
	if( sp->status != DAO_PROCESS_SUSPENDED || sp->pauseType != DAO_PAUSE_COROUTINE_YIELD ){
		DaoProcess_RaiseError( proc, NULL, "coroutine cannot be resumed." );
		return;
	}
	DaoProcess_Resume( self->process, p+1, N-1, proc );
	if( sp->status == DAO_PROCESS_SUSPENDED && sp->pauseType != DAO_PAUSE_COROUTINE_YIELD ){
		DaoProcess_RaiseError( proc, NULL, "coroutine is not suspended properly." );
		return;
	}
	if( self->process->status == DAO_PROCESS_ABORTED )
		DaoProcess_RaiseError( proc, NULL, "coroutine execution is aborted." );
}
static void COROUT_Suspend( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCoroutine *self = (DaoxCoroutine*) p[0];
	DaoValue *value = N > 1 ? p[1] : DaoValue_MakeNone();
	if( self->process != proc ){
		DaoProcess_RaiseError( proc, NULL, "coroutine cannot suspend in alien process." );
		return;
	}
	GC_Assign( & proc->stackValues[0], value );
	proc->status = DAO_PROCESS_SUSPENDED;
	proc->pauseType = DAO_PAUSE_COROUTINE_YIELD;
}
static void COROUT_Status( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCoroutine *self = (DaoxCoroutine*) p[0];
	const char *status = "running";
	switch( self->process->status ){
	case DAO_PROCESS_SUSPENDED : status ="suspended"; break;
	case DAO_PROCESS_RUNNING :   status ="running";   break;
	case DAO_PROCESS_ABORTED :   status ="aborted";   break;
	case DAO_PROCESS_FINISHED :  status ="finished";  break;
	default : break;
	}
	DaoProcess_PutEnum( proc, status );
}


static DaoFuncItem coroutineMeths[]=
{
	{ COROUT_New,    "Coroutine<@RESUME,@SUSPEND>()" },
	{ COROUT_Start,  "start( self: Coroutine<@RESUME,@SUSPEND>, rout: routine, ... ) => @SUSPEND" },
	{ COROUT_Resume, "resume( self: Coroutine<@RESUME,@SUSPEND> ) => @SUSPEND" },
	{ COROUT_Resume, "resume( self: Coroutine<@RESUME,@SUSPEND>, value: @RESUME ) => @SUSPEND" },
	{ COROUT_Suspend, "suspend( self: Coroutine<@RESUME,@SUSPEND> ) => @RESUME" },
	{ COROUT_Suspend, "suspend( self: Coroutine<@RESUME,@SUSPEND>, value: @SUSPEND ) => @RESUME" },
	{ COROUT_Status, "status( self: Coroutine<@RESUME,@SUSPEND> ) => enum<running,suspended,finished,aborted>" },
	{ NULL, NULL },
};

static void DaoxCoroutine_GC( void *p, DList *values, DList *as, DList *maps, int remove )
{
	DaoxCoroutine *self = (DaoxCoroutine*) p;
	DList_Append( values, self->process );
	if( remove ) self->process = NULL;
}

DaoTypeBase coroutineTyper =
{
	"Coroutine<@RESUME,@SUSPEND>", NULL, NULL, (DaoFuncItem*) coroutineMeths, {0}, {0},
	(FuncPtrDel)DaoxCoroutine_Delete, DaoxCoroutine_GC
};


DAO_DLL int DaoCoroutine_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_coroutine = DaoNamespace_WrapType( ns, & coroutineTyper, 0 );
	return 0;
}
