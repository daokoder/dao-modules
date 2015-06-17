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

#ifndef __DAO_RANDOM_H__
#define __DAO_RANDOM_H__

#include<stdlib.h>
#include<math.h>
#include"daoValue.h"


#define DAO_MTCOUNT 624

typedef struct DaoxRandMT       DaoxRandMT;
typedef struct DaoxUniformRand  DaoxUniformRand;
typedef struct DaoxNormalRand   DaoxNormalRand;

/* Mersenne Twister: */
struct DaoxRandMT
{
	uint_t  states[DAO_MTCOUNT];
	uint_t  index;
};

DAO_DLL void DaoxRandMT_Seed( DaoxRandMT *self, uint_t seed );
DAO_DLL void DaoxRandMT_Generate( DaoxRandMT *self );
DAO_DLL uint_t DaoxRandMT_Extract( DaoxRandMT *self );



struct DaoxUniformRand
{
	DAO_CSTRUCT_COMMON;

	DaoxRandMT randmt;
};
DAO_DLL DaoType *daox_type_uniform_rand;



struct DaoxNormalRand
{
	DAO_CSTRUCT_COMMON;

	DaoxRandMT randmt;
	float      gset;
	int        iset;
};
DAO_DLL DaoType *daox_type_normal_rand;


#endif
