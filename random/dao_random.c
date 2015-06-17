/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2015, Limin Fu
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
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
*/

#include<time.h>
#include"dao_random.h"


void DaoxRandMT_Seed( DaoxRandMT *self, uint_t seed )
{
	int i;
	if( seed == 0 ) seed = time(NULL);
	self->index = 0;
	self->states[0] = seed;
	for(i=1; i<DAO_MTCOUNT; ++i){
		uint_t prev = self->states[i-1];
		self->states[i] = 0x6c078965 * (prev ^ (prev>>30)) + i;
	}
}

void DaoxRandMT_Generate( DaoxRandMT *self )
{
	uint_t i, *mtnums = self->states;
	for(i=1; i<DAO_MTCOUNT; ++i){
		uint_t y = (mtnums[i] & 0x80000000) + (mtnums[(i+1)%DAO_MTCOUNT] & 0x7fffffff);
		mtnums[i] = mtnums[(i+397)%DAO_MTCOUNT] ^ (y >> 1);
		if( y % 2 ) mtnums[i] ^= 0x9908b0df;
	}
}

uint_t DaoxRandMT_Extract( DaoxRandMT *self )
{
	uint_t y;
	if( self->index == 0 ) DaoxRandMT_Generate( self );
	y = self->states[ self->index ];
	y ^= y>>11;
	y ^= (y<<7) & 0x9d2c5680;
	y ^= (y<<15) & 0xefc60000;
	y ^= y>>18;
	self->index = (self->index + 1) % DAO_MTCOUNT;
	return y;
}

double DaoxRandMT_Uniform( DaoxRandMT *self )
{
	return DaoxRandMT_Extract( self ) / (double) 0xffffffff;
}




DaoxUniformRand* DaoxUniformRand_New( uint_t seed )
{
	DaoxUniformRand *self = (DaoxUniformRand*) dao_calloc( 1, sizeof(DaoxUniformRand) );
	DaoCstruct_Init( (DaoCstruct*)self, daox_type_uniform_rand );
	DaoxRandMT_Seed( & self->randmt, seed );
	return self;
}

void DaoxUniformRand_Delete( DaoxUniformRand *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}

double DaoxUniformRand_Get( DaoxUniformRand *self )
{
	return DaoxRandMT_Uniform( & self->randmt );
}


DaoxNormalRand* DaoxNormalRand_New( uint_t seed )
{
	DaoxNormalRand *self = (DaoxNormalRand*) dao_calloc( 1, sizeof(DaoxNormalRand) );
	DaoCstruct_Init( (DaoCstruct*)self, daox_type_normal_rand );
	DaoxRandMT_Seed( & self->randmt, seed );
	self->gset = 0.0;
	self->iset = 0;
	return self;
}

void DaoxNormalRand_Delete( DaoxNormalRand *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}

double DaoxNormalRand_Get( DaoxNormalRand *self )
{
	float fac, rsq, v1, v2;

	if( self->iset ){
		self->iset = 0;
		return self->gset;
	}
	do {
		v1 = 2.0 * DaoxRandMT_Uniform( & self->randmt ) -1.0;
		v2 = 2.0 * DaoxRandMT_Uniform( & self->randmt ) -1.0;
		rsq = v1*v1 + v2*v2 ;
	} while( rsq >= 1.0 || rsq == 0.0 );
	fac = sqrt( -2.0 * log( rsq ) / rsq );
	self->gset = v1 * fac;
	self->iset = 1;
	return v2 * fac;
}


static void UNI_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxUniformRand *self = DaoxUniformRand_New( N ? p[0]->xInteger.value : time(NULL) );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void UNI_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxUniformRand *self = (DaoxUniformRand*) p[0];
	double max = p[1]->xFloat.value;
	DaoProcess_PutFloat( proc, max * DaoxUniformRand_Get( self ) );
}
static void UNI_Get2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxUniformRand *self = (DaoxUniformRand*) p[0];
	double min = p[1]->xFloat.value;
	double max = p[2]->xFloat.value;
	if( max < min ){
		DaoProcess_RaiseError( proc, "Param", "invalid range" );
		return;
	}
	DaoProcess_PutFloat( proc, min + (max - min) * DaoxUniformRand_Get( self ) );
}
static void UNI_GetInt( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxUniformRand *self = (DaoxUniformRand*) p[0];
	dao_integer max = p[1]->xInteger.value;
	double random = DaoxUniformRand_Get( self );
	if( max < 0 ){
		DaoProcess_PutInteger( proc, (max - 1) * random );
	}else{
		DaoProcess_PutInteger( proc, (max + 1) * random );
	}
}
static void UNI_GetInt2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxUniformRand *self = (DaoxUniformRand*) p[0];
	dao_integer min = p[1]->xInteger.value;
	dao_integer max = p[2]->xInteger.value;
	if( max < min ){
		DaoProcess_RaiseError( proc, "Param", "invalid range" );
		return;
	}
	DaoProcess_PutFloat( proc, min + (max - min + 1) * DaoxUniformRand_Get( self ) );
}

