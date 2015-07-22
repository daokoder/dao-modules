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
*/

#include<time.h>
#include"daoValue.h"
#include"dao_random.h"

#define DAO_MTCOUNT 624

typedef struct DaoxRandGenWrapper DaoxRandGenWrapper;

struct DaoxRandGenerator
{
	uint_t  states[DAO_MTCOUNT];
	uint_t  index;
	uint_t  iset;
	float   gset;
};


DaoxRandGenerator* DaoxRandGenerator_New( uint_t seed )
{
	DaoxRandGenerator *self = (DaoxRandGenerator*) dao_malloc( sizeof(DaoxRandGenerator) );
	DaoxRandGenerator_Seed( self, seed );
	return self;
}
void DaoxRandGenerator_Delete( DaoxRandGenerator *self )
{
	dao_free( self );
}
void DaoxRandGenerator_Seed( DaoxRandGenerator *self, uint_t seed )
{
	int i;
	if( seed == 0 ) seed = time(NULL);
	self->iset = 0;
	self->gset = 0.0;
	self->index = 0;
	self->states[0] = seed;
	for(i=1; i<DAO_MTCOUNT; ++i){
		uint_t prev = self->states[i-1];
		self->states[i] = 0x6c078965 * (prev ^ (prev>>30)) + i;
	}
}
static void DaoxRandGenerator_GenerateMT( DaoxRandGenerator *self )
{
	uint_t i, *mtnums = self->states;
	for(i=1; i<DAO_MTCOUNT; ++i){
		uint_t y = (mtnums[i] & 0x80000000) + (mtnums[(i+1)%DAO_MTCOUNT] & 0x7fffffff);
		mtnums[i] = mtnums[(i+397)%DAO_MTCOUNT] ^ (y >> 1);
		if( y % 2 ) mtnums[i] ^= 0x9908b0df;
	}
}
static uint_t DaoxRandGenerator_ExtractMT( DaoxRandGenerator *self )
{
	uint_t y;
	if( self->index == 0 ) DaoxRandGenerator_GenerateMT( self );
	y = self->states[ self->index ];
	y ^= y>>11;
	y ^= (y<<7) & 0x9d2c5680;
	y ^= (y<<15) & 0xefc60000;
	y ^= y>>18;
	self->index = (self->index + 1) % DAO_MTCOUNT;
	return y;
}
double DaoxRandGenerator_GetUniform( DaoxRandGenerator *self )
{
	return DaoxRandGenerator_ExtractMT( self ) / (double) 0xffffffff;
}
double DaoxRandGenerator_GetNormal( DaoxRandGenerator *self )
{
	float fac, rsq, v1, v2;

	if( self->iset ){
		self->iset = 0;
		return self->gset;
	}
	do {
		v1 = 2.0 * DaoxRandGenerator_GetUniform( self ) -1.0;
		v2 = 2.0 * DaoxRandGenerator_GetUniform( self ) -1.0;
		rsq = v1*v1 + v2*v2 ;
	} while( rsq >= 1.0 || rsq == 0.0 );
	fac = sqrt( -2.0 * log( rsq ) / rsq );
	self->gset = v1 * fac;
	self->iset = 1;
	return v2 * fac;
}




struct DaoxRandGenWrapper
{
	DAO_CSTRUCT_COMMON;

	DaoxRandGenerator generator;
};
DAO_DLL DaoType *daox_type_rand_generator;



DaoxRandGenWrapper* DaoxRandGenWrapper_New( uint_t seed )
{
	DaoxRandGenWrapper *self = (DaoxRandGenWrapper*) dao_calloc(1, sizeof(DaoxRandGenWrapper));
	DaoCstruct_Init( (DaoCstruct*)self, daox_type_rand_generator );
	DaoxRandGenerator_Seed( & self->generator, seed );
	return self;
}

void DaoxRandGenWrapper_Delete( DaoxRandGenWrapper *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}



static void GEN_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenWrapper *self = DaoxRandGenWrapper_New( N ? p[0]->xInteger.value : time(NULL) );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void GEN_GetUniform( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenWrapper *self = (DaoxRandGenWrapper*) p[0];
	double random = DaoxRandGenerator_GetUniform( & self->generator );
	double max = p[1]->xFloat.value;
	DaoProcess_PutFloat( proc, max * random );
}
static void GEN_GetUniform2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenWrapper *self = (DaoxRandGenWrapper*) p[0];
	double random = DaoxRandGenerator_GetUniform( & self->generator );
	double min = p[1]->xFloat.value;
	double max = p[2]->xFloat.value;
	if( max < min ){
		DaoProcess_RaiseError( proc, "Param", "invalid range" );
		return;
	}
	DaoProcess_PutFloat( proc, min + (max - min) * random );
}
static void GEN_GetUniformInt( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenWrapper *self = (DaoxRandGenWrapper*) p[0];
	double random = DaoxRandGenerator_GetUniform( & self->generator );
	dao_integer max = p[1]->xInteger.value;
	if( max < 0 ){
		DaoProcess_PutInteger( proc, (max - 1) * random );
	}else{
		DaoProcess_PutInteger( proc, (max + 1) * random );
	}
}
static void GEN_GetUniformInt2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenWrapper *self = (DaoxRandGenWrapper*) p[0];
	double random = DaoxRandGenerator_GetUniform( & self->generator );
	dao_integer min = p[1]->xInteger.value;
	dao_integer max = p[2]->xInteger.value;
	if( max < min ){
		DaoProcess_RaiseError( proc, "Param", "invalid range" );
		return;
	}
	DaoProcess_PutFloat( proc, min + (max - min + 1) * random );
}

