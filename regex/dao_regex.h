/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2014, Limin Fu
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

#ifndef __DAO_REGEX_H__
#define __DAO_REGEX_H__

#include"dao.h"
#include"daoValue.h"
#include"daoStdtype.h"
#include"daoProcess.h"


#include "oniguruma.h"

enum {
	Regws_Strict = 1,
	Regws_Free,
	Regws_Implied
};

typedef struct DaoOnigRegex DaoOnigRegex;
typedef struct DaoOnigMatch DaoOnigMatch;
typedef struct DaoOnigIter DaoOnigIter;

struct DaoOnigRegex {
	regex_t *regex;
	DString *pattern;
	int icase;
};

struct DaoOnigMatch {
	OnigRegion *match;
	DString *target;
	DaoOnigRegex *regex;
	DaoValue *regval;
};

struct DaoOnigIter {
	DString *target;
	DaoOnigRegex *regex;
	DaoValue *regval;
	daoint start, end;
	OnigRegion *match;
};

#endif
