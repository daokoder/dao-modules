/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2015,2016, Limin Fu
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

#define DAO_DECIMAL

#include<time.h>
#include<string.h>
#include<errno.h>
#include"dao_decimal.h"
#include"daoValue.h"
#include"daoVmspace.h"


static void DaoDecContext_Delete( decContext *self )
{
	dao_free( self );
}
static decContext* DaoProcess_GetDecimalContext( DaoProcess *self )
{
	void *context = DaoProcess_GetAuxData( self, DaoDecContext_Delete );
	if( context == NULL ){
		context = (decContext*) dao_malloc( sizeof(decContext) );
		decContextDefault( context, DEC_INIT_DECQUAD );
		DaoProcess_SetAuxData( self, DaoDecContext_Delete, context );
	}
	return (decContext*) context;
}


DaoType *dao_type_decimal = NULL;

DaoType* DaoDecimal_Type()
{
	return dao_type_decimal;
}


DaoDecimal* DaoDecimal_New()
{
	DaoDecimal *self = (DaoDecimal*) DaoCstruct_New( dao_type_decimal, sizeof(DaoDecimal) );
	return self;
}
void DaoDecimal_Delete( DaoDecimal *self )
{
	DaoCstruct_Delete( (DaoCstruct*) self );
}

static void DecQuad_FromValue( decQuad *self, DaoValue *value, decContext *ctx )
{
	DString *buffer = DString_New();
	DaoValue_GetString( value, buffer );
	decQuadFromString( self, buffer->chars, ctx );
	DString_Delete( buffer );
}

void DaoDecimal_FromValue( DaoDecimal *self, DaoValue *value, DaoProcess *proc )
{
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	DecQuad_FromValue( & self->value, value, ctx );
}

void DaoDecimal_FromString(DaoDecimal *self, DString *in, DaoProcess *proc )
{
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadFromString( & self->value, in->chars, ctx );
}

void DaoDecimal_ToString(DaoDecimal *self, DString *out )
{
	DString_Reserve( out, DECQUAD_String );
	decQuadToString( & self->value, out->chars );
	DString_Reset( out, strlen( out->chars ) );
}

DaoDecimal* DaoProcess_PutDecimal( DaoProcess *self, double value )
{
	decContext *ctx = DaoProcess_GetDecimalContext( self );
	DaoCstruct *obj = DaoProcess_PutCstruct( self, dao_type_decimal );
	DaoDecimal *res = (DaoDecimal*) obj;
	char buffer[64];

	if( res == NULL ) return NULL;

	sprintf( buffer, "%g", value );
	decQuadFromString( & res->value, buffer, ctx );
	return res;
}



static void DEC_FromNumber( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDecimal *dec = DaoProcess_PutDecimal( proc, 0 );
	DString *buffer = DString_New();
	DaoValue_GetString( p[0], buffer );
	DaoDecimal_FromString( dec, buffer, proc );
	DString_Delete( buffer );
}
static void DEC_FromString( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDecimal *dec = DaoProcess_PutDecimal( proc, 0 );
	DaoDecimal_FromString( dec, p[0]->xString.value, proc );
}
static void DEC_ToString( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDecimal *self = (DaoDecimal*) p[0];
	DString *res = DaoProcess_PutChars( proc, "" );
	DaoDecimal_ToString( self, res );
}

