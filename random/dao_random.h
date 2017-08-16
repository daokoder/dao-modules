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

#ifndef DAO_RANDOM_DLL
#ifdef DAO_RANDOM
#  define DAO_RANDOM_DLL DAO_DLL_EXPORT
#else
#  define DAO_RANDOM_DLL DAO_DLL_IMPORT
#endif
#endif

#ifndef __DAO_RANDOM_H__
#define __DAO_RANDOM_H__

#include<stdlib.h>
#include<math.h>

typedef struct DaoRandGenerator DaoRandGenerator;
#endif

DAO_API( RANDOM, DaoRandGenerator*, DaoRandGenerator_New, (uint_t seed) );
DAO_API( RANDOM, void, DaoRandGenerator_Delete, (DaoRandGenerator *self) );
DAO_API( RANDOM, void, DaoRandGenerator_Seed, (DaoRandGenerator *self, uint_t seed) );
DAO_API( RANDOM, unsigned, DaoRandGenerator_GetUniformInt, (DaoRandGenerator *self, unsigned max) );
DAO_API( RANDOM, double, DaoRandGenerator_GetUniform, (DaoRandGenerator *self) );
DAO_API( RANDOM, double, DaoRandGenerator_GetNormal, (DaoRandGenerator *self) );

/*
%s/DAO_DLL \(.*\) \(\w\+\)( \(.*\) )/DAO_API( RANDOM, \1, \2, (\3) )/g
*/
