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


#include"dao.h"

#ifndef DAO_DECIMAL_DLL
#ifdef DAO_DECIMAL
#  define DAO_DECIMAL_DLL DAO_DLL_EXPORT
#else
#  define DAO_DECIMAL_DLL DAO_DLL_IMPORT
#endif
#endif


#ifndef __DAO_MOD_DECIMAL_H__
#define __DAO_MOD_DECIMAL_H__

#include<stdlib.h>
#include<math.h>
#include"daoStdtype.h"
#include"decNumber/decQuad.h"


typedef struct DaoDecimal  DaoDecimal;


struct DaoDecimal
{
	DAO_CPOD_COMMON;

	decQuad  value;
};

#endif


DAO_API( DECIMAL, DaoType*, DaoDecimal_Type, () );

DAO_API( DECIMAL, DaoDecimal*, DaoDecimal_New, () );
DAO_API( DECIMAL, void, DaoDecimal_Delete, (DaoDecimal *self) );

DAO_API( DECIMAL, void, DaoDecimal_FromString, (DaoDecimal *self, DString *in, DaoProcess*) );
DAO_API( DECIMAL, void, DaoDecimal_ToString, (DaoDecimal *self, DString *out) );

DAO_API( DECIMAL, DaoDecimal*, DaoProcess_PutDecimal, (DaoProcess *self, double value) );