static decQuad DEC_GetDecimal( DaoValue *value, DaoProcess *proc )
{
	decQuad quad;
	if( value->type == DAO_CSTRUCT ){
		DaoDecimal *dec = (DaoDecimal*) value;
		quad = dec->value;
	}else{
		decContext *ctx = DaoProcess_GetDecimalContext( proc );
		DecQuad_FromValue( & quad, value, ctx );
	}
	return quad;
}
static void DEC_ADD( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadAdd( & C->value, & A->value, & B, ctx );
}
static void DEC_SUB( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadSubtract( & C->value, & A->value, & B, ctx );
}
static void DEC_MUL( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadMultiply( & C->value, & A->value, & B, ctx );
}
static void DEC_DIV( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadDivide( & C->value, & A->value, & B, ctx );
}
static void DEC_MOD( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadRemainder( & C->value, & A->value, & B, ctx );
}
static void DEC_AND( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	int a = ! decQuadIsZero( & A->value );
	int b = ! decQuadIsZero( & B );
	DaoProcess_PutBoolean( proc, a && b );
}
static void DEC_OR( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	int a = ! decQuadIsZero( & A->value );
	int b = ! decQuadIsZero( & B );
	DaoProcess_PutBoolean( proc, a || b );
}
static void DEC_LT( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuad Q;
	decQuadCompare( & Q, & A->value, & B, ctx );
	DaoProcess_PutBoolean( proc, decQuadIsNegative( & Q ) );
}
static void DEC_LE( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuad Q;
	decQuadCompare( & Q, & A->value, & B, ctx );
	DaoProcess_PutBoolean( proc, ! decQuadIsPositive( & Q ) );
}
static void DEC_EQ( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuad Q;
	decQuadCompare( & Q, & A->value, & B, ctx );
	DaoProcess_PutBoolean( proc, decQuadIsZero( & Q ) );
}
static void DEC_NE( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuad Q;
	decQuadCompare( & Q, & A->value, & B, ctx );
	DaoProcess_PutBoolean( proc, ! decQuadIsZero( & Q ) );
}
static void DEC_MINUS( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decQuadCopyNegate( & C->value, & A->value );
}
static void DEC_NOT( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoProcess_PutDecimal( proc, decQuadIsZero( & A->value ) );
}
static void DEC_TILDE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadInvert( & C->value, & A->value, ctx );
}
static void DEC_BITAND( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadAnd( & C->value, & A->value, & B, ctx );
}
static void DEC_BITOR( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadOr( & C->value, & A->value, & B, ctx );
}
static void DEC_BITXOR( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuadXor( & C->value, & A->value, & B, ctx );
}
static void DEC_BITLFT( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	if( decQuadIsNegative( & B ) ){
		DaoProcess_RaiseError( proc, "Param", "Negative bit offset" );
		return;
	}
	decQuadShift( & C->value, & A->value, & B, ctx );
}
static void DEC_BITRIT( DaoProcess *proc, DaoValue *p[], int N )
{
	decQuad B = DEC_GetDecimal( p[1], proc );
	DaoDecimal *A = (DaoDecimal*) p[0];
	DaoDecimal *C = DaoProcess_PutDecimal( proc, 0 );
	decContext *ctx = DaoProcess_GetDecimalContext( proc );
	decQuad Q;

	if( decQuadIsNegative( & B ) ){
		DaoProcess_RaiseError( proc, "Param", "Negative bit offset" );
		return;
	}
	decQuadCopyNegate( & Q, & B );
	decQuadShift( & C->value, & A->value, & Q, ctx );
}

static DaoFunctionEntry daoDecimalMeths[] =
{
	{ DEC_FromNumber,  "Decimal( value = 0.0 )" },
	{ DEC_FromString,  "Decimal( value: string )" },
	{ DEC_ToString,    "(string)( invar self: Decimal )" },

	{ DEC_ADD,         "+( A: Decimal, B: Decimal|float ) => Decimal" },
	{ DEC_SUB,         "-( A: Decimal, B: Decimal|float ) => Decimal" },
	{ DEC_MUL,         "*( A: Decimal, B: Decimal|float ) => Decimal" },
	{ DEC_DIV,         "/( A: Decimal, B: Decimal|float ) => Decimal" },
	{ DEC_MOD,         "%( A: Decimal, B: Decimal|float ) => Decimal" },

	{ DEC_AND,         "&&( A: Decimal, B: Decimal|float ) => bool" },
	{ DEC_OR,          "||( A: Decimal, B: Decimal|float ) => bool" },
	{ DEC_LT,          "< ( A: Decimal, B: Decimal|float ) => bool" },
	{ DEC_LE,          "<=( A: Decimal, B: Decimal|float ) => bool" },
	{ DEC_EQ,          "==( A: Decimal, B: Decimal|float ) => bool" },
	{ DEC_NE,          "!=( A: Decimal, B: Decimal|float ) => bool" },

	{ DEC_MINUS,       "-( A: Decimal ) => Decimal" },
	{ DEC_NOT,         "!( A: Decimal ) => Decimal" },
	{ DEC_TILDE,       "~( A: Decimal ) => Decimal" },

	{ DEC_BITAND,      "&( A: Decimal, B: Decimal|float ) => Decimal" },
	{ DEC_BITOR,       "|( A: Decimal, B: Decimal|float ) => Decimal" },
	{ DEC_BITXOR,      "^( A: Decimal, B: Decimal|float ) => Decimal" },
	{ DEC_BITLFT,      "<<( A: Decimal, B: Decimal|float ) => Decimal" },
	{ DEC_BITRIT,      ">>( A: Decimal, B: Decimal|float ) => Decimal" },

	{ NULL, NULL }
};