static DaoFuncItem DaoxUniformRandMeths[]=
{
	{ UNI_New,       "Uniform( seed = 0 )" },

	{ UNI_Get,       "Get( self: Uniform, max = 1.0 ) => float" },
	{ UNI_Get2,      "Get( self: Uniform, min: float, max: float ) => float" },

	{ UNI_GetInt,    "Get( self: Uniform, max: int ) => int" },
	{ UNI_GetInt2,   "Get( self: Uniform, min: int, max: int ) => float" },

	{ UNI_Get,       "()( self: Uniform, max = 1.0 ) => float" },
	{ UNI_GetInt,    "()( self: Uniform, max: int ) => int" },

	{ NULL, NULL }
};

DaoTypeBase DaoxUniformRand_Typer =
{
	"Uniform", NULL, NULL, (DaoFuncItem*) DaoxUniformRandMeths, { NULL }, { NULL },
	(FuncPtrDel)DaoxUniformRand_Delete, NULL
};



static void NORM_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNormalRand *self = DaoxNormalRand_New( N ? p[0]->xInteger.value : time(NULL) );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void NORM_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNormalRand *self = (DaoxNormalRand*) p[0];
	double stdev = p[1]->xFloat.value;
	DaoProcess_PutFloat( proc, stdev * DaoxNormalRand_Get( self ) );
}
static void NORM_Get2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNormalRand *self = (DaoxNormalRand*) p[0];
	double mean = p[1]->xFloat.value;
	double stdev = p[2]->xFloat.value;
	DaoProcess_PutFloat( proc, mean + stdev * DaoxNormalRand_Get( self ) );
}

static DaoFuncItem DaoxNormalRandMeths[]=
{
	{ NORM_New,       "Normal( seed = 0 )" },

	{ NORM_Get,       "Get( self: Normal, stdev = 1.0 ) => float" },
	{ NORM_Get2,      "Get( self: Normal, mean: float, stdev: float ) => float" },

	{ NULL, NULL }
};

DaoTypeBase DaoxNormalRand_Typer =
{
	"Normal", NULL, NULL, (DaoFuncItem*) DaoxNormalRandMeths, { NULL }, { NULL },
	(FuncPtrDel)DaoxNormalRand_Delete, NULL
};



static DaoxRandMT* DaoxRandMT_New( uint_t seed )
{
	DaoxRandMT *self = (DaoxRandMT*) dao_malloc( sizeof(DaoxRandMT) );
	DaoxRandMT_Seed( self, seed );
	return self;
}
static void DaoxRandMT_Delete( DaoxRandMT *self )
{
	dao_free( self );
}
static DaoxRandMT* DaoProcess_GetRandCache( DaoProcess *self, uint_t seeding )
{
	void *randgen = DaoProcess_GetAuxData( self, DaoxRandMT_Delete );
	if( randgen == NULL ){
		randgen = DaoxRandMT_New( seeding ? seeding : (uint_t)time(NULL) );
		DaoProcess_SetAuxData( self, DaoxRandMT_Delete, randgen );
	}
	return (DaoxRandMT*) randgen;
}

static void RAND_Swap( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxUniformRand *self = (DaoxUniformRand*) p[0];
	DaoxRandMT *randmt = DaoProcess_GetRandCache( proc, 0 );
	DaoxRandMT tmp = *randmt;
	*randmt = self->randmt;
	self->randmt = tmp;
}
static void RAND_SRand( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandMT *randmt = DaoProcess_GetRandCache( proc, 0 );
	uint_t seed = (uint_t)p[0]->xInteger.value;
	DaoxRandMT_Seed( randmt, seed );
}
static void RAND_Rand( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandMT *randmt = DaoProcess_GetRandCache( proc, 0 );
	double max = p[0]->xFloat.value;
	DaoProcess_PutFloat( proc, max * DaoxRandMT_Uniform( randmt ) );
}
static void RAND_Rand2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandMT *randmt = DaoProcess_GetRandCache( proc, 0 );
	double random = DaoxRandMT_Uniform( randmt );
	dao_integer max = p[0]->xInteger.value;
	if( max < 0 ){
		DaoProcess_PutInteger( proc, (max - 1) * random );
	}else{
		DaoProcess_PutInteger( proc, (max + 1) * random );
	}
}

/*
// The following global methods in the random module use a process-private
// buffer for random number generation.
//
// Note:
// The process-private buffer is cleared every the process is released
// back to the VM space by DaoVmSpace_ReleaseProcess(), in order to
// keep the state of the random number generator, the swap() method can be
// used to swap the buffer states between the process private buffer
// and the buffer of the parameter generator.
*/
static DaoFuncItem randomMeths[]=
{
	{ RAND_Swap,      "swap( randgen: Uniform )" },
	{ RAND_SRand,     "srand( seed: int )" },
	{ RAND_Rand,      "rand( max = 1.0 ) => float" },
	{ RAND_Rand2,     "rand( max: int ) => int" },
	{ NULL, NULL }
};


DaoType *daox_type_uniform_rand = NULL;
DaoType *daox_type_normal_rand = NULL;

DAO_DLL int DaoRandom_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *randomns = DaoNamespace_GetNamespace( ns, "random" );
	daox_type_uniform_rand = DaoNamespace_WrapType( randomns, & DaoxUniformRand_Typer, 0 );
	daox_type_normal_rand = DaoNamespace_WrapType( randomns, & DaoxNormalRand_Typer, 0 );
	DaoNamespace_WrapFunctions( randomns, randomMeths );
	return 0;
}