static void GEN_GetNormal( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenWrapper *self = (DaoxRandGenWrapper*) p[0];
	double random = DaoxRandGenerator_GetNormal( & self->generator );
	double stdev = p[1]->xFloat.value;
	DaoProcess_PutFloat( proc, stdev * random );
}
static void GEN_GetNormal2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenWrapper *self = (DaoxRandGenWrapper*) p[0];
	double random = DaoxRandGenerator_GetNormal( & self->generator );
	double mean = p[1]->xFloat.value;
	double stdev = p[2]->xFloat.value;
	DaoProcess_PutFloat( proc, mean + stdev * random );
}

static DaoFuncItem DaoxRandGeneratorMeths[]=
{
	{ GEN_New,             "Generator( seed = 0 )" },

	{ GEN_GetUniform,      "GetUniform( self: Generator, max = 1.0 ) => float" },
	{ GEN_GetUniform2,     "GetUniform( self: Generator, min: float, max: float ) => float" },

	{ GEN_GetUniformInt,   "GetUniform( self: Generator, max: int ) => int" },
	{ GEN_GetUniformInt2,  "GetUniform( self: Generator, min: int, max: int ) => float" },

	{ GEN_GetUniform,      "()( self: Generator, max = 1.0 ) => float" },
	{ GEN_GetUniformInt,   "()( self: Generator, max: int ) => int" },

	{ GEN_GetNormal,       "GetNormal( self: Generator, stdev = 1.0 ) => float" },
	{ GEN_GetNormal2,      "GetNormal( self: Generator, mean: float, stdev: float ) => float" },

	{ NULL, NULL }
};

DaoTypeBase DaoxRandGenerator_Typer =
{
	"Generator", NULL, NULL, (DaoFuncItem*) DaoxRandGeneratorMeths, { NULL }, { NULL },
	(FuncPtrDel)DaoxRandGenWrapper_Delete, NULL
};




static DaoxRandGenerator* DaoProcess_GetRandCache( DaoProcess *self, uint_t seeding )
{
	void *randgen = DaoProcess_GetAuxData( self, DaoxRandGenerator_Delete );
	if( randgen == NULL ){
		randgen = DaoxRandGenerator_New( seeding ? seeding : (uint_t)time(NULL) );
		DaoProcess_SetAuxData( self, DaoxRandGenerator_Delete, randgen );
	}
	return (DaoxRandGenerator*) randgen;
}

static void RAND_Swap( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenWrapper *self = (DaoxRandGenWrapper*) p[0];
	DaoxRandGenerator *generator = DaoProcess_GetRandCache( proc, 0 );
	DaoxRandGenerator tmp = *generator;
	*generator = self->generator;
	self->generator = tmp;
}
static void RAND_SRand( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenerator *generator = DaoProcess_GetRandCache( proc, 0 );
	uint_t seed = (uint_t)p[0]->xInteger.value;
	DaoxRandGenerator_Seed( generator, seed );
}
static void RAND_Rand( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenerator *generator = DaoProcess_GetRandCache( proc, 0 );
	double max = p[0]->xFloat.value;
	DaoProcess_PutFloat( proc, max * DaoxRandGenerator_GetUniform( generator ) );
}
static void RAND_Rand2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxRandGenerator *generator = DaoProcess_GetRandCache( proc, 0 );
	double random = DaoxRandGenerator_GetUniform( generator );
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
	{ RAND_Swap,      "swap( randgen: Generator )" },
	{ RAND_SRand,     "srand( seed: int )" },
	{ RAND_Rand,      "rand( max = 1.0 ) => float" },
	{ RAND_Rand2,     "rand( max: int ) => int" },
	{ NULL, NULL }
};


DaoType *daox_type_rand_generator = NULL;

DAO_DLL_EXPORT int DaoRandom_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *randomns = DaoNamespace_GetNamespace( ns, "random" );
	daox_type_rand_generator = DaoNamespace_WrapType( randomns, & DaoxRandGenerator_Typer, 1 );
	DaoNamespace_WrapFunctions( randomns, randomMeths );
	return 0;
}