static void DaoDecimal_Print( DaoValue *self, DaoStream *stream, DMap *cycmap, DaoProcess *proc )
{
	DaoDecimal *pod = (DaoDecimal*) self;
	char buffer[DECQUAD_String];

	decQuadToString( & pod->value, buffer );
	DaoStream_WriteChars( stream, buffer );
}

int DaoDecimal_Compare( DaoValue *self, DaoValue *other, DMap *cycmap )
{
	decContext context;
	DaoDecimal *left = (DaoDecimal*) self;
	DaoDecimal *right = (DaoDecimal*) other;
	decContext *ctx = decContextDefault( & context, DEC_INIT_DECQUAD );
	decQuad Q;
	decQuadCompare( & Q, & left->value, & right->value, ctx );
	return decQuadToInt32( & Q, ctx, DEC_ROUND_HALF_UP );
}

size_t DaoDecimal_Hash( DaoValue *self )
{
	DaoDecimal *pod = (DaoDecimal*) self;
	return Dao_Hash( & pod->value, sizeof(pod->value), 0 );
}

DaoValue* DaoDecimal_Copy( DaoValue *self, DaoValue *target )
{
	DaoDecimal *src = (DaoDecimal*) self;
	DaoDecimal *dest = (DaoDecimal*) target;
	if( target ){
		if( src ) dest->value = src->value;
		return target;
	}
	dest = DaoDecimal_New();
	if( src ) dest->value = src->value;
	return (DaoValue*) dest;
}


/* TODO: Unary, Binary and Conversion; */
static DaoTypeCore daoDecimalCore =
{
	"Decimal",                                             /* name */
	sizeof(DaoDecimal),                                    /* size */
	{ NULL },                                              /* bases */
	{ NULL },                                              /* casts */
	NULL,                                                  /* numbers */
	daoDecimalMeths,                                       /* methods */
	DaoCstruct_CheckGetField,    DaoCstruct_DoGetField,    /* GetField */
	DaoCstruct_CheckSetField,    DaoCstruct_DoSetField,    /* SetField */
	DaoCstruct_CheckGetItem,     DaoCstruct_DoGetItem,     /* GetItem */
	DaoCstruct_CheckSetItem,     DaoCstruct_DoSetItem,     /* SetItem */
	DaoCstruct_CheckUnary,       DaoCstruct_DoUnary,       /* Unary */
	DaoCstruct_CheckBinary,      DaoCstruct_DoBinary,      /* Binary */
	DaoCstruct_CheckConversion,  DaoCstruct_DoConversion,  /* Conversion */
	NULL,                        NULL,                     /* ForEach */
	DaoDecimal_Print,                                      /* Print */
	NULL,                                                  /* Slice */
	DaoDecimal_Compare,                                    /* Compare */
	DaoCstruct_HashPOD,                                    /* Hash */
	DaoCstruct_CreatePOD,                                  /* Create */
	DaoCstruct_CopyPOD,                                    /* Copy */
	(DaoDeleteFunction) DaoCstruct_Delete,                 /* Delete */
	NULL                                                   /* HandleGC */
};



#undef DAO_DECIMAL
#undef DAO_DECIMAL_DLL
#define DAO_HAS_DECIMAL
#include"dao_api.h"

DAO_DLL_EXPORT int DaoDecimal_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *mathns = DaoVmSpace_GetNamespace( vmSpace, "math" );
	DaoNamespace_AddConstValue( ns, "math", (DaoValue*) mathns );

	decContextTestEndian(0);

	dao_type_decimal = DaoNamespace_WrapType( mathns, & daoDecimalCore, DAO_CSTRUCT, 0 );

#define DAO_API_INIT
#include"dao_api.h"
	return 0;
}
